#pragma once

#include "fun/base/base.h"
#include "fun/base/mutex.h"
#include "fun/base/event.h"
#include "fun/base/exception.h"
#include "fun/base/ref_counted.h"

#include <algorithm>

namespace fun {

/**
 * This class holds the result of an asynchronous method
 * invocation. It is used to pass the result from the
 * execution thread back to the invocation thread.
 * The class uses reference counting for memory management.
 * Do not use this class directly, use AsyncResult instead.
 */
template <typename ResultType>
class AsyncResultHolder : public RefCountedObject {
 public:
  AsyncResultHolder()
    : data_(nullptr),
      exception_(nullptr),
      event_(EventResetType::Manual) {
    //event_ = EventPool::DefaultPool().Take();
  }

  /**
   * Returns a reference to the actual result.
   */
  ResultType& GetDataRef() {
    fun_check_ptr(data_);
    return *data_;
  }

  /**
   * Sets the acutal value.
   */
  void SetData(ResultType* data) {
    delete data_;
    data_ = data;
  }

  /**
   * Pauses the caller until the result becomes available.
   */
  void Wait() {
    event_.Wait();
  }

  /**
   * Waits up to the specified interval for the result to
   * become available. Returns true if the result became
   * available, false otherwise.
   */
  bool TryWait(int32 milliseconds) {
    return event_.TryWait(milliseconds);
  }

  /**
   * Waits up to the specified interval for the result to
   * become available. Throws a TimeoutException if the
   * result did not became available.
   */
  void Wait(int32 milliseconds) {
    event_.Wait(milliseconds);
  }

  /**
   * Notifies the invoking thread that the result became available.
   */
  void Notify() {
    event_.Notify();
  }

  /**
   * Returns true if the active method failed (and threw an exception).
   * Information about the exception can be obtained by calling GetError().
   */
  bool Failed() const {
    return exception_ != nullptr;
  }

  /**
   * If the active method threw an exception, a textual representation
   * of the exception is returned. An empty string is returned if the
   * active method completed successfully.
   */
  String GetError() const {
    if (exception_) {
      return exception_->GetMessage();
    }
    else {
      return String();
    }
  }

  /**
   * If the active method threw an exception, a clone of the exception
   * object is returned, otherwise null.
   */
  Exception* GetException() const {
    return exception_;
  }

  /**
   * Sets the exception.
   */
  void SetError(const Exception& e) {
    delete exception_;
    exception_ = e.Clone();
  }

  /**
   * Sets the exception.
   */
  void SetError(const String& error) {
    SetError(UnhandledException(error));
  }

 protected:
  virtual ~AsyncResultHolder() {
    delete data_;
    delete exception_;

    //EventPool::DefaultPool().ReturnToPool(event_);
    //event_ = nullptr;
  }

 private:
  ResultType* data_;
  Exception* exception_;
  Event event_;
  //Event* event_;
};


template <>
class AsyncResultHolder<void> : public RefCountedObject {
 public:
  /**
   * Creates an ActiveResultHolder.
   */
  AsyncResultHolder()
    : exception_(nullptr)
    , event_(EventResetType::Manual) {
    //event_ = EventPool<EventResetType::Manual>::GetOrCreate();
  }

  /**
   * Pauses the caller until the result becomes available.
   */
  void Wait() {
    event_.Wait();
    //event_->Wait();
  }

  /**
   * Waits up to the specified interval for the result to
   * become available. Returns true if the result became
   * available, false otherwise.
   */
  bool TryWait(int32 milliseconds) {
    return event_.TryWait(milliseconds);
    //return event_->TryWait(milliseconds);
  }

  /**
   * Waits up to the specified interval for the result to
   * become available. Throws a TimeoutException if the
   * result did not became available.
   */
  void Wait(int32 milliseconds) {
    event_.Wait(milliseconds);
    //event_->Wait(milliseconds);
  }

  /**
   * Notifies the invoking thread that the result became available.
   */
  void Notify() {
    event_.Set();
    //event_->Set();
  }

  /**
   * Returns true if the active method failed (and threw an exception).
   * Information about the exception can be obtained by calling GetError().
   */
  bool Failed() const {
    return exception_ != nullptr;
  }

  /**
   * If the active method threw an exception, a textual representation
   * of the exception is returned. An empty string is returned if the
   * active method completed successfully.
   */
  String GetError() const {
    if (exception_) {
      return exception_->GetMessage();
    } else {
      return String();
    }
  }

  /**
   * If the active method threw an exception, a clone of the exception
   * object is returned, otherwise null.
   */
  Exception* GetException() const {
    return exception_;
  }

  /**
   * Sets the exception.
   */
  void SetError(const Exception& e) {
    delete exception_;
    exception_ = e.Clone();
  }

  /**
   * Sets the exception.
   */
  void SetError(const String& error) {
    SetError(UnhandledException(error));
  }

 protected:
  virtual ~AsyncResultHolder() {
    delete exception_;
    //EventPool::DefaultPool().ReturnToPool(event_);
    //event_ = nullptr;
  }

 private:
  Exception* exception_;
  Event event_;
  //Event* event_;
};


/**
 * This class holds the result of an asynchronous method
 * invocation (see class AsyncMethod). It is used to pass the
 * result from the execution thread back to the invocation thread.
 */
template <typename _ResultType>
class AsyncResult {
 public:
  typedef _ResultType ResultType;
  typedef AsyncResultHolder<ResultType> AsyncResultHolderType;

  /**
   * Creates the active result. For internal use only.
   */
  AsyncResult(AsyncResultHolderType* holder) FUN_INTERNAL_USE_ONLY
    : holder_(holder) {
    fun_check_ptr(holder_);

    // 객체 생성시 참조 카운터가 0이므로, 여기서 1회 올려주어야함.
    holder_->AddRef();
  }

  /**
   * Copy constructor.
   */
  AsyncResult(const AsyncResult& rhs)
    : holder_(rhs.holder_) {
    //if (holder_) {
    //  holder_->AddRef();
    //}
    fun_check_ptr(holder_);
    holder_->AddRef();
  }

  /**
   * Destroys the result.
   */
  ~AsyncResult() {
    if (holder_) {
      holder_->Release();
    }
  }

  /**
   * Assignment operator.
   */
  AsyncResult& operator = (const AsyncResult& rhs) {
    AsyncResult tmp(rhs);
    Swap(tmp);
    return *this;
  }

  void Swap(AsyncResult& other) {
    fun::Swap(holder_, other.holder_);
  }

  /**
   * Returns a reference to the result data.
   */
  ResultType& Data() const {
    return holder_->Data();
  }

  void SetData(ResultType* data) {
    holder_->SetData(data);
  }

  /**
   * Pauses the caller until the result becomes available.
   */
  void Wait() {
    holder_->Wait();
  }

  /**
   * Waits up to the specified interval for the result to
   * become available. Returns true if the result became
   * available, false otherwise.
   */
  bool TryWait(int32 milliseconds) {
    return holder_->TryWait(milliseconds);
  }

  /**
   * Waits up to the specified interval for the result to
   * become available. Throws a TimeoutException if the
   * result did not became available.
   */
  void Wait(int32 milliseconds) {
    holder_->Wait(milliseconds);
  }

  /**
   * Returns true if a result is available.
   */
  bool Available() const {
    return holder_->TryWait(0);
  }

  /**
   * Returns true if the active method failed (and threw an exception).
   * Information about the exception can be obtained by calling GetError().
   */
  bool Failed() const {
    return holder_->Failed();
  }

  /**
   * If the active method threw an exception, a textual representation
   * of the exception is returned. An empty string is returned if the
   * active method completed successfully.
   */
  String GetError() const {
    return holder_->GetError();
  }

  /**
   * If the active method threw an exception, a clone of the exception
   * object is returned, otherwise null.
   */
  Exception* GetException() const {
    return holder_->GetException();
  }

  /**
   * Notifies the invoking thread that the result became available.
   * For internal use only.
   */
  void Notify() FUN_INTERNAL_USE_ONLY {
    holder_->Notify();
  }

  /**
   * Returns a non-const reference to the result data. For internal
   * use only.
   */
  ResultType& GetDataRef() FUN_INTERNAL_USE_ONLY {
    return holder_->GetDataRef();
  }

  /**
   * Sets the failed flag and the exception message.
   */
  void SetError(const String& error) {
    holder_->SetError(error);
  }

  /**
   * Sets the failed flag and the exception message.
   */
  void SetError(const Exception& e) {
    holder_->SetError(e);
  }

 private:
  AsyncResult();

  AsyncResultHolderType* holder_;
  //RefCountedPtr<AsyncResultHolderType> holder_;
};


/**
 * This class holds the result of an asynchronous method
 * invocation (see class AsyncMethod). It is used to pass the
 * result from the execution thread back to the invocation thread.
 */
template <>
class AsyncResult<void> {
 public:
  typedef AsyncResultHolder<void> AsyncResultHolderType;

  /**
   * Creates the active result. For internal use only.
   */
  AsyncResult(AsyncResultHolderType* holder) FUN_INTERNAL_USE_ONLY
    : holder_(holder) {
    fun_check_ptr(holder_);
  }

  /**
   * Copy constructor.
   */
  AsyncResult(const AsyncResult& rhs)
    : holder_(rhs.holder_) {
    //if (holder_) {
    //  holder_->AddRef();
    //}
    fun_check_ptr(holder_);
    holder_->AddRef();
  }

  /**
   * Destroys the result.
   */
  ~AsyncResult() {
    holder_->Release();
  }

  /**
   * Assignment operator.
   */
  AsyncResult& operator = (const AsyncResult& rhs) {
    AsyncResult tmp(rhs);
    Swap(tmp);
    return *this;
  }

  void Swap(AsyncResult& other) {
    fun::Swap(holder_, other.holder_);
  }

  /**
   * Pauses the caller until the result becomes available.
   */
  void Wait() {
    holder_->Wait();
  }

  /**
   * Waits up to the specified interval for the result to
   * become available. Returns true if the result became
   * available, false otherwise.
   */
  bool TryWait(int32 milliseconds) {
    return holder_->TryWait(milliseconds);
  }

  /**
   * Waits up to the specified interval for the result to
   * become available. Throws a TimeoutException if the
   * result did not became available.
   */
  void Wait(int32 milliseconds) {
    holder_->Wait(milliseconds);
  }

  /**
   * Returns true if a result is available.
   */
  bool Available() const {
    return holder_->TryWait(0);
  }

  /**
   * Returns true if the active method failed (and threw an exception).
   * Information about the exception can be obtained by calling GetError().
   */
  bool Failed() const {
    return holder_->Failed();
  }

  /**
   * If the active method threw an exception, a textual representation
   * of the exception is returned. An empty string is returned if the
   * active method completed successfully.
   */
  String GetError() const {
    return holder_->GetError();
  }

  /**
   * If the active method threw an exception, a clone of the exception
   * object is returned, otherwise null.
   */
  Exception* GetException() const {
    return holder_->GetException();
  }

  /**
   * Notifies the invoking thread that the result became available.
   * For internal use only.
   */
  void Notify() FUN_INTERNAL_USE_ONLY {
    holder_->Notify();
  }

  /**
   * Sets the failed flag and the exception message.
   */
  void SetError(const String& error) {
    holder_->SetError(error);
  }

  /**
   * Sets the failed flag and the exception message.
   */
  void SetError(const Exception& e) {
    holder_->SetError(e);
  }

 private:
  AsyncResult();

  AsyncResultHolderType* holder_;
  //RefCountedPtr<AsyncResultHolderType> holder_;
};

} // namespace fun
