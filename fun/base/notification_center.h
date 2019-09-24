#pragma once

#include "fun/base/base.h"
#include "fun/base/container/array.h"
#include "fun/base/container/list.h"
#include "fun/base/ftl/shared_ptr.h"
#include "fun/base/mutex.h"
#include "fun/base/notification.h"
#include "fun/base/observer_base.h"

namespace fun {

/**
 * A NotificationCenter is essentially a notification dispatcher.
 * It notifies all observers of notifications meeting specific criteria.
 * This information is encapsulated in Notification objects.
 * Client objects register themselves with the notification center as observers
 * of specific notifications posted by other objects. When an event occurs, an
 * object posts an appropriate notification to the notification center. The
 * notification center invokes the registered method on each matching observer,
 * passing the notification as argument.
 *
 * The order in which observers receive notifications is undefined.
 * It is possible for the posting object and the observing object to be the
 * same. The NotificationCenter delivers notifications to observers
 * synchronously. In other words the PostNotification() method does not return
 * until all observers have received and processed the notification. If an
 * observer throws an exception while handling a notification, the
 * NotificationCenter stops dispatching the notification and PostNotification()
 * rethrows the exception.
 *
 * In a multithreaded scenario, notifications are always delivered in the thread
 * in which the notification was posted, which may not be the same thread in
 * which an observer registered itself.
 *
 * The NotificationCenter class is basically a C++ implementation of the
 * NSNotificationCenter class found in Apple's Cocoa (or OpenStep).
 *
 * While handling a notification, an observer can unregister itself from the
 * notification center, or it can register or unregister other observers.
 * Observers added during a dispatch cycle will not receive the current
 * notification.
 *
 * The method receiving the notification must be implemented as
 *     void HandleNotification(MyNotification* noti);
 * The handler method gets co-ownership of the Notification object
 * and must release it when done. This is best done with an Ptr:
 *   void MyClass::HandleNotification(MyNotification* noti) {
 *     SharedPtr<MyNotification> noti_ptr(noti);
 *     ...
 *   }
 *
 * Alternatively, the NObserver class template can be used to register a
 * callback method. In this case, the callback method receives the Notification
 * in an Ptr and thus does not have to deal with object ownership issues: void
 * MyClass::HandleNotification(const SharedPtr<MyNotification>& noti) {
 *     ...
 *   }
 */
class FUN_BASE_API NotificationCenter {
 public:
  /**
   * Creates the NotificationCenter.
   */
  NotificationCenter();

  /**
   * Destroys the NotificationCenter.
   */
  virtual ~NotificationCenter();

  /**
   * Register an observer with the NotificationCenter.
   *
   * Usage:
   *   Observer<MyClass,MyNotification> observer(*this,
   * &MyClass::HandleNotification); notification_center.AddObserver(observer);
   *
   * Alternatively, the NObserver template class can be used instead of
   * Observer.
   */
  void AddObserver(const ObserverBase& observer);

  /**
   * Unregisters an observer with the NotificationCenter.
   */
  void RemoveObserver(const ObserverBase& observer);

  /**
   * Returns true if the observer is registered with this NotificationCenter.
   */
  bool HasObserver(const ObserverBase& observer) const;

  /**
   * Posts a notification to the NotificationCenter.
   * The NotificationCenter then delivers the notification
   * to all interested observers.
   * If an observer throws an exception, dispatching terminates
   * and the exception is rethrown to the caller.
   * Ownership of the notification object is claimed and the
   * notification is released before returning. Therefore,
   * a call like
   *    notification_center.Post(new MyNotification);
   * does not result in a memory PostNotification.
   */
  void Post(Notification::Ptr noti);

  /**
   * Returns true if there is at least one registered observer.
   *
   * Can be used to improve performance if an expensive notification
   * shall only be created and posted if there are any observers.
   */
  bool HasOservers() const;

  /**
   * Returns the number of registered observers.
   */
  int32 GetObserverCount() const;

  /**
   * Returns a reference to the default
   * NotificationCenter.
   */
  static NotificationCenter& DefaultCenter();

 private:
  Array<SharedPtr<ObserverBase>> observers_;
  Mutex mutex_;
};

}  // namespace fun
