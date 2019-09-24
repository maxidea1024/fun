#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/unique_ptr.h"
#include "fun/base/serialization/archive.h"

namespace fun {

// TODO UniqueRef가 맞지 않을까??
/**
 * This is essentially a reference version of UniquePtr
 * Useful to force heap allocation of a value,
 * e.g. in a Map to give similar behaviour to IndirectArray
 */
template <typename T>
class UniqueObj {
 public:
  FUN_ALWAYS_INLINE UniqueObj(const UniqueObj& other)
      : object_(MakeUnique<T>(*other)) {}

  FUN_ALWAYS_INLINE UniqueObj(UniqueObj&& other)
      : object_(MakeUnique<T>(MoveTemp(*other))) {}

  template <typename... Args>
  FUN_ALWAYS_INLINE explicit UniqueObj(Args&&... args)
      : object_(MakeUnique<T>(Forward<Args>(args)...)) {}

  // Disable assignment.
  UniqueObj& operator=(const UniqueObj&) = delete;

  FUN_ALWAYS_INLINE UniqueObj& operator=(UniqueObj&& other) {
    Swap(object_, other.object_);
    return *this;
  }

  template <typename OtherType>
  FUN_ALWAYS_INLINE UniqueObj& operator=(OtherType&& other) {
    *object_ = Forward<OtherType>(other);
    return *this;
  }

  FUN_ALWAYS_INLINE T& Get() { return *object_; }

  FUN_ALWAYS_INLINE const T& Get() const { return *object_; }

  FUN_ALWAYS_INLINE T* operator->() { return object_.Get(); }

  FUN_ALWAYS_INLINE const T* operator->() const { return object_.Get(); }

  FUN_ALWAYS_INLINE T& operator*() { return *object_; }

  FUN_ALWAYS_INLINE const T& operator*() const { return *object_; }

  FUN_ALWAYS_INLINE friend Archive& operator&(Archive& ar, UniqueObj& obj) {
    ar&* obj.object_;
    return ar;
  }

 private:
  UniquePtr<T> object_;
};

}  // namespace fun
