#include "fun/framework/int_validator.h"

#include "fun/framework/option.h"
#include "fun/framework/option_exception.h"

namespace fun {
namespace framework {

IntValidator::IntValidator(int32 min, int32 max) : min_(min), max_(max) {}

IntValidator::~IntValidator() {}

void IntValidator::Validate(const Option& option, const String& value) {
  bool ok = false;
  int32 n = value.ToInt32(&ok);
  if (ok) {
    if (n < min_ || n > max_) {
      throw InvalidArgumentException(
          String::Format("argument for {0} must be in range {1} to {2}",
                         option.GetFullName(), min_, max_));
    }
  } else {
    throw InvalidArgumentException(String::Format(
        "argument for {0} must be an integer", option.GetFullName()));
  }
}

}  // namespace framework
}  // namespace fun
