#pragma once

#include "fun/base/base.h"
#include "fun/base/notification.h"
#include "fun/base/task.h"

namespace fun {

/**
 * Base class for TaskManager notifications.
 */
class FUN_BASE_API TaskNotification : public Notification {
 public:
  /**
   * Creates the TaskNotification.
   */
  TaskNotification(Task* task);

  /**
   * Returns the subject of the notification.
   */
  Task* GetTask() const;

 protected:
  /**
   * Destroys the TaskNotification.
   */
  virtual ~TaskNotification();

 private:
  Task* task_;
};


/**
 * This notification is posted by the TaskManager for
 * every task that has been started.
 */
class FUN_BASE_API TaskStartedNotification : public TaskNotification {
 public:
  TaskStartedNotification(Task* task);

 protected:
  ~TaskStartedNotification();
};


/**
 * This notification is posted by the TaskManager for
 * every task that has been cancelled.
 */
class FUN_BASE_API TaskCancelledNotification : public TaskNotification {
 public:
  TaskCancelledNotification(Task* task);

 protected:
  ~TaskCancelledNotification();
};


/**
 * This notification is posted by the TaskManager for
 * every task that has finished.
 */
class FUN_BASE_API TaskFinishedNotification : public TaskNotification {
 public:
  TaskFinishedNotification(Task* task);

 protected:
  ~TaskFinishedNotification();
};


/**
 * This notification is posted by the TaskManager for
 * every task that has failed with an exception.
 */
class FUN_BASE_API TaskFailedNotification : public TaskNotification {
 public:
  TaskFailedNotification(Task* task, const Exception& e);

  const Exception& GetReason() const;

 protected:
  ~TaskFailedNotification();

 private:
  Exception* exception_;
};


/**
 * This notification is posted by the TaskManager for
 * a task when its progress changes.
*/
class FUN_BASE_API TaskProgressNotification : public TaskNotification {
 public:
  TaskProgressNotification(Task* task, float progress);

  float GetProgress() const;

 protected:
  ~TaskProgressNotification();

 private:
  float progress_;
};


/**
 * This is a template for "custom" notification.
 * Unlike other notifications, this notification
 * is instantiated and posted by the task itself.
 * The purpose is to provide generic notification
 * mechanism between the task and its observer(s).
 */
template <typename C>
class TaskCustomNotification : public TaskNotification {
 public:
  TaskCustomNotification(Task* task, const C& custom)
    : TaskNotification(task),
      custom_(custom) {
  }

  const C& GetCustom() const {
    return custom_;
  }

 protected:
  ~TaskCustomNotification() {}

 private:
  C custom_;
};


//
// inlines
//

FUN_ALWAYS_INLINE Task* TaskNotification::GetTask() const {
  return task_;
}

FUN_ALWAYS_INLINE const Exception& TaskFailedNotification::GetReason() const {
  return *exception_;
}

FUN_ALWAYS_INLINE float TaskProgressNotification::GetProgress() const {
  return progress_;
}

} // namespace fun
