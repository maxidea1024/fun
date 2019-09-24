#include "fun/base/logging/console_sink.h"
#include "fun/base/logging/log_message.h"
#include "fun/base/str.h"
#include <iostream>

namespace fun {

//
// ConsoleSink
//

FastMutex ConsoleSink::mutex_;

ConsoleSink::ConsoleSink() : stream_(std::clog) {}

ConsoleSink::ConsoleSink(std::ostream& stream) : stream_(stream) {}

ConsoleSink::~ConsoleSink() {}

void ConsoleSink::Log(const LogMessage& msg) {
  ScopedLock<FastMutex> guard(mutex_);

  stream_ << msg.GetText().c_str() << std::endl;
}


//
// ColorConsoleSink
//

FastMutex ColorConsoleSink::mutex_;
const String ColorConsoleSink::CSI("\033[");

ColorConsoleSink::ColorConsoleSink()
  : stream_(std::clog), enable_colors_(true) {
  InitColors();
}

ColorConsoleSink::ColorConsoleSink(std::ostream& stream)
  : stream_(stream), enable_colors_(true) {
  InitColors();
}

void ColorConsoleSink::Log(const LogMessage& msg) {
  ScopedLock<FastMutex> guard(mutex_);

  if (enable_colors_) {
    int32 color = colors_[msg.GetLevel()];
    if (color & 0x100) {
      stream_ << CSI.c_str() << "1m";
    }
    color &= 0xff;
    stream_ << CSI.c_str() << "m";
  }

  stream_ << msg.GetText().c_str();

  if (enable_colors_) {
    stream_ << CSI.c_str() << "0m";
  }

  stream_ << std::endl;
}

void ColorConsoleSink::SetProperty(const String& name, const String& value) {
  if (icompare(name, "EnableColors") == 0) {
    enable_colors_ = icompare(value, "True") == 0;
  } else if (icompare(name, "TraceColor") == 0) {
    colors_[LogLevel::Trace] = ParseColor(value);
  } else if (icompare(name, "DebugColor") == 0) {
    colors_[LogLevel::Debug] = ParseColor(value);
  } else if (icompare(name, "InformationColor") == 0) {
    colors_[LogLevel::Information] = ParseColor(value);
  } else if (icompare(name, "NoticeColor") == 0) {
    colors_[LogLevel::Notice] = ParseColor(value);
  } else if (icompare(name, "WarningColor") == 0) {
    colors_[LogLevel::Warning] = ParseColor(value);
  } else if (icompare(name, "ErrorColor") == 0) {
    colors_[LogLevel::Error] = ParseColor(value);
  } else if (icompare(name, "CriticalColor") == 0) {
    colors_[LogLevel::Critical] = ParseColor(value);
  } else if (icompare(name, "FatalColor") == 0) {
    colors_[LogLevel::Fatal] = ParseColor(value);
  } else {
    LogSink::SetProperty(name, value);
  }
}

String ColorConsoleSink::GetProperty(const String& name) const {
  if (icompare(name, "EnableColors") == 0) {
    return enable_colors_ ? "True" : "False";
  } else if (icompare(name, "TraceColor") == 0) {
    return FormatColor(colors_[LogLevel::Trace]);
  } else if (icompare(name, "DebugColor") == 0) {
    return FormatColor(colors_[LogLevel::Debug]);
  } else if (icompare(name, "InformationColor") == 0) {
    return FormatColor(colors_[LogLevel::Information]);
  } else if (icompare(name, "NoticeColor") == 0) {
    return FormatColor(colors_[LogLevel::Notice]);
  } else if (icompare(name, "WarningColor") == 0) {
    return FormatColor(colors_[LogLevel::Warning]);
  } else if (icompare(name, "ErrorColor") == 0) {
    return FormatColor(colors_[LogLevel::Error]);
  } else if (icompare(name, "CriticalColor") == 0) {
    return FormatColor(colors_[LogLevel::Critical]);
  } else if (icompare(name, "FatalColor") == 0) {
    return FormatColor(colors_[LogLevel::Fatal]);
  } else {
    return LogSink::GetProperty(name);
  }
}

ColorConsoleSink::~ColorConsoleSink() {}

ColorConsoleSink::Color
ColorConsoleSink::ParseColor(const String& color) const {
  if (icompare(color, "Default") == 0) {
    return CC_DEFAULT;
  } else if (icompare(color, "Black") == 0) {
    return CC_BLACK;
  } else if (icompare(color, "Red") == 0) {
    return CC_RED;
  } else if (icompare(color, "Green") == 0) {
    return CC_GREEN;
  } else if (icompare(color, "Brown") == 0) {
    return CC_BROWN;
  } else if (icompare(color, "Blue") == 0) {
    return CC_BLUE;
  } else if (icompare(color, "Magenta") == 0) {
    return CC_MAGENTA;
  } else if (icompare(color, "Cyan") == 0) {
    return CC_CYAN;
  } else if (icompare(color, "Gray") == 0) {
    return CC_GRAY;
  } else if (icompare(color, "DarkGray") == 0) {
    return CC_DARKGRAY;
  } else if (icompare(color, "LightRed") == 0) {
    return CC_LIGHTRED;
  } else if (icompare(color, "LightGreen") == 0) {
    return CC_LIGHTGREEN;
  } else if (icompare(color, "Yellow") == 0) {
    return CC_YELLOW;
  } else if (icompare(color, "LightBlue") == 0) {
    return CC_LIGHTBLUE;
  } else if (icompare(color, "LightMagenta") == 0) {
    return CC_LIGHTMAGENTA;
  } else if (icompare(color, "LightCyan") == 0) {
    return CC_LIGHTCYAN;
  } else if (icompare(color, "White") == 0) {
    return CC_WHITE;
  } else {
    throw InvalidArgumentException("Invalid color value", color);
  }
}

String ColorConsoleSink::FormatColor(Color color) const {
  switch (color) {
    case CC_DEFAULT:      return "Default";
    case CC_BLACK:        return "Black";
    case CC_RED:          return "Red";
    case CC_GREEN:        return "Green";
    case CC_BROWN:        return "Brown";
    case CC_BLUE:         return "Blue";
    case CC_MAGENTA:      return "Magenta";
    case CC_CYAN:         return "Cyan";
    case CC_GRAY:         return "Gray";
    case CC_DARKGRAY:     return "DarkGray";
    case CC_LIGHTRED:     return "LightRed";
    case CC_LIGHTGREEN:   return "LightGreen";
    case CC_YELLOW:       return "Yellow";
    case CC_LIGHTBLUE:    return "LightBlue";
    case CC_LIGHTMAGENTA: return "LightMagenta";
    case CC_LIGHTCYAN:    return "LightCyan";
    case CC_WHITE:        return "White";
    default:              return "Invalid";
  }
}

void ColorConsoleSink::InitColors() {
  colors_[0] = CC_DEFAULT; // unused
  colors_[LogLevel::Fatal]       = CC_LIGHTRED;
  colors_[LogLevel::Critical]    = CC_LIGHTRED;
  colors_[LogLevel::Error]       = CC_LIGHTRED;
  colors_[LogLevel::Warning]     = CC_YELLOW;
  colors_[LogLevel::Notice]      = CC_DEFAULT;
  colors_[LogLevel::Information] = CC_DEFAULT;
  colors_[LogLevel::Debug]       = CC_GRAY;
  colors_[LogLevel::Trace]       = CC_GRAY;
}

} // namespace fun
