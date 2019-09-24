#pragma once

#include "fun/framework/framework.h"
#include "fun/framework/option_validator.h"

namespace fun {
namespace framework {

class Option;

/**
 * The IntValidator tests whether the option argument,
 * which must be an integer, lies within a given range.
 */
class FUN_FRAMEWORK_API IntValidator : public OptionValidator {
 public:
  /**
   * Creates the IntValidator.
   */
  IntValidator(int32 min, int32 max);

  /**
   * Destroys the IntValidator.
   */
  ~IntValidator();

  /**
   * Validates the value for the given option by
   * testing whether it's an integer that lies within
   * a given range.
   */
  void Validate(const Option& option, const String& value) override;

 private:
  IntValidator();

  int32 min_;
  int32 max_;
};

}  // namespace framework
}  // namespace fun
