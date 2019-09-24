#pragma once

#include "fun/base/base.h"
#include "fun/base/container/allocation_policies.h"
#include "fun/base/math/math_base.h"
#include "fun/base/delegate/delegate_config.h"
#include "fun/base/delegate/i_delegate_instance.h"
#include "fun/base/string/string.h"

namespace fun {

struct WeakObjectPtr;

#if !defined(_WIN32) || defined(_WIN64)
  // Let delegates store up to 32 bytes which are 16-byte aligned before we heap allocate
  typedef AlignedStorage<16, 16> AlignedInlineDelegateType;
  #if FUN_USE_SMALL_DELEGATES
    typedef HeapAllocator DelegateAllocatorType;
  #else
    typedef InlineAllocator<2> DelegateAllocatorType;
  #endif
#else
  // ... except on Win32, because we can't pass 16-byte aligned types by value, as some delegates are
  // so we'll just keep it heap-allocated, which are always sufficiently aligned.
  typedef AlignedStorage<16, 8> AlignedInlineDelegateType;
  typedef HeapAllocator DelegateAllocatorType;
#endif

//TODO
//struct WeakObjectPtr;

template <typename ObjectPtrType>
class MulticastDelegateBase;

/**
 * base class for unicast delegates.
 */
class DelegateBase
{
  friend class MulticastDelegateBase<WeakObjectPtr>;

 public:
  /**
   * Creates and initializes a new instance.
   */
  explicit DelegateBase()
    : delegate_size_(0)
  {
  }

  ~DelegateBase()
  {
    Unbind();
  }

  /**
   * Move constructor.
   */
  DelegateBase(DelegateBase&& other)
  {
    delegate_allocator_.MoveToEmpty(other.delegate_allocator_);
    delegate_size_ = other.delegate_size_;
    other.delegate_size_ = 0;
  }

  /**
   * Move assignment.
   */
  DelegateBase& operator = (DelegateBase&& other)
  {
    if (FUN_LIKELY(&other != this)) {
      Unbind();
      delegate_allocator_.MoveToEmpty(other.delegate_allocator_);
      delegate_size_ = other.delegate_size_;
      other.delegate_size_ = 0;
    }
    return *this;
  }

#if FUN_USE_DELEGATE_TRYGETBOUNDFUNCTIONNAME
  /**
   * Tries to return the name of a bound function.
   * Returns "" if the delegate is unbound or
   * a binding name is unavailable.
   *
   * Note: Only intended to be used to aid debugging of delegates.
   *
   * @return The name of the bound function, "" if no name was available.
   */
  String TryGetBoundFunctionName() const
  {
    if (IDelegateInstance* ptr = GetDelegateInstanceProtected()) {
      return ptr->TryGetBoundFunctionName();
    }

    return String();
  }
#endif // FUN_USE_DELEGATE_TRYGETBOUNDFUNCTIONNAME

  /**
   * If this is a FFunction or FObject delegate, return the FObject.
   *
   * @return The object associated with this delegate if there is one.
   */
  FUN_ALWAYS_INLINE FObject* GetFObject() const
  {
    if (IDelegateInstance* ptr = GetDelegateInstanceProtected()) {
      return ptr->GetFObject();
    }

    return nullptr;
  }

  /**
   * Checks to see if the user object bound to this delegate is still valid.
   *
   * @return True if the user object is still valid and it's safe to execute the function call.
   */
  FUN_ALWAYS_INLINE bool IsBound() const
  {
    IDelegateInstance* ptr = GetDelegateInstanceProtected();
    return ptr && ptr->IsSafeToExecute();
  }

  /**
   * Checks to see if this delegate is bound to the given user object.
   *
   * @return True if this delegate is bound to object, false otherwise.
   */
  FUN_ALWAYS_INLINE bool IsBoundToObject(void const* object) const
  {
    if (!object) {
      return false;
    }

    IDelegateInstance* ptr = GetDelegateInstanceProtected();
    return ptr && ptr->HasSameObject(object);
  }

  /**
   * Unbinds this delegate
   */
  FUN_ALWAYS_INLINE void Unbind()
  {
    if (IDelegateInstance* ptr = GetDelegateInstanceProtected()) {
      ptr->~IDelegateInstance();
      delegate_allocator_.ResizeAllocation(0, 0, sizeof(AlignedInlineDelegateType));
      delegate_size_ = 0;
    }
  }

  /**
   * Gets the delegate instance.
   *
   * @return The delegate instance.
   * @see SetDelegateInstance
   */
  //FUN_DEPRECATED(4.11, "GetDelegateInstance has been deprecated - calls to IDelegateInstance::GetFObject() and IDelegateInstance::GetHandle() should call the same functions on the delegate.  other calls should be reconsidered.")
  //FUN_ALWAYS_INLINE IDelegateInstance* GetDelegateInstance() const
  //{
  //  return GetDelegateInstanceProtected();
  //}

  /**
   * Gets a handle to the delegate.
   *
   * @return The delegate instance.
   */
  FUN_ALWAYS_INLINE DelegateHandle GetHandle() const
  {
    DelegateHandle result;
    if (IDelegateInstance* ptr = GetDelegateInstanceProtected()) {
      result = ptr->GetHandle();
    }

    return result;
  }

 protected:
  /**
   * Gets the delegate instance.  Not intended for use by user code.
   *
   * @return The delegate instance.
   *
   * @see SetDelegateInstance
   */
  FUN_ALWAYS_INLINE IDelegateInstance* GetDelegateInstanceProtected() const
  {
    return delegate_size_ ? (IDelegateInstance*)delegate_allocator_.GetAllocation() : nullptr;
  }

 public:
  void* AllocateInternal(size_t size)
  {
    if (IDelegateInstance* current_instance = GetDelegateInstanceProtected()) {
      current_instance->~IDelegateInstance();
    }

    const size_t new_delegate_size = MathBase::DivideAndRoundUp(size, sizeof(AlignedInlineDelegateType));
    if (delegate_size_ != new_delegate_size) {
      delegate_allocator_.ResizeAllocation(0, (int32)new_delegate_size, sizeof(AlignedInlineDelegateType));
      delegate_size_ = new_delegate_size;
    }

    return delegate_allocator_.GetAllocation();
  }

 private:
  DelegateAllocatorType::ForElementType<AlignedInlineDelegateType> delegate_allocator_;
  size_t delegate_size_;
};

} // namespace fun

inline void* operator new(fun::size_t size, fun::DelegateBase& base)
{
  return base.AllocateInternal(size);
}
