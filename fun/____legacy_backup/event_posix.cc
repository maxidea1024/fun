#include "fun/event_posix.h"
#include <time.h>
#include <sys/time.h>

//
// Note: pthread_cond_timedwait() with CLOCK_MONOTONIC is supported
// on Linux and QNX, as well as on Android >= 5.0. On Android < 5.0,
// HAVE_PTHREAD_COND_TIMEDWAIT_MONOTONIC is defined to indicate
// availability of non-standard pthread_cond_timedwait_monotonic().
//
#ifndef FUN_HAVE_MONOTONIC_PTHREAD_COND_TIMEDWAIT
  #if (defined(__linux__) || defined(__QNX__)) && !(defined(__ANDROID__) && defined(HAVE_PTHREAD_COND_TIMEDWAIT_MONOTONIC))
    #define FUN_HAVE_MONOTONIC_PTHREAD_COND_TIMEDWAIT 1
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

EventImpl::EventImpl(EventResetType type)
  : auto_reset_(type == EventResetType::Auto)
  , state_(false)
{
  if (pthread_mutex_init(&mutex_, NULL)) {
    throw SystemException("cannot create event (mutex)");
  }

#if defined(FUN_HAVE_MONOTONIC_PTHREAD_COND_TIMEDWAIT)
  pthread_condattr_t attr;
  if (pthread_condattr_init(&attr)) {
    pthread_mutex_destroy(&mutex_);
    throw SystemException("cannot create event (condition attribute)");
  }

  if (pthread_condattr_setclock(&attr, CLOCK_MONOTONIC)) {
    pthread_condattr_destroy(&attr);
    pthread_mutex_destroy(&mutex_);
    throw SystemException("cannot create event (condition attribute clock)");
  }

  if (pthread_cond_init(&cond_, &attr)) {
    pthread_condattr_destroy(&attr);
    pthread_mutex_destroy(&mutex_);
    throw SystemException("cannot create event (condition)");
  }

  pthread_condattr_destroy(&attr);
#else
  if (pthread_cond_init(&cond_, NULL)) {
    pthread_mutex_destroy(&mutex_);
    throw SystemException("cannot create event (condition)");
  }
#endif
}

EventImpl::~EventImpl()
{
  pthread_cond_destroy(&cond_);
  pthread_mutex_destroy(&mutex_);
}

void EventImpl::WaitImpl()
{
  if (pthread_mutex_lock(&mutex_)) {
    throw SystemException("wait for event failed (lock)");
  }

  while (!state_) {
    if (pthread_cond_wait(&cond_, &mutex_)) {
      pthread_mutex_unlock(&mutex_);
      throw SystemException("wait for event failed");
    }
  }

  if (auto_reset_) {
    state_ = false;
  }

  pthread_mutex_unlock(&mutex_);
}

bool EventImpl::WaitImpl(int32 milliseconds)
{
  int rc = 0;
  struct timespec abs_time;

#if defined(__VMS)
  struct timespec delta;
  delta.tv_sec = milliseconds / 1000;
  delta.tv_nsec = (milliseconds % 1000)*1000000;
  pthread_get_expiration_np(&delta, &abs_time);
#elif defined(FUN_HAVE_MONOTONIC_PTHREAD_COND_TIMEDWAIT)
  clock_gettime(CLOCK_MONOTONIC, &abs_time);
  abs_time.tv_sec += milliseconds / 1000;
  abs_time.tv_nsec += (milliseconds % 1000)*1000000;
  if (abs_time.tv_nsec >= 1000000000) {
    abs_time.tv_nsec -= 1000000000;
    abs_time.tv_sec++;
  }
#elif defined(FUN_HAVE_CLOCK_GETTIME)
  clock_gettime(CLOCK_REALTIME, &abs_time);
  abs_time.tv_sec += milliseconds / 1000;
  abs_time.tv_nsec += (milliseconds % 1000)*1000000;
  if (abs_time.tv_nsec >= 1000000000) {
    abs_time.tv_nsec -= 1000000000;
    abs_time.tv_sec++;
  }
#else
  struct timeval tv;
  gettimeofday(&tv, NULL);
  abs_time.tv_sec = tv.tv_sec + milliseconds / 1000;
  abs_time.tv_nsec = tv.tv_usec*1000 + (milliseconds % 1000)*1000000;
  if (abs_time.tv_nsec >= 1000000000) {
    abs_time.tv_nsec -= 1000000000;
    abs_time.tv_sec++;
  }
#endif

  if (pthread_mutex_lock(&mutex_) != 0) {
    throw SystemException("wait for event failed (lock)");
  }

  while (!state_) {
    if ((rc = pthread_cond_timedwait(&cond_, &mutex_, &abs_time))) {
      if (rc == ETIMEDOUT) {
        break;
      }

      pthread_mutex_unlock(&mutex_);
      throw SystemException("cannot wait for event");
    }
  }

  if (rc == 0 && auto_reset_) {
    state_ = false;
  }

  pthread_mutex_unlock(&mutex_);
  return rc == 0;
}

} // namespace fun
