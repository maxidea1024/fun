#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/template.h"
#include "fun/base/ftl/type_traits.h"
#include "fun/base/ftl/default_delete.h"

namespace fun {

/**
 * Single-ownership smart pointer in the vein of std::unique_ptr.
 * Use this when you need an object's lifetime to be strictly bound to
 * the lifetime of a single smart pointer.
 *
 * This class is non-copyable - ownership can only be transferred via a move operation, e.g.:
 *
 * UniquePtr<MyClass> ptr1(new MyClass);    // The MyClass object is owned by ptr1.
 * UniquePtr<MyClass> ptr2(ptr1);           // Error - UniquePtr is not copyable
 * UniquePtr<MyClass> ptr3(MoveTemp(ptr1)); // ptr3 now owns the MyClass object - ptr1 is now nullptr.
 */
template <typename T, typename DeleterType = DefaultDelete<T>>
class UniquePtr : private DeleterType {
  template <typename OtherType, typename OtherDeleterType>
  friend class UniquePtr;

 public:
  FUN_ALWAYS_INLINE UniquePtr() : ptr_(nullptr) {}

  explicit FUN_ALWAYS_INLINE UniquePtr(T* ptr) : ptr_(ptr) {}

  FUN_ALWAYS_INLINE UniquePtr(decltype(nullptr)) : ptr_(nullptr) {}

  FUN_ALWAYS_INLINE UniquePtr(UniquePtr&& other)
    : DeleterType(MoveTemp(other.GetDeleter())),
      ptr_(other.ptr_) {
    other.ptr_ = nullptr;
  }

  template <
      typename OtherType,
      typename OtherDeleterType,
      typename = typename EnableIf<!IsCppArray<OtherType>::Value>::Type
    >
  FUN_ALWAYS_INLINE UniquePtr(UniquePtr<OtherType, OtherDeleterType>&& other)
    : DeleterType(MoveTemp(other.GetDeleter())),
      ptr_(other.ptr_) {
    other.ptr_ = nullptr;
  }

  FUN_ALWAYS_INLINE UniquePtr& operator = (UniquePtr&& other) {
    if (FUN_LIKELY(&other != this)) {
      T* old_ptr = ptr_;
      ptr_ = other.ptr_;
      other.ptr_ = nullptr;
      GetDeleter()(old_ptr);
      GetDeleter() = MoveTemp(other.GetDeleter());
    }
    return *this;
  }

  template <typename OtherType, typename OtherDeleterType>
  FUN_ALWAYS_INLINE typename EnableIf<!IsCppArray<OtherType>::Value, UniquePtr&>::Type
  operator = (UniquePtr<OtherType, OtherDeleterType>&& other) {
    if (FUN_LIKELY(&other != this)) {
      T* old_ptr = ptr_;
      ptr_ = other.ptr_;
      other.ptr_ = nullptr;
      GetDeleter()(old_ptr);
      GetDeleter() = MoveTemp(other.GetDeleter());
    }
    return *this;
  }

  FUN_ALWAYS_INLINE UniquePtr& operator = (decltype(nullptr)) {
    T* old_ptr = ptr_;
    ptr_ = nullptr;
    GetDeleter()(old_ptr);
    return *this;
  }

  // Disable copy
  UniquePtr(const UniquePtr&) = delete;
  UniquePtr& operator = (const UniquePtr&) = delete;

  FUN_ALWAYS_INLINE ~UniquePtr() {
    GetDeleter()(ptr_);
  }

  bool IsValid() const {
    return ptr_ != nullptr;
  }

  FUN_ALWAYS_INLINE explicit operator bool () const {
    return IsValid();
  }

  FUN_ALWAYS_INLINE bool operator ! () const {
    return !IsValid();
  }

  FUN_ALWAYS_INLINE T* operator -> () const {
    return ptr_;
  }

  FUN_ALWAYS_INLINE T& operator * () const {
    return *ptr_;
  }

  FUN_ALWAYS_INLINE T* Get() const {
    return ptr_;
  }

  FUN_ALWAYS_INLINE T* Detach() {
    T* result = ptr_;
    ptr_ = nullptr;
    return result;
  }

  FUN_ALWAYS_INLINE void Reset(T* new_ptr = nullptr) {
    T* old_ptr = ptr_;
    ptr_ = new_ptr;
    GetDeleter()(old_ptr);
  }

  FUN_ALWAYS_INLINE DeleterType& GetDeleter() {
    return static_cast<DeleterType&>(*this);
  }

  FUN_ALWAYS_INLINE const DeleterType& GetDeleter() const {
    return static_cast<const DeleterType&>(*this);
  }

 private:
  T* ptr_;
};


template <typename T, typename DeleterType>
class UniquePtr<T[], DeleterType> : private DeleterType {
  template <typename OtherType, typename OtherDeleterType>
  friend class UniquePtr;

 public:
  FUN_ALWAYS_INLINE UniquePtr()
    : ptr_(nullptr) {}

  template <
      typename U,
      typename = typename EnableIf<PointerIsConvertibleFromTo<U[], T[]>::Value>::Type
    >
  explicit FUN_ALWAYS_INLINE UniquePtr(T* ptr)
    : ptr_(ptr) {}

  FUN_ALWAYS_INLINE UniquePtr(decltype(nullptr))
    : ptr_(nullptr) {}

  FUN_ALWAYS_INLINE UniquePtr(UniquePtr&& other)
    : DeleterType(MoveTemp(other.GetDeleter())),
      ptr_(other.ptr_) {
    other.ptr_ = nullptr;
  }

  template <
      typename OtherType,
      typename OtherDeleterType,
      typename = typename EnableIf<PointerIsConvertibleFromTo<OtherType[], T[]>::Value>::Type
    >
  FUN_ALWAYS_INLINE UniquePtr(UniquePtr<OtherType, OtherDeleterType>&& other)
    : DeleterType(MoveTemp(other.GetDeleter())),
      ptr_(other.ptr_) {
    other.ptr_ = nullptr;
  }

  FUN_ALWAYS_INLINE UniquePtr& operator = (UniquePtr&& other) {
    if (FUN_LIKELY(&other != this)) {
      T* old_ptr = ptr_;
      ptr_ = other.ptr_;
      other.ptr_ = nullptr;
      GetDeleter()(old_ptr);
      GetDeleter() = MoveTemp(other.GetDeleter());
    }
    return *this;
  }

  template <typename OtherType, typename OtherDeleterType>
  FUN_ALWAYS_INLINE typename EnableIf<PointerIsConvertibleFromTo<OtherType[], T[]>::Value>::Type
  operator = (UniquePtr<OtherType, OtherDeleterType>&& other) {
    if (FUN_LIKELY(&other != this)) {
      T* old_ptr = ptr_;
      ptr_ = other.ptr_;
      other.ptr_ = nullptr;
      GetDeleter()(old_ptr);
      GetDeleter() = MoveTemp(other.GetDeleter());
    }
    return *this;
  }

  FUN_ALWAYS_INLINE UniquePtr& operator = (decltype(nullptr)) {
    T* old_ptr = ptr_;
    ptr_ = nullptr;
    GetDeleter()(old_ptr);
    return *this;
  }

  // Disable copy
  UniquePtr(const UniquePtr&) = delete;
  UniquePtr& operator = (const UniquePtr&) = delete;

  FUN_ALWAYS_INLINE ~UniquePtr() {
    GetDeleter()(ptr_);
  }

  bool IsValid() const {
    return ptr_ != nullptr;
  }

  FUN_ALWAYS_INLINE explicit operator bool () const {
    return IsValid();
  }

  FUN_ALWAYS_INLINE bool operator !() const {
    return !IsValid();
  }

  FUN_ALWAYS_INLINE T* operator -> () const {
    return ptr_;
  }

  FUN_ALWAYS_INLINE T& operator * () const {
    return *ptr_;
  }

  FUN_ALWAYS_INLINE T& operator[] (size_t index) const {
    return ptr_[index];
  }

  FUN_ALWAYS_INLINE T* Get() const {
    return ptr_;
  }

  FUN_ALWAYS_INLINE T* Detach() {
    T* result = ptr_;
    ptr_ = nullptr;
    return result;
  }

  template <typename U>
  FUN_ALWAYS_INLINE typename EnableIf<PointerIsConvertibleFromTo<U[], T[]>::Value>::Type
  Reset(T* new_ptr = nullptr) {
    T* old_ptr = ptr_;
    ptr_ = new_ptr;
    GetDeleter()(old_ptr);
  }

  FUN_ALWAYS_INLINE DeleterType& GetDeleter() {
    return static_cast<DeleterType&>(*this);
  }

  FUN_ALWAYS_INLINE const DeleterType& GetDeleter() const {
    return static_cast<const DeleterType&>(*this);
  }

 private:
  T* ptr_;
};


//
// inlines
//

/**
 * Equality comparison operator
 *
 * \param lhs - The first UniquePtr to compare.
 * \param rhs - The second UniquePtr to compare.
 *
 * \return true if the two UniquePtrs are logically substitutable for each other, false otherwise.
 */
template <typename LhsType, typename RhsType>
FUN_ALWAYS_INLINE bool operator == (const UniquePtr<LhsType>& lhs, const UniquePtr<RhsType>& rhs) {
  return lhs.Get() == rhs.Get();
}

template <typename T>
FUN_ALWAYS_INLINE bool operator == (const UniquePtr<T>& lhs, const UniquePtr<T>& rhs) {
  return lhs.Get() == rhs.Get();
}

/**
 * Inequality comparison operator
 *
 * \param lhs - The first UniquePtr to compare.
 * \param rhs - The second UniquePtr to compare.
 *
 * \return false if the two UniquePtrs are logically substitutable for each other, true otherwise.
 */
template <typename LhsType, typename RhsType>
FUN_ALWAYS_INLINE bool operator != (const UniquePtr<LhsType>& lhs, const UniquePtr<RhsType>& rhs) {
  return lhs.Get() != rhs.Get();
}

template <typename T>
FUN_ALWAYS_INLINE bool operator != (const UniquePtr<T>& lhs, const UniquePtr<T>& rhs) {
  return lhs.Get() != rhs.Get();
}

/**
 * Equality comparison operator against nullptr.
 *
 * \param lhs - The UniquePtr to compare.
 *
 * \return true if the UniquePtr is null, false otherwise.
 */
template <typename T>
FUN_ALWAYS_INLINE bool operator == (const UniquePtr<T>& lhs, decltype(nullptr)) {
  return !lhs.IsValid();
}

template <typename T>
FUN_ALWAYS_INLINE bool operator == (decltype(nullptr), const UniquePtr<T>& rhs) {
  return !rhs.IsValid();
}

/**
 * Inequality comparison operator against nullptr.
 *
 * \param rhs - The UniquePtr to compare.
 *
 * \return true if the UniquePtr is not null, false otherwise.
 */
template <typename T>
FUN_ALWAYS_INLINE bool operator != (const UniquePtr<T>& lhs, decltype(nullptr)) {
  return lhs.IsValid();
}

template <typename T>
FUN_ALWAYS_INLINE bool operator != (decltype(nullptr), const UniquePtr<T>& rhs) {
  return rhs.IsValid();
}

// Trait which allows UniquePtr to be default constructed by memsetting to zero.
template <typename T>
struct IsZeroConstructible<UniquePtr<T>> {
  enum { Value = true };
};

// Trait which allows UniquePtr to be memcpy'able from pointers.
template <typename T>
struct IsBitwiseConstructible<UniquePtr<T>, T*> {
  enum { Value = true };
};

/**
 * Constructs a new object with the given arguments and returns it as a UniquePtr.
 *
 * \param args - The arguments to pass to the constructor of T.
 *
 * \return A UniquePtr which points to a newly-constructed T with the specified args.
 */
template <typename T, typename... Args>
FUN_ALWAYS_INLINE typename EnableIf<!IsCppArray<T>::Value, UniquePtr<T>>::Type
  MakeUnique(Args&&... args) {
  return UniquePtr<T>(new T(Forward<Args>(args)...));
}

template <typename T>
FUN_ALWAYS_INLINE typename EnableIf<IsBoundedCppArray<T>::Value, UniquePtr<T>>::Type
  MakeUnique(size_t size) {
  using ElementType = typename RemoveExtent<T>::Type;
  return UniquePtr<T>(new ElementType[size]());
}

template <typename T, typename... Args>
typename EnableIf<IsBoundedCppArray<T>::Value, UniquePtr<T>>::Type
  MakeUnique(Args&&... args) = delete;

} // namespace fun
