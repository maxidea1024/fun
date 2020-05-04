#pragma once

#include "fun/json/value.h"

namespace fun {
namespace json {

class FUN_JSON_API Schema {
 public:
  enum ValueType {
    Null,
    Integer,
    Double,
    String,
    Array,
    Object,
  };

  struct Item {
    String name;
    ValueType type;
    bool required;
  };

  Schema();

  bool ValidateJson(const Value& Json) const;
};

}  // namespace json
}  // namespace fun
