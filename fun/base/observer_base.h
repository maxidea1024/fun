#pragma once

#include "fun/base/base.h"
#include "fun/base/notification.h"

namespace fun {

class FUN_BASE_API ObserverBase {
 public:
  ObserverBase();
  ObserverBase(const ObserverBase& rhs);
  virtual ~ObserverBase();

  ObserverBase& operator=(const ObserverBase& rhs);

  virtual void Notify(Notification* noti) const = 0;
  virtual bool Equals(const ObserverBase& other) const = 0;
  virtual bool Accepts(Notification* noti) const = 0;
  virtual ObserverBase* Clone() const = 0;
  virtual void Disable() = 0;
};

}  // namespace fun
