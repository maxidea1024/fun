#pragma once

// 윈도우즈 플랫폼이 아닌 경우에만 사용됨.
#include <ostream>
#include "fun/base/base.h"
#include "fun/base/logging/log_sink.h"
#include "fun/base/mutex.h"

namespace fun {

/**
 * A sink that writes to an ostream.
 *
 * Only the message's text is written, followed
 * by a newline.
 *
 * Chain this sink to a FormattingSink with an
 * appropriate LogFormatter to control what is contained
 * in the text.
 *
 * Similar to StreamSink, except that a static
 * mutex is used to protect against multiple
 * console sinks concurrently writing to the
 * same stream.
 */
class FUN_BASE_API ConsoleSink : public LogSink {
 public:
  ConsoleSink();
  ConsoleSink(std::ostream& stream);

  void Log(const LogMessage& msg) override;

 protected:
  ~ConsoleSink();

 private:
  std::ostream& stream_;
  static FastMutex mutex_;
};

/**
 * A sink that writes to an ostream.
 *
 * Only the message's text is written, followed
 * by a newline.
 *
 * Messages can be colored depending on priority.
 * The console device must support ANSI escape codes
 * in order to display colored messages.
 *
 * To enable message coloring, set the "enablecolors"
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
 * Similar to StreamSink, except that a static
 * mutex is used to protect against multiple
 * console sinks concurrently writing to the
 * same stream.
 */
class FUN_BASE_API ColorConsoleSink : public LogSink {
 public:
  ColorConsoleSink();
  ColorConsoleSink(std::ostream& stream);

  void Log(const LogMessage& msg) override;

  void SetProperty(const String& name, const String& value) override;
  String GetProperty(const String& name) const override;

 protected:
  ~ColorConsoleSink();

  enum Color {
    CC_DEFAULT = 0x0027,
    CC_BLACK = 0x001e,
    CC_RED = 0x001f,
    CC_GREEN = 0x0020,
    CC_BROWN = 0x0021,
    CC_BLUE = 0x0022,
    CC_MAGENTA = 0x0023,
    CC_CYAN = 0x0024,
    CC_GRAY = 0x0025,
    CC_DARKGRAY = 0x011e,
    CC_LIGHTRED = 0x011f,
    CC_LIGHTGREEN = 0x0120,
    CC_YELLOW = 0x0121,
    CC_LIGHTBLUE = 0x0122,
    CC_LIGHTMAGENTA = 0x0123,
    CC_LIGHTCYAN = 0x0124,
    CC_WHITE = 0x0125
  };

  Color ParseColor(const String& color) const;
  String FormatColor(Color color) const;
  void InitColors();

 private:
  std::ostream& stream_;
  bool enable_colors_;
  Color colors_[9];
  static FastMutex mutex_;
  static const String CSI;
};

}  // namespace fun
