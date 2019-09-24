#pragma once

#include "fun/base/base.h"
#include "fun/base/async_result.h"
#include "fun/base/async_runnable.h"
#include "fun/base/async_starter.h"

namespace fun {

/**
 * An active method is a method that, when called, executes
 * in its own thread. AsyncMethod's take exactly one
 * argument and can return a value. To pass more than one
 * argument to the method, use a struct.
 * The following example shows how to add an AsyncMethod
 * to a class:
 * 
 *   class OurAsyncObject {
 *   public:
 *     OurAsyncObject()
 *       : async_method_(this, &OurAsyncObject::DoSomething) {
 *     }
 * 
 *     AsyncMethod<String, String, OurAsyncObject> async_method_;
 * 
 *   protected:
 *     String DoSomething(const String& arg) {
 *       ...
 *     }
 *   };
 * 
 * And following is an example that shows how to invoke an AsyncMethod.
 * 
 *   OurAsyncObject our_async_object;
 *   ActiveResult<String> result = our_async_object.async_method_("foo");
 *   ...
 *   result.Wait();
 *   std::cout << result.data() << std::endl;
 * 
 * The way an AsyncMethod is started can be changed by passing a StarterType
 * template argument with a corresponding class. The default AsyncStarter
 * starts the method in its own thread, obtained from a thread pool.
 * 
 * For an alternative implementation of StarterType, see AsyncDispatcher.
 * 
 * For methods that do not require an argument or a return value, the Void
 * class can be used.
 */
template <
    typename ResultType,
    typename ArgsType,
    typename OwnerType,
    typename StarterType = AsyncStarter<OwnerType>
  >
class AsyncMethod {
 public:
  typedef ResultType (OwnerType::*Callback)(const ArgsType&);
  typedef AsyncResult<ResultType> AsyncResultType;
  typedef AsyncRunnable<ResultType, ArgsType, OwnerType> AsyncRunnableType;

  AsyncMethod(OwnerType* owner, Callback method)
    : owner_(owner), method_(method) {
    fun_check_ptr(owner_);
  }

  AsyncResultType operator () (const ArgsType& arg) {
    AsyncResultType result(new AsyncResultHolder<ResultType>());
    AsyncRunnableBase::Ptr runnable(new AsyncRunnableType(owner_, method_, arg, result));
    StarterType::Start(owner_, runnable);
    return result;
  }

  AsyncMethod(const AsyncMethod& rhs)
    : owner_(rhs.owner_), method_(rhs.method_) {}

  AsyncMethod& operator = (const AsyncMethod& rhs) {
    AsyncMethod tmp(rhs);
    Swap(tmp);
    return *this;
  }

  void Swap(AsyncMethod& other) {
    fun::Swap(owner_, other.owner_);
    fun::Swap(method_, other.method_);
  }

 private:
  AsyncMethod();

  OwnerType* owner_;
  Callback method_;
};


template <
    typename ResultType,
    typename OwnerType,
    typename StarterType
  >
class AsyncMethod<ResultType, void, OwnerType, StarterType> {
 public:
  typedef ResultType (OwnerType::*Callback)();
  typedef AsyncResult<ResultType> AsyncResultType;
  typedef AsyncRunnable<ResultType, void, OwnerType> AsyncRunnableType;

  AsyncMethod(OwnerType* owner, Callback method)
    : owner_(owner), method_(method) {
    fun_check_ptr(owner_);
  }

  AsyncResultType operator () () {
    AsyncResultType result(new AsyncResultHolder<ResultType>());
    AsyncRunnableBase::Ptr runnable(new AsyncRunnableType(owner_, method_, result));
    StarterType::Start(owner_, runnable);
    return result;
  }

  AsyncMethod(const AsyncMethod& rhs)
    : owner_(rhs.owner_), method_(rhs.method_) {}

  AsyncMethod& operator = (const AsyncMethod& rhs) {
    AsyncMethod tmp(rhs);
    Swap(tmp);
    return *this;
  }

  void Swap(AsyncMethod& other) {
    fun::Swap(owner_, other.owner_);
    fun::Swap(method_, other.method_);
  }

 private:
  AsyncMethod();

  OwnerType* owner_;
  Callback method_;
};

} // namespace fun
