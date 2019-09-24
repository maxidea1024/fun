#include "fun/framework/option.h"
#include "fun/framework/option_exception.h"
#include "fun/framework/option_validator.h"
#include "fun/base/str.h"

#include <algorithm>

namespace fun {
namespace framework {

Option::Option()
  : required_(false),
    repeatable_(false),
    arg_required_(false),
    validator_(0),
    callback_(0),
    config_(0) {}

Option::Option(const Option& option)
  : short_name_(option.short_name_),
    full_name_(option.full_name_),
    description_(option.description_),
    required_(option.required_),
    repeatable_(option.repeatable_),
    arg_name_(option.arg_name_),
    arg_required_(option.arg_required_),
    group_(option.group_),
    binding_(option.binding_),
    validator_(option.validator_),
    callback_(option.callback_),
    config_(option.config_) {
  if (validator_) {
    validator_->AddRef();
  }

  if (callback_) {
    callback_ = callback_->Clone();
  }
}

Option::Option(const String& full_name, const String& short_name)
  : short_name_(short_name),
    full_name_(full_name),
    required_(false),
    repeatable_(false),
    arg_required_(false),
    validator_(0),
    callback_(0),
    config_(0) {}

Option::Option( const String& full_name,
                const String& short_name,
                const String& description,
                bool required)
  : short_name_(short_name),
    full_name_(full_name),
    description_(description),
    required_(required),
    repeatable_(false),
    arg_required_(false),
    validator_(0),
    callback_(0),
    config_(0) {}

Option::Option( const String& full_name,
                const String& short_name,
                const String& description,
                bool required,
                const String& arg_name,
                bool arg_required)
  : short_name_(short_name),
    full_name_(full_name),
    description_(description),
    required_(required),
    repeatable_(false),
    arg_name_(arg_name),
    arg_required_(arg_required),
    validator_(0),
    callback_(0),
    config_(0) {}

Option::~Option() {
  if (validator_) {
    validator_->Release();
  }

  delete callback_;
}

Option& Option::operator = (const Option& option) {
  if (&option != this) {
    Option tmp(option);
    Swap(tmp);
  }

  return *this;
}

void Option::Swap(Option& option) {
  fun::Swap(short_name_, option.short_name_);
  fun::Swap(full_name_, option.full_name_);
  fun::Swap(description_, option.description_);
  fun::Swap(required_, option.required_);
  fun::Swap(repeatable_, option.repeatable_);
  fun::Swap(arg_name_, option.arg_name_);
  fun::Swap(arg_required_, option.arg_required_);
  fun::Swap(group_, option.group_);
  fun::Swap(binding_, option.binding_);
  fun::Swap(validator_, option.validator_);
  fun::Swap(callback_, option.callback_);
  fun::Swap(config_, option.config_);
}

Option& Option::ShortName(const String& name) {
  short_name_ = name;
  return *this;
}

Option& Option::FullName(const String& name) {
  full_name_ = name;
  return *this;
}

Option& Option::Description(const String& text) {
  description_ = text;
  return *this;
}

Option& Option::Required(bool flag) {
  required_ = flag;
  return *this;
}

Option& Option::Repeatable(bool flag) {
  repeatable_ = flag;
  return *this;
}

Option& Option::Argument(const String& name, bool required) {
  arg_name_ = name;
  arg_required_ = required;
  return *this;
}

Option& Option::NoArgument() {
  arg_name_.Clear();
  arg_required_ = false;
  return *this;
}

Option& Option::Group(const String& group) {
  group_ = group;
  return *this;
}

Option& Option::Binding(const String& property_name) {
  return Binding(property_name, 0);
}

Option& Option::Binding(const String& property_name, ConfigurationBase* config) {
  binding_ = property_name;
  config_ = config;
  return *this;
}

Option& Option::Callback(const OptionCallbackBase& cb) {
  callback_ = cb.Clone();
  return *this;
}

Option& Option::Validator(OptionValidator* validator) {
  if (validator_) {
    validator_->Release();
  }
  validator_ = validator;
  return *this;
}

bool Option::MatchesShort(const String& option) const {
  return option.Len() > 0
    && !short_name_.IsEmpty() && option.compare(0, short_name_.Len(), short_name_) == 0;
}

bool Option::MatchesFull(const String& option) const {
  String::size_type pos = option.find_first_of(":=");
  String::size_type len = pos == String::npos ? option.Len() : pos;
  return len == full_name_.Len()
    && icompare(option, 0, len, full_name_, 0, len) == 0;
}

bool Option::MatchesPartial(const String& option) const {
  String::size_type pos = option.find_first_of(":=");
  String::size_type len = pos == String::npos ? option.Len() : pos;
  return option.Len() > 0
    && icompare(option, 0, len, full_name_, 0, len) == 0;
}

void Option::Process(const String& option, String& arg) const {
  String::size_type pos = option.find_first_of(":=");
  String::size_type len = pos == String::npos ? option.Len() : pos;
  if (icompare(option, 0, len, full_name_, 0, len) == 0) {
    if (TakesArgument()) {
      if (IsArgumentRequired() && pos == String::npos) {
        throw MissingArgumentException(full_name_ + " requires " + GetArgumentName());
      }

      if (pos != String::npos) {
        arg.assign(option, pos + 1, option.Len() - pos - 1);
      } else {
        arg.Clear();
      }
    } else if (pos != String::npos) {
      throw UnexpectedArgumentException(option);
    } else {
      arg.Clear();
    }
  } else if (!short_name_.IsEmpty() && option.compare(0, short_name_.Len(), short_name_) == 0) {
    if (TakesArgument()) {
      if (IsArgumentRequired() && option.Len() == short_name_.Len()) {
        throw MissingArgumentException(short_name_ + " requires " + GetArgumentName());
      }

      arg.assign(option, short_name_.Len(), option.Len() - short_name_.Len());
    } else if (option.Len() != short_name_.Len()) {
      throw UnexpectedArgumentException(option);
    } else {
      arg.Clear();
    }
  } else {
    throw UnknownOptionException(option);
  }
}

} // namespace framework
} // namespace fun
