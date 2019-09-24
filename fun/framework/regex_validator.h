#pragma once

#include "fun/framework/framework.h"
#include "fun/framework/option_validator.h"

namespace fun {
namespace framework {

/**
 * This validator matches the option value against
 * a regular expression.
 */
class FUN_FRAMEWORK_API RegexValidator : public OptionValidator {
 public:
  /**
   * Creates the RegexValidator, using the given regular expression.
   */
  RegexValidator(const String& regex);

  /**
   * Destroys the RegexValidator.
   */
  ~RegexValidator();

  /**
   * Validates the value for the given option by
   * matching it with the regular expression.
   */
  void Validate(const Option& option, const String& value) override;

 private:
  RegexValidator();

  //TODO Regex 객체로 가지고 있는 형태가 좋을듯도...
  String regex_;
};

} // namespace framework
} // namespace fun
