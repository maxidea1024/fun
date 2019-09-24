#include "fun/base/thread_std.h"
#include "fun/base/thread.h"
#include "fun/base/exception.h"
#include "fun/base/error_handler.h"
//#include "fun/base/format.h"

#if FUN_PLATFORM_WINDOWS_FAMILY
#include "fun/base/thread_std_win32.cc"
#elif FUN_PLATFORM == FUN_PLATFORM_VXWORKS
#include "fun/base/thread_std_vx.cc"
#else
#include "fun/base/thread_std_posix.cc"
#endif

namespace fun {

ThreadImpl::CurrentThreadHolder ThreadImpl::current_thread_holder_;
thread_local ThreadImpl* ThreadImpl::CurrentThreadHolder::thread_;

ThreadImpl::ThreadImpl() : data_(new ThreadData) {}

ThreadImpl::~ThreadImpl() {
  if (data_->started && !data_->joined && data_->thread) {
    data_->thread->detach();
  }
}

void ThreadImpl::StartImpl(SharedPtr<Runnable> target) {
  if (data_->runnable_target) {
    throw SystemException("thread already running");
  }

  data_->runnable_target = target;

  try {
    data_->thread = UniquePtr<std::thread>(new std::thread(RunnableEntry, this));
  } catch (std::system_error& e) {
    data_->runnable_target = nullptr;
    throw SystemException(String::Format("cannot start thread: %s", String(e.what())));
  }

  data_->tid = data_->thread->native_handle();

  data_->started = true;
}

void ThreadImpl::JoinImpl() {
  if (!data_->started) {
    return;
  }

  data_->done.Wait();
  try {
    data_->thread->join();
  } catch (std::system_error& e) {
    throw SystemException(String::Format("cannot join thread: %s", String(e.what())));
  }

  data_->joined = true;
}

bool ThreadImpl::JoinImpl(int32 milliseconds) {
  if (data_->started && data_->done.TryWait(milliseconds)) {
    try {
      data_->thread->join();
    } catch (std::system_error& e) {
      throw SystemException(String::Format("cannot join thread: %s", String(e.what())));
    }
    data_->joined = true;
    return true;
  } else if (data_->started) {
    return false;
  } else {
    return true;
  }
}

ThreadImpl* ThreadImpl::CurrentImpl() {
  return current_thread_holder_.Get();
}

ThreadImpl::TIDImpl ThreadImpl::CurrentTidImpl() {
  ThreadImpl* thread_impl = CurrentImpl();
  if (!thread_impl) {
    return 0;
  }

  return thread_impl->GetTidImpl();
  //return std::this_thread::get_id();
}

void ThreadImpl::SleepImpl(int32 milliseconds) {
  std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

void* ThreadImpl::RunnableEntry(void* thread) {
  current_thread_holder_.Set(reinterpret_cast<ThreadImpl*>(thread));

  ThreadImpl* thread_impl = reinterpret_cast<ThreadImpl*>(thread);

  RefCountedPtr<ThreadData> data = thread_impl->data_;

  try {
    data->runnable_target->Run();
  } catch (Exception& e) {
    ErrorHandler::Handle(e);
  } catch (std::exception& e) {
    ErrorHandler::Handle(e);
  } catch (...) {
    ErrorHandler::Handle();
  }

  data->runnable_target = nullptr;
  data->done.Set();
  return 0;
}

} // namespace fun
