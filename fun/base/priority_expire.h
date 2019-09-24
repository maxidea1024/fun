#pragma once

#include "fun/base/base.h"
#include "fun/base/timestamp.h"
#include "fun/base/priority_delegate_base.h"

namespace fun {

/**
 * Decorator for PriorityDelegateBase adding automatic
 * expiring of registrations to PriorityDelegateBase.
 */
template <typename ArgsType>
class PriorityExpire : public PriorityDelegateBase<ArgsType> {
 public:
  PriorityExpire(const PriorityDelegateBase<ArgsType>& p, Timestamp::TimeDiff expire_msec)
    : PriorityDelegateBase<ArgsType>(p),
      delegate_(static_cast<PriorityDelegateBase<ArgsType>*>(p.Clone())),
      expire_(expire_msec * 1000) {}

  PriorityExpire(const PriorityExpire& expire)
    : PriorityDelegateBase<ArgsType>(expire),
      delegate_(static_cast<PriorityDelegateBase<ArgsType>*>(expire.delegate_->Clone())),
      expire_(expire.expire_),
      creation_time_(expire.creation_time_) {}

  ~PriorityExpire() {
    delete delegate_;
  }

  PriorityExpire& operator = (const PriorityExpire& expire) {
    if (FUN_LIKELY(&expire != this)) {
      delete this->delegate_;
      this->target_ = expire.target_;
      this->delegate_ = expire.delegate_->Clone();
      this->expire_ = expire.expire_;
      this->creation_time_ = expire.creation_time_;
    }
    return *this;
  }

  bool Notify(const void* sender, ArgsType& arguments) {
    if (!Expired()) {
      return this->delegate_->Notify(sender, arguments);
    } else {
      return false;
    }
  }

  bool Equals(const DelegateBase<ArgsType>& other) const {
    return other.Equals(*delegate_);
  }

  PriorityDelegateBase<ArgsType>* Clone() const {
    return new PriorityExpire(*this);
  }

  void Disable() {
    delegate_->Disable();
  }

  const PriorityDelegateBase<ArgsType>* Unwrap() const {
    return this->delegate_;
  }

 protected:
  bool Expired() const {
    return creation_time_.IsElapsed(expire_);
  }

  PriorityDelegateBase<ArgsType>* delegate_;
  Timestamp::TimeDiff expire_;
  Timestamp creation_time_;

 private:
  PriorityExpire();
};


/**
 * Decorator for PriorityDelegateBase adding automatic
 * expiring of registrations to PriorityDelegateBase.
 */
template <>
class PriorityExpire<void> : public PriorityDelegateBase<void>
{
 public:
  PriorityExpire(const PriorityDelegateBase<void>& p, Timestamp::TimeDiff expire_msec)
    : PriorityDelegateBase<void>(p),
      delegate_(static_cast<PriorityDelegateBase<void>*>(p.Clone())),
      expire_(expire_msec * 1000) {}

  PriorityExpire(const PriorityExpire& expire)
    : PriorityDelegateBase<void>(expire),
      delegate_(static_cast<PriorityDelegateBase<void>*>(expire.delegate_->Clone())),
      expire_(expire.expire_),
      creation_time_(expire.creation_time_) {}

  ~PriorityExpire() {
    delete delegate_;
  }

  PriorityExpire& operator = (const PriorityExpire& expire) {
    if (FUN_LIKELY(&expire != this)) {
      delete this->delegate_;
      this->delegate_ = static_cast<PriorityDelegateBase<void>*>(expire.delegate_->Clone());
      this->expire_ = expire.expire_;
      this->creation_time_ = expire.creation_time_;
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

  PriorityDelegateBase<void>* Clone() const {
    return new PriorityExpire(*this);
  }

  void Disable() {
    delegate_->Disable();
  }

  const PriorityDelegateBase<void>* Unwrap() const {
    return this->delegate_;
  }

 protected:
  bool Expired() const {
    return creation_time_.IsElapsed(expire_);
  }

  PriorityDelegateBase<void>* delegate_;
  Timestamp::TimeDiff expire_;
  Timestamp creation_time_;

 private:
  PriorityExpire();
};

} // namespace fun
