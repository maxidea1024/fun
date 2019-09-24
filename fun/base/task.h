#pragma once

#include "fun/base/base.h"
#include "fun/base/runnable.h"
#include "fun/base/mutex.h"
#include "fun/base/event.h"
#include "fun/base/ref_counted_object.h"

namespace fun {

class TaskManager;
class Notification;
class NotificationCenter;

/**
 * A Task is a subclass of Runnable that has a name
 * and supports progress reporting and cancellation.
 *
 * A TaskManager object can be used to take care of the
 * lifecycle of a Task.
 */
class FUN_BASE_API Task : public Runnable, public RefCountedObject {
 public:
  enum class TaskState {
    Idle,
    Starting,
    Running,
    Cancelling,
    Finished,
  };

  Task(const String& name);

  const String& GetName() const;
  float GetProgress() const;
  virtual void Cancel();
  bool IsCancelled() const;
  TaskState GetState() const;
  void Reset();
  virtual void RunTask() = 0;
  void Run();

  Task() = delete;
  Task(const Task&) = delete;
  Task& operator = (const Task&) = delete;

 protected:
  bool Sleep(int32 milliseconds);
  bool Yield();
  void SetProgress(float progress);
  virtual void PostNotification(Notification* noti);
  void SetOwner(TaskManager* owner);
  TaskManager* GetOwner() const;
  void SetState(TaskState state);
  virtual ~Task();

 private:
  String name_;
  TaskManager* owner_;
  float progress_;
  TaskState state_;
  //TODO pooling
  Event cancel_event_;
  mutable FastMutex mutex_;

  friend class TaskManager;
};


//
// inlines
//

inline const String& Task::GetName() const {
  return name_;
}

inline float Task::GetProgress() const {
  FastMutex::ScopedLock guard(mutex_);
  return progress_;
}

inline bool Task::IsCancelled() const {
  return state_ == TaskState::Cancelling;
}

inline Task::TaskState Task::GetState() const {
  return state_;
}

inline TaskManager* Task::GetOwner() const {
  FastMutex::ScopedLock guard(mutex_);
  return owner_;
}

} // namespace fun
