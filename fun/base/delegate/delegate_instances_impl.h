#pragma once

#include "fun/base/base.h"
#include "fun/base/delegate/delegate_instance_interface.h"
#include "fun/base/ftl/tuple.h"
#include "fun/base/ftl/type_traits.h"

namespace fun {

class DelegateBase;
class DelegateHandle;
enum class SPMode;

//-----------------------------------------------------------------------------
// Macros for function parameter and delegate payload lists
//-----------------------------------------------------------------------------

// TODO
///**
// * Implements a delegate binding for FFunctions.
// *
// * @params Class Must be an FObject derived class.
// */
// template <typename Class, typename FuncType, typename... Vars>
// class BaseFFunctionDelegateInstance;
//
// template <typename Class, typename WrappedRetType, typename... Params,
// typename... Vars> class BaseFFunctionDelegateInstance<Class, WrappedRetType
// (Params...), Vars...>
//  : public IBaseDelegateInstance<typename UnwrapType<WrappedRetType>::Type
//  (Params...)>
//{
// public:
//  using RetType = typename UnwrapType<WrappedRetType>::Type;
//
// private:
//  using Super = IBaseDelegateInstance<RetType (Params...)>;
//  using UnwrappedThisType = BaseFFunctionDelegateInstance<Class, RetType
//  (Params...), Vars...>;
//
//  static_assert(PointerIsConvertibleFromTo<Class, const FObjectBase>::Value,
//  "You cannot use FFunction delegates with non FObject classes.");
//
// public:
//  BaseFFunctionDelegateInstance(Class* object, const String& function_name,
//  Vars... vars)
//    : function_name_(function_name)
//    , object_ptr_(object)
//    , payload_(vars...)
//    , handle_(DelegateHandle::GenerateNewHandle)
//  {
//    fun_check(!function_name.IsEmpty());
//
//    if (object) {
//      cached_function_ = object_ptr_->FindFunctionChecked(function_name);
//    }
//  }
//
//  // IDelegateInstance interface
//
//#if FUN_USE_DELEGATE_TRYGETBOUNDFUNCTIONNAME
//  String TryGetBoundFunctionName() const override final
//  {
//    return function_name_;
//  }
//#endif
//
//  FObject* GetFObject() const override final
//  {
//    return (FObject*)object_ptr_.Get();
//  }
//
//  // Deprecated
//  bool HasSameObject(const void* object) const override final
//  {
//    return (object_ptr_.Get() == object);
//  }
//
//  bool IsCompactable() const override final
//  {
//    return !object_ptr_.Get(true);
//  }
//
//  bool IsSafeToExecute() const override final
//  {
//    return object_ptr_.IsValid();
//  }
//
// public:
//  // IBaseDelegateInstance interface
//  void CreateCopy(DelegateBase& base) override final
//  {
//    new(base) UnwrappedThisType(*(UnwrappedThisType*)this);
//  }
//
//  RetType Execute(Params... params) const override final
//  {
//    using ParamsWithPayload = Payload<RetType (Params..., Vars...)>;
//
//    fun_check_dbg(IsSafeToExecute());
//
//    PlacementNewer<ParamsWithPayload> payload_and_params;
//    payload_.ApplyAfter(payload_and_params, params...);
//    object_ptr_->ProcessEvent(cached_function_, &payload_and_params);
//    return payload_and_params->GetResult();
//  }
//
//  DelegateHandle GetHandle() const override final
//  {
//    return handle_;
//  }
//
// public:
//  /**
//   * Creates a new FFunction delegate binding for the given user object and
//   function name.
//   *
//   * @param object The user object to call the function on.
//   * @param function_name The name of the function call.
//   *
//   * @return The new delegate.
//   */
//  FUN_ALWAYS_INLINE static void Create(DelegateBase& base, Class* object,
//  const String& function_name, Vars... vars)
//  {
//    new(base) UnwrappedThisType(object, function_name, vars...);
//  }
//
// public:
//  /** Holds the cached FFunction to call. */
//  FFunction* cached_function_;
//
//  /** Holds the name of the function to call. */
//  String function_name_;
//
//  /** The user object to call the function on. */
//  WeakObjectPtr<Class> object_ptr_;
//
//  Tuple<Vars...> payload_;
//
//  /** The handle of this delegate */
//  DelegateHandle handle_;
//};
//
// template <typename Class, typename... Params, typename... Vars>
// class BaseFFunctionDelegateInstance<Class, void (Params...), Vars...>
//  : public BaseFFunctionDelegateInstance<Class, TypeWrapper<void> (Params...),
//  Vars...>
//{
//  using Super = BaseFFunctionDelegateInstance<Class, TypeWrapper<void>
//  (Params...), Vars...>;
//
// public:
//  /**
//   * Creates and initializes a new instance.
//   *
//   * @param object The FObject to call the function on.
//   * @param function_name The name of the function call.
//   */
//  BaseFFunctionDelegateInstance(Class* object, const String& function_name,
//  Vars... vars)
//    : Super(object, function_name, vars...)
//  {}
//
//  bool ExecuteIfSafe(Params... params) const override final
//  {
//    if (Super::IsSafeToExecute()) {
//      Super::Execute(params...);
//      return true;
//    }
//
//    return false;
//  }
//};

//-----------------------------------------------------------------------------
// Delegate binding types
//-----------------------------------------------------------------------------

/**
 * Implements a delegate binding for shared pointer member functions.
 */
template <bool IsConst, typename Class, SPMode SPMode, typename FuncType,
          typename... Vars>
class BaseSPMethodDelegateInstance;

template <bool IsConst, typename Class, SPMode SPMode, typename WrappedRetType,
          typename... Params, typename... Vars>
class BaseSPMethodDelegateInstance<IsConst, Class, SPMode,
                                   WrappedRetType(Params...), Vars...>
    : public IBaseDelegateInstance<typename UnwrapType<WrappedRetType>::Type(
          Params...)> {
 public:
  using RetType = typename UnwrapType<WrappedRetType>::Type;

 private:
  using Super = IBaseDelegateInstance<RetType(Params...)>;
  using UnwrappedThisType =
      BaseSPMethodDelegateInstance<IsConst, Class, SPMode, RetType(Params...),
                                   Vars...>;

 public:
  using MethodPtr =
      typename MemberFunctionPtrType<IsConst, Class,
                                     RetType(Params..., Vars...)>::Type;

  BaseSPMethodDelegateInstance(const SharedPtr<Class, SPMode>& object,
                               MethodPtr method, Vars... vars)
      : object_(object),
        method_(method),
        payload_(vars...),
        handle_(DelegateHandle::GenerateNewHandle) {
    // NOTE: Shared pointer delegates are allowed to have a null incoming object
    // pointer.  Weak pointers can expire,
    //       an it is possible for a copy of a delegate instance to end up with
    //       a null pointer.
    fun_check_dbg(method_ != nullptr);
  }

  // IDelegateInstance interface

#if FUN_USE_DELEGATE_TRYGETBOUNDFUNCTIONNAME
  String TryGetBoundFunctionName() const override final { return String(); }
#endif

  FObject* GetFObject() const override final { return nullptr; }

  // Deprecated
  bool HasSameObject(const void* object) const override final {
    return object_.HasSameObject(object);
  }

  bool IsSafeToExecute() const override final { return object_.IsValid(); }

 public:
  // IBaseDelegateInstance interface
  void CreateCopy(DelegateBase& base) override final {
    new (base) UnwrappedThisType(*(UnwrappedThisType*)this);
  }

  RetType Execute(Params... params) const override final {
    using MutableUserClass = typename RemoveConst<Class>::Type;

    // Verify that the user object is still valid.  We only have a weak
    // reference to it.
    auto shared_object = object_.Lock();
    fun_check_dbg(shared_object.IsValid());

    // Safely remove const to work around a compiler issue with instantiating
    // template permutations for overloaded functions that take a function
    // pointer typedef as a member of a templated class.  In all cases where
    // this code is actually invoked, the Class will already be a const pointer.
    auto mutable_object = const_cast<MutableUserClass*>(shared_object.Get());

    // Call the member function on the user's object.  And yes, this is the
    // correct C++ syntax for calling a pointer-to-member function.
    fun_check_dbg(method_ != nullptr);

    return payload_.ApplyAfter(
        MemberFunctionCaller<MutableUserClass, MethodPtr>(mutable_object,
                                                          method_),
        params...);
  }

  DelegateHandle GetHandle() const override final { return handle_; }

 public:
  /**
   * Creates a new shared pointer delegate binding for the given user object and
   * method pointer.
   *
   * @param object_ref Shared reference to the user's object that contains the
   * class method.
   * @param func Member function pointer to your class method.
   *
   * @return The new delegate.
   */
  FUN_ALWAYS_INLINE static void Create(
      DelegateBase& base, const SharedPtr<Class, SPMode>& object_ref,
      MethodPtr func, Vars... vars) {
    new (base) UnwrappedThisType(object_ref, func, vars...);
  }

  /**
   * Creates a new shared pointer delegate binding for the given user object and
   * method pointer.
   *
   * This overload requires that the supplied object derives from
   * TSharedFromThis.
   *
   * @param object The user's object that contains the class method.  Must
   * derive from TSharedFromThis.
   * @param func Member function pointer to your class method.
   *
   * @return The new delegate.
   */
  FUN_ALWAYS_INLINE static void Create(DelegateBase& base, Class* object,
                                       MethodPtr func, Vars... vars) {
    // We expect the incoming object to derived from TSharedFromThis.
    auto UserObjectRef = StaticCastSharedRef<Class>(object->AsShared());
    Create(base, UserObjectRef, func, vars...);
  }

 protected:
  /** Weak reference to an instance of the user's class which contains a method
   * we would like to call. */
  WeakPtr<Class, SPMode> object_;

  /** C++ member function pointer. */
  MethodPtr method_;

  /** payload_ member variables, if any. */
  Tuple<Vars...> payload_;

  /** The handle of this delegate */
  DelegateHandle handle_;
};

template <bool IsConst, typename Class, SPMode SPMode, typename... Params,
          typename... Vars>
class BaseSPMethodDelegateInstance<IsConst, Class, SPMode, void(Params...),
                                   Vars...>
    : public BaseSPMethodDelegateInstance<
          IsConst, Class, SPMode, TypeWrapper<void>(Params...), Vars...> {
  using Super =
      BaseSPMethodDelegateInstance<IsConst, Class, SPMode,
                                   TypeWrapper<void>(Params...), Vars...>;

 public:
  /**
   * Creates and initializes a new instance.
   *
   * @param object A shared reference to an arbitrary object (templated) that
   * hosts the member function.
   * @param method C++ member function pointer for the method to bind.
   */
  BaseSPMethodDelegateInstance(const SharedPtr<Class, SPMode>& object,
                               typename Super::MethodPtr method, Vars... vars)
      : Super(object, method, vars...) {}

  bool ExecuteIfSafe(Params... params) const override final {
    // Verify that the user object is still valid.
    // We only have a weak reference to it.
    auto shared_object = Super::object_.Lock();
    if (shared_object.IsValid()) {
      Super::Execute(params...);
      return true;
    }

    return false;
  }
};

/**
 * Implements a delegate binding for C++ member functions.
 */
template <bool IsConst, typename Class, typename FuncType, typename... Vars>
class BaseRawMethodDelegateInstance;

template <bool IsConst, typename Class, typename WrappedRetType,
          typename... Params, typename... Vars>
class BaseRawMethodDelegateInstance<IsConst, Class, WrappedRetType(Params...),
                                    Vars...>
    : public IBaseDelegateInstance<typename UnwrapType<WrappedRetType>::Type(
          Params...)> {
 public:
  using RetType = typename UnwrapType<WrappedRetType>::Type;

 private:
  static_assert(!PointerIsConvertibleFromTo<Class, const FObjectBase>::Value,
                "You cannot use raw method delegates with FObjects.");

  using Super = IBaseDelegateInstance<RetType(Params...)>;
  using UnwrappedThisType =
      BaseRawMethodDelegateInstance<IsConst, Class, RetType(Params...),
                                    Vars...>;

 public:
  using MethodPtr =
      typename MemberFunctionPtrType<IsConst, Class,
                                     RetType(Params..., Vars...)>::Type;

  /**
   * Creates and initializes a new instance.
   *
   * @param object An arbitrary object (templated) that hosts the member
   * function.
   * @param method C++ member function pointer for the method to bind.
   */
  BaseRawMethodDelegateInstance(Class* object, MethodPtr method, Vars... vars)
      : object_(object),
        method_(method),
        payload_(vars...),
        handle_(DelegateHandle::GenerateNewHandle) {
    // Non-expirable delegates must always have a non-null object pointer
    // on creation (otherwise they could never execute.)
    fun_check(object != nullptr && method_ != nullptr);
  }

  // IDelegateInstance interface

#if FUN_USE_DELEGATE_TRYGETBOUNDFUNCTIONNAME
  String TryGetBoundFunctionName() const override final { return String(); }
#endif

  FObject* GetFObject() const override final { return nullptr; }

  // Deprecated
  bool HasSameObject(const void* object) const override final {
    return object_ == object;
  }

  bool IsSafeToExecute() const override final {
    // We never know whether or not it is safe to deference
    // a C++ pointer, but we have to trust the user in this case.
    // Prefer using a shared-pointer based delegate type instead.
    return true;
  }

 public:
  // IBaseDelegateInstance interface

  void CreateCopy(DelegateBase& base) override final {
    new (base) UnwrappedThisType(*(UnwrappedThisType*)this);
  }

  RetType Execute(Params... params) const override final {
    using MutableUserClass = typename RemoveConst<Class>::Type;

    // Safely remove const to work around a compiler issue with
    // instantiating template permutations for overloaded functions
    // that take a function pointer typedef as a member of a templated class.
    // In all cases where this code is actually invoked, the Class will
    // already be a const pointer.
    auto mutable_object = const_cast<MutableUserClass*>(object_);

    // Call the member function on the user's object.
    // And yes, this is the correct C++ syntax for calling a
    // pointer-to-member function.
    fun_check_dbg(method_ != nullptr);

    return payload_.ApplyAfter(
        MemberFunctionCaller<MutableUserClass, MethodPtr>(mutable_object,
                                                          method_),
        params...);
  }

  DelegateHandle GetHandle() const override final { return handle_; }

 public:
  /**
   * Creates a new raw method delegate binding for
   * the given user object and function pointer.
   *
   * @param object User's object that contains the class method.
   * @param func Member function pointer to your class method.
   */
  FUN_ALWAYS_INLINE static void Create(DelegateBase& base, Class* object,
                                       MethodPtr func, Vars... vars) {
    new (base) UnwrappedThisType(object, func, vars...);
  }

 protected:
  /** Pointer to the user's class which contains a method we would like to call.
   */
  Class* object_;

  /** C++ member function pointer. */
  MethodPtr method_;

  /** payload_ member variables (if any). */
  Tuple<Vars...> payload_;

  /** The handle of this delegate */
  DelegateHandle handle_;
};

template <bool IsConst, typename Class, typename... Params, typename... Vars>
class BaseRawMethodDelegateInstance<IsConst, Class, void(Params...), Vars...>
    : public BaseRawMethodDelegateInstance<
          IsConst, Class, TypeWrapper<void>(Params...), Vars...> {
  using Super =
      BaseRawMethodDelegateInstance<IsConst, Class,
                                    TypeWrapper<void>(Params...), Vars...>;

 public:
  /**
   * Creates and initializes a new instance.
   *
   * @param object An arbitrary object (templated) that hosts the member
   * function.
   * @param method C++ member function pointer for the method to bind.
   */
  BaseRawMethodDelegateInstance(Class* object, typename Super::MethodPtr method,
                                Vars... vars)
      : Super(object, method, vars...) {}

  bool ExecuteIfSafe(Params... params) const override final {
    // We never know whether or not it is safe to deference a C++ pointer, but
    // we have to trust the user in this case.  Prefer using a shared-pointer
    // based delegate type instead!
    Super::Execute(params...);
    return true;
  }
};

///**
// * Implements a delegate binding for FObject methods.
// */
// template <bool IsConst, typename Class, typename FuncType, typename... Vars>
// class BaseFObjectMethodDelegateInstance;
//
// template <bool IsConst, typename Class, typename WrappedRetType, typename...
// Params, typename... Vars> class BaseFObjectMethodDelegateInstance<IsConst,
// Class, WrappedRetType (Params...), Vars...>
//  : public IBaseDelegateInstance<typename UnwrapType<WrappedRetType>::Type
//  (Params...)>
//{
// public:
//  using RetType = typename UnwrapType<WrappedRetType>::Type;
//
// private:
//  using Super = IBaseDelegateInstance<RetType (Params...)>;
//  using UnwrappedThisType = BaseFObjectMethodDelegateInstance<IsConst, Class,
//  RetType (Params...), Vars...>;
//
//  static_assert(PointerIsConvertibleFromTo<Class, const FObjectBase>::Value,
//  "You cannot use FObject method delegates with raw pointers.");
//
// public:
//  using MethodPtr = typename MemberFunctionPtrType<IsConst, Class, RetType
//  (Params..., Vars...)>::Type;
//
//  BaseFObjectMethodDelegateInstance(Class* object, MethodPtr method, Vars...
//  vars)
//    : object_(object)
//    , method_(method)
//    , payload_(vars...)
//    , handle_(DelegateHandle::GenerateNewHandle)
//  {
//    // NOTE: FObject delegates are allowed to have a null incoming object
//    pointer.  FObject weak pointers can expire,
//    //       an it is possible for a copy of a delegate instance to end up
//    with a null pointer. fun_check_dbg(method_ != nullptr);
//  }
//
//  // IDelegateInstance interface
//
//#if FUN_USE_DELEGATE_TRYGETBOUNDFUNCTIONNAME
//  String TryGetBoundFunctionName() const override final
//  {
//    return String();
//  }
//#endif
//
//  FObject* GetFObject() const override final
//  {
//    return (FObject*)object_.Get();
//  }
//
//  // Deprecated
//  bool HasSameObject(const void* object) const override final
//  {
//    return object_.Get() == object;
//  }
//
//  bool IsCompactable() const override final
//  {
//    return !object_.Get(true);
//  }
//
//  bool IsSafeToExecute() const override final
//  {
//    return !!object_.Get();
//  }
//
// public:
//  // IBaseDelegateInstance interface
//
//  void CreateCopy(DelegateBase& base) override final
//  {
//    new(base) UnwrappedThisType(*(UnwrappedThisType*)this);
//  }
//
//  RetType Execute(Params... params) const override final
//  {
//    using MutableUserClass = typename RemoveConst<Class>::Type;
//
//    // Verify that the user object is still valid.  We only have a weak
//    reference to it. fun_check_dbg(object_.IsValid());
//
//    // Safely remove const to work around a compiler issue with instantiating
//    template permutations for
//    // overloaded functions that take a function pointer typedef as a member
//    of a templated class.  In
//    // all cases where this code is actually invoked, the Class will already
//    be a const pointer. auto mutable_object =
//    const_cast<MutableUserClass*>(object_.Get());
//
//    // Call the member function on the user's object.
//    // And yes, this is the correct C++ syntax for calling a pointer-to-member
//    function. fun_check_dbg(method_ != nullptr);
//
//    return payload_.ApplyAfter(MemberFunctionCaller<MutableUserClass,
//    MethodPtr>(mutable_object, method_), params...);
//  }
//
//  DelegateHandle GetHandle() const override final
//  {
//    return handle_;
//  }
//
// public:
//  /**
//   * Creates a new FObject delegate binding for the given user object and
//   method pointer.
//   *
//   * @param object User's object that contains the class method.
//   * @param func Member function pointer to your class method.
//   *
//   * @return The new delegate.
//   */
//  FUN_ALWAYS_INLINE static void Create(DelegateBase& base, Class* object,
//  MethodPtr func, Vars... vars)
//  {
//    new(base) UnwrappedThisType(object, func, vars...);
//  }
//
// protected:
//  /** Pointer to the user's class which contains a method we would like to
//  call. */ WeakObjectPtr<Class> object_;
//
//  /** C++ member function pointer. */
//  MethodPtr method_;
//
//  /** payload_ member variables (if any). */
//  Tuple<Vars...> payload_;
//
//  /** The handle of this delegate */
//  DelegateHandle handle_;
//};
//
// template <bool IsConst, typename Class, typename... Params, typename... Vars>
// class BaseFObjectMethodDelegateInstance<IsConst, Class, void (Params...),
// Vars...>
//  : public BaseFObjectMethodDelegateInstance<IsConst, Class, TypeWrapper<void>
//  (Params...), Vars...>
//{
//  using Super = BaseFObjectMethodDelegateInstance<IsConst, Class,
//  TypeWrapper<void> (Params...), Vars...>;
//
// public:
//  /**
//   * Creates and initializes a new instance.
//   *
//   * @param object An arbitrary object (templated) that hosts the member
//   function.
//   * @param method C++ member function pointer for the method to bind.
//   */
//  BaseFObjectMethodDelegateInstance(Class* object, typename Super::MethodPtr
//  method, Vars... vars)
//    : Super(object, method, vars...)
//  {}
//
//  bool ExecuteIfSafe(Params... params) const override final
//  {
//    // Verify that the user object is still valid.  We only have a weak
//    reference to it. auto actual_object = Super::object_.Get(); if
//    (actual_object) {
//      Super::Execute(params...);
//      return true;
//    }
//
//    return false;
//  }
//};

/**
 * Implements a delegate binding for regular C++ functions.
 */
template <typename FuncType, typename... Vars>
class BaseStaticDelegateInstance;

template <typename WrappedRetType, typename... Params, typename... Vars>
class BaseStaticDelegateInstance<WrappedRetType(Params...), Vars...>
    : public IBaseDelegateInstance<typename UnwrapType<WrappedRetType>::Type(
          Params...)> {
 public:
  using RetType = typename UnwrapType<WrappedRetType>::Type;

  using Super = IBaseDelegateInstance<RetType(Params...)>;
  using UnwrappedThisType =
      BaseStaticDelegateInstance<RetType(Params...), Vars...>;

 public:
  typedef RetType (*FuncPtr)(Params..., Vars...);

  BaseStaticDelegateInstance(FuncPtr static_function, Vars... vars)
      : static_function_(static_function),
        payload_(vars...),
        handle_(DelegateHandle::GenerateNewHandle) {
    fun_check_ptr(static_function_);
  }

  // IDelegateInstance interface
#if FUN_USE_DELEGATE_TRYGETBOUNDFUNCTIONNAME
  String TryGetBoundFunctionName() const override final { return String(); }
#endif

  FObject* GetFObject() const override final { return nullptr; }

  // Deprecated
  bool HasSameObject(const void* object_) const override final {
    // Raw Delegates aren't bound to an object so they can never match
    return false;
  }

  bool IsSafeToExecute() const override final {
    // Static functions are always safe to execute!
    return true;
  }

 public:
  // IBaseDelegateInstance interface

  void CreateCopy(DelegateBase& base) override final {
    new (base) UnwrappedThisType(*(UnwrappedThisType*)this);
  }

  RetType Execute(Params... params) const override final {
    // Call the static function
    fun_check_dbg(static_function_ != nullptr);

    return payload_.ApplyAfter(static_function_, params...);
  }

  DelegateHandle GetHandle() const override final { return handle_; }

 public:
  /**
   * Creates a new static function delegate binding for the given function
   * pointer.
   *
   * @param func Static function pointer.
   *
   * @return The new delegate.
   */
  FUN_ALWAYS_INLINE static void Create(DelegateBase& base, FuncPtr func,
                                       Vars... vars) {
    new (base) UnwrappedThisType(func, vars...);
  }

 private:
  /** C++ function pointer. */
  FuncPtr static_function_;

  /** payload_ member variables, if any. */
  Tuple<Vars...> payload_;

  /** The handle of this delegate */
  DelegateHandle handle_;
};

template <typename... Params, typename... Vars>
class BaseStaticDelegateInstance<void(Params...), Vars...>
    : public BaseStaticDelegateInstance<TypeWrapper<void>(Params...), Vars...> {
  using Super =
      BaseStaticDelegateInstance<TypeWrapper<void>(Params...), Vars...>;

 public:
  /**
   * Creates and initializes a new instance.
   *
   * @param static_function C++ function pointer.
   */
  BaseStaticDelegateInstance(typename Super::FuncPtr static_function,
                             Vars... vars)
      : Super(static_function, vars...) {}

  bool ExecuteIfSafe(Params... params) const override final {
    Super::Execute(params...);
    return true;
  }
};

/**
 * Implements a delegate binding for C++ functors, e.g. lambdas.
 */
template <typename FuncType, typename Functor, typename... Vars>
class BaseFunctorDelegateInstance;

template <typename WrappedRetType, typename... Params, typename Functor,
          typename... Vars>
class BaseFunctorDelegateInstance<WrappedRetType(Params...), Functor, Vars...>
    : public IBaseDelegateInstance<typename UnwrapType<WrappedRetType>::Type(
          Params...)> {
 public:
  using RetType = typename UnwrapType<WrappedRetType>::Type;

 private:
  static_assert(IsSame<Functor, typename RemoveReference<Functor>::Type>::Value,
                "Functor cannot be a reference");

  using Super = IBaseDelegateInstance<typename UnwrapType<WrappedRetType>::Type(
      Params...)>;
  using UnwrappedThisType =
      BaseFunctorDelegateInstance<RetType(Params...), Functor, Vars...>;

 public:
  BaseFunctorDelegateInstance(const Functor& functor, Vars... vars)
      : functor_(functor),
        payload_(vars...),
        handle_(DelegateHandle::GenerateNewHandle) {}

  BaseFunctorDelegateInstance(Functor&& functor, Vars... vars)
      : functor_(MoveTemp(functor)),
        payload_(vars...),
        handle_(DelegateHandle::GenerateNewHandle) {}

  // IDelegateInstance interface
#if FUN_USE_DELEGATE_TRYGETBOUNDFUNCTIONNAME
  String TryGetBoundFunctionName() const override final { return String(); }
#endif

  FObject* GetFObject() const override final { return nullptr; }

  // Deprecated
  bool HasSameObject(const void* object) const override final {
    // Functor delegates aren't bound to a user object so they can never match.
    return false;
  }

  bool IsSafeToExecute() const override final {
    // Functors are always considered safe to execute.
    return true;
  }

 public:
  // IBaseDelegateInstance interface
  void CreateCopy(DelegateBase& base) override final {
    new (base) UnwrappedThisType(*(UnwrappedThisType*)this);
  }

  RetType Execute(Params... params) const override final {
    return payload_.ApplyAfter(functor_, params...);
  }

  DelegateHandle GetHandle() const override final { return handle_; }

 public:
  /**
   * Creates a new static function delegate binding for
   * the given function pointer.
   *
   * @param functor C++ functor
   *
   * @return The new delegate.
   */
  FUN_ALWAYS_INLINE static void Create(DelegateBase& base,
                                       const Functor& functor, Vars... vars) {
    new (base) UnwrappedThisType(functor, vars...);
  }

  FUN_ALWAYS_INLINE static void Create(DelegateBase& base, Functor&& functor,
                                       Vars... vars) {
    new (base) UnwrappedThisType(MoveTemp(functor), vars...);
  }

 private:
  /**
   * C++ functor
   * We make this mutable to allow mutable lambdas to be bound and executed.
   * We don't really want to model the functor as being a direct subobject
   * of the delegate (which would maintain transivity of const -
   * because the binding doesn't affect the substitutability
   * of a copied delegate.
   */
  mutable typename RemoveConst<Functor>::Type functor_;

  /** Payload_ member variables, if any. */
  Tuple<Vars...> payload_;

  /** The handle of this delegate */
  DelegateHandle handle_;
};

template <typename Functor, typename... Params, typename... Vars>
class BaseFunctorDelegateInstance<void(Params...), Functor, Vars...>
    : public BaseFunctorDelegateInstance<TypeWrapper<void>(Params...), Functor,
                                         Vars...> {
  using Super = BaseFunctorDelegateInstance<TypeWrapper<void>(Params...),
                                            Functor, Vars...>;

 public:
  /**
   * Creates and initializes a new instance.
   *
   * @param functor C++ functor
   */
  BaseFunctorDelegateInstance(const Functor& functor, Vars... vars)
      : Super(functor, vars...) {}

  BaseFunctorDelegateInstance(Functor&& functor, Vars... vars)
      : Super(MoveTemp(functor), vars...) {}

  bool ExecuteIfSafe(Params... params) const override final {
    // Functors are always considered safe to execute.
    Super::Execute(params...);
    return true;
  }
};

}  // namespace fun
