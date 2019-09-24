#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/unique_ptr.h"

namespace fun {
namespace tl {

/**
 * This is essentially a reference version of UniquePtr
 * Useful to force heap allocation of a value,
 * e.g. in a Map to give similar behaviour to IndirectArray
 */
template <typename T>
class UniqueObj
{
 public:
  inline UniqueObj(const UniqueObj& other)
    : object_(MakeUnique<T>(*other))
  {
  }

  inline UniqueObj(UniqueObj&& other)
    : object_(MakeUnique<T>(MoveTemp(*other)))
  {
  }

  template <typename... Args>
  inline explicit UniqueObj(Args&&... args)
    : object_(MakeUnique<T>(Forward<Args>(args)...))
  {
  }

  inline UniqueObj& operator = (const UniqueObj&) = delete;

  inline UniqueObj& operator = (UniqueObj&& other)
  {
    Swap(object_, other.object_);
    return *this;
  }

  template <typename OtherT>
  inline UniqueObj& operator = (OtherT&& other)
  {
    *object_ = Forward<OtherT>(other);
    return *this;
  }

  inline T& Get()
  {
    return *object_;
  }

  inline const T& Get() const
  {
    return *object_;
  }

  inline T* operator -> ()
  {
    return object_.Get();
  }

  inline const T* operator -> () const
  {
    return object_.Get();
  }

  inline T& operator * ()
  {
    return *object_;
  }

  inline const T& operator * () const
  {
    return *object_;
  }

  //inline friend Archive& operator & (Archive& ar, UniqueObj& obj)
  //{
  //  ar & *obj.object_;
  //  return ar;
  //}

 private:
  UniquePtr<T> object_;
};

} // namepace tl
} // namespace fun
