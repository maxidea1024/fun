#include "fun/base/thread.h"
#include "fun/base/mutex.h"
#include "fun/base/exception.h"
#include "fun/base/thread_local.h"
#include "fun/base/atomic_counter.h"

#include <sstream>

#include "fun/base/thread_std.cc"

namespace fun {

namespace {

class RunnableHolder : public Runnable {
  public:
  RunnableHolder(Runnable& target) : target_(target) {}

  ~RunnableHolder() {}

  void Run() override {
    target_.Run();
  }

 private:
  Runnable& target_;
};

class CallableHolder : public Runnable {
  public:
  CallableHolder(Thread::Callable callable, void* data)
    : callable_(callable), data_(data) {}

  ~CallableHolder() {}

  void Run() override {
    callable_(data_);
  }

 private:
  Thread::Callable callable_;
  void* data_;
};

} // namespace


Thread::Thread()
  : id_(UniqueId()),
    name_(MakeName()),
    tls_(nullptr),
    event_() {}

Thread::Thread(const String& name)
  : id_(UniqueId()),
    name_(name),
    tls_(nullptr),
    event_() {}

Thread::~Thread() {
  delete tls_;
}

void Thread::SetPriority(Priority prior) {
  SetPriorityImpl(prior);
}

Thread::Priority Thread::GetPriority() const {
  return (Priority)GetPriorityImpl();
}

void Thread::Start(Runnable& target) {
  StartImpl(SharedPtr<Runnable>(new RunnableHolder(target)));
}

void Thread::Start(Callable target, void* data) {
  StartImpl(SharedPtr<Runnable>(new CallableHolder(target, data)));
}

void Thread::Join() {
  JoinImpl();
}

void Thread::Join(int32 milliseconds) {
  if (!JoinImpl(milliseconds)) {
    throw TimeoutException();
  }
}

bool Thread::TryJoin(int32 milliseconds) {
  return JoinImpl(milliseconds);
}

bool Thread::TrySleep(int32 milliseconds) {
  Thread* current = Current();
  fun_check_ptr(current);
  return !(current->event_.TryWait(milliseconds));
}

void Thread::WakeUp() {
  event_.Set();
}

ThreadLocalStorage& Thread::Tls() {
  if (tls_ == nullptr) {
    tls_ = new ThreadLocalStorage;
  }
  return *tls_;
}

void Thread::ClearTls() {
  delete tls_;
  tls_ = nullptr;
}

String Thread::MakeName() {
  return String("#") + id_;
}

int32 Thread::UniqueId() {
  static AtomicCounter32 counter(0);
  return ++counter;
}

void Thread::SetName(const String& name) {
  ScopedLock<FastMutex> guard(mutex_);
  name_ = name;
}

} // namespace fun
