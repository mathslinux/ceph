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
}

void RedisClient::disconnect()
{
}

RedisClient::~RedisClient()
{
}

bool RedisClient::push(string &cmd)
{
}

bool RedisClient::flush()
{
}
