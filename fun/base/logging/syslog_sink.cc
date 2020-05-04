#include "fun/base/logging/syslog_sink.h"

#if FUN_PLATFORM_UNIX_FAMILY

#include <syslog.h>

#include "fun/base/logging/log_message.h"

namespace fun {

const String SyslogSink::PROP_NAME = "Name";
const String SyslogSink::PROP_FACILITY = "Facility";
const String SyslogSink::PROP_OPTIONS = "Options";

SyslogSink::SyslogSink()
    : options_(SYSLOG_CONS), facility_(SYSLOG_USER), opened_(false) {}

SyslogSink::SyslogSink(const String& name, int options, int facility)
    : name_(name), options_(options), facility_(facility), opened_(false) {}

SyslogSink::~SyslogSink() { Close(); }

void SyslogSink::Open() {
  openlog(name_.c_str(), options_, facility_);
  opened_ = true;
}

void SyslogSink::Close() {
  if (opened_) {
    closelog();
    opened_ = false;
  }
}

void SyslogSink::Log(const LogMessage& msg) {
  if (!opened_) {
    Open();
  }

  syslog(GetPrio(msg), "%s", msg.GetText().c_str());
}

void SyslogSink::SetProperty(const String& name, const String& value) {
  if (icompare(name, PROP_NAME) == 0) {
    name_ = value;
  } else if (icompare(name, PROP_FACILITY) == 0) {
    if (icompare(value, "LOG_KERN") == 0) {
      facility_ = SYSLOG_KERN;
    } else if (icompare(value, "LOG_USER") == 0) {
      facility_ = SYSLOG_USER;
    } else if (icompare(value, "LOG_MAIL") == 0) {
      facility_ = SYSLOG_MAIL;
    } else if (icompare(value, "LOG_DAEMON") == 0) {
      facility_ = SYSLOG_DAEMON;
    } else if (icompare(value, "LOG_AUTH") == 0) {
      facility_ = SYSLOG_AUTH;
    } else if (icompare(value, "LOG_AUTHPRIV") == 0) {
      facility_ = SYSLOG_AUTHPRIV;
    } else if (icompare(value, "LOG_SYSLOG") == 0) {
      facility_ = SYSLOG_SYSLOG;
    } else if (icompare(value, "LOG_LPR") == 0) {
      facility_ = SYSLOG_LPR;
    } else if (icompare(value, "LOG_NEWS") == 0) {
      facility_ = SYSLOG_NEWS;
    } else if (icompare(value, "LOG_UUCP") == 0) {
      facility_ = SYSLOG_UUCP;
    } else if (icompare(value, "LOG_CRON") == 0) {
      facility_ = SYSLOG_CRON;
    } else if (icompare(value, "LOG_FTP") == 0) {
      facility_ = SYSLOG_FTP;
    } else if (icompare(value, "LOG_LOCAL0") == 0) {
      facility_ = SYSLOG_LOCAL0;
    } else if (icompare(value, "LOG_LOCAL1") == 0) {
      facility_ = SYSLOG_LOCAL1;
    } else if (icompare(value, "LOG_LOCAL2") == 0) {
      facility_ = SYSLOG_LOCAL2;
    } else if (icompare(value, "LOG_LOCAL3") == 0) {
      facility_ = SYSLOG_LOCAL3;
    } else if (icompare(value, "LOG_LOCAL4") == 0) {
      facility_ = SYSLOG_LOCAL4;
    } else if (icompare(value, "LOG_LOCAL5") == 0) {
      facility_ = SYSLOG_LOCAL5;
    } else if (icompare(value, "LOG_LOCAL6") == 0) {
      facility_ = SYSLOG_LOCAL6;
    } else if (icompare(value, "LOG_LOCAL7") == 0) {
      facility_ = SYSLOG_LOCAL7;
    }
  } else if (icompare(name, PROP_OPTIONS) == 0) {
    options_ = 0;
    StringTokenizer tokenizer(
        value, "|+:;,",
        StringTokenizer::TOK_IGNORE_EMPTY | StringTokenizer::TOK_TRIM);
    for (StringTokenizer::Iterator it = tokenizer.begin();
         it != tokenizer.end(); ++it) {
      if (icompare(*it, "LOG_CONS") == 0) {
        options_ |= SYSLOG_CONS;
      } else if (icompare(*it, "LOG_NDELAY") == 0) {
        options_ |= SYSLOG_NDELAY;
      } else if (icompare(*it, "LOG_PERROR") == 0) {
        options_ |= SYSLOG_PERROR;
      } else if (icompare(*it, "LOG_PID") == 0) {
        options_ |= SYSLOG_PID;
      }
    }
  } else {
    LogSink::SetProperty(name, value);
  }
}

String SyslogSink::GetProperty(const String& name) const {
  if (icompare(name, PROP_NAME) == 0) {
    return name_;
  } else if (icompare(name, PROP_FACILITY) == 0) {
    if (facility_ == SYSLOG_KERN) {
      return "LOG_KERN";
    } else if (facility_ == SYSLOG_USER) {
      return "LOG_USER";
    } else if (facility_ == SYSLOG_MAIL) {
      return "LOG_MAIL";
    } else if (facility_ == SYSLOG_DAEMON) {
      return "LOG_DAEMON";
    } else if (facility_ == SYSLOG_AUTH) {
      return "LOG_AUTH";
    } else if (facility_ == SYSLOG_AUTHPRIV) {
      return "LOG_AUTHPRIV";
    } else if (facility_ == SYSLOG_SYSLOG) {
      return "LOG_SYSLOG";
    } else if (facility_ == SYSLOG_LPR) {
      return "LOG_LPR";
    } else if (facility_ == SYSLOG_NEWS) {
      return "LOG_NEWS";
    } else if (facility_ == SYSLOG_UUCP) {
      return "LOG_UUCP";
    } else if (facility_ == SYSLOG_CRON) {
      return "LOG_CRON";
    } else if (facility_ == SYSLOG_FTP) {
      return "LOG_FTP";
    } else if (facility_ == SYSLOG_LOCAL0) {
      return "LOG_LOCAL0";
    } else if (facility_ == SYSLOG_LOCAL1) {
      return "LOG_LOCAL1";
    } else if (facility_ == SYSLOG_LOCAL2) {
      return "LOG_LOCAL2";
    } else if (facility_ == SYSLOG_LOCAL3) {
      return "LOG_LOCAL3";
    } else if (facility_ == SYSLOG_LOCAL4) {
      return "LOG_LOCAL4";
    } else if (facility_ == SYSLOG_LOCAL5) {
      return "LOG_LOCAL5";
    } else if (facility_ == SYSLOG_LOCAL6) {
      return "LOG_LOCAL6";
    } else if (facility_ == SYSLOG_LOCAL7) {
      return "LOG_LOCAL7";
    } else {
      return "";
    }
  } else if (icompare(name, PROP_OPTIONS) == 0) {
    String result;
    if (options_ & SYSLOG_CONS) {
      if (!result.IsEmpty()) result.Append("|");
      result.Append("LOG_CONS");
    }
    if (options_ & SYSLOG_NDELAY) {
      if (!result.IsEmpty()) result.Append("|");
      result.Append("LOG_NDELAY");
    }
    if (options_ & SYSLOG_PERROR) {
      if (!result.IsEmpty()) result.Append("|");
      result.Append("LOG_PERROR");
    }
    if (options_ & SYSLOG_PID) {
      if (!result.IsEmpty()) result.Append("|");
      result.Append("LOG_PID");
    }
    return result;
  } else {
    return LogSink::GetProperty(name);
  }
}

int SyslogSink::GetPrio(const LogMessage& msg) {
  switch (msg.GetLevel()) {
    case LogLevel::Trace:
    case LogLevel::Debug:
      return LOG_DEBUG;
    case LogLevel::Information:
      return LOG_INFO;
    case LogLevel::Notice:
      return LOG_NOTICE;
    case LogLevel::Warning:
      return LOG_WARNING;
    case LogLevel::Error:
      return LOG_ERR;
    case LogLevel::Critical:
      return LOG_CRIT;
    case LogLevel::Fatal:
      return LOG_ALERT;
    default:
      return 0;
  }
}

}  // namespace fun

#endif  // FUN_PLATFORM_UNIX_FAMILY
