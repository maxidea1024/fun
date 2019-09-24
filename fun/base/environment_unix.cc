#include "fun/base/environment_unix.h"
#include "fun/base/exception.h"
//#include "fun/base/buffer.h"

#include <cstring>
#include <unistd.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <sys/param.h>

#if FUN_PLATFORM = FUN_PLATFORM_BSD_FAMILY
#include <sys/sysctl.h>
#elif FUN_PLATFORM == FUN_PLATFORM_HPUX
#include <pthread.h>
#endif

#include <thread> // std::thread::hardware_concurrency

namespace fun {

EnvironmentImpl::StringMap EnvironmentImpl::map_;
FastMutex EnvironmentImpl::mutex_;

String EnvironmentImpl::GetImpl(const String& name) {
  ScopedLock<FastMutex> guard(mutex_);

  const char* val = getenv(name.c_str());
  if (val) {
    return String(val);
  } else {
    throw NotFoundException(name);
  }
}

bool EnvironmentImpl::HasImpl(const String& name) {
  ScopedLock<FastMutex> guard(mutex_);
  return !!getenv(name.c_str());
}

void EnvironmentImpl::SetImpl(const String& name, const String& value) {
  ScopedLock<FastMutex> guard(mutex_);

  String var = name;
  var.Append("=");
  var.Append(value);
  map_[name] = var;
  if (putenv((char*)map_[name].c_str())) {
    String msg = "cannot set environment variable: ";
    msg.Append(name);
    throw SystemException(msg);
  }
}

String EnvironmentImpl::GetOsNameImpl() {
  struct utsname uts;
  uname(&uts);
  return uts.sysname;
}

String EnvironmentImpl::GetOsDisplayNameImpl() {
  return GetOsNameImpl();
}

String EnvironmentImpl::GetOsVersionImpl() {
  struct utsname uts;
  uname(&uts);
  return uts.release;
}

String EnvironmentImpl::GetOsArchitectureImpl() {
  struct utsname uts;
  uname(&uts);
  return uts.machine;
}

String EnvironmentImpl::GetNodeNameImpl() {
  struct utsname uts;
  uname(&uts);
  return uts.nodename;
}

void EnvironmentImpl::GetNodeIdImpl(NodeId& out_id) {
  //TODO
  fun_check(0);
}

uint32 EnvironmentImpl::GetProcessorCountImpl() {
  return (uint32)std::thread::hardware_concurrency();
}

} // namespace fun
