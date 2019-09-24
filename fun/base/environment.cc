#include "fun/base/environment.h"

#include <cstdio>  // sprintf()
#include <cstdlib>

#if FUN_PLATFORM_WINDOWS_FAMILY
#include "fun/base/environment_win32.cc"
#else
#include "fun/base/environment_unix.cc"
#endif

#include <thread>  //std::thread::hardware_concurrency

namespace fun {

String Environment::Get(const String& name) {
  return EnvironmentImpl::GetImpl(name);
}

String Environment::Get(const String& name, const String& default_value) {
  if (Has(name)) {
    return Get(name);
  } else {
    return default_value;
  }
}

bool Environment::Has(const String& name) {
  return EnvironmentImpl::HasImpl(name);
}

void Environment::Set(const String& name, const String& value) {
  EnvironmentImpl::SetImpl(name, value);
}

String Environment::GetOsName() {
  // TODO caching
  return EnvironmentImpl::GetOsNameImpl();
}

String Environment::GetOsDisplayName() {
  // TODO caching
  return EnvironmentImpl::GetOsDisplayNameImpl();
}

String Environment::GetOsVersion() {
  // TODO caching
  return EnvironmentImpl::GetOsVersionImpl();
}

String Environment::GetOsArchitecture() {
  // TODO caching
  return EnvironmentImpl::GetOsArchitectureImpl();
}

String Environment::GetNodeName() {
  // TODO caching
  return EnvironmentImpl::GetNodeNameImpl();
}

String Environment::GetNodeId() {
  NodeId id;
  GetNodeId(id);

  char result[18];
  std::sprintf(result, "%02x:%02x:%02x:%02x:%02x:%02x", id[0], id[1], id[2],
               id[3], id[4], id[5]);
  return String(result);
}

void Environment::GetNodeId(NodeId& out_id) {
  // TODO caching
  return EnvironmentImpl::GetNodeIdImpl(out_id);
}

int32 Environment::GetProcessorCount() {
  // TODO caching
  // return ProcessorCountImpl();
  // return (int32)std::thread::hardware_concurrency();
  return EnvironmentImpl::GetProcessorCountImpl();
}

uint32 Environment::GetLibraryVersion() { return FUN_VERSION; }

int32 Environment::GetOs() { return FUN_PLATFORM; }

int32 Environment::GetArch() { return FUN_ARCH; }

bool Environment::IsUnix() {
#if FUN_PLATFORM_UNIX_FAMILY
  return true;
#else
  return false;
#endif
}

bool Environment::IsWindows() {
#if FUN_PLATFORM_WINDOWS_FAMILY
  return true;
#else
  return false;
#endif
}

}  // namespace fun
