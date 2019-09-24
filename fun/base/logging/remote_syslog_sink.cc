#include "fun/net/remote_syslog_sink.h"

namespace fun {

// TODO?
// FUN_IMPLEMENT_RTCLASS(RemoteSyslogSink)

const String RemoteSyslogSink::BSD_TIMEFORMAT("%b %f %H:%M:%S");
const String RemoteSyslogSink::SYSLOG_TIMEFORMAT("%Y-%m-%dT%H:%M:%S.%i%z");
const String RemoteSyslogSink::PROP_NAME("Name");
const String RemoteSyslogSink::PROP_FACILITY("Facility");
const String RemoteSyslogSink::PROP_FORMAT("Format");
const String RemoteSyslogSink::PROP_LOGHOST("LogHost");
const String RemoteSyslogSink::PROP_HOST("Host");
const String RemoteSyslogSink::STRUCTURED_DATA("structured-data");

RemoteSyslogSink::RemoteSyslogSink()
    : log_host_("localhost"),
      name_("-"),
      facility_(SYSLOG_USER),
      bsd_format_(false),
      is_opened_(false) {}

RemoteSyslogSink::RemoteSyslogSink(const String& address, const String& name,
                                   int facility, bool bsd_format)
    : log_host_(address),
      name_(name),
      facility_(facility),
      bsd_format_(bsd_format),
      is_opened_(false) {
  if (name_.IsEmpty()) {
    name_ = "-";
  }
}

RemoteSyslogSink::~RemoteSyslogSink() {
  try {
    Close();
  } catch (...) {
    fun_unexpected();
  }
}

void RemoteSyslogSink::Open() {
  if (is_opened_) return;

  if (log_host_.find(':') != String::npos) {
    socket_addr_ = InetAddress(log_host_);
  } else {
    socket_addr_ = InetAddress(log_host_, SYSLOG_PORT);
  }

  // reset socket for the case that it has been previously closed
  socket_ = DatagramSocket(socket_addr_.family());

  if (host_.IsEmpty()) {
    try {
      host_ = Dns::ThisHost().GetName();
    } catch (fun::Exception&) {
      host_ = socket_.GetAddress().GetHost().ToString();
    }
  }

  is_opened_ = true;
}

void RemoteSyslogSink::Close() {
  if (is_opened_) {
    socket_.Close();
    is_opened_ = false;
  }
}

void RemoteSyslogSink::Log(const LogMessage& msg) {
  fun::FastMutex::ScopedLock guard(mutex_);

  if (!is_opened_) {
    Open();
  }

  String m;
  m.Reserve(1024);
  m += '<';
  fun::NumberFormatter::Append(m, GetPrio(msg) + facility_);
  m += '>';
  if (bsd_format_) {
    fun::DateTimeFormatter::Append(m, msg.GetTime(), BSD_TIMEFORMAT);
    m += ' ';
    m += host_;
  } else {
    m += "1 ";  // version
    fun::DateTimeFormatter::Append(m, msg.GetTime(), SYSLOG_TIMEFORMAT);
    m += ' ';
    m += host_;
    m += ' ';
    m += name_;
    m += ' ';
    fun::NumberFormatter::Append(m, static_cast<fun::UInt64>(msg.GetPid()));
    m += ' ';
    m += msg.GetSource();
    m += ' ';
    if (msg.Has(STRUCTURED_DATA)) {
      m += msg.Get(STRUCTURED_DATA);
    } else {
      m += "-";
    }
  }
  m += ' ';
  m += msg.GetText();

  socket_.SendTo(m.data(), static_cast<int>(m.size()), socket_addr_);
}

void RemoteSyslogSink::SetProperty(const String& name, const String& value) {
  if (name == PROP_NAME) {
    name_ = value;
    if (name_.IsEmpty()) {
      name_ = "-";
    }
  } else if (name == PROP_FACILITY) {
    String facility;
    if (fun::icompare(value, 4, "LOG_") == 0) {
      facility = fun::ToUpper(value.substr(4));
    } else if (fun::icompare(value, 4, "SYSLOG_") == 0) {
      facility = fun::ToUpper(value.substr(7));
    } else {
      facility = fun::ToUpper(value);
    }

    if (facility == "KERN") {
      facility_ = SYSLOG_KERN;
    } else if (facility == "USER") {
      facility_ = SYSLOG_USER;
    } else if (facility == "MAIL") {
      facility_ = SYSLOG_MAIL;
    } else if (facility == "DAEMON") {
      facility_ = SYSLOG_DAEMON;
    } else if (facility == "AUTH") {
      facility_ = SYSLOG_AUTH;
    } else if (facility == "AUTHPRIV") {
      facility_ = SYSLOG_AUTHPRIV;
    } else if (facility == "SYSLOG") {
      facility_ = SYSLOG_SYSLOG;
    } else if (facility == "LPR") {
      facility_ = SYSLOG_LPR;
    } else if (facility == "NEWS") {
      facility_ = SYSLOG_NEWS;
    } else if (facility == "UUCP") {
      facility_ = SYSLOG_UUCP;
    } else if (facility == "CRON") {
      facility_ = SYSLOG_CRON;
    } else if (facility == "FTP") {
      facility_ = SYSLOG_FTP;
    } else if (facility == "NTP") {
      facility_ = SYSLOG_NTP;
    } else if (facility == "LOGAUDIT") {
      facility_ = SYSLOG_LOGAUDIT;
    } else if (facility == "LOGALERT") {
      facility_ = SYSLOG_LOGALERT;
    } else if (facility == "CLOCK") {
      facility_ = SYSLOG_CLOCK;
    } else if (facility == "LOCAL0") {
      facility_ = SYSLOG_LOCAL0;
    } else if (facility == "LOCAL1") {
      facility_ = SYSLOG_LOCAL1;
    } else if (facility == "LOCAL2") {
      facility_ = SYSLOG_LOCAL2;
    } else if (facility == "LOCAL3") {
      facility_ = SYSLOG_LOCAL3;
    } else if (facility == "LOCAL4") {
      facility_ = SYSLOG_LOCAL4;
    } else if (facility == "LOCAL5") {
      facility_ = SYSLOG_LOCAL5;
    } else if (facility == "LOCAL6") {
      facility_ = SYSLOG_LOCAL6;
    } else if (facility == "LOCAL7") {
      facility_ = SYSLOG_LOCAL7;
    }
  } else if (name == PROP_LOGHOST) {
    log_host_ = value;
  } else if (name == PROP_HOST) {
    host_ = value;
  } else if (name == PROP_FORMAT) {
    bsd_format_ = (value == "bsd" || value == "rfc3164");
  } else {
    Sink::SetProperty(name, value);
  }
}

String RemoteSyslogSink::GetProperty(const String& name) const {
  if (name == PROP_NAME) {
    if (name_ != "-") {
      return name_;
    } else {
      return "";
    }
  } else if (name == PROP_FACILITY) {
    if (facility_ == SYSLOG_KERN) {
      return "KERN";
    } else if (facility_ == SYSLOG_USER) {
      return "USER";
    } else if (facility_ == SYSLOG_MAIL) {
      return "MAIL";
    } else if (facility_ == SYSLOG_DAEMON) {
      return "DAEMON";
    } else if (facility_ == SYSLOG_AUTH) {
      return "AUTH";
    } else if (facility_ == SYSLOG_AUTHPRIV) {
      return "AUTHPRIV";
    } else if (facility_ == SYSLOG_SYSLOG) {
      return "SYSLOG";
    } else if (facility_ == SYSLOG_LPR) {
      return "LPR";
    } else if (facility_ == SYSLOG_NEWS) {
      return "NEWS";
    } else if (facility_ == SYSLOG_UUCP) {
      return "UUCP";
    } else if (facility_ == SYSLOG_CRON) {
      return "CRON";
    } else if (facility_ == SYSLOG_FTP) {
      return "FTP";
    } else if (facility_ == SYSLOG_NTP) {
      return "NTP";
    } else if (facility_ == SYSLOG_LOGAUDIT) {
      return "LOGAUDIT";
    } else if (facility_ == SYSLOG_LOGALERT) {
      return "LOGALERT";
    } else if (facility_ == SYSLOG_CLOCK) {
      return "CLOCK";
    } else if (facility_ == SYSLOG_LOCAL0) {
      return "LOCAL0";
    } else if (facility_ == SYSLOG_LOCAL1) {
      return "LOCAL1";
    } else if (facility_ == SYSLOG_LOCAL2) {
      return "LOCAL2";
    } else if (facility_ == SYSLOG_LOCAL3) {
      return "LOCAL3";
    } else if (facility_ == SYSLOG_LOCAL4) {
      return "LOCAL4";
    } else if (facility_ == SYSLOG_LOCAL5) {
      return "LOCAL5";
    } else if (facility_ == SYSLOG_LOCAL6) {
      return "LOCAL6";
    } else if (facility_ == SYSLOG_LOCAL7) {
      return "LOCAL7";
    } else {
      return "";
    }
  } else if (name == PROP_LOGHOST) {
    return log_host_;
  } else if (name == PROP_HOST) {
    return host_;
  } else if (name == PROP_FORMAT) {
    return bsd_format_ ? "rfc3164" : "rfc5424";
  } else {
    return Sink::GetProperty(name);
  }
}

int RemoteSyslogSink::GetPrio(const LogMessage& msg) {
  switch (msg.GetLevel()) {
    case LogLevel::Trace:
    case LogLevel::Debug:
      return SYSLOG_DEBUG;
    case LogLevel::Information:
      return SYSLOG_INFORMATIONAL;
    case LogLevel::Notice:
      return SYSLOG_NOTICE;
    case LogLevel::Warning:
      return SYSLOG_WARNING;
    case LogLevel::Error:
      return SYSLOG_ERROR;
    case LogLevel::Critical:
      return SYSLOG_CRITICAL;
    case LogLevel::Fatal:
      return SYSLOG_ALERT;
    default:
      return 0;
  }
}

void RemoteSyslogSink::RegisterSink() {
  fun::LoggingFactory::DefaultFactory().RegisterSinkClass(
      "RemoteSyslogSink", new fun::Instantiator<RemoteSyslogSink, fun::Sink>);
}

}  // namespace fun
