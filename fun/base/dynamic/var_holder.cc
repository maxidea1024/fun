#include "fun/base/dynamic/var_holder.h"
#include "fun/base/dynamic/var.h"
#include "fun/base/json_string.h"

namespace fun {
namespace dynamic {

VarHolder::VarHolder() {}

VarHolder::~VarHolder() {}

namespace Impl {

void Escape(String& target, const String& source) {
  target = ToJson(source);
}

bool IsJsonString(const Var& any) {
  return  any.Type() == typeid(String) ||
          any.Type() == typeid(char) ||
          any.Type() == typeid(char*) ||
          any.Type() == typeid(fun::DateTime) ||
          any.Type() == typeid(fun::Timestamp);
}

void AppendJsonString(String& val, const Var& any) {
  String json;
  Escape(json, any.Convert<String>());
  val.Append(json);
}

void AppendJsonKey(String& val, const Var& any) {
  return AppendJsonString(val, any);
}

void AppendJsonValue(String& val, const Var& any) {
  if (any.IsEmpty()) {
    val.Append("null");
    return;
  }

  bool is_json_str = IsJsonString(any);
  if (is_json_str) {
    AppendJsonString(val, any.Convert<String>());
  } else {
    val.Append(any.Convert<String>());
  }
}

} // namespace Impl

} // namespace dynamic
} // namespace fun
