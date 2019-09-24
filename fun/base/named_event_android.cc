#include "fun/base/named_event_android.h"
#include "fun/base/exception.h"

namespace fun {

NamedEventImpl::NamedEventImpl(const String&) {
  // NOOP
}

NamedEventImpl::~NamedEventImpl() {
  // NOOP
}

void NamedEventImpl::SetImpl() {
  throw NotImplementedException("NamedEvent::set() not available on Android");
}

void NamedEventImpl::WaitImpl() {
  throw NotImplementedException("NamedEvent::wait() not available on Android");
}

} // namespace fun
