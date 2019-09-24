#pragma once

#include "fun/base/base.h"
#include "fun/base/async_result.h"
#include "fun/base/runnable.h"
#include "fun/base/exception.h"
#include "fun/base/ref_counted.h"

namespace fun {

/**
 * The base class for all ActiveRunnable instantiations.
 */
class AsyncRunnableBase : public Runnable, public RefCountedObject {
 public:
  typedef RefCountedPtr<AsyncRunnableBase> Ptr;
};

/**
 * This class is used by AsyncMethod.
 * See the AsyncMethod class for more information.
 */
template <typename ResultType, typename ArgsType, typename OwnerType>
class AsyncRunnable : public AsyncRunnableBase {
 public:
  typedef ResultType (OwnerType::*Callback)(const ArgsType&);
  typedef AsyncResult<ResultType> AsyncResultType;

  AsyncRunnable(OwnerType* owner,
                Callback method,
                const ArgsType& arg,
                const AsyncResultType& result)
    : owner_(owner),
      method_(method),
      arg_(arg),
      result_(result) {
    fun_check_ptr(owner_);
  }

  void Run() override {
    AsyncRunnableBase::Ptr guard(this, false); // Auto release. (참조를 획득하지 않고, 해제만 해줌)
    try {
      result_.SetData(new ResultType((owner_->*method_)(arg_)));
    } catch (Exception& e) {
      result_.SetError(e);
    } catch (std::exception& e) {
      result_.SetError(e.what());
    } catch (...) {
      result_.SetError("unknown exception");
    }

    result_.Notify();
  }

 private:
  OwnerType* owner_;
  Callback method_;
  ArgsType arg_;
  AsyncResultType result_;
};

/**
 * This class is used by AsyncMethod.
 * See the AsyncMethod class for more information.
 */
template <typename ArgsType, typename OwnerType>
class AsyncRunnable<void, ArgsType, OwnerType> : public AsyncRunnableBase {
 public:
  typedef void (OwnerType::*Callback)(const ArgsType&);
  typedef AsyncResult<void> AsyncResultType;

  AsyncRunnable(OwnerType* owner,
                Callback method,
                const ArgsType& arg,
                const AsyncResultType& result)
    : owner_(owner),
      method_(method),
      arg_(arg),
      result_(result) {
    fun_check_ptr(owner_);
  }

  void Run() override {
    AsyncRunnableBase::Ptr guard(this, false);
    try {
      (owner_->*method_)(arg_);
    } catch (Exception& e) {
      result_.SetError(e);
    } catch (std::exception& e) {
      result_.SetError(e.what());
    } catch (...) {
      result_.SetError("unknown exception");
    }

    result_.Notify();
  }

 private:
  OwnerType* owner_;
  Callback method_;
  ArgsType arg_;
  AsyncResultType result_;
};

/**
 * This class is used by AsyncMethod.
 * See the AsyncMethod class for more information.
 */
template <typename ResultType, typename OwnerType>
class AsyncRunnable<ResultType, void, OwnerType> : public AsyncRunnableBase {
 public:
  typedef ResultType (OwnerType::*Callback)();
  typedef AsyncResult<ResultType> AsyncResultType;

  AsyncRunnable(OwnerType* owner, Callback method, const AsyncResultType& result)
    : owner_(owner),
      method_(method),
      result_(result) {
    fun_check_ptr(owner_);
  }

  void Run() override {
    AsyncRunnableBase::Ptr guard(this, false);
    try {
      result_.SetData(new ResultType((owner_->*method_)()));
    } catch (Exception& e) {
      result_.SetError(e);
    } catch (std::exception& e) {
      result_.SetError(e.what());
    } catch (...) {
      result_.SetError("unknown exception");
    }

    result_.Notify();
  }

 private:
  OwnerType* owner_;
  Callback method_;
  AsyncResultType result_;
};

/**
 * This class is used by AsyncMethod.
 * See the AsyncMethod class for more information.
 */
template <typename OwnerType>
class AsyncRunnable<void, void, OwnerType> : public AsyncRunnableBase {
 public:
  typedef void (OwnerType::*Callback)();
  typedef AsyncResult<void> AsyncResultType;

  AsyncRunnable(OwnerType* owner, Callback method, const AsyncResultType& result)
    : owner_(owner),
      method_(method),
      result_(result) {
    fun_check_ptr(owner_);
  }

  void Run() override {
    AsyncRunnableBase::Ptr guard(this, false);
    try {
      (owner_->*method_)();
    } catch (Exception& e) {
      result_.SetError(e);
    } catch (std::exception& e) {
      result_.SetError(e.what());
    } catch (...) {
      result_.SetError("unknown exception");
    }

    result_.Notify();
  }

 private:
  OwnerType* owner_;
  Callback method_;
  AsyncResultType result_;
};

} // namespace fun
