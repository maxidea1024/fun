#pragma once

#include "fun/net/net.h"

namespace fun {
namespace net {

class IThreadPoolCallbacks {
 public:
  virtual ~IThreadPoolCallbacks() {}

  virtual void OnThreadBegin() = 0;
  virtual void OnThreadEnd() = 0;
};

class ThreadPool2 {
 public:
  FUN_NETX_API static ThreadPool2* New();

  virtual ~ThreadPool2() {}

  virtual void SetCallbacks(IThreadPoolCallbacks* callback) = 0;
  virtual void Start(int32 desired_thread_count) = 0;
  virtual void Stop() = 0;
};

}  // namespace net
}  // namespace fun
