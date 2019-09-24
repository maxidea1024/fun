#pragma once

#include "fun/base/base.h"
#include "fun/base/configurable.h"
#include "fun/base/logging/log_message.h"
#include "fun/base/ref_counted_object.h"
#include "fun/base/ref_counted_ptr.h"

namespace fun {

/**
 * The base class for all LogFormatter classes.
 *
 * A formatter basically takes a LogMessage object
 * and formats it into a string. How the formatting
 * is exactly done is up to the implementation of
 * LogFormatter. For example, a very simple implementation
 * might simply take the message's Text (see LogMessage::GetText()).
 * A useful implementation should at least take the LogMessage's
 * Time, Priority and Text fields and put them into a string.
 *
 * The LogFormatter class supports the Configurable interface,
 * so the behaviour of certain formatters is configurable.
 * It also supports reference counting based garbage collection.
 *
 * Trivial implementations of of GetProperty() and SetProperty() are provided.
 *
 * Subclasses must at least provide a Format() method.
 */
class FUN_BASE_API LogFormatter : public Configurable, public RefCountedObject {
 public:
  using Ptr = RefCountedPtr<LogFormatter>;

  /**
   * Creates the formatter.
   */
  LogFormatter();

  /**
   * Formats the message and places the result in text.
   * Subclasses must override this method.
   */
  virtual void Format(const LogMessage& msg, String& text) = 0;

  /**
   * Throws a PropertyNotSupportedException.
   */
  void SetProperty(const String& name, const String& value) override;

  /**
   * Throws a PropertyNotSupportedException.
   */
  String GetProperty(const String& name) const override;

 protected:
  /**
   * Destroys the formatter.
   */
  virtual ~LogFormatter();
};

}  // namespace fun
