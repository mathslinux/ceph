// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
#ifndef CEPH_RGW_CDN_H
#define CEPH_RGW_CDN_H

#include <string>
#include <queue>
#include "common/Mutex.h"
#include "common/Cond.h"
#include "common/Thread.h"
#include "rgw_common.h"

using namespace std;

class CDNPublisher
{
  CephContext *cct;
  Mutex lock;
  Cond cond;
  atomic_t down_flag;
  std::queue<CDNMessage> msg_queue;	// use map instead?
  class PublisherWorker : public Thread {
    CephContext *cct;
    CDNPublisher *publisher;
  public:
    PublisherWorker(CephContext *_cct, CDNPublisher *_publisher)
      : cct(_cct), publisher(_publisher) {}
    void *entry();
  };

  PublisherWorker *worker;

public:
  CDNPublisher(CephContext *_cct) : cct(_cct), lock("CDNWorker") {
    worker = new PublisherWorker(cct, this);
    worker->create();
  }

  ~CDNPublisher() {
    down_flag.set(1);
    worker->join();
    delete worker;
  }
  void enqueue(CDNMessage &msg);
  bool going_down();
};

void rgw_cdn_init(CephContext *cct);
void rgw_cdn_finalize();
void rgw_cdn_publish(struct req_state *s, const string& op_name);

#endif
