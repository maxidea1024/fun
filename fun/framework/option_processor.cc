#include "fun/framework/option_processor.h"
#include "fun/framework/option.h"
#include "fun/framework/option_exception.h"
#include "fun/framework/option_set.h"

namespace fun {
namespace framework {

OptionProcessor::OptionProcessor(const OptionSet& options)
    : options_(options), unix_style_(true), ignore_(false) {}

OptionProcessor::~OptionProcessor() {}

void OptionProcessor::SetUnixStyle(bool flag) { unix_style_ = flag; }

bool OptionProcessor::Process(const String& argument, String& option_name,
                              String& option_arg) {
  option_name.Clear();
  option_arg.Clear();

  if (!ignore_) {
    if (!specified_options_.IsEmpty()) {
      return ProcessCommon(argument, false, option_name, option_arg);
    } else if (unix_style_) {
      return ProcessUnix(argument, option_name, option_arg);
    } else {
      return ProcessDefault(argument, option_name, option_arg);
    }
  }

  return false;
}

void OptionProcessor::CheckRequired() const {
  for (OptionSet::Iterator it = options_.begin(); it != options_.end(); ++it) {
    if (it->IsRequired() && specified_options_.find(it->GetFullName()) ==
                                specified_options_.end()) {
      throw MissingOptionException(it->GetFullName());
    }
  }

  if (!specified_options_.IsEmpty()) {
    String option_arg;
    const Option& option = options_.GetOption(specified_options_, false);
    option.Process(specified_options_,
                   option_arg);  // will throw MissingArgumentException
  }
}

bool OptionProcessor::ProcessUnix(const String& argument, String& option_name,
                                  String& option_arg) {
  String::const_iterator it = argument.begin();
  String::const_iterator end = argument.end();
  if (it != end) {
    if (*it == '-') {
      ++it;
      if (it != end) {
        if (*it == '-') {
          ++it;
          if (it == end) {
            ignore_ = true;
            return true;
          } else {
            return ProcessCommon(String(it, end), false, option_name,
                                 option_arg);
          }
        } else {
          return ProcessCommon(String(it, end), true, option_name, option_arg);
        }
      }
    }
  }
  return false;
}

bool OptionProcessor::ProcessDefault(const String& argument,
                                     String& option_name, String& option_arg) {
  String::const_iterator it = argument.begin();
  String::const_iterator end = argument.end();
  if (it != end) {
    if (*it == '/') {
      ++it;
      return ProcessCommon(String(it, end), false, option_name, option_arg);
    }
  }
  return false;
}

bool OptionProcessor::ProcessCommon(const String& option_str, bool is_short,
                                    String& option_name, String& option_arg) {
  if (!specified_options_.IsEmpty()) {
    const Option& option = options_.GetOption(specified_options_, false);
    String option_with_arg(specified_options_);
    specified_options_.Clear();
    option_with_arg += '=';
    option_with_arg += option_str;
    option.Process(option_with_arg, option_arg);
    option_name = option.GetFullName();
    return true;
  }

  if (option_str.IsEmpty()) {
    throw EmptyOptionException();
  }

  const Option& option = options_.GetOption(option_str, is_short);
  const String& group = option.GetGroup();
  if (!group.IsEmpty()) {
    if (groups_.find(group) != groups_.end()) {
      throw IncompatibleOptionsException(option.GetFullName());
    } else {
      groups_.insert(group);
    }
  }

  if (specified_options_.find(option.GetFullName()) !=
          specified_options_.end() &&
      !option.IsRepeatable()) {
    throw DuplicateOptionException(option.GetFullName());
  }

  specified_options_.insert(option.GetFullName());

  if (option.IsArgumentRequired() &&
      ((!is_short && option_str.find_first_of(":=") == String::npos) ||
       (is_short && option_str.Len() == option.GetShortName().Len()))) {
    specified_options_ = option.GetFullName();
    return true;
  }

  option.Process(option_str, option_arg);
  option_name = option.GetFullName();

  return true;
}

}  // namespace framework
}  // namespace fun
