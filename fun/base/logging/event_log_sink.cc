#include "fun/base/logging/event_log_sink.h"
#include "fun/base/exception.h"
#include "fun/base/logging/log_message.h"
#include "fun/base/str.h"
#include "fun/base/string/string.h"

#if FUN_WITH_EVENT_LOG_SINK

// TODO
//#include "fun_msg.h"

namespace fun {

const String EventLogSink::PROP_NAME = "Name";
const String EventLogSink::PROP_HOST = "Host";
const String EventLogSink::PROP_LOGHOST = "LogHost";
const String EventLogSink::PROP_LOGFILE = "LogFile";

EventLogSink::EventLogSink() : log_file_("Application"), handle_(0) {
  const DWORD MAX_PATH_LEN = MAX_PATH + 1;
  wchar_t name[MAX_PATH_LEN];
  int n = GetModuleFileNameW(NULL, name, MAX_PATH_LEN);
  if (n > 0) {
    wchar_t* end = name + n - 1;
    while (end > name && *end != '\\') {
      --end;
    }
    if (*end == '\\') {
      ++end;
    }
    UString uname(end);
    name_ = WCHAR_TO_UTF8(uname.c_str());
  }
}

EventLogSink::EventLogSink(const String& name)
    : name_(name), log_file_("Application"), handle_(0) {}

EventLogSink::EventLogSink(const String& name, const String& host)
    : name_(name), host_(host), log_file_("Application"), handle_(0) {}

EventLogSink::~EventLogSink() {
  try {
    Close();
  } catch (...) {
    fun_unexpected();
  }
}

void EventLogSink::Open() {
  SetupRegistry();
  UString uhost = UString::FromUtf8(host_);
  UString uname = UString::FromUtf8(name_);
  handle_ = RegisterEventSourceW(uhost.IsEmpty() ? NULL : uhost.c_str(),
                                 uname.c_str());
  if (!handle_) {
    throw SystemException("cannot register event source");
  }
}

void EventLogSink::Close() {
  if (handle_) {
    DeregisterEventSource(handle_);
  }
  handle_ = 0;
}

void EventLogSink::Log(const LogMessage& msg) {
  if (!handle_) {
    Open();
  }

  // TODO
  // UString utext = UString::FromUtf8(msg.GetText());
  // const wchar_t* umsg = utext.c_str();
  // ReportEventW(handle_,
  //    static_cast<WORD>(GetType(msg)),
  //    static_cast<WORD>(GetCategory(msg)),
  //    FUN_MSG_LOG,
  //    NULL,
  //    1,
  //    0,
  //    &umsg,
  //    NULL);
}

void EventLogSink::SetProperty(const String& name, const String& value) {
  if (icompare(name, PROP_NAME) == 0) {
    name_ = value;
  } else if (icompare(name, PROP_HOST) == 0) {
    host_ = value;
  } else if (icompare(name, PROP_LOGHOST) == 0) {
    host_ = value;
  } else if (icompare(name, PROP_LOGFILE) == 0) {
    log_file_ = value;
  } else {
    LogSink::SetProperty(name, value);
  }
}

String EventLogSink::GetProperty(const String& name) const {
  if (icompare(name, PROP_NAME) == 0) {
    return name_;
  } else if (icompare(name, PROP_HOST) == 0) {
    return host_;
  } else if (icompare(name, PROP_LOGHOST) == 0) {
    return host_;
  } else if (icompare(name, PROP_LOGFILE) == 0) {
    return log_file_;
  } else {
    return LogSink::GetProperty(name);
  }
}

int32 EventLogSink::GetType(const LogMessage& msg) {
  switch (msg.GetLevel()) {
    case LogLevel::Trace:
    case LogLevel::Debug:
    case LogLevel::Information:
      return EVENTLOG_INFORMATION_TYPE;
    case LogLevel::Notice:
    case LogLevel::Warning:
      return EVENTLOG_WARNING_TYPE;
    default:
      return EVENTLOG_ERROR_TYPE;
  }
}

int32 EventLogSink::GetCategory(const LogMessage& msg) {
  // TODO
  // switch (msg.GetLevel()) {
  //  case LogLevel::Trace:
  //    return FUN_CTG_TRACE;
  //  case LogLevel::Debug:
  //    return FUN_CTG_DEBUG;
  //  case LogLevel::Information:
  //    return FUN_CTG_INFORMATION;
  //  case LogLevel::Notice:
  //    return FUN_CTG_NOTICE;
  //  case LogLevel::Warning:
  //    return FUN_CTG_WARNING;
  //  case LogLevel::Error:
  //    return FUN_CTG_ERROR;
  //  case LogLevel::Critical:
  //    return FUN_CTG_CRITICAL;
  //  case LogLevel::Fatal:
  //    return FUN_CTG_FATAL;
  //  default:
  //    return 0;
  //}
  return 0;
}

void EventLogSink::SetupRegistry() const {
  String key = "SYSTEM\\CurrentControlSet\\Services\\EventLog\\";
  key.Append(log_file_);
  key.Append("\\");
  key.Append(name_);
  HKEY key_handle;
  DWORD disp;
  UString ukey = UString::FromUtf8(key);
  DWORD rc = RegCreateKeyExW(HKEY_LOCAL_MACHINE, ukey.c_str(), 0, NULL,
                             REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                             &key_handle, &disp);
  if (rc != ERROR_SUCCESS) {
    return;
  }

  if (disp == REG_CREATED_NEW_KEY) {
    UString path;
#if defined(FUN_DLL)
#if defined(_DEBUG)
#if defined(_WIN64)
    path = FindLibrary(L"fun-base64d.dll");
#else
    path = FindLibrary(L"fun-based.dll");
#endif
#else
#if defined(_WIN64)
    path = FindLibrary(L"fun-base64.dll");
#else
    path = FindLibrary(L"fun-base.dll");
#endif
#endif
#endif

    if (path.IsEmpty()) {
      path = FindLibrary(L"fun-msg.dll");
    }

    if (!path.IsEmpty()) {
      DWORD count = 8;
      DWORD types = 7;
      RegSetValueExW(key_handle, L"CategoryMessageFile", 0, REG_SZ,
                     (const BYTE*)path.c_str(),
                     static_cast<DWORD>(sizeof(wchar_t) * (path.Len() + 1)));
      RegSetValueExW(key_handle, L"EventMessageFile", 0, REG_SZ,
                     (const BYTE*)path.c_str(),
                     static_cast<DWORD>(sizeof(wchar_t) * (path.Len() + 1)));
      RegSetValueExW(key_handle, L"CategoryCount", 0, REG_DWORD,
                     (const BYTE*)&count, static_cast<DWORD>(sizeof(count)));
      RegSetValueExW(key_handle, L"TypesSupported", 0, REG_DWORD,
                     (const BYTE*)&types, static_cast<DWORD>(sizeof(types)));
    }
  }
  RegCloseKey(key_handle);
}

UString EventLogSink::FindLibrary(const wchar_t* name) {
  UString path;
  HMODULE dll = LoadLibraryW(name);
  if (dll) {
    const DWORD MAX_PATH_LEN = MAX_PATH + 1;
    wchar_t module_name[MAX_PATH_LEN];
    int n = GetModuleFileNameW(dll, module_name, MAX_PATH_LEN);
    if (n > 0) {
      path = module_name;
    }
    FreeLibrary(dll);
  }
  return path;
}

}  // namespace fun

#endif  // FUN_WITH_EVENT_LOG_SINK
