#pragma once

#include "fun/base/base.h"
#include "fun/base/container/list.h"
#include "fun/base/mutex.h"
#include "fun/base/notification_center.h"
#include "fun/base/ref_counted.h"
#include "fun/base/task.h"
#include "fun/base/thread_pool.h"
#include "fun/base/timestamp.h"

namespace fun {

class Notification;
class Exception;

/**
 * The TaskManager manages a collection of tasks
 * and monitors their lifetime.
 *
 * A TaskManager has a built-in NotificationCenter that
 * is used to send out notifications on task progress
 * and task states. See the TaskNotification class and its
 * subclasses for the various events that result in a notification.
 * To keep the number of notifications small, a TaskProgressNotification
 * will only be sent out once in 100 milliseconds.
 */
class FUN_BASE_API TaskManager {
 public:
  typedef RefCountedPtr<Task> TaskPtr;
  typedef List<TaskPtr> TaskList;

  /**
   * Creates the TaskManager, using the default ThreadPool.
   */
  TaskManager(ThreadPool::ThreadAffinityPolicy affinity_policy =
                  ThreadPool::TAP_DEFAULT);

  /**
   * Creates the TaskManager, using the given ThreadPool.
   */
  TaskManager(ThreadPool& pool);

  /**
   * Destroys the TaskManager.
   */
  ~TaskManager();

  /**
   * Starts the given task in a thread obtained from the thread pool,
   * on specified cpu.
   *
   * The TaskManager takes ownership of the Task object
   * and deletes it when it it finished.
   */
  void Start(Task* task, int cpu = -1);

  /**
   * Starts the given task in the current thread.
   * The TaskManager takes ownership of the Task object
   * and deletes it when it it finished.
   */
  void StartSync(Task* task);

  /**
   * Requests cancellation of all tasks.
   */
  void CancelAll();

  /**
   * Waits for the completion of all the threads
   * in the TaskManager's thread pool.
   *
   * Note: JoinAll() will wait for ALL tasks in the
   * TaskManager's ThreadPool to complete. If the
   * ThreadPool has threads created by other
   * facilities, these threads must also complete
   * before JoinAll() can return.
   */
  void JoinAll();

  /**
   * Returns a copy of the internal task list.
   */
  TaskList GetTaskList() const;

  /**
   * Returns the number of tasks in the internal task list.
   */
  std::size_t Count() const;

  /**
   * Registers an observer with the NotificationCenter.
   *
   * Usage:
   *     Observer<MyClass, MyNotification> obs(*this,
   * &MyClass::HandleNotification); notificationCenter.AddObserver(obs);
   */
  void AddObserver(const ObserverBase& observer);

  /**
   * Unregisters an observer with the NotificationCenter.
   */
  void RemoveObserver(const ObserverBase& observer);

  static const int MIN_PROGRESS_NOTIFICATION_INTERVAL;

 protected:
  /**
   * Posts a notification to the task manager's
   * notification center.
   */
  void Post(const Notification::Ptr& noti);

  void OnTaskStarted(Task* task);
  void OnProgress(Task* task, float progress);
  void OnTaskCancelled(Task* task);
  void OnTaskFinished(Task* task);
  void OnTaskFailed(Task* task, const Exception& e);

 private:
  ThreadPool& thread_pool_;
  TaskList task_list_;
  Timestamp last_progress_notification_time_;
  NotificationCenter nc_;
  mutable FastMutex mutex_;

  friend class Task;
};

//
// inlines
//

FUN_ALWAYS_INLINE size_t TaskManager::Count() const {
  FastMutex::ScopedLock guard(mutex_);

  return task_list_.Count();
}

}  // namespace fun
