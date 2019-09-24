#include "fun/base/async_dispatcher.h"
#include "fun/base/notification.h"

namespace fun {

namespace {

class MethodNotification : public Notification {
 public:
  MethodNotification(AsyncRunnableBase::Ptr runnable) : runnable_(runnable) {}

  AsyncRunnableBase::Ptr GetRunnable() const { return runnable_; }

 private:
  AsyncRunnableBase::Ptr runnable_;
};

class StopNotification : public Notification {};

}  // namespace

//
// AsyncDispatcher
//

AsyncDispatcher::AsyncDispatcher() { thread_.Start(*this); }

AsyncDispatcher::AsyncDispatcher(Thread::Priority priority) {
  thread_.SetPriority(priority);
  thread_.Start(*this);
}

AsyncDispatcher::~AsyncDispatcher() {
  try {
    Stop();
  } catch (...) {
    fun_unexpected();
  }
}

void AsyncDispatcher::Start(AsyncRunnableBase::Ptr runnable) {
  fun_check_ptr(runnable);
  queue_.Enqueue(new MethodNotification(runnable));
}

void AsyncDispatcher::Cancel() { queue_.Clear(); }

void AsyncDispatcher::Run() {
  // TODO dynamic_cast 제거??

  Notification::Ptr noti = queue_.WaitDequeue();
  while (noti && !dynamic_cast<StopNotification*>(noti.Get())) {
    MethodNotification* method_noti =
        dynamic_cast<MethodNotification*>(noti.Get());
    fun_check_ptr(method_noti);

    AsyncRunnableBase::Ptr runnable = method_noti->GetRunnable();
    // TODO 레퍼런스 관리 체계 점검.
    runnable->AddRef();  // Run() will release
    runnable->Run();
    runnable = nullptr;
    noti = queue_.WaitDequeue();
  }
}

void AsyncDispatcher::Stop() {
  queue_.Clear();
  queue_.WakeUpAll();
  queue_.Enqueue(new StopNotification);
  thread_.Join();
}

}  // namespace fun
