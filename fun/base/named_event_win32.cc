#include "fun/base/named_event_win32.h"
#include "fun/base/error.h"
#include "fun/base/exception.h"

namespace fun {

NamedEventImpl::NamedEventImpl(const String& name)
  : name_(name) {
  uname_ = UString::FromUtf8(name);
  event_ = CreateEventW(NULL, FALSE, FALSE, uname_.c_str());
  if (!event_) {
    DWORD rc = GetLastError();
    throw SystemException(String::Format("cannot create named event {0} [Error {1}: {2}]", name_, (int32)rc, Error::Message(rc)));
  }
}

NamedEventImpl::~NamedEventImpl() {
  CloseHandle(event_);
}

void NamedEventImpl::SetImpl() {
  if (!SetEvent(event_)) {
    throw SystemException("cannot signal named event", name_);
  }
}

void NamedEventImpl::WaitImpl() {
  switch (WaitForSingleObject(event_, INFINITE)) {
    case WAIT_OBJECT_0:
      return;
    default:
      throw SystemException("wait for named event failed", name_);
  }
}

} // namespace fun
