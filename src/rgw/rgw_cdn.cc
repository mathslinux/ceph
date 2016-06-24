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

static CDNPublisher *cdn_publisher = NULL;

void CDNPublisher::enqueue(CDNMessage &msg)
{
}

bool CDNPublisher::going_down()
{
  return (down_flag.read() != 0);
}

void *CDNPublisher::PublisherWorker::entry()
{
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
  return true;
}

void rgw_cdn_publish(struct req_state *s, const string& op_name)
{
}
