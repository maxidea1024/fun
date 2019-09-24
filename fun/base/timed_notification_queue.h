#pragma once

#include "fun/base/base.h"
#include "fun/base/notification.h"
#include "fun/base/mutex.h"
#include "fun/base/event.h"
#include "fun/base/timestamp.h"
#include "fun/base/clock.h"
#include <map>

namespace fun {

/**
 * A TimedNotificationQueue object provides a way to implement timed, asynchronous
 * notifications. This is especially useful for sending notifications
 * from one thread to another, for example from a background thread to
 * the main (user interface) thread.
 *
 * The TimedNotificationQueue is quite similar to the NotificationQueue class.
 * The only difference to NotificationQueue is that each Notification is tagged
 * with a Timestamp. When inserting a Notification into the queue, the
 * Notification is inserted according to the given Timestamp, with
 * lower Timestamp values being inserted before higher ones.
 *
 * Notifications are dequeued in order of their timestamps.
 *
 * TimedNotificationQueue has some restrictions regarding multithreaded use.
 * While multiple threads may enqueue notifications, only one thread at a
 * time may dequeue notifications from the queue.
 *
 * If two threads try to dequeue a notification simultaneously, the results
 * are undefined.
 */
class FUN_BASE_API TimedNotificationQueue {
 public:
  /**
   * Creates the TimedNotificationQueue.
   */
  TimedNotificationQueue();

  /**
   * Destroys the TimedNotificationQueue.
   */
  ~TimedNotificationQueue();

  /**
   * Enqueues the given notification by adding it to
   * the queue according to the given timestamp.
   * Lower timestamp values are inserted before higher ones.
   * The queue takes ownership of the notification, thus
   * a call like
   *     notification_queue.Enqueue(new MyNotification, some_time);
   * does not result in a memory leak.
   *
   * The Timestamp is converted to an equivalent Clock value.
   */
  void Enqueue(Notification::Ptr noti, const Timestamp& timestamp);

  /**
   * Enqueues the given notification by adding it to
   * the queue according to the given clock value.
   * Lower clock values are inserted before higher ones.
   * The queue takes ownership of the notification, thus
   * a call like
   *     notification_queue.Enqueue(new MyNotification, some_time);
   * does not result in a memory leak.
   */
  void Enqueue(Notification::Ptr noti, const Clock& clock);

  /**
   * Dequeues the next pending notification with a timestamp
   * less than or equal to the current time.
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
   * Returns true if the queue is empty.
   */
  bool IsEmpty() const;

  /**
   * Returns the number of notifications in the queue.
   */
  int Count() const;

  /**
   * Removes all notifications from the queue.
   *
   * Calling Clear() while another thread executes one of
   * the dequeue member functions will result in undefined
   * behavior.
   */
  void Clear();

 protected:
  typedef std::multimap<Clock, Notification::Ptr> NfQueue;
  Notification::Ptr DequeueOne(NfQueue::iterator& it);
  bool Wait(Clock::DiffType interval);

 private:
  NfQueue noti_queue_;
  Event noti_available_;
  mutable FastMutex mutex_;
};

} // namespace fun
