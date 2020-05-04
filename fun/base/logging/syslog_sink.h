#include "fun/base/base.h"

#if FUN_PLATFORM_UNIX_FAMILY

#include "fun/base/logging/log_sink.h"
#include "fun/base/ref_counted_ptr.h"
#include "fun/base/string/string.h"

namespace fun {

/**
 * This Unix-only sink works with the Unix syslog service.
 */
class FUN_BASE_API SyslogSink : public LogSink {
 public:
  using Ptr = RefCountedPtr<SyslogSink>;

  enum Option {
    SYSLOG_PID = 0x01,     /// log the pid with each message
    SYSLOG_CONS = 0x02,    /// log on the console if errors in sending
    SYSLOG_NDELAY = 0x08,  /// don't delay open
    SYSLOG_PERROR =
        0x20  /// log to stderr as well (not supported on all platforms)
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
    SYSLOG_LOCAL0 = (16 << 3),    /// reserved for local use
    SYSLOG_LOCAL1 = (17 << 3),    /// reserved for local use
    SYSLOG_LOCAL2 = (18 << 3),    /// reserved for local use
    SYSLOG_LOCAL3 = (19 << 3),    /// reserved for local use
    SYSLOG_LOCAL4 = (20 << 3),    /// reserved for local use
    SYSLOG_LOCAL5 = (21 << 3),    /// reserved for local use
    SYSLOG_LOCAL6 = (22 << 3),    /// reserved for local use
    SYSLOG_LOCAL7 = (23 << 3)     /// reserved for local use
  };

 public:
  /**
   * Creates a SyslogSink.
   */
  SyslogSink();

  /**
   * Creates a SyslogSink with the given name, options and facility.
   */
  SyslogSink(const String& name, int options = SYSLOG_CONS,
             int facility = SYSLOG_USER);

  /**
   * Opens the SyslogSink.
   */
  void Open() override;

  /**
   * Closes the SyslogSink.
   */
  void Close() override;

  /**
   * Sens the message's text to the syslog service.
   */
  void Log(const LogMessage& msg) override;

  /**
   * Sets the property with the given value.
   *
   * The following properties are supported:
   *     * name:     The name used to identify the source of log messages.
   *     * facility: The facility added to each log message. See the Facility
   * enumeration for a list of supported values.
   *     * options:  The logging options. See the Option enumeration for a list
   * of supported values.
   */
  void SetProperty(const String& name, const String& value) override;

  /**
   * Returns the value of the property with the given name.
   */
  String GetProperty(const String& name) const override;

  static const String PROP_NAME;
  static const String PROP_FACILITY;
  static const String PROP_OPTIONS;

 protected:
  ~SyslogSink();

  static int GetPrio(const LogMessage& msg);

 private:
  String name_;
  int options_;
  int facility_;
  bool opened_;
};

}  // namespace fun

#endif  //#if FUN_PLATFORM_UNIX_FAMILY
