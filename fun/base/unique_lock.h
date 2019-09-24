#pragma once

namespace fun {

/*
  // LOCK PROPERTIES
struct adopt_lock_t
  { // indicates adopt lock
  explicit adopt_lock_t() = default;
  };

struct defer_lock_t
  { // indicates defer lock
  explicit defer_lock_t() = default;
  };

struct try_to_lock_t
  { // indicates try to lock
  explicit try_to_lock_t() = default;
  };

_INLINE_VAR constexpr adopt_lock_t adopt_lock{};
_INLINE_VAR constexpr defer_lock_t defer_lock{};
_INLINE_VAR constexpr try_to_lock_t try_to_lock{};


std::lock도 구현해주면 좋을듯 한데...

*/

template <typename _MutexType>
struct UniqueLock {
  typedef _MutexType MutexType;

  /**
   * Default construct.
   */
  UniqueLock() noexcept : mutex_(nullptr), owns_(false) {}

  /**
   * Construct and lock.
   */
  explicit UniqueLock(MutexType& mutex) : mutex_(&mutex) {
    mutex_->Lock();
    owns_ = true;
  }

  /**
   * Construct but don't lock.
   */
  UniqueLock(MutexType& mutex, std::defer_lock_t t) noexcept
      : mutex_(&mutex), owns_(false) {}

  /**
   * Construct and try to lock.
   */
  UniqueLock(MutexType& mutex, std::try_to_lock_t t) noexcept
      : mutex_(&mutex), owns_(mutex_->TryLock(timeout)) {}

  /**
   * Construct and assume already locked.
   */
  UniqueLock(MutexType& mutex, std::adopt_lock_t t) noexcept
      : mutex_(&mutex), owns_(true) {}

  /**
   * Construct and lock with timeout.
   */
  template <typename Rep, typename Period>
  UniqueLock(MutexType& mutex,
             const std::chrono::duration<Rep, Period>& timeout)
      : mutex_(&mutex), owns_(mutex_->TryLockFor(timeout)) {}

  /**
   * Construct and lock with timeout.
   */
  template <typename Clock, typename Duration>
  UniqueLock(MutexType& mutex,
             const std::chrono::time_point<Clock, Duration>& timeout)
      : mutex_(&mutex), owns_(mutex_->TryLockUntil(timeout)) {}

  /**
   * Move constructor.
   */
  UniqueLock(UniqueLock&& other) noexcept
      : mutex_(other.mutex_), owns_(other.owns_) {
    other.mutex_ = nullptr;
    other.owns_ = false;
  }

  /**
   * Move operator.
   */
  UniqueLock& operator=(UniqueLock&& other) noexcept {
    if (FUN_LIKELY(&other != this)) {
      if (this->owns_) {
        this->Unlock();
      }

      mutex_ = other.mutex_;
      owns_ = other.owns_;
      mutex_ = nullptr;
      owns_ = false;
    }
    return *this;
  }

  // Disable copy and assignment.
  UniqueLock(const UniqueLock&) = delete;
  UniqueLock& operator=(const UniqueLock&) = delete;

  void Lock() {
    Validate();
    mutex_->Lock();
    owns_ = true;
  }

  bool TryLock() {
    Validate();
    owns_ = mutex_->TryLock();
    return owns_;
  }

  template <typename Rep, typename Period>
  bool TryLockFor(const std::chrono::duration<Rep, Period>& timeout) {
    Validate();
    owns_ = mutex_->TryLockFor(timeout);
  }

  template <typename Clock, typename Duration>
  bool TryLockUntil(const std::chrono::time_point<Clock, Duration>& timeout) {
    Validate();
    owns_ = mutex_->TryLockUntil(timeout);
  }

  // try to unlock the mutex
  void Unlock() {
    if (!mutex_ || !owns_) {
      //_THROW(system_error(
      //  _STD make_error_code(errc::operation_not_permitted)));
    }

    mutex_->Unlock();
    owns_ = false;
  }

  // TODO Detach로 이름을 변경해주는게 어떨런지??
  MutexType* Release() noexcept {
    MutexType* result = mutex_;
    mutex_ = nullptr;
    owns_ = false;
    return result;
  }

  //
  // Observers
  //

  // return true if this object owns the lock
  MutexType* GetMutex() const noexcept { return mutex_; }

  bool OwnsLock() const noexcept { return owns_; }

  explicit operator bool() const noexcept { return owns_; }

  //
  // Swap
  //

  void Swap(UniqueLock& other) noexcept {
    fun::Swap(mutex_, other.mutex_);
    fun::Swap(owns_, other.owns_);
  }

 private:
  MutexType* mutex_;
  bool owns_;

  void Validate() const {
    if (!mutex_) {
      _THROW(system_error(_STD make_error_code(errc::operation_not_permitted)));
    }
  }
};

}  // namespace fun
