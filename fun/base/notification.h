#pragma once

#include "fun/base/base.h"
#include "fun/base/ref_counted.h"
#include "fun/base/string/string.h"

namespace fun {

/**
 * The base class for all notification classes used
 * with the NotificationCenter and the NotificationQueue
 * classes.
 *
 * The Notification class can be used with the RefCountedPtr
 * template class.
 */
class Notification : public RefCountedObject {
 public:
  using Ptr = RefCountedPtr<Notification>;

  /**
   * Creates the notification.
   */
  Notification();

  /**
   * dtor.
   */
  virtual ~Notification();

  /**
   * Returns the name of the notification.
   * The default implementation returns the class name.
   */
  // virtual const String& GetName() const;
  virtual String GetName() const;
};

}  // namespace fun
