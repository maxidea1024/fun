#pragma once

// Default behavior.
#define FORCE_THREADSAFE_SHAREDPTRS  1

//TODO SPMode를 ThreadSafety로 바꿔주는게 어떨까??

/**
 * SPMode is used select between either 'fast' or 'thread safe' shared pointer types.
 * This is only used by templates at compile time to generate one code path or another.
 */
enum class SPMode {
  /**
   * Forced to be not thread-safe.
   */
  NotThreadSafe = 0,

  /**
   * Fast, doesn't ever use atomic interlocks.
   * Some code requires that all shared pointers are thread-safe.
   * It's better to change it here, instead of replacing SPMode::Fast to SPMode::ThreadSafe throughout the code.
   */
  Fast = FORCE_THREADSAFE_SHAREDPTRS ? 1 : 0,

  /**
   * Conditionally thread-safe, never spin locks, but slower
   */
  ThreadSafe = 1
};


/**
 * Forward declarations.  Note that in the interest of fast performance, thread safety
 * features are mostly turned off (Mode = SPMode::Fast).  If you need to access your
 * object on multiple threads, you should use SPMode::ThreadSafe!
 */
template <typename ObjectType, SPMode Mode = SPMode::Fast> class SharedRef;
template <typename ObjectType, SPMode Mode = SPMode::Fast> class SharedPtr;
template <typename ObjectType, SPMode Mode = SPMode::Fast> class WeakPtr;
template <typename ObjectType, SPMode Mode = SPMode::Fast> class SharedFromThis;

/**
 * SharedPtr_internal contains internal workings of shared and weak pointers.
 * You should hopefully never have to use anything
 * inside this namespace directly.
 */
namespace SharedPtr_internal {

  // Forward declarations
  template <SPMode Mode> class WeakReferencer;

  /**
   * Dummy structures used internally as template arguments for typecasts
   */
  struct StaticCastTag {};
  struct ConstCastTag {};

  // NOTE: The following is an FUN extension to standard shared_ptr behavior
  struct NullTag {};

  class ReferenceControllerBase {
   public:
    /**
     * Constructor
     */
    FUN_ALWAYS_INLINE explicit ReferenceControllerBase(void* object)
      : shared_reference_count_(1),
        weak_reference_count_(1),
        object_(object) {}

    // NOTE: The primary reason these reference counters are 32-bit values
    //       (and not 16-bit to save memory), is that atomic operations
    //       require at least 32-bit objects.

    /**
     * Number of shared references to this object.  When this count
     * reaches zero, the associated object will be destroyed.
     * (even if there are still weak references!)
     */
    int32 shared_reference_count_;

    /**
     * Number of weak references to this object.
     * If there are any shared references, that counts as one
     * weak reference too.
     */
    int32 weak_reference_count_;

    /**
     * The object associated with this reference counter.
     */
    void* object_;

    /**
     * Destroys the object associated with this reference counter.
     */
    virtual void DestroyObject() = 0;

    /**
     * Destroys the object associated with this reference counter.
     */
    virtual ~ReferenceControllerBase() {};

   private:
    ReferenceControllerBase(ReferenceControllerBase const&) = delete;
    ReferenceControllerBase& operator = (ReferenceControllerBase const&) = delete;
  };


  template <typename ObjectType, typename DeleterType>
  class ReferenceControllerWithDeleter
    : private DeleterType
    , public ReferenceControllerBase {
   public:
    explicit ReferenceControllerWithDeleter(void* object, DeleterType&& deleter)
      : DeleterType(MoveTemp(deleter)),
        ReferenceControllerBase(object) {}

    virtual void DestroyObject() {
      (*static_cast<DeleterType*>(this))((ObjectType*)static_cast<ReferenceControllerBase*>(this)->object_);
    }
  };


  /**
   * Deletes an object via the standard delete operator
   */
  template <typename Type>
  struct DefaultDeleter {
    FUN_ALWAYS_INLINE void operator()(Type* object) const {
      delete object;
    }
  };

  /**
   * Creates a reference controller which just calls delete
   */
  template <typename ObjectType>
  FUN_ALWAYS_INLINE ReferenceControllerBase* CreateDefaultReferenceController(ObjectType* object) {
    return new ReferenceControllerWithDeleter<ObjectType, DefaultDeleter<ObjectType>>(object, DefaultDeleter<ObjectType>());
  }

  /**
   * Creates a custom reference controller with a specified deleter
   */
  template <typename ObjectType, typename DeleterType>
  FUN_ALWAYS_INLINE ReferenceControllerBase* CreateCustomReferenceController(ObjectType* object, DeleterType&& deleter) {
    return new ReferenceControllerWithDeleter<ObjectType, typename RemoveReference<DeleterType>::Type>(object, Forward<DeleterType>(deleter));
  }

  /**
   * Proxy structure for implicitly converting raw pointers
   * to shared/weak pointers
   *
   * NOTE: The following is an FUN extension to standard shared_ptr behavior
   */
  template <typename ObjectType>
  struct RawPtrProxy {
    /**
     * The object pointer.
     */
    ObjectType* object;

    /**
     * Reference controller used to destroy the object.
     */
    ReferenceControllerBase* reference_controller;

    /**
     * Construct implicitly from an object.
     */
    FUN_ALWAYS_INLINE RawPtrProxy(ObjectType* object)
      : object(object),
        reference_controller(CreateDefaultReferenceController(object)) {}

    /**
     * Construct implicitly from an object and a custom deleter.
     */
    template <typename DeleterType>
    FUN_ALWAYS_INLINE RawPtrProxy(ObjectType* object, DeleterType&& deleter)
      : object(object),
        reference_controller(CreateCustomReferenceController(object, Forward<DeleterType>(deleter))) {}
  };


  /**
   * ReferenceController is a standalone heap-allocated object that tracks
   * the number of references to an object referenced by SharedRef,
   * SharedPtr or WeakPtr objects.
   *
   * It is specialized for different threading modes.
   */
  template <SPMode Mode> struct ReferenceControllerOps;

  template <>
  struct ReferenceControllerOps<SPMode::ThreadSafe> {
    /**
     * Returns the shared reference count
     */
    static FUN_ALWAYS_INLINE const int32 GetSharedReferenceCount(const ReferenceControllerBase* reference_controller) {
      // This reference count may be accessed by multiple threads
      return static_cast<int32 const volatile&>(reference_controller->shared_reference_count_);
    }

    /**
     * Adds a shared reference to this counter
     */
    static FUN_ALWAYS_INLINE void AddSharedReference(ReferenceControllerBase* reference_controller) {
      Atomics::Increment(&reference_controller->shared_reference_count_);
    }

    /**
     * Adds a shared reference to this counter ONLY if there is already
     * at least one reference
     *
     * \return True if the shared reference was added successfully
     */
    static bool ConditionallyAddSharedReference(ReferenceControllerBase* reference_controller) {
      for (;;) {
        // Peek at the current shared reference count.  Remember, this value may be updated by
        // multiple threads.
        const int32 original_count = static_cast<int32 const volatile&>(reference_controller->shared_reference_count_);
        if (original_count == 0) {
          // Never add a shared reference if the pointer has already expired
          return false;
        }

        // Attempt to increment the reference count.
        const int32 actual_original_count = Atomics::CompareExchange(&reference_controller->shared_reference_count_, original_count + 1, original_count);

        // We need to make sure that we never revive a counter that has already expired, so if the
        // actual value what we expected (because it was touched by another thread), then we'll try
        // again.  Note that only in very unusual cases will this actually have to loop.
        if (actual_original_count == original_count) {
          return true;
        }
      }
    }

    /**
     * Releases a shared reference to this counter
     */
    static FUN_ALWAYS_INLINE void ReleaseSharedReference(ReferenceControllerBase* reference_controller) {
      fun_check_dbg(reference_controller->shared_reference_count_ > 0);

      if (Atomics::Decrement(&reference_controller->shared_reference_count_) == 0) {
        // Last shared reference was released!  Destroy the referenced object.
        reference_controller->DestroyObject();

        // No more shared referencers, so decrement the weak reference count by one.  When the weak
        // reference count reaches zero, this object will be deleted.
        ReleaseWeakReference(reference_controller);
      }
    }

    /**
     * Adds a weak reference to this counter
     */
    static FUN_ALWAYS_INLINE void AddWeakReference(ReferenceControllerBase* reference_controller) {
      Atomics::Increment(&reference_controller->weak_reference_count_);
    }

    /**
     * Releases a weak reference to this counter
     */
    static void ReleaseWeakReference(ReferenceControllerBase* reference_controller) {
      fun_check_dbg(reference_controller->weak_reference_count_ > 0);

      if (Atomics::Decrement(&reference_controller->weak_reference_count_) == 0) {
        // No more references to this reference count.  Destroy it!
        delete reference_controller;
      }
    }
  };

  template <>
  struct ReferenceControllerOps<SPMode::NotThreadSafe> {
    /**
     * Returns the shared reference count
     */
    static FUN_ALWAYS_INLINE const int32 GetSharedReferenceCount(const ReferenceControllerBase* reference_controller) {
      return reference_controller->shared_reference_count_;
    }

    /**
     * Adds a shared reference to this counter
     */
    static FUN_ALWAYS_INLINE void AddSharedReference(ReferenceControllerBase* reference_controller) {
      ++reference_controller->shared_reference_count_;
    }

    /**
     * Adds a shared reference to this counter ONLY if there is already at least one reference
     *
     * \return True if the shared reference was added successfully
     */
    static bool ConditionallyAddSharedReference(ReferenceControllerBase* reference_controller) {
      if (reference_controller->shared_reference_count_ == 0) {
        // Never add a shared reference if the pointer has already expired
        return false;
      }

      ++reference_controller->shared_reference_count_;
      return true;
    }

    /**
     * Releases a shared reference to this counter
     */
    static FUN_ALWAYS_INLINE void ReleaseSharedReference(ReferenceControllerBase* reference_controller) {
      fun_check_dbg(reference_controller->shared_reference_count_ > 0);

      if (--reference_controller->shared_reference_count_ == 0) {
        // Last shared reference was released!  Destroy the referenced object.
        reference_controller->DestroyObject();

        // No more shared referencers, so decrement the weak reference count by one.  When the weak
        // reference count reaches zero, this object will be deleted.
        ReleaseWeakReference(reference_controller);
      }
    }

    /**
     * Adds a weak reference to this counter
     */
    static FUN_ALWAYS_INLINE void AddWeakReference(ReferenceControllerBase* reference_controller) {
      ++reference_controller->weak_reference_count_;
    }

    /**
     * Releases a weak reference to this counter
     */
    static void ReleaseWeakReference(ReferenceControllerBase* reference_controller) {
      fun_check_dbg(reference_controller->weak_reference_count_ > 0);

      if (--reference_controller->weak_reference_count_ == 0) {
        // No more references to this reference count.  Destroy it!
        delete reference_controller;
      }
    }
  };


  /**
   * SharedReferencer is a wrapper around a pointer to a reference controller that is used by either a
   * SharedRef or a SharedPtr to keep track of a referenced object's lifetime
   */
  template <SPMode Mode>
  class SharedReferencer {
    typedef ReferenceControllerOps<Mode> Ops;

   public:
    /**
     * Constructor for an empty shared referencer object
     */
    FUN_ALWAYS_INLINE SharedReferencer()
      : reference_controller_(nullptr) {}

    /**
     * Constructor that counts a single reference to the specified object
     */
    FUN_ALWAYS_INLINE explicit SharedReferencer(ReferenceControllerBase* reference_controller)
      : reference_controller_(reference_controller) {}

    /**
     * Copy constructor creates a new reference to the existing object
     */
    FUN_ALWAYS_INLINE SharedReferencer(SharedReferencer const& shared_ref)
      : reference_controller_(shared_ref.reference_controller_) {
      // If the incoming reference had an object associated with it, then go ahead and increment the
      // shared reference count
      if (reference_controller_) {
        Ops::AddSharedReference(reference_controller_);
      }
    }

    /**
     * Move constructor creates no new references
     */
    FUN_ALWAYS_INLINE SharedReferencer(SharedReferencer&& shared_ref)
      : reference_controller_(shared_ref.reference_controller_) {
      shared_ref.reference_controller_ = nullptr;
    }

    /**
     * Creates a shared referencer object from a weak referencer object.  This will only result
     * in a valid object reference if the object already has at least one other shared referencer.
     */
    SharedReferencer(WeakReferencer<Mode> const& weak_ref)
      : reference_controller_(weak_ref.reference_controller_) {
      // If the incoming reference had an object associated with it, then go ahead and increment the
      // shared reference count
      if (reference_controller_) {
        // Attempt to elevate a weak reference to a shared one.  For this to work, the object this
        // weak counter is associated with must already have at least one shared reference.  We'll
        // never revive a pointer that has already expired!
        if (!Ops::ConditionallyAddSharedReference(reference_controller_)) {
          reference_controller_ = nullptr;
        }
      }
    }

    /**
     * Destructor.
     */
    FUN_ALWAYS_INLINE ~SharedReferencer() {
      if (reference_controller_) {
        // Tell the reference counter object that we're no longer referencing the object with
        // this shared pointer
        Ops::ReleaseSharedReference(reference_controller_);
      }
    }

    /**
     * Assignment operator adds a reference to the assigned object.  If this counter was previously
     * referencing an object, that reference will be released.
     */
    FUN_ALWAYS_INLINE SharedReferencer& operator = (SharedReferencer const& shared_ref) {
      // Make sure we're not be reassigned to ourself!
      auto new_reference_controller = shared_ref.reference_controller_;
      if (new_reference_controller != reference_controller_) {
        // First, add a shared reference to the new object
        if (new_reference_controller) {
          Ops::AddSharedReference(new_reference_controller);
        }

        // Release shared reference to the old object
        if (reference_controller_) {
          Ops::ReleaseSharedReference(reference_controller_);
        }

        // Assume ownership of the assigned reference counter
        reference_controller_ = new_reference_controller;
      }

      return *this;
    }

    /**
     * Move assignment operator adds no references to the assigned object.
     * If this counter was previously referencing an object,
     * that reference will be released.
     */
    FUN_ALWAYS_INLINE SharedReferencer& operator = (SharedReferencer&& shared_ref) {
      // Make sure we're not be reassigned to ourself.
      auto new_reference_controller = shared_ref.reference_controller_;
      auto old_reference_controller = reference_controller_;
      if (new_reference_controller != old_reference_controller) {
        // Assume ownership of the assigned reference counter
        shared_ref.reference_controller_ = nullptr;
        reference_controller_ = new_reference_controller;

        // Release shared reference to the old object
        if (old_reference_controller) {
          Ops::ReleaseSharedReference(old_reference_controller);
        }
      }

      return *this;
    }

    /**
     * Tests to see whether or not this shared counter contains
     * a valid reference
     *
     * \return True if reference is valid
     */
    FUN_ALWAYS_INLINE const bool IsValid() const {
      return !!reference_controller_;
    }

    /**
     * Returns the number of shared references to this object
     * (including this reference.)
     *
     * \return Number of shared references to the object (including this reference.)
     */
    FUN_ALWAYS_INLINE const int32 GetSharedReferenceCount() const {
      return reference_controller_ ? Ops::GetSharedReferenceCount(reference_controller_) : 0;
    }

    /**
     * Returns true if this is the only shared reference to this object.
     * Note that there may be outstanding weak references left.
     *
     * \return True if there is only one shared reference to the object, and this is it!
     */
    FUN_ALWAYS_INLINE const bool IsUnique() const {
      return GetSharedReferenceCount() == 1;
    }

  private:
    // Expose access to reference_controller_ to WeakReferencer
    template <SPMode OtherMode> friend class WeakReferencer;

  private:
    /**
     * Pointer to the reference controller for the object
     * a shared reference/pointer is referencing
     */
    ReferenceControllerBase* reference_controller_;
  };


  /**
   * WeakReferencer is a wrapper around a pointer to a reference controller
   * that is used by a WeakPtr to keep track of a referenced object's lifetime.
   */
  template <SPMode Mode>
  class WeakReferencer {
    typedef ReferenceControllerOps<Mode> Ops;

   public:
    /**
     * Default constructor with empty counter
     */
    FUN_ALWAYS_INLINE WeakReferencer()
      : reference_controller_(nullptr) {}

    /**
     * Construct a weak referencer object from another weak referencer
     */
    FUN_ALWAYS_INLINE WeakReferencer(WeakReferencer const& weak_referencer)
      : reference_controller_(weak_referencer.reference_controller_) {
      // If the weak referencer has a valid controller,
      // then go ahead and add a weak reference to it.
      if (reference_controller_) {
        Ops::AddWeakReference(reference_controller_);
      }
    }

    /**
     * Construct a weak referencer object from an rvalue weak referencer
     */
    FUN_ALWAYS_INLINE WeakReferencer(WeakReferencer&& weak_referencer)
      : reference_controller_(weak_referencer.reference_controller_) {
      weak_referencer.reference_controller_ = nullptr;
    }

    /**
     * Construct a weak referencer object from a shared referencer object
     */
    FUN_ALWAYS_INLINE WeakReferencer(SharedReferencer<Mode> const& shared_referencer)
      : reference_controller_(shared_referencer.reference_controller_) {
      // If the shared referencer had a valid controller,
      // then go ahead and add a weak reference to it.
      if (reference_controller_) {
        Ops::AddWeakReference(reference_controller_);
      }
    }

    /**
     * Destructor.
     */
    FUN_ALWAYS_INLINE ~WeakReferencer() {
      if (reference_controller_) {
        // Tell the reference counter object that we're no longer
        // referencing the object with this weak pointer
        Ops::ReleaseWeakReference(reference_controller_);
      }
    }

    /**
     * Assignment operator from a weak referencer object.
     * If this counter was previously referencing an
     * object, that reference will be released.
     */
    FUN_ALWAYS_INLINE WeakReferencer& operator = (WeakReferencer const& weak_referencer) {
      AssignReferenceController(weak_referencer.reference_controller_);
      return *this;
    }

    /**
     * Assignment operator from an rvalue weak referencer object.
     * If this counter was previously referencing an
     * object, that reference will be released.
     */
    FUN_ALWAYS_INLINE WeakReferencer& operator = (WeakReferencer&& weak_referencer) {
      auto old_reference_controller = reference_controller_;
      reference_controller_ = weak_referencer.reference_controller_;
      weak_referencer.reference_controller_ = nullptr;
      if (old_reference_controller) {
        Ops::ReleaseWeakReference(old_reference_controller);
      }

      return *this;
    }

    /**
     * Assignment operator from a shared reference counter.
     * If this counter was previously referencing an
     * object, that reference will be released.
     */
    FUN_ALWAYS_INLINE WeakReferencer& operator = (SharedReferencer<Mode> const& shared_referencer) {
      AssignReferenceController(shared_referencer.reference_controller_);
      return *this;
    }

    /**
     * Tests to see whether or not this weak counter contains a valid reference
     *
     * \return True if reference is valid
     */
    FUN_ALWAYS_INLINE const bool IsValid() const {
      return reference_controller_ && Ops::GetSharedReferenceCount(reference_controller_) > 0;
    }

   private:
    /**
     * Assigns a new reference controller to this counter object, first adding
     * a reference to it, then releasing the previous object.
     */
    FUN_ALWAYS_INLINE void AssignReferenceController(ReferenceControllerBase* new_reference_controller) {
      // Only proceed if the new reference counter is different than our current
      if (new_reference_controller != reference_controller_) {
        // First, add a weak reference to the new object
        if (new_reference_controller) {
          Ops::AddWeakReference(new_reference_controller);
        }

        // Release weak reference to the old object
        if (reference_controller_) {
          Ops::ReleaseWeakReference(reference_controller_);
        }

        // Assume ownership of the assigned reference counter
        reference_controller_ = new_reference_controller;
      }
    }

   private:
    /**
     * Expose access to reference_controller_ to SharedReferencer.
     */
    template <SPMode OtherMode> friend class SharedReferencer;

   private:
    /**
     * Pointer to the reference controller for the object
     * a WeakPtr is referencing
     */
    ReferenceControllerBase* reference_controller_;
  };

  /**
   * Templated helper function (const) that creates a shared pointer
   * from an object instance
   */
  template <typename SharedPtrType, typename ObjectType, typename OtherType, SPMode Mode>
  FUN_ALWAYS_INLINE void EnableSharedFromThis(SharedPtr<SharedPtrType, Mode> const* shared_ptr, ObjectType const* object, SharedFromThis<OtherType, Mode> const* shareable) {
    if (shareable) {
      shareable->UpdateWeakReferenceInternal(shared_ptr, const_cast<ObjectType*>(object));
    }
  }

  /**
   * Templated helper function that creates a shared pointer
   * from an object instance
   */
  template <typename SharedPtrType, typename ObjectType, typename OtherType, SPMode Mode>
  FUN_ALWAYS_INLINE void EnableSharedFromThis(SharedPtr<SharedPtrType, Mode>* shared_ptr, ObjectType const* object, SharedFromThis<OtherType, Mode> const* shareable) {
    if (shareable) {
      shareable->UpdateWeakReferenceInternal(shared_ptr, const_cast<ObjectType*>(object));
    }
  }

  /**
   * Templated helper function (const) that creates a shared reference
   * from an object instance
   */
  template <typename SharedRefType, typename ObjectType, typename OtherType, SPMode Mode>
  FUN_ALWAYS_INLINE void EnableSharedFromThis(SharedRef<SharedRefType, Mode> const* shared_ref, ObjectType const* object, SharedFromThis<OtherType, Mode> const* shareable) {
    if (shareable) {
      shareable->UpdateWeakReferenceInternal(shared_ref, const_cast<ObjectType*>(object));
    }
  }

  /**
   * Templated helper function that creates a shared reference
   * from an object instance
   */
  template <typename SharedRefType, typename ObjectType, typename OtherType, SPMode Mode>
  FUN_ALWAYS_INLINE void EnableSharedFromThis(SharedRef<SharedRefType, Mode>* shared_ref, ObjectType const* object, SharedFromThis<OtherType, Mode> const* shareable) {
    if (shareable) {
      shareable->UpdateWeakReferenceInternal(shared_ref, const_cast<ObjectType*>(object));
    }
  }

  /**
   * Templated helper catch-all function, accomplice to
   * the above helper functions
   */
  FUN_ALWAYS_INLINE void EnableSharedFromThis(...) {}
}
