#pragma once

#include "fun/base/base.h"
#include "fun/base/dynamic/var.h"
#include "fun/base/dynamic/var_holder.h"
#include <utility>

namespace fun {
namespace dynamic {

/**
 * Pair allows to define a pair of values.
 */
template <typename K>
class Pair {
 public:
  typedef typename std::pair<K, Var> Data;

  /**
   * Creates an empty Pair
   */
  Pair() : data_() {}

  /**
   * Creates the Pair from another pair.
   */
  Pair(const Pair& other) : data_(other.data_) {}

  /**
   * Creates the Pair from the given value.
   */
  Pair(const Data& val) : data_(val) {}

  /**
   * Creates Pair form standard pair.
   */
  template <typename T>
  Pair(const std::pair<K, T>& val)
    : data_(std::make_pair(val.first, val.second)) {}

  /**
   * Creates pair from two values.
   */
  template <typename T>
  Pair(const K& first, const T& second)
    : data_(std::make_pair(first, second)) {}

  /**
   * Destroys the Pair.
   */
  virtual ~Pair() {}

  /**
   * Swaps the content of the two Pairs.
   */
  Pair& Swap(Pair& other) {
    fun::Swap(data_, other.data_);
    return *this;
  }

  /**
   * Copy constructs Pair from another pair.
   */
  Pair& operator = (const Pair& other) {
    Pair(other).Swap(*this);
    return *this;
  }

  /**
   * Returns the first member of the pair.
   */
  inline const K& first() const {
    return data_.first;
  }

  /**
   * Returns the second member of the pair.
   */
  inline const Var& second() const {
    return data_.second;
  }

  std::string ToString() {
    std::string str;
    Var(*this).template Convert<std::string>(str);
    return str;
  }

 private:
  Data data_;
};

template <>
class VarHolderImpl<Pair<std::string> > : public VarHolder {
 public:
  VarHolderImpl(const Pair<std::string>& val) : val_(val) {}
  ~VarHolderImpl() {}

  const std::type_info& Type() const {
    return typeid(Pair<std::string>);
  }

  void Convert(int8& val) const {
    throw BadCastException("Cannot cast Pair type to int8");
  }

  void Convert(int16& val) const {
    throw BadCastException("Cannot cast Pair type to int16");
  }

  void Convert(int32& val) const {
    throw BadCastException("Cannot cast Pair type to int32");
  }

  void Convert(int64& val) const {
    throw BadCastException("Cannot cast Pair type to int64");
  }

  void Convert(uint8& val) const {
    throw BadCastException("Cannot cast Pair type to uint8");
  }

  void Convert(uint16& val) const {
    throw BadCastException("Cannot cast Pair type to uint16");
  }

  void Convert(uint32& val) const {
    throw BadCastException("Cannot cast Pair type to uint32");
  }

  void Convert(uint64& val) const {
    throw BadCastException("Cannot cast Pair type to uint64");
  }

  void Convert(bool& val) const {
    throw BadCastException("Cannot cast Pair type to bool");
  }

  void Convert(float& val) const {
    throw BadCastException("Cannot cast Pair type to float");
  }

  void Convert(double& val) const {
    throw BadCastException("Cannot cast Pair type to double");
  }

  void Convert(char& val) const {
    throw BadCastException("Cannot cast Pair type to char");
  }

  void Convert(std::string& val) const {
    // Serialize in JSON format: equals an object
    // JSON format definition: { string ':' value } string:value pair n-times, sep. by ','
    val.Append("{ ");
    Var key(val_.first());
    Impl::AppendJsonKey(val, key);
    val.Append(" : ");
    Impl::AppendJsonValue(val, val_.second());
    val.Append(" }");
  }

  void Convert(fun::DateTime&) const {
    throw BadCastException("Pair -> fun::DateTime");
  }

  void Convert(fun::Timestamp&) const {
    throw BadCastException("Pair -> fun::Timestamp");
  }

  VarHolder* Clone(Placeholder<VarHolder>* var_holder = nullptr) const {
    return CloneHolder(var_holder, val_);
  }

  const Pair<std::string>& Value() const {
    return val_;
  }

  bool IsArray() const {
    return false;
  }

  bool IsStruct() const {
    return false;
  }

  bool IsInteger() const {
    return false;
  }

  bool IsSigned() const {
    return false;
  }

  bool IsNumeric() const {
    return false;
  }

  bool IsString() const {
    return false;
  }

 private:
  Pair<std::string> val_;
};

template <>
class VarHolderImpl<Pair<int> > : public VarHolder {
 public:
  VarHolderImpl(const Pair<int>& val) : val_(val) {}
  ~VarHolderImpl() {}

  const std::type_info& Type() const {
    return typeid(Pair<int>);
  }

  void Convert(int8& val) const {
    throw BadCastException("Cannot cast Pair type to int8");
  }

  void Convert(int16& val) const {
    throw BadCastException("Cannot cast Pair type to int16");
  }

  void Convert(int32& val) const {
    throw BadCastException("Cannot cast Pair type to int32");
  }

  void Convert(int64& val) const {
    throw BadCastException("Cannot cast Pair type to int64");
  }

  void Convert(uint8& val) const {
    throw BadCastException("Cannot cast Pair type to uint8");
  }

  void Convert(uint16& val) const {
    throw BadCastException("Cannot cast Pair type to uint16");
  }

  void Convert(uint32& val) const {
    throw BadCastException("Cannot cast Pair type to uint32");
  }

  void Convert(uint64& val) const {
    throw BadCastException("Cannot cast Pair type to uint64");
  }

  void Convert(bool& val) const {
    throw BadCastException("Cannot cast Pair type to bool");
  }

  void Convert(float& val) const {
    throw BadCastException("Cannot cast Pair type to float");
  }

  void Convert(double& val) const {
    throw BadCastException("Cannot cast Pair type to double");
  }

  void Convert(char& val) const {
    throw BadCastException("Cannot cast Pair type to char");
  }

  void Convert(std::string& val) const {
    // Serialize in JSON format: equals an object
    // JSON format definition: { string ':' value } string:value pair n-times, sep. by ','
    val.Append("{ ");
    Var key(val_.first());
    Impl::AppendJsonKey(val, key);
    val.Append(" : ");
    Impl::AppendJsonValue(val, val_.second());
    val.Append(" }");
  }

  void Convert(fun::DateTime&) const {
    throw BadCastException("Pair -> fun::DateTime");
  }

  void Convert(fun::LocalDateTime&) const {
    throw BadCastException("Pair -> fun::LocalDateTime");
  }

  void Convert(fun::Timestamp&) const {
    throw BadCastException("Pair -> fun::Timestamp");
  }

  VarHolder* Clone(Placeholder<VarHolder>* var_holder = nullptr) const {
    return CloneHolder(var_holder, val_);
  }

  const Pair<int>& Value() const {
    return val_;
  }

  bool IsArray() const {
    return false;
  }

  bool IsStruct() const {
    return false;
  }

  bool IsInteger() const {
    return false;
  }

  bool IsSigned() const {
    return false;
  }

  bool IsNumeric() const {
    return false;
  }

  bool IsString() const {
    return false;
  }

 private:
  Pair<int> val_;
};

} // namespace dynamic
} // namespace fun
