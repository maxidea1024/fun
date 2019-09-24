#pragma once

#include "fun/base/base.h"
#include "fun/base/exception.h"
#include "fun/base/windows_less.h"

namespace fun {

class FUN_BASE_API EventImpl
{
 public
  explicit EventImpl(EventResetType type = EventResetType::Auto);
  ~EventImpl();

  void SetImpl();
  void WaitImpl();
  bool WaitImpl(int32 milliseconds);
  void ResetImpl();

 private:
  HANDLE event_handle_;
};


//
// inlines
//

inline EventImpl::EventImpl(EventResetType type)
{
  event_handle_ = CreateEventW(NULL, type == EventResetType::Auto ? FALSE : TRUE, FALSE, NULL);
  if (!event_handle_) {
    throw SystemException("cannot create event");
  }
}

inline EventImpl::~EventImpl()
{
  CloseHandle(event_handle_);
}

inline void EventImpl::WaitImpl()
{
  switch (WaitForSingleObject(event_handle_, INFINITE)) {
    case WAIT_OBJECT_0:
      return;
    default:
      throw SystemException("wait for event failed");
  }
}

inline bool EventImpl::WaitImpl(int32 milliseconds)
{
  switch (WaitForSingleObject(event_handle_, milliseconds + 1)) {
    case WAIT_TIMEOUT:
      return false;
    case WAIT_OBJECT_0:
      return true;
    default:
      throw SystemException("wait for event failed");
  }
}

inline void EventImpl::SetImpl()
{
  if (!SetEvent(event_handle_) {
    throw SystemException("cannot signal event");
  }
}

inline void EventImpl::ResetImpl()
{
  if (!ResetEvent(event_handle_) {
    throw SystemException("cannot reset event");
  }
}

} // namespace fun
