#include "fun/base/task_notification.h"

namespace fun {

TaskNotification::TaskNotification(Task* task)
  : task_(task) {
  if (task_) {
    task_->AddRef();
  }
}

TaskNotification::~TaskNotification() {
  if (task_) {
    task_->Release();
  }
}

TaskStartedNotification::TaskStartedNotification(Task* task)
  : TaskNotification(task) {}

TaskStartedNotification::~TaskStartedNotification() {}

TaskCancelledNotification::TaskCancelledNotification(Task* task)
  : TaskNotification(task) {}

TaskCancelledNotification::~TaskCancelledNotification() {
}

TaskFinishedNotification::TaskFinishedNotification(Task* task)
  : TaskNotification(task) {}

TaskFinishedNotification::~TaskFinishedNotification() {}

TaskFailedNotification::TaskFailedNotification(Task* task, const Exception& e)
  : TaskNotification(task),
    exception_(e.Clone()) {}

TaskFailedNotification::~TaskFailedNotification() {
  delete exception_;
}

TaskProgressNotification::TaskProgressNotification(Task* task, float progress)
  : TaskNotification(task),
    progress_(progress) {}

TaskProgressNotification::~TaskProgressNotification() {}

} // namespace fun
