#include "fun/base/rw_lock_win32.h"
#include "fun/base/exception.h"

namespace fun {

RWLockImpl::RWLockImpl() : readers_(0), writes_waiting_(0), writers_(0) {
  mutex_ = CreateMutexW(NULL, FALSE, NULL);
  if (mutex_ == NULL) {
    throw SystemException("cannot create reader/writer lock");
  }

  read_event_ = CreateEventW(NULL, TRUE, TRUE, NULL);
  if (read_event_ == NULL) {
    throw SystemException("cannot create reader/writer lock");
  }

  write_event_ = CreateEventW(NULL, TRUE, TRUE, NULL);
  if (write_event_ == NULL) {
    throw SystemException("cannot create reader/writer lock");
  }
}

RWLockImpl::~RWLockImpl() {
  CloseHandle(mutex_);
  CloseHandle(read_event_);
  CloseHandle(write_event_);
}

void RWLockImpl::AddWriter() {
  switch (WaitForSingleObject(mutex_, INFINITE)) {
    case WAIT_OBJECT_0:
      if (++writes_waiting_ == 1) {
        ResetEvent(read_event_);
      }
      ReleaseMutex(mutex_);
      break;
    default:
      throw SystemException("cannot lock reader/writer lock");
  }
}

void RWLockImpl::RemoveWriter() {
  switch (WaitForSingleObject(mutex_, INFINITE)) {
    case WAIT_OBJECT_0:
      if (--writes_waiting_ == 0 && writers_ == 0) {
        SetEvent(read_event_);
      }
      ReleaseMutex(mutex_);
      break;
    default:
      throw SystemException("cannot lock reader/writer lock");
  }
}

void RWLockImpl::ReadLockImpl() {
  HANDLE h[2] = {mutex_, read_event_};
  switch (WaitForMultipleObjects(2, h, TRUE, INFINITE)) {
    case WAIT_OBJECT_0:
    case WAIT_OBJECT_0 + 1:
      ++readers_;
      ResetEvent(write_event_);
      ReleaseMutex(mutex_);
      fun_check_dbg(writers_ == 0);
      break;
    default:
      throw SystemException("cannot lock reader/writer lock");
  }
}

bool RWLockImpl::TryReadLockImpl() {
  for (;;) {
    if (writers_ != 0 || writes_waiting_ != 0) {
      return false;
    }

    DWORD rc = TryReadLockOnce();
    switch (rc) {
      case WAIT_OBJECT_0:
      case WAIT_OBJECT_0 + 1:
        return true;
      case WAIT_TIMEOUT:
        continue;  // try again
      default:
        throw SystemException("cannot lock reader/writer lock");
    }
  }
}

void RWLockImpl::WriteLockImpl() {
  AddWriter();

  HANDLE h[2] = {mutex_, write_event_};
  switch (WaitForMultipleObjects(2, h, TRUE, INFINITE)) {
    case WAIT_OBJECT_0:
    case WAIT_OBJECT_0 + 1:
      --writes_waiting_;
      ++readers_;
      ++writers_;
      ResetEvent(read_event_);
      ResetEvent(write_event_);
      ReleaseMutex(mutex_);
      fun_check_dbg(writers_ == 1);
      break;
    default:
      RemoveWriter();
      throw SystemException("cannot lock reader/writer lock");
  }
}

bool RWLockImpl::TryWriteLockImpl() {
  AddWriter();

  HANDLE h[2] = {mutex_, write_event_};
  switch (WaitForMultipleObjects(2, h, TRUE, 1)) {
    case WAIT_OBJECT_0:
    case WAIT_OBJECT_0 + 1:
      --writes_waiting_;
      ++readers_;
      ++writers_;
      ResetEvent(read_event_);
      ResetEvent(write_event_);
      ReleaseMutex(mutex_);
      fun_check_dbg(writers_ == 1);
      return true;
    case WAIT_TIMEOUT:
      RemoveWriter();
      return false;
    default:
      RemoveWriter();
      throw SystemException("cannot lock reader/writer lock");
  }
}

void RWLockImpl::UnlockImpl() {
  switch (WaitForSingleObject(mutex_, INFINITE)) {
    case WAIT_OBJECT_0:
      writers_ = 0;
      if (writes_waiting_ == 0) {
        SetEvent(read_event_);
      }
      if (--readers_ == 0) {
        SetEvent(write_event_);
      }
      ReleaseMutex(mutex_);
      break;
    default:
      throw SystemException("cannot unlock reader/writer lock");
  }
}

DWORD RWLockImpl::TryReadLockOnce() {
  HANDLE h[2] = {mutex_, read_event_};
  DWORD rc = WaitForMultipleObjects(2, h, TRUE, 1);
  switch (rc) {
    case WAIT_OBJECT_0:
    case WAIT_OBJECT_0 + 1:
      ++readers_;
      ResetEvent(write_event_);
      ReleaseMutex(mutex_);
      fun_check_dbg(writers_ == 0);
      return rc;
    case WAIT_TIMEOUT:
      return rc;
    default:
      throw SystemException("cannot lock reader/writer lock");
  }
}

}  // namespace fun
