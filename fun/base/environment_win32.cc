#include "fun/base/environment_win32.h"
#include "fun/base/exception.h"
#include "fun/base/container/array.h"
#include "fun/base/string/string.h"
#include <sstream>
#include <cstring>
#include "fun/base/windows_less.h"
#include <winsock2.h>
#include <ws2ipdef.h>
#include <wincrypt.h>
#include <iphlpapi.h>

#ifdef _MSC_VER
#pragma warning(disable : 4996) // GetVerwionExW deprecated warning.
#endif

namespace fun {

String EnvironmentImpl::GetImpl(const String& name) {
  UString uname = UString::FromUtf8(name);
  DWORD len = GetEnvironmentVariableW(uname.c_str(), 0, 0);
  if (len == 0) {
    throw NotFoundException(name);
  }

  Array<wchar_t> buffer(len, NoInit);
  GetEnvironmentVariableW(uname.c_str(), buffer.MutableData(), len);
  String result;
  //UnicodeConverter::ToUtf8(buffer.begin(), len - 1, result);
  result = WCHAR_TO_UTF8_N(buffer.ConstData(), len - 1);
  return result;
}

bool EnvironmentImpl::HasImpl(const String& name) {
  UString uname = UString::FromUtf8(name);
  DWORD len = GetEnvironmentVariableW(uname.c_str(), 0, 0);
  return len > 0;
}

void EnvironmentImpl::SetImpl(const String& name, const String& value) {
  UString uname = UString::FromUtf8(name);
  UString uvalue = UString::FromUtf8(value);
  if (SetEnvironmentVariableW(uname.c_str(), uvalue.c_str()) == 0) {
    String msg = "cannot set environment variable: ";
    msg.Append(name);
    throw SystemException(msg);
  }
}

String EnvironmentImpl::GetOsNameImpl() {
  OSVERSIONINFO vi;
  vi.dwOSVersionInfoSize = sizeof(vi);
  if (GetVersionEx(&vi) == 0) {
    throw SystemException("Cannot get OS version information");
  }

  switch (vi.dwPlatformId) {
    case VER_PLATFORM_WIN32s: return "Windows 3.x";
    case VER_PLATFORM_WIN32_WINDOWS: return vi.dwMinorVersion == 0 ? "Windows 95" : "Windows 98";
    case VER_PLATFORM_WIN32_NT: return "Windows NT";
    default: return "Unknown";
  }
}

String EnvironmentImpl::GetOsDisplayNameImpl() {
  OSVERSIONINFOEX vi; // OSVERSIONINFOEX is supported starting at Windows 2000
  vi.dwOSVersionInfoSize = sizeof(vi);
  if (GetVersionEx((OSVERSIONINFO*)&vi) == 0) {
    throw SystemException("Cannot get OS version information");
  }

  switch (vi.dwMajorVersion) {
    case 10:
      switch (vi.dwMinorVersion) {
        case 0: return vi.wProductType == VER_NT_WORKSTATION ? "Windows 10" : "Windows Server 2016";
      }
    case 6:
      switch (vi.dwMinorVersion) {
        case 0: return vi.wProductType == VER_NT_WORKSTATION ? "Windows Vista" : "Windows Server 2008";
        case 1: return vi.wProductType == VER_NT_WORKSTATION ? "Windows 7" : "Windows Server 2008 R2";
        case 2: return vi.wProductType == VER_NT_WORKSTATION ? "Windows 8" : "Windows Server 2012";
        case 3: return vi.wProductType == VER_NT_WORKSTATION ? "Windows 8.1" : "Windows Server 2012 R2";
        default: return "Unknown";
      }
    case 5:
      switch (vi.dwMinorVersion) {
        case 0: return "Windows 2000";
        case 1: return "Windows XP";
        case 2: return "Windows Server 2003/Windows Server 2003 R2";
        default: return "Unknown";
      }
    default:
      return "Unknown";
  }
}

String EnvironmentImpl::GetOsVersionImpl() {
  OSVERSIONINFOW vi;
  vi.dwOSVersionInfoSize = sizeof(vi);
  if (GetVersionExW(&vi) == 0) {
    throw SystemException("Cannot get OS version information");
  }

  String result;
  result << (uint32)vi.dwMajorVersion << "." << (uint32)vi.dwMinorVersion << " (Build " << uint32(vi.dwBuildNumber & 0xFFFF);
  if (vi.szCSDVersion[0]) {
    result << WCHAR_TO_UTF8(vi.szCSDVersion);
  }
  result << ")";
  return result;
}

String EnvironmentImpl::GetOsArchitectureImpl() {
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  switch (si.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_INTEL: return "IA32";
    case PROCESSOR_ARCHITECTURE_MIPS: return "MIPS";
    case PROCESSOR_ARCHITECTURE_ALPHA: return "ALPHA";
    case PROCESSOR_ARCHITECTURE_PPC: return "PPC";
    case PROCESSOR_ARCHITECTURE_IA64: return "IA64";
#ifdef PROCESSOR_ARCHITECTURE_IA32_ON_WIN64
    case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64: return "IA64/32";
#endif
#ifdef PROCESSOR_ARCHITECTURE_AMD64
    case PROCESSOR_ARCHITECTURE_AMD64: return "AMD64";
#endif
    default: return "Unknown";
  }
}

// user name
String EnvironmentImpl::GetNodeNameImpl() {
  wchar_t name[MAX_COMPUTERNAME_LENGTH + 1];
  DWORD size = MAX_COMPUTERNAME_LENGTH + 1;
  if (GetComputerNameW(name, &size) == 0) {
    throw SystemException("Cannot get computer name");
  }

  String result = WCHAR_TO_UTF8(name);
  return result;
}

void EnvironmentImpl::GetNodeIdImpl(NodeId& out_id) {
  UnsafeMemory::Memset(&out_id, 0, sizeof(out_id));

  PIP_ADAPTER_INFO pAdapterInfo;
  PIP_ADAPTER_INFO pAdapter = 0;
  ULONG len = sizeof(IP_ADAPTER_INFO);
  pAdapterInfo = reinterpret_cast<IP_ADAPTER_INFO*>(new char[len]);
  // Make an initial call to GetAdaptersInfo to get
  // the necessary size into len
  DWORD rc = GetAdaptersInfo(pAdapterInfo, &len);
  if (rc == ERROR_BUFFER_OVERFLOW) {
    delete [] reinterpret_cast<char*>(pAdapterInfo);
    pAdapterInfo = reinterpret_cast<IP_ADAPTER_INFO*>(new char[len]);
  } else if (rc != ERROR_SUCCESS) {
    return;
  }
  
  if (GetAdaptersInfo(pAdapterInfo, &len) == NO_ERROR) {
    pAdapter = pAdapterInfo;
    bool found = false;
    while (pAdapter && !found) {
      if (pAdapter->Type == MIB_IF_TYPE_ETHERNET && pAdapter->AddressLength == sizeof(out_id)) {
        found = true;
        UnsafeMemory::Memcpy(&out_id, pAdapter->Address, pAdapter->AddressLength);
      }
      pAdapter = pAdapter->Next;
    }
  }
  delete [] reinterpret_cast<char*>(pAdapterInfo);
}

uint32 EnvironmentImpl::GetProcessorCountImpl() {
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  return si.dwNumberOfProcessors;
}

} // namespace fun
