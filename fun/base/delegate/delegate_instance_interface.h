#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/tuple.h"
#include "fun/base/ftl/type_compatible_storage.h"

namespace fun {

class DelegateBase;
class IDelegateInstance;

template <typename FuncType>
struct IBaseDelegateInstance;

template <typename FuncType>
struct IBaseDelegateInstanceCommon;

template <typename RetType, typename... Args>
struct IBaseDelegateInstanceCommon<RetType(Args...)>
    : public IDelegateInstance {
  /**
   * Emplaces a copy of the delegate instance into the DelegateBase.
   */
  virtual void CreateCopy(DelegateBase& base) = 0;

  /**
   * Execute the delegate.
   * If the function pointer is not valid, an error will occur.
   */
  virtual RetType Execute(Args...) const = 0;
};

template <typename FuncType>
struct IBaseDelegateInstance : public IBaseDelegateInstanceCommon<FuncType> {};

template <typename... Args>
struct IBaseDelegateInstance<void(Args...)>
    : public IBaseDelegateInstanceCommon<void(Args...)> {
  /**
   * Execute the delegate, but only if the function pointer is still valid
   *
   * @return  Returns true if the function was executed
   */
  // NOTE: Currently only delegates with no return value support ExecuteIfSafe()
  virtual bool ExecuteIfSafe(Args...) const = 0;
};

template <bool IsConst, typename Class, typename FuncType>
struct MemberFunctionPtrType;

template <typename Class, typename RetType, typename... Args>
struct MemberFunctionPtrType<false, Class, RetType(Args...)> {
  typedef RetType (Class::*Type)(Args...);
};

template <typename Class, typename RetType, typename... Args>
struct MemberFunctionPtrType<true, Class, RetType(Args...)> {
  typedef RetType (Class::*Type)(Args...) const;
};

template <typename FuncType>
struct Payload;

template <typename RetType, typename... Ts>
struct Payload<RetType(Ts...)> {
  Tuple<Ts..., RetType> values;

  template <typename... Args>
  explicit Payload(Args&&... args)
      : values(Forward<Args>(args)..., RetType()) {}

  RetType& GetResult() { return values.template Get<sizeof...(Ts)>(); }
};

template <typename... Ts>
struct Payload<void(Ts...)> {
  Tuple<Ts...> values;

  template <typename... Args>
  explicit Payload(Args&&... args) : values(Forward<Args>(args)...) {}

  void GetResult() {}
};

template <typename T>
class PlacementNewer {
 public:
  PlacementNewer() : is_constructed_(false) {}

  ~PlacementNewer() {
    if (is_constructed_) {
      reinterpret_cast<T*>(&storage_)->~T();
    }
  }

  template <typename... Args>
  T* operator()(Args&&... args) {
    fun_check(!is_constructed_);
    T* result = new (&storage_) T(Forward<Args>(args)...);
    is_constructed_ = true;
    return result;
  }

  T* operator->() {
    fun_check(is_constructed_);
    return (T*)&storage_;
  }

 private:
  TypeCompatibleStorage<T> storage_;
  bool is_constructed_;
};

template <typename T, typename MemberFunctionPtrType>
class MemberFunctionCaller {
 public:
  MemberFunctionCaller(T* object, MemberFunctionPtrType method)
      : object_(object), method_(method) {}

  template <typename... Args>
  decltype(auto) operator()(Args&&... args) {
    return (object_->*method_)(Forward<Args>(args)...);
  }

 private:
  T* object_;
  MemberFunctionPtrType method_;
};

}  // namespace fun
