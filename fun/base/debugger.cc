#include "fun/base/debugger.h"
#include "fun/base/ndc.h"  // Nested Debugging Context
#include "fun/base/windows_less.h"

#include <cstdio>
#include <cstdlib>
#include <sstream>

//#include "fun/base/unicode_converter.h"

namespace fun {

bool Debugger::IsPresent() {
#if defined(_DEBUG)
#if FUN_PLATFORM_WINDOWS_FAMILY
#if defined(_WIN32_WCE)
#if (_WIN32_WCE >= 0x600)
  BOOL is_debugger_present;
  if (CheckRemoteDebuggerPresent(GetCurrentProcess(), &is_debugger_present)) {
    return is_debugger_present ? true : false;
  }
  return false;
#else
  return false;
#endif
#else
  return IsDebuggerPresent() ? true : false;
#endif
#elif FUN_PLATFORM_UNIX_FAMILY
  return std::getenv("FUN_ENABLE_DEBUGGER") ? true : false;
#endif
#else
  return false;
#endif
}

void Debugger::Message(const String& msg, bool backtrace) {
#if defined(_DEBUG)
  std::fputs("\n%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n", stderr);
  String msg2 = msg;
  if (backtrace) {
    msg2.Append(1, '\n').Append(Ndc::Backtrace(5, 1));
  }
  std::fputs(msg2.ConstData(), stderr);
  std::fputs("\n%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n", stderr);

#if FUN_PLATFORM_WINDOWS_FAMILY
  if (IsPresent()) {
    UString umsg = UString::FromUtf8(msg);
    umsg += UTEXT("\n");
    OutputDebugStringW(umsg.c_str());
  }
#endif  // defined(FUN_PLATFORM_WINDOWS_FAMILY)
#endif  // defined(_DEBUG)
}

void Debugger::Message(const String& msg, const char* file, int32 line) {
#if defined(_DEBUG)
  String str;
  str << msg << " [in file \"" << file << "\", line " << line << "]";
  Message(str);
#endif
}

void Debugger::Enter() {
#if defined(_DEBUG)
#if FUN_PLATFORM_WINDOWS_FAMILY
  if (IsPresent()) {
    DebugBreak();
  }
#elif FUN_PLATFORM_UNIX_FAMILY
  if (IsPresent()) {
    kill(getpid(), SIGINT);
  }
#endif
#endif
}

void Debugger::Enter(const String& msg) {
#if defined(_DEBUG)
  Message(msg);
  Enter();
#endif
}

void Debugger::Enter(const String& msg, const char* file, int32 line) {
#if defined(_DEBUG)
  Message(msg, file, line);
  Enter();
#endif
}

void Debugger::Enter(const String& msg, int32 line) {
#if defined(_DEBUG)
  Message("BREAK", nullptr, line);
  Enter();
#endif
}

}  // namespace fun
