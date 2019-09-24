#include "fun/base/debug.h"
#include "fun/base/debugger.h"
#include "fun/base/exception.h"
#include "fun/base/ndc.h"

//#include <sstream>

namespace fun {

void Debug::Assertion(const char* cond, const char* file, int32 line,
                      const char* text) {
  String msg("Assertion violation: ");
  msg += cond;
  if (text) {
    msg += " (";
    msg += text;
    msg += ")";
  }

  Debugger::Enter(msg, file, line);
  throw AssertionViolationException(What(cond, file, line, text));
}

void Debug::NullPointer(const char* ptr, const char* file, int32 line) {
  Debugger::Enter(String("NULL pointer: ") + ptr, file, line);
  throw NullPointerException(What(ptr, file, line));
}

void Debug::Bugcheck(const char* file, int32 line) {
  Debugger::Enter("Bugcheck", file, line);
  throw BugcheckException(What(0, file, line));
}

void Debug::Bugcheck(const char* msg, const char* file, int32 line) {
  String str("Bugcheck");
  if (msg) {
    str += ": ";
    str += msg;
  }
  Debugger::Enter(str, file, line);
  throw BugcheckException(What(msg, file, line));
}

void Debug::Unexpected(const char* file, int32 line) {
#if defined(_DEBUG)
  try {
    String msg("Unexpected exception in noexcept function or destructor: ");
    try {
      throw;  // rethrow...??
    } catch (Exception& e) {
      msg += e.GetDisplayText();
    } catch (...) {
      msg += "unknown exception";
    }

    Debugger::Enter(msg, file, line);
  } catch (...) {
    // sink..
  }
#endif
}

void Debug::Debugger(const char* file, int32 line) {
  Debugger::Enter(file, line);
}

void Debug::Debugger(const char* msg, const char* file, int32 line) {
  Debugger::Enter(msg, file, line);
}

String Debug::What(const char* msg, const char* file, int32 line,
                   const char* text) {
  String str;

  if (msg) {
    str << msg << " ";
  }

  if (text) {
    str << "(" << text << ") ";
  }

  str << "in file \"" << file << "\", line " << line;

#if FUN_EXCEPTION_BACKTRACE
  str << "\n" << Ndc::Backtrace(3, 2);
#endif  // FUN_EXCEPTION_BACKTRACE
  return str;
}

}  // namespace fun
