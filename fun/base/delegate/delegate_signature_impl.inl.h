// Only designed to be included directly by Delegate.h
#if !defined(__FUN_BASE_DELEGATE_DELEGATE_H__) || !defined(FUNC_INCLUDING_INLINE_IMPL)
  #error "This inline header must only be included by Delegate.h"
#endif

#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/template.h"
#include "fun/base/ftl/type_traits.h"
#include "fun/base/crc.h"
#include "fun/base/ftl/shared_ptr.h"
//TODO
//#include "fun/object/weak_object_ptr_templates.h"
//#include "fun/object/script_delegate.h"

namespace fun {

struct WeakObjectPtr;

class DelegateBase;
class DelegateHandle;
class IDelegateInstance;

template <typename FuncType>
struct IBaseDelegateInstance;

template <typename ObjectPtrType>
class MulticastDelegateBase;

/**
 * Unicast delegate base object.
 *
 * Use the various FUN_DECLARE_DELEGATE macros to create the actual delegate type, templated to
 * the function signature the delegate is compatible with. Then, you can create an instance
 * of that class when you want to bind a function to the delegate.
 */
template <typename WrappedRetType, typename... Params>
class BaseDelegate : public DelegateBase
{
 public:
  /** Type definition for return value type. */
  typedef typename UnwrapType<WrappedRetType>::Type RetType;
  typedef RetType FuncType(Params...);

  /** Type definition for the shared interface of delegate instance types compatible with this delegate class. */
  typedef IBaseDelegateInstance<FuncType> DelegateInstanceInterface;

  /** Declare the user's "fast" shared pointer-based delegate instance types. */
  template <typename Class                                                                            > struct SPMethodDelegate                 : BaseSPMethodDelegateInstance<false, Class, SPMode::Fast, FuncType                                        > { typedef BaseSPMethodDelegateInstance<false, Class, SPMode::Fast, FuncType                                        > Super; SPMethodDelegate                (const SharedRef< Class, SPMode::Fast >& object, typename Super::MethodPtr method     ) : Super(object, method) {} };
  template <typename Class                                                                            > struct SPMethodDelegate_Const           : BaseSPMethodDelegateInstance<true , Class, SPMode::Fast, FuncType                                        > { typedef BaseSPMethodDelegateInstance<true , Class, SPMode::Fast, FuncType                                        > Super; SPMethodDelegate_Const          (const SharedRef< Class, SPMode::Fast >& object, typename Super::MethodPtr method     ) : Super(object, method) {} };
  template <typename Class, typename Var1Type                                                         > struct SPMethodDelegate_OneVar          : BaseSPMethodDelegateInstance<false, Class, SPMode::Fast, FuncType, Var1Type                              > { typedef BaseSPMethodDelegateInstance<false, Class, SPMode::Fast, FuncType, Var1Type                              > Super; SPMethodDelegate_OneVar         (const SharedRef< Class, SPMode::Fast >& object, typename Super::MethodPtr method, Var1Type Var1) : Super(object, method, Var1) {} };
  template <typename Class, typename Var1Type                                                         > struct SPMethodDelegate_OneVar_Const    : BaseSPMethodDelegateInstance<true , Class, SPMode::Fast, FuncType, Var1Type                              > { typedef BaseSPMethodDelegateInstance<true , Class, SPMode::Fast, FuncType, Var1Type                              > Super; SPMethodDelegate_OneVar_Const   (const SharedRef< Class, SPMode::Fast >& object, typename Super::MethodPtr method, Var1Type Var1) : Super(object, method, Var1) {} };
  template <typename Class, typename Var1Type, typename Var2Type                                      > struct SPMethodDelegate_TwoVars         : BaseSPMethodDelegateInstance<false, Class, SPMode::Fast, FuncType, Var1Type, Var2Type                    > { typedef BaseSPMethodDelegateInstance<false, Class, SPMode::Fast, FuncType, Var1Type, Var2Type                    > Super; SPMethodDelegate_TwoVars        (const SharedRef< Class, SPMode::Fast >& object, typename Super::MethodPtr method, Var1Type Var1, Var2Type Var2) : Super(object, method, Var1, Var2) {} };
  template <typename Class, typename Var1Type, typename Var2Type                                      > struct SPMethodDelegate_TwoVars_Const   : BaseSPMethodDelegateInstance<true , Class, SPMode::Fast, FuncType, Var1Type, Var2Type                    > { typedef BaseSPMethodDelegateInstance<true , Class, SPMode::Fast, FuncType, Var1Type, Var2Type                    > Super; SPMethodDelegate_TwoVars_Const  (const SharedRef< Class, SPMode::Fast >& object, typename Super::MethodPtr method, Var1Type Var1, Var2Type Var2) : Super(object, method, Var1, Var2) {} };
  template <typename Class, typename Var1Type, typename Var2Type, typename Var3Type                   > struct SPMethodDelegate_ThreeVars       : BaseSPMethodDelegateInstance<false, Class, SPMode::Fast, FuncType, Var1Type, Var2Type, Var3Type          > { typedef BaseSPMethodDelegateInstance<false, Class, SPMode::Fast, FuncType, Var1Type, Var2Type, Var3Type          > Super; SPMethodDelegate_ThreeVars      (const SharedRef< Class, SPMode::Fast >& object, typename Super::MethodPtr method, Var1Type Var1, Var2Type Var2, Var3Type Var3) : Super(object, method, Var1, Var2, Var3) {} };
  template <typename Class, typename Var1Type, typename Var2Type, typename Var3Type                   > struct SPMethodDelegate_ThreeVars_Const : BaseSPMethodDelegateInstance<true , Class, SPMode::Fast, FuncType, Var1Type, Var2Type, Var3Type          > { typedef BaseSPMethodDelegateInstance<true , Class, SPMode::Fast, FuncType, Var1Type, Var2Type, Var3Type          > Super; SPMethodDelegate_ThreeVars_Const(const SharedRef< Class, SPMode::Fast >& object, typename Super::MethodPtr method, Var1Type Var1, Var2Type Var2, Var3Type Var3) : Super(object, method, Var1, Var2, Var3) {} };
  template <typename Class, typename Var1Type, typename Var2Type, typename Var3Type, typename Var4Type> struct SPMethodDelegate_FourVars        : BaseSPMethodDelegateInstance<false, Class, SPMode::Fast, FuncType, Var1Type, Var2Type, Var3Type, Var4Type> { typedef BaseSPMethodDelegateInstance<false, Class, SPMode::Fast, FuncType, Var1Type, Var2Type, Var3Type, Var4Type> Super; SPMethodDelegate_FourVars       (const SharedRef< Class, SPMode::Fast >& object, typename Super::MethodPtr method, Var1Type Var1, Var2Type Var2, Var3Type Var3, Var4Type Var4) : Super(object, method, Var1, Var2, Var3, Var4) {} };
  template <typename Class, typename Var1Type, typename Var2Type, typename Var3Type, typename Var4Type> struct SPMethodDelegate_FourVars_Const  : BaseSPMethodDelegateInstance<true , Class, SPMode::Fast, FuncType, Var1Type, Var2Type, Var3Type, Var4Type> { typedef BaseSPMethodDelegateInstance<true , Class, SPMode::Fast, FuncType, Var1Type, Var2Type, Var3Type, Var4Type> Super; SPMethodDelegate_FourVars_Const (const SharedRef< Class, SPMode::Fast >& object, typename Super::MethodPtr method, Var1Type Var1, Var2Type Var2, Var3Type Var3, Var4Type Var4) : Super(object, method, Var1, Var2, Var3, Var4) {} };

  /** Declare the user's "thread-safe" shared pointer-based delegate instance types. */
  template <typename Class                                                                            > struct ThreadSafeSPMethodDelegate                 : BaseSPMethodDelegateInstance<false, Class, SPMode::ThreadSafe, FuncType                                        > { typedef BaseSPMethodDelegateInstance<false, Class, SPMode::ThreadSafe, FuncType                                        > Super; ThreadSafeSPMethodDelegate                (const SharedRef<Class, SPMode::ThreadSafe>& object, typename Super::MethodPtr method     ) : Super(object, method) {} };
  template <typename Class                                                                            > struct ThreadSafeSPMethodDelegate_Const           : BaseSPMethodDelegateInstance<true , Class, SPMode::ThreadSafe, FuncType                                        > { typedef BaseSPMethodDelegateInstance<true , Class, SPMode::ThreadSafe, FuncType                                        > Super; ThreadSafeSPMethodDelegate_Const          (const SharedRef<Class, SPMode::ThreadSafe>& object, typename Super::MethodPtr method     ) : Super(object, method) {} };
  template <typename Class, typename Var1Type                                                         > struct ThreadSafeSPMethodDelegate_OneVar          : BaseSPMethodDelegateInstance<false, Class, SPMode::ThreadSafe, FuncType, Var1Type                              > { typedef BaseSPMethodDelegateInstance<false, Class, SPMode::ThreadSafe, FuncType, Var1Type                              > Super; ThreadSafeSPMethodDelegate_OneVar         (const SharedRef<Class, SPMode::ThreadSafe>& object, typename Super::MethodPtr method, Var1Type Var1) : Super(object, method, Var1) {} };
  template <typename Class, typename Var1Type                                                         > struct ThreadSafeSPMethodDelegate_OneVar_Const    : BaseSPMethodDelegateInstance<true , Class, SPMode::ThreadSafe, FuncType, Var1Type                              > { typedef BaseSPMethodDelegateInstance<true , Class, SPMode::ThreadSafe, FuncType, Var1Type                              > Super; ThreadSafeSPMethodDelegate_OneVar_Const   (const SharedRef<Class, SPMode::ThreadSafe>& object, typename Super::MethodPtr method, Var1Type Var1) : Super(object, method, Var1) {} };
  template <typename Class, typename Var1Type, typename Var2Type                                      > struct ThreadSafeSPMethodDelegate_TwoVars         : BaseSPMethodDelegateInstance<false, Class, SPMode::ThreadSafe, FuncType, Var1Type, Var2Type                    > { typedef BaseSPMethodDelegateInstance<false, Class, SPMode::ThreadSafe, FuncType, Var1Type, Var2Type                    > Super; ThreadSafeSPMethodDelegate_TwoVars        (const SharedRef<Class, SPMode::ThreadSafe>& object, typename Super::MethodPtr method, Var1Type Var1, Var2Type Var2) : Super(object, method, Var1, Var2) {} };
  template <typename Class, typename Var1Type, typename Var2Type                                      > struct ThreadSafeSPMethodDelegate_TwoVars_Const   : BaseSPMethodDelegateInstance<true , Class, SPMode::ThreadSafe, FuncType, Var1Type, Var2Type                    > { typedef BaseSPMethodDelegateInstance<true , Class, SPMode::ThreadSafe, FuncType, Var1Type, Var2Type                    > Super; ThreadSafeSPMethodDelegate_TwoVars_Const  (const SharedRef<Class, SPMode::ThreadSafe>& object, typename Super::MethodPtr method, Var1Type Var1, Var2Type Var2) : Super(object, method, Var1, Var2) {} };
  template <typename Class, typename Var1Type, typename Var2Type, typename Var3Type                   > struct ThreadSafeSPMethodDelegate_ThreeVars       : BaseSPMethodDelegateInstance<false, Class, SPMode::ThreadSafe, FuncType, Var1Type, Var2Type, Var3Type          > { typedef BaseSPMethodDelegateInstance<false, Class, SPMode::ThreadSafe, FuncType, Var1Type, Var2Type, Var3Type          > Super; ThreadSafeSPMethodDelegate_ThreeVars      (const SharedRef<Class, SPMode::ThreadSafe>& object, typename Super::MethodPtr method, Var1Type Var1, Var2Type Var2, Var3Type Var3) : Super(object, method, Var1, Var2, Var3) {} };
  template <typename Class, typename Var1Type, typename Var2Type, typename Var3Type                   > struct ThreadSafeSPMethodDelegate_ThreeVars_Const : BaseSPMethodDelegateInstance<true , Class, SPMode::ThreadSafe, FuncType, Var1Type, Var2Type, Var3Type          > { typedef BaseSPMethodDelegateInstance<true , Class, SPMode::ThreadSafe, FuncType, Var1Type, Var2Type, Var3Type          > Super; ThreadSafeSPMethodDelegate_ThreeVars_Const(const SharedRef<Class, SPMode::ThreadSafe>& object, typename Super::MethodPtr method, Var1Type Var1, Var2Type Var2, Var3Type Var3) : Super(object, method, Var1, Var2, Var3) {} };
  template <typename Class, typename Var1Type, typename Var2Type, typename Var3Type, typename Var4Type> struct ThreadSafeSPMethodDelegate_FourVars        : BaseSPMethodDelegateInstance<false, Class, SPMode::ThreadSafe, FuncType, Var1Type, Var2Type, Var3Type, Var4Type> { typedef BaseSPMethodDelegateInstance<false, Class, SPMode::ThreadSafe, FuncType, Var1Type, Var2Type, Var3Type, Var4Type> Super; ThreadSafeSPMethodDelegate_FourVars       (const SharedRef<Class, SPMode::ThreadSafe>& object, typename Super::MethodPtr method, Var1Type Var1, Var2Type Var2, Var3Type Var3, Var4Type Var4) : Super(object, method, Var1, Var2, Var3, Var4) {} };
  template <typename Class, typename Var1Type, typename Var2Type, typename Var3Type, typename Var4Type> struct ThreadSafeSPMethodDelegate_FourVars_Const  : BaseSPMethodDelegateInstance<true , Class, SPMode::ThreadSafe, FuncType, Var1Type, Var2Type, Var3Type, Var4Type> { typedef BaseSPMethodDelegateInstance<true , Class, SPMode::ThreadSafe, FuncType, Var1Type, Var2Type, Var3Type, Var4Type> Super; ThreadSafeSPMethodDelegate_FourVars_Const (const SharedRef<Class, SPMode::ThreadSafe>& object, typename Super::MethodPtr method, Var1Type Var1, Var2Type Var2, Var3Type Var3, Var4Type Var4) : Super(object, method, Var1, Var2, Var3, Var4) {} };

  /** Declare the user's C++ pointer-based delegate instance types. */
  template <typename Class                                                                            > struct RawMethodDelegate                 : BaseRawMethodDelegateInstance<false, Class, FuncType                                        > { typedef BaseRawMethodDelegateInstance<false, Class, FuncType                                        > Super; RawMethodDelegate                (Class* object, typename Super::MethodPtr method     ) : Super(object, method) {} };
  template <typename Class                                                                            > struct RawMethodDelegate_Const           : BaseRawMethodDelegateInstance<true , Class, FuncType                                        > { typedef BaseRawMethodDelegateInstance<true , Class, FuncType                                        > Super; RawMethodDelegate_Const          (Class* object, typename Super::MethodPtr method     ) : Super(object, method) {} };
  template <typename Class, typename Var1Type                                                         > struct RawMethodDelegate_OneVar          : BaseRawMethodDelegateInstance<false, Class, FuncType, Var1Type                              > { typedef BaseRawMethodDelegateInstance<false, Class, FuncType, Var1Type                              > Super; RawMethodDelegate_OneVar         (Class* object, typename Super::MethodPtr method, Var1Type Var1) : Super(object, method, Var1) {} };
  template <typename Class, typename Var1Type                                                         > struct RawMethodDelegate_OneVar_Const    : BaseRawMethodDelegateInstance<true , Class, FuncType, Var1Type                              > { typedef BaseRawMethodDelegateInstance<true , Class, FuncType, Var1Type                              > Super; RawMethodDelegate_OneVar_Const   (Class* object, typename Super::MethodPtr method, Var1Type Var1) : Super(object, method, Var1) {} };
  template <typename Class, typename Var1Type, typename Var2Type                                      > struct RawMethodDelegate_TwoVars         : BaseRawMethodDelegateInstance<false, Class, FuncType, Var1Type, Var2Type                    > { typedef BaseRawMethodDelegateInstance<false, Class, FuncType, Var1Type, Var2Type                    > Super; RawMethodDelegate_TwoVars        (Class* object, typename Super::MethodPtr method, Var1Type Var1, Var2Type Var2) : Super(object, method, Var1, Var2) {} };
  template <typename Class, typename Var1Type, typename Var2Type                                      > struct RawMethodDelegate_TwoVars_Const   : BaseRawMethodDelegateInstance<true , Class, FuncType, Var1Type, Var2Type                    > { typedef BaseRawMethodDelegateInstance<true , Class, FuncType, Var1Type, Var2Type                    > Super; RawMethodDelegate_TwoVars_Const  (Class* object, typename Super::MethodPtr method, Var1Type Var1, Var2Type Var2) : Super(object, method, Var1, Var2) {} };
  template <typename Class, typename Var1Type, typename Var2Type, typename Var3Type                   > struct RawMethodDelegate_ThreeVars       : BaseRawMethodDelegateInstance<false, Class, FuncType, Var1Type, Var2Type, Var3Type          > { typedef BaseRawMethodDelegateInstance<false, Class, FuncType, Var1Type, Var2Type, Var3Type          > Super; RawMethodDelegate_ThreeVars      (Class* object, typename Super::MethodPtr method, Var1Type Var1, Var2Type Var2, Var3Type Var3) : Super(object, method, Var1, Var2, Var3) {} };
  template <typename Class, typename Var1Type, typename Var2Type, typename Var3Type                   > struct RawMethodDelegate_ThreeVars_Const : BaseRawMethodDelegateInstance<true , Class, FuncType, Var1Type, Var2Type, Var3Type          > { typedef BaseRawMethodDelegateInstance<true , Class, FuncType, Var1Type, Var2Type, Var3Type          > Super; RawMethodDelegate_ThreeVars_Const(Class* object, typename Super::MethodPtr method, Var1Type Var1, Var2Type Var2, Var3Type Var3) : Super(object, method, Var1, Var2, Var3) {} };
  template <typename Class, typename Var1Type, typename Var2Type, typename Var3Type, typename Var4Type> struct RawMethodDelegate_FourVars        : BaseRawMethodDelegateInstance<false, Class, FuncType, Var1Type, Var2Type, Var3Type, Var4Type> { typedef BaseRawMethodDelegateInstance<false, Class, FuncType, Var1Type, Var2Type, Var3Type, Var4Type> Super; RawMethodDelegate_FourVars       (Class* object, typename Super::MethodPtr method, Var1Type Var1, Var2Type Var2, Var3Type Var3, Var4Type Var4) : Super(object, method, Var1, Var2, Var3, Var4) {} };
  template <typename Class, typename Var1Type, typename Var2Type, typename Var3Type, typename Var4Type> struct RawMethodDelegate_FourVars_Const  : BaseRawMethodDelegateInstance<true , Class, FuncType, Var1Type, Var2Type, Var3Type, Var4Type> { typedef BaseRawMethodDelegateInstance<true , Class, FuncType, Var1Type, Var2Type, Var3Type, Var4Type> Super; RawMethodDelegate_FourVars_Const (Class* object, typename Super::MethodPtr method, Var1Type Var1, Var2Type Var2, Var3Type Var3, Var4Type Var4) : Super(object, method, Var1, Var2, Var3, Var4) {} };

  ///** Declare the user's FFunction-based delegate instance types. */
  //template <typename FObjectTemplate                                                                            > struct FunctionDelegateBinding           : BaseFFunctionDelegateInstance<FObjectTemplate, FuncType                                        > { typedef TBaseFFunctionDelegateInstance<FObjectTemplate, FuncType                                        > Super; FunctionDelegateBinding          (FObjectTemplate* object, const String& function_name     ) : Super(object, function_name) {} };
  //template <typename FObjectTemplate, typename Var1Type                                                         > struct FunctionDelegateBinding_OneVar    : BaseFFunctionDelegateInstance<FObjectTemplate, FuncType, Var1Type                              > { typedef TBaseFFunctionDelegateInstance<FObjectTemplate, FuncType, Var1Type                              > Super; FunctionDelegateBinding_OneVar   (FObjectTemplate* object, const String& function_name, Var1Type Var1) : Super(object, function_name, Var1) {} };
  //template <typename FObjectTemplate, typename Var1Type, typename Var2Type                                      > struct FunctionDelegateBinding_TwoVars   : BaseFFunctionDelegateInstance<FObjectTemplate, FuncType, Var1Type, Var2Type                    > { typedef TBaseFFunctionDelegateInstance<FObjectTemplate, FuncType, Var1Type, Var2Type                    > Super; FunctionDelegateBinding_TwoVars  (FObjectTemplate* object, const String& function_name, Var1Type Var1, Var2Type Var2) : Super(object, function_name, Var1, Var2) {} };
  //template <typename FObjectTemplate, typename Var1Type, typename Var2Type, typename Var3Type                   > struct FunctionDelegateBinding_ThreeVars : BaseFFunctionDelegateInstance<FObjectTemplate, FuncType, Var1Type, Var2Type, Var3Type          > { typedef TBaseFFunctionDelegateInstance<FObjectTemplate, FuncType, Var1Type, Var2Type, Var3Type          > Super; FunctionDelegateBinding_ThreeVars(FObjectTemplate* object, const String& function_name, Var1Type Var1, Var2Type Var2, Var3Type Var3) : Super(object, function_name, Var1, Var2, Var3) {} };
  //template <typename FObjectTemplate, typename Var1Type, typename Var2Type, typename Var3Type, typename Var4Type> struct FunctionDelegateBinding_FourVars  : BaseFFunctionDelegateInstance<FObjectTemplate, FuncType, Var1Type, Var2Type, Var3Type, Var4Type> { typedef TBaseFFunctionDelegateInstance<FObjectTemplate, FuncType, Var1Type, Var2Type, Var3Type, Var4Type> Super; FunctionDelegateBinding_FourVars (FObjectTemplate* object, const String& function_name, Var1Type Var1, Var2Type Var2, Var3Type Var3, Var4Type Var4) : Super(object, function_name, Var1, Var2, Var3, Var4) {} };

  ///** Declare the user's FObject-based delegate instance types. */
  //template <typename Class                                                                            > struct FObjectMethodDelegate                 : BaseFObjectMethodDelegateInstance<false, Class, FuncType                                        > { typedef TBaseFObjectMethodDelegateInstance<false, Class, FuncType                                        > Super; FObjectMethodDelegate                (Class* object, typename Super::MethodPtr method     ) : Super(object, method) {} };
  //template <typename Class                                                                            > struct FObjectMethodDelegate_Const           : BaseFObjectMethodDelegateInstance<true , Class, FuncType                                        > { typedef TBaseFObjectMethodDelegateInstance<true , Class, FuncType                                        > Super; FObjectMethodDelegate_Const          (Class* object, typename Super::MethodPtr method     ) : Super(object, method) {} };
  //template <typename Class, typename Var1Type                                                         > struct FObjectMethodDelegate_OneVar          : BaseFObjectMethodDelegateInstance<false, Class, FuncType, Var1Type                              > { typedef TBaseFObjectMethodDelegateInstance<false, Class, FuncType, Var1Type                              > Super; FObjectMethodDelegate_OneVar         (Class* object, typename Super::MethodPtr method, Var1Type Var1) : Super(object, method, Var1) {} };
  //template <typename Class, typename Var1Type                                                         > struct FObjectMethodDelegate_OneVar_Const    : BaseFObjectMethodDelegateInstance<true , Class, FuncType, Var1Type                              > { typedef TBaseFObjectMethodDelegateInstance<true , Class, FuncType, Var1Type                              > Super; FObjectMethodDelegate_OneVar_Const   (Class* object, typename Super::MethodPtr method, Var1Type Var1) : Super(object, method, Var1) {} };
  //template <typename Class, typename Var1Type, typename Var2Type                                      > struct FObjectMethodDelegate_TwoVars         : BaseFObjectMethodDelegateInstance<false, Class, FuncType, Var1Type, Var2Type                    > { typedef TBaseFObjectMethodDelegateInstance<false, Class, FuncType, Var1Type, Var2Type                    > Super; FObjectMethodDelegate_TwoVars        (Class* object, typename Super::MethodPtr method, Var1Type Var1, Var2Type Var2) : Super(object, method, Var1, Var2) {} };
  //template <typename Class, typename Var1Type, typename Var2Type                                      > struct FObjectMethodDelegate_TwoVars_Const   : BaseFObjectMethodDelegateInstance<true , Class, FuncType, Var1Type, Var2Type                    > { typedef TBaseFObjectMethodDelegateInstance<true , Class, FuncType, Var1Type, Var2Type                    > Super; FObjectMethodDelegate_TwoVars_Const  (Class* object, typename Super::MethodPtr method, Var1Type Var1, Var2Type Var2) : Super(object, method, Var1, Var2) {} };
  //template <typename Class, typename Var1Type, typename Var2Type, typename Var3Type                   > struct FObjectMethodDelegate_ThreeVars       : BaseFObjectMethodDelegateInstance<false, Class, FuncType, Var1Type, Var2Type, Var3Type          > { typedef TBaseFObjectMethodDelegateInstance<false, Class, FuncType, Var1Type, Var2Type, Var3Type          > Super; FObjectMethodDelegate_ThreeVars      (Class* object, typename Super::MethodPtr method, Var1Type Var1, Var2Type Var2, Var3Type Var3) : Super(object, method, Var1, Var2, Var3) {} };
  //template <typename Class, typename Var1Type, typename Var2Type, typename Var3Type                   > struct FObjectMethodDelegate_ThreeVars_Const : BaseFObjectMethodDelegateInstance<true , Class, FuncType, Var1Type, Var2Type, Var3Type          > { typedef TBaseFObjectMethodDelegateInstance<true , Class, FuncType, Var1Type, Var2Type, Var3Type          > Super; FObjectMethodDelegate_ThreeVars_Const(Class* object, typename Super::MethodPtr method, Var1Type Var1, Var2Type Var2, Var3Type Var3) : Super(object, method, Var1, Var2, Var3) {} };
  //template <typename Class, typename Var1Type, typename Var2Type, typename Var3Type, typename Var4Type> struct FObjectMethodDelegate_FourVars        : BaseFObjectMethodDelegateInstance<false, Class, FuncType, Var1Type, Var2Type, Var3Type, Var4Type> { typedef TBaseFObjectMethodDelegateInstance<false, Class, FuncType, Var1Type, Var2Type, Var3Type, Var4Type> Super; FObjectMethodDelegate_FourVars       (Class* object, typename Super::MethodPtr method, Var1Type Var1, Var2Type Var2, Var3Type Var3, Var4Type Var4) : Super(object, method, Var1, Var2, Var3, Var4) {} };
  //template <typename Class, typename Var1Type, typename Var2Type, typename Var3Type, typename Var4Type> struct FObjectMethodDelegate_FourVars_Const  : BaseFObjectMethodDelegateInstance<true , Class, FuncType, Var1Type, Var2Type, Var3Type, Var4Type> { typedef TBaseFObjectMethodDelegateInstance<true , Class, FuncType, Var1Type, Var2Type, Var3Type, Var4Type> Super; FObjectMethodDelegate_FourVars_Const (Class* object, typename Super::MethodPtr method, Var1Type Var1, Var2Type Var2, Var3Type Var3, Var4Type Var4) : Super(object, method, Var1, Var2, Var3, Var4) {} };

  /** Declare the user's static function pointer delegate instance types. */
                                                                                        struct StaticDelegate           : BaseStaticDelegateInstance<FuncType                                        > { typedef BaseStaticDelegateInstance<FuncType                                        > Super; StaticDelegate          (typename Super::FuncPtr func     ) : Super(func) {} };
  template <typename Var1Type                                                         > struct StaticDelegate_OneVar    : BaseStaticDelegateInstance<FuncType, Var1Type                              > { typedef BaseStaticDelegateInstance<FuncType, Var1Type                              > Super; StaticDelegate_OneVar   (typename Super::FuncPtr func, Var1Type Var1) : Super(func, Var1) {} };
  template <typename Var1Type, typename Var2Type                                      > struct StaticDelegate_TwoVars   : BaseStaticDelegateInstance<FuncType, Var1Type, Var2Type                    > { typedef BaseStaticDelegateInstance<FuncType, Var1Type, Var2Type                    > Super; StaticDelegate_TwoVars  (typename Super::FuncPtr func, Var1Type Var1, Var2Type Var2) : Super(func, Var1, Var2) {} };
  template <typename Var1Type, typename Var2Type, typename Var3Type                   > struct StaticDelegate_ThreeVars : BaseStaticDelegateInstance<FuncType, Var1Type, Var2Type, Var3Type          > { typedef BaseStaticDelegateInstance<FuncType, Var1Type, Var2Type, Var3Type          > Super; StaticDelegate_ThreeVars(typename Super::FuncPtr func, Var1Type Var1, Var2Type Var2, Var3Type Var3) : Super(func, Var1, Var2, Var3) {} };
  template <typename Var1Type, typename Var2Type, typename Var3Type, typename Var4Type> struct StaticDelegate_FourVars  : BaseStaticDelegateInstance<FuncType, Var1Type, Var2Type, Var3Type, Var4Type> { typedef BaseStaticDelegateInstance<FuncType, Var1Type, Var2Type, Var3Type, Var4Type> Super; StaticDelegate_FourVars (typename Super::FuncPtr func, Var1Type Var1, Var2Type Var2, Var3Type Var3, Var4Type Var4) : Super(func, Var1, Var2, Var3, Var4) {} };

 public:
  /**
   * Static: Creates a raw C++ pointer global function delegate
   */
  template <typename... Vars>
  inline static BaseDelegate<RetType, Params...> CreateStatic(typename Identity<RetType (*)(Params..., Vars...)>::Type func, Vars... vars)
  {
    BaseDelegate<RetType, Params...> result;
    BaseStaticDelegateInstance<FuncType, Vars...>::Create(result, func, vars...);
    return result;
  }

  /**
   * Static: Creates a C++ lambda delegate
   * technically this works for any functor types, but lambdas are the primary use case
   */
  template <typename Functor, typename... Vars>
  inline static BaseDelegate<RetType, Params...> CreateLambda(Functor&& functor, Vars... vars)
  {
    BaseDelegate<RetType, Params...> result;
    BaseFunctorDelegateInstance<FuncType, typename RemoveReference<Functor>::Type, Vars...>::Create(result, Forward<Functor>(functor), vars...);
    return result;
  }

  /**
   * Static: Creates a raw C++ pointer member function delegate.
   *
   * Raw pointer doesn't use any sort of reference, so may be unsafe to call if the object was
   * deleted out from underneath your delegate. Be careful when calling Execute()!
   */
  template <typename Class, typename... Vars>
  inline static BaseDelegate<RetType, Params...> CreateRaw(Class* object, typename MemberFunctionPtrType<false, Class, RetType (Params..., Vars...)>::Type method, Vars... vars)
  {
    BaseDelegate<RetType, Params...> result;
    BaseRawMethodDelegateInstance<false, Class, FuncType, Vars...>::Create(result, object, method, vars...);
    return result;
  }
  template <typename Class, typename... Vars>
  inline static BaseDelegate<RetType, Params...> CreateRaw(Class* object, typename MemberFunctionPtrType<true, Class, RetType (Params..., Vars...)>::Type method, Vars... vars)
  {
    BaseDelegate<RetType, Params...> result;
    BaseRawMethodDelegateInstance<true , Class, FuncType, Vars...>::Create(result, object, method, vars...);
    return result;
  }

  /**
   * Static: Creates a shared pointer-based (fast, not thread-safe) member function delegate.
   *
   * Shared pointer delegates keep a weak reference to your object.
   * You can use ExecuteIfBound() to call them.
   */
  template <typename Class, typename... Vars>
  inline static BaseDelegate<RetType, Params...> CreateSP(const SharedRef<Class, SPMode::Fast>& object_ref, typename MemberFunctionPtrType<false, Class, RetType (Params..., Vars...)>::Type method, Vars... vars)
  {
    BaseDelegate<RetType, Params...> result;
    BaseSPMethodDelegateInstance<false, Class, SPMode::Fast, FuncType, Vars...>::Create(result, object_ref, method, vars...);
    return result;
  }
  template <typename Class, typename... Vars>
  inline static BaseDelegate<RetType, Params...> CreateSP(const SharedRef<Class, SPMode::Fast>& object_ref, typename MemberFunctionPtrType<true, Class, RetType (Params..., Vars...)>::Type method, Vars... vars)
  {
    BaseDelegate<RetType, Params...> result;
    BaseSPMethodDelegateInstance<true , Class, SPMode::Fast, FuncType, Vars...>::Create(result, object_ref, method, vars...);
    return result;
  }

  /**
   * Static: Creates a shared pointer-based (fast, not thread-safe) member function delegate.
   *
   * Shared pointer delegates keep a weak reference to your object.
   * You can use ExecuteIfBound() to call them.
   */
  template <typename Class, typename... Vars>
  inline static BaseDelegate<RetType, Params...> CreateSP(Class* object, typename MemberFunctionPtrType<false, Class, RetType (Params..., Vars...)>::Type method, Vars... vars)
  {
    return CreateSP(StaticCastSharedRef<Class>(object->AsShared()), method, vars...);
  }
  template <typename Class, typename... Vars>
  inline static BaseDelegate<RetType, Params...> CreateSP(Class* object, typename MemberFunctionPtrType<true, Class, RetType (Params..., Vars...)>::Type method, Vars... vars)
  {
    return CreateSP(StaticCastSharedRef<Class>(object->AsShared()), method, vars...);
  }

  /**
   * Static: Creates a shared pointer-based (slower, conditionally thread-safe) member function delegate.
   *
   * Shared pointer delegates keep a weak reference to your object.
   * You can use ExecuteIfBound() to call them.
   */
  template <typename Class, typename... Vars>
  inline static BaseDelegate<RetType, Params...> CreateThreadSafeSP(const SharedRef<Class, SPMode::ThreadSafe>& object_ref, typename MemberFunctionPtrType<false, Class, RetType (Params..., Vars...)>::Type method, Vars... vars)
  {
    BaseDelegate<RetType, Params...> result;
    BaseSPMethodDelegateInstance<false, Class, SPMode::ThreadSafe, FuncType, Vars...>::Create(result, object_ref, method, vars...);
    return result;
  }
  template <typename Class, typename... Vars>
  inline static BaseDelegate<RetType, Params...> CreateThreadSafeSP(const SharedRef<Class, SPMode::ThreadSafe>& object_ref, typename MemberFunctionPtrType<true, Class, RetType (Params..., Vars...)>::Type method, Vars... vars)
  {
    BaseDelegate<RetType, Params...> result;
    BaseSPMethodDelegateInstance<true , Class, SPMode::ThreadSafe, FuncType, Vars...>::Create(result, object_ref, method, vars...);
    return result;
  }

  /**
   * Static: Creates a shared pointer-based (slower, conditionally thread-safe) member function delegate.
   *
   * Shared pointer delegates keep a weak reference to your object.
   * You can use ExecuteIfBound() to call them.
   */
  template <typename Class, typename... Vars>
  inline static BaseDelegate<RetType, Params...> CreateThreadSafeSP(Class* object, typename MemberFunctionPtrType<false, Class, RetType (Params..., Vars...)>::Type method, Vars... vars)
  {
    return CreateThreadSafeSP(StaticCastSharedRef<Class>(object->AsShared()), method, vars...);
  }
  template <typename Class, typename... Vars>
  inline static BaseDelegate<RetType, Params...> CreateThreadSafeSP(Class* object, typename MemberFunctionPtrType<true, Class, RetType (Params..., Vars...)>::Type method, Vars... vars)
  {
    return CreateThreadSafeSP(StaticCastSharedRef<Class>(object->AsShared()), method, vars...);
  }

  ///**
  // * Static: Creates a FFunction-based member function delegate.
  // *
  // * FFunction delegates keep a weak reference to your object.
  // * You can use ExecuteIfBound() to call them.
  // */
  //template <typename FObjectTemplate, typename... Vars>
  //inline static BaseDelegate<RetType, Params...> CreateFFunction(FObjectTemplate* object, const String& function_name, Vars... vars)
  //{
  //  BaseDelegate<RetType, Params...> result;
  //  BaseFFunctionDelegateInstance<FObjectTemplate, FuncType, Vars...>::Create(result, object, function_name, vars...);
  //  return result;
  //}
  //
  ///**
  // * Static: Creates a FObject-based member function delegate.
  // *
  // * FObject delegates keep a weak reference to your object.
  // * You can use ExecuteIfBound() to call them.
  // */
  //template <typename Class, typename... Vars>
  //inline static BaseDelegate<RetType, Params...> CreateFObject(Class* object, typename MemberFunctionPtrType<false, Class, RetType (Params..., Vars...)>::Type method, Vars... vars)
  //{
  //  BaseDelegate<RetType, Params...> result;
  //  BaseFObjectMethodDelegateInstance<false, Class, FuncType, Vars...>::Create(result, object, method, vars...);
  //  return result;
  //}
  //template <typename Class, typename... Vars>
  //inline static BaseDelegate<RetType, Params...> CreateFObject(Class* object, typename MemberFunctionPtrType<true, Class, RetType (Params..., Vars...)>::Type method, Vars... vars)
  //{
  //  BaseDelegate<RetType, Params...> result;
  //  BaseFObjectMethodDelegateInstance<true , Class, FuncType, Vars...>::Create(result, object, method, vars...);
  //  return result;
  //}

 public:
  /**
   * Default constructor
   */
  inline BaseDelegate()
  {
  }

  /**
   * 'Null' constructor
   */
  inline BaseDelegate(decltype(nullptr))
  {
  }

  /**
   * Destructor.
   */
  inline ~BaseDelegate()
  {
    Unbind();
  }

  /**
   * Move constructor.
   *
   * @param other The delegate object to move from.
   */
  inline BaseDelegate(BaseDelegate&& other)
  {
    *this = MoveTemp(other);
  }

  /**
   * Creates and initializes a new instance from an existing delegate object.
   *
   * @param other The delegate object to copy from.
   */
  inline BaseDelegate(const BaseDelegate& other)
  {
    *this = other;
  }

  /**
   * Move assignment operator.
   *
   * @param OtherDelegate Delegate object to copy from
   */
  inline BaseDelegate& operator = (BaseDelegate&& other)
  {
    if (FUN_LIKELY(&other != this)) {
      // this down-cast is OK! allows for managing invocation list
      // in the base class without requiring virtual functions
      DelegateInstanceInterface* other_instance = other.GetDelegateInstanceProtected();
      if (other_instance) {
        other_instance->CreateCopy(*this);
      }
      else {
        Unbind();
      }
    }

    return *this;
  }

  /**
   * Assignment operator.
   *
   * @param OtherDelegate Delegate object to copy from
   */
  inline BaseDelegate& operator = (const BaseDelegate& other)
  {
    if (FUN_LIKELY(&other != this)) {
      // this down-cast is OK! allows for managing invocation list
      // in the base class without requiring virtual functions
      DelegateInstanceInterface* other_instance = other.GetDelegateInstanceProtected();
      if (other_instance) {
        other_instance->CreateCopy(*this);
      }
      else {
        Unbind();
      }
    }

    return *this;
  }

 public:
  /**
   * Binds a raw C++ pointer global function delegate
   */
  template <typename... Vars>
  inline void BindStatic(typename BaseStaticDelegateInstance<FuncType, Vars...>::FuncPtr func, Vars... vars)
  {
    *this = CreateStatic(func, vars...);
  }

  /**
   * Static: Binds a C++ lambda delegate
   * technically this works for any functor types, but lambdas are the primary use case
   */
  template <typename Functor, typename... Vars>
  inline void BindLambda(Functor&& functor, Vars... vars)
  {
    *this = CreateLambda(Forward<Functor>(functor), vars...);
  }

  /**
   * Binds a raw C++ pointer delegate.
   *
   * Raw pointer doesn't use any sort of reference, so may be unsafe to call if the object was
   * deleted out from underneath your delegate.
   *
   * Be careful when calling Execute()
   */
  template <typename Class, typename... Vars>
  inline void BindRaw(Class* object, typename MemberFunctionPtrType<false, Class, RetType (Params..., Vars...)>::Type method, Vars... vars)
  {
    *this = CreateRaw(object, method, vars...);
  }
  template <typename Class, typename... Vars>
  inline void BindRaw(Class* object, typename MemberFunctionPtrType<true, Class, RetType (Params..., Vars...)>::Type method, Vars... vars)
  {
    *this = CreateRaw(object, method, vars...);
  }

  /**
   * Binds a shared pointer-based (fast, not thread-safe) member function delegate.
   * Shared pointer delegates keep a weak reference to your object.
   *
   * You can use ExecuteIfBound() to call them.
   */
  template <typename Class, typename... Vars>
  inline void BindSP(const SharedRef<Class, SPMode::Fast>& object_ref, typename MemberFunctionPtrType<false, Class, RetType (Params..., Vars...)>::Type method, Vars... vars)
  {
    *this = CreateSP(object_ref, method, vars...);
  }
  template <typename Class, typename... Vars>
  inline void BindSP(const SharedRef<Class, SPMode::Fast>& object_ref, typename MemberFunctionPtrType<true, Class, RetType (Params..., Vars...)>::Type method, Vars... vars)
  {
    *this = CreateSP(object_ref, method, vars...);
  }

  /**
   * Binds a shared pointer-based (fast, not thread-safe) member function delegate.
   *
   * Shared pointer delegates keep a weak reference to your object.
   *
   * You can use ExecuteIfBound() to call them.
   */
  template <typename Class, typename... Vars>
  inline void BindSP(Class* object, typename MemberFunctionPtrType<false, Class, RetType (Params..., Vars...)>::Type method, Vars... vars)
  {
    *this = CreateSP(object, method, vars...);
  }
  template <typename Class, typename... Vars>
  inline void BindSP(Class* object, typename MemberFunctionPtrType<true, Class, RetType (Params..., Vars...)>::Type method, Vars... vars)
  {
    *this = CreateSP(object, method, vars...);
  }

  /**
   * Binds a shared pointer-based (slower, conditionally thread-safe) member function delegate.
   *
   * Shared pointer delegates keep a weak reference to your object.
   *
   * You can use ExecuteIfBound() to call them.
   */
  template <typename Class, typename... Vars>
  inline void BindThreadSafeSP(const SharedRef<Class, SPMode::ThreadSafe>& object_ref, typename MemberFunctionPtrType<false, Class, RetType (Params..., Vars...)>::Type method, Vars... vars)
  {
    *this = CreateThreadSafeSP(object_ref, method, vars...);
  }
  template <typename Class, typename... Vars>
  inline void BindThreadSafeSP(const SharedRef<Class, SPMode::ThreadSafe>& object_ref, typename MemberFunctionPtrType<true, Class, RetType (Params..., Vars...)>::Type method, Vars... vars)
  {
    *this = CreateThreadSafeSP(object_ref, method, vars...);
  }

  /**
   * Binds a shared pointer-based (slower, conditionally thread-safe) member function delegate.
   *
   * Shared pointer delegates keep a weak reference to your object.
   *
   * You can use ExecuteIfBound() to call them.
   */
  template <typename Class, typename... Vars>
  inline void BindThreadSafeSP(Class* object, typename MemberFunctionPtrType<false, Class, RetType (Params..., Vars...)>::Type method, Vars... vars)
  {
    *this = CreateThreadSafeSP(object, method, vars...);
  }
  template <typename Class, typename... Vars>
  inline void BindThreadSafeSP(Class* object, typename MemberFunctionPtrType<true, Class, RetType (Params..., Vars...)>::Type method, Vars... vars)
  {
    *this = CreateThreadSafeSP(object, method, vars...);
  }

  ///**
  // * Binds a FFunction-based member function delegate.
  // *
  // * FFunction delegates keep a weak reference to your object.
  // *
  // * You can use ExecuteIfBound() to call them.
  // */
  //template <typename FObjectTemplate, typename... Vars>
  //inline void BindFFunction(FObjectTemplate* object, const String& function_name, Vars... vars)
  //{
  //  *this = CreateFFunction(object, function_name, vars...);
  //}
  //
  ///**
  // * Binds a FObject-based member function delegate.
  // *
  // * FObject delegates keep a weak reference to your object.
  // *
  // * You can use ExecuteIfBound() to call them.
  // */
  //template <typename Class, typename... Vars>
  //inline void BindFObject(Class* object, typename MemberFunctionPtrType<false, Class, RetType (Params..., Vars...)>::Type method, Vars... vars)
  //{
  //  *this = CreateFObject(object, method, vars...);
  //}
  //template <typename Class, typename... Vars>
  //inline void BindFObject(Class* object, typename MemberFunctionPtrType<true, Class, RetType (Params..., Vars...)>::Type method, Vars... vars)
  //{
  //  *this = CreateFObject(object, method, vars...);
  //}

 public:
  /**
   * Execute the delegate.
   *
   * If the function pointer is not valid, an error will occur.
   *
   * Check IsBound() before calling this method or use ExecuteIfBound() instead.
   *
   * @see ExecuteIfBound
   */
  FUN_ALWAYS_INLINE RetType Execute(Params... params) const
  {
    DelegateInstanceInterface* local_delegate_instance = GetDelegateInstanceProtected();

    // If this assert goes off, Execute() was called before a function was bound to the delegate.
    // Consider using ExecuteIfSafe() instead.
    fun_check_dbg(local_delegate_instance != nullptr);

    return local_delegate_instance->Execute(params...);
  }

  /**
   * Returns a pointer to the correctly-typed delegate instance.
   */
  FUN_ALWAYS_INLINE DelegateInstanceInterface* GetDelegateInstanceProtected() const
  {
    return (DelegateInstanceInterface*)DelegateBase::GetDelegateInstanceProtected();
  }
};


template <typename... Params>
class BaseDelegate<void, Params...>
  : public BaseDelegate<TypeWrapper<void>, Params...>
{
  typedef BaseDelegate<TypeWrapper<void>, Params...> Super;

 public:
  typedef typename Super::DelegateInstanceInterface DelegateInstanceInterface;

  /**
   * Default constructor
   */
  BaseDelegate()
  {
  }

  /**
   * 'Null' constructor
   */
  FUN_ALWAYS_INLINE BaseDelegate(decltype(nullptr))
    : Super(nullptr)
  {
  }

  /**
   * Move constructor.
   *
   * @param other The delegate object to move from.
   */
  FUN_ALWAYS_INLINE BaseDelegate(BaseDelegate&& other)
    : Super(MoveTemp(other))
  {
  }

  /**
   * Creates and initializes a new instance from an existing delegate object.
   *
   * @param other The delegate object to copy from.
   */
  FUN_ALWAYS_INLINE BaseDelegate(const BaseDelegate& other)
    : Super(other)
  {
  }

  /**
   * Move assignment operator.
   *
   * @param OtherDelegate Delegate object to copy from
   */
  inline BaseDelegate& operator = (BaseDelegate&& other)
  {
    (Super&)*this = MoveTemp((Super&)other);
    return *this;
  }

  /**
   * Assignment operator.
   *
   * @param OtherDelegate Delegate object to copy from
   */
  inline BaseDelegate& operator = (const BaseDelegate& other)
  {
    (Super&)*this = (Super&)other;
    return *this;
  }

  /**
   * Execute the delegate, but only if the function pointer is still valid
   *
   * @return  Returns true if the function was executed
   */
  // NOTE: Currently only delegates with no return value support ExecuteIfBound()
  inline bool ExecuteIfBound(Params... params) const
  {
    if (Super::IsBound()) {
      return Super::GetDelegateInstanceProtected()->ExecuteIfSafe(params...);
    }

    return false;
  }
};


/**
 * Multicast delegate base class.
 *
 * This class implements the functionality of multicast delegates.
 * It is templated to the function signature that it is compatible with.
 * Use the various FUN_DECLARE_MULTICAST_DELEGATE and
 * FUN_DECLARE_EVENT macros to create actual delegate types.
 *
 * Multicast delegates offer no guarantees for the calling order of
 * bound functions. As bindings get added and removed over time,
 * the calling order may change. Only bindings
 * without return values are supported.
 */
template <typename RetType, typename... Params>
class BaseMulticastDelegate;

template <typename... Params>
class BaseMulticastDelegate<void, Params...>
  : public MulticastDelegateBase<WeakObjectPtr>
{
  typedef MulticastDelegateBase<WeakObjectPtr> Super;

 public:
  /** FUN_DEPRECATED: Type definition for unicast delegate classes whose delegate instances are compatible with this delegate. */
  typedef BaseDelegate< void, Params... > Delegate;

  /** Type definition for the shared interface of delegate instance types compatible with this delegate class. */
  typedef IBaseDelegateInstance<void (Params...)> DelegateInstanceInterface;

 public:
  /**
   * FUN_DEPRECATED: Adds a delegate instance to this multicast delegate's invocation list.
   *
   * This method is retained for backwards compatibility.
   *
   * @param new_delegate The delegate to add.
   */
  DelegateHandle Add(Delegate&& new_delegate)
  {
    DelegateHandle result;
    if (Super::GetDelegateInstanceProtectedHelper(new_delegate)) {
      result = AddDelegateInstance(MoveTemp(new_delegate));
    }

    return result;
  }

  /**
   * FUN_DEPRECATED: Adds a unicast delegate to this multi-cast delegate's invocation list.
   *
   * This method is retained for backwards compatibility.
   *
   * @param new_delegate The delegate to add.
   */
  DelegateHandle Add(const Delegate& new_delegate)
  {
    DelegateHandle result;
    if (Super::GetDelegateInstanceProtectedHelper(new_delegate)) {
      result = AddDelegateInstance(Delegate(new_delegate));
    }

    return result;
  }

  /**
   * Adds a raw C++ pointer global function delegate
   *
   * @param func Function pointer
   */
  template <typename... Vars>
  inline DelegateHandle AddStatic(typename BaseStaticDelegateInstance<void (Params...), Vars...>::FuncPtr func, Vars... vars)
  {
    return Add(Delegate::CreateStatic(func, vars...));
  }

  /**
   * Adds a C++ lambda delegate
   * technically this works for any functor types, but lambdas are the primary use case
   *
   * @param functor Functor (e.g. Lambda)
   */
  template <typename Functor, typename... Vars>
  inline DelegateHandle AddLambda(Functor&& functor, Vars... vars)
  {
    return Add(Delegate::CreateLambda(Forward<Functor>(functor), vars...));
  }

  /**
   * Adds a raw C++ pointer delegate.
   *
   * Raw pointer doesn't use any sort of reference, so may be unsafe to call if the object was
   * deleted out from underneath your delegate. Be careful when calling Execute()!
   *
   * @param object User object to bind to
   * @param method Class method function address
   */
  template <typename Class, typename... Vars>
  inline DelegateHandle AddRaw(Class* object, typename MemberFunctionPtrType<false, Class, void (Params..., Vars...)>::Type method, Vars... vars)
  {
    return Add(Delegate::CreateRaw(object, method, vars...));
  }
  template <typename Class, typename... Vars>
  inline DelegateHandle AddRaw(Class* object, typename MemberFunctionPtrType<true, Class, void (Params..., Vars...)>::Type method, Vars... vars)
  {
    return Add(Delegate::CreateRaw(object, method, vars...));
  }

  /**
   * Adds a shared pointer-based (fast, not thread-safe) member function delegate.
   *
   * Shared pointer delegates keep a weak reference to your object.
   *
   * @param object_ref User object to bind to
   * @param method Class method function address
   */
  template <typename Class, typename... Vars>
  inline DelegateHandle AddSP(const SharedRef<Class, SPMode::Fast>& object_ref, typename MemberFunctionPtrType<false, Class, void (Params..., Vars...)>::Type method, Vars... vars)
  {
    return Add(Delegate::CreateSP(object_ref, method, vars...));
  }
  template <typename Class, typename... Vars>
  inline DelegateHandle AddSP(const SharedRef<Class, SPMode::Fast>& object_ref, typename MemberFunctionPtrType<true, Class, void (Params..., Vars...)>::Type method, Vars... vars)
  {
    return Add(Delegate::CreateSP(object_ref, method, vars...));
  }

  /**
   * Adds a shared pointer-based (fast, not thread-safe) member function delegate.
   *
   * Shared pointer delegates keep a weak reference to your object.
   *
   * @param object  User object to bind to
   * @param method Class method function address
   */
  template <typename Class, typename... Vars>
  inline DelegateHandle AddSP(Class* object, typename MemberFunctionPtrType<false, Class, void (Params..., Vars...)>::Type method, Vars... vars)
  {
    return Add(Delegate::CreateSP(object, method, vars...));
  }
  template <typename Class, typename... Vars>
  inline DelegateHandle AddSP(Class* object, typename MemberFunctionPtrType<true, Class, void (Params..., Vars...)>::Type method, Vars... vars)
  {
    return Add(Delegate::CreateSP(object, method, vars...));
  }

  /**
   * Adds a shared pointer-based (slower, conditionally thread-safe) member function delegate.
   * Shared pointer delegates keep a weak reference to your object.
   *
   * @param object_ref User object to bind to
   * @param method Class method function address
   */
  template <typename Class, typename... Vars>
  inline DelegateHandle AddThreadSafeSP(const SharedRef<Class, SPMode::ThreadSafe>& object_ref, typename MemberFunctionPtrType<false, Class, void (Params..., Vars...)>::Type method, Vars... vars)
  {
    return Add(Delegate::CreateThreadSafeSP(object_ref, method, vars...));
  }
  template <typename Class, typename... Vars>
  inline DelegateHandle AddThreadSafeSP(const SharedRef<Class, SPMode::ThreadSafe>& object_ref, typename MemberFunctionPtrType<true, Class, void (Params..., Vars...)>::Type method, Vars... vars)
  {
    return Add(Delegate::CreateThreadSafeSP(object_ref, method, vars...));
  }

  /**
   * Adds a shared pointer-based (slower, conditionally thread-safe) member function delegate.
   *
   * Shared pointer delegates keep a weak reference to your object.
   *
   * @param object User object to bind to
   * @param method Class method function address
   */
  template <typename Class, typename... Vars>
  inline DelegateHandle AddThreadSafeSP(Class* object, typename MemberFunctionPtrType<false, Class, void (Params..., Vars...)>::Type method, Vars... vars)
  {
    return Add(Delegate::CreateThreadSafeSP(object, method, vars...));
  }
  template <typename Class, typename... Vars>
  inline DelegateHandle AddThreadSafeSP(Class* object, typename MemberFunctionPtrType<true, Class, void (Params..., Vars...)>::Type method, Vars... vars)
  {
    return Add(Delegate::CreateThreadSafeSP(object, method, vars...));
  }

  ///**
  // * Adds a FFunction-based member function delegate.
  // *
  // * FFunction delegates keep a weak reference to your object.
  // *
  // * @param object  User object to bind to
  // * @param function_name Class method function name
  // */
  //template <typename FObjectTemplate, typename... Vars>
  //inline DelegateHandle AddFFunction(FObjectTemplate* object, const String& function_name, Vars... vars)
  //{
  //  return Add(Delegate::CreateFFunction(object, function_name, vars...));
  //}
  //
  ///**
  // * Adds a FObject-based member function delegate.
  // *
  // * FObject delegates keep a weak reference to your object.
  // *
  // * @param object User object to bind to
  // * @param method Class method function address
  // */
  //template <typename Class, typename... Vars>
  //inline DelegateHandle AddFObject(Class* object, typename MemberFunctionPtrType<false, Class, void (Params..., Vars...)>::Type method, Vars... vars)
  //{
  //  return Add(Delegate::CreateFObject(object, method, vars...));
  //}
  //template <typename Class, typename... Vars>
  //inline DelegateHandle AddFObject(Class* object, typename MemberFunctionPtrType<true, Class, void (Params..., Vars...)>::Type method, Vars... vars)
  //{
  //  return Add(Delegate::CreateFObject(object, method, vars...));
  //}

 public:
  /**
   * Removes a delegate instance from this multi-cast delegate's
   * invocation list (performance is O(N)).
   *
   * Note that the order of the delegate instances may not be preserved!
   *
   * @param handle The handle of the delegate instance to remove.
   */
  void Remove(DelegateHandle handle)
  {
    RemoveDelegateInstance(handle);
  }

 protected:
  /**
   * Hidden default constructor.
   */
  inline BaseMulticastDelegate() {}

  /**
   * Hidden copy constructor (for proper deep copies).
   *
   * @param other The multicast delegate to copy from.
   */
  BaseMulticastDelegate(const BaseMulticastDelegate& other)
  {
    *this = other;
  }

  /**
   * Hidden assignment operator (for proper deep copies).
   *
   * @param other The delegate to assign from.
   * @return This instance.
   */
  BaseMulticastDelegate& operator = (const BaseMulticastDelegate& other)
  {
    if (FUN_LIKELY(&other != this)) {
      Super::Clear();

      for (const DelegateBase& other_delegate_ref : other.GetInvocationList()) {
        if (IDelegateInstance* other_instance = Super::GetDelegateInstanceProtectedHelper(other_delegate_ref)) {
          Delegate tmp_delegate;
          ((DelegateInstanceInterface*)other_instance)->CreateCopy(tmp_delegate);
          Super::AddInternal(MoveTemp(tmp_delegate));
        }
      }
    }

    return *this;
  }

 protected:
  /**
   * Adds a function delegate to this multi-cast delegate's invocation list.
   *
   * This method will assert if the same function has already been bound.
   *
   * @param new_delegate Delegate to add
   */
  DelegateHandle AddDelegateInstance(Delegate&& new_delegate)
  {
    return Super::AddInternal(MoveTemp(new_delegate));
  }

 public:
  /**
   * Broadcasts this delegate to all bound objects, except to those that may have expired.
   *
   * The constness of this method is a lie, but it allows for broadcasting from const functions.
   */
  void Broadcast(Params... params) const
  {
    bool needs_compaction = false;

    Super::LockInvocationList();
    {
      const InvocationList& local_invocation_list = Super::GetInvocationList();

      // call bound functions in reverse order, so we ignore any instances that may be added by callees
      for (int32 invocation_item_index = local_invocation_list.Count() - 1; invocation_item_index >= 0; --invocation_item_index) {
        // this down-cast is OK! allows for managing invocation list in the base class without requiring virtual functions
        const Delegate& delegate_base = (const Delegate&)local_invocation_list[invocation_item_index];

        IDelegateInstance* delegate_instance_interface = Super::GetDelegateInstanceProtectedHelper(delegate_base);
        if (delegate_instance_interface == nullptr || !((DelegateInstanceInterface*)delegate_instance_interface)->ExecuteIfSafe(params...)) {
          needs_compaction = true;
        }
      }
    }
    Super::UnlockInvocationList();

    if (needs_compaction) {
      const_cast<BaseMulticastDelegate*>(this)->CompactInvocationList();
    }
  }

 protected:
  /**
   * Removes a function from this multi-cast delegate's
   * invocation list (performance is O(N)).
   *
   * The function is not actually removed, but deleted and marked as removed.
   * It will be removed next time the invocation list is compacted within Broadcast().
   *
   * @param handle The handle of the delegate instance to remove.
   */
  void RemoveDelegateInstance(DelegateHandle handle)
  {
    const InvocationList& local_invocation_list = Super::GetInvocationList();

    for (int32 invocation_item_index = 0; invocation_item_index < local_invocation_list.Count(); ++invocation_item_index) {
      // InvocationList is const, so we const_cast to be able to unbind the entry
      // TODO: This is horrible, can we get the base class to do it?
      DelegateBase& delegate_base = const_cast<DelegateBase&>(local_invocation_list[invocation_item_index]);

      IDelegateInstance* delegate_instance_interface = Super::GetDelegateInstanceProtectedHelper(delegate_base);
      if (delegate_instance_interface && delegate_instance_interface->GetHandle() == handle) {
        delegate_base.Unbind();
        break; // each delegate binding has a unique handle, so once we find it, we can stop
      }
    }

    Super::CompactInvocationList();
  }
};


/**
 * Implements a multicast delegate.
 *
 * This class should not be instantiated directly.
 * Use the FUN_DECLARE_MULTICAST_DELEGATE macros instead.
 */
template <typename RetType, typename... Params>
class MulticastDelegate;

template <typename... Params>
class MulticastDelegate<void, Params...>
  : public BaseMulticastDelegate<void, Params... >
{
 private:
  // Prevents erroneous use by other classes.
  typedef BaseMulticastDelegate< void, Params... > Super;
};


///**
// * Dynamic delegate base object (FObject-based, serializable).
// * You'll use the various FUN_DECLARE_DYNAMIC_DELEGATE
// *
// * macros to create the actual delegate type, templated to
// * the function signature the delegate is compatible with.
// * Then, you can create an instance of that class when you want to
// * assign functions to the delegate.
// */
//template <typename WeakPtr, typename RetType, typename... Params>
//class BaseDynamicDelegate : public ScriptDelegate<WeakPtr>
//{
// public:
//  /**
//   * Default constructor
//   */
//  BaseDynamicDelegate() {}
//
//  /**
//   * Construction from an FScriptDelegate must be explicit.
//   * This is really only used by FObject system internals.
//   *
//   * @param script_delegate The delegate to construct from by copying
//   */
//  explicit BaseDynamicDelegate(const ScriptDelegate<WeakPtr>& script_delegate)
//    : ScriptDelegate<WeakPtr>(script_delegate)
//  {}
//
//  /**
//   * Templated helper class to define a typedef for user's method pointer, then used below
//   */
//  template <typename Class>
//  class MethodPtrResolver {
//   public:
//    typedef RetType (Class::*MethodPtr)(Params... params);
//  };
//
//  /**
//   * Binds a FObject instance and a FObject method address to this delegate.
//   *
//   * @param object FObject instance
//   * @param method Member function address pointer
//   * @param function_name Name of member function, without class name
//   *
//   * NOTE:  Do not call this function directly.  Instead, call BindDynamic() which is a macro proxy function that
//   *        automatically sets the function name string for the caller.
//   */
//  template <typename Class>
//  void __Internal_BindDynamic(Class* object, typename MethodPtrResolver<Class>::MethodPtr method, String function_name)
//  {
//    fun_check(object != nullptr && method != nullptr);
//
//    // NOTE: We're not actually storing the incoming method pointer or calling it.  We simply require it for type-safety reasons.
//
//    // NOTE: If you hit a compile error on the following line, it means you're trying to use a non-FObject type
//    //       with this delegate, which is not supported
//    this->object_ = object;
//
//    // Store the function name.  The incoming function name was generated by a macro and includes the method's class name.
//    this->function_name_ = function_name;
//
//    fun_check_msg(this->IsBound(), "Unable to bind delegate to '{0}' (function might not be marked as a FFUNCTION or object may be pending kill)"), function_name.ToString());
//  }
//
//  friend uint32 HashOf(const BaseDynamicDelegate& key)
//  {
//    return Crc::Crc32(&key, sizeof(key));
//  }
//
//  // NOTE:  Execute() method must be defined in derived classes
//
//  // NOTE:  ExecuteIfBound() method must be defined in derived classes
//};
//
//
///**
// * Dynamic multi-cast delegate base object (FObject-based, serializable).
// *
// * You'll use the various FUN_DECLARE_DYNAMIC_MULTICAST_DELEGATE macros
// * to create the actual delegate type, templated to the function
// * signature the delegate is compatible with.
// *
// * Then, you can create an instance of that class when you
// * want to assign functions to the delegate.
// */
//template <typename WeakPtr, typename RetType, typename... Params>
//class BaseDynamicMulticastDelegate : public MulticastScriptDelegate<WeakPtr>
//{
// public:
//  /** The actual single-cast delegate class for this multi-cast delegate */
//  typedef BaseDynamicDelegate<WeakObjectPtr, RetType, Params...> Delegate;
//
//  /**
//   * Default constructor
//   */
//  BaseDynamicMulticastDelegate() {}
//
//  /**
//   * Construction from an MulticastScriptDelegate must be explicit.
//   * This is really only used by FObject system internals.
//   *
//   * @param script_delegate The delegate to construct from by copying
//   */
//  explicit BaseDynamicMulticastDelegate(const MulticastScriptDelegate<WeakPtr>& multicast_script_delegate)
//    : MulticastScriptDelegate<WeakPtr>(multicast_script_delegate)
//  {}
//
//  /**
//   * Tests if a FObject instance and a FObject method address pair are
//   * already bound to this multi-cast delegate.
//   *
//   * @param object FObject instance
//   * @param method Member function address pointer
//   * @param function_name Name of member function, without class name
//   *
//   * @return True if the instance/method is already bound.
//   *
//   * NOTE:  Do not call this function directly.  Instead, call IsAlreadyBound() which is a macro proxy function that
//   *        automatically sets the function name string for the caller.
//   */
//  template <typename Class>
//  bool __Internal_IsAlreadyBound(Class* object, typename Delegate::template MethodPtrResolver<Class>::MethodPtr method, String function_name) const
//  {
//    fun_check(object != nullptr && method != nullptr);
//
//    // NOTE: We're not actually using the incoming method pointer or calling it.
//    // We simply require it for type-safety reasons.
//
//    return this->Contains(object, function_name);
//  }
//
//  /**
//   * Binds a FObject instance and a FObject method address to this multi-cast delegate.
//   *
//   * @param object FObject instance
//   * @param method Member function address pointer
//   * @param function_name Name of member function, without class name
//   *
//   * NOTE:  Do not call this function directly.  Instead, call AddDynamic() which is a macro proxy function that
//   *        automatically sets the function name string for the caller.
//   */
//  template <typename Class>
//  void __Internal_AddDynamic(Class* object, typename Delegate::template MethodPtrResolver<Class>::MethodPtr method, String function_name)
//  {
//    fun_check(object != nullptr && method != nullptr);
//
//    // NOTE: We're not actually storing the incoming method pointer or calling it.  We simply require it for type-safety reasons.
//
//    Delegate new_delegate;
//    new_delegate.__Internal_BindDynamic(object, method, function_name);
//
//    this->Add(new_delegate);
//  }
//
//  /**
//   * Binds a FObject instance and a FObject method address to
//   * this multi-cast delegate, but only if it hasn't been bound before.
//   *
//   * @param object FObject instance
//   * @param method Member function address pointer
//   * @param function_name Name of member function, without class name
//   *
//   * NOTE:  Do not call this function directly.  Instead, call AddUniqueDynamic() which is a macro proxy function that
//   *        automatically sets the function name string for the caller.
//   */
//  template <typename Class>
//  void __Internal_AddUniqueDynamic(Class* object, typename Delegate::template MethodPtrResolver<Class>::MethodPtr method, String function_name)
//  {
//    fun_check(object != nullptr && method != nullptr);
//
//    // NOTE: We're not actually storing the incoming method pointer or calling it.
//    // We simply require it for type-safety reasons.
//
//    Delegate new_delegate;
//    new_delegate.__Internal_BindDynamic(object, method, function_name);
//
//    this->AddUnique(new_delegate);
//  }
//
//  /**
//   * Unbinds a FObject instance and a FObject method address from this multi-cast delegate.
//   *
//   * @param object FObject instance
//   * @param method Member function address pointer
//   * @param function_name Name of member function, without class name
//   *
//   * NOTE:  Do not call this function directly.  Instead, call RemoveDynamic() which is a macro proxy function that
//   *        automatically sets the function name string for the caller.
//   */
//  template <typename Class>
//  void __Internal_RemoveDynamic(Class* object, typename Delegate::template MethodPtrResolver<Class>::MethodPtr method, String function_name)
//  {
//    fun_check(object != nullptr && method != nullptr);
//
//    // NOTE: We're not actually storing the incoming method pointer or calling it.
//    // We simply require it for type-safety reasons.
//
//    this->Remove(object, function_name);
//  }
//
//  // NOTE:  Broadcast() method must be defined in derived classes
//};

} // namespace fun
