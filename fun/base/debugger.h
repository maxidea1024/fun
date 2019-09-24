#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * The Debugger class provides an interface to the debugger.
 * The presence of a debugger can be checked for,
 * messages can be written to the debugger's log window
 * and a break into the debugger can be enforced.
 * The methods only work if the program is compiled
 * in debug mode (the macro _DEBUG is defined).
 */
class FUN_BASE_API Debugger {
 public:
  /**
   * Returns true if a debugger is available, false otherwise.
   * On Windows, this function uses the IsDebuggerPresent()
   * function.
   * On Unix, this function returns true if the environment
   * variable FUN_ENABLE_DEBUGGER is set.
   */
  static bool IsPresent();

  /**
   * Writes a message to the debugger log, if available, otherwise to
   * standard error output.
   */
  static void Message(const String& msg, bool backtrace = true);

  /**
   * Writes a message to the debugger log, if available, otherwise to
   * standard error output.
   */
  static void Message(const String& msg, const char* file, int32 line);

  /**
   * Breaks into the debugger, if it is available.
   * On Windows, this is done using the DebugBreak() function.
   * On Unix, the SIGINT signal is raised.
   */
  static void Enter();

  /**
   * Writes a debug message to the debugger log and breaks into it.
   */
  static void Enter(const String& msg);

  /**
   * Writes a debug message to the debugger log and breaks into it.
   */
  static void Enter(const String& msg, const char* file, int32 line);

  /**
   * Writes a debug message to the debugger log and breaks into it.
   */
  static void Enter(const String& msg, int32 line);
};

}  // namespace fun
