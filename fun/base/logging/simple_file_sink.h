#pragma once

#include "fun/base/base.h"
#include "fun/base/logging/log_sink.h"
#include "fun/base/mutex.h"
#include "fun/base/timestamp.h"

namespace fun {

class LogFile;

/**
 * A LogSink that writes to a file. This class only
 * supports simple log file rotation.
 *
 * For more features, see the FileSink class.
 *
 * Only the message's text is written, followed
 * by a newline.
 *
 * Chain this sink to a FormattingChannel with an
 * appropriate Formatter to control what is in the text.
 *
 * Log file rotation based on log file size is supported.
 *
 * If rotation is enabled, the SimpleFileSink will
 * alternate between two log files. If the size of
 * the primary log file exceeds a specified limit,
 * the secondary log file will be used, and vice
 * versa.
 *
 * Log rotation is configured with the "rotation"
 * property, which supports the following values:
 *
 *   - Never:         no log rotation
 *   - <n>:           the file is rotated when its size exceeds
 *                    <n> bytes.
 *   - <n> K:         the file is rotated when its size exceeds
 *                    <n> Kilobytes.
 *   - <n> M:         the file is rotated when its size exceeds
 *                    <n> Megabytes.
 *
 * The path of the (primary) log file can be specified with
 * the "path" property. Optionally, the path of the secondary
 * log file can be specified with the "secondaryPath" property.
 *
 * If no secondary path is specified, the secondary path will
 * default to <PrimaryPath>.1.
 *
 * The flush property specifies whether each log message is flushed
 * immediately to the log file (which may hurt application performance,
 * but ensures that everything is in the log in case of a system crash),
 * or whether it's allowed to stay in the system's file buffer for some time.
 * Valid values are:
 *
 *   - True:   Every message is immediately flushed to the log file (default).
 *   - False:  Messages are not immediately flushed to the log file.
 *
 */
class FUN_BASE_API SimpleFileSink : public LogSink {
 public:
  using Ptr = RefCountedPtr<SimpleFileSink>;

  /**
   * Creates the FileSink.
   */
  SimpleFileSink();

  /**
   * Creates the FileSink for a file with the given path.
   */
  SimpleFileSink(const String& path);

  /**
   * Opens the FileSink and creates the log file if necessary.
   */
  void Open() override;

  /**
   * Closes the FileSink.
   */
  void Close() override;

  /**
   * Logs the given message to the file.
   */
  void Log(const LogMessage& msg) override;

  /**
   * Sets the property with the given name.
   *
   * The following properties are supported:
   *
   *   - Path:          The primary log file's path.
   *   - SecondaryPath: The secondary log file's path.
   *   - Rotation:      The log file's rotation mode. See the
   *                    SimpleFileSink class for details.
   *   - Flush:         Specifies whether messages are immediately
   *                    flushed to the log file. See the SimpleFileSink
   *                    class for details.
   */
  void SetProperty(const String& name, const String& value) override;

  /**
   * Returns the value of the property with the given name.
   * See SetProperty() for a description of the supported
   * properties.
   */
  String GetProperty(const String& name) const override;

  /**
   * Returns the log file's creation date.
   */
  Timestamp GetCreationDate() const;

  /**
   * Returns the log file's current size in bytes.
   */
  uint64 GetSize() const;

  /**
   * Returns the log file's primary path.
   */
  const String& GetPath() const;

  /**
   * Returns the log file's secondary path.
   */
  const String& GetSecondaryPath() const;

  static const String PROP_PATH;
  static const String PROP_SECONDARYPATH;
  static const String PROP_ROTATION;
  static const String PROP_FLUSH;

 protected:
  ~SimpleFileSink();

  void SetRotation(const String& rotation);
  void SetFlush(const String& flush);
  void Rotate();

 private:
  String path_;
  String secondard_path_;
  String rotation_;
  uint64 limit_;
  bool flush_;
  LogFile* file_;
  FastMutex mutex_;
};

}  // namespace fun
