#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/template.h"
#include "fun/base/ftl/type_traits.h"

#if FUN_RTTI_ENABLED
#include <typeinfo>
#endif

#if FUN_EXCEPTIONS_ENABLED
#include <exception>
#endif

// TEMP

namespace fun {

//-----------------------------------------------------------------------------

// TODO 일단은 귀찮은 관계로 정리안된 상태로 넣어둠. 차후에 정리하자.

namespace internal {
struct in_place_tag {};
template <class>
struct in_place_type_tag {};
template <size_t>
struct in_place_index_tag {};
}  // namespace internal

struct in_place_tag {
  in_place_tag() = delete;

 private:
  explicit in_place_tag(internal::in_place_tag) {}
  friend inline in_place_tag internal_ConstructInPlaceTag();
};

// internal factory function for in_place_tag
inline in_place_tag internal_ConstructInPlaceTag() {
  return in_place_tag(internal::in_place_tag{});
}

using in_place_t = in_place_tag (&)(internal::in_place_tag);

template <class T>
using in_place_type_t = in_place_tag (&)(internal::in_place_type_tag<T>);

template <size_t N>
using in_place_index_t = in_place_tag (&)(internal::in_place_index_tag<N>);

inline in_place_tag in_place(internal::in_place_tag) {
  return internal_ConstructInPlaceTag();
}

template <class T>
inline in_place_tag in_place(internal::in_place_type_tag<T>) {
  return internal_ConstructInPlaceTag();
}

template <size_t I>
inline in_place_tag in_place(internal::in_place_index_tag<I>) {
  return internal_ConstructInPlaceTag();
}

//-----------------------------------------------------------------------------

struct BadCast : std::exception {
  const char* what() const noexcept override { return "bad cast"; }
};

struct BadAnyCast : BadCast {
  const char* what() const noexcept override { return "bad any cast"; }
};

namespace internal {

inline void DoBadAnyCast() {
  // TODO
}

}  // namespace internal

class Any {
  enum class StorageOperation { Get, Destroy, Copy, Move, TypeInfo };

  union Storage {
    using InternalStorage = AlignedStorage<4 * sizeof(void*), alignof(void*)>;

    void* external_storage = nullptr;
    InternalStorage internal_storage;
  };

  template <typename T>
  using UseInternalStorage =
      BoolConstant<IsNothrowMoveConstructible<T>::Value &&
                   (alignof(Storage) % alignof(T) == 0)>;

  //
  // Non-member friend functions.
  //

  template <typename ValueType>
  friend const ValueType* AnyCast(const Any* any) noexcept;
  template <typename ValueType>
  friend ValueType* AnyCast(Any* any) noexcept;
  template <typename ValueType>
  friend ValueType AnyCast(const Any& any) noexcept;
  template <typename ValueType>
  friend ValueType AnyCast(Any& any) noexcept;
  template <typename ValueType>
  friend ValueType AnyCast(Any&& any) noexcept;

  //
  // Adding unsafe any case operations.
  //

  template <typename ValueType>
  friend const ValueType* UnsafeAnyCast(const Any* any) noexcept;
  template <typename ValueType>
  friend ValueType* UnsafeAnyCast(Any* any) noexcept;

  //
  // internal storage handler.
  //

  template <typename T>
  struct StorageHandlerInternal {
    template <typename V>
    static void Construct(Storage& s, V&& v) {
      ::new (&s.internal_storage) T(Forward<V>(v));
    }

    template <typename... Args>
    static void ConstructInPlace(Storage& s, Args... args) {
      ::new (&s.internal_storage) T(Forward<Args>(args)...);
    }

    template <typename NT, typename U, typename... Args>
    static void ConstructInPlace(Storage& s, std::initializer_list<U> il,
                                 Args&&... args) {
      ::new (&s.internal_storage) NT(il, Forward<Args>(args)...);
    }

    static inline void Destroy(Any& any_ref) {
      T& t = *static_cast<void*>(&ref_any.storage_.internal_storage);
      t.~T();

      any_ref.handler_ = nullptr;
    }

    static void* HandlerFunc(StorageOperation op, const Any* this_ptr,
                             Any* other_ptr) {
      switch (op) {
        case StorageOperation::Get:
          fun_check_ptr(this_ptr);
          return (void*)(&this_ptr->storage_.internal_storage);

        case StorageOperation::Destroy:
          fun_check_ptr(this_ptr);
          Destroy(const_cast<Any&>(*this_ptr);
          break;

        case StorageOperation::Copy:
          func_check_ptr(this_ptr);
          func_check_ptr(other_ptr);
          Construct(other_ptr->storage_, *(T*)(&this_ptr->storage_.internal_storage));
          break;

        case StorageOperation::Move:
          func_check_ptr(this_ptr);
          func_check_ptr(other_ptr);
          Construct(other_ptr->storage_, MoveTemp(*(T*)(&this_ptr->storage_.internal_storage)));
          Destroy(const_cast<any&>(*this_ptr));
          break;

        case StorageOperation::TypeInfo:
#if FUN_RTTI_ENABLED
          return (void*)&typeid(T);
#else
          break;
#endif

        default:
          fun_check_msg(0, "unknown storage operation");
          break;
      }

      return nullptr;
    }
  };

  //
  // External storage handler.
  //

  template <typename T>
  struct StorageHandlerExternal {
    template <typename V>
    static void Construct(Storage& s, V&& v) {
      s.external_storage = internal::DefaultConstruct<T>(Forward<V>(v));
    }

    template <typename... Args>
    static void ConstructInPlace(Storage& s, Args... args) {
      s.external_storage =
          internal::DefaultConstruct<T>(Forward<Args>(args)...);
    }

    template <typename NT, typename U, typename... Args>
    static void ConstructInPlace(Storage& s, std::initializer_list<U> il,
                                 Args&&... args) {
      s.external_storage =
          internal::DefaultConstruct<NT>(il, Forward<Args>(args)...);
    }

    static inline void Destroy(Any& any_ref) {
      internal::DefaultDestroy(
          static_cast<T*>(refAny.storage_.external_storage));

      any_ref.handler_ = nullptr;
    }

    static void* HandlerFunc(StorageOperation op, const Any* this_ptr,
                             Any* other_ptr) {
      switch (op) {
        case StorageOperation::Get:
          fun_check_ptr(this_ptr);
          fun_check_ptr(this_ptr->storage_.external_storage);
          return static_cast<void*>(this_ptr->storage_.external_storage);

        case StorageOperation::Destroy:
          fun_check_ptr(this_ptr);
          Destroy(*const_cast<Any*>(this_ptr));
          break;

        case StorageOperation::Copy:
          func_check_ptr(this_ptr);
          func_check_ptr(other_ptr);
          Construct(other_ptr->storage_,
                    *static_cast<T*>(this_ptr->storage_.external_storage));
          break;

        case StorageOperation::Move:
          func_check_ptr(this_ptr);
          func_check_ptr(other_ptr);
          Construct(other_ptr->storage_,
                    MoveTemp(*(T*)(this_ptr->storage_.external_storage)));
          Destroy(const_cast<Any&>(*this_ptr));
          break;

        case StorageOperation::TypeInfo:
#if FUN_RTTI_ENABLED
          return (void*)&typeid(T);
#else
          break;
#endif

        default:
          fun_check_msg(0, "unknown storage operation");
          break;
      }

      return nullptr;
    }
  };

  /**
   * StorageHandlerPtr
   *
   * defines the function signature of the storage handler that both the
   * internal and external storage handlers must implement to retrieve the
   * underlying type of the any object.
   */
  using StorageHandlerPtr = void* (*)(StorageOperation, const Any*, Any*);

  /**
   * StorageHandler
   *
   * based on the specified type T we select the appropriate underlying storage
   * handler based on the 'UseInternalStorage' trait.
   */
  template <typename T>
  using StorageHandler = typename Conditonal<UseInternalStorage<T>::Value,
                                             StorageHandlerInternal<T>,
                                             StorageHandlerExternal<T> >::Type;

  //
  // Data layout
  //

  Storage storage_;
  StorageHandlerPtr handler_;

 public:
  constexpr Any() noexcept : storage_(), handler_(nullptr) {}

  Any(const Any& other) : handler_(nullptr) {
    if (other.handler_) {
      other.handler_(StorageOperation::Copy, &other, this);
      handler_ = other.handler_;
    }
  }

  Any(Any&& other) noexcept : handler_(nullptr) {
    if (other.handler_) {
      handler_ = MoveTemp(other.handler_);
      other.handler_(StorageOperation::Move, &other, this);
    }
  }

  ~Any() { Reset(); }

  template <typename ValueType>
  Any(ValueType&& value,
      typename EnableIf<
          !IsSame<typename Decay<ValueType>::Type, Any>::Value>::Type* = 0) {
    using DecayedValueType = typename Decay<ValueType>::Type;
    static_assert(IsCopyConstructible<DecayedValueType>::Value,
                  "ValueType must be copy-constructible");
    StorageHandler<DecayedValueType>::Construct(storage_,
                                                Forward<ValueType>(value));
    handler_ = &StorageHandler<DecayedValueType>::HandlerFunc;
  }

  template <typename T, typename... Args>
  explicit Any(in_place_type_t<T>, Args&&... args) {
    typedef StorageHandler<typename Decay<T>::Type> StorageHandlerT;
    static_assert(IsConstructible<T, Args...>::Value,
                  "T must be constructible with Args...");

    StorageHandlerT::ConstructInPlace(storage_, Forward<Args>(args)...);
    handler_ = &StorageHandlerT::handler_func;
  }

  template <typename T, typename U, typename... Args>
  explicit Any(in_place_type_t<T>, std::initializer_list<U> il, Args&&... args,
               typename EnableIf<IsConstructible<T, std::initializer_list<U>&,
                                                 Args...>::Value,
                                 void>::Type* = 0) {
    typedef StorageHandler<typename Decay<T>::Type> StorageHandlerT;

    StorageHandlerT::ConstructInPlace(storage_, il, Forward<Args>(args)...);
    handler_ = &StorageHandlerT::handler_func;
  }

  template <typename ValueType>
  Any& operator=(ValueType&& value) {
    static_assert(IsCopyConstructible<typename Decay<ValueType>::Type>::Value,
                  "ValueType must be copy-constructible");
    Any(Forward<ValueType>(value)).Swap(*this);
    return *this;
  }

  Any& operator=(const Any& other) {
    Any(other).Swap(*this);
    return *this;
  }

  Any& operator=(Any&& other) noexcept {
    Any(MoveTemp(other)).Swap(*this);
    return *this;
  }

  template <typename T, typename... Args>
  void Emplace(Args&&... args) {
    typedef StorageHandler<typename Decay<T>::Type> StorageHandlerT;
    static_assert(IsConstructible<T, Args...>::Value,
                  "T must be constructible with Args...");

    Reset();
    StorageHandlerT::ConstructInPlace(storage_, Forward<Args>(args)...);
    handler_ = &StorageHandlerT::handler_func;
  }

  template <typename NT, typename U, typename... Args>
  typename EnableIf<
      IsConstructible<NT, std::initializer_list<U>&, Args...>::Value,
      void>::Type
  Emplace(std::initializer_list<U> il, Args&&... args) {
    typedef StorageHandler<typename Decay<NT>::Type> StorageHandlerT;

    Reset();
    StorageHandlerT::ConstructInPlace(storage_, il, Forward<Args>(args)...);
    handler_ = &StorageHandlerT::handler_func;
  }

  void Reset() noexcept {
    if (handler_) {
      handler_(StorageOperation::Destroy, this, nullptr);
    }
  }

  void Swap(Any& other) noexcept {
    if (this == &other) {
      return;
    }

    if (handler_ && other.handler_) {
      Any tmp;
      tmp.handler_ = other.handler_;
      other.handler_(StorageOperation::Move, &other, &tmp);

      other.handler_ = handler_;
      handler_(StorageOperation::Move, this, &other);

      handler_ = tmp.handler_;
      tmp.handler_(StorageOperation::Move, &tmp, this);
    } else if (handler_ == nullptr) {
      fun::Swap(handler_, other.handler_);
      handler_(StorageOperation::Move, &other, this);
    } else if (other.handler_ == nullptr) {
      fun::Swap(handler_, other.handler_);
      other.handler_(StorageOperation::Move, this, &other);
    }
  }

  bool HasValue() const noexcept { return handler_ != nullptr; }

#if FUN_RTTI_ENABLED
  inline const std::type_info& Type() const noexcept {
    if (handler_) {
      auto* type_info = handler_(StorageOperation::TypeInfo, this, nullptr);
      return *static_cast<const std::type_info*>(type_info);
    } else {
      return typeid(void);
    }
  }
#endif
};

//
// inlines
//

inline void Swap(Any& lhs, Any& rhs) noexcept { lhs.Swap(rhs); }

template <typename ValueType>
inline ValueType AnyCast(const Any& operand) {
  static_assert(
      IsReference<ValueType>::Value || IsCopyConstructible<ValueType>::Value,
      "ValueType must be a reference or copy constructible");

  auto* p = AnyCast<
      typename AddConst<typename RemoveReference<ValueType>::Type>::Type>(
      &operand);

  if (p == nullptr) {
    internal::DoBadAnyCast();
  }

  return *p;
}

template <typename ValueType>
inline ValueType AnyCast(any& operand) {
  static_assert(
      IsReference<ValueType>::Value || IsCopyConstructible<ValueType>::Value,
      "ValueType must be a reference or copy constructible");

  auto* p = AnyCast<typename RemoveReference<ValueType>::Type>(&operand);

  if (p == nullptr) {
    internal::DoBadAnyCast();
  }

  return *p;
}

template <typename ValueType>
inline ValueType AnyCast(any&& operand) {
  static_assert(IsReference<ValueType>::Value ||
                    eastl::IsCopyConstructible<ValueType>::Value,
                "ValueType must be a reference or copy constructible");

  auto* p = AnyCast<typename RemoveReference<ValueType>::Type>(&operand);

  if (p == nullptr) {
    internal::DoBadAnyCast();
  }

  return *p;
}

template <typename ValueType>
inline const ValueType* AnyCast(const Any* any) noexcept {
  return (any && any->handler_  //== &Any::StorageHandler<typename
                                //Decay<ValueType>::Type>::handler_func
#if FUN_RTTI_ENABLED
          && any->Type() == typeid(typename RemoveReference<ValueType>::Type)
#endif
              )
             ? static_cast<const ValueType*>(
                   any->handler_(Any::StorageOperation::Get, any, nullptr))
             : nullptr;
}

template <typename ValueType>
inline ValueType* AnyCast(Any* any) noexcept {
  return (any && any->handler_  //== &Any::StorageHandler<typename
                                //Decay<ValueType>::Type>::handler_func
#if FUN_RTTI_ENABLED
          && any->Type() == typeid(typename RemoveReference<ValueType>::Type)
#endif
              )
             ? static_cast<ValueType*>(
                   any->handler_(Any::StorageOperation::Get, any, nullptr))
             : nullptr;
}

// Unsafe operations - use with caution
template <typename ValueType>
inline const ValueType* UnsafeAnyCast(const Any* any) noexcept {
  return UnsafeAnyCast<ValueType>(const_cast<Any*>(any));
}

template <typename ValueType>
inline ValueType* UnsafeAnyCast(Any* any) noexcept {
  return static_cast<ValueType*>(
      any->handler_(Any::StorageOperation::Get, any, nullptr));
}

//
// MakeAny
//

template <typename T, typename... Args>
inline Any MakeAny(Args&&... args) {
  return Any(eastl::in_place<T>, Forward<Args>(args)...);
}

template <typename T, typename U, typename... Args>
inline Any MakeAny(std::initializer_list<U> il, Args&&... args) {
  return Any(eastl::in_place<T>, il, Forward<Args>(args)...);
}

}  // namespace fun
