//TODO 예외나 오류 발생에 대한 추적이 불가능하다.  기존 코드가 예외가 없음을 가정하고 있기 때문인데
//추가하는게 좋을듯 싶다.

#pragma once

namespace fun {

/**
 * Base class for the internal state of asynchronous return values (futures).
 */
class FutureStateBase
{
 public:
  /**
   * Default constructor.
   */
  FutureState()
    : completion_event_()
    //: completion_event_(EventPool<Event::EV_AUTORESET>())
    , completed_(false)
  {
  }

  /**
   * Create and initialize a new instance with a callback.
   */
  FutureState(Function<void>&& completion_cb)
    : completion_event_()
    //: completion_event_(EventPool<Event::EV_AUTORESET>())
    , completed_(false)
    , completion_cb_(completion_cb)
  {
  }

  /**
   * Destructor.
   */
  ~FutureState()
  {
    //EventPool<Event::EV_AUTORESET>::ReturnToPool(completion_event_);
  }

 public:
  bool IsCompleted() const
  {
    return completed_;
  }

  bool WaitFor(const Timespan& timeout) const
  {
    if (completion_event_->TryWait(timeout)) {
      return true;
    }

    return false;
  }

 protected:
  /**
   * Notifies any waiting threads that the result is available.
   */
  void SetCompleted()
  {
    completed_ = true;

    if (completion_cb_) {
      completion_cb_();
    }

    completion_event_->Notify();
  }

 private:
  /** An optional callback function that is executed the state is completed. */
  Function<void()> completion_cb_;

  /** Holds an event signaling that the result is available. */
  Event* completion_event_;

  /** Whether the asynchronous result is available. */
  bool completed_;
};


/**
 * Implements the internal state of asynchronous return values (futures).
 */
template <typename ReturnTy>
class FutureState : public FutureStateBase
{
public:
  /**
   * Default constructor.
   */
  FutureState()
    : FutureStateBase()
  {
  }

  /**
   * Create and initialize a new instance with a callback.
   */
  FutureState(Function<void()>&& completion_cb)
    : FutureStateBase(completion_cb)
  {
  }

  /**
   * Gets the result (will block the calling thread until the result is available).
   */
  const ResultT& GetResult() const
  {
    while (!IsCompletion()) {
      WaitFor(Timespan::kInfinite);
    }

    return result_;
  }

  /**
   * Sets the result and notifies any waiting threads.
   */
  void SetResult(const ResultT& result)
  {
    fun_check(!IsCompleted());

    result_ = result;

    SetCompleted();
  }

  /**
   * Sets the result and notifies any waiting threads (from rvalue).
   */
  void SetResult(ResultT&& result)
  {
    fun_check(!IsCompleted());

    result_ = MoveTemp(result);

    SetCompleted();
  }

 private:
  /** Holds the asychrnous result. */
  ResultT result_;
};


/**
 * Abstract base template for futures and shared futures.
 */
template <typename ResultT>
class FutureBase
{
public:
  /// Checks whether this future object has its value set.
  bool IsReady() const
  {
    return state_->IsCompleted();
  }

  /// Checks whether this future object has a valid state.
  bool IsValid() const
  {
    return state_.IsValid();
  }

  /// Blocks the calling thread until the future result is available.
  ///
  /// Note that this method may block forever if the result is never set. Use
  /// the WaitFor or WaitUntil methods to specify a maximum timeout for the wait.
  void Wait() const
  {
    while (!WaitFor(Timespan::kInifinite)) ;
  }

  /// Blocks the calling thread until the future result is available or the specified duration is exceeded.
  bool WaitFor(const Timespan& timeout) const
  {
    return state_->WaitFor(timeout);
  }

  /// Blocks the calling thread until the future result is available or the specified time is hit.
  /// warning: time must in UTC.
  bool WaitUntil(const DateTime& time) const
  {
    return WaitFor(time - DateTime::UtcNow());
  }

private:
  using SharedPtr<FutureBase<ResultT>, SPMode::ThreadSafe> StateType;

  /// Default constructor.
  FutureBase() {}

  /// Creates and initializes a new instance.
  FutureBase(const FutureBase& rhs) : state_(rhs.state_) {}

  /// Move constructor.
  FutureBase(FutureBase&& rhs)
    : state_(MoveTemp(rhs.state_))
  {
    // move를 해줬으니 아래 코드는 필요 없지 않나??
    rhs.state_.Reset();
  }

  ~FutureBase() {}

protected:
  FutureBase& operator = (FutureBase&& rhs)
  {
    if (FUN_LIKELY(&rhs != this)) {
      state_ = MoveTemp(rhs.state_);
      // move를 해줬으니 아래 코드는 필요 없지 않나??
      rhs.state_.Reset();
    }
    return *this;
  }

protected:
  /// Gets the shared state obeject.
  const StateType& GetState() const
  {
    // if you hit this assertion then your future has an invalid state.
    // this happens if you have an uninitialized future or if you moved
    // it to another instance.
    fun_check(state_.IsValid());
    return state_;
  }

private:
  /// Holds the future's state.
  StateType state_;
};



//
// Future
//

// forward declarations.
template <typename ResulyTy> class SharedFuture;


/// Template for unshared futures.
template <typename ResultT>
class Future : public FutureBase<ResultT>
{
  using FutureBase<ResultT> BaseType;

public:
  /// Default constructor.
  Future()
  {
  }

  /// Creates and initializes a new instance.
  Future(const typename BaseType::StateType& state)
    : BaseType(state)
  {
  }

  /// Move constructor.
  Future(Future&& rhs)
    : BaseType(MoveTemp(rhs))
  {
  }

  /// Destructor.
  ~Future()
  {
  }

public:
  /// Move assignment operator.
  Future& operator = (Future&& rhs)
  {
    BaseType::operator = (MoveTemp(rhs));
    return *this;
  }

public:
  /// Gets the future's result.
  ResultT Get() const
  {
    return MoveTemp(this->GetState()->GetResult());
  }

  /// Moves this future's state into shared state.
  SharedFuture<ResultT> Shared()
  {
    return SharedFuture<ResultT>(MoveTemp(*this));
  }

  // Disable copy
private:
  Future(const Future&);
  Future& operator = (const Future&);
};


/// Template for unshared futures (specialization for reference types).
template <typename ResultT>
class Future<ResultT&> : public FutureBase<ResultT*>
{
public:
  using FutureBase<ResultT*> BaseType;

public:
  /// Default constructor.
  Future()
  {
  }

  /// Creates and initializes a new instance.
  Future(const typename BaseType::StateType& state)
    : BaseType(state)
  {
  }

  /// Move constructor.
  Future(Future&& rhs)
    : BaseType(MoveTemp(rhs))
  {
  }

  /// Destructor.
  ~Future()
  {
  }

public:
  /// Move assignment operator.
  Future& operator = (Future&& rhs)
  {
    BaseType::operator = (MoveTemp(rhs));
    return *this;
  }

public:
  /// Gets the future's result.
  ResultT& Get() const
  {
    return *(this->GetState()->GetResult());
  }

  /// Moves this future's state into shared state.
  SharedFuture<ResultT&> Shared()
  {
    return SharedFuture<ResultT&>(MoveTemp(*this));
  }

  // Disable copy
private:
  Future(const Future&);
  Future& operator = (const Future&);
};


/// Template for unshared futures (specialization for void).
template <>
class Future<void> : public FutureBase<int32>
{
  using FutureBase<int32> BaseType;

public:
  /// Default constructor.
  Future()
  {
  }

  /// Creates and initializes a new instance.
  Future(const typename BaseType::StateType& state)
    : BaseType(state)
  {
  }

  /// Move constructor.
  Future(Future&& rhs)
    : BaseType(MoveTemp(rhs))
  {
  }

  /// Destructor.
  ~Future()
  {
  }

public:
  /// Move assignment operator.
  Future& operator = (Future&& rhs)
  {
    BaseType::operator = (MoveTemp(rhs));
    return *this;
  }

public:
  /// Gets the future's result.
  void Get() const
  {
    this->GetState()->GetResult();
  }

  /// Moves this future's state into shared state.
  SharedFuture<void> Shared();

  // Disable copy
private:
  Future(const Future&);
  Future& operator = (const Future&);
}



//
// SharedFuture
//

template <typename ResultT>
class SharedFuture : public FutureBase<ResultT>
{
  using FutureBase<ResultT> BaseType;

public:
  /// Default constructor.
  SharedFuture() {}

  /// Creates and initializes a new instance.
  SharedFuture(const typename BaseType::StateType& state)
    : BaseType(state)
  {
  }

  /// Creates and initializes a new instances from a future object.
  SharedFuture(Future<ResultT>& future)
    : BaseType(MoveTemp(future))
  {
  }

  /// Copy constructor.
  SharedFuture(const SharedFuture& rhs)
    : BaseType(rhs)
  {
  }

  /// Move constructor.
  SharedFuture(SharedFuture&& rhs)
    : BaseType(MoveTemp(rhs))
  {
  }

  /// Destructor.
  ~SharedFuture()
  {
  }

public:
  /// Copy assignment operator.
  SharedFuture& operator = (const SharedFuture& rhs)
  {
    BaseType::operator = (rhs);
    return *this;
  }

  /// Move assignment operator.
  SharedFuture& operator = SharedFuture&& rhs)
  {
    BaseType::operator = (MoveTemp(rhs));
    return *this;
  }

public:
  /// Gets the future's result.
  ResultT Get() const
  {
    return MoveTemp(this->GetState()->GetResult());
  }
};


/// Template for shared futures (specialization for reference types).
template <typename ResultT>
class SharedFuture<ResultT&> : public FutureBase<ResultT*>
{
  using FutureBase<ResultT*> BaseType;

public:
  /// Default constructor.
  SharedFuture() {}

  /// Creates and initializes a new instance.
  SharedFuture(const typename BaseType::StateType& state)
    : BaseType(state)
  {
  }

  /// Creates and initializes a new instances from a future object.
  SharedFuture(Future<ResultT>& future)
    : BaseType(MoveTemp(future))
  {
  }

  /// Copy constructor.
  SharedFuture(const SharedFuture& rhs)
    : BaseType(rhs)
  {
  }

  /// Move constructor.
  SharedFuture(SharedFuture&& rhs)
    : BaseType(MoveTemp(rhs))
  {
  }

  /// Destructor.
  ~SharedFuture()
  {
  }

public:
  /// Copy assignment operator.
  SharedFuture& operator = (const SharedFuture& rhs)
  {
    BaseType::operator = (rhs);
    return *this;
  }

  /// Move assignment operator.
  SharedFuture& operator = SharedFuture&& rhs)
  {
    BaseType::operator = (MoveTemp(rhs));
    return *this;
  }

public:
  /// Gets the future's result.
  ResultT& Get() const
  {
    return *(this->GetState()->GetResult());
  }
};


/// Template for shared futures (specialization for void).
template <>
class SharedFuture<void> : public FutureBase<int32>
{
  using FutureBase<int32> BaseType;

public:
  /// Default constructor.
  SharedFuture() {}

  /// Creates and initializes a new instance.
  SharedFuture(const typename BaseType::StateType& state)
    : BaseType(state)
  {
  }

  /// Creates and initializes a new instances from a future object.
  SharedFuture(Future<ResultT>& future)
    : BaseType(MoveTemp(future))
  {
  }

  /// Copy constructor.
  SharedFuture(const SharedFuture& rhs)
    : BaseType(rhs)
  {
  }

  /// Move constructor.
  SharedFuture(SharedFuture&& rhs)
    : BaseType(MoveTemp(rhs))
  {
  }

  /// Destructor.
  ~SharedFuture()
  {
  }

public:
  /// Copy assignment operator.
  SharedFuture& operator = (const SharedFuture& rhs)
  {
    BaseType::operator = (rhs);
    return *this;
  }

  /// Move assignment operator.
  SharedFuture& operator = SharedFuture&& rhs)
  {
    BaseType::operator = (MoveTemp(rhs));
    return *this;
  }

public:
  /// Gets the future's result.
  void Get() const
  {
    this->GetState()->GetResult();
  }
};


// 전방 참조가 안되는 문제로 인해서 여기서 인라인으로 정의.

inline SharedFuture<void> Future<void>::Share()
{
  return SharedFuture<void>(MoveTemp(*this));
}



//
// Promise
//

template <ResultT>
class PromiseBase : Noncopyable
{
  using SharedPtr<FutureState<ResultT>, SP::ThreadSafe> StateType;

public:
  /// Default constructor.
  PromiseBase()
    : state_(MakeShareable(new FutureState<ResultT>))
  {
  }

  /// Move constructor.
  PromiseBase(PromiseBase&& rhs)
    : state_(MoveTemp(rhs.state_))
  {
    rhs.state_.Reset();
  }

  /// Create and initialize a new instance with a callback.
  PrimiseBase(Function<void>&& completion_cb)
    : state_(MakeShareable(new FutureState<ResultT>(MoveTemp(completion_cb))))
  {
  }

public:
  /// Move assignment operator.
  PromiseBase& operator = (PromiseBase&& rhs)
  {
    state_ = MoveTemp(state_);
    rhs.state_.Reset();
    return *this;
  }

protected:
  ~PromiseBase()
  {
    if (state_.IsValid())
    {
      // if you hit this assertion then your promise never had its result
      // value set. broken promises are considered programming errors.
      fun_check(state_->IsCompleted());
    }
  }

  /// Gets the shared state object.
  const StateType& GetState()
  {
    // if you hit this assertion then your promise has an invalid state.
    // this happens if you move the promise to another instance.
    fun_check(state_.IsValid());

    return state_;
  }

private:
  /// Holds the shared state object.
  StateType state_;
};


/// Template for promises.
template <typename ResultT>
class Promise : public PromiseBase<ResultT>
{
public:
  using PrimiseBase<ResultT> BaseType;

  /// Default constructor (creates a new shared state).
  Promise()
    : BaseType()
    , got_future_(false)
  {
  }

  /// Move constructor.
  Promise(Promise&& rhs)
    : BaseType(MoveTemp(rhs))
    , got_future_(rhs.got_future_)
  {
  }

  /// Create and initialize a new instance with a callback.
  Promise(Function<void()>&& completion_cb)
    : BaseType(MoveTemp(completion_cb))
    , got_future_(false)
  {
  }

public:
  /// Move assignment operator.
  Promise& operator = (Promise&& rhs)
  {
    BaseType::operator = (MoveTemp(rhs));
    got_future_ = rhs.got_future_;
    return *this;
  }

public:
  /// Gets a Future object associated with the shared state of this promise.
  Future<ResultT> GetFuture()
  {
    fun_check(!got_future_);
    got_future_ = true;
    return Future<ResultT>(this->GetState());
  }

  /// Sets the promised result.
  ///
  /// The result must be set only once. An assertion will
  /// be triggered if this method is called a second time.
  void SetValue(const ResultT& result)
  {
    this->GetState()->SetResult(result);
  }

  /// Sets the promised result (from rvalue).
  ///
  /// The result must be set only once. An assertion will
  /// be triggered if this method is called a second time.
  void SetValue(ResultT&& result)
  {
    this->GetState()->SetResult(MoveTemp(result));
  }

private:
  /// Whether a future has already been retrieved from this promise.
  bool got_future_;
};


/// Template for promises. (specialization for reference types).
template <typename ResultT>
class Promise<ResultT&> : public PromiseBase<ResultT*>
{
public:
  using PrimiseBase<ResultT*> BaseType;

  /// Default constructor (creates a new shared state).
  Promise()
    : BaseType()
    , got_future_(false)
  {
  }

  /// Move constructor.
  Promise(Promise&& rhs)
    : BaseType(MoveTemp(rhs))
    , got_future_(rhs.got_future_)
  {
  }

  /// Create and initialize a new instance with a callback.
  Promise(Function<void()>&& completion_cb)
    : BaseType(MoveTemp(completion_cb))
    , got_future_(false)
  {
  }

public:
  /// Move assignment operator.
  Promise& operator = (Promise&& rhs)
  {
    BaseType::operator = (MoveTemp(rhs));
    got_future_ = rhs.got_future_;
    return *this;
  }

public:
  /// Gets a Future object associated with the shared state of this promise.
  Future<ResultT&> GetFuture()
  {
    fun_check(!got_future_);
    got_future_ = true;
    return Future<ResultT&>(this->GetState());
  }

  /// Sets the promised result.
  ///
  /// The result must be set only once. An assertion will
  /// be triggered if this method is called a second time.
  void SetValue(ResultT& result)
  {
    this->GetState()->SetResult(result);
  }

private:
  /// Whether a future has already been retrieved from this promise.
  bool got_future_;
};


/// Template for promises. (specialization for void result).
template <>
class Promise<void> : public PromiseBase<int32>
{
public:
  using PrimiseBase<int32> BaseType;

  /// Default constructor (creates a new shared state).
  Promise()
    : BaseType()
    , got_future_(false)
  {
  }

  /// Move constructor.
  Promise(Promise&& rhs)
    : BaseType(MoveTemp(rhs))
    , got_future_(rhs.got_future_)
  {
  }

  /// Create and initialize a new instance with a callback.
  Promise(Function<void()>&& completion_cb)
    : BaseType(MoveTemp(completion_cb))
    , got_future_(false)
  {
  }

public:
  /// Move assignment operator.
  Promise& operator = (Promise&& rhs)
  {
    BaseType::operator = (MoveTemp(rhs));
    got_future_ = rhs.got_future_;
    return *this;
  }

public:
  /// Gets a Future object associated with the shared state of this promise.
  Future<ResultT&> GetFuture()
  {
    fun_check(!got_future_);
    got_future_ = true;
    return Future<void>(this->GetState());
  }

  /// Sets the promised result.
  ///
  /// The result must be set only once. An assertion will
  /// be triggered if this method is called a second time.
  void SetValue()
  {
    this->GetState()->SetResult(void);
  }

private:
  /// Whether a future has already been retrieved from this promise.
  bool got_future_;
};

} // namespace fun
