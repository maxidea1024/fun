#include "fun/framework/option_set.h"

#include "fun/base/exception.h"
#include "fun/framework/option_exception.h"

namespace fun {
namespace framework {

OptionSet::OptionSet() {}

OptionSet::OptionSet(const OptionSet& options) : options_(options.options_) {}

OptionSet::~OptionSet() {}

OptionSet& OptionSet::operator=(const OptionSet& options) {
  if (FUN_LIKELY(
          &options !=
          this)) {  // TODO 어짜피 내부에서 체크하니 두번 체크하는 꼴일듯...
    options_ = options.options_;
  }

  return *this;
}

void OptionSet::AddOption(const Option& option) {
  fun_check(!option.GetFullName().IsEmpty());
  OptionList::const_iterator it = options_.begin();
  OptionList::const_iterator it_end = options_.end();
  for (; it != it_end; ++it) {
    if (it->GetFullName() == option.GetFullName()) {
      throw DuplicateOptionException(it->GetFullName());
    }
  }

  options_.push_back(option);
}

bool OptionSet::HasOption(const String& name, bool match_short) const {
  bool found = false;

  for (Iterator it = options_.begin(); it != options_.end(); ++it) {
    if ((match_short && it->MatchesShort(name)) ||
        (!match_short && it->MatchesFull(name))) {
      if (!found) {
        found = true;
      } else {
        return false;
      }
    }
  }

  return found;
}

const Option& OptionSet::GetOption(const String& name, bool match_short) const {
  const Option* option = nullptr;

  for (Iterator it = options_.begin(); it != options_.end(); ++it) {
    if ((match_short && it->MatchesShort(name)) ||
        (!match_short && it->MatchesPartial(name))) {
      if (!option) {
        option = &*it;

        if (!match_short && it->MatchesFull(name)) {
          break;
        }
      } else if (!match_short && it->MatchesFull(name)) {
        option = &*it;
        break;
      } else {
        throw AmbiguousOptionException(name);
      }
    }
  }

  if (option) {
    return *option;
  } else {
    throw UnknownOptionException(name);
  }
}

OptionSet::Iterator OptionSet::begin() const { return options_.begin(); }

OptionSet::Iterator OptionSet::end() const { return options_.end(); }

}  // namespace framework
}  // namespace fun
