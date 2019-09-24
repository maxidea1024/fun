#include "fun/base/notification.h"
#include <typeinfo>

namespace fun {

Notification::Notification() {
  // NOOP
}

Notification::~Notification() {
  // NOOP
}

String Notification::GetName() const {
  //TODO 이것은 제거하는게??
  return typeid(*this).name();
}

} // namespace fun
