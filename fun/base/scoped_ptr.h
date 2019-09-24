#pragma once

#include "fun/base/base.h"
#include "fun/base/serialization/archive.h"

namespace fun {

/**
 * Wrapper around a raw pointer that destroys it automatically.
 * Calls operator delete on the object, so it must have been allocated with
 * operator new. Modeled after boost::scoped_ptr.
 *
 * If a custom deallocator is needed, this class will have to
 * expanded with a deletion policy.
 */
template <typename T>
class ScopedPtr {
 public:
  /**
   * Initialization constructor.
   */
  explicit ScopedPtr(T* ptr = nullptr) : ptr_(ptr) {}

  /**
   * Copy constructor.
   */
  ScopedPtr(const ScopedPtr& rhs) {
    ptr_ = rhs.ptr_ ? new T(*rhs.ptr_) : nullptr;
  }

  /**
   * Destructor.
   */
  ~ScopedPtr() {
    delete ptr_;
    ptr_ = nullptr;
  }

  /**
   * Assignment operator.
   */
  ScopedPtr& operator = (const ScopedPtr& rhs) {
    if (FUN_LIKELY(&rhs != this)) {
      delete ptr_;
      ptr_ = rhs.ptr_ ? new T(*rhs.ptr_) : nullptr;
    }

    return *this;
  }

  /**
   * Assignment operator.
   */
  ScopedPtr& operator = (T* ptr) {
    Reset(ptr);
    return *this;
  }

  // Dereferencing operators.
  T& operator * () const {
    fun_check_ptr(ptr_);
    return *ptr_;
  }

  T* operator -> () const {
    fun_check_ptr(ptr_);
    return ptr_;
  }

  T* Get() const {
    return ptr_;
  }

  /**
   * Returns true if the pointer is valid
   */
  bool IsValid() const {
    return !!ptr_;
  }

  FUN_ALWAYS_INLINE explicit operator bool () const {
    return !!ptr_;
  }

  FUN_ALWAYS_INLINE bool operator ! () const {
    return ptr_ == nullptr;
  }

  // implicit conversion to the reference type.
  operator T* () const {
    return ptr_;
  }

  void Swap(ScopedPtr& other) {
    T* tmp = other.ptr_;
    other.ptr_ = ptr_;
    ptr_ = tmp;
  }

  /**
   * Deletes the current pointer and sets it to a new value.
   */
  void Reset(T* ptr = nullptr) {
    fun_check(ptr_ == nullptr || ptr_ != ptr);
    delete ptr_;
    ptr_ = ptr;
  }

  /**
   * Releases the owned pointer and returns it so it doesn't get deleted.
   */
  T* Release() {
    T* ret = Get();
    ptr_ = nullptr;
    return ret;
  }

  // Serializer.
  friend Archive& operator & (Archive& ar, ScopedPtr& v) {
    if (ar.IsLoading()) {
      // When loading, allocate a new value.
      T* old_ptr = v.ptr_;
      v.ptr_ = new T;

      // Delete the old value.
      delete old_ptr;
    }

    // Serialize the value.  The caller of this serializer is responsible to only serialize for saving non-nullptr pointers.
    fun_check_ptr(v.ptr_);
    ar & *v.ptr_;

    return ar;
  }

 private:
  T* ptr_;
};


/**
 * specialize container traits
 */
template <typename T>
struct TypeTraits<ScopedPtr<T>> : public TypeTraitsBase<ScopedPtr<T>> {
  typedef T* ConstInitType;
  typedef T* ConstPointerType;
};


/**
 * Implement movement of a scoped pointer to avoid copying the referenced value.
 */
template <typename T>
void Move(ScopedPtr<T>& x, T* y) {
  x.Reset(y);
}


/**
 * Wrapper around a raw pointer that destroys it automatically.
 * Calls operator delete on the object, so it must have been allocated with
 * operator new.
 *
 * Same as ScopedPtr, except never calls new to make a duplicate
 */
template <typename T>
class AutoPtr {
 public:
  /**
   * Initialization constructor.
   */
  explicit AutoPtr(T* ptr = nullptr) : ptr_(ptr) {}

  /**
   * Destructor.
   */
  ~AutoPtr() {
    delete ptr_;
    ptr_ = nullptr;
  }

  /**
   * Assignment operator.
   */
  AutoPtr& operator = (T* ptr) {
    Reset(ptr);
    return *this;
  }


  //
  // Dereferencing operators.
  //

  T& operator * () const {
    fun_check_ptr(ptr_);
    return *ptr_;
  }

  T* operator -> () const {
    fun_check_ptr(ptr_);
    return ptr_;
  }

  T* GetOwnedPointer() const {
    return ptr_;
  }

  /**
   * Returns true if the pointer is valid
   */
  bool IsValid() const {
    return !!ptr_;
  }

  FUN_ALWAYS_INLINE explicit operator bool () const {
    return IsValid();
  }

  FUN_ALWAYS_INLINE bool operator ! () const {
    return !IsValid();
  }

  /**
   * implicit conversion to the reference type.
   */
  operator T* () const {
    return ptr_;
  }

  void Swap(AutoPtr& other) {
    if (FUN_LIKELY(&other != this)) {
      T* tmp = other.ptr_;
      other.ptr_ = ptr_;
      ptr_ = tmp;
    }
  }

  /**
   * Deletes the current pointer and sets it to a new value.
   */
  void Reset(T* ptr = nullptr) {
    fun_check(ptr_ == nullptr || ptr_ != ptr);
    delete ptr_;
    ptr_ = ptr;
  }

 private:
  T* ptr_;
};


/**
 * specialize container traits
 */
template <typename T>
struct TypeTraits<AutoPtr<T>> : public TypeTraitsBase<AutoPtr<T>> {
  typedef T* ConstInitType;
  typedef T* ConstPointerType;
};


/**
 * Implement movement of a scoped pointer to avoid copying the referenced value.
 */
template <typename T>
void Move(AutoPtr<T>& x, T* y) {
  x.Reset(y);
}

} // namespace fun
