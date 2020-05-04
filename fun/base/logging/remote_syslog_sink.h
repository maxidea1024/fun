#pragma once

#include "fun/base/logging/log_sink.h"
#include "fun/base/mutex.h"
#include "fun/net/inet_address.h"
#include "fun/net/net.h"
#include "fun/net/udp_socket.h"

namespace fun {
namespace net {

/**
 * This Sink implements remote syslog logging over UDP according
 * to RFC 5424 "The Syslog Protocol"
 * and RFC 5426 "Transmission of syslog messages over UDP".
 *
 * In addition, RemoteSyslogListener also supports the "old" BSD syslog
 * protocol, as described in RFC 3164.
 *
 * RFC 5425 structured data can be passed via the "structured-data"
 * property of the log LogMessage. The content of the "structured-data"
 * property must be correct according to RFC 5425.
 *
 * Example:
 *     msg.Set("structured-data", "[exampleSDID@32473 iut=\"3\"
 * eventSource=\"Application\" eventID=\"1011\"]");
 */
class FUN_BASE_API RemoteSyslogSink : public LogSink {
  static const String BSD_TIMEFORMAT;
  static const String SYSLOG_TIMEFORMAT;

  enum Severity {
    SYSLOG_EMERGENCY = 0,      /// Emergency: system is unusable
    SYSLOG_ALERT = 1,          /// Alert: action must be taken immediately
    SYSLOG_CRITICAL = 2,       /// Critical: critical conditions
    SYSLOG_ERROR = 3,          /// Error: error conditions
    SYSLOG_WARNING = 4,        /// Warning: warning conditions
    SYSLOG_NOTICE = 5,         /// Notice: normal but significant condition
    SYSLOG_INFORMATIONAL = 6,  /// Informational: informational messages
    SYSLOG_DEBUG = 7           /// Debug: debug-level messages
  };

  enum Facility {
    SYSLOG_KERN = (0 << 3),       /// kernel messages
    SYSLOG_USER = (1 << 3),       /// random user-level messages
    SYSLOG_MAIL = (2 << 3),       /// mail system
    SYSLOG_DAEMON = (3 << 3),     /// system daemons
    SYSLOG_AUTH = (4 << 3),       /// security/authorization messages
    SYSLOG_SYSLOG = (5 << 3),     /// messages generated internally by syslogd
    SYSLOG_LPR = (6 << 3),        /// line printer subsystem
    SYSLOG_NEWS = (7 << 3),       /// network news subsystem
    SYSLOG_UUCP = (8 << 3),       /// UUCP subsystem
    SYSLOG_CRON = (9 << 3),       /// clock daemon
    SYSLOG_AUTHPRIV = (10 << 3),  /// security/authorization messages (private)
    SYSLOG_FTP = (11 << 3),       /// ftp daemon
    SYSLOG_NTP = (12 << 3),       /// ntp subsystem
    SYSLOG_LOGAUDIT = (13 << 3),  /// log audit
    SYSLOG_LOGALERT = (14 << 3),  /// log alert
    SYSLOG_CLOCK = (15 << 3),     /// clock daemon
    SYSLOG_LOCAL0 = (16 << 3),    /// reserved for local use
    SYSLOG_LOCAL1 = (17 << 3),    /// reserved for local use
    SYSLOG_LOCAL2 = (18 << 3),    /// reserved for local use
    SYSLOG_LOCAL3 = (19 << 3),    /// reserved for local use
    SYSLOG_LOCAL4 = (20 << 3),    /// reserved for local use
    SYSLOG_LOCAL5 = (21 << 3),    /// reserved for local use
    SYSLOG_LOCAL6 = (22 << 3),    /// reserved for local use
    SYSLOG_LOCAL7 = (23 << 3)     /// reserved for local use
  };

  enum { SYSLOG_PORT = 514 };

  /**
   * Creates a RemoteSyslogSink.
   */
  RemoteSyslogSink();

  /**
   * Creates a RemoteSyslogSink with the given target address, name, and
   * facility. If bsd_format is true, messages are formatted according to RFC
   * 3164.
   */
  RemoteSyslogSink(const String& address, const String& name,
                   int facility = SYSLOG_USER, bool bsd_format = false);

  /**
   * Opens the RemoteSyslogSink.
   */
  void Open() override;

  /**
   * Closes the RemoteSyslogSink.
   */
  void Close() override;

  /**
   * Sends the message's text to the syslog service.
   */
  void Log(const LogMessage& msg) override;

  /**
   * Sets the property with the given value.
   *
   * The following properties are supported:
   *
   *   - Name:      The name used to identify the source of log messages.
   *   - Facility:  The facility added to each log message. See the Facility
   * enumeration for a list of supported values. The LOG_ prefix can be omitted
   * and values are case insensitive (e.g. a facility value "mail" is recognized
   * as SYSLOG_MAIL)
   *   - Format:    "bsd"/"rfc3164" (RFC 3164 format) or "new"/"rfc5424"
   * (default)
   *   - Loghost:   The target IP address or host name where log messages are
   * sent. Optionally, a port number (separated by a colon) can also be
   * specified.
   *   - Host:      (optional) Host name included in syslog messages. If not
   * specified, the host's real domain name or IP address will be used.
   */
  void SetProperty(const String& name, const String& value) override;

  /**
   * Returns the value of the property with the given name.
   */
  String GetProperty(const String& name) const override;

  /**
   * Registers the sink with the global LoggingFactory.
   */
  static void RegisterSink();

  static const String PROP_NAME;
  static const String PROP_FACILITY;
  static const String PROP_FORMAT;
  static const String PROP_LOGHOST;
  static const String PROP_HOST;
  static const String STRUCTURED_DATA;

 protected:
  ~RemoteSyslogSink();

  static int GetPrio(const LogMessage& msg);

 private:
  String log_host_;
  String name_;
  String host_;
  int facility_;
  bool bsd_format_;
  DatagramSocket socket_;
  InetAddress socket_addr_;
  bool is_opened_;
  FastMutex mutex_;
};

}  // namespace net
}  // namespace fun
