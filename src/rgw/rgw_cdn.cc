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

CDNMessage *CDNPublisher::dequeue()
{
  // TODO: DEBUG
  lock.Lock();
  if (msg_queue.empty()) {
    lock.Unlock();
    return NULL;
  }
  CDNMessage *msg = msg_queue.front();
  msg_queue.pop();
  lock.Unlock();
  return msg;
}

bool CDNPublisher::going_down()
{
  return (down_flag.read() != 0);
}

void *CDNPublisher::PublisherWorker::entry()
{
  // TODO: url from config
  string url = "x.x.x.x";
  RedisClient *rclient = new RedisClient(url);
  CDNMessage *msg;
  Mutex& lock = publisher->lock;
  Cond& cond = publisher->cond;

  bool ret = rclient->connect();
  if (!ret) {
    // TODO: log error
    return NULL;
  }
  lock.Lock();
  while(!cdn_publisher->going_down()) {
    msg = publisher->dequeue();
    lock.Unlock();
    if (msg) {
      // TODO: flush every 1000 msg?
      // TODO: if push failed?
      // TODO: wrap fun in struct CDNMessage
      rclient->push("HMSET %s type:%s op:%s time:%ld", msg->key.c_str(), msg->type.c_str(), msg->op.c_str(), msg->time);
      delete msg;
      msg = NULL;
      continue;
    }
    rclient->flush();
    lock.Lock();
    cond.WaitInterval(cct, lock, utime_t(2, 0));
  }
  lock.Unlock();

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
