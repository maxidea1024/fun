﻿#include "fun/base/base.h"
#include "fun/base/logging/log_formatter.h"
#include "fun/base/logging/log_message.h"
#include "fun/base/container/array.h"

namespace fun {

/**
 * This LogFormatter allows for custom formatting of
 * log messages based on format patterns.
 *
 * The format pattern is used as a template to format the message and
 * is copied character by character except for the following special characters,
 * which are replaced by the corresponding value.
 *
 *   - %s : message source
 *   - %t : message text
 *   - %l : message priority level (1 .. 7)
 *   - %p : message priority
 *          (Fatal, Critical, Error, Warning, Notice, Information, Debug, Trace)
 *   - %q : abbreviated message priority (F, C, E, W, N, I, D, T)
 *   - %P : message process identifier
 *   - %T : message thread name
 *   - %I : message thread identifier (numeric)
 *   - %O : message thread OS identifier (numeric)
 *   - %N : node or host name
 *   - %U : message source file path (empty string if not set)
 *   - %u : message source line number (0 if not set)
 *   - %w : message date/time abbreviated weekday (Mon, Tue, ...)
 *   - %W : message date/time full weekday (Monday, Tuesday, ...)
 *   - %b : message date/time abbreviated month (Jan, Feb, ...)
 *   - %B : message date/time full month (January, February, ...)
 *   - %d : message date/time zero-padded day of month (01 .. 31)
 *   - %e : message date/time day of month (1 .. 31)
 *   - %f : message date/time space-padded day of month ( 1 .. 31)
 *   - %m : message date/time zero-padded month (01 .. 12)
 *   - %n : message date/time month (1 .. 12)
 *   - %o : message date/time space-padded month ( 1 .. 12)
 *   - %y : message date/time year without century (70)
 *   - %Y : message date/time year with century (1970)
 *   - %H : message date/time hour (00 .. 23)
 *   - %h : message date/time hour (00 .. 12)
 *   - %a : message date/time am/pm
 *   - %A : message date/time AM/PM
 *   - %M : message date/time minute (00 .. 59)
 *   - %S : message date/time second (00 .. 59)
 *   - %i : message date/time millisecond (000 .. 999)
 *   - %c : message date/time centisecond (0 .. 9)
 *   - %F : message date/time fractional seconds/microseconds (000000 - 999999)
 *   - %z : time zone differential in ISO 8601 format (Z or +NN.NN)
 *   - %Z : time zone differential in RFC format (GMT or +NNNN)
 *   - %L : convert time to local time
 *          (must be specified before any date/time specifier;
 *           does not itself output anything)
 *   - %E : epoch time (UTC, seconds since midnight, January 1, 1970)
 *   - %v[width] : the message source (%s) but text length is padded/cropped to 'width'
 *   - %[name] : the value of the message parameter with the given name
 *   - %% : percent sign
 */
class FUN_BASE_API PatternFormatter : public LogFormatter {
 public:
  using Ptr = RefCountedPtr<PatternFormatter>;

  /**
   * Creates a PatternFormatter.
   *
   * The format pattern must be specified with a call to SetProperty.
   */
  PatternFormatter();

  /**
   * Creates a PatternFormatter that uses the given format pattern.
   */
  PatternFormatter(const String& format);

  /**
   * Destroys the PatternFormatter.
   */
  ~PatternFormatter();

  /**
   * Formats the message according to the specified
   * format pattern and places the result in text.
   */
  void Format(const LogMessage& msg, String& text);

  /**
   * Sets the property with the given name to the given value.
   *
   * The following properties are supported:
   *
   *     * pattern: The format pattern. See the PatternFormatter class
   *       for details.
   *     * times: Specifies whether times are adjusted for local time
   *       or taken as they are in UTC. Supported values are "local" and "UTC".
   *     * priorityNames: Provide a comma-separated list of custom priority names,
   *       e.g. "Fatal, Critical, Error, Warning, Notice, Information, Debug, Trace"
   *
   * If any other property name is given, a PropertyNotSupported
   * exception is thrown.
   */
  void SetProperty(const String& name, const String& value);

  /**
   * Returns the value of the property with the given name or
   * throws a PropertyNotSupported exception if the given
   * name is not recognized.
   */
  String GetProperty(const String& name) const;

  static const String PROP_PATTERN;
  static const String PROP_TIMES;
  static const String PROP_LEVEL_NAMES;

 protected:
  /**
   * Returns a string for the given level value.
   */
  const String& GetLevelName(LogLevel::Type level);

 private:
  struct PatternAction {
    char key;
    int32 length;
    String property;
    String prepend;

    PatternAction() : key(0), length(0) {}
  };

  /**
   * Will parse the pattern_ string into the vector of PatternActions,
   * which contains the message key, any text that needs to be written first
   * a property in case of %[] and required length.
   */
  void ParsePattern();

  void ParsePriorityNames();

  Array<PatternAction> pattern_actions_;
  bool local_time_;
  String pattern_;
  String level_names_;
  String levels_[9];
};

} // namespace fun
