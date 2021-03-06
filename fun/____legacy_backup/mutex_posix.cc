﻿#include "fun/base/mutex_posix.h"
#include "fun/base/timestamp.h"

#if !defined(FUN_NO_SYS_SELECT_H)
#include <sys/select.h>
#endif

#include <unistd.h>

#if FUN_PLATFORM == FUN_PLATFORM_VXWORKS
#include <timers.h>
#include <cstring>
#else
#include <sys/time.h>
#endif

#if defined(_POSIX_TIMEOUTS) && (_POSIX_TIMEOUTS - 200112L) >= 0L
  #if defined(_POSIX_THREADS) && (_POSIX_THREADS - 200112L) >= 0L
    #define FUN_HAVE_MUTEX_TIMEOUT
  #endif
#endif

#ifndef FUN_HAVE_CLOCK_GETTIME
  #if (defined(_POSIX_TIMERS) && defined(CLOCK_REALTIME)) || defined(FUN_PLATFORM_VXWORKS) || defined(__QNX__)
    #ifndef __APPLE__ // See GitHub issue #1453 - not available before Mac OS 10.12/iOS 10
      #define FUN_HAVE_CLOCK_GETTIME
    #endif
  #endif
#endif

namespace fun {

MutexImpl::MutexImpl(MutexTypeImpl type)
{
#if defined(FUN_PLATFORM_VXWORKS)
  // This workaround is for VxWorks 5.x where
  // pthread_mutex_init() won't properly initialize the mutex
  // resulting in a subsequent freeze in pthread_mutex_destroy()
  // if the mutex has never been used.
  std::memset(&mutex_, 0, sizeof(mutex_));
#endif
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
#if defined(PTHREAD_MUTEX_RECURSIVE_NP)
  pthread_mutexattr_settype_np(&attr, type == MUTEX_RECURSIVE_IMPL ? PTHREAD_MUTEX_RECURSIVE_NP : PTHREAD_MUTEX_NORMAL_NP);
#elif (FUN_PLATFORM != FUN_PLATFORM_VXWORKS)
  pthread_mutexattr_settype(&attr, type == MUTEX_RECURSIVE_IMPL ? PTHREAD_MUTEX_RECURSIVE : PTHREAD_MUTEX_NORMAL);
#endif
  if (pthread_mutex_init(&mutex_, &attr)) {
    pthread_mutexattr_destroy(&attr);
    throw SystemException("cannot create mutex");
  }
  pthread_mutexattr_destroy(&attr);
}

MutexImpl::~MutexImpl()
{
  pthread_mutex_destroy(&mutex_);
}

bool MutexImpl::TryLockImpl(int32 milliseconds)
{
#if defined(FUN_HAVE_MUTEX_TIMEOUT)
  struct timespec abstime;
#if defined(FUN_HAVE_CLOCK_GETTIME)
  clock_gettime(CLOCK_REALTIME, &abstime);
  abstime.tv_sec  += milliseconds / 1000;
  abstime.tv_nsec += (milliseconds % 1000)*1000000;
  if (abstime.tv_nsec >= 1000000000)
  {
    abstime.tv_nsec -= 1000000000;
    abstime.tv_sec++;
  }
#else
  struct timeval tv;
  gettimeofday(&tv, NULL);
  abstime.tv_sec  = tv.tv_sec + milliseconds / 1000;
  abstime.tv_nsec = tv.tv_usec*1000 + (milliseconds % 1000)*1000000;
  if (abstime.tv_nsec >= 1000000000) {
    abstime.tv_nsec -= 1000000000;
    abstime.tv_sec++;
  }
#endif
  int rc = pthread_mutex_timedlock(&mutex_, &abstime);
  if (rc == 0) {
    return true;
  }
  else if (rc == ETIMEDOUT) {
    return false;
  }
  else {
    throw SystemException("cannot Lock mutex");
  }
#else
  const int sleep_msecs = 5;
  Timestamp now(Timestamp::Now());
  Timestamp::TimeDiff diff(Timestamp::TimeDiff(milliseconds)*1000);
  do {
    int rc = pthread_mutex_trylock(&mutex_);
    if (rc == 0) {
      return true;
    }
    else if (rc != EBUSY) {
      throw SystemException("cannot Lock mutex");
    }
#if defined(FUN_PLATFORM_VXWORKS)
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = sleep_msecs*1000000;
    nanosleep(&ts, NULL);

#else
    struct timeval tv;
    tv.tv_sec  = 0;
    tv.tv_usec = sleep_msecs * 1000;
    select(0, NULL, NULL, NULL, &tv);
#endif
  }
  while (!now.IsElapsed(diff));
  return false;
#endif
}

FastMutexImpl::FastMutexImpl()
  : MutexImpl(MUTEX_NONRECURSIVE_IMPL)
{
}

FastMutexImpl::~FastMutexImpl()
{
}

} // namespace fun
