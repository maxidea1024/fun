#pragma once

#include "fun/base/base.h"
#include "fun/base/event.h"
#include "fun/base/mutex.h"
#include "fun/base/notification.h"

#include <deque>
#include <map>

namespace fun {

class NotificationCenter;

/**
 * A PriorityNotificationQueue object provides a way to implement asynchronous
 * notifications. This is especially useful for sending notifications
 * from one thread to another, for example from a background thread to
 * the main (user interface) thread.
 *
 * The PriorityNotificationQueue is quite similar to the NotificationQueue
 * class. The only difference to NotificationQueue is that each Notification is
 * tagged with a priority value. When inserting a Notification into the queue,
 * the Notification is inserted according to the given priority value, with
 * lower priority values being inserted before higher priority
 * values. Therefore, the lower the numerical priority value, the higher
 * the actual notification priority.
 *
 * Notifications are dequeued in order of their priority.
 *
 * The PriorityNotificationQueue can also be used to distribute work from
 * a controlling thread to one or more worker threads. Each worker thread
 * repeatedly calls WaitDequeue() and processes the
 * returned notification. Special care must be taken when shutting
 * down a queue with worker threads waiting for notifications.
 * The recommended sequence to shut down and destroy the queue is to
 *   1. set a termination flag for every worker thread
 *   2. call the WakeUpAll() method
 *   3. join each worker thread
 *   4. destroy the notification queue.
 */
class FUN_BASE_API PriorityNotificationQueue {
 public:
  /**
   * Creates the PriorityNotificationQueue.
   */
  PriorityNotificationQueue();

  /**
   * Destroys the PriorityNotificationQueue.
   */
  ~PriorityNotificationQueue();

  /**
   * Enqueues the given notification by adding it to
   * the queue according to the given priority.
   * Lower priority values are inserted before higher priority values.
   * The queue takes ownership of the notification, thus
   * a call like
   *     notification_queue.Enqueue(new MyNotification, 1);
   * does not result in a memory leak.
   */
  void Enqueue(Notification::Ptr noti, int32 priority);

  /**
   * Dequeues the next pending notification.
   * Returns 0 (null) if no notification is available.
   * The caller gains ownership of the notification and
   * is expected to release it when done with it.
   *
   * It is highly recommended that the result is immediately
   * assigned to a Notification::Ptr, to avoid potential
   * memory management issues.
   */
  Notification* Dequeue();

  /**
   * Dequeues the next pending notification.
   * If no notification is available, waits for a notification
   * to be enqueued.
   * The caller gains ownership of the notification and
   * is expected to release it when done with it.
   * This method returns 0 (null) if WakeUpAll()
   * has been called by another thread.
   *
   * It is highly recommended that the result is immediately
   * assigned to a Notification::Ptr, to avoid potential
   * memory management issues.
   */
  Notification* WaitDequeue();

  /**
   * Dequeues the next pending notification.
   * If no notification is available, waits for a notification
   * to be enqueued up to the specified time.
   * Returns 0 (null) if no notification is available.
   * The caller gains ownership of the notification and
   * is expected to release it when done with it.
   *
   * It is highly recommended that the result is immediately
   * assigned to a Notification::Ptr, to avoid potential
   * memory management issues.
   */
  Notification* WaitDequeue(int32 milliseconds);

  /**
   * Dispatches all queued notifications to the given
   * notification center.
   */
  void Dispatch(NotificationCenter& notification_center);

  /**
   * Wakes up all threads that wait for a notification.
   */
  void WakeUpAll();

  /**
   * Returns true if the queue is empty.
   */
  bool IsEmpty() const;

  /**
   * Returns the number of notifications in the queue.
   */
  int32 Count() const;

  /**
   * Removes all notifications from the queue.
   */
  void Clear();

  /**
   * Returns true if the queue has at least one thread waiting
   * for a notification.
   */
  bool HasIdleThreads() const;

  /**
   * Returns a reference to the default
   * PriorityNotificationQueue.
   */
  static PriorityNotificationQueue& DefaultQueue();

 protected:
  Notification::Ptr DequeueOne();

 private:
  typedef std::multimap<int, Notification::Ptr> NfQueue;
  struct WaitInfo {
    Notification::Ptr noti;
    Event available;
  };
  typedef std::deque<WaitInfo*> WaitQueue;

  NfQueue noti_queue_;
  WaitQueue wait_queue_;
  mutable FastMutex mutex_;
};

}  // namespace fun
