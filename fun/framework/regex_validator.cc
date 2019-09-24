#include "fun/framework/regex_validator.h"
#include "fun/framework/option.h"
#include "fun/framework/option_exception.h"
#include "fun/base/regex.h"

namespace fun {
namespace framework {

RegexValidator::RegexValidator(const String& regex)
  : regex_(regex) {}

RegexValidator::~RegexValidator() {}

void RegexValidator::Validate(const Option& option, const String& value) {
  if (!Regex::Match(value, regex_, Regex::RE_ANCHORED | Regex::RE_UTF8)) {
    throw InvalidArgumentException(String::Format("argument for {0} does not match regular expression {1}", option.GetFullName(), regex_));
  }
}

} // namespace framework
} // namespace fun
