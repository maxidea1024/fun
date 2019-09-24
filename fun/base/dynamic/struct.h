#pragma once

#include "fun/base/base.h"
#include "fun/base/dynamic/var.h"
#include "fun/base/dynamic/varholder.h"
#include "fun/base/sharedptr.h"
#include "fun/base/orderedmap.h"
#include "fun/base/orderedset.h"
#include <map>
#include <set>

namespace fun {
namespace dynamic {

/**
 * Struct allows to define a named collection of Var objects.
 */
template <typename K, typename M = std::map<K, Var>, typename S = std::set<K> >
class Struct {
 public:
  typedef M Data;
  typedef S NameSet;
  typedef typename Data::iterator Iterator;
  typedef typename Data::const_iterator ConstIterator;
  typedef typename Struct<K>::Data::value_type ValueType;
  typedef typename Struct<K>::Data::size_type SizeType;
  typedef typename std::pair<typename Struct<K, M, S>::Iterator, bool> InsRetVal;
  typedef typename fun::SharedPtr<Struct<K, M, S> > Ptr;

  /**
   * Creates an empty Struct
   */
  Struct() : data_() {}

  /**
   * Creates the Struct from the given value.
   */
  Struct(const Data& val) : data_(val) {}

  template <typename T>
  Struct(const std::map<K, T>& val) {
    AssignMap(val);
  }

#ifdef FUN_ENABLE_CPP11
  template <typename T>
  Struct(const OrderedMap<K, T>& val) {
    AssignMap(val);
  }
#endif // FUN_ENABLE_CPP11

  /**
   * Destroys the Struct.
   */
  virtual ~Struct() {}

  /**
   * Returns the Var with the given name, creates an entry if not found.
   */
  inline Var& operator [] (const K& name) {
    return data_[name];
  }

  /**
   * Returns the Var with the given name, throws a
   * NotFoundException if the data member is not found.
   */
  const Var& operator [] (const K& name) const {
    ConstIterator it = find(name);
    if (it == end()) {
      throw NotFoundException(name);
    }
    return it->second;
  }

  /**
   * Returns true if the Struct contains a member with the given name
   */
  inline bool contains(const K& name) const {
    return find(name) != end();
  }

  /**
   * Returns an iterator, pointing to the <name,Var> pair containing
   * the element, or it returns end() if the member was not found
   */
  inline Iterator find(const K& name) {
    return data_.find(name);
  }

  /**
   * Returns a const iterator, pointing to the <name,Var> pair containing
   * the element, or it returns end() if the member was not found
   */
  inline ConstIterator find(const K& name) const {
    return data_.find(name);
  }

  /**
   * Returns the end iterator for the Struct
   */
  inline Iterator end() {
    return data_.end();
  }

  /**
   * Returns the end const iterator for the Struct
   */
  inline ConstIterator end() const {
    return data_.end();
  }

  /**
   * Returns the begin iterator for the Struct
   */
  inline Iterator begin() {
    return data_.begin();
  }

  /**
   * Returns the begin const iterator for the Struct
   */
  inline ConstIterator begin() const {
    return data_.begin();
  }

  /**
   * Inserts a <name, Var> pair into the Struct,
   * returns a pair containing the iterator and a boolean which
   * indicates success or not (is true, when insert succeeded, false,
   * when already another element was present, in this case Iterator
   * points to that other element)
   */
  template <typename T>
  inline InsRetVal insert(const K& key, const T& value) {
    // fix: SunPro C++ is silly ...
    ValueType valueType(key, value);
    return insert(valueType);
  }

  /**
   * Inserts a <name, Var> pair into the Struct,
   * returns a pair containing the iterator and a boolean which
   * indicates success or not (is true, when insert succeeded, false,
   * when already another element was present, in this case Iterator
   * points to that other element)
   */
  inline InsRetVal insert(const ValueType& aPair) {
    return data_.insert(aPair);
  }

  /**
   * Erases the element if found, returns number of elements deleted
   */
  inline SizeType erase(const K& key) {
    return data_.erase(key);
  }

  /**
   * Erases the element At the given position
   */
  inline void erase(Iterator& it) {
    data_.erase(it);
  }

  /**
   * Remove all elements from the struct
   */
  inline void Clear() {
    data_.Clear();
  }

  /**
   * Swap content of Struct with another Struct
   */
  inline void Swap(Struct& other) {
    data_.Swap(other.data_);
  }

  /**
   * Returns true if the Struct doesn't contain any members
   */
  inline bool IsEmpty() const {
    return data_.IsEmpty();
  }

  /**
   * Returns the number of members the Struct contains
   */
  SizeType size() const {
    return data_.size();
  }

  /**
   * Returns a sorted collection containing all member names
   */
  inline NameSet members() const {
    NameSet keys;
    ConstIterator it = begin();
    ConstIterator itEnd = end();
    for (; it != itEnd; ++it) {
      keys.insert(it->first);
    }
    return keys;
  }

  /**
   * Returns the var value of the element with the given name.
   * Throws a NotFoundException if the key does not exist.
   */
  inline Var GetVar(const K& key) const {
    ConstIterator it = find(key);
    if(it == end()) {
      throw NotFoundException("Key not found in Struct");
    }
    return it->second;
  }

  /**
   * Returns the var value of the element with the given name.
   * or default_value if none is found.
   */
  template<typename DefT = Var>
  inline Var GetVar(const K& key, const DefT& default_value) const {
    ConstIterator it = find(key);
    if(it == end()) {
      return default_value;
    }
    return it->second;
  }

  std::string ToString() const {
    std::string str;
    Var(*this).template Convert<std::string>(str);
    return str;
  }

 private:
  template <typename T>
  void AssignMap(const T& map) {
    typedef typename T::const_iterator MapConstIterator;

    MapConstIterator it = map.begin();
    MapConstIterator end = map.end();
    for (; it != end; ++it) data_.insert(ValueType(it->first, Var(it->second)));
  }

  Data data_;
};

template <>
class VarHolderImpl<Struct<std::string, std::map<std::string, Var>, std::set<std::string> > > : public VarHolder {
 public:
  typedef std::string KeyType;
  typedef std::map<KeyType, Var> MapType;
  typedef std::set<KeyType> SetType;
  typedef Struct<KeyType, MapType, SetType> ValueType;

  VarHolderImpl(const ValueType& val) : val_(val) {}
  ~VarHolderImpl() {}

  const std::type_info& Type() const {
    return typeid(ValueType);
  }

  void Convert(int8&) const {
    throw BadCastException("Cannot cast Struct type to int8");
  }

  void Convert(int16&) const {
    throw BadCastException("Cannot cast Struct type to int16");
  }

  void Convert(int32&) const {
    throw BadCastException("Cannot cast Struct type to int32");
  }

  void Convert(int64&) const {
    throw BadCastException("Cannot cast Struct type to int64");
  }

  void Convert(uint8&) const {
    throw BadCastException("Cannot cast Struct type to uint8");
  }

  void Convert(uint16&) const {
    throw BadCastException("Cannot cast Struct type to uint16");
  }

  void Convert(uint32&) const {
    throw BadCastException("Cannot cast Struct type to uint32");
  }

  void Convert(uint64&) const {
    throw BadCastException("Cannot cast Struct type to uint64");
  }

  void Convert(bool&) const {
    throw BadCastException("Cannot cast Struct type to bool");
  }

  void Convert(float&) const {
    throw BadCastException("Cannot cast Struct type to float");
  }

  void Convert(double&) const {
    throw BadCastException("Cannot cast Struct type to double");
  }

  void Convert(char&) const {
    throw BadCastException("Cannot cast Struct type to char");
  }

  void Convert(std::string& val) const {
    val.Append("{ ");

    ValueType::ConstIterator it = val_.begin();
    ValueType::ConstIterator itEnd = val_.end();
    if (!val_.IsEmpty()) {
      Var key(it->first);
      Impl::AppendJsonKey(val, key);
      val.Append(" : ");
      Impl::AppendJsonValue(val, it->second);
      ++it;
    }

    for (; it != itEnd; ++it) {
      val.Append(", ");
      Var key(it->first);
      Impl::AppendJsonKey(val, key);
      val.Append(" : ");
      Impl::AppendJsonValue(val, it->second);
    }

    val.Append(" }");
  }

  void Convert(fun::DateTime&) const {
    throw BadCastException("Struct -> fun::DateTime");
  }

  void Convert(fun::Timestamp&) const {
    throw BadCastException("Struct -> fun::Timestamp");
  }

  VarHolder* Clone(Placeholder<VarHolder>* var_holder = nullptr) const {
    return CloneHolder(var_holder, val_);
  }

  const ValueType& Value() const {
    return val_;
  }

  bool IsArray() const {
    return false;
  }

  bool IsStruct() const {
    return true;
  }

  bool IsOrdered() const {
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

  size_t size() const {
    return val_.size();
  }

  Var& operator [] (const KeyType& name) {
    return val_[name];
  }

  const Var& operator [] (const KeyType& name) const {
    return val_[name];
  }

 private:
  ValueType val_;
};

template <>
class VarHolderImpl<Struct<int, std::map<int, Var>, std::set<int> > > : public VarHolder {
 public:
  typedef int KeyType;
  typedef std::map<KeyType, Var> MapType;
  typedef std::set<KeyType> SetType;
  typedef Struct<KeyType, MapType, SetType> ValueType;

  VarHolderImpl(const ValueType& val) : val_(val) {}
  ~VarHolderImpl() {}

  const std::type_info& Type() const {
    return typeid(ValueType);
  }

  void Convert(int8&) const {
    throw BadCastException("Cannot cast Struct type to int8");
  }

  void Convert(int16&) const {
    throw BadCastException("Cannot cast Struct type to int16");
  }

  void Convert(int32&) const {
    throw BadCastException("Cannot cast Struct type to int32");
  }

  void Convert(int64&) const {
    throw BadCastException("Cannot cast Struct type to int64");
  }

  void Convert(uint8&) const {
    throw BadCastException("Cannot cast Struct type to uint8");
  }

  void Convert(uint16&) const {
    throw BadCastException("Cannot cast Struct type to uint16");
  }

  void Convert(uint32&) const {
    throw BadCastException("Cannot cast Struct type to uint32");
  }

  void Convert(uint64&) const {
    throw BadCastException("Cannot cast Struct type to uint64");
  }

  void Convert(bool&) const {
    throw BadCastException("Cannot cast Struct type to bool");
  }

  void Convert(float&) const {
    throw BadCastException("Cannot cast Struct type to float");
  }

  void Convert(double&) const {
    throw BadCastException("Cannot cast Struct type to double");
  }

  void Convert(char&) const {
    throw BadCastException("Cannot cast Struct type to char");
  }

  void Convert(std::string& val) const {
    val.Append("{ ");

    ValueType::ConstIterator it = val_.begin();
    ValueType::ConstIterator itEnd = val_.end();
    if (!val_.IsEmpty()) {
      Var key(it->first);
      Impl::AppendJsonKey(val, key);
      val.Append(" : ");
      Impl::AppendJsonValue(val, it->second);
      ++it;
    }

    for (; it != itEnd; ++it) {
      val.Append(", ");
      Var key(it->first);
      Impl::AppendJsonKey(val, key);
      val.Append(" : ");
      Impl::AppendJsonValue(val, it->second);
    }

    val.Append(" }");
  }

  void Convert(fun::DateTime&) const {
    throw BadCastException("Struct -> fun::DateTime");
  }

  void Convert(fun::LocalDateTime&) const {
    throw BadCastException("Struct -> fun::LocalDateTime");
  }

  void Convert(fun::Timestamp&) const {
    throw BadCastException("Struct -> fun::Timestamp");
  }

  VarHolder* Clone(Placeholder<VarHolder>* var_holder = nullptr) const {
    return CloneHolder(var_holder, val_);
  }

  const ValueType& Value() const {
    return val_;
  }

  bool IsArray() const {
    return false;
  }

  bool IsStruct() const {
    return true;
  }

  bool IsOrdered() const {
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

  size_t size() const {
    return val_.size();
  }

  Var& operator [] (const KeyType& name) {
    return val_[name];
  }

  const Var& operator [] (const KeyType& name) const {
    return val_[name];
  }

 private:
  ValueType val_;
};

#ifdef FUN_ENABLE_CPP11

template <>
class VarHolderImpl<Struct<std::string, fun::OrderedMap<std::string, Var>, fun::OrderedSet<std::string> > > : public VarHolder {
 public:
  typedef std::string KeyType;
  typedef fun::OrderedMap<KeyType, Var> MapType;
  typedef fun::OrderedSet<KeyType> SetType;
  typedef Struct<KeyType, MapType, SetType> ValueType;

  VarHolderImpl(const ValueType& val) : val_(val) {}
  ~VarHolderImpl() {}

  const std::type_info& Type() const {
    return typeid(ValueType);
  }

  void Convert(int8&) const {
    throw BadCastException("Cannot cast Struct type to int8");
  }

  void Convert(int16&) const {
    throw BadCastException("Cannot cast Struct type to int16");
  }

  void Convert(int32&) const {
    throw BadCastException("Cannot cast Struct type to int32");
  }

  void Convert(int64&) const {
    throw BadCastException("Cannot cast Struct type to int64");
  }

  void Convert(uint8&) const {
    throw BadCastException("Cannot cast Struct type to uint8");
  }

  void Convert(uint16&) const {
    throw BadCastException("Cannot cast Struct type to uint16");
  }

  void Convert(uint32&) const {
    throw BadCastException("Cannot cast Struct type to uint32");
  }

  void Convert(uint64&) const {
    throw BadCastException("Cannot cast Struct type to uint64");
  }

  void Convert(bool&) const {
    throw BadCastException("Cannot cast Struct type to bool");
  }

  void Convert(float&) const {
    throw BadCastException("Cannot cast Struct type to float");
  }

  void Convert(double&) const {
    throw BadCastException("Cannot cast Struct type to double");
  }

  void Convert(char&) const {
    throw BadCastException("Cannot cast Struct type to char");
  }

  void Convert(std::string& val) const {
    val.Append("{ ");

    ValueType::ConstIterator it = val_.begin();
    ValueType::ConstIterator itEnd = val_.end();
    if (!val_.IsEmpty()) {
      Var key(it->first);
      Impl::AppendJsonKey(val, key);
      val.Append(" : ");
      Impl::AppendJsonValue(val, it->second);
      ++it;
    }

    for (; it != itEnd; ++it) {
      val.Append(", ");
      Var key(it->first);
      Impl::AppendJsonKey(val, key);
      val.Append(" : ");
      Impl::AppendJsonValue(val, it->second);
    }

    val.Append(" }");
  }

  void Convert(fun::DateTime&) const {
    throw BadCastException("Struct -> fun::DateTime");
  }

  void Convert(fun::LocalDateTime&) const {
    throw BadCastException("Struct -> fun::LocalDateTime");
  }

  void Convert(fun::Timestamp&) const {
    throw BadCastException("Struct -> fun::Timestamp");
  }

  VarHolder* Clone(Placeholder<VarHolder>* var_holder = nullptr) const {
    return CloneHolder(var_holder, val_);
  }

  const ValueType& Value() const {
    return val_;
  }

  bool IsArray() const {
    return false;
  }

  bool IsStruct() const {
    return true;
  }

  bool IsOrdered() const {
    return true;
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

  size_t size() const {
    return val_.size();
  }

  Var& operator [] (const KeyType& name) {
    return val_[name];
  }

  const Var& operator [] (const KeyType& name) const {
    return val_[name];
  }

 private:
  ValueType val_;
};

template <>
class VarHolderImpl<Struct<int, fun::OrderedMap<int, Var>, fun::OrderedSet<int> > > : public VarHolder {
 public:
  typedef int KeyType;
  typedef fun::OrderedMap<KeyType, Var> MapType;
  typedef fun::OrderedSet<KeyType> SetType;
  typedef Struct<KeyType, MapType, SetType> ValueType;

  VarHolderImpl(const ValueType& val) : val_(val) {}
  ~VarHolderImpl() {}

  const std::type_info& Type() const {
    return typeid(ValueType);
  }

  void Convert(int8&) const {
    throw BadCastException("Cannot cast Struct type to int8");
  }

  void Convert(int16&) const {
    throw BadCastException("Cannot cast Struct type to int16");
  }

  void Convert(int32&) const {
    throw BadCastException("Cannot cast Struct type to int32");
  }

  void Convert(int64&) const {
    throw BadCastException("Cannot cast Struct type to int64");
  }

  void Convert(uint8&) const {
    throw BadCastException("Cannot cast Struct type to uint8");
  }

  void Convert(uint16&) const {
    throw BadCastException("Cannot cast Struct type to uint16");
  }

  void Convert(uint32&) const {
    throw BadCastException("Cannot cast Struct type to uint32");
  }

  void Convert(uint64&) const {
    throw BadCastException("Cannot cast Struct type to uint64");
  }

  void Convert(bool&) const {
    throw BadCastException("Cannot cast Struct type to bool");
  }

  void Convert(float&) const {
    throw BadCastException("Cannot cast Struct type to float");
  }

  void Convert(double&) const {
    throw BadCastException("Cannot cast Struct type to double");
  }

  void Convert(char&) const {
    throw BadCastException("Cannot cast Struct type to char");
  }

  void Convert(std::string& val) const {
    val.Append("{ ");

    ValueType::ConstIterator it = val_.begin();
    ValueType::ConstIterator itEnd = val_.end();
    if (!val_.IsEmpty()) {
      Var key(it->first);
      Impl::AppendJsonKey(val, key);
      val.Append(" : ");
      Impl::AppendJsonValue(val, it->second);
      ++it;
    }

    for (; it != itEnd; ++it) {
      val.Append(", ");
      Var key(it->first);
      Impl::AppendJsonKey(val, key);
      val.Append(" : ");
      Impl::AppendJsonValue(val, it->second);
    }

    val.Append(" }");
  }

  void Convert(fun::DateTime&) const {
    throw BadCastException("Struct -> fun::DateTime");
  }

  void Convert(fun::LocalDateTime&) const {
    throw BadCastException("Struct -> fun::LocalDateTime");
  }

  void Convert(fun::Timestamp&) const {
    throw BadCastException("Struct -> fun::Timestamp");
  }

  VarHolder* Clone(Placeholder<VarHolder>* var_holder = nullptr) const {
    return CloneHolder(var_holder, val_);
  }

  const ValueType& Value() const {
    return val_;
  }

  bool IsArray() const {
    return false;
  }

  bool IsStruct() const {
    return true;
  }

  bool IsOrdered() const {
    return true;
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

  size_t size() const {
    return val_.size();
  }

  Var& operator [] (const KeyType& name) {
    return val_[name];
  }

  const Var& operator [] (const KeyType& name) const {
    return val_[name];
  }

 private:
  ValueType val_;
};

#endif // FUN_ENABLE_CPP11

} // namespace dynamic

typedef Dynamic::Struct<std::string> DynamicStruct;

#ifdef FUN_ENABLE_CPP11
typedef Dynamic::Struct<std::string, fun::OrderedMap<std::string, Dynamic::Var>, fun::OrderedSet<std::string> > OrderedDynamicStruct;
#endif // FUN_ENABLE_CPP11

} // namespace fun
