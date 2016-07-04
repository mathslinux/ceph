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

#include "rgw_redis.h"

#define dout_subsys ceph_subsys_rgw

bool RedisClient::connect()
{
  // TODO: disconnect ?
  struct timeval tv = {timeout, 0};
  rct = redisConnectWithTimeout(url.c_str(), port, tv);
  if ( (NULL == rct) || (rct->err) ) {
    if (rct) {
      dout(0) << "ERROR: failed to connect redis server: " << rct->errstr << dendl;
    }
    else {
      dout(0) << "ERROR: failed to alloc redis contect" << dendl;
    }
    return false;
  }
  return true;
}

void RedisClient::disconnect()
{
}

RedisClient::~RedisClient()
{
}

bool RedisClient::push(string &cmd)
{
  // if failed to append, cmd_len++?
  cmd_len++;
  if (redisAppendCommand(rct, cmd.c_str()) == REDIS_OK) {
    return true;
  } else {
    // TODO: log error
    // TODO: reconnection?
    return false;
  }
}

bool RedisClient::push(const char *fmt, ...)
{
  va_list args;
  int ret;
  va_start(args, fmt);
  ret = redisvAppendCommand(rct, fmt, args);
  va_end(args);
  // if failed to append, cmd_len++?
  cmd_len++;
  if (ret == REDIS_OK) {
    return true;
  } else {
    // TODO: log error
    // TODO: reconnection?
    return false;
  }
}

void RedisClient::flush()
{
  redisReply* reply = NULL;
  while (cmd_len--) {
    if (redisGetReply(rct, (void **)&reply) == REDIS_OK) {
      if (reply) {
	freeReplyObject(reply);
	reply = NULL;
      }
    } else {
      // TODO: log error
    }
  }
}
