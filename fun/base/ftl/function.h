#pragma once

#include "fun/base/base.h"

#include <new>

#include "fun/base/container/allocation_policies.h"
#include "fun/base/ftl/template.h"
#include "fun/base/ftl/type_traits.h"

//#include "Math/MathUtility.h"
//#include "HAL/FunMemory.h"

//#include "fun/base/math/math_base.h"
//#include "fun/base/memory.h"

// Disable visualization hack for shipping or test builds.
#if !(FUN_BUILD_SHIPPING || FUN_BUILD_TEST)
#define FUN_ENABLE_TFUNCTIONREF_VISUALIZATION 1
#else
#define FUN_ENABLE_TFUNCTIONREF_VISUALIZATION 0
#endif

namespace fun {

/**
 * TFunction<FunctionType>
 *
 * See the class definition for intended usage.
 */
template <typename FunctionType>
class TFunction;

/**
 * TFunctionRef<FunctionType>
 *
 * See the class definition for intended usage.
 */
template <typename FunctionType>
class TFunctionRef;

/**
 * Traits class which checks if T is a TFunction<> type.
 */
template <typename T>
struct IsATFunction : FalseType {};
template <typename T>
struct IsATFunction<TFunction<T>> : TrueType {};

/**
 * Traits class which checks if T is a TFunction<> type.
 */
template <typename T>
struct IsATFunctionRef : FalseType {};
template <typename T>
struct IsATFunctionRef<TFunctionRef<T>> : TrueType {};

// Private implementation details of TFunction and TFunctionRef.
namespace Function_internal {

struct FunctionStorage;

/**
 * Common interface to a callable object owned by TFunction.
 */
struct IFunction_OwnedObject {
  /**
   * Creates a copy of the object in the allocator and returns a pointer to it.
   */
  virtual IFunction_OwnedObject* CopyToEmptyStorage(
      FunctionStorage& storage) const = 0;

  /**
   * Returns the address of the object.
   */
  virtual void* GetAddress() = 0;

  /**
   * Destructor.
   */
  virtual ~IFunction_OwnedObject() = 0;
};

/**
 * Destructor.
 */
FUN_ALWAYS_INLINE IFunction_OwnedObject::~IFunction_OwnedObject() {}

#if !defined(_WIN32) || defined(_WIN64)
// Let TFunction store up to 32 bytes which are 16-byte aligned before we heap
// allocate
typedef AlignedStorage<16, 16> AlignedInlineFunctionType;
typedef InlineAllocator<2> FunctionAllocatorType;
#else
// ... except on Win32, because we can't pass 16-byte aligned types by value, as
// some TFunctions are. So we'll just keep it heap-allocated, which is always
// sufficiently aligned.
typedef AlignedStorage<16, 8> AlignedInlineFunctionType;
typedef HeapAllocator FunctionAllocatorType;
#endif

struct FunctionStorage {
  FunctionStorage() : allocated_size(0) {}

  FunctionStorage(FunctionStorage&& other) : allocated_size(0) {
    allocator.MoveToEmpty(other.allocator);
    allocated_size = other.allocated_size;
    other.allocated_size = 0;
  }

  void Clear() {
    allocator.ResizeAllocation(
        0, 0, sizeof(Function_internal::AlignedInlineFunctionType));
    allocated_size = 0;
  }

  typedef FunctionAllocatorType::ForElementType<AlignedInlineFunctionType>
      AllocatorType;

  IFunction_OwnedObject* GetBoundObject() const {
    return allocated_size > 0
               ? (IFunction_OwnedObject*)allocator.GetAllocation()
               : nullptr;
  }

  AllocatorType allocator;
  size_t allocated_size;
};
}  // namespace Function_internal

}  // namespace fun

// 전역 new는 namespace안에 놓일 수 없음.
FUN_ALWAYS_INLINE void* operator new(
    fun::size_t size, fun::Function_internal::FunctionStorage& storage) {
  if (auto* obj = storage.GetBoundObject()) {
    obj->~IFunction_OwnedObject();
  }

  const fun::size_t new_size = fun::MathBase::DivideAndRoundUp(
      size, sizeof(fun::Function_internal::AlignedInlineFunctionType));
  if (storage.allocated_size != new_size) {
    storage.allocator.ResizeAllocation(
        0, (fun::int32)new_size,
        sizeof(fun::Function_internal::AlignedInlineFunctionType));
    storage.allocated_size = new_size;
  }

  return storage.allocator.GetAllocation();
}

namespace fun {

namespace Function_internal {

/**
 * Implementation of IFunction_OwnedObject for a given T.
 */
template <typename T>
struct TFunction_OwnedObject : public IFunction_OwnedObject {
  /**
   * Constructor which creates its T by copying.
   */
  explicit TFunction_OwnedObject(const T& obj) : obj(obj) {}

  /**
   * Constructor which creates its T by moving.
   */
  explicit TFunction_OwnedObject(T&& obj) : obj(MoveTemp(obj)) {}

  IFunction_OwnedObject* CopyToEmptyStorage(
      FunctionStorage& storage) const override {
    return new (storage) TFunction_OwnedObject(obj);
  }

  void* GetAddress() override { return &obj; }

  T obj;
};

/**
 * A class which is used to instantiate the code needed to call a bound
 * function.
 */
template <typename Functor, typename FunctionType>
struct TFunctionRefCaller;

/**
 * A class which is used to instantiate the code needed to assert when called -
 * used for null bindings.
 */
template <typename FunctionType>
struct TFunctionRefAsserter;

/**
 * A class which defines an operator() which will invoke the
 * TFunctionRefCaller::Call function.
 */
template <typename DerivedType, typename FunctionType>
struct TFunctionRefBase;

#if FUN_ENABLE_TFUNCTIONREF_VISUALIZATION
/**
 * Helper classes to help debugger visualization.
 */
struct IDebugHelper {
  virtual ~IDebugHelper() = 0;
};

FUN_ALWAYS_INLINE IDebugHelper::~IDebugHelper() {}

template <typename T>
struct TDebugHelper : IDebugHelper {
  T* ptr;
};
#endif

template <typename T>
FUN_ALWAYS_INLINE T&& FakeCall(T* ptr) {
  return MoveTemp(*ptr);
}

FUN_ALWAYS_INLINE void FakeCall(void* ptr) {}

template <typename FunctionType, typename CallableType>
struct TFunctionRefBaseCommon {
  explicit TFunctionRefBaseCommon(NoInit_TAG) {
    // Not really designed to be initialized directly, but want to be explicit
    // about that.
  }

  template <typename FunctorType>
  void Set(FunctorType* functor) {
    callable_ =
        &Function_internal::TFunctionRefCaller<FunctorType, FunctionType>::Call;

#if FUN_ENABLE_TFUNCTIONREF_VISUALIZATION
    // We placement new over the top of the same object each time.  This is
    // illegal, but it ensures that the vptr is set correctly for the bound
    // type, and so is visualizable.  We never depend on the state of this
    // object at runtime, so it's ok.
    new ((void*)&debug_ptr_storage_)
        Function_internal::TDebugHelper<FunctorType>;
    debug_ptr_storage_.ptr = (void*)functor;
#endif
  }

  void CopyAndReseat(const TFunctionRefBaseCommon& other, void* functor) {
    callable_ = other.callable_;

#if FUN_ENABLE_TFUNCTIONREF_VISUALIZATION
    // Use Memcpy to copy the other debug_ptr_storage_, including vptr (because
    // we don't know the bound type here), and then reseat the underlying
    // pointer.  Possibly even more evil than the Set code.
    UnsafeMemory::Memcpy(&debug_ptr_storage_, &other.debug_ptr_storage_,
                         sizeof(debug_ptr_storage_));
    debug_ptr_storage_.ptr = functor;
#endif
  }

  void Unset() {
    callable_ = &Function_internal::TFunctionRefAsserter<FunctionType>::Call;
  }

  CallableType* GetCallable() const { return callable_; }

 private:
  // A pointer to a function which invokes the call operator on the callable
  // object
  CallableType* callable_;

#if FUN_ENABLE_TFUNCTIONREF_VISUALIZATION
  // To help debug visualizers
  Function_internal::TDebugHelper<void> debug_ptr_storage_;
#endif
};

/**
 * Switch on the existence of variadics.  Once all our supported compilers
 * support variadics, a lot of this code
 *
 * can be collapsed into FunctionRefCaller.
 * They're currently separated out to minimize the amount of workarounds
 * needed.
 */
template <typename Functor, typename R, typename... Args>
struct TFunctionRefCaller<Functor, R(Args...)> {
  static R Call(void* obj, Args&... args) {
    return (*(Functor*)obj)(Forward<Args>(args)...);
  }
};

/**
 * Specialization for void return types.
 */
template <typename Functor, typename... Args>
struct TFunctionRefCaller<Functor, void(Args...)> {
  static void Call(void* obj, Args&... args) {
    (*(Functor*)obj)(Forward<Args>(args)...);
  }
};

template <typename R, typename... Args>
struct TFunctionRefAsserter<R(Args...)> {
  static R Call(void* obj, Args&...) {
    fun_check_msg(false, "Attempting to call a null TFunction!");

    // This doesn't need to be valid, because it'll never be reached,
    // but it does at least need to compile.
    return FakeCall((R*)obj);
  }
};

template <typename DerivedType, typename R, typename... Args>
struct TFunctionRefBase<DerivedType, R(Args...)>
    : TFunctionRefBaseCommon<R(Args...), R(void*, Args&...)> {
  typedef TFunctionRefBaseCommon<R(Args...), R(void*, Args&...)> Super;

  explicit TFunctionRefBase(NoInit_TAG) : Super(NoInit) {}

  template <typename FunctorType>
  explicit TFunctionRefBase(FunctorType* functor) : Super(functor) {}

  R operator()(Args... args) const {
    const auto* derived = static_cast<const DerivedType*>(this);
    return this->GetCallable()(derived->GetPtr(), args...);
  }
};
}  // namespace Function_internal

/**
 * TFunctionRef<FunctionType>
 *
 * A class which represents a reference to something callable.
 * The important part here is *reference* - if you bind it to a lambda and
 * the lambda goes out of scope, you will be left with an invalid reference.
 *
 * FunctionType represents a function type and so TFunctionRef should be
 * defined as follows:
 *
 * // A function taking a string and float and returning int32.
 * // Parameter names are optional.
 * TFunctionRef<int32 (const string& name, float scale)>
 *
 * If you also want to take ownership of the callable thing,
 * e.g. you want to return a lambda from a
 * function, you should use TFunction.  TFunctionRef does not concern
 * itself with ownership because it'sintended to be FAST.
 *
 * TFunctionRef is most useful when you want to parameterize a function
 * with some caller-defined code without making it a template.
 *
 * Example:
 *
 * // something.h
 * void DoSomethingWithConvertingStringsToInts(TFunctionRef<int32 (const String&
 * str)> convert);
 *
 * // something.cc
 * void DoSomethingWithConvertingStringsToInts(TFunctionRef<int32 (const String&
 * str)> convert) { for (const String& str : SomeBunchOfStrings) { int32 i =
 * func(str); DoSomething(i);
 *   }
 * }
 *
 * // somewhere_else.cc
 * #include "something.h"
 *
 * void func() {
 *   // First do something using string length
 *   DoSomethingWithConvertingStringsToInts([](const String& str) {
 *     return str.Len();
 *   });
 *
 *   // Then do something using string conversion
 *   DoSomethingWithConvertingStringsToInts([](const String& str) {
 *     int32 result;
 *     TypeFromString<int32>::FromString(result, *str);
 *     return result;
 *   });
 * }
 */
template <typename FunctionType>
class TFunctionRef
    : public Function_internal::TFunctionRefBase<TFunctionRef<FunctionType>,
                                                 FunctionType> {
  friend struct Function_internal::TFunctionRefBase<TFunctionRef<FunctionType>,
                                                    FunctionType>;

  typedef Function_internal::TFunctionRefBase<TFunctionRef<FunctionType>,
                                              FunctionType>
      Super;

 public:
  /**
   * Constructor which binds a TFunctionRef to a non-const lvalue function
   * object.
   */
  template <typename FunctorType,
            typename = typename EnableIf<
                !IsCppFunction<FunctorType>::Value &&
                !IsSame<TFunctionRef, FunctorType>::Value>::Type>
  TFunctionRef(FunctorType& functor) : Super(NoInit) {
    // This constructor is disabled for function types
    // because we want it to call the function pointer overload.
    // It is also disabled for TFunctionRef types
    // because VC is incorrectly treating it as a copy constructor.
    Set(&functor);
  }

  /**
   * Constructor which binds a TFunctionRef to an rvalue or const lvalue
   * function object.
   */
  template <typename FunctorType,
            typename = typename EnableIf<
                !IsCppFunction<FunctorType>::Value &&
                !IsSame<TFunctionRef, FunctorType>::Value>::Type>
  TFunctionRef(const FunctorType& functor) : Super(NoInit) {
    // This constructor is disabled for function types
    // because we want it to call the function pointer overload.
    // It is also disabled for TFunctionRef types
    // because VC is incorrectly treating it as a copy constructor.
    Set(&functor);
  }

  /**
   * Constructor which binds a TFunctionRef to a function pointer.
   */
  template <
      typename FunctionType,
      typename = typename EnableIf<IsCppFunction<FunctionType>::Value>::Type>
  TFunctionRef(FunctionType* function) : Super(NoInit) {
    // This constructor is enabled only for function types
    // because we don't want weird errors from it being called with arbitrary
    // pointers.
    Set(function);
  }

#if FUN_ENABLE_TFUNCTIONREF_VISUALIZATION
  /**
   * Copy constructor.
   */
  TFunctionRef(const TFunctionRef& other) : Super(NoInit) {
    // If visualization is enabled, then we need to do an explicit copy
    // to ensure that our hacky debug_ptr_storage_'s vptr is copied.
    CopyAndReseat(other, other.ptr_);
  }
#endif

// We delete the assignment operators
// because we don't want it to be confused with being related to
// regular C++ reference assignment
// - i.e. calling the assignment operator of whatever the reference
// is bound to - because that's not what TFunctionRef does,
// or is it even capable of doing that.
#if !FUN_ENABLE_TFUNCTIONREF_VISUALIZATION
  TFunctionRef(const TFunctionRef&) = default;
#endif
  TFunctionRef& operator=(const TFunctionRef&) const = delete;
  ~TFunctionRef() = default;

 private:
  /**
   * Sets the state of the TFunctionRef given a pointer to a callable thing.
   */
  template <typename FunctorType>
  void Set(FunctorType* functor) {
    // We force a void* cast here because if FunctorType is an actual function
    // then this won't compile.  We convert it back again before we use it
    // anyway.

    ptr_ = (void*)functor;
    Super::Set(functor);
  }

  /**
   * 'Nulls' the TFunctionRef.
   */
  void Unset() {
    ptr_ = nullptr;
    Super::Unset();
  }

  /**
   * Copies another TFunctionRef and rebinds it to another object of
   * the same type which was originally bound.
   *
   * Only intended to be used by TFunction's copy constructor/assignment
   * operator.
   */
  void CopyAndReseat(const TFunctionRef& other, void* functor) {
    ptr_ = functor;
    Super::CopyAndReseat(other, functor);
  }

  /**
   * Returns a pointer to the callable object - needed by TFunctionRefBase.
   */
  void* GetPtr() const { return ptr_; }

  // A pointer to the callable object
  void* ptr_;
};

/**
 * TFunction<FunctionType>
 *
 * A class which represents a copy of something callable.
 * FunctionType represents a function type and so TFunction
 * should be defined as follows:
 *
 * // A function taking a string and float and returning int32.  Parameter names
 * are optional. TFunction<int32 (const String& name, float scale)>
 *
 * Unlike TFunctionRef, this object is intended to be used like a FUN version of
 * std::function.  That is, it takes a copy of whatever is bound to it, meaning
 * you can return it from functions and store them in objects without caring
 * about the lifetime of the original object being bound.
 *
 * Example:
 *
 * // Something.h
 * TFunction<String (int32)> GetTransform();
 *
 * // Something.cc
 * TFunction<String (int32)> GetTransform(const String& prefix) {
 *   // Squares number and returns it as a string with the specified prefix
 *   return [=](int32 num) {
 *     return prefix + TEXT(": ") + TypeToString<int32>::ToString(num * num);
 *   };
 * }
 *
 * // SomewhereElse.cc
 * #include "Something.h"
 *
 * void func() {
 *   TFunction<String (int32)> transform = GetTransform(TEXT("Hello"));
 *
 *   String result = transform(5); // "Hello: 25"
 * }
 */
template <typename FunctionType>
class TFunction
    : public Function_internal::TFunctionRefBase<TFunction<FunctionType>,
                                                 FunctionType> {
  friend struct Function_internal::TFunctionRefBase<TFunction<FunctionType>,
                                                    FunctionType>;

  typedef Function_internal::TFunctionRefBase<TFunction<FunctionType>,
                                              FunctionType>
      Super;

 public:
  /**
   * Default constructor.
   */
  TFunction(decltype(nullptr) = nullptr) : Super(NoInit) { Super::Unset(); }

  /**
   * Constructor which binds a TFunction to any function object.
   */
  template <typename FunctorType,
            typename = typename EnableIf<!IsSame<
                TFunction, typename Decay<FunctorType>::Type>::Value>::Type>
  TFunction(FunctorType&& functor) : Super(NoInit) {
    // This constructor is disabled for TFunction types because VC is
    // incorrectly treating it as copy/move constructors.

    typedef typename Decay<FunctorType>::Type DecayedFunctorType;
    typedef Function_internal::TFunction_OwnedObject<DecayedFunctorType>
        OwnedType;

    // This is probably a mistake if you expect TFunction to take a copy of what
    // TFunctionRef is bound to, because that's not possible.
    //
    // If you really intended to bind a TFunction to a TFunctionRef, you can
    // just wrap it in a lambda (and thus it's clear you're just binding to a
    // call to another reference):
    //
    // TFunction<int32(float)> MyFunction = [=](float F) -> int32 { return
    // MyFunctionRef(F); };
    static_assert(!IsATFunctionRef<DecayedFunctorType>::Value,
                  "Cannot construct a TFunction from a TFunctionRef");

    OwnedType* new_obj =
        new (storage_) OwnedType(Forward<FunctorType>(functor));
    Super::Set(&new_obj->obj);
  }

  /**
   * Copy constructor.
   */
  TFunction(const TFunction& other) : Super(NoInit) {
    if (auto* other_func = other.storage_.GetBoundObject()) {
      auto* this_func = other_func->CopyToEmptyStorage(storage_);
      Super::CopyAndReseat(other, this_func->GetAddress());
    } else {
      Super::Unset();
    }
  }

  /**
   * Move constructor.
   */
  TFunction(TFunction&& other)
      : Super(NoInit), storage_(MoveTemp(other.storage_)) {
    if (auto* func = storage_.GetBoundObject()) {
      Super::CopyAndReseat(other, func->GetAddress());
    } else {
      Super::Unset();
    }

    other.Unset();
  }

  /**
   * Copy/move assignment operator.
   */
  TFunction& operator=(TFunction other) {
    UnsafeMemory::Memswap(&other, this, sizeof(TFunction));
    return *this;
  }

  /**
   * Nullptr assignment operator.
   */
  TFunction& operator=(decltype(nullptr)) {
    if (auto* obj = storage_.GetBoundObject()) {
      obj->~IFunction_OwnedObject();
    }
    storage_.Clear();
    Super::Unset();

    return *this;
  }

  /**
   * Destructor.
   */
  ~TFunction() {
    if (auto* obj = storage_.GetBoundObject()) {
      obj->~IFunction_OwnedObject();
    }
  }

  /**
   * TFunctions are not usefully comparable.
   */
  FUN_ALWAYS_INLINE explicit operator bool() const {
    return storage_.GetBoundObject() != nullptr;
  }

  FUN_ALWAYS_INLINE bool operator!() const {
    return storage_.GetBoundObject() == nullptr;
  }

 private:
  /**
   * Returns a pointer to the callable object - needed by TFunctionRefBase.
   */
  void* GetPtr() const {
    auto* ptr = storage_.GetBoundObject();
    return ptr ? ptr->GetAddress() : nullptr;
  }

  Function_internal::FunctionStorage storage_;
};

template <typename FunctionType>
FUN_ALWAYS_INLINE bool operator==(decltype(nullptr),
                                  const TFunction<FunctionType>& func) {
  return !func;
}

template <typename FunctionType>
FUN_ALWAYS_INLINE bool operator==(const TFunction<FunctionType>& func,
                                  decltype(nullptr)) {
  return !func;
}

template <typename FunctionType>
FUN_ALWAYS_INLINE bool operator!=(decltype(nullptr),
                                  const TFunction<FunctionType>& func) {
  return (bool)func;
}

template <typename FunctionType>
FUN_ALWAYS_INLINE bool operator!=(const TFunction<FunctionType>& func,
                                  decltype(nullptr)) {
  return (bool)func;
}

}  // namespace fun
