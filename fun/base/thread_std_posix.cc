#include "fun/base/thread_std.h"
#include "fun/base/thread.h"
#include "fun/base/exception.h"

#include <pthread.h>
#include <signal.h>

#if defined(__sun) && defined(__SVR4)
  #if !defined(__EXTENSIONS__)
    #define __EXTENSIONS__
  #endif
#endif

#if FUN_PLATFORM == FUN_PLATFORM_LINUX || FUN_PLATFORM == FUN_PLATFORM_ANDROID || FUN_PLATFORM == FUN_PLATFORM_MAC_OS_X || FUN_PLATFORM == FUN_PLATFORM_QNX
  #include <time.h>
  #include <unistd.h>
#endif

#if FUN_PLATFORM == FUN_PLATFORM_MAC_OS_X
  #include <mach/mach.h>
  #include <mach/task.h>
  #include <mach/thread_policy.h>
#endif

#if FUN_PLATFORM == FUN_PLATFORM_LINUX || FUN_PLATFORM == FUN_PLATFORM_ANDROID
  #include <sys/syscall.h>
#endif

#include <cstring>

namespace fun {

int MapPrio(int prio, int policy) {
  int pmin = ThreadImpl::GetMinOsPriorityImpl(policy);
  int pmax = ThreadImpl::GetMaxOsPriorityImpl(policy);

  switch (prio) {
    case ThreadImpl::PRIO_LOWEST_IMPL:
      return pmin;
    case ThreadImpl::PRIO_LOW_IMPL:
      return pmin + (pmax - pmin) / 4;
    case ThreadImpl::PRIO_NORMAL_IMPL:
      return pmin + (pmax - pmin) / 2;
    case ThreadImpl::PRIO_HIGH_IMPL:
      return pmin + 3 * (pmax - pmin) / 4;
    case ThreadImpl::PRIO_HIGHEST_IMPL:
      return pmax;
    default:
      fun_bugcheck_msg("invalid thread priority");
  }
  return -1; // just to satisfy compiler - we'll never get here anyway
}

int ReverseMapPrio(int prio, int policy) {
  if (policy == SCHED_OTHER) {
    int pmin = ThreadImpl::GetMinOsPriorityImpl(policy);
    int pmax = ThreadImpl::GetMaxOsPriorityImpl(policy);
    int normal = pmin + (pmax - pmin) / 2;
    if (prio == pmax) {
      return ThreadImpl::PRIO_HIGHEST_IMPL;
    }

    if (prio > normal) {
      return ThreadImpl::PRIO_HIGH_IMPL;
    } else if (prio == normal) {
      return ThreadImpl::PRIO_NORMAL_IMPL;
    } else if (prio > pmin) {
      return ThreadImpl::PRIO_LOW_IMPL;
    } else {
      return ThreadImpl::PRIO_LOWEST_IMPL;
    }
  } else {
    return ThreadImpl::PRIO_HIGHEST_IMPL;
  }
}

void ThreadImpl::SetPriorityImpl(int prio) {
  if (prio != data_->prio) {
    data_->prio = prio;
    data_->policy = SCHED_OTHER;

    if (IsRunningImpl()) {
      struct sched_param par;
      par.sched_priority = MapPrio(data_->prio, SCHED_OTHER);
      if (pthread_setschedparam(data_->thread->native_handle(), SCHED_OTHER, &par)) {
        throw SystemException("cannot set thread priority");
      }
    }
  }
}

void ThreadImpl::SetOsPriorityImpl(int prio, int policy) {
  if (prio != data_->os_prio || policy != data_->policy) {
    if (data_->runnable_target) {
      struct sched_param par;
      par.sched_priority = (policy == SCHED_OTHER) ? 0 : prio;
      if (pthread_setschedparam(data_->thread->native_handle(), policy, &par)) {
        throw SystemException("cannot set thread priority");
      }
    }
    data_->prio = ReverseMapPrio(prio, policy);
    data_->os_prio = prio;
    data_->policy = policy;
  }
}

int ThreadImpl::GetMinOsPriorityImpl(int policy) {
#if defined(FUN_THREAD_PRIORITY_MIN)
  return FUN_THREAD_PRIORITY_MIN;
#elif defined(__digital__)
  return PRI_OTHER_MIN;
#else
  return sched_get_priority_min(policy);
#endif
}

int ThreadImpl::GetMaxOsPriorityImpl(int policy) {
#if defined(FUN_THREAD_PRIORITY_MAX)
  return FUN_THREAD_PRIORITY_MAX;
#elif defined(__digital__)
  return PRI_OTHER_MAX;
#else
  return sched_get_priority_max(policy);
#endif
}

void ThreadImpl::SetStackSizeImpl(int size) {
  data_->stack_size = size;
  // not supported
}

void ThreadImpl::SetAffinityImpl(int cpu) {
#if FUN_PLATFORM_UNIX_FAMILY && FUN_PLATFORM != FUN_PLATFORM_MAC_OS_X && FUN_PLATFORM != FUN_PLATFORM_FREE_BSD
#ifdef HAVE_PTHREAD_SETAFFINITY_NP
  cpu_set_t cpu_set;
  CPU_ZERO(&cpu_set);
  CPU_SET(cpu, &cpu_set);
#ifdef HAVE_THREE_PARAM_SCHED_SETAFFINITY
  if (pthread_setaffinity_np(data_->thread->native_handle(), sizeof(cpu_set), &cpu_set) != 0) {
    throw SystemException("Failed to set affinity");
  }
#else
  if (pthread_setaffinity_np(data_->thread->native_handle(), &cpu_set) != 0) {
    throw SystemException("Failed to set affinity");
  }
#endif
#endif
#endif // defined unix & !defined mac os x

#if FUN_PLATFORM == FUN_PLATFORM_MAC_OS_X
  kern_return_t ret;
  thread_affinity_policy policy;
  policy.affinity_tag = cpu;

  ret = thread_policy_set(pthread_mach_thread_np(data_->thread->native_handle()),
                          THREAD_AFFINITY_POLICY,
                          (thread_policy_t)&policy,
                          THREAD_AFFINITY_POLICY_COUNT);
  if (ret != KERN_SUCCESS) {
    throw SystemException("Failed to set affinity");
  }
#endif
  YieldImpl();
  data_->cpu = cpu;
}

int ThreadImpl::GetAffinityImpl() const {
  int ret = -1;
#if FUN_PLATFORM_UNIX_FAMILY && FUN_PLATFORM != FUN_PLATFORM_MAC_OS_X && FUN_PLATFORM != FUN_PLATFORM_FREE_BSD
#ifdef HAVE_PTHREAD_SETAFFINITY_NP
  cpu_set_t cpu_set;
  CPU_ZERO(&cpu_set);
#ifdef HAVE_THREE_PARAM_SCHED_SETAFFINITY
  if (pthread_getaffinity_np(data_->thread->native_handle(), sizeof(cpu_set), &cpu_set) != 0) {
    throw SystemException("Failed to get affinity", errno);
  }
#else
  if (pthread_getaffinity_np(data_->thread->native_handle(), &cpu_set) != 0) {
    throw SystemException("Failed to get affinity", errno);
  }
#endif
  int cpu_count = Environment::GetProcessorCount();
  for (int i = 0; i < cpu_count; i++) {
    if (CPU_ISSET(i, &cpu_set)) {
      ret = i;
      break;
    }
  }
#endif
#endif // defined unix & !defined mac os x

#if FUN_PLATFORM == FUN_PLATFORM_MAC_OS_X
  kern_return_t ret;
  thread_affinity_policy policy;
  mach_msg_type_number_t count = THREAD_AFFINITY_POLICY_COUNT;
  boolean_t get_default = false;
  ret = thread_policy_get(pthread_mach_thread_np(data_->thread->native_handle()),
                          THREAD_AFFINITY_POLICY,
                          (thread_policy_t)&policy,
                          &count,
                          &get_default);
  if (ret != KERN_SUCCESS) {
    throw SystemException("Failed to get affinity", errno);
  }
  ret = policy.affinity_tag;
  int cpu_count = Environment::GetProcessorCount();
  if (ret >= cpu_count) {
    ret = -1;
  }

#endif
  return ret;
}

} // namespace fun
