#include "fun/base/path_win32.h"
#include "fun/base/container/array.h"
#include "fun/base/environment_win32.h"
#include "fun/base/exception.h"
#include "fun/base/windows_less.h"

namespace fun {

String PathImpl::GetCurrentImpl() {
  String result;
  DWORD len = GetCurrentDirectoryW(0, NULL);
  if (len > 0) {
    Array<wchar_t> buffer(len, NoInit);
    DWORD n = GetCurrentDirectoryW(len, buffer.MutableData());
    if (n > 0 && n <= len) {
      result = WCHAR_TO_UTF8(buffer.ConstData());
      if (result[result.Len() - 1] != '\\') {
        result.Append("\\");
      }
      return result;
    }
  }
  throw SystemException("Cannot get current directory");
}

String PathImpl::GetSystemImpl() {
  Array<wchar_t> buffer(MAX_PATH_LEN, NoInit);
  DWORD n = GetSystemDirectoryW(buffer.MutableData(),
                                static_cast<DWORD>(buffer.Count()));
  if (n > 0) {
    n = GetLongPathNameW(buffer.MutableData(), buffer.MutableData(),
                         static_cast<DWORD>(buffer.Count()));
    if (n <= 0) {
      throw SystemException("Cannot get system directory long path name");
    }

    String result = WCHAR_TO_UTF8(buffer.ConstData());
    if (result[result.Len() - 1] != '\\') {
      result.Append("\\");
    }
    return result;
  }
  throw SystemException("Cannot get temporary directory path");
}

String PathImpl::GetHomeImpl() {
  String result;
  if (EnvironmentImpl::HasImpl("USERPROFILE")) {
    result = EnvironmentImpl::GetImpl("USERPROFILE");
  } else if (EnvironmentImpl::HasImpl("HOMEDRIVE") &&
             EnvironmentImpl::HasImpl("HOMEPATH")) {
    result = EnvironmentImpl::GetImpl("HOMEDRIVE");
    result.Append(EnvironmentImpl::GetImpl("HOMEPATH"));
  } else {
    result = GetSystemImpl();
  }

  int32 n = result.Len();
  if (n > 0 && result[n - 1] != '\\') {
    result.Append("\\");
  }
  return result;
}

String PathImpl::GetConfigHomeImpl() {
  String result;

  // if APPDATA environment variable no exist, return home directory instead
  try {
    result = EnvironmentImpl::GetImpl("APPDATA");
  } catch (NotFoundException&) {
    result = GetHomeImpl();
  }

  int32 n = result.Len();
  if (n > 0 && result[n - 1] != '\\') {
    result.Append("\\");
  }
  return result;
}

String PathImpl::GetDataHomeImpl() {
  String result;

  // if LOCALAPPDATA environment variable no exist, return config home instead
  try {
    result = EnvironmentImpl::GetImpl("LOCALAPPDATA");
  } catch (NotFoundException&) {
    result = GetConfigHomeImpl();
  }

  int32 n = result.Len();
  if (n > 0 && result[n - 1] != '\\') {
    result.Append("\\");
  }
  return result;
}

String PathImpl::GetCacheHomeImpl() { return GetTempImpl(); }

String PathImpl::GetSelfImpl() {
  Array<wchar_t> buffer(MAX_PATH_LEN, NoInit);
  DWORD n = GetModuleFileNameW(NULL, buffer.MutableData(),
                               static_cast<DWORD>(buffer.Count()));
  DWORD err = GetLastError();
  if (n > 0) {
    if (err == ERROR_INSUFFICIENT_BUFFER) {
      throw SystemException("Buffer too small to get executable name.");
    }

    String result = WCHAR_TO_UTF8(buffer.ConstData());
    return result;
  }
  throw SystemException("Cannot get executable name.");
}

String PathImpl::GetTempImpl() {
  Array<wchar_t> buffer(MAX_PATH_LEN, NoInit);
  DWORD n =
      GetTempPathW(static_cast<DWORD>(buffer.Count()), buffer.MutableData());
  if (n > 0) {
    n = GetLongPathNameW(buffer.MutableData(), buffer.MutableData(),
                         static_cast<DWORD>(buffer.Count()));
    if (n <= 0) {
      throw SystemException("Cannot get temporary directory long path name");
    }

    String result = WCHAR_TO_UTF8(buffer.ConstData());
    if (result[result.Len() - 1] != '\\') {
      result.Append("\\");
    }
    return result;
  }
  throw SystemException("Cannot get temporary directory path");
}

String PathImpl::GetConfigImpl() {
  String result;

  // if PROGRAMDATA environment variable not exist, return system directory
  // instead
  try {
    result = EnvironmentImpl::GetImpl("PROGRAMDATA");
  } catch (NotFoundException&) {
    result = GetSystemImpl();
  }

  int32 n = result.Len();
  if (n > 0 && result[n - 1] != '\\') {
    result.Append("\\");
  }
  return result;
}

String PathImpl::GetNullImpl() { return "NUL:"; }

String PathImpl::ExpandImpl(const String& path) {
  UString upath = UString::FromUtf8(path);
  Array<wchar_t> buffer(MAX_PATH_LEN, NoInit);
  DWORD n = ExpandEnvironmentStringsW(upath.c_str(), buffer.MutableData(),
                                      static_cast<DWORD>(buffer.Count()));
  if (n > 0 && n < buffer.Count() - 1) {
    buffer[n + 1] = 0;
    String result = WCHAR_TO_UTF8(buffer.ConstData());
    return result;
  } else {
    return path;
  }
}

void PathImpl::ListRootsImpl(Array<String>& roots) {
  roots.Clear();

  const int BUFFER_LENGTH = 128;
  wchar_t buffer[BUFFER_LENGTH];
  DWORD n = GetLogicalDriveStringsW(BUFFER_LENGTH - 1, buffer);
  wchar_t* it = buffer;
  wchar_t* end = buffer + (n > BUFFER_LENGTH ? BUFFER_LENGTH : n);
  while (it < end) {
    UString udev;
    while (it < end && *it) {
      udev += *it++;
    }

    String dev = WCHAR_TO_UTF8(udev.c_str());
    roots.Add(dev);
    ++it;
  }
}

}  // namespace fun
