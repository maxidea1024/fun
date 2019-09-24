#include "fun/net/remote_syslog_sink.h"

namespace fun {
namespace net {

//
// MessageNotification
//

class MessageNotification : public Notification {
 public:
  MessageNotification(const char* buffer, size_t length,
                      const InetAddress& source_addr)
      : message_(buffer, length), source_addr_(source_addr) {}

  MessageNotification(const String& message, const InetAddress& source_addr)
      : message_(message), source_addr_(source_addr) {}

  ~MessageNotification() {}

  const String& GetMessage() const { return message_; }

  const InetAddress& GetSourceAddress() const { return source_addr_; }

 private:
  String message_;
  InetAddress source_addr_;
};

//
// RemoteUdpListener
//

class RemoteUdpListener : public Runnable {
 public:
  enum { WAITTIME_MILLISEC = 1000, BUFFER_SIZE = 65536 };

  RemoteUdpListener(NotificationQueue& queue, uint16 port);
  ~RemoteUdpListener();

  void Run();
  void SafeStop();

 private:
  NotificationQueue& queue_;
  DatagramSocket socket_;
  bool stopped_;
};

RemoteUdpListener::RemoteUdpListener(NotificationQueue& queue, uint16 port)
    : queue_(queue), socket_(InetAddress(port)), stopped_(false) {}

RemoteUdpListener::~RemoteUdpListener() {}

void RemoteUdpListener::Run() {
  Buffer<char> buffer(BUFFER_SIZE);
  Timespan wait_time = Timespan::FromMilliseconds(WAITTIME_MILLISEC);
  while (!stopped_) {
    try {
      if (socket_.Poll(wait_time, Socket::SELECT_READ)) {
        InetAddress source_addr;
        int n = socket_.ReceiveFrom(buffer.begin(), BUFFER_SIZE, source_addr);
        if (n > 0) {
          queue_.Enqueue(
              new MessageNotification(buffer.begin(), n, source_addr));
        }
      }
    } catch (...) {
      // lazy exception catching
    }
  }
}

void RemoteUdpListener::SafeStop() { stopped_ = true; }

//
// SyslogParser
//

class SyslogParser : public Runnable {
 public:
  static const String NIL_VALUE;

  enum { WAITTIME_MILLISEC = 1000 };

  SyslogParser(NotificationQueue& queue, RemoteSyslogListener* listener);
  ~SyslogParser();

  void Parse(const String& line, LogMessage& message);
  void Run();
  void SafeStop();

  static LogLevel::Type Convert(RemoteSyslogSink::Severity severity);

 private:
  void ParsePrio(const String& line, size_t& pos,
                 RemoteSyslogSink::Severity& severity,
                 RemoteSyslogSink::Facility& facility);

  void ParseNew(const String& line, RemoteSyslogSink::Severity severity,
                RemoteSyslogSink::Facility facility, size_t& pos,
                LogMessage& message);

  void ParseBsd(const String& line, RemoteSyslogSink::Severity severity,
                RemoteSyslogSink::Facility facility, size_t& pos,
                LogMessage& message);

  /**
   * Parses until it encounters the next space char, returns the string
   * from pos, excluding space pos will point past the space char
   */
  static String ParseUntilSpace(const String& line, size_t& pos);

  /**
   * Parses the structured data field.
   */
  static String ParseStructuredData(const String& line, size_t& pos);

  /**
   * Parses a token from the structured data field.
   */
  static String ParseStructuredDataToken(const String& line, size_t& pos);

 private:
  NotificationQueue& queue_;
  bool stopped_;
  RemoteSyslogListener* listener_;
};

const String SyslogParser::NIL_VALUE("-");

SyslogParser::SyslogParser(NotificationQueue& queue,
                           RemoteSyslogListener* listener)
    : queue_(queue), stopped_(false), listener_(listener) {
  fun_check_ptr(listener_);
}

SyslogParser::~SyslogParser() {}

void SyslogParser::Run() {
  while (!stopped_) {
    try {
      RefCountedPtr<Notification> noti(queue_.WaitDequeue(WAITTIME_MILLISEC));
      if (noti) {
        RefCountedPtr<MessageNotification> msg_noti =
            noti.Cast<MessageNotification>();
        LogMessage message;
        Parse(msg_noti->GetMessage(), message);
        message["addr"] = msg_noti->GetSourceAddress().GetHost().ToString();
        listener_->Log(message);
      }
    } catch (Exception&) {
      // parsing exception, what should we do?
    } catch (...) {
    }
  }
}

void SyslogParser::SafeStop() { stopped_ = true; }

void SyslogParser::Parse(const String& line, LogMessage& message) {
  // <int> -> int: lower 3 bits severity, upper bits: facility
  size_t pos = 0;
  RemoteSyslogSink::Severity severity;
  RemoteSyslogSink::Facility facility;
  ParsePrio(line, pos, severity, facility);

  // the next field decide if we Parse an old BSD message or a new syslog
  // message BSD: expects a month value in string form: Jan, Feb... SYSLOG
  // expects a version number: 1

  if (CharTraitsA::IsDigit(line[pos])) {
    ParseNew(line, severity, facility, pos, message);
  } else {
    ParseBsd(line, severity, facility, pos, message);
  }
  fun_check(pos == line.Len());
}

void SyslogParser::ParsePrio(const String& line, size_t& pos,
                             RemoteSyslogSink::Severity& severity,
                             RemoteSyslogSink::Facility& facility) {
  fun_check(pos < line.Len());
  fun_check(line[pos] == '<');
  ++pos;
  size_t start = pos;

  while (pos < line.Len() && CharTraitsA::IsDigit(line[pos])) {
    ++pos;
  }

  fun_check(line[pos] == '>');
  fun_check(pos - start > 0);
  String value_str = line.Mid(start, pos - start);
  ++pos;  // skip the >

  int val = NumberParser::Parse(value_str);
  fun_check(val >= 0 && val <= (RemoteSyslogSink::SYSLOG_LOCAL7 +
                                RemoteSyslogSink::SYSLOG_DEBUG));

  uint16 pri = static_cast<uint16>(val);
  // now get the lowest 3 bits
  severity = static_cast<RemoteSyslogSink::Severity>(pri & 0x0007u);
  facility = static_cast<RemoteSyslogSink::Facility>(pri & 0xfff8u);
}

void SyslogParser::ParseNew(const String& line,
                            RemoteSyslogSink::Severity severity,
                            RemoteSyslogSink::Facility /*facility*/,
                            size_t& pos, LogMessage& message) {
  LogLevel::Type level = Convert(severity);
  // rest of the unparsed header is:
  // VERSION SP TIMESTAMP SP HOSTNAME SP APP-NAME SP PROCID SP MSGID
  String version_str(ParseUntilSpace(line, pos));
  String time_str(ParseUntilSpace(line, pos));  // can be the nilvalue!
  String host_name(ParseUntilSpace(line, pos));
  String app_name(ParseUntilSpace(line, pos));
  String proc_id(ParseUntilSpace(line, pos));
  String msg_id(ParseUntilSpace(line, pos));
  String sd(ParseStructuredData(line, pos));
  String message_text(line.Mid(pos));
  pos = line.Len();
  DateTime date;
  int tzd = 0;
  bool has_date = DateTimeParser::TryParse(RemoteSyslogSink::SYSLOG_TIMEFORMAT,
                                           time_str, date, tzd);
  LogMessage log_entry(msg_id, message_text, level);
  log_entry[RemoteSyslogListener::LOG_PROP_HOST] = host_name;
  log_entry[RemoteSyslogListener::LOG_PROP_APP] = app_name;
  log_entry[RemoteSyslogListener::LOG_PROP_STRUCTURED_DATA] = sd;

  if (has_date) {
    log_entry.SetTime(date.timestamp());
  }

  int lval(0);
  NumberParser::TryParse(proc_id, lval);
  log_entry.SetPid(lval);
  message.Swap(log_entry);
}

void SyslogParser::ParseBsd(const String& line,
                            RemoteSyslogSink::Severity severity,
                            RemoteSyslogSink::Facility /*facility*/,
                            size_t& pos, LogMessage& message) {
  LogLevel::Type level = Convert(severity);
  // rest of the unparsed header is:
  // "%b %f %H:%M:%S" SP hostname|ipaddress
  // detect three spaces
  int space_count = 0;
  size_t start = pos;
  while (space_count < 3 && pos < line.Len()) {
    if (line[pos] == ' ') {
      space_count++;
      if (space_count == 1) {
        // size must be 3 chars for month
        if (pos - start != 3) {
          // probably a shortened time value, or the hostname
          // assume host_name
          LogMessage log_entry(line.Mid(start, pos - start), line.Mid(pos + 1),
                               level);
          message.Swap(log_entry);
          return;
        }
      } else if (space_count == 2) {
        // a day value!
        if (!(CharTraitsA::IsDigit(line[pos - 1]) &&
              (CharTraitsA::IsDigit(line[pos - 2]) ||
               CharTraitsA::IsSpace(line[pos - 2])))) {
          // assume the next field is a hostname
          space_count = 3;
        }
      }
      if (pos + 1 < line.Len() && line[pos + 1] == ' ') {
        // we have two spaces when the day value is smaller than 10!
        ++pos;  // skip one
      }
    }
    ++pos;
  }

  String time_str(line.Mid(start, pos - start - 1));
  int tzd(0);
  DateTime date;
  int year = date.year();  // year is not included, use the current one
  bool has_date = DateTimeParser::TryParse(RemoteSyslogSink::BSD_TIMEFORMAT,
                                           time_str, date, tzd);
  if (has_date) {
    int32 m = date.Month();
    int32 d = date.Day();
    int32 h = date.Hour();
    int32 min = date.Minute();
    int32 sec = date.Second();
    date = DateTime(year, m, d, h, min, sec);
  }
  // next entry is host SP
  String host_name(ParseUntilSpace(line, pos));

  // TAG: at most 32 alphanumeric chars, ANY non alphannumeric indicates start
  // of message content ignore: treat everything as content
  String message_text(line.Mid(pos));
  pos = line.Len();
  LogMessage log_entry(host_name, message_text, level);
  log_entry.SetTime(date.timestamp());
  message.Swap(log_entry);
}

String SyslogParser::ParseUntilSpace(const String& line, size_t& pos) {
  size_t start = pos;
  while (pos < line.Len() && !CharTraitsA::IsSpace(line[pos])) {
    ++pos;
  }
  // skip space
  ++pos;
  return line.Mid(start, pos - start - 1);
}

String SyslogParser::ParseStructuredData(const String& line, size_t& pos) {
  String sd;
  if (pos < line.Len()) {
    if (line[pos] == '-') {
      ++pos;
    } else if (line[pos] == '[') {
      String tok = ParseStructuredDataToken(line, pos);
      while (tok == "[") {
        sd += tok;
        tok = ParseStructuredDataToken(line, pos);
        while (tok != "]" && !tok.empty()) {
          sd += tok;
          tok = ParseStructuredDataToken(line, pos);
        }
        sd += tok;
        if (pos < line.Len() && line[pos] == '[') {
          tok = ParseStructuredDataToken(line, pos);
        }
      }
    }
    if (pos < line.Len() && CharTraitsA::IsSpace(line[pos])) {
      ++pos;
    }
  }
  return sd;
}

String SyslogParser::ParseStructuredDataToken(const String& line, size_t& pos) {
  String tok;
  if (pos < line.Len()) {
    if (CharTraitsA::IsSpace(line[pos]) || line[pos] == '=' ||
        line[pos] == '[' || line[pos] == ']') {
      tok += line[pos++];
    } else if (line[pos] == '"') {
      tok += line[pos++];
      while (pos < line.Len() && line[pos] != '"') {
        tok += line[pos++];
      }
      tok += '"';
      if (pos < line.Len()) {
        pos++;
      }
    } else {
      while (pos < line.Len() && !CharTraitsA::IsSpace(line[pos]) &&
             line[pos] != '=') {
        tok += line[pos++];
      }
    }
  }
  return tok;
}

LogLevel::Type SyslogParser::Convert(RemoteSyslogSink::Severity severity) {
  switch (severity) {
    case RemoteSyslogSink::SYSLOG_EMERGENCY:
      return LogLevel::Fatal;
    case RemoteSyslogSink::SYSLOG_ALERT:
      return LogLevel::Fatal;
    case RemoteSyslogSink::SYSLOG_CRITICAL:
      return LogLevel::Critical;
    case RemoteSyslogSink::SYSLOG_ERROR:
      return LogLevel::Error;
    case RemoteSyslogSink::SYSLOG_WARNING:
      return LogLevel::Warning;
    case RemoteSyslogSink::SYSLOG_NOTICE:
      return LogLevel::Notice;
    case RemoteSyslogSink::SYSLOG_INFORMATIONAL:
      return LogLevel::Information;
    case RemoteSyslogSink::SYSLOG_DEBUG:
      return LogLevel::Debug;
  }
  throw LogicException("Illegal severity value in message");
}

//
// RemoteSyslogListener
//

const String RemoteSyslogListener::PROP_PORT("Port");
const String RemoteSyslogListener::PROP_THREADS("Threads");

const String RemoteSyslogListener::LOG_PROP_APP("App");
const String RemoteSyslogListener::LOG_PROP_HOST("Host");
const String RemoteSyslogListener::LOG_PROP_STRUCTURED_DATA("structured-data");

RemoteSyslogListener::RemoteSyslogListener()
    : listener_(nullptr),
      parser_(nullptr),
      port_(RemoteSyslogSink::SYSLOG_PORT),
      thread_count_(1) {}

RemoteSyslogListener::RemoteSyslogListener(uint16 port)
    : listener_(nullptr), parser_(nullptr), port_(port), thread_count_(1) {}

RemoteSyslogListener::RemoteSyslogListener(uint16 port, int thread_count)
    : listener_(nullptr),
      parser_(nullptr),
      port_(port),
      thread_count_(thread_count) {}

RemoteSyslogListener::~RemoteSyslogListener() {}

void RemoteSyslogListener::ProcessMessage(const String& text) {
  LogMessage message;
  parser_->Parse(text, message);
  Log(message);
}

void RemoteSyslogListener::EnqueueMessage(const String& text,
                                          const InetAddress& sender_addr) {
  queue_.Enqueue(new MessageNotification(text, sender_addr));
}

void RemoteSyslogListener::SetProperty(const String& name,
                                       const String& value) {
  if (icompare(name, PROP_PORT) == 0) {
    int val = NumberParser::Parse(value);
    if (val >= 0 && val < 65536) {
      port_ = static_cast<uint16>(val);
    } else {
      throw InvalidArgumentException("Not a valid port number", value);
    }
  } else if (icompare(name, PROP_THREADS) == 0) {
    int val = NumberParser::Parse(value);
    if (val > 0 && val < 16) {
      thread_count_ = val;
    } else {
      throw InvalidArgumentException("Invalid number of threads", value);
    }
  } else {
    SplitterSink::SetProperty(name, value);
  }
}

String RemoteSyslogListener::GetProperty(const String& name) const {
  if (icompare(name, PROP_PORT) == 0) {
    return NumberFormatter::Format(port_);
  } else if (icompare(name, PROP_THREADS) == 0) {
    return NumberFormatter::Format(thread_count_);
  } else {
    return SplitterSink::GetProperty(name);
  }
}

void RemoteSyslogListener::Open() {
  SplitterSink::Open();

  parser_ = new SyslogParser(queue_, this);

  if (port_ > 0) {
    listener_ = new RemoteUdpListener(queue_, port_);
  }

  for (int32 i = 0; i < thread_count_; i++) {
    thread_pool_.Start(*parser_);
  }

  if (listener_) {
    thread_pool_.Start(*listener_);
  }
}

void RemoteSyslogListener::Close() {
  if (listener_) {
    listener_->SafeStop();
  }

  if (parser_) {
    parser_->SafeStop();
  }

  queue_.Clear();
  thread_pool_.JoinAll();
  delete listener_;
  delete parser_;
  listener_ = nullptr;
  parser_ = nullptr;

  SplitterSink::Close();
}

void RemoteSyslogListener::RegisterSink() {
  LoggingFactory::DefaultFactory().RegisterSinkClass(
      "RemoteSyslogListener", new Instantiator<RemoteSyslogListener, LogSink>);
}

}  // namespace net
}  // namespace fun
