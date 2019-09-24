#include "fun/base/task.h"
#include "fun/base/task_manager.h"
#include "fun/base/exception.h"

namespace fun {

Task::Task(const String& name)
  : name_(name),
    owner_(nullptr),
    progress_(0),
    state_(TaskState::Idle),
    cancel_event_(EventResetType::Manual) {
}

Task::~Task() {}

void Task::Cancel() {
  state_ = TaskState::Cancelling;
  cancel_event_.Set();
  if (owner_) {
    owner_->OnTaskCancelled(this);
  }
}

void Task::Reset() {
  progress_ = 0.0;
  state_ = TaskState::Idle;
  cancel_event_.Reset();
}

void Task::Run() {
  TaskManager* owner = GetOwner();

  if (owner) {
    owner->OnTaskStarted(this);
  }

  try {
    state_ = TaskState::Running;
    RunTask();
  } catch (Exception& e) {
    if (owner) {
      owner->OnTaskFailed(this, e);
    }
  } catch (std::exception& e) {
    if (owner) {
      owner->OnTaskFailed(this, SystemException(e.what()));
    }
  } catch (...) {
    if (owner) {
      owner->OnTaskFailed(this, SystemException("unknown exception"));
    }
  }

  state_ = TaskState::Finished;

  if (owner) {
    owner->OnTaskFinished(this);
  }
}

bool Task::Sleep(int32 milliseconds) {
  return cancel_event_.TryWait(milliseconds);
}

bool Task::Yield() {
  Thread::Yield();
  return IsCancelled();
}

void Task::SetProgress(float progress) {
  FastMutex::ScopedLock guard(mutex_);

  if (progress_ != progress) {
    progress_ = progress;

    if (owner_) {
      owner_->OnProgress(this, progress_);
    }
  }
}

void Task::SetOwner(TaskManager* owner) {
  FastMutex::ScopedLock guard(mutex_);

  owner_ = owner;
}

void Task::SetState(TaskState state) {
  state_ = state;
}

void Task::PostNotification(Notification* noti) {
  fun_check_ptr(noti);

  FastMutex::ScopedLock guard(mutex_);

  if (owner_) {
    owner_->Post(noti);
  }
}

} // namespace fun
