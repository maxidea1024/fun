#include "fun/base/task_manager.h"
#include "fun/base/task_notification.h"
#include "fun/base/thread_pool.h"

namespace fun {

const int TaskManager::MIN_PROGRESS_NOTIFICATION_INTERVAL = 100000; // 100 milliseconds

TaskManager::TaskManager(ThreadPool::ThreadAffinityPolicy affinity_policy)
  : thread_pool_(ThreadPool::DefaultPool(affinity_policy)) {}

TaskManager::TaskManager(ThreadPool& pool) : thread_pool_(pool) {}

TaskManager::~TaskManager() {}

void TaskManager::Start(Task* task, int32 cpu) {
  TaskPtr auto_task(task); // take ownership immediately
  FastMutex::ScopedLock guard(mutex_);

  auto_task->SetOwner(this);
  auto_task->SetState(Task::TaskState::Starting);
  task_list_.PushBack(auto_task);
  try {
    thread_pool_.Start(*auto_task, auto_task->GetName(), cpu);
  } catch (...) {
    // Make sure that we don't act like we own the task since
    // we never started it.  If we leave the task on our task
    // list, the size of the list is incorrect.
    task_list_.PopBack();
    throw;
  }
}

void TaskManager::StartSync(Task* task) {
  TaskPtr auto_task(task); // take ownership immediately
  ScopedLockWithUnlock<FastMutex> guard(mutex_);

  auto_task->SetOwner(this);
  auto_task->SetState(Task::TaskState::Starting);
  task_list_.PushBack(auto_task);
  guard.Unlock();
  try {
    auto_task->Run();
  } catch (...) {
    FastMutex::ScopedLock mini_guard(mutex_);

    // Make sure that we don't act like we own the task since
    // we never started it.  If we leave the task on our task
    // list, the size of the list is incorrect.
    task_list_.PopBack();
    throw;
  }
}

void TaskManager::CancelAll() {
  FastMutex::ScopedLock guard(mutex_);

  for (auto& task : task_list_) {
    task->Cancel();
  }
}

void TaskManager::JoinAll() {
  thread_pool_.JoinAll();
}

TaskManager::TaskList TaskManager::GetTaskList() const {
  FastMutex::ScopedLock guard(mutex_);

  return task_list_;
}

void TaskManager::AddObserver(const ObserverBase& observer) {
  nc_.AddObserver(observer);
}

void TaskManager::RemoveObserver(const ObserverBase& observer) {
  nc_.RemoveObserver(observer);
}

void TaskManager::Post(const Notification::Ptr& noti) {
  nc_.Post(noti);
}

void TaskManager::OnTaskStarted(Task* task) {
  nc_.Post(new TaskStartedNotification(task));
}

void TaskManager::OnProgress(Task* task, float progress) {
  ScopedLockWithUnlock<FastMutex> guard(mutex_);

  if (last_progress_notification_time_.IsElapsed(MIN_PROGRESS_NOTIFICATION_INTERVAL)) {
    last_progress_notification_time_.Update();
    guard.Unlock();

    nc_.Post(new TaskProgressNotification(task, progress));
  } else {
    // Ignore..
  }
}

void TaskManager::OnTaskCancelled(Task* task) {
  nc_.Post(new TaskCancelledNotification(task));
}

void TaskManager::OnTaskFinished(Task* task) {
  TaskPtr current_task;
  ScopedLockWithUnlock<FastMutex> guard(mutex_);

  for (auto it = task_list_.begin(); it != task_list_.end(); ++it) {
    if (*it == task) {
      current_task = *it; // hold reference for notifying
      task_list_.Remove(it);
    }
  }

  guard.Unlock();

  nc_.Post(new TaskFinishedNotification(task));
}

void TaskManager::OnTaskFailed(Task* task, const Exception& e) {
  nc_.Post(new TaskFailedNotification(task, e));
}

} // namespace fun
