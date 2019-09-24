#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/functional.h"
#include "fun/base/delegate/delegate_config.h"

namespace fun {

class FObject;

/**
 * Class representing an handle to a delegate.
 */
class DelegateHandle
{
 public:
  enum EGenerateNewHandleType {
    GenerateNewHandle
  };

  DelegateHandle()
    : id_(0)
  {}

  explicit DelegateHandle(EGenerateNewHandleType)
    : id_(GenerateNewId())
  {}

  bool IsValid() const
  {
    return id_ != 0;
  }

  void Reset()
  {
    id_ = 0;
  }

 private:
  friend bool operator == (const DelegateHandle& lhs, const DelegateHandle& rhs)
  {
    return lhs.id_ == rhs.id_;
  }

  friend bool operator != (const DelegateHandle& lhs, const DelegateHandle& rhs)
  {
    return lhs.id_ != rhs.id_;
  }

  friend FUN_ALWAYS_INLINE uint32 HashOf(const DelegateHandle& handle)
  {
    return HashOf(handle.id_);
  }

  /**
   * Generates a new id_ for use the delegate handle.
   *
   * @return A unique id_ for the delegate.
   */
  static FUN_BASE_API uint64 GenerateNewId();

  uint64 id_;
};


/**
 * Interface for delegate instances.
 */
class IDelegateInstance
{
 public:
#if FUN_USE_DELEGATE_TRYGETBOUNDFUNCTIONNAME
  /**
   * Tries to return the name of a bound function.  Returns "" if the delegate is unbound or
   * a binding name is unavailable.
   *
   * Note: Only intended to be used to aid debugging of delegates.
   *
   * @return The name of the bound function, "" if no name was available.
   */
  virtual String TryGetBoundFunctionName() const = 0;
#endif

  /**
   * Returns the FObject that this delegate instance is bound to.
   *
   * @return Pointer to the FObject, or nullptr if not bound to a FObject.
   */
  virtual FObject* GetFObject() const = 0;

  /**
   * Returns true if this delegate is bound to the specified UserObject,
   *
   * Deprecated.
   *
   * @param object
   *
   * @return True if delegate is bound to the specified UserObject
   */
  virtual bool HasSameObject(const void* object) const = 0;

  /**
   * Checks to see if the user object bound to this delegate can ever be valid again.
   * used to compact multicast delegate arrays so they don't expand without limit.
   *
   * @return True if the user object can never be used again
   */
  virtual bool IsCompactable() const
  {
    return !IsSafeToExecute();
  }

  /**
   * Checks to see if the user object bound to this delegate is still valid
   *
   * @return True if the user object is still valid and it's safe to execute the function call
   */
  virtual bool IsSafeToExecute() const = 0;

  /**
   * Returns a handle for the delegate.
   */
  virtual DelegateHandle GetHandle() const = 0;

 public:
  /**
   * Virtual destructor.
   */
  virtual ~IDelegateInstance() {}
};

} // namespace fun
