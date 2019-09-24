#pragma once

#include "fun/base/base.h"
#include "fun/base/container/allocation_policies.h"
#include "fun/base/container/array.h"
#include "fun/base/delegate/delegate_base.h"
#include "fun/base/delegate/i_delegate_instance.h"
#include "fun/base/math/math_base.h"

namespace fun {

#if FUN_USE_SMALL_MULTICAST_DELEGATES
typedef HeapAllocator MulticastInvocationListAllocatorType;
#else
typedef InlineAllocator<1> MulticastInvocationListAllocatorType;
#endif

typedef Array<DelegateBase, MulticastInvocationListAllocatorType>
    InvocationList;

/**
 * Abstract base class for multicast delegates.
 */
template <typename ObjectPtrType>
class MulticastDelegateBase {
 public:
  ~MulticastDelegateBase() {}

  /**
   * Removes all functions from this delegate's invocation list.
   */
  void Clear() {
    for (DelegateBase& delegate_base_ref : invocation_list_) {
      delegate_base_ref.Unbind();
    }

    CompactInvocationList(false);
  }

  /**
   * Checks to see if any functions are bound to this multi-cast delegate.
   *
   * @return true if any functions are bound, false otherwise.
   */
  inline bool IsBound() const {
    for (const DelegateBase& delegate_base_ref : invocation_list_) {
      if (delegate_base_ref.GetDelegateInstanceProtected()) {
        return true;
      }
    }
    return false;
  }

  /**
   * Checks to see if any functions are bound to the given user object.
   *
   * @return  True if any functions are bound to object, false otherwise.
   */
  inline bool IsBoundToObject(void const* object) const {
    for (const DelegateBase& delegate_base_ref : invocation_list_) {
      IDelegateInstance* delegate_instance =
          delegate_base_ref.GetDelegateInstanceProtected();
      if (delegate_instance && delegate_instance->HasSameObject(object)) {
        return true;
      }
    }

    return false;
  }

  /**
   * Removes all functions from this multi-cast delegate's invocation list
   * that are bound to the specified UserObject.
   *
   * Note that the order of the delegates may not be preserved.
   *
   * @param object The object to remove all delegates for.
   */
  void RemoveAll(const void* object) {
    if (invocation_list_lock_count_ > 0) {
      bool needs_compacted = false;
      for (DelegateBase& delegate_base_ref : invocation_list_) {
        IDelegateInstance* delegate_instance =
            delegate_base_ref.GetDelegateInstanceProtected();
        if (delegate_instance && delegate_instance->HasSameObject(object)) {
          // Manually unbind the delegate here so the compaction
          // will find and remove it.
          delegate_base_ref.Unbind();
          needs_compacted = true;
        }
      }

      // can't compact at the moment, but set out threshold to
      // zero so the next add will do it
      if (needs_compacted) {
        compaction_threshold_ = 0;
      }
    } else {
      // compact us while shuffling in later delegates to fill holes
      for (int32 invocation_item_index = 0;
           invocation_item_index < invocation_list_.Count();) {
        DelegateBase& delegate_base_ref =
            invocation_list_[invocation_item_index];

        IDelegateInstance* delegate_instance =
            delegate_base_ref.GetDelegateInstanceProtected();
        if (delegate_instance == nullptr ||
            delegate_instance->HasSameObject(object) ||
            delegate_instance->IsCompactable()) {
          invocation_list_.RemoveAtSwap(invocation_item_index, 1, false);
        } else {
          invocation_item_index++;
        }
      }

      compaction_threshold_ = MathBase::Max(2, 2 * invocation_list_.Count());

      invocation_list_.Shrink();
    }
  }

 protected:
  /**
   * Hidden default constructor.
   */
  inline MulticastDelegateBase()
      : compaction_threshold_(2), invocation_list_lock_count_(0) {}

 protected:
  /**
   * Adds the given delegate instance to the invocation list.
   *
   * @param new_delegate_base_ref The delegate instance to add.
   */
  inline DelegateHandle AddInternal(DelegateBase&& new_delegate_base_ref) {
    // Compact but obey threshold of when this will trigger
    CompactInvocationList(true);
    DelegateHandle result = new_delegate_base_ref.GetHandle();
    invocation_list_.Add(MoveTemp(new_delegate_base_ref));
    return result;
  }

  /**
   * Removes any expired or deleted functions from the invocation list.
   *
   * @see RequestCompaction
   */
  void CompactInvocationList(bool check_threshold = false) {
    // If locked and no object, just return
    if (invocation_list_lock_count_ > 0) {
      return;
    }

    // if checking threshold, obey but decay.
    // This is to ensure that even infrequently called delegates will
    // eventually compact during an Add()
    if (check_threshold && --compaction_threshold_ > invocation_list_.Count()) {
      return;
    }

    const int32 old_item_count = invocation_list_.Count();

    // Find anything null or compactable and remove it
    for (int32 invocation_item_index = 0;
         invocation_item_index < invocation_list_.Count();) {
      DelegateBase& delegate_base_ref = invocation_list_[invocation_item_index];

      IDelegateInstance* delegate_instance =
          delegate_base_ref.GetDelegateInstanceProtected();
      if (delegate_instance == nullptr || delegate_instance->IsCompactable()) {
        invocation_list_.RemoveAtSwap(invocation_item_index);
      } else {
        invocation_item_index++;
      }
    }

    compaction_threshold_ = MathBase::Max(2, 2 * invocation_list_.Count());

    if (old_item_count > compaction_threshold_) {
      // Would be nice to shrink down to threshold, but reserve only grows..?
      invocation_list_.Shrink();
    }
  }

  /**
   * Gets a read-only reference to the invocation list.
   */
  inline const InvocationList& GetInvocationList() const {
    return invocation_list_;
  }

  /**
   * Increments the lock counter for the invocation list.
   */
  inline void LockInvocationList() const { ++invocation_list_lock_count_; }

  /**
   * Decrements the lock counter for the invocation list.
   */
  inline void UnlockInvocationList() const { --invocation_list_lock_count_; }

 protected:
  /**
   * Helper function for derived classes of MulticastDelegateBase
   * to get at the delegate instance.
   */
  static FUN_ALWAYS_INLINE IDelegateInstance*
  GetDelegateInstanceProtectedHelper(const DelegateBase& base) {
    return base.GetDelegateInstanceProtected();
  }

 private:
  /** Holds the collection of delegate instances to invoke. */
  InvocationList invocation_list_;

  /** Used to determine when a compaction should happen. */
  int32 compaction_threshold_;

  /** Holds a lock counter for the invocation list. */
  mutable int32 invocation_list_lock_count_;
};

}  // namespace fun
