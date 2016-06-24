// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
#ifndef CEPH_RGW_REDIS_H
#define CEPH_RGW_REDIS_H

#include <string>
#include "rgw_common.h"
#include <hiredis/hiredis.h>

using namespace std;

class RedisClient {
  string url;
  uint port;
  redisContext *rct;
  uint timeout;
public:
  RedisClient(string &_url) : url(_url), port(6379), rct(NULL), timeout(3) {}
  ~RedisClient();
  bool connect();
  void disconnect();
  bool push(string &cmd);
  bool flush();
};

#endif
