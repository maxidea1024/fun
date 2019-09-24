#pragma once

#include "fun/base/base.h"
#include "fun/base/delegate/multicast_delegate_base.h"
#include "fun/base/ftl/integer_sequence.h"
#include "fun/base/ftl/shared_ptr.h"
#include "fun/base/string/string.h"

// TODO
//#include "fun/object/weak_object_ptr_templates.h"

/**
 *  C++ DELEGATES
 *  -----------------------------------------------------------------------------------------------
 *
 *  This system allows you to call member functions on C++ objects in a generic,
 * yet type-safe way. Using delegates, you can dynamically bind to a member
 * function of an arbitrary object, then call functions on the object, even if
 * the caller doesn't know the object's type.
 *
 *  The system predefines various combinations of generic function signatures
 * with which you can declare a delegate type from, filling in the type names
 * for return value and parameters with whichever types you need.
 *
 *  Both single-cast and multi-cast delegates are supported, as well as
 * "dynamic" delegates which can be safely serialized to disk.  Additionally,
 * delegates may define "payload" data which will stored and passed directly to
 * bound functions.
 *
 *
 *
 *  DELEGATE FEATURES
 *  -----------------------------------------------------------------------------------------------
 *
 *  Currently we support delegate signatures using any combination of the
 * following:
 *      - Functions returning a value
 *      - Up to four "payload" variables
 *      - Up to eight function parameters
 *      - Functions declared as 'const'
 *
 *  Multi-cast delegates are also supported, using the
 * 'FUN_DECLARE_MULTICAST_DELEGATE...' macros. Multi-cast delegates allow you to
 * attach multiple function delegates, then execute them all at once by calling
 * a single "Broadcast()" function.  Multi-cast delegate signatures are not
 * allowed to use a return value.
 *
 *  You can assign "payload data" to your delegates!  These are arbitrary
 * variables that will be passed directly to any bound function when it is
 * invoked.  This is really useful as it allows you to store parameters within
 * the delegate it self at bind-time.  All delegate types (except for "dynamic")
 * supports payload variables automatically!
 *
 *  When binding to a delegate, you can pass payload data along.  This example
 * passes two custom variables, a bool and an int32 to a delegate.  Then when
 * the delegate is invoked, these parameters will be passed to your bound
 * function.  The extra variable arguments must always be accepted after the
 * delegate type parameter arguments.
 *
 *      MyDelegate.BindStatic(&MyFunction, true, 20);
 *
 *  Remember to look at the table at the bottom of this documentation comment
 * for the macro names to use for each function signature type.
 *
 *
 *
 *  DELEGATES EXAMPLE
 *  -----------------------------------------------------------------------------------------------
 *
 *  Suppose you have a class with a method that you'd like to be able to call
 * from anywhere:
 *
 *    class LogWriter {
 *      void WriteToLog(String);
 *    };
 *
 *  To call the WriteToLog function, we'll need to create a delegate type for
 * that function's signature. To do this, you will first declare the delegate
 * using one of the macros below.  For example, here is a simple delegate type:
 *
 *    FUN_DECLARE_DELEGATE_OneParam(StringDelegate, String);
 *
 *  This creates a delegate type called 'StringDelegate' that takes a single
 * parameter of type 'String'.
 *
 *  Here's an example of how you'd use this 'StringDelegate' in a class:
 *
 *    class MyClass {
 *      StringDelegate write_to_log_delegate;
 *    };
 *
 *  This allows your class to hold a pointer to a method in an arbitrary class.
 * The only thing the class really knows about this delegate is it's function
 * signature.
 *
 *  Now, to assign the delegate, simply create an instance of your delegate
 * class, passing along the class that owns the method as a template parameter.
 * You'll also pass the instance of your object and the actual function address
 * of the method.  So, here we'll create an instance of our 'LogWriter' class,
 * then create a delegate for the 'WriteToLog' method of that object instance:
 *
 *    SharedRef<LogWriter> log_writer(new LogWriter());
 *
 *    write_to_log_delegate.BindSP(log_writer, &LogWriter::WriteToLog);
 *
 *  You've just dynamically bound a delegate to a method of a class!  Pretty
 * simple, right?
 *
 *  Note that the 'SP' part of 'BindSP' stands for 'shared pointer', because
 * we're binding to an object that's owned by a shared pointer.  There are
 * versions for different object types, such as BindRaw() and BindFObject(). You
 * can bind to global function pointers with BindStatic().
 *
 *  Now, your 'WriteToLog' method can be called by MyClass without it even
 * knowing anything about the 'LogWriter' class!  To call a your delegate, just
 * use the 'Execute()' method:
 *
 *    write_to_log_delegate.Execute("Delegates are spiffy!");
 *
 *  If you call Execute() before binding a function to the delegate, an
 * assertion will be triggered.  In many cases, you'll instead want to do this:
 *
 *    write_to_log_delegate.ExecuteIfBound("Only executes if a function was
 * bound!");
 *
 *  That's pretty much all there is to it!!  You can read below for a bit more
 * information.
 *
 *
 *  MORE INFORMATION
 *  -----------------------------------------------------------------------------------------------
 *
 *  The delegate system understands certain types of objects, and additional
 * features are enabled when using these objects.  If you bind a delegate to a
 * member of a FObject or shared pointer class, the delegate system can keep a
 * weak reference to the object, so that if the object gets destroyed out from
 * underneath the delegate, you'll be able to handle these cases by calling
 * IsBound() or ExecuteIfBound() functions.  Note the special binding syntax for
 * the various types of supported objects.
 *
 *  It's perfectly safe to copy delegate objects.  Delegates can be passed
 * around by value but this is generally not recommended since they do have to
 * allocate memory on the heap.  Pass them by reference when possible!
 *
 *  Delegate signature declarations can exist at global scope, within a
 * namespace or even within a class declaration (but not function bodies.)
 *
 *
 *  FUNCTION SIGNATURES
 *  -----------------------------------------------------------------------------------------------
 *
 *  Use this table to find the declaration macro to use to declare your
 * delegate.
 *
 *  Function signature                            | Declaration macro
 *  ------------------------------------------------------------------------------------------------------------------------------------------------------------
 *  void Function()                               |
 * FUN_DECLARE_DELEGATE(DelegateName) void Function(<Param1>) |
 * FUN_DECLARE_DELEGATE_OneParam(DelegateName, Param1Type) void
 * Function(<Param1>, <Param2>)             |
 * FUN_DECLARE_DELEGATE_TwoParams(DelegateName, Param1Type, Param2Type) void
 * Function(<Param1>, <Param2>, ...)        |
 * FUN_DECLARE_DELEGATE_<Num>Params(DelegateName, Param1Type, Param2Type, ...)
 *  <RetVal> Function()                           |
 * FUN_DECLARE_DELEGATE_RetVal(RetType, DelegateName) <RetVal>
 * Function(<Param1>)                   |
 * FUN_DECLARE_DELEGATE_RetVal_OneParam(RetType, DelegateName, Param1Type)
 *  <RetVal> Function(<Param1>, <Param2>)         |
 * FUN_DECLARE_DELEGATE_RetVal_TwoParams(RetType, DelegateName, Param1Type,
 * Param2Type) <RetVal> Function(<Param1>, <Param2>, ...)    |
 * FUN_DECLARE_DELEGATE_RetVal_<Num>Params(RetType, DelegateName, Param1Type,
 * Param2Type, ...)
 *  ------------------------------------------------------------------------------------------------------------------------------------------------------------
 *
 *  Remember, there are three different delegate types you can define (any of
 * the above signatures will work):
 *
 *                       Single-cast delegates:  FUN_DECLARE_DELEGATE...()
 *                        Multi-cast delegates:
 * FUN_DECLARE_MULTICAST_DELEGATE...() Dynamic (FObject, serializable)
 * delegates:  FUN_DECLARE_DYNAMIC_DELEGATE...()
 */

// This suffix is appended to all header exported delegates
#define FUN_HEADER_GENERATED_DELEGATE_SIGNATURE_SUFFIX "__DelegateSignature"

/** Helper macro that enables passing comma-separated arguments as a single
 * macro parameter */
#define FUN_FUNC_CONCAT(...) __VA_ARGS__

/** Declare the user's delegate object */
// NOTE: The last parameter is variadic and is used as the 'template args' for
// this delegate's classes (__VA_ARGS__)
#define FUN_FUNC_FUN_DECLARE_DELEGATE(DelegateName, ...) \
  typedef fun::BaseDelegate<__VA_ARGS__> DelegateName;

/** Declare the user's multicast delegate object */
// NOTE: The last parameter is variadic and is used as the 'template args' for
// this delegate's classes (__VA_ARGS__)
#define FUNC_FUN_DECLARE_MULTICAST_DELEGATE(MulticastDelegateName, ...) \
  typedef fun::MulticastDelegate<__VA_ARGS__> MulticastDelegateName;

#define FUNC_FUN_DECLARE_EVENT(OwningType, EventName, ...)            \
  class EventName : public fun::TBaseMulticastDelegate<__VA_ARGS__> { \
    friend class OwningType;                                          \
  };

///** Declare user's dynamic delegate, with wrapper proxy method for executing
///the delegate */
//#define FUN_FUN_FUNC_FUN_DECLARE_DYNAMIC_DELEGATE(WeakPtr,
//DynamicDelegateName, ExecFunction, FuncParamList, FuncParamPassThru, ...) \
//  class DynamicDelegateName : public fun::BaseDynamicDelegate<WeakPtr,
//  __VA_ARGS__> { \
//   public: \
//    /** Default constructor */ \
//    DynamicDelegateName() \
//    {} \
//    \
//    /** Construction from an ScriptDelegate must be explicit.  This is really
//    only used by FObject system internals. */ \
//    explicit DynamicDelegateName(const ScriptDelegate<>& script_delegate) \
//      : fun::BaseDynamicDelegate<WeakPtr, __VA_ARGS__>(script_delegate) \
//    {} \
//    \
//    /** Execute the delegate.  If the function pointer is not valid, an error
//    will occur. */ \
//    inline void Execute(FuncParamList) const { \
//      /* Verify that the user object is still valid.  We only have a weak
//      reference to it. */ \
//      fun_check_dbg(IsBound()); \
//      ExecFunction(FuncParamPassThru); \
//    } \
//    /** Execute the delegate, but only if the function pointer is still valid
//    */ \
//    inline bool ExecuteIfBound(FuncParamList) const { \
//      if(IsBound()) { \
//        ExecFunction(FuncParamPassThru); \
//        return true; \
//      } \
//      return false; \
//    } \
//  };

//#define FUN_FUN_FUNC_FUN_DECLARE_DYNAMIC_DELEGATE_RETVAL(WeakPtr, DynamicDelegateName, ExecFunction, RetType, FuncParamList, FuncParamPassThru, ...) \
//  class DynamicDelegateName : public fun::BaseDynamicDelegate<WeakPtr, __VA_ARGS__> { \
//   public: \
//    /** Default constructor */ \
//    DynamicDelegateName() \
//    {} \
//    \
//    /** Construction from an ScriptDelegate must be explicit.  This is really only used by FObject system internals. */ \
//    explicit DynamicDelegateName(const fun::ScriptDelegate<>& script_delegate) \
//      : fun::BaseDynamicDelegate<WeakPtr, __VA_ARGS__>(script_delegate) \
//    {} \
//    \
//    /** Execute the delegate.  If the function pointer is not valid, an error will occur. */ \
//    inline RetType Execute(FuncParamList) const \
//    { \
//      /* Verify that the user object is still valid.  We only have a weak reference to it. */ \
//      fun_check_dbg(IsBound()); \
//      return ExecFunction(FuncParamPassThru); \
//    } \
//  };

///** Declare user's dynamic multi-cast delegate, with wrapper proxy method for
///executing the delegate */
//#define FUN_FUNC_FUN_DECLARE_DYNAMIC_MULTICAST_DELEGATE(WeakPtr,
//DynamicMulticastDelegateName, ExecFunction, FuncParamList, FuncParamPassThru,
//...) \
//  class DynamicMulticastDelegateName : public
//  fun::TBaseDynamicMulticastDelegate<WeakPtr, __VA_ARGS__> { \
//   public: \
//    /** Default constructor */ \
//    DynamicMulticastDelegateName() \
//    {} \
//    \
//    /** Construction from an FMulticastScriptDelegate must be explicit.  This
//    is really only used by FObject system internals. */ \
//    explicit DynamicMulticastDelegateName(const
//    fun::TMulticastScriptDelegate<>& multicast_script_delegate) \
//      : fun::TBaseDynamicMulticastDelegate<WeakPtr,
//      __VA_ARGS__>(multicast_script_delegate) \
//    {} \
//    \
//    /** Broadcasts this delegate to all bound objects, except to those that
//    may have expired */ \
//    void Broadcast(FuncParamList) const { \
//      ExecFunction(FuncParamPassThru); \
//    } \
//  };

// Simple macro chain to concatenate code text
#define FUN_FUNC_COMBINE_ACTUAL(A, B) A##B
#define FUN_FUNC_COMBINE(A, B) FUN_FUNC_COMBINE_ACTUAL(A, B)

#define ENABLE_STATIC_FUNCTION_FNAMES \
  (PLATFORM_COMPILER_CLANG &&         \
   (__clang_major__ > 3 || (__clang_major__ == 3 && __clang_minor__ >= 5)))

#if ENABLE_STATIC_FUNCTION_FNAMES

namespace fun {
namespace NStrAfterLastDoubleColon_internal {
template <typename T>
struct TypedImpl {
  // Dummy bool parameter needed so that we can ensure the last specialization
  // is partial
  template <bool Dummy, T... Chars>
  struct Inner;

  template <typename SecondMatch, T... Chars>
  struct Inner2 {
    typedef typename TypedImpl<T>::template Inner<true, Chars...>::Type Type;
  };

  template <T... Chars>
  struct Inner2<IntegerSequence<T>, Chars...> {
    typedef IntegerSequence<T, Chars...> Type;
  };

  template <bool Dummy, T Char, T... Chars>
  struct Inner<Dummy, Char, Chars...> {
    typedef typename Inner<Dummy, Chars...>::Type Type;
  };

  template <bool Dummy, T... Chars>
  struct Inner<Dummy, ':', ':', Chars...> {
    typedef
        typename Inner2<typename Inner<Dummy, Chars...>::Type, Chars...>::Type
            Type;
  };

  // Without the dummy bool parameter, this would not compile, as full
  // specializations of nested class templates is not allowed.
  template <bool Dummy>
  struct Inner<Dummy> {
    typedef IntegerSequence<T> Type;
  };
};

template <typename IntSeq>
struct StaticStringFromCharSequence;

template <typename T, T... Chars>
struct StaticStringFromCharSequence<IntegerSequence<T, Chars...>> {
  static String Get() {
    static String result = Create();
    return result;
  }

 private:
  static String Create() {
    const T str[sizeof...(Chars) + 1] = {Chars..., 0};
    return str;
  }
};

/**
 * Metafunction which evaluates to a IntegerSequence of chars containing only
 * the function name.
 *
 * Example:
 * StrAfterLastDoubleColon<"&SomeClass::SomeNestedClass::SomeFunc"_intseq>::Type
 * == IntegerSequence<char, 'S', 'o', 'm', 'e', 'F', 'u', 'n', 'c'>
 */
template <typename T>
struct StrAfterLastDoubleColon;

template <typename T, T... Chars>
struct StrAfterLastDoubleColon<IntegerSequence<T, Chars...>> {
  typedef typename NStrAfterLastDoubleColon_internal::TypedImpl<
      T>::template Inner<true, Chars...>::Type Type;
};
}  // namespace NStrAfterLastDoubleColon_internal

/**
 * Custom literal operator which converts a string into a IntegerSequence of
 * chars.
 */
template <typename T, T... Chars>
FUN_ALWAYS_INLINE constexpr IntegerSequence<T, Chars...> operator""_intseq() {
  return {};
}
}  // namespace fun

#define FUN_STATIC_FUNCTION_NAME(str)                                   \
  fun::NStrAfterLastDoubleColon_internal::StaticStringFromCharSequence< \
      typename fun::NStrAfterLastDoubleColon_internal::                 \
          StrAfterLastDoubleColon<decltype(                             \
              PREPROCESSOR_JOIN(str, _intseq))>::Type>::Get()

#else

#define FUN_STATIC_FUNCTION_NAME(str) \
  fun::Delegate_internal::GetTrimmedMemberFunctionName(str)

#endif

//// Helper macro for calling BindDynamic() on dynamic delegates.  Automatically
///generates the function name string.
//#define BindDynamic(UserObject, FuncName)  __Internal_BindDynamic(UserObject,
//FuncName, FUN_STATIC_FUNCTION_NAME(#FuncName))
//
//// Helper macro for calling AddDynamic() on dynamic multi-cast delegates.
///Automatically generates the function name string.
//#define AddDynamic(UserObject, FuncName)  __Internal_AddDynamic(UserObject,
//FuncName, FUN_STATIC_FUNCTION_NAME(#FuncName))
//
//// Helper macro for calling AddUniqueDynamic() on dynamic multi-cast
///delegates.  Automatically generates the function name string.
//#define AddUniqueDynamic(UserObject, FuncName)
//__Internal_AddUniqueDynamic(UserObject, FuncName,
//FUN_STATIC_FUNCTION_NAME(#FuncName))
//
//// Helper macro for calling RemoveDynamic() on dynamic multi-cast delegates.
///Automatically generates the function name string.
//#define RemoveDynamic(UserObject, FuncName)
//__Internal_RemoveDynamic(UserObject, FuncName,
//FUN_STATIC_FUNCTION_NAME(#FuncName))
//
//// Helper macro for calling IsAlreadyBound() on dynamic multi-cast delegates.
///Automatically generates the function name string.
//#define IsAlreadyBound(UserObject, FuncName)
//__Internal_IsAlreadyBound(UserObject, FuncName,
//FUN_STATIC_FUNCTION_NAME(#FuncName))

namespace fun {
namespace Delegate_internal {
/**
 * Returns the root function name from a string representing a member function
 * pointer. Note: this function only returns a pointer to the substring and
 * doesn't create a new string.
 *
 * @param  function_name  The string containing the member function name.
 * @return An String of the member function name.
 */
inline String GetTrimmedMemberFunctionName(const char* function_name) {
  // We strip off the class prefix and just return the function name by itself.
  fun_check(function_name);
  const char* result = CStringTraitsA::Strrstr(function_name, "::");
  fun_check_msg(result && result[2] != '0',
                "'{0}' does not look like a member function", function_name);
  return String(result + 2);
}
}  // namespace Delegate_internal
}  // namespace fun

//-----------------------------------------------------------------------------
// We define this as a guard to prevent delegate_signature_impl.inl
// being included outside of this file
//-----------------------------------------------------------------------------
#define __FUN_BASE_DELEGATE_DELEGATE_H__
#define FUNC_INCLUDING_INLINE_IMPL

#if !FUN_BUILD_DOCS
#include "fun/base/delegate/delegate_combinations.h"
#include "fun/base/delegate/delegate_instance_interface.h"
#include "fun/base/delegate/delegate_instances_impl.h"
#include "fun/base/delegate/delegate_signature_impl.inl.h"
#endif  //! FUN_BUILD_DOCS

// No longer allowed to include delegate_signature_impl.inl
#undef FUNC_INCLUDING_INLINE_IMPL
#undef __FUN_BASE_DELEGATE_DELEGATE_H__

//-----------------------------------------------------------------------------

#define FUN_DECLARE_DERIVED_EVENT(OwningType, BaseTypeEvent, EventName) \
  class EventName : public BaseTypeEvent {                              \
    friend class OwningType;                                            \
  };

// Undefine temporary macros
#undef FUN_FUNC_COMBINE_ACTUAL
#undef FUN_FUNC_COMBINE

// Simple delegate used by various utilities such as timers
namespace fun {
FUN_DECLARE_DELEGATE(SimpleDelegate);
FUN_DECLARE_MULTICAST_DELEGATE(SimpleMulticastDelegate);
}  // namespace fun
