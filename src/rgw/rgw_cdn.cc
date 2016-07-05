// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab
/*
 *
 * Copyright (C) 2016 eayun <contact@eayun.com>
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software
 * Foundation.  See file COPYING.
 *
 */

using namespace std;

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>

#include "rgw_cdn.h"
#include "rgw_redis.h"

#define dout_subsys ceph_subsys_rgw

static CDNPublisher *cdn_publisher = NULL;

void CDNPublisher::enqueue(CDNMessage *msg)
{
  // TODO: DEBUG
  lock.Lock();
  msg_queue.push(msg);
  cond.Signal();
  dout(1) << "push msg(" << msg->key << ") to queue" << dendl;
  lock.Unlock();
}

// Do not need to lock/unlock, since we have only on lock for msg_queue 
// and thread signal and caller has lock it before calling
CDNMessage *CDNPublisher::dequeue()
{
  // TODO: DEBUG
  if (msg_queue.empty()) {
    return NULL;
  }
  CDNMessage *msg = msg_queue.front();
  msg_queue.pop();
  return msg;
}

bool CDNPublisher::going_down()
{
  return (down_flag.read() != 0);
}

static string parse_redis_url()
{
  // FIXME: hack filepath
  string conf = "/etc/ceph/eayunobs.conf";
  string redis_url_key = "redis.url";

  try {
    boost::property_tree::ptree pt;
    boost::property_tree::ini_parser::read_ini(conf, pt);
    return pt.get<std::string>(redis_url_key);
  } catch (std::exception const &e) {
    dout(1) << "ERROR: failed to parse redis_url: " << e.what() << dendl;
    return "";
  }
}

void *CDNPublisher::PublisherWorker::entry()
{
  // TODO: url from config
  RedisClient *rclient = NULL;
  CDNMessage *msg;
  Mutex& lock = publisher->lock;
  Cond& cond = publisher->cond;

  string url = parse_redis_url();
  if (url.empty()) {
    return NULL;
  }
  rclient = new RedisClient(url);
  bool ret = rclient->connect();
  if (!ret) {
    // TODO: log error
    return NULL;
  }
  while(!cdn_publisher->going_down()) {
    lock.Lock();
    msg = publisher->dequeue();
    lock.Unlock();
    if (msg) {
      // TODO: flush every 1000 msg?
      // TODO: if push failed?
      // TODO: wrap fun in struct CDNMessage
      dout(3) << "push msg(" << msg->key << ") to redis server" << dendl;
      rclient->push("HMSET %s type %s op %s time %ld", msg->key.c_str(), msg->type.c_str(), msg->op.c_str(), msg->time);
      delete msg;
      msg = NULL;
      continue;
    }
    dout(3) << "begin flush msgs" << dendl;
    rclient->flush();
    dout(3) << "end flush msgs" << dendl;
    lock.Lock();
    cond.WaitInterval(cct, lock, utime_t(2, 0));
    lock.Unlock();
  }

  // TODO: log info
  rclient->disconnect();
  delete rclient;
  return NULL;
}

void rgw_cdn_init(CephContext *cct)
{
  cdn_publisher = new CDNPublisher(cct);
}

void rgw_cdn_finalize()
{
  delete cdn_publisher;
  cdn_publisher = NULL;
}

static bool op_need_publish(const string& op_name)
{
  // TODO: COPY obj ?
  if (op_name == "put_obj" ||
      op_name == "complete_multipart" ||
      op_name == "delete_obj" ||
      op_name == "delete_bucket" ||
      op_name == "copy_obj")  {
    return true;
  } else {
    return false;
  }
}

static void set_msg(struct req_state *s, const string& op_name, CDNMessage *msg)
{
  if (op_name == "put_obj" ||
      op_name == "complete_multipart" ||
      op_name == "delete_obj" ||
      op_name == "copy_obj") {
    msg->key = s->bucket.name;
    msg->key.append(":");
    msg->key.append(s->object_str);
    msg->type = "obj";
    if (op_name == "delete_obj") {
      msg->op = "DELETE";
    } else {
      msg->op = "PUT";
    }
  } else if (op_name == "delete_bucket") {
    msg->key = s->bucket.name;
    msg->type = "blk";
    msg->op = "DELETE";
  }

  msg->time = time(NULL);
}

void rgw_cdn_publish(struct req_state *s, const string& op_name)
{
  if (op_need_publish(op_name)) {
    // TODO: warp msg
    CDNMessage *msg = new CDNMessage;
    set_msg(s, op_name, msg);
    cdn_publisher->enqueue(msg);
  }
}
