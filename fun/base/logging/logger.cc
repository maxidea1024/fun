#include "fun/base/logging/logger.h"

#include "fun/base/exception.h"
#include "fun/base/logging/log_formatter.h"
#include "fun/base/logging/logging_registry.h"
//#include "fun/base/number_formatter.h"
//#include "fun/base/number_parser.h"
#include "fun/base/str.h"
#include "fun/base/string/string.h"

namespace fun {

Logger::Logger(const String& name, LogSink::Ptr sink, LogLevel::Type level)
    : name_(name), sink_(sink), level_(level) {}

Logger::~Logger() {
  // NOOP
}

void Logger::SetSink(LogSink::Ptr sink) { sink_ = sink; }

LogSink::Ptr Logger::GetSink() const { return sink_; }

void Logger::SetLevel(LogLevel::Type level) { level_ = level; }

void Logger::SetLevel(const String& level) { SetLevel(ParseLevel(level)); }

void Logger::SetProperty(const String& name, const String& value) {
  if (icompare(name, "sink") == 0) {
    SetSink(LoggingRegistry::DefaultRegistry().SinkForName(value));
  } else if (icompare(name, "level") == 0) {
    SetLevel(value);
  } else {
    LogSink::SetProperty(name, value);
  }
}

void Logger::Log(const LogMessage& msg) {
  if (level_ >= msg.GetLevel() && sink_) {
    sink_->Log(msg);
  }
}

void Logger::Log(const Exception& e) { LogError(e.GetDisplayText()); }

void Logger::Log(const Exception& e, const char* file, int line) {
  LogError(e.GetDisplayText(), file, line);
}

void Logger::Dump(const String& msg, const void* buffer, size_t length,
                  LogLevel::Type level) {
  if (level_ >= level && sink_) {
    String text(msg);
    FormatDump(text, buffer, length);
    sink_->Log(LogMessage(name_, text, level));
  }
}

void Logger::SetLevel(const String& name, LogLevel::Type level) {
  ScopedLock<Mutex> lock(logger_map_mutex_);

  if (logger_map_) {
    int32 len = name.Len();
    for (auto& pair : *logger_map_) {
      if (len == 0 || (pair.key.MidRef(0, len) == name &&
                       (pair.key.Len() == len || pair.key[len] == '.'))) {
        pair.value->SetLevel(level);
      }
    }
  }
}

void Logger::SetSink(const String& name, LogSink::Ptr sink) {
  ScopedLock<Mutex> lock(logger_map_mutex_);

  if (logger_map_) {
    int32 len = name.Len();
    for (auto& pair : *logger_map_) {
      if (len == 0 || (pair.key.MidRef(0, len) == name &&
                       (pair.key.Len() == len || pair.key[len] == '.'))) {
        pair.value->SetSink(sink);
      }
    }
  }
}

void Logger::SetProperty(const String& logger_name, const String& property_name,
                         const String& value) {
  ScopedLock<Mutex> lock(logger_map_mutex_);

  if (logger_map_) {
    int32 len = logger_name.Len();
    for (auto& pair : *logger_map_) {
      if (len == 0 || (pair.key.MidRef(0, len) == logger_name &&
                       (pair.key.Len() == len || pair.key[len] == '.'))) {
        pair.value->SetProperty(property_name, value);
      }
    }
  }
}

//
// Formatting
//

// String Logger::Format(const String& fmt, const String& arg) {
//  String args[] = { arg };
//  return Format(fmt, 1, args);
//}
//
// String Logger::Format(const String& fmt, const String& arg0, const String&
// arg1) {
//  String args[] = { arg0, arg1 };
//  return Format(fmt, 2, args);
//}
//
// String Logger::Format(const String& fmt, const String& arg0, const String&
// arg1, const String& arg2) {
//  String args[] = { arg0, arg1, arg2 };
//  return Format(fmt, 3, args);
//}
//
// String Logger::Format(const String& fmt, const String& arg0, const String&
// arg1, const String& arg2, const String& arg3) {
//  String args[] = { arg0, arg1, arg2, arg3 };
//  return Format(fmt, 4, args);
//}
//
// String Logger::Format(const String& fmt, int argc, String argv[]) {
//  String result;
//  String::const_iterator it = fmt.begin();
//  while (it != fmt.end()) {
//    if (*it == '$') {
//      ++it;
//      if (*it == '$') {
//        result += '$';
//      } else if (*it >= '0' && *it <= '9') {
//        int32 i = *it - '0';
//        if (i < argc)
//          result += argv[i];
//      } else {
//        result += '$';
//        result += *it;
//      }
//    } else {
//      result += *it;
//    }
//    ++it;
//  }
//  return result;
//}

void Logger::FormatDump(String& message, const void* buffer, size_t length) {
  // TODO

  fun_check(0);

  // const int BYTES_PER_LINE = 16;
  //
  // message.Reserve(message.Len() + length*6);
  // if (!message.IsEmpty()) {
  //  message.Append("\n");
  //}
  //
  // unsigned char* base = (unsigned char*) buffer;
  // int addr = 0;
  // while (addr < length) {
  //  if (addr > 0) {
  //    message.Append("\n");
  //  }
  //
  //  message.Append(NumberFormatter::FormatHex(addr, 4));
  //  message.Append("  ");
  //  int offset = 0;
  //  while (addr + offset < length && offset < BYTES_PER_LINE) {
  //    message.Append(NumberFormatter::FormatHex(base[addr + offset], 2));
  //    message.Append(offset == 7 ? "  " : " ");
  //    ++offset;
  //  }
  //  if (offset < 7) message.Append(" ");
  //  while (offset < BYTES_PER_LINE) {
  //    message.Append("   ");
  //    ++offset;
  //  }
  //  message.Append(" ");
  //  offset = 0;
  //  while (addr + offset < length && offset < BYTES_PER_LINE) {
  //    unsigned char c = base[addr + offset];
  //    message += (c >= 32 && c < 127) ? (char)c : '.';
  //    ++offset;
  //  }
  //  addr += BYTES_PER_LINE;
  //}
}

Logger& Logger::Get(const String& name) {
  ScopedLock<Mutex> lock(logger_map_mutex_);
  return UnsafeGet(name);
}

Logger& Logger::UnsafeGet(const String& name) {
  Ptr logger = Find(name);
  if (!logger) {
    if (name == ROOT) {
      logger = new Logger(name, 0, LogLevel::Information);
    } else {
      Logger& par = GetParent(name);
      logger = new Logger(name, par.GetSink(), par.GetLevel());
    }
    Add(logger);
  }
  return *logger;
}

Logger& Logger::Create(const String& name, LogSink::Ptr sink,
                       LogLevel::Type level) {
  ScopedLock<Mutex> lock(logger_map_mutex_);

  if (Find(name)) {
    throw ExistsException();
  }

  Ptr logger = new Logger(name, sink, level);
  Add(logger);
  return *logger;
}

Logger& Logger::Root() {
  ScopedLock<Mutex> lock(logger_map_mutex_);

  return UnsafeGet(ROOT);
}

Logger::Ptr Logger::Has(const String& name) {
  ScopedLock<Mutex> lock(logger_map_mutex_);

  return Find(name);
}

void Logger::Shutdown() {
  ScopedLock<Mutex> lock(logger_map_mutex_);

  if (logger_map_) {
    logger_map_->Reset();
  }
}

Logger::Ptr Logger::Find(const String& name) {
  if (logger_map_) {
    return logger_map_->FindRef(name);
  }

  return nullptr;
}

void Logger::Destroy(const String& name) {
  ScopedLock<Mutex> lock(logger_map_mutex_);

  if (logger_map_) {
    logger_map_->Remove(name);
  }
}

void Logger::Names(Array<String>& names) {
  ScopedLock<Mutex> lock(logger_map_mutex_);

  names.Clear();

  if (logger_map_) {
    for (const auto& pair : *logger_map_) {
      names.Add(pair.key);
    }
  }
}

Logger& Logger::GetParent(const String& name) {
  int32 pos = name.LastIndexOf('.');
  if (pos != INVALID_INDEX) {
    String pname = name.Mid(0, pos);
    Ptr parent = Find(pname);
    if (parent) {
      return *parent;
    } else {
      return GetParent(pname);
    }
  } else {
    return UnsafeGet(ROOT);
  }
}

LogLevel::Type Logger::ParseLevel(const String& level) {
  if (icompare(level, "None") == 0) {
    return 0;
  } else if (icompare(level, "Fatal") == 0) {
    return LogLevel::Fatal;
  } else if (icompare(level, "Critical") == 0) {
    return LogLevel::Critical;
  } else if (icompare(level, "Error") == 0) {
    return LogLevel::Error;
  } else if (icompare(level, "Warning") == 0) {
    return LogLevel::Warning;
  } else if (icompare(level, "Notice") == 0) {
    return LogLevel::Notice;
  } else if (icompare(level, "Information") == 0) {
    return LogLevel::Information;
  } else if (icompare(level, "Debug") == 0) {
    return LogLevel::Debug;
  } else if (icompare(level, "Trace") == 0) {
    return LogLevel::Trace;
  } else {
    bool ok = false;
    int32 level_value = level.ToInt32(&ok);
    if (ok) {
      if (level_value > 0 && level_value < 9) {
        return level_value;
      } else {
        throw InvalidArgumentException("Log level out of range ", level);
      }
    } else {
      throw InvalidArgumentException("Not a valid Log level", level);
    }
  }
}

void Logger::Add(Ptr logger) {
  if (!logger_map_) {
    logger_map_.Reset(new LoggerMap);
  }

  logger_map_->Add(logger->GetName(), logger);
}

}  // namespace fun
