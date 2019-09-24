#pragma once

#include "fun/base/base.h"
#include "fun/base/delegate/delegate.h"
#include "fun/base/ftl/shared_ptr.h"

namespace fun {

template <typename T>
class Attribute {
 public:
  FUN_DECLARE_DELEGATE_RET(T, Getter);

  Attribute()
    : value_(),
      is_set_(false),
      getter_() {}

  template <typename OtherType>
  Attribute(const OtherType& initial_value)
    : value_(initial_value),
      is_set_(true),
      getter_() {}

  template <typename ObjectType>
  Attribute(SharedPtr<ObjectType> object, typename Getter::template SPMethodDelegate_Const<ObjectType>::MethodPtr method)
    : value_(),
      is_set_(true),
      getter_(Getter::CreateSP(object, method)) {}

  template <typename ObjectType>
  Attribute(ObjectType* object, typename Getter::template SPMethodDelegate_Const<ObjectType>::MethodPtr method)
    : value_(),
      is_set_(true),
      getter_(Getter::CreateSP(object, method)) {}

  static Attribute Create(const Getter& getter) {
    const bool explicit_constructor_tag = true;
    return Attribute(getter, explicit_constructor_tag);
  }

  static Attribute Create(typename Getter::StaticDelegate::FuncPtr func) {
    const bool explicit_constructor_tag = true;
    return Attribute(Getter::CreateStatic(func), explicit_constructor_tag);
  }

  template <typename OtherType>
  void Set(const OtherType& new_value) {
    getter_.Unbind();
    value_ = new_value;
    is_set_ = true;
  }

  const T& Get() const {
    if (getter_.IsBound()) {
      value_ = getter_.Execute();
    }
    return value_;
  }

  const T& GetOr(const T& default_value) const {
    return is_set_ ? Get() : default_value;
  }

  void Bind(const Getter& new_getter) {
    is_set_ = true;
    getter_ = new_getter;
  }

  void BindStatic(typename Getter::StaticDelegate::FuncPtr func) {
    is_set_ = true;
    getter_.BindStatic(func);
  }

  template <typename ObjectType>
  void BindRaw(ObjectType* object, typename Getter::template RawMethodDelegate_Const<ObjectType>::MethodPtr method) {
    is_set_ = true;
    getter_.BindRaw(object, method);
  }

  template <typename ObjectType>
  void Bind(SharedPtr<ObjectType> object, typename Getter::template SPMethodDelegate_Const<ObjectType>::MethodPtr method) {
    is_set_ = true;
    getter_.BindSP(object, method);
  }

  template <typename ObjectType>
  void Bind(ObjectType* object, typename Getter::template SPMethodDelegate_Const<ObjectType>::MethodPtr method) {
    is_set_ = true;
    getter_.BindSP(object, method);
  }

  //TODO FObject feature
  //template <typename ObjectType>
  //void BindFObject(ObjectType* object, typename Getter::template FObjectMethodDelegate_Const<ObjectType>::MethodPtr method) {
  //  is_set_ = true;
  //  getter_.BindFObject(object, method);
  //}

  //TODO FFunction feature
  //template <typename ObjectType>
  //void BindFFunction(ObjectType* object, const String& function_name) {
  //  is_set_ = true;
  //  getter_.BindFFunction(object, function_name);
  //}

  template <typename ObjectType>
  static Attribute<T> Create(ObjectType* object, const String& function_name) {
    Attribute<T> attrib;
    attrib.BindUFunction<ObjectType>(object, function_name);
    return attrib;
  }

  bool IsBound() const {
    return getter_.IsBound();
  }

  const Getter& GetBinding() const {
    return getter_;
  }

  bool IdenticalTo(const Attribute& other) const {
    const bool is_bound = IsBound();

    if (is_bound == other.IsBound()) {
      if (is_bound) {
        return getter_.GetHandle() == other.getter_.GetHandle();
      } else {
        return value_ == other.value_;
      }
    }

    return false;
  }

  //@note deprecated?
  bool operator == (const Attribute& other) const {
    return Get() == other.Get();
  }

  //@note deprecated?
  bool operator != (const Attribute& other) const {
    return Get() != other.Get();
  }

 private:
  /**
   * Special explicit constructor for Attribute::Create()
   */
  Attribute(const Getter& getter, bool explicit_constructor_tag)
    : value_(),
      is_set_(true),
      getter_(getter) {}

  /**
   * We declare ourselves as a friend (templated using OtherType)
   * so we can access members as needed
   */
  template <typename OtherType>
  friend class Attribute;

  /**
   * Current value.
   *
   * Mutable so that we can cache the value locally when using a bound getter
   * (allows const ref return value.)
   */
  mutable T value_;

  /**
   * True when this attribute was explicitly set by a consumer,
   * false when the attribute's value is set to the default
   */
  bool is_set_;

  /**
   * Bound member function for this attribute (may be NULL if no function is bound.)
   * When set, all attempts to read the attribute's value will instead call
   * this delegate to generate the value.
   *
   * Our attribute's 'getter' delegate.
   */
  Getter getter_;
};

} // namespace fun
