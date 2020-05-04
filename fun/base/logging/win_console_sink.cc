#include "fun/base/logging/win_console_sink.h"

#include "fun/base/exception.h"
#include "fun/base/str.h"

namespace fun {

//
// WinConsoleSink
//

WinConsoleSink::WinConsoleSink()
    : console_handle_(INVALID_HANDLE_VALUE), is_file_(false) {
  console_handle_ = GetStdHandle(STD_OUTPUT_HANDLE);

  DWORD mode;
  is_file_ = (GetConsoleMode(console_handle_, &mode) == 0);
}

WinConsoleSink::~WinConsoleSink() {
  // NOOP
}

void WinConsoleSink::Log(const LogMessage& msg) {
  String text = msg.GetText();
  text << "\r\n";

  if (is_file_) {
    DWORD written;
    WriteFile(console_handle_, text.ConstData(), static_cast<DWORD>(text.Len()),
              &written, NULL);
  } else {
    UString utext = UString::FromUtf8(text);

    DWORD written;
    WriteConsoleW(console_handle_, utext.ConstData(),
                  static_cast<DWORD>(utext.Len()), &written, NULL);
  }
}

//
// WinColorConsoleSink
//

WinColorConsoleSink::WinColorConsoleSink()
    : console_handle_(INVALID_HANDLE_VALUE),
      is_file_(false),
      enable_colors_(true) {
  console_handle_ = GetStdHandle(STD_OUTPUT_HANDLE);

  DWORD mode;
  is_file_ = (GetConsoleMode(console_handle_, &mode) == 0);
}

WinColorConsoleSink::~WinColorConsoleSink() {
  // NOOP
}

void WinColorConsoleSink::Log(const LogMessage& msg) {
  String text = msg.GetText();
  text << "\r\n";

  if (enable_colors_ && !is_file_) {
    WORD attr = colors_[0];
    attr &= 0xFFF0;
    attr |= colors_[msg.GetLevel()];
    SetConsoleTextAttribute(console_handle_, attr);
  }

  if (is_file_) {
    DWORD written;
    WriteFile(console_handle_, text.ConstData(), static_cast<DWORD>(text.Len()),
              &written, NULL);
  } else {
    UString utext = UString::FromUtf8(text);

    DWORD written;
    WriteConsoleW(console_handle_, utext.ConstData(),
                  static_cast<DWORD>(utext.Len()), &written, NULL);
  }

  if (enable_colors_ && !is_file_) {
    SetConsoleTextAttribute(console_handle_, colors_[0]);
  }
}

void WinColorConsoleSink::SetProperty(const String& name, const String& value) {
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

String WinColorConsoleSink::GetProperty(const String& name) const {
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

WORD WinColorConsoleSink::ParseColor(const String& color) const {
  if (icompare(color, "Default") == 0) {
    return colors_[0];
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

String WinColorConsoleSink::FormatColor(DWORD color) const {
  switch (color) {
    case CC_BLACK:
      return "Black";
    case CC_RED:
      return "Red";
    case CC_GREEN:
      return "Green";
    case CC_BROWN:
      return "Brown";
    case CC_BLUE:
      return "Blue";
    case CC_MAGENTA:
      return "Magenta";
    case CC_CYAN:
      return "Cyan";
    case CC_GRAY:
      return "Gray";
    case CC_DARKGRAY:
      return "DarkGray";
    case CC_LIGHTRED:
      return "LightRed";
    case CC_LIGHTGREEN:
      return "LightGreen";
    case CC_YELLOW:
      return "Yellow";
    case CC_LIGHTBLUE:
      return "LightBlue";
    case CC_LIGHTMAGENTA:
      return "LightMagenta";
    case CC_LIGHTCYAN:
      return "LightCyan";
    case CC_WHITE:
      return "White";
    default:
      return "Invalid";
  }
}

void WinColorConsoleSink::InitColors() {
  if (!is_file_) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(console_handle_, &csbi);
    colors_[0] = csbi.wAttributes;
  } else {
    colors_[0] = CC_WHITE;
  }

  colors_[LogLevel::Fatal] = CC_LIGHTRED;
  colors_[LogLevel::Critical] = CC_LIGHTRED;
  colors_[LogLevel::Error] = CC_LIGHTRED;
  colors_[LogLevel::Warning] = CC_YELLOW;
  colors_[LogLevel::Notice] = colors_[0];
  colors_[LogLevel::Information] = colors_[0];
  colors_[LogLevel::Debug] = CC_GRAY;
  colors_[LogLevel::Trace] = CC_GRAY;
}

}  // namespace fun
