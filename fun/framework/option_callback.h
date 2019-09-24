#pragma once

#include "fun/framework/framework.h"

namespace fun {
namespace framework {

/**
 * Base class for OptionCallback.
 */
class FUN_FRAMEWORK_API OptionCallbackBase {
 public:
  /** Invokes the callback member function. */
  virtual void Invoke(const String& name, const String& value) const = 0;

  /** Creates and returns a copy of the object. */
  virtual OptionCallbackBase* Clone() const = 0;

  /** Destroys the OptionCallbackBase. */
  virtual ~OptionCallbackBase();

 protected:
  OptionCallbackBase();
  OptionCallbackBase(const OptionCallbackBase&);
};

/**
 * This class is used as an argument to Option::Callback().
 *
 * It stores a pointer to an object and a pointer to a member
 * function of the object's class.
 */
template <typename C>
class OptionCallback : public OptionCallbackBase {
 public:
  typedef void (C::*Callback)(const String& name, const String& value);

  /** Creates the OptionCallback for the given object and member function. */
  OptionCallback(C* object, Callback method)
      : object_(fun_check_ptr(object)), method_(method) {}

  /** Creates an OptionCallback from another one. */
  OptionCallback(const OptionCallback& cb)
      : OptionCallbackBase(cb), object_(cb.object_), method_(cb.method_) {}

  /** Destroys the OptionCallback. */
  ~OptionCallback() {}

  OptionCallback& operator=(const OptionCallback& cb) {
    if (&cb != this) {
      object_ = cb.object_;
      method_ = cb.method_;
    }

    return *this;
  }

  void Invoke(const String& name, const String& value) const {
    (object_->*method_)(name, value);
  }

  OptionCallbackBase* Clone() const {
    return new OptionCallback(object_, method_);
  }

 private:
  OptionCallback();

  C* object_;
  Callback method_;
};

}  // namespace framework
}  // namespace fun
