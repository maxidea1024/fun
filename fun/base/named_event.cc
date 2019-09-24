#include "fun/base/named_event.h"

#if FUN_PLATFORM_WINDOWS_FAMILY
#include "fun/base/named_event_win32.cc"
#elif FUN_PLATFORM == FUN_PLATFORM_ANDROID
#include "fun/base/named_event_android.cc"
#elif FUN_PLATFORM_UNIX_FAMILY
#include "fun/base/named_event_unix.cc"
#endif

namespace fun {

NamedEvent::NamedEvent(const String& name)
  : NamedEventImpl(name) {
  // NOOP
}

NamedEvent::~NamedEvent() {
  // NOOP
}

} // namespace fun
