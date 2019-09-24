#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/template.h"
#include "fun/base/atomics.h"

namespace fun {

#include "fun/base/ftl/shared_ptr_internal.h"

/**
 * Casts a shared reference of one type to another type. (static_cast)  Useful for down-casting.
 *
 * \param shared_ref - The shared reference to cast
 */
template <typename CastToType, typename CastFromType, SPMode Mode>
FUN_ALWAYS_INLINE SharedRef<CastToType, Mode> StaticCastSharedRef(SharedRef<CastFromType, Mode> const& shared_ref) {
  return SharedRef<CastToType, Mode>(shared_ref, SharedPtr_internal::StaticCastTag());
}


class FObjectBase;

/**
 * SharedRef is a non-nullable, non-intrusive reference-counted authoritative object reference.
 *
 * This shared reference will be conditionally thread-safe when the optional Mode template argument is set to ThreadSafe.
 * OTE: SharedRef is an FUN extension to standard smart pointer feature set
 */
template <typename ObjectType, SPMode Mode>
class SharedRef {
  // TSharedRefs with FObjects are illegal.
  static_assert(!PointerIsConvertibleFromTo<ObjectType, const FObjectBase>::Value, "You cannot use SharedRef with FObjects.");

 public:
  // NOTE: SharedRef has no default constructor as it does not support empty references.  You must
  //       initialize your SharedRef to a valid object at construction time.

  /**
   * Constructs a shared reference that owns the specified object.  Must not be nullptr.
   *
   * \param object - object this shared reference to retain a reference to
   */
  template <typename OtherType>
  FUN_ALWAYS_INLINE explicit SharedRef(OtherType* object)
    : object_(object),
      shared_referencer_(SharedPtr_internal::CreateDefaultReferenceController(object)) {
    Init(object);
  }

  /**
   * Constructs a shared reference that owns the specified object.  Must not be nullptr.
   *
   * \param object - object this shared pointer to retain a reference to
   * \param deleter - Deleter object used to destroy the object when it is no longer referenced.
   */
  template <typename OtherType, typename DeleterType>
  FUN_ALWAYS_INLINE SharedRef(OtherType* object, DeleterType&& deleter)
    : object_(object),
      shared_referencer_(SharedPtr_internal::CreateCustomReferenceController(object, Forward<DeleterType>(deleter))) {
    Init(object);
  }

#if FUN_WITH_HOT_RELOAD_CTORS
  /**
   * Constructs default shared reference that owns the default object for specified type.
   *
   * Used internally only. Please do not use!
   */
  SharedRef()
    : object_(new ObjectType()),
      shared_referencer_(SharedPtr_internal::CreateDefaultReferenceController(object_)) {
    EnsureRetrievingVTablePtrDuringCtor(TEXT("SharedRef()"));
    Init(object_);
  }
#endif //FUN_WITH_HOT_RELOAD_CTORS

  /**
   * Constructs a shared reference using a proxy reference to a raw pointer. (See MakeShareable())
   * Must not be nullptr.
   *
   * \param raw_ptr_proxy - Proxy raw pointer that contains the object that the new shared reference will reference
   *
   * NOTE: The following is an FUN extension to standard shared_ptr behavior
   */
  template <typename OtherType>
  FUN_ALWAYS_INLINE SharedRef(SharedPtr_internal::RawPtrProxy<OtherType> const& raw_ptr_proxy)
    : object_(raw_ptr_proxy.object),
      shared_referencer_(raw_ptr_proxy.reference_controller) {
    // If the following assert goes off, it means a SharedRef was initialized from a nullptr object pointer.
    // Shared references must never be nullptr, so either pass a valid object or consider using SharedPtr instead.
    fun_check_ptr(raw_ptr_proxy.object_);

    // If the object happens to be derived from SharedFromThis, the following method
    // will prime the object with a weak pointer to itself.
    SharedPtr_internal::EnableSharedFromThis(this, raw_ptr_proxy.object, raw_ptr_proxy.object);
  }

  /**
   * Constructs a shared reference as a reference to an existing shared reference's object.
   * This constructor is needed so that we can implicitly upcast to base classes.
   *
   * \param shared_ref - The shared reference whose object we should create an additional reference to
   */
  template <typename OtherType>
  FUN_ALWAYS_INLINE SharedRef(SharedRef<OtherType, Mode> const& shared_ref)
    : object_(shared_ref.object_),
      shared_referencer_(shared_ref.shared_referencer_) {}

  /**
   * Special constructor used internally to statically cast one shared reference type to another.  You
   * should never call this constructor directly.  Instead, use the StaticCastSharedRef() function.
   * This constructor creates a shared reference as a shared reference to an existing shared reference after
   * statically casting that reference's object.  This constructor is needed for static casts.
   *
   * \param shared_ref - The shared reference whose object we should create an additional reference to
   */
  template <typename OtherType>
  FUN_ALWAYS_INLINE SharedRef(SharedRef<OtherType, Mode> const& shared_ref, SharedPtr_internal::StaticCastTag)
    : object_(static_cast<ObjectType*>(shared_ref.object_)),
      shared_referencer_(shared_ref.shared_referencer_) {}

  /**
   * Special constructor used internally to cast a 'const' shared reference a 'mutable' reference.  You
   * should never call this constructor directly.  Instead, use the ConstCastSharedRef() function.
   * This constructor creates a shared reference as a shared reference to an existing shared reference after
   * const casting that reference's object.  This constructor is needed for const casts.
   *
   * \param shared_ref - The shared reference whose object we should create an additional reference to
   */
  template <typename OtherType>
  FUN_ALWAYS_INLINE SharedRef(SharedRef<OtherType, Mode> const& shared_ref, SharedPtr_internal::ConstCastTag)
    : object_(const_cast<ObjectType*>(shared_ref.object_)),
      shared_referencer_(shared_ref.shared_referencer_) {}

  /**
   * Special constructor used internally to create a shared reference from an existing shared reference,
   * while using the specified object reference instead of the incoming shared reference's object
   * pointer.  This is used by with the SharedFromThis feature (by UpdateWeakReferenceInternal)
   *
   * \param other_shared_ref - The shared reference whose reference count
   * \param object - The object pointer to use (instead of the incoming shared reference's object)
   */
  template <typename OtherType>
  FUN_ALWAYS_INLINE SharedRef(SharedRef<OtherType, Mode> const& other_shared_ref, ObjectType* object)
    : object_(object),
      shared_referencer_(other_shared_ref.shared_referencer_) {}

  FUN_ALWAYS_INLINE SharedRef(SharedRef const& shared_ref)
    : object_(shared_ref.object_),
      shared_referencer_(shared_ref.shared_referencer_) {}

  FUN_ALWAYS_INLINE SharedRef(SharedRef&& shared_ref)
    : object_(shared_ref.object_),
      shared_referencer_(shared_ref.shared_referencer_) {
    // We're intentionally not moving here, because we don't want to leave shared_ref in a
    // null state, because that breaks the class invariant.  But we provide a move constructor
    // anyway in case the compiler complains that we have a move assign but no move construct.
  }

  /**
   * Assignment operator replaces this shared reference with the specified shared reference.  The object
   * currently referenced by this shared reference will no longer be referenced and will be deleted if
   * there are no other referencers.
   *
   * \param shared_ref - Shared reference to replace with
   */
  FUN_ALWAYS_INLINE SharedRef& operator = (SharedRef const& shared_ref) {
    shared_referencer_ = shared_ref.shared_referencer_;
    object_ = shared_ref.object_;
    return *this;
  }

  FUN_ALWAYS_INLINE SharedRef& operator = (SharedRef&& shared_ref) {
    UnsafeMemory::Memswap(this, &shared_ref, sizeof(SharedRef));
    return *this;
  }

  /**
   * Assignment operator replaces this shared reference with the specified shared reference.  The object
   * currently referenced by this shared reference will no longer be referenced and will be deleted if
   * there are no other referencers.  Must not be nullptr.
   *
   * \param raw_ptr_proxy - Proxy object used to assign the object (see MakeShareable helper function)
   *
   * NOTE: The following is an FUN extension to standard shared_ptr behavior
   */
  template <typename OtherType>
  FUN_ALWAYS_INLINE SharedRef& operator = (SharedPtr_internal::RawPtrProxy<OtherType> const& raw_ptr_proxy) {
    // If the following assert goes off, it means a SharedRef was initialized from a nullptr object pointer.
    // Shared references must never be nullptr, so either pass a valid object or consider using SharedPtr instead.
    fun_check_ptr(raw_ptr_proxy.object);

    *this = SharedRef<ObjectType, Mode>(raw_ptr_proxy);
    return *this;
  }

  /**
   * Returns a C++ reference to the object this shared reference is referencing
   *
   * \return The object owned by this shared reference
   */
  FUN_ALWAYS_INLINE ObjectType& Get() const {
    // Should never be nullptr as SharedRef is never nullable
    fun_check_dbg(IsValid());
    return *object_;
  }

  /**
   * Dereference operator returns a reference to the object this shared pointer points to
   *
   * \return Reference to the object
   */
  FUN_ALWAYS_INLINE ObjectType& operator * () const {
    // Should never be nullptr as SharedRef is never nullable
    fun_check_dbg(IsValid());
    return *object_;
  }

  /**
   * Arrow operator returns a pointer to this shared reference's object
   *
   * \return Returns a pointer to the object referenced by this shared reference
   */
  FUN_ALWAYS_INLINE ObjectType* operator -> () const {
    // Should never be nullptr as SharedRef is never nullable
    fun_check_dbg(IsValid());
    return object_;
  }

  /**
   * Returns the number of shared references to this object
   * (including this reference.)
   *
   * IMPORTANT: Not necessarily fast!  Should only be used for debugging purposes!
   *
   * \return Number of shared references to the object (including this reference.)
   */
  FUN_ALWAYS_INLINE const int32 GetSharedReferenceCount() const {
    return shared_referencer_.GetSharedReferenceCount();
  }

  /**
   * Returns true if this is the only shared reference to this object.
   * Note that there may be outstanding weak references left.
   *
   * IMPORTANT: Not necessarily fast!  Should only be used for debugging purposes!
   *
   * \return True if there is only one shared reference to the object, and this is it!
   */
  FUN_ALWAYS_INLINE const bool IsUnique() const {
    return shared_referencer_.IsUnique();
  }

 private:
  template <typename OtherType>
  void Init(OtherType* object) {
    // If the following assert goes off, it means a SharedRef was initialized from a nullptr object pointer.
    // Shared references must never be nullptr, so either pass a valid object or consider using SharedPtr instead.
    fun_check_ptr(object);

    // If the object happens to be derived from SharedFromThis, the following method
    // will prime the object with a weak pointer to itself.
    SharedPtr_internal::EnableSharedFromThis(this, object, object);
  }

  /**
   * Converts a shared pointer to a shared reference.  The pointer *must* be valid or an assertion will trigger.
   * NOTE: This explicit conversion constructor is intentionally private.  Use 'ToSharedRef()' instead.
   *
   * \return Reference to the object
   */
  template <typename OtherType>
  FUN_ALWAYS_INLINE explicit SharedRef(SharedPtr<OtherType, Mode> const& shared_ptr)
    : object_(shared_ptr.object_),
      shared_referencer_(shared_ptr.shared_referencer_) {
    // If this assert goes off, it means a shared reference was created from a shared pointer that was nullptr.
    // Shared references are never allowed to be null.  Consider using SharedPtr instead.
    fun_check(IsValid());
  }

  template <typename OtherType>
  FUN_ALWAYS_INLINE explicit SharedRef(SharedPtr<OtherType, Mode>&& shared_ptr)
    : object_(shared_ptr.object_),
      shared_referencer_(MoveTemp(shared_ptr.shared_referencer_)) {
    shared_ptr.object_ = nullptr;

    // If this assert goes off, it means a shared reference was created from a shared pointer that was nullptr.
    // Shared references are never allowed to be null.  Consider using SharedPtr instead.
    fun_check(IsValid());
  }

  /**
   * Checks to see if this shared reference is actually pointing to an object.
   * NOTE: This validity test is intentionally private because shared references must always be valid.
   *
   * \return True if the shared reference is valid and can be dereferenced
   */
  FUN_ALWAYS_INLINE const bool IsValid() const {
    return !!object_;
  }

  /**
   * Computes a hash code for this object
   *
   * \param shared_ref - Shared pointer to compute hash code for
   *
   * \return Hash code value
   */
  friend uint32 HashOf(const SharedRef<ObjectType, Mode>& shared_ref) {
    return fun::PointerHash(shared_ref.object_);
  }

  //friend HashResult& operator << (HashResult& result, const SharedRef<ObjectType, Mode>& shared_ref)
  //{
  //  return result << fun::PointerHash(shared_ref.object_);
  //}

  // We declare ourselves as a friend (templated using OtherType) so we can access members as needed
  template <typename OtherType, SPMode OtherMode> friend class SharedRef;

  // Declare other smart pointer types as friends as needed
  template <typename OtherType, SPMode OtherMode> friend class SharedPtr;
  template <typename OtherType, SPMode OtherMode> friend class WeakPtr;

 private:
  /**
   * The object we're holding a reference to.  Can be nullptr.
   */
  ObjectType* object_;

  /**
   * Interface to the reference counter for this object.  Note that the actual reference
   * controller object is shared by all shared and weak pointers that refer to the object
   */
  SharedPtr_internal::SharedReferencer<Mode> shared_referencer_;
};


/**
 * SharedPtr is a non-intrusive reference-counted authoritative object pointer.  This shared pointer
 * will be conditionally thread-safe when the optional Mode template argument is set to ThreadSafe.
 */
template <typename ObjectType, SPMode Mode>
class SharedPtr {
  // TSharedPtrs with FObjects are illegal.
  //static_assert(!PointerIsConvertibleFromTo<ObjectType, const FObjectBase>::Value, "You cannot use SharedPtr or WeakPtr with FObjects. Consider a UProperty() pointer or TWeakObjectPtr.");

  enum {
    ObjectTypeHasSameModeSharedFromThis = PointerIsConvertibleFromTo<ObjectType, SharedFromThis<ObjectType, Mode>>::Value,
    ObjectTypeHasOppositeModeSharedFromThis = PointerIsConvertibleFromTo<ObjectType, SharedFromThis<ObjectType, (Mode == SPMode::NotThreadSafe) ? SPMode::ThreadSafe : SPMode::NotThreadSafe>>::Value
  };

  // SharedPtr of one mode to a type which has a SharedFromThis only of another mode is illegal.
  // A type which does not inherit SharedFromThis at all is ok.
  static_assert(SharedPtr::ObjectTypeHasSameModeSharedFromThis || !SharedPtr::ObjectTypeHasOppositeModeSharedFromThis, "You cannot use a SharedPtr of one mode with a type which inherits SharedFromThis of another mode.");

 public:
  /**
   * Constructs an empty shared pointer
   *
   * NOTE: NullTag parameter is an FUN extension to standard shared_ptr behavior
   */
  FUN_ALWAYS_INLINE SharedPtr(SharedPtr_internal::NullTag* = nullptr)
    : object_(nullptr), shared_referencer_() {}

  /**
   * Constructs a shared pointer that owns the specified object.  Note that passing nullptr here will
   * still create a tracked reference to a nullptr pointer. (Consistent with std::shared_ptr)
   *
   * \param object - object this shared pointer to retain a reference to
   */
  template <typename OtherType>
  FUN_ALWAYS_INLINE explicit SharedPtr(OtherType* object)
    : object_(object),
      shared_referencer_(SharedPtr_internal::CreateDefaultReferenceController(object)) {
    // If the object happens to be derived from SharedFromThis, the following method
    // will prime the object with a weak pointer to itself.
    SharedPtr_internal::EnableSharedFromThis(this, object, object);
  }

  /**
   * Constructs a shared pointer that owns the specified object.  Note that passing nullptr here will
   * still create a tracked reference to a nullptr pointer. (Consistent with std::shared_ptr)
   *
   * \param object - object_ this shared pointer to retain a reference to
   * \param deleter - Deleter object used to destroy the object when it is no longer referenced.
   */
  template <typename OtherType, typename DeleterType>
  FUN_ALWAYS_INLINE SharedPtr(OtherType* object, DeleterType&& deleter)
    : object_(object),
      shared_referencer_(SharedPtr_internal::CreateCustomReferenceController(object, Forward<DeleterType>(deleter))) {
    // If the object happens to be derived from SharedFromThis, the following method
    // will prime the object with a weak pointer to itself.
    SharedPtr_internal::EnableSharedFromThis(this, object, object);
  }

  /**
   * Constructs a shared pointer using a proxy reference to a raw pointer. (See MakeShareable())
   *
   * \param raw_ptr_proxy - Proxy raw pointer that contains the object that the new shared pointer will reference
   */
  // NOTE: The following is an FUN extension to standard shared_ptr behavior
  template <typename OtherType>
  FUN_ALWAYS_INLINE SharedPtr(SharedPtr_internal::RawPtrProxy<OtherType> const& raw_ptr_proxy)
    : object_(raw_ptr_proxy.object),
      shared_referencer_(raw_ptr_proxy.reference_controller) {
    // If the object happens to be derived from SharedFromThis, the following method
    // will prime the object with a weak pointer to itself.
    SharedPtr_internal::EnableSharedFromThis(this, raw_ptr_proxy.object, raw_ptr_proxy.object);
  }

  /**
   * Constructs a shared pointer as a shared reference to an existing shared pointer's object.
   * This constructor is needed so that we can implicitly upcast to base classes.
   *
   * \param shared_ptr - The shared pointer whose object we should create an additional reference to
   */
  template <typename OtherType>
  FUN_ALWAYS_INLINE SharedPtr(SharedPtr<OtherType, Mode> const& shared_ptr)
    : object_(shared_ptr.object_),
      shared_referencer_(shared_ptr.shared_referencer_) {}

  FUN_ALWAYS_INLINE SharedPtr(SharedPtr const& shared_ptr)
    : object_(shared_ptr.object_),
      shared_referencer_(shared_ptr.shared_referencer_) {}

  FUN_ALWAYS_INLINE SharedPtr(SharedPtr&& shared_ptr)
    : object_(shared_ptr.object_),
      shared_referencer_(MoveTemp(shared_ptr.shared_referencer_)) {
    shared_ptr.object_ = nullptr;
  }

  /**
   * Implicitly converts a shared reference to a shared pointer, adding a reference to the object.
   * NOTE: We allow an implicit conversion from SharedRef to SharedPtr because it's always a safe conversion.
   *
   * \param shared_ref - The shared reference that will be converted to a shared pointer
   *
   * NOTE: The following is an FUN extension to standard shared_ptr behavior
   */
  template <typename OtherType>
  FUN_ALWAYS_INLINE SharedPtr(SharedRef<OtherType, Mode> const& shared_ref)
    : object_(shared_ref.object_),
      shared_referencer_(shared_ref.shared_referencer_) {
    // There is no rvalue overload of this constructor, because 'stealing' the pointer from a
    // SharedRef would leave it as null, which would invalidate its invariant.
  }

  /**
   * Special constructor used internally to statically cast one shared pointer type to another.  You
   * should never call this constructor directly.  Instead, use the StaticCastSharedPtr() function.
   * This constructor creates a shared pointer as a shared reference to an existing shared pointer after
   * statically casting that pointer's object.  This constructor is needed for static casts.
   *
   * \param shared_ptr - The shared pointer whose object we should create an additional reference to
   */
  template <typename OtherType>
  FUN_ALWAYS_INLINE SharedPtr(SharedPtr<OtherType, Mode> const& shared_ptr, SharedPtr_internal::StaticCastTag)
    : object_(static_cast<ObjectType*>(shared_ptr.object_)),
      shared_referencer_(shared_ptr.shared_referencer_) {}

  /**
   * Special constructor used internally to cast a 'const' shared pointer a 'mutable' pointer.  You
   * should never call this constructor directly.  Instead, use the ConstCastSharedPtr() function.
   * This constructor creates a shared pointer as a shared reference to an existing shared pointer after
   * const casting that pointer's object.  This constructor is needed for const casts.
   *
   * \param shared_ptr - The shared pointer whose object we should create an additional reference to
   */
  template <typename OtherType>
  FUN_ALWAYS_INLINE SharedPtr(SharedPtr<OtherType, Mode> const& shared_ptr, SharedPtr_internal::ConstCastTag)
    : object_(const_cast<ObjectType*>(shared_ptr.object_)),
      shared_referencer_(shared_ptr.shared_referencer_) {}

  /**
   * Special constructor used internally to create a shared pointer from an existing shared pointer,
   * while using the specified object pointer instead of the incoming shared pointer's object
   * pointer.  This is used by with the SharedFromThis feature (by UpdateWeakReferenceInternal)
   *
   * \param other_shared_ptr - The shared pointer whose reference count
   * \param object - The object pointer to use (instead of the incoming shared pointer's object)
   */
  template <typename OtherType>
  FUN_ALWAYS_INLINE SharedPtr(SharedPtr<OtherType, Mode> const& other_shared_ptr, ObjectType* object)
    : object_(object),
      shared_referencer_(other_shared_ptr.shared_referencer_) {}

  /**
   * Assignment to a nullptr pointer.  The object currently referenced by this shared pointer will no longer be
   * referenced and will be deleted if there are no other referencers.
   * NOTE: The following is an FUN extension to standard shared_ptr behavior
   */
  FUN_ALWAYS_INLINE SharedPtr& operator = (SharedPtr_internal::NullTag*) {
    Reset();
    return *this;
  }

  /**
   * Assignment operator replaces this shared pointer with the specified shared pointer.  The object
   * currently referenced by this shared pointer will no longer be referenced and will be deleted if
   * there are no other referencers.
   *
   * \param shared_ptr - Shared pointer to replace with
   */
  FUN_ALWAYS_INLINE SharedPtr& operator = (SharedPtr const& shared_ptr) {
    shared_referencer_ = shared_ptr.shared_referencer_;
    object_ = shared_ptr.object_;
    return *this;
  }

  FUN_ALWAYS_INLINE SharedPtr& operator = (SharedPtr&& shared_ptr) {
    if (&shared_ptr != this) {
      object_ = shared_ptr.object_;
      shared_ptr.object_ = nullptr;
      shared_referencer_ = MoveTemp(shared_ptr.shared_referencer_);
    }
    return *this;
  }

  /**
   * Assignment operator replaces this shared pointer with the specified shared pointer.  The object
   * currently referenced by this shared pointer will no longer be referenced and will be deleted if
   * there are no other referencers.
   *
   * \param raw_ptr_proxy - Proxy object used to assign the object (see MakeShareable helper function)
   *
   * NOTE: The following is an FUN extension to standard shared_ptr behavior
   */
  template <typename OtherType>
  FUN_ALWAYS_INLINE SharedPtr& operator = (SharedPtr_internal::RawPtrProxy<OtherType> const& raw_ptr_proxy) {
    *this = SharedPtr<ObjectType, Mode>(raw_ptr_proxy);
    return *this;
  }

  /**
   * Converts a shared pointer to a shared reference.  The pointer *must* be valid or an assertion will trigger.
   *
   * \return Reference to the object
   *
   * NOTE: The following is an FUN extension to standard shared_ptr behavior
   */
  FUN_ALWAYS_INLINE SharedRef<ObjectType, Mode> ToSharedRef() const {
    // If this assert goes off, it means a shared reference was created from a shared pointer that was nullptr.
    // Shared references are never allowed to be null.  Consider using SharedPtr instead.
    fun_check(IsValid());
    return SharedRef<ObjectType, Mode>(*this);
  }

  /**
   * Returns the object referenced by this pointer, or nullptr if no object is reference
   *
   * \return The object owned by this shared pointer, or nullptr
   */
  FUN_ALWAYS_INLINE ObjectType* Get() const {
    return object_;
  }

  /**
   * Checks to see if this shared pointer is actually pointing to an object
   *
   * \return True if the shared pointer is valid and can be dereferenced
   */
  FUN_ALWAYS_INLINE const bool IsValid() const {
    return !!object_;
  }

  FUN_ALWAYS_INLINE explicit operator bool () const {
    return IsValid();
  }

  FUN_ALWAYS_INLINE bool operator ! () const {
    return !IsValid();
  }

  /**
   * Dereference operator returns a reference to the object this shared pointer points to
   *
   * \return Reference to the object
   */
  FUN_ALWAYS_INLINE typename MakeReferenceTo<ObjectType>::Type operator * () const {
    fun_check(IsValid());
    return *object_;
  }

  /**
   * Arrow operator returns a pointer to the object this shared pointer references
   *
   * \return Returns a pointer to the object referenced by this shared pointer
   */
  FUN_ALWAYS_INLINE ObjectType* operator -> () const {
    fun_check(IsValid());
    return object_;
  }

  /**
   * Resets this shared pointer, removing a reference to the object.
   * If there are no other shared references to the object
   * then it will be destroyed.
   */
  FUN_ALWAYS_INLINE void Reset() {
    *this = SharedPtr<ObjectType, Mode>();
  }

  template <typename OtherType>
  FUN_ALWAYS_INLINE void Reset(OtherType* new_reference) {
    *this = SharedPtr<OtherType, Mode>(new_reference);
  }

  /**
   * Returns the number of shared references to this object
   * (including this reference.)
   *
   * IMPORTANT: Not necessarily fast!  Should only be used for debugging purposes!
   *
   * \return Number of shared references to the object (including this reference.)
   */
  FUN_ALWAYS_INLINE const int32 GetSharedReferenceCount() const {
    return shared_referencer_.GetSharedReferenceCount();
  }

  /**
   * Returns true if this is the only shared reference to this object.  Note that there may be
   * outstanding weak references left.
   *
   * IMPORTANT: Not necessarily fast!  Should only be used for debugging purposes!
   *
   * \return True if there is only one shared reference to the object, and this is it!
   */
  FUN_ALWAYS_INLINE const bool IsUnique() const {
    return shared_referencer_.IsUnique();
  }

 private:
  /**
   * Constructs a shared pointer from a weak pointer, allowing you to access the object (if it
   * hasn't expired yet.)  Remember, if there are no more shared references to the object, the
   * shared pointer will not be valid.  You should always check to make sure this shared
   * pointer is valid before trying to dereference the shared pointer!
   *
   * NOTE: This constructor is private to force users to be explicit when converting a weak
   *       pointer to a shared pointer.  Use the weak pointer's Lock() method instead!
   */
  template <typename OtherType>
  FUN_ALWAYS_INLINE explicit SharedPtr(WeakPtr<OtherType, Mode> const& weak_ptr)
    : object_(nullptr),
      shared_referencer_(weak_ptr.weak_reference_count_) {
    // Check that the shared reference was created from the weak reference successfully.
    // We'll only cache a pointer to the object if we have a valid shared reference.
    if (shared_referencer_.IsValid()) {
      object_ = weak_ptr.object_;
    }
  }

  /**
   * Computes a hash code for this object
   *
   * \param shared_ptr  Shared pointer to compute hash code for
   *
   * \return Hash code value
   */
  friend uint32 HashOf(const SharedPtr<ObjectType, Mode>& shared_ptr) {
    return fun::PointerHash(shared_ptr.object_);
  }

  // We declare ourselves as a friend (templated using OtherType) so we can access members as needed
  template <typename OtherType, SPMode OtherMode> friend class SharedPtr;

  // Declare other smart pointer types as friends as needed
  template <typename OtherType, SPMode OtherMode> friend class SharedRef;
  template <typename OtherType, SPMode OtherMode> friend class WeakPtr;
  template <typename OtherType, SPMode OtherMode> friend class SharedFromThis;

 private:
  /** The object we're holding a reference to.  Can be nullptr. */
  ObjectType* object_;

  /**
   * Interface to the reference counter for this object.  Note that the actual reference
   * controller object is shared by all shared and weak pointers that refer to the object
   */
  SharedPtr_internal::SharedReferencer<Mode> shared_referencer_;
};

template <typename ObjectType, SPMode Mode>
struct IsZeroConstructible<SharedPtr<ObjectType, Mode>> { enum { Value = true }; };


/**
 * WeakPtr is a non-intrusive reference-counted weak object pointer.
 * This weak pointer will beconditionally thread-safe
 * when the optional Mode template argument is set to ThreadSafe.
 */
template <typename ObjectType, SPMode Mode>
class WeakPtr {
 public:
  /**
   * Constructs an empty WeakPtr
   *
   * NOTE: NullTag parameter is an FUN extension to standard shared_ptr behavior
   */
  FUN_ALWAYS_INLINE WeakPtr(SharedPtr_internal::NullTag* = nullptr)
    : object_(nullptr),
      weak_reference_count_() {}

  /**
   * Constructs a weak pointer from a shared reference
   *
   * \param shared_ref - The shared reference to create a weak pointer from
   *
   * NOTE: The following is an FUN extension to standard shared_ptr behavior
   */
  template <typename OtherType>
  FUN_ALWAYS_INLINE WeakPtr(SharedRef<OtherType, Mode> const& shared_ref)
    : object_(shared_ref.object_),
      weak_reference_count_(shared_ref.shared_referencer_) {}

  /**
   * Constructs a weak pointer from a shared pointer
   *
   * \param shared_ptr - The shared pointer to create a weak pointer from
   */
  template <typename OtherType>
  FUN_ALWAYS_INLINE WeakPtr(SharedPtr<OtherType, Mode> const& shared_ptr)
    : object_(shared_ptr.object_),
      weak_reference_count_(shared_ptr.shared_referencer_) {}

  /**
   * Constructs a weak pointer from a weak pointer of another type.
   * This constructor is intended to allow derived-to-base conversions.
   *
   * \param weak_ptr - The weak pointer to create a weak pointer from
   */
  template <typename OtherType>
  FUN_ALWAYS_INLINE WeakPtr(WeakPtr<OtherType, Mode> const& weak_ptr)
    : object_(weak_ptr.object_),
      weak_reference_count_(weak_ptr.weak_reference_count_) {}

  template <typename OtherType>
  FUN_ALWAYS_INLINE WeakPtr(WeakPtr<OtherType, Mode>&& weak_ptr)
    : object_(weak_ptr.object_),
      weak_reference_count_(MoveTemp(weak_ptr.weak_reference_count_)) {
    weak_ptr.object_ = nullptr;
  }

  FUN_ALWAYS_INLINE WeakPtr(WeakPtr const& weak_ptr)
    : object_(weak_ptr.object_),
      weak_reference_count_(weak_ptr.weak_reference_count_) {}

  FUN_ALWAYS_INLINE WeakPtr(WeakPtr&& weak_ptr)
    : object_(weak_ptr.object_),
      weak_reference_count_(MoveTemp(weak_ptr.weak_reference_count_)) {
    weak_ptr.object_ = nullptr;
  }

  /**
   * Assignment to a nullptr pointer.  Clears this weak pointer's reference.
   *
   * NOTE: The following is an FUN extension to standard shared_ptr behavior
   */
  FUN_ALWAYS_INLINE WeakPtr& operator = (SharedPtr_internal::NullTag*) {
    Reset();
    return *this;
  }

  /**
   * Assignment operator adds a weak reference to
   * the object referenced by the specified weak pointer
   *
   * \param weak_ptr - The weak pointer for the object to assign
   */
  FUN_ALWAYS_INLINE WeakPtr& operator = (WeakPtr const& weak_ptr) {
    object_ = weak_ptr.Lock().Get();
    weak_reference_count_ = weak_ptr.weak_reference_count_;
    return *this;
  }

  FUN_ALWAYS_INLINE WeakPtr& operator = (WeakPtr&& weak_ptr) {
    if (FUN_LIKELY(&weak_ptr != this)) {
      object_ = weak_ptr.object_;
      weak_ptr.object_ = nullptr;
      weak_reference_count_ = MoveTemp(weak_ptr.weak_reference_count_);
    }
    return *this;
  }

  /**
   * Assignment operator adds a weak reference to
   * the object referenced by the specified weak pointer.
   * This assignment operator is intended to allow derived-to-base conversions.
   *
   * \param weak_ptr - The weak pointer for the object to assign
   */
  template <typename OtherType>
  FUN_ALWAYS_INLINE WeakPtr& operator = (WeakPtr<OtherType, Mode> const& weak_ptr) {
    object_ = weak_ptr.Lock().Get();
    weak_reference_count_ = weak_ptr.weak_reference_count_;
    return *this;
  }

  template <typename OtherType>
  FUN_ALWAYS_INLINE WeakPtr& operator = (WeakPtr<OtherType, Mode>&& weak_ptr) {
    object_ = weak_ptr.object_;
    weak_ptr.object_ = nullptr;
    weak_reference_count_ = MoveTemp(weak_ptr.weak_reference_count_);
    return *this;
  }

  /**
   * Assignment operator sets this weak pointer from a shared reference
   *
   * \param shared_ref - The shared reference used to assign to this weak pointer
   *
   * NOTE: The following is an FUN extension to standard shared_ptr behavior
   */
  template <typename OtherType>
  FUN_ALWAYS_INLINE WeakPtr& operator = (SharedRef<OtherType, Mode> const& shared_ref) {
    object_ = shared_ref.object_;
    weak_reference_count_ = shared_ref.shared_referencer_;
    return *this;
  }

  /**
   * Assignment operator sets this weak pointer from a shared pointer
   *
   * \param shared_ptr - The shared pointer used to assign to this weak pointer
   */
  template <typename OtherType>
  FUN_ALWAYS_INLINE WeakPtr& operator = (SharedPtr<OtherType, Mode> const& shared_ptr) {
    object_ = shared_ptr.object_;
    weak_reference_count_ = shared_ptr.shared_referencer_;
    return *this;
  }

  /**
   * Converts this weak pointer to a shared pointer that
   * you can use to access the object (if it* hasn't expired yet.)
   * Remember, if there are no more shared references to the object, the
   * returned shared pointer will not be valid.  You should always
   * check to make sure the returned pointer is valid before trying to
   * dereference the shared pointer!
   *
   * \return Shared pointer for this object (will only be valid if still referenced!)
   */
  FUN_ALWAYS_INLINE SharedPtr<ObjectType, Mode> Lock() const {
    return SharedPtr<ObjectType, Mode>(*this);
  }

  /**
   * Checks to see if this weak pointer actually has
   * a valid reference to an object
   *
   * \return True if the weak pointer is valid and a pin operator would have succeeded
   */
  FUN_ALWAYS_INLINE const bool IsValid() const {
    return object_ && weak_reference_count_.IsValid();
  }

  FUN_ALWAYS_INLINE explicit operator bool () const {
    return IsValid();
  }

  FUN_ALWAYS_INLINE bool operator ! () const {
    return !IsValid();
  }

  /**
   * Resets this weak pointer, removing a weak reference to the object.
   * If there are no other shared or weak references to the object,
   * then the tracking object will be destroyed.
   */
  FUN_ALWAYS_INLINE void Reset() {
    *this = WeakPtr<ObjectType, Mode>();
  }

  /**
   * Returns true if the object this weak pointer points to
   * is the same as the specified object pointer.
   */
  FUN_ALWAYS_INLINE bool HasSameObject(const void* ptr) const {
    return Lock().Get() == ptr;
  }

 private:
  /**
   * Computes a hash code for this object
   *
   * \param weak_ptr - Weak pointer to compute hash code for
   *
   * \return Hash code value
   */
  friend uint32 HashOf(const WeakPtr<ObjectType, Mode>& weak_ptr) {
    return PointerHash(weak_ptr.object_);
  }

  // We declare ourselves as a friend (templated using OtherType) so we can access members as needed
  template <typename OtherType, SPMode OtherMode> friend class WeakPtr;

  // Declare ourselves as a friend of SharedPtr so we can access members as needed
  template <typename OtherType, SPMode OtherMode> friend class SharedPtr;

 private:
  /**
   * The object we have a weak reference to.  Can be nullptr.
   * Also, it's important to note that because* this is a weak reference,
   * the object this pointer points to may have already been destroyed.
   */
  ObjectType* object_;

  /**
   * Interface to the reference counter for this object.
   * Note that the actual reference controller object is shared by
   * all shared and weak pointers that refer to the object
   */
  SharedPtr_internal::WeakReferencer<Mode> weak_reference_count_;
};

template <typename T, SPMode Mode> struct IsWeakPointerType<WeakPtr<T, Mode>> { enum { Value = true }; };
template <typename T, SPMode Mode> struct IsZeroConstructible<WeakPtr<T, Mode>> { enum { Value = true }; };


/**
 * Derive your class from SharedFromThis to enable access to
 * a SharedRef directly from an object instance that's already been allocated.
 * Use the optional Mode template argument for thread-safety.
 */
template <typename ObjectType, SPMode Mode>
class SharedFromThis {
 public:
  /**
   * Provides access to a shared reference to this object.
   * Note that is only valid to call this after a shared reference
   * (or shared pointer) to the object has already been created.
   * Also note that it is illegal to call this in the object's destructor.
   *
   * \return Returns this object as a shared pointer
   */
  SharedRef<ObjectType, Mode> AsShared() {
    SharedPtr<ObjectType, Mode> shared_this(weak_this_.Lock());

    // If the following assert goes off, it means one of the following:
    //
    //     - You tried to request a shared pointer before the object was ever assigned to one. (e.g. constructor)
    //     - You tried to request a shared pointer while the object is being destroyed (destructor chain)
    //
    // To fix this, make sure you create at least one shared reference to your object instance before requested,
    // and also avoid calling this function from your object's destructor.
    //
    fun_check(shared_this.Get() == this);

    // Now that we've verified the shared pointer is valid, we'll convert it to a shared reference
    // and return it!
    return shared_this.ToSharedRef();
  }

  /**
   * Provides access to a shared reference to this object (const.)
   * Note that is only valid to call this after a shared reference
   * (or shared pointer) to the object has already been created.
   * Also note that it is illegal to call this in the object's destructor.
   *
   * \return Returns this object as a shared pointer (const)
   */
  SharedRef<ObjectType const, Mode> AsShared() const {
    SharedPtr<ObjectType const, Mode> shared_this(weak_this_);

    // If the following assert goes off, it means one of the following:
    //
    //     - You tried to request a shared pointer before the object was ever assigned to one. (e.g. constructor)
    //     - You tried to request a shared pointer while the object is being destroyed (destructor chain)
    //
    // To fix this, make sure you create at least one shared reference to your object instance before requested,
    // and also avoid calling this function from your object's destructor.
    //
    fun_check(shared_this.Get() == this);

    // Now that we've verified the shared pointer is valid, we'll convert it to a shared reference
    // and return it!
    return shared_this.ToSharedRef();
  }

 protected:
  /**
   * Provides access to a shared reference to an object,
   * given the object's 'this' pointer.  Uses the 'this' pointer to derive
   * the object's actual type, then casts and returns an appropriately
   * typed shared reference.  Intentionally declared 'protected',
   * as should only be called when the 'this' pointer can be passed.
   *
   * \return Returns this object as a shared pointer
   */
  template <typename OtherType>
  FUN_ALWAYS_INLINE static SharedRef<OtherType, Mode> SharedThis(OtherType* this_ptr) {
    return StaticCastSharedRef<OtherType>(this_ptr->AsShared());
  }

  /**
   * Provides access to a shared reference to an object,
   * given the object's 'this' pointer. Uses the 'this' pointer to derive
   * the object's actual type, then casts and returns
   * an appropriately typed shared reference.
   * Intentionally declared 'protected', as should only be called when the
   * 'this' pointer can be passed.
   *
   * \return Returns this object as a shared pointer (const)
   */
  template <typename OtherType>
  FUN_ALWAYS_INLINE static SharedRef<OtherType const, Mode> SharedThis(const OtherType* this_ptr) {
    return StaticCastSharedRef<OtherType const>(this_ptr->AsShared());
  }

 public: // @todo: Ideally this would be private, but template sharing problems prevent it
  /**
   * INTERNAL USE ONLY -- Do not call this method.
   * Freshens the internal weak pointer object using
   * the supplied object pointer along with the authoritative
   * shared reference to the object.
   *
   * Note that until this function is called, calls to AsShared()
   * will result in an empty pointer.
   */
  template <typename SharedPtrType, typename OtherType>
  FUN_ALWAYS_INLINE void UpdateWeakReferenceInternal(SharedPtr<SharedPtrType, Mode> const* shared_ptr, OtherType* object) const {
    if (!weak_this_.IsValid()) {
      weak_this_ = SharedPtr<ObjectType, Mode>(*shared_ptr, object);
    }
  }

  /**
   * INTERNAL USE ONLY -- Do not call this method.
   * Freshens the internal weak pointer object using
   * the supplied object pointer along with the authoritative
   * shared reference to the object.
   *
   * Note that until this function is called, calls to AsShared()
   * will result in an empty pointer.
   */
  template <typename SharedRefType, typename OtherType>
  FUN_ALWAYS_INLINE void UpdateWeakReferenceInternal(SharedRef<SharedRefType, Mode> const* shared_ref, OtherType* object) const {
    if (!weak_this_.IsValid()) {
      weak_this_ = SharedRef<ObjectType, Mode>(*shared_ref, object);
    }
  }

  /**
   * Checks whether given instance has been already made sharable
   * (use in checks to detect when it happened,
   * since it's a straight way to crashing)
   */
  FUN_ALWAYS_INLINE bool HasBeenAlreadyMadeSharable() const {
    return weak_this_.IsValid();
  }

 protected:
  /**
   * Hidden stub constructor.
   */
  SharedFromThis() {}

  /**
   * Hidden stub copy constructor.
   */
  SharedFromThis(SharedFromThis const&) {}

  /**
   * Hidden stub assignment operator.
   */
  FUN_ALWAYS_INLINE SharedFromThis& operator = (SharedFromThis const&) { return *this; }

  /**
   * Hidden destructor.
   */
  ~SharedFromThis() {}

 private:
  /**
   * Weak reference to ourselves.  If we're destroyed then
   * this weak pointer reference will be destructed with ourselves.
   * Note this is declared mutable only so that
   * UpdateWeakReferenceInternal() can update it.
   */
  mutable WeakPtr<ObjectType, Mode> weak_this_;
};


/**
 * Global equality operator for SharedRef
 *
 * \return True if the two shared references are equal
 */
template <typename ObjectTypeA, typename ObjectTypeB, SPMode Mode>
FUN_ALWAYS_INLINE bool operator == (SharedRef<ObjectTypeA, Mode> const& shared_ref_a, SharedRef<ObjectTypeB, Mode> const& shared_ref_b) {
  return &(shared_ref_a.Get()) == &(shared_ref_b.Get());
}

/**
 * Global inequality operator for SharedRef
 *
 * \return True if the two shared references are not equal
 */
template <typename ObjectTypeA, typename ObjectTypeB, SPMode Mode>
FUN_ALWAYS_INLINE bool operator != (SharedRef<ObjectTypeA, Mode> const& shared_ref_a, SharedRef<ObjectTypeB, Mode> const& shared_ref_b) {
  return &(shared_ref_a.Get()) != &(shared_ref_b.Get());
}

/**
 * Global equality operator for SharedPtr
 *
 * \return True if the two shared pointers are equal
 */
template <typename ObjectTypeA, typename ObjectTypeB, SPMode Mode>
FUN_ALWAYS_INLINE bool operator == (SharedPtr<ObjectTypeA, Mode> const& shared_ptr_a, SharedPtr<ObjectTypeB, Mode> const& shared_ptr_b) {
  return shared_ptr_a.Get() == shared_ptr_b.Get();
}

/**
 * Global inequality operator for SharedPtr
 *
 * \return True if the two shared pointers are not equal
 */
template <typename ObjectTypeA, typename ObjectTypeB, SPMode Mode>
FUN_ALWAYS_INLINE bool operator != (SharedPtr<ObjectTypeA, Mode> const& shared_ptr_a, SharedPtr<ObjectTypeB, Mode> const& shared_ptr_b) {
  return shared_ptr_a.Get() != shared_ptr_b.Get();
}

/**
 * Tests to see if a SharedRef is "equal" to a SharedPtr
 * (both are valid and refer to the same object)
 *
 * \return True if the shared reference and shared pointer are "equal"
 */
template <typename ObjectTypeA, typename ObjectTypeB, SPMode Mode>
FUN_ALWAYS_INLINE bool operator == (SharedRef<ObjectTypeA, Mode> const& shared_ref, SharedPtr<ObjectTypeB, Mode> const& shared_ptr) {
  return shared_ptr.IsValid() && shared_ptr.Get() == &(shared_ref.Get());
}

/**
 * Tests to see if a SharedRef is not "equal" to a SharedPtr
 * (shared pointer is invalid, or both refer to different objects)
 *
 * \return True if the shared reference and shared pointer are not "equal"
 */
template <typename ObjectTypeA, typename ObjectTypeB, SPMode Mode>
FUN_ALWAYS_INLINE bool operator != (SharedRef<ObjectTypeA, Mode> const& shared_ref, SharedPtr<ObjectTypeB, Mode> const& shared_ptr) {
  return !shared_ptr.IsValid() || (shared_ptr.Get() != &(shared_ref.Get()));
}

/**
 * Tests to see if a SharedRef is "equal" to a SharedPtr
 * (both are valid and refer to the same object) (reverse)
 *
 * \return True if the shared reference and shared pointer are "equal"
 */
template <typename ObjectTypeA, typename ObjectTypeB, SPMode Mode>
FUN_ALWAYS_INLINE bool operator == (SharedPtr<ObjectTypeB, Mode> const& shared_ptr, SharedRef<ObjectTypeA, Mode> const& shared_ref) {
  return shared_ref == shared_ptr;
}

/**
 * Tests to see if a SharedRef is not "equal" to a SharedPtr
 * (shared pointer is invalid, or both refer to different objects) (reverse)
 *
 * \return True if the shared reference and shared pointer are not "equal"
 */
template <typename ObjectTypeA, typename ObjectTypeB, SPMode Mode>
FUN_ALWAYS_INLINE bool operator != (SharedPtr<ObjectTypeB, Mode> const& shared_ptr, SharedRef<ObjectTypeA, Mode> const& shared_ref) {
  return shared_ref != shared_ptr;
}

/**
 * Global equality operator for WeakPtr
 *
 * \return True if the two weak pointers are equal
 */
template <typename ObjectTypeA, typename ObjectTypeB, SPMode Mode>
FUN_ALWAYS_INLINE bool operator == (WeakPtr<ObjectTypeA, Mode> const& weak_ptr_a, WeakPtr<ObjectTypeB, Mode> const& weak_ptr_b) {
  return weak_ptr_a.Lock().Get() == weak_ptr_b.Lock().Get();
}

/**
 * Global equality operator for WeakPtr
 *
 * \return True if the weak pointer and the shared ref are equal
 */
template <typename ObjectTypeA, typename ObjectTypeB, SPMode Mode>
FUN_ALWAYS_INLINE bool operator == (WeakPtr<ObjectTypeA, Mode> const& weak_ptr_a, SharedRef<ObjectTypeB, Mode> const& shared_ref_b) {
  return weak_ptr_a.Lock().Get() == &shared_ref_b.Get();
}

/**
 * Global equality operator for WeakPtr
 *
 * \return True if the weak pointer and the shared ptr are equal
 */
template <typename ObjectTypeA, typename ObjectTypeB, SPMode Mode>
FUN_ALWAYS_INLINE bool operator == (WeakPtr<ObjectTypeA, Mode> const& weak_ptr_a, SharedPtr<ObjectTypeB, Mode> const& shared_ptr_b) {
  return weak_ptr_a.Lock().Get() == shared_ptr_b.Get();
}

/**
 * Global equality operator for WeakPtr
 *
 * \return True if the weak pointer and the shared ref are equal
 */
template <typename ObjectTypeA, typename ObjectTypeB, SPMode Mode>
FUN_ALWAYS_INLINE bool operator == (SharedRef<ObjectTypeA, Mode> const& shared_ref_a, WeakPtr<ObjectTypeB, Mode> const& weak_ptr_b) {
  return &shared_ref_a.Get() == weak_ptr_b.Lock().Get();
}

/**
 * Global equality operator for WeakPtr
 *
 * \return True if the weak pointer and the shared ptr are equal
 */
template <typename ObjectTypeA, typename ObjectTypeB, SPMode Mode>
FUN_ALWAYS_INLINE bool operator == (SharedPtr<ObjectTypeA, Mode> const& shared_ptr_a, WeakPtr<ObjectTypeB, Mode> const& weak_ptr_b) {
  return shared_ptr_a.Get() == weak_ptr_b.Lock().Get();
}

/**
 * Global equality operator for WeakPtr
 *
 * \return True if the weak pointer is null
 */
template <typename ObjectTypeA, SPMode Mode>
FUN_ALWAYS_INLINE bool operator == (WeakPtr<ObjectTypeA, Mode> const& weak_ptr_a, decltype(nullptr)) {
  return !weak_ptr_a.IsValid();
}

/**
 * Global equality operator for WeakPtr
 *
 * \return True if the weak pointer is null
 */
template <typename ObjectTypeB, SPMode Mode>
FUN_ALWAYS_INLINE bool operator == (decltype(nullptr), WeakPtr<ObjectTypeB, Mode> const& weak_ptr_b) {
  return !weak_ptr_b.IsValid();
}

/**
 * Global inequality operator for WeakPtr
 *
 * \return True if the two weak pointers are not equal
 */
template <typename ObjectTypeA, typename ObjectTypeB, SPMode Mode>
FUN_ALWAYS_INLINE bool operator != (WeakPtr<ObjectTypeA, Mode> const& weak_ptr_a, WeakPtr<ObjectTypeB, Mode> const& weak_ptr_b) {
  return weak_ptr_a.Lock().Get() != weak_ptr_b.Lock().Get();
}

/**
 * Global equality operator for WeakPtr
 *
 * \return True if the weak pointer and the shared ref are not equal
 */
template <typename ObjectTypeA, typename ObjectTypeB, SPMode Mode>
FUN_ALWAYS_INLINE bool operator != (WeakPtr<ObjectTypeA, Mode> const& weak_ptr_a, SharedRef<ObjectTypeB, Mode> const& shared_ref_b) {
  return weak_ptr_a.Lock().Get() != &shared_ref_b.Get();
}

/**
 * Global equality operator for WeakPtr
 *
 * \return True if the weak pointer and the shared ptr are not equal
 */
template <typename ObjectTypeA, typename ObjectTypeB, SPMode Mode>
FUN_ALWAYS_INLINE bool operator != (WeakPtr<ObjectTypeA, Mode> const& weak_ptr_a, SharedPtr<ObjectTypeB, Mode> const& shared_ptr_b) {
  return weak_ptr_a.Lock().Get() != shared_ptr_b.Get();
}

/**
 * Global equality operator for WeakPtr
 *
 * \return True if the weak pointer and the shared ref are not equal
 */
template <typename ObjectTypeA, typename ObjectTypeB, SPMode Mode>
FUN_ALWAYS_INLINE bool operator != (SharedRef<ObjectTypeA, Mode> const& shared_ref_a, WeakPtr<ObjectTypeB, Mode> const& weak_ptr_b) {
  return &shared_ref_a.Get() != weak_ptr_b.Lock().Get();
}

/**
 * Global equality operator for WeakPtr
 *
 * \return True if the weak pointer and the shared ptr are not equal
 */
template <typename ObjectTypeA, typename ObjectTypeB, SPMode Mode>
FUN_ALWAYS_INLINE bool operator != (SharedPtr<ObjectTypeA, Mode> const& shared_ptr_a, WeakPtr<ObjectTypeB, Mode> const& weak_ptr_b) {
  return shared_ptr_a.Get() != weak_ptr_b.Lock().Get();
}

/**
 * Global inequality operator for WeakPtr
 *
 * \return True if the weak pointer is not null
 */
template <typename ObjectTypeA, SPMode Mode>
FUN_ALWAYS_INLINE bool operator != (WeakPtr<ObjectTypeA, Mode> const& weak_ptr_a, decltype(nullptr)) {
  return weak_ptr_a.IsValid();
}

/**
 * Global inequality operator for WeakPtr
 *
 * \return True if the weak pointer is not null
 */
template <typename ObjectTypeB, SPMode Mode>
FUN_ALWAYS_INLINE bool operator != (decltype(nullptr), WeakPtr<ObjectTypeB, Mode> const& weak_ptr_b) {
  return weak_ptr_b.IsValid();
}

/**
 * Casts a shared pointer of one type to another type.
 * (static_cast)  Useful for down-casting.
 *
 * \param shared_ptr - The shared pointer to cast
 */
template <typename CastToType, typename CastFromType, SPMode Mode>
FUN_ALWAYS_INLINE SharedPtr<CastToType, Mode> StaticCastSharedPtr(SharedPtr<CastFromType, Mode> const& shared_ptr) {
  return SharedPtr<CastToType, Mode>(shared_ptr, SharedPtr_internal::StaticCastTag());
}

/**
 * Casts a 'const' shared reference to 'mutable' shared reference. (const_cast)
 *
 * \param shared_ref - The shared reference to cast
 */
template <typename CastToType, typename CastFromType, SPMode Mode>
FUN_ALWAYS_INLINE SharedRef<CastToType, Mode> ConstCastSharedRef(SharedRef<CastFromType, Mode> const& shared_ref) {
  return SharedRef<CastToType, Mode>(shared_ref, SharedPtr_internal::ConstCastTag());
}

/**
 * Casts a 'const' shared pointer to 'mutable' shared pointer. (const_cast)
 *
 * \param shared_ptr - The shared pointer to cast
 */
template <typename CastToType, typename CastFromType, SPMode Mode>
FUN_ALWAYS_INLINE SharedPtr<CastToType, Mode> ConstCastSharedPtr(SharedPtr<CastFromType, Mode> const& shared_ptr) {
  return SharedPtr<CastToType, Mode>(shared_ptr, SharedPtr_internal::ConstCastTag());
}

/**
 * MakeShareable utility function.  Wrap object pointers with MakeShareable
 * to allow them to be implicitly
 * converted to shared pointers!  This is useful in assignment operations,
 * or when returning a shared pointer from a function.
 *
 * NOTE: The following is an FUN extension to standard shared_ptr behavior
 */
template <typename ObjectType>
FUN_ALWAYS_INLINE SharedPtr_internal::RawPtrProxy<ObjectType> MakeShareable(ObjectType* object) {
  return SharedPtr_internal::RawPtrProxy<ObjectType>(object);
}

/**
 * MakeShareable utility function.  Wrap object pointers with MakeShareable
 * to allow them to be implicitly
 * converted to shared pointers!  This is useful in assignment operations,
 * or when returning a shared pointer from a function.
 *
 * NOTE: The following is an FUN extension to standard shared_ptr behavior
 */
template <typename ObjectType, typename DeleterType>
FUN_ALWAYS_INLINE SharedPtr_internal::RawPtrProxy<ObjectType> MakeShareable(ObjectType* object, DeleterType&& deleter) {
  return SharedPtr_internal::RawPtrProxy<ObjectType>(object, Forward<DeleterType>(deleter));
}

//TODO Array, Map이 정상화되면 다시 풀어주어야함!!!
//TODO Array, Map이 정상화되면 다시 풀어주어야함!!!
//TODO Array, Map이 정상화되면 다시 풀어주어야함!!!
//TODO Array, Map이 정상화되면 다시 풀어주어야함!!!
#if 0

/**
 * Given a Array of WeakPtr's, will remove any invalid pointers.
 *
 * \param pointers - The pointer array to prune invalid pointers out of
 */
template <typename T>
FUN_ALWAYS_INLINE void CleanupPointerArray(Array<WeakPtr<T>>& pointers) {
  Array<WeakPtr<T>> cleanuped;
  for (int32 i = 0; i < pointers.Count(); ++i) {
    if (pointers[i].IsValid()) {
      cleanuped.Add(pointers[i]);
    }
  }
  pointers = cleanuped;
}

/**
 * Given a Map of WeakPtr's, will remove any invalid pointers.
 * Not the most efficient.
 *
 * \param map - The pointer map to prune invalid pointers out of
 */
template <typename KeyType, typename ValueType>
FUN_ALWAYS_INLINE void CleanupPointerMap(Map<WeakPtr<KeyType>, ValueType>& map) {
  Map<WeakPtr<KeyType>, ValueType> cleanuped;
  for (typename Map<WeakPtr<KeyType>, ValueType>::ConstIterator op(map); op; ++op) {
    const WeakPtr<KeyType> weak_ptr = op.Key();
    if (weak_ptr.IsValid()) {
      cleanuped.Add(weak_ptr, op.Value());
    }
  }
  map = cleanuped;
}

#endif

} // namespace fun
