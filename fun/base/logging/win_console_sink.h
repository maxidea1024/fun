#pragma once

#include "fun/base/base.h"
#include "fun/base/logging/log_sink.h"
#include "fun/base/logging/log_message.h"

namespace fun {

/**
 * A sink that writes to the Windows console.
 *
 * Only the message's text is written, followed
 * by a newline.
 *
 * Log messages are assumed to be UTF-8 encoded, and
 * are converted to UTF-16 prior to writing them to the
 * console. This is the main difference to the ConsoleSink
 * class, which cannot handle UTF-8 encoded messages on Windows.
 *
 * Chain this sink to a FormattingSink with an
 * appropriate LogFormatter to control what is contained
 * in the text.
 *
 * Only available on Windows platforms.
 */
class FUN_BASE_API WinConsoleSink : public LogSink {
 public:
  /**
   * Creates the WindowsConsoleSink.
   */
  WinConsoleSink();

  // LogSink interface.
  void Log(const LogMessage& msg) override;

 protected:
  ~WinConsoleSink();

 private:
  HANDLE console_handle_;
  bool is_file_;
};

/**
 * A sink that writes to the Windows console.
 *
 * Only the message's text is written, followed by a newline.
 *
 * Log messages are assumed to be UTF-8 encoded, and
 * are converted to UTF-16 prior to writing them to the
 * console. This is the main difference to the ConsoleSink
 * class, which cannot handle UTF-8 encoded messages on Windows.
 *
 * Messages can be colored depending on priority.
 *
 * To enable message coloring, set the "EnableColors"
 * property to true (default). Furthermore, colors can be
 * configured by setting the following properties
 * (default values are given in parenthesis):
 *
 *   - TraceColor       (Gray)
 *   - DebugColor       (Gray)
 *   - InformationColor (Default)
 *   - NoticeColor      (Default)
 *   - WarningColor     (Yellow)
 *   - ErrorColor       (LightRed)
 *   - CriticalColor    (LightRed)
 *   - FatalColor       (LightRed)
 *
 * The following color values are supported:
 *
 *   - Default
 *   - Black
 *   - Red
 *   - Green
 *   - Brown
 *   - Blue
 *   - Magenta
 *   - Cyan
 *   - Gray
 *   - DarkGray
 *   - LightRed
 *   - LightGreen
 *   - Yellow
 *   - LightBlue
 *   - LightMagenta
 *   - LightCyan
 *   - White
 *
 * Chain this sink to a FormattingSink with an
 * appropriate LogFormatter to control what is contained
 * in the text.
 *
 * Only available on Windows platforms.
 */
class FUN_BASE_API WinColorConsoleSink : public LogSink {
 public:
  /**
   * Creates the WindowsConsoleSink.
   */
  WinColorConsoleSink();

  // LogSink interface.
  void Log(const LogMessage& msg) override;

  /**
   * Sets the property with the given name.
   *
   * The following properties are supported:
   *
   *   - EnableColors:      enable or disable colors.
   *   - TraceColor:        specify color for trace messages.
   *   - DebugColor:        specify color for debug messages.
   *   - InformationColor:  specify color for information messages.
   *   - NoticeColor:       specify color for notice messages.
   *   - WarningColor:      specify color for warning messages.
   *   - ErrorColor:        specify color for error messages.
   *   - CriticalColor:     specify color for critical messages.
   *   - FatalColor:        specify color for fatal messages.
   *
   * See the class documentation for a list of supported color values.
   */
  void SetProperty(const String& name, const String& value) override;

  /**
   * Returns the value of the property with the given name.
   * See SetProperty() for a description of the supported
   * properties.
   */
  String GetProperty(const String& name) const override;

 protected:
  enum Color {
    CC_BLACK        = 0x0000,
    CC_RED          = 0x0004,
    CC_GREEN        = 0x0002,
    CC_BROWN        = 0x0006,
    CC_BLUE         = 0x0001,
    CC_MAGENTA      = 0x0005,
    CC_CYAN         = 0x0003,
    CC_GRAY         = 0x0007,
    CC_DARKGRAY     = 0x0008,
    CC_LIGHTRED     = 0x000C,
    CC_LIGHTGREEN   = 0x000A,
    CC_YELLOW       = 0x000E,
    CC_LIGHTBLUE    = 0x0009,
    CC_LIGHTMAGENTA = 0x000D,
    CC_LIGHTCYAN    = 0x000B,
    CC_WHITE        = 0x000F
  };

  ~WinColorConsoleSink();

  WORD ParseColor(const String& color) const;
  String FormatColor(DWORD color) const;
  void InitColors();

 private:
  bool enable_colors_;
  HANDLE console_handle_;
  bool is_file_;
  WORD colors_[9];
};

} // namespace fun
