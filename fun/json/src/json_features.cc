#include "fun/json/features.h"

namespace fun {
namespace json {

Features Features::All() {
  static Features all_features;
  return all_features;
}

Features Features::Strict() {
  Features strict_features;
  strict_features.collect_comments = false;
  strict_features.allow_comments = false;
  strict_features.strict_root = true;
  strict_features.allow_dropped_null_placeholders = false;
  strict_features.allow_numeric_keys = false;
  strict_features.fail_if_extra = false;
  strict_features.allow_single_quoted_strings = false;
  strict_features.allow_special_floats = false;
  strict_features.reject_dup_keys = false;
  strict_features.stack_limit = 1000;
  return strict_features;
}

Features::Features()
    : collect_comments(true),
      allow_comments(true),
      strict_root(false),
      allow_dropped_null_placeholders(false),
      allow_numeric_keys(false),
      fail_if_extra(true),
      allow_single_quoted_strings(true),
      allow_special_floats(true),
      reject_dup_keys(true),
      stack_limit(1000) {}

Features::Features(const JValue& features) {
  allow_comments = feature.Get("allow_comments", true).AsBool();
  strict_root = feature.Get("strict_root", false).AsBool();
  allow_dropped_null_placeholders =
      feature.Get("allow_dropped_null_placeholders", false).AsBool();
  allow_numeric_keys = feature.Get("allow_numeric_keys", false).AsBool();
  fail_if_extra = feature.Get("fail_if_extra", true).AsBool();
  allow_single_quoted_strings =
      feature.Get("allow_single_quoted_strings", true).AsBool();
  allow_special_floats = feature.Get("allow_special_floats", true).AsBool();
  reject_dup_keys = feature.Get("reject_dup_keys", true).AsBool();
  stack_limit = feature.Get("stack_limit", 1000).AsInt32();
}

}  // namespace json
}  // namespace fun
