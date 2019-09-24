#if !defined(_WIN32_WCE)

#include "fun/framework/win_service.h"
#include "fun/base/exception.h"
#include "fun/base/thread.h"
#include "fun/framework/win_registry_key.h"

namespace fun {
namespace framework {

const int32 WinService::STARTUP_TIMEOUT = 30000;
const String WinService::REGISTRY_KEY("SYSTEM\\CurrentControlSet\\Services\\");
const String WinService::REGISTRY_DESCRIPTION("Description");

WinService::WinService(const String& name) : name_(name), svc_handle_(0) {
  scm_handle_ = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
  if (!scm_handle_) {
    throw SystemException("cannot Open Service Control Manager");
  }
}

WinService::~WinService() {
  Close();

  CloseServiceHandle(scm_handle_);
}

const String& WinService::GetName() const { return name_; }

String WinService::GetDisplayName() const {
  FUN_LPQUERY_SERVICE_CONFIG svc_config = GetConfig();
  UString udisplay_name(svc_config->lpDisplayName);
  String display_name = WCHAR_TO_UTF8(udisplay_name.c_str());
  LocalFree(svc_config);
  return display_name;
}

String WinService::GetPath() const {
  FUN_LPQUERY_SERVICE_CONFIG svc_config = GetConfig();
  UString upath(svc_config->lpBinaryPathName);
  String path = WCHAR_TO_UTF8(upath.c_str());
  LocalFree(svc_config);
  return path;
}

void WinService::RegisterService(const String& path,
                                 const String& display_name) {
  Close();

  UString uname = UString::FromUtf8(name_);
  UString udisplay_name = UString::FromUtf8(display_name);
  UString upath = UString::FromUtf8(path);
  svc_handle_ = CreateServiceW(
      scm_handle_, uname.c_str(), udisplay_name.c_str(), SERVICE_ALL_ACCESS,
      SERVICE_WIN32_OWN_PROCESS, SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
      upath.c_str(), NULL, NULL, NULL, NULL, NULL);
  if (!svc_handle_) {
    throw SystemException("cannot register service", name_);
  }
}

void WinService::RegisterService(const String& path) {
  RegisterService(path, name_);
}

void WinService::UnregisterService() {
  Open();

  if (!DeleteService(svc_handle_)) {
    throw SystemException("cannot unregister service", name_);
  }
}

bool WinService::IsRegistered() const { return TryOpen(); }

bool WinService::IsRunning() const {
  Open();

  SERVICE_STATUS ss;
  if (!QueryServiceStatus(svc_handle_, &ss)) {
    throw SystemException("cannot query service status", name_);
  }

  return ss.dwCurrentState == SERVICE_RUNNING;
}

void WinService::Start() {
  Open();

  if (!StartService(svc_handle_, 0, NULL)) {
    throw SystemException("cannot start service", name_);
  }

  SERVICE_STATUS svc_status;
  long msecs = 0;
  while (msecs < STARTUP_TIMEOUT) {
    if (!QueryServiceStatus(svc_handle_, &svc_status)) {
      break;
    }

    if (svc_status.dwCurrentState != SERVICE_START_PENDING) {
      break;
    }

    Thread::Sleep(250);
    msecs += 250;
  }

  if (!QueryServiceStatus(svc_handle_, &svc_status)) {
    throw SystemException("cannot query status of starting service", name_);
  } else if (svc_status.dwCurrentState != SERVICE_RUNNING) {
    throw SystemException("service failed to start within a reasonable time",
                          name_);
  }
}

void WinService::Stop() {
  Open();

  SERVICE_STATUS svc_status;
  if (!ControlService(svc_handle_, SERVICE_CONTROL_STOP, &svc_status)) {
    throw SystemException("cannot stop service", name_);
  }
}

void WinService::SetStartup(WinService::Startup startup) {
  Open();

  DWORD start_type;
  switch (startup) {
    case SVC_AUTO_START:
      start_type = SERVICE_AUTO_START;
      break;
    case SVC_MANUAL_START:
      start_type = SERVICE_DEMAND_START;
      break;
    case SVC_DISABLED:
      start_type = SERVICE_DISABLED;
      break;
    default:
      start_type = SERVICE_NO_CHANGE;
  }

  if (!ChangeServiceConfig(svc_handle_, SERVICE_NO_CHANGE, start_type,
                           SERVICE_NO_CHANGE, NULL, NULL, NULL, NULL, NULL,
                           NULL, NULL)) {
    throw SystemException("cannot change service startup mode");
  }
}

WinService::Startup WinService::GetStartup() const {
  FUN_LPQUERY_SERVICE_CONFIG svc_config = GetConfig();
  Startup result;
  switch (svc_config->dwStartType) {
    case SERVICE_AUTO_START:
    case SERVICE_BOOT_START:
    case SERVICE_SYSTEM_START:
      result = SVC_AUTO_START;
      break;
    case SERVICE_DEMAND_START:
      result = SVC_MANUAL_START;
      break;
    case SERVICE_DISABLED:
      result = SVC_DISABLED;
      break;
    default:
      fun_debugger();
      result = SVC_MANUAL_START;
  }
  LocalFree(svc_config);
  return result;
}

void WinService::SetDescription(const String& description) {
  String key(REGISTRY_KEY);
  key += name_;
  WinRegistryKey reg_key(HKEY_LOCAL_MACHINE, key);
  reg_key.SetString(REGISTRY_DESCRIPTION, description);
}

String WinService::GetDescription() const {
  String key(REGISTRY_KEY);
  key += name_;
  WinRegistryKey reg_key(HKEY_LOCAL_MACHINE, key, true);
  return reg_key.GetString(REGISTRY_DESCRIPTION);
}

void WinService::Open() const {
  if (!TryOpen()) {
    throw NotFoundException("service does not exist", name_);
  }
}

bool WinService::TryOpen() const {
  if (!svc_handle_) {
    UString uname = UString::FromUtf8(name_);
    svc_handle_ = OpenServiceW(scm_handle_, uname.c_str(), SERVICE_ALL_ACCESS);
  }

  return svc_handle_ != 0;
}

void WinService::Close() const {
  if (svc_handle_) {
    CloseServiceHandle(svc_handle_);
    svc_handle_ = 0;
  }
}

FUN_LPQUERY_SERVICE_CONFIG WinService::GetConfig() const {
  Open();

  int32 size = 4096;
  DWORD bytes_needed;
  FUN_LPQUERY_SERVICE_CONFIG svc_config =
      (FUN_LPQUERY_SERVICE_CONFIG)LocalAlloc(LPTR, size);
  if (!svc_config) {
    throw OutOfMemoryException("cannot allocate service config buffer");
  }

  try {
    while (!QueryServiceConfigW(svc_handle_, svc_config, size, &bytes_needed)) {
      if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        LocalFree(svc_config);
        size = bytes_needed;
        svc_config = (FUN_LPQUERY_SERVICE_CONFIG)LocalAlloc(LPTR, size);
      } else {
        throw SystemException("cannot query service configuration", name_);
      }
    }
  } catch (...) {
    LocalFree(svc_config);
    throw;
  }

  return svc_config;
}

}  // namespace framework
}  // namespace fun

#endif  // !defined(_WIN32_WCE)
