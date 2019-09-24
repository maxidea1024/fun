#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * Simply event arguments class to transfer a key and a value via an event call.
 * Note that key and value are *NOT* copied, only references to them are stored.
 */
template <typename _KeyType, typename _ValueType>
class KeyValueArgs {
 public:
  typedef _KeyType   KeyType;
  typedef _ValueType ValueType;

  KeyValueArgs() = delete;
  KeyValueArgs& operator = (const KeyValueArgs&) = delete;

  KeyValueArgs(const KeyType& key, const ValueType& value)
    : key_(key), value_(value) {}

  KeyValueArgs(const KeyValueArgs& rhs)
    : key_(rhs.key_), value_(rhs.value_) {}

  const KeyType& Key() const { return key_; }
  const ValueType& Value() const { return value_; }

 protected:
  const KeyType& key_;
  const ValueType& value_;
};

} // namespace fun
