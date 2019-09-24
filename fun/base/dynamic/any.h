#pragma once

#include "fun/base/base.h"
#include "fun/base/exception.h"

#include <algorithm>
#include <typeinfo>
#include <cstring>

namespace fun {

class Any;

namespace dynamic {

class Var;
class VarHolder;
template <typename, typename Enable = void> class VarHolderImpl;

} // namespace dynamic

#ifndef FUN_NO_SOO

/**
 * ValueHolder union (used by fun::Any and fun::dynamic::Var for small
 * object optimization, when enabled).
 *
 * If Holder<Type> fits into FUN_SMALL_OBJECT_SIZE bytes of storage,
 * it will be placement-new-allocated into the local buffer
 * (i.e. there will be no heap-allocation). The local buffer size is one byte
 * larger - [FUN_SMALL_OBJECT_SIZE + 1], additional byte value indicating
 * where the object was allocated (0 => heap, 1 => local).
 */
template <typename PlaceholderT, unsigned int SizeV = FUN_SMALL_OBJECT_SIZE>
union Placeholder {
 public:
  struct Size {
    static const unsigned int value = SizeV;
  };

  Placeholder() {
    Clear();
  }

  void Clear() {
    UnsafeMemory::Memset(holder, 0, sizeof(Placeholder));
  }

  bool IsLocal() const {
    return holder[SizeV] != 0;
  }

  void SetLocal(bool local) const {
    holder[SizeV] = local ? 1 : 0;
  }

  PlaceholderT* GetContent() const {
    if (IsLocal()) {
      return reinterpret_cast<PlaceholderT*>(holder);
    } else {
      return holder_ptr;
    }
  }

// MSVC71,80 won't extend friendship to nested class (Any::Holder)
#if !defined(FUN_MSVC_VERSION) || (defined(FUN_MSVC_VERSION) && (FUN_MSVC_VERSION > 80))
 private:
#endif
  typedef typename std::aligned_storage<SizeV + 1>::type AlignerType;

  PlaceholderT* holder_ptr;
  mutable char holder[SizeV + 1];
  AlignerType aligner;

  friend class Any;
  friend class dynamic::Var;
  friend class dynamic::VarHolder;
  template <class> friend class dynamic::VarHolderImpl;
};

#else // !FUN_NO_SOO

/**
 * ValueHolder union (used by fun::Any and fun::dynamic::Var for small
 * object optimization, when enabled).
 *
 * If Holder<Type> fits into FUN_SMALL_OBJECT_SIZE bytes of storage,
 * it will be placement-new-allocated into the local buffer
 * (i.e. there will be no heap-allocation). The local buffer size is one byte
 * larger - [FUN_SMALL_OBJECT_SIZE + 1], additional byte value indicating
 * where the object was allocated (0 => heap, 1 => local).
 */
template <typename PlaceholderT>
union Placeholder {
 public:
  Placeholder() {}

  PlaceholderT* GetContent() const {
    return holder_ptr;
  }

// MSVC71,80 won't extend friendship to nested class (Any::Holder)
#if !defined(FUN_MSVC_VERSION) || (defined(FUN_MSVC_VERSION) && (FUN_MSVC_VERSION > 80))
 private:
#endif

  PlaceholderT* holder_ptr;

  friend class Any;
  friend class dynamic::Var;
  friend class dynamic::VarHolder;
  template <class, class Enable> friend class dynamic::VarHolderImpl;
};

#endif // FUN_NO_SOO

/**
 * An Any class represents a general type and is capable of storing any type, supporting type-safe extraction
 * of the internally stored data.
 *
 * Code taken from the Boost 1.33.1 library. Original copyright by Kevlin Henney. Modified for fun
 * by Applied Informatics.
 *
 * Modified for small object optimization support (optionally supported through conditional compilation)
 * by Alex Fabijanic.
 */
class Any {
 public:
#ifndef FUN_NO_SOO
  /**
   * Creates an empty any type.
   */
  Any() {}

  /**
   * Creates an any which stores the init parameter inside.
   *
   * Example:
   *   Any a(13);
   *   Any a(string("12345"));
   */
  template<typename ValueType>
  Any(const ValueType & value) {
    Construct(value);
  }

  /**
   * Copy constructor, works with both empty and initialized Any values.
   */
  Any(const Any& other) {
    if ((this != &other) && !other.IsEmpty()) {
      Construct(other);
    }
  }

  /**
   * Destructor. If Any is locally held, calls ValueHolder destructor;
   * otherwise, deletes the placeholder from the heap.
   */
  ~Any() {
    if (!IsEmpty()) {
      if (value_holder_.IsLocal()) {
        Destruct();
      } else {
        delete GetContent();
      }
    }
  }

  /**
   * Swaps the content of the two Anys.
   *
   * When small object optimization is enabled, Swap only
   * has no-throw guarantee when both (*this and other)
   * objects are allocated on the heap.
   */
  Any& Swap(Any& other) {
    if (this == &other) {
      return *this;
    }

    if (!value_holder_.IsLocal() && !other.value_holder_.IsLocal()) {
      std::Swap(value_holder_.holder_ptr, other.value_holder_.holder_ptr);
    } else {
      Any tmp(*this);
      try {
        if (value_holder_.IsLocal()) {
          Destruct();
        }
        Construct(other);
        other = tmp;
      } catch (...) {
        Construct(tmp);
        throw;
      }
    }

    return *this;
  }

  /**
   * Assignment operator for all types != Any.
   *
   * Example:
   *   Any a = 13;
   *   Any a = string("12345");
   */
  template<typename ValueType>
  Any& operator = (const ValueType& rhs) {
    Construct(rhs);
    return *this;
  }

  /**
   * Assignment operator for Any.
   */
  Any& operator = (const Any& rhs) {
    if ((this != &rhs) && !rhs.IsEmpty()) {
      Construct(rhs);
    } else if ((this != &rhs) && rhs.IsEmpty()) {
      value_holder_.Clear();
    }

    return *this;
  }

  /**
   * Returns true if the Any is empty.
   */
  bool IsEmpty() const
  {
    char buf[FUN_SMALL_OBJECT_SIZE] = { 0 };
    return 0 == std::memcmp(value_holder_.holder, buf, FUN_SMALL_OBJECT_SIZE);
  }

  /**
   * Returns the type information of the stored content.
   * If the Any is empty typeid(void) is returned.
   * It is recommended to always query an Any for its type info before
   * trying to extract data via an AnyCast/RefAnyCast.
   */
  const std::type_info& Type() const {
    return IsEmpty() ? typeid(void) : GetContent()->Type();
  }

 private:
  class ValueHolder {
   public:
    virtual ~ValueHolder() {}

    virtual const std::type_info& Type() const = 0;
    virtual void Clone(Placeholder<ValueHolder>*) const = 0;
  };

  template<typename ValueType>
  class Holder : public ValueHolder {
   public:
    Holder(const ValueType& value) : held_(value) {}

    virtual const std::type_info& Type() const {
      return typeid(ValueType);
    }

    virtual void Clone(Placeholder<ValueHolder>* holder) const {
      if ((sizeof(Holder<ValueType>) <= FUN_SMALL_OBJECT_SIZE)) {
        new((ValueHolder*) holder->holder) Holder(held_);
        holder->SetLocal(true);
      } else {
        holder->holder_ptr = new Holder(held_);
        holder->SetLocal(false);
      }
    }

    ValueType held_;

   private:
    Holder& operator = (const Holder&);
  };

  ValueHolder* GetContent() const {
    return value_holder_.GetContent();
  }

  template<typename ValueType>
  void Construct(const ValueType& value) {
    if (sizeof(Holder<ValueType>) <= Placeholder<ValueType>::Size::value) {
      new(reinterpret_cast<ValueHolder*>(value_holder_.holder)) Holder<ValueType>(value);
      value_holder_.SetLocal(true);
    } else {
      value_holder_.holder_ptr = new Holder<ValueType>(value);
      value_holder_.SetLocal(false);
    }
  }

  void Construct(const Any& other) {
    if (!other.IsEmpty()) {
      other.GetContent()->Clone(&value_holder_);
    } else {
      value_holder_.Clear();
    }
  }

  void Destruct() {
    GetContent()->~ValueHolder();
  }

  Placeholder<ValueHolder> value_holder_;

#else // if FUN_NO_SOO

  /**
   * Creates an empty any type.
   */
  Any() : holder_ptr_(nullptr) {}

  /**
   * Creates an any which stores the init parameter inside.
   *
   * Example:
   * Any a(13);
   * Any a(string("12345"));
   */
  template <typename ValueType>
  Any(const ValueType& value):
    holder_ptr_(new Holder<ValueType>(value)) {}

  /**
   * Copy constructor, works with both empty and initialized Any values.
   */
  Any(const Any& other)
    : holder_ptr_(other.holder_ptr_ ? other.holder_ptr_->Clone() : nullptr) {}

  ~Any() {
    delete holder_ptr_;
  }

  /**
   * Swaps the content of the two Anys.
   */
  Any& Swap(Any& rhs)
  {
    fun::Swap(holder_ptr_, rhs.holder_ptr_);
    return *this;
  }

  /**
   * Assignment operator for all types != Any.
   *
   * Example:
   *   Any a = 13;
   *   Any a = string("12345");
   */
  template <typename ValueType>
  Any& operator = (const ValueType& rhs) {
    Any(rhs).Swap(*this);
    return *this;
  }

  /**
   * Assignment operator for Any.
   */
  Any& operator = (const Any& rhs) {
    Any(rhs).Swap(*this);
    return *this;
  }

  /**
   * Returns true if the Any is empty.
   */
  bool IsEmpty() const {
    return !holder_ptr_;
  }

  /**
   * Returns the type information of the stored content.
   * If the Any is empty typeid(void) is returned.
   * It is recommended to always query an Any for its type info before
   * trying to extract data via an AnyCast/RefAnyCast.
   */
  const std::type_info& Type() const {
    return holder_ptr_ ? holder_ptr_->Type() : typeid(void);
  }

 private:
  class ValueHolder {
   public:
    virtual ~ValueHolder() {}
    virtual const std::type_info& Type() const = 0;
    virtual ValueHolder* Clone() const = 0;
  };

  template <typename ValueType>
  class Holder : public ValueHolder {
   public:
    Holder(const ValueType& value) : held_(value) {}

    virtual const std::type_info& Type() const {
      return typeid(ValueType);
    }

    virtual ValueHolder* Clone() const {
      return new Holder(held_);
    }

    ValueType held_;

    Holder & operator=(const Holder&) = delete;
  };

  ValueHolder* GetContent() const {
    return holder_ptr_;
  }

 private:
  ValueHolder* holder_ptr_;
#endif // FUN_NO_SOO

  template <typename ValueType>
  friend ValueType* AnyCast(Any*);

  template <typename ValueType>
  friend ValueType* UnsafeAnyCast(Any*);

  template <typename ValueType>
  friend const ValueType& RefAnyCast(const Any&);

  template <typename ValueType>
  friend ValueType& RefAnyCast(Any&);

  template <typename ValueType>
  friend ValueType AnyCast(Any&);
};


/**
 * AnyCast operator used to extract the ValueType from an Any*. Will return a pointer
 * to the stored value.
 *
 * Example Usage:
 * MyType* casted = AnyCast<MyType*>(pAny).
 * Will return NULL if the cast fails, i.e. types don't match.
 */
template <typename ValueType>
ValueType* AnyCast(Any* operand) {
  return operand && operand->Type() == typeid(ValueType)
        ? &static_cast<Any::Holder<ValueType>*>(operand->GetContent())->held_
        : nullptr;
}

/**
 * AnyCast operator used to extract a const ValueType pointer from an const Any*. Will return a const pointer
 * to the stored value.
 *
 * Example Usage:
 * const MyType* casted = AnyCast<MyType*>(pAny).
 * Will return NULL if the cast fails, i.e. types don't match.
 */
template <typename ValueType>
const ValueType* AnyCast(const Any* operand) {
  return AnyCast<ValueType>(const_cast<Any*>(operand));
}

/**
 * AnyCast operator used to extract a copy of the ValueType from an Any&.
 *
 * Example Usage:
 * MyType tmp = AnyCast<MyType>(anAny).
 * Will throw a BadCastException if the cast fails.
 * Do not use an AnyCast in combination with references, i.e. MyType& tmp = ... or const MyType& tmp = ...
 * Some compilers will accept this code although a copy is returned. Use the RefAnyCast in
 * these cases.
 */
template <typename ValueType>
ValueType AnyCast(Any& operand) {
  typedef typename TypeWrapper<ValueType>::TYPE NonRef;

  NonRef* result = AnyCast<NonRef>(&operand);
  if (!result) {
    String s = "RefAnyCast: Failed to convert between Any types ";
    if (operand.holder_ptr_) {
      s.Append('(');
      s.Append(operand.holder_ptr_->Type().name());
      s.Append(" => ");
      s.Append(typeid(ValueType).name());
      s.Append(')');
    }
    throw BadCastException(s);
  }
  return *result;
}

/**
 * AnyCast operator used to extract a copy of the ValueType from an const Any&.
 *
 * Example Usage:
 * MyType tmp = AnyCast<MyType>(anAny).
 * Will throw a BadCastException if the cast fails.
 * Do not use an AnyCast in combination with references, i.e. MyType& tmp = ... or const MyType& = ...
 * Some compilers will accept this code although a copy is returned. Use the RefAnyCast in
 * these cases.
 */
template <typename ValueType>
ValueType AnyCast(const Any& operand) {
  typedef typename TypeWrapper<ValueType>::TYPE NonRef;

  return AnyCast<NonRef&>(const_cast<Any&>(operand));
}

/**
 * AnyCast operator used to return a const reference to the internal data.
 *
 * Example Usage:
 * const MyType& tmp = RefAnyCast<MyType>(anAny);
 */
template <typename ValueType>
const ValueType& RefAnyCast(const Any& operand) {
  ValueType* result = AnyCast<ValueType>(const_cast<Any*>(&operand));
  if (!result) {
    String s = "RefAnyCast: Failed to convert between Any types ";
    if (operand.holder_ptr_) {
      s.Append('(');
      s.Append(operand.holder_ptr_->Type().name());
      s.Append(" => ");
      s.Append(typeid(ValueType).name());
      s.Append(')');
    }
    throw BadCastException(s);
  }
  return *result;
}

/**
 * AnyCast operator used to return a reference to the internal data.
 *
 * Example Usage:
 * MyType& tmp = RefAnyCast<MyType>(anAny);
 */
template <typename ValueType>
ValueType& RefAnyCast(Any& operand) {
  ValueType* result = AnyCast<ValueType>(&operand);
  if (!result) {
    String s = "RefAnyCast: Failed to convert between Any types ";
    if (operand.holder_ptr_) {
      s.Append('(');
      s.Append(operand.holder_ptr_->Type().name());
      s.Append(" => ");
      s.Append(typeid(ValueType).name());
      s.Append(')');
    }
    throw BadCastException(s);
  }
  return *result;
}

/**
 * The "unsafe" versions of AnyCast are not part of the
 * public interface and may be removed At any time. They are
 * required where we know what type is stored in the any and can't
 * use typeid() comparison, e.g., when our types may travel across
 * different shared libraries.
 */
template <typename ValueType>
ValueType* UnsafeAnyCast(Any* operand) {
  return &static_cast<Any::Holder<ValueType>*>(operand->GetContent())->held_;
}

/**
 * The "unsafe" versions of AnyCast are not part of the
 * public interface and may be removed At any time. They are
 * required where we know what type is stored in the any and can't
 * use typeid() comparison, e.g., when our types may travel across
 * different shared libraries.
 */
template <typename ValueType>
const ValueType* UnsafeAnyCast(const Any* operand) {
  return AnyCast<ValueType>(const_cast<Any*>(operand));
}

} // namespace fun
