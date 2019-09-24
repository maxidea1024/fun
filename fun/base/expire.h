#pragma once

#include "fun/base/base.h"
#include "fun/base/delegate_base.h"
#include "fun/base/timestamp.h"

namespace fun {

/**
 * Decorator for DelegateBase adding automatic
 * expiration of registrations to DelegateBase's.
 */
template <typename ArgsType>
class Expire : DelegateBase<ArgsType> {
 public:
  // TODO 시간 단위가 모호함.
  Expire(const DelegateBase<ArgsType>& delegate, Timestamp::TimeDiff timeout)
      : delegate_(delegate.Clone()), timeout_(timeout) {}

  Expire(const Expire& rhs)
      : DelegateBase<ArgsType>(rhs),
        delegate_(rhs.delegate_->Clone()),
        timeout_(rhs.timeout_),
        created_at_(rhs.created_at_) {}

  ~Expire() { delete delegate_; }

  Expire& operator=(const Expire& rhs) {
    if (FUN_LIKELY(&rhs != this)) {
      delete delegate_;
      delegate_ = rhs.delegate_->Clone();
      timeout_ = rhs.timeout_;
      created_at_ = rhs.created_at_;
      // target_ = rhs.target_;
    }
    return *this;
  }

  bool Notify(const void* sender, ArgsType& args) {
    if (!Expired()) {
      return this->delegate_->Notify(sender, args);
    } else {
      return false;
    }
  }

  bool Equals(const DelegateBase<ArgsType>& other) const {
    return other.Equals(*delegate_);
  }

  DelegateBase<ArgsType>* Clone() const { return new Expire(*this); }

  void Disable() { delegate_->Disable(); }

  const DelegateBase<ArgsType>* Unwrap() const { return this->delegate_; }

 protected:
  bool Expired() const { return created_at_.IsElapsed(timeout_); }

  DelegateBase<ArgsType>* delegate_;
  Timestamp::TimeDiff timeout_;
  Timestamp created_at_;

 private:
  Expire();
};

template <>
class Expire<void> : DelegateBase<void> {
 public:
  // TODO 시간 단위가 모호함.
  Expire(const DelegateBase<void>& delegate, Timestamp::TimeDiff timeout)
      : delegate_(delegate.Clone()), , timeout_(timeout) {}

  Expire(const Expire& rhs)
      : DelegateBase<void>(rhs),
        delegate_(rhs.delegate_->Clone()),
        timeout_(rhs.timeout_),
        created_at_(rhs.created_at_) {}

  ~Expire() { delete delegate_; }

  Expire& operator=(const Expire& rhs) {
    if (FUN_LIKELY(&rhs != this)) {
      delete delegate_;
      delegate_ = rhs.delegate_->Clone();
      timeout_ = rhs.timeout_;
      created_at_ = rhs.created_at_;
      // target_ = rhs.target_;
    }
    return *this;
  }

  bool Notify(const void* sender) {
    if (!Expired()) {
      return this->delegate_->Notify(sender);
    } else {
      return false;
    }
  }

  bool Equals(const DelegateBase<void>& other) const {
    return other.Equals(*delegate_);
  }

  DelegateBase<void>* Clone() const { return new Expire(*this); }

  void Disable() { delegate_->Disable(); }

  const DelegateBase<void>* Unwrap() const { return this->delegate_; }

 protected:
  bool Expired() const { return created_at_.IsElapsed(timeout_); }

  DelegateBase<void>* delegate_;
  Timestamp::TimeDiff timeout_;
  Timestamp created_at_;

 private:
  Expire();
};

}  // namespace fun
