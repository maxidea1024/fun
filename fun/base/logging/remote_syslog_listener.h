#include "fun/net/net.h"
#include "fun/net/inet_address.h"
#include "fun/base/thread_pool.h"
#include "fun/base/splitter_sink.h"
#include "fun/base/notification_queue.h"

namespace fun {
namespace net {

class RemoteUdpListener;
class SyslogParser;

/**
 * RemoteSyslogListener implements listening for syslog messages
 * sent over UDP, according to RFC 5424 "The Syslog Protocol"
 * and RFC 5426 "Transmission of syslog messages over UDP".
 *
 * In addition, RemoteSyslogListener also supports the "old" BSD syslog
 * protocol, as described in RFC 3164.
 *
 * The RemoteSyslogListener is a subclass of SplitterSink.
 * Every received log message is sent to the sinks registered
 * with AddSink() or the "sink" property.
 *
 * LogMessage objects created by RemoteSyslogListener will have
 * the following named parameters:
 *
 *   - addr: IP address of the host/interface sending the message.
 *   - host: host name; only for "new" syslog messages.
 *   - app:  application name; only for "new" syslog messages.
 *   - structured-data: RFC 5424 structured data, or empty if not present.
 */
class Net_API RemoteSyslogListener : public SplitterSink {
 public:
  /**
   * Creates the RemoteSyslogListener.
   */
  RemoteSyslogListener();

  /**
   * Creates the RemoteSyslogListener, listening on the given port number.
   */
  RemoteSyslogListener(uint16 port);

  /**
   * Creates the RemoteSyslogListener, listening on the given port number
   * and using the number of threads for message processing.
   */
  RemoteSyslogListener(uint16 port, int32 thread_count);

  /**
   * Sets the property with the given value.
   *
   * The following properties are supported:
   *     * Port: The UDP port number where to listen for UDP packets
   *       containing syslog messages. If 0 is specified, does not
   *       listen for UDP messages.
   *     * Threads: The number of parser threads processing
   *       received syslog messages. Defaults to 1. A maximum
   *       of 16 threads is supported.
   */
  void SetProperty(const String& name, const String& value) override;

  /**
   * Returns the value of the property with the given name.
   */
  String GetProperty(const String& name) const override;

  /**
   * Starts the listener.
   */
  void Open() override;

  /**
   * Stops the listener.
   */
  void Close() override;

  /**
   * Parses a single line of text containing a syslog message
   * and sends it down the filter chain.
   */
  void ProcessMessage(const String& text);

  /**
   * Enqueues a single line of text containing a syslog message
   * for asynchronous processing by a parser thread.
   */
  void EnqueueMessage(const String& text, const InetAddress& sender_addr);

  /**
   * Registers the sink with the global LoggingFactory.
   */
  static void RegisterSink();

  static const String PROP_PORT;
  static const String PROP_THREADS;

  static const String LOG_PROP_APP;
  static const String LOG_PROP_HOST;
  static const String LOG_PROP_STRUCTURED_DATA;

 protected:
  /**
   * Destroys the RemoteSyslogListener.
   */
  ~RemoteSyslogListener();

 private:
  RemoteUdpListener* listener_;
  SyslogParser* parser_;
  ThreadPool thread_pool_;
  NotificationQueue queue_;
  uint16 port_;
  int32 thread_count_;
};

} // namespace net
} // namespace fun
