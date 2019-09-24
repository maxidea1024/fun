#pragma once

#include "fun/base/base.h"
#include "fun/base/container/list.h"
#include "fun/base/event.h"
#include "fun/base/mutex.h"
#include "fun/base/notification.h"
#include "fun/base/notification_center.h"

namespace fun {

/**
 * A NotificationQueue object provides a way to implement asynchronous
 * notifications. This is especially useful for sending notifications
 * from one thread to another, for example from a background thread to
 * the main (user interface) thread.
 *
 * The NotificationQueue can also be used to distribute work from
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
class NotificationQueue {
 public:
  /**
   * Creates the NotificationQueue.
   */
  NotificationQueue();

  /**
   * Destroys the NotificationQueue.
   */
  virtual ~NotificationQueue();

  /**
   * Enqueues the given notification by adding it to
   * the end of the queue (FIFO).
   * The queue takes ownership of the notification, thus
   * a call like
   *     notification_queue.Enqueue(new MyNotification);
   * does not result in a memory leak.
   */
  void Enqueue(Notification::Ptr noti);

  /**
   * Enqueues the given notification by adding it to
   * the front of the queue (LIFO). The event therefore gets processed
   * before all other events already in the queue.
   * The queue takes ownership of the notification, thus
   * a call like
   *     notification_queue.EnqueueUrgent(new MyNotification);
   * does not result in a memory leak.
   */
  void EnqueueUrgent(Notification::Ptr noti);

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
  Notification::Ptr Dequeue();

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
  Notification::Ptr WaitDequeue();

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
  Notification::Ptr WaitDequeue(int32 milliseconds);

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
   * Removes a notification from the queue.
   * Returns true if remove succeeded, false otherwise
   */
  bool Remove(Notification::Ptr noti);

  /**
   * Returns true if the queue has at least one thread waiting
   * for a notification.
   */
  bool HasIdleThread() const;

  /**
   * Returns a reference to the default
   * NotificationQueue.
   */
  static NotificationQueue& DefaultQueue();

 protected:
  Notification::Ptr DequeueOne();

 private:
  struct WaitInfo {
    Notification::Ptr noti;
    Event available;
  };
  List<Notification::Ptr> queue_;
  List<WaitInfo*> wait_queue_;
  FastMutex mutex_;
};

}  // namespace fun
