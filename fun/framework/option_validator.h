#pragma once

#include "fun/base/ref_counted.h"
#include "fun/framework/framework.h"

namespace fun {
namespace framework {

class Option;

/**
 * OptionValidator specifies the interface for option validators.
 *
 * Option validators provide a simple way for the automatic
 * validation of command line argument values.
 */
class FUN_FRAMEWORK_API OptionValidator : public RefCountedObject {
 public:
  using Ptr = RefCountedPtr<OptionValidator>;

  /**
   * Validates the value for the given option.
   * Does nothing if the value is valid.
   *
   * Throws an OptionException otherwise.
   */
  virtual void Validate(const Option& option, const String& value) = 0;

 protected:
  /** Creates the OptionValidator. */
  OptionValidator();

  /** Destroys the OptionValidator. */
  virtual ~OptionValidator();
};

}  // namespace framework
}  // namespace fun
