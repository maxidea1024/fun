#include "fun/base/thread_std.h"
#include "fun/base/thread.h"
#include "fun/base/exception.h"

//TODO 제거하자 이미 포함되어 있음..
#if FUN_PLATFORM_WINDOWS_FAMILY
#include "fun/base/windows_less.h"
#endif

namespace fun {

void ThreadImpl::SetPriorityImpl(int prio) {
  HANDLE handle = reinterpret_cast<HANDLE>(data_->thread->native_handle());
  if (prio != data_->prio) {
    data_->prio = prio;
    data_->policy = 0;

    if (data_->started && !data_->joined && data_->thread) {
      if (SetThreadPriority(handle, data_->prio) == 0) {
        throw SystemException("cannot set thread priority");
      }
    }
  }
}

void ThreadImpl::SetOsPriorityImpl(int prio, int /*policy*/) {
  SetPriorityImpl(prio);
}

int ThreadImpl::GetMinOsPriorityImpl(int /*policy*/) {
  return PRIO_LOWEST_IMPL;
}

int ThreadImpl::GetMaxOsPriorityImpl(int /*policy*/) {
  return PRIO_HIGHEST_IMPL;
}

void ThreadImpl::SetStackSizeImpl(int size) {
  data_->stack_size = size;
  // not supported
}

void ThreadImpl::SetAffinityImpl(int cpu) {
  HANDLE handle = reinterpret_cast<HANDLE>(data_->thread->native_handle());
  DWORD mask = 1;
  mask <<= cpu;
  if (data_->started && !data_->joined && data_->thread) {
    if (SetThreadAffinityMask(handle, mask) == 0) {
      throw SystemException("Failed to set affinity");
    }
  }
  data_->cpu = cpu;
}

int ThreadImpl::GetAffinityImpl() const {
  return data_->cpu;
}

} // namespace fun
