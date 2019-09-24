#pragma once

#include "fun/base/base.h"
#include "fun/base/ref_counted_object.h"

namespace fun {

/**
 * A smart pointer to an object which implements AddRef / Release.
 */
template <typename T>
class RefCountedPtr {
 public:
  template <typename U>
  friend class RefCountedPtr;

  RefCountedPtr() : ptr_(nullptr) {}

  // TODO 초기 참조 카운터가 1일 경우와 0일 경우에 따라서 처리가 달라짐.
  //이부분을 확실히 해주어야함.
  RefCountedPtr(T* ptr) : ptr_(ptr) {
    if (ptr_) {
      ptr_->AddRef();
    }
  }

  /**
   * `initial_add_ref` 가 `false` 일 경우 참조는 획득하지 않고,
   * Release하는 역활만 수행함.
   * 즉, auto-releasing 기능을 수행함.
   */
  RefCountedPtr(T* ptr, bool initial_add_ref) : ptr_(ptr) {
    if (ptr_ && initial_add_ref) {
      ptr_->AddRef();
    }
  }

  RefCountedPtr(const RefCountedPtr& rhs) : ptr_(rhs.ptr_) {
    if (ptr_) {
      ptr_->AddRef();
    }
  }

  // move semantics.
  RefCountedPtr(RefCountedPtr&& rhs) : ptr_(rhs.ptr_) { rhs.ptr_ = nullptr; }

  template <typename OtherType>
  RefCountedPtr(const RefCountedPtr<OtherType>& other)
      : ptr_(const_cast<OtherType*>(other.Get())) {
    if (ptr_) {
      ptr_->AddRef();
    }
  }

  template <typename OtherType>
  RefCountedPtr(RefCountedPtr<OtherType>&& other) : ptr_(other.Get()) {
    other.ptr_ = nullptr;
  }

  ~RefCountedPtr() { Release(); }

  RefCountedPtr& operator=(T* ptr) {
    if (FUN_LIKELY(ptr != ptr_)) {
      T* old_ptr = ptr_;
      ptr_ = ptr;

      if (ptr_) {
        ptr_->AddRef();
      }

      if (old_ptr) {
        old_ptr->Release();
      }
    }
    return *this;
  }

  RefCountedPtr& operator=(const RefCountedPtr& rhs) {
    return (*this = rhs.ptr_);
  }

  RefCountedPtr& operator=(RefCountedPtr&& rhs) {
    if (FUN_LIKELY(&rhs != this)) {
      T* old_ptr = ptr_;
      ptr_ = rhs.ptr_;
      rhs.ptr_ = nullptr;
      if (old_ptr) {
        old_ptr->Release();
      }
    }
    return *this;
  }

  template <typename OtherType>
  RefCountedPtr& operator=(const RefCountedPtr<OtherType>& rhs) {
    return (*this = rhs.ptr_);
  }

  template <typename OtherType>
  RefCountedPtr& operator=(RefCountedPtr<OtherType>&& rhs) {
    if (FUN_LIKELY(&rhs != this)) {
      T* old_ptr = ptr_;
      ptr_ = rhs.ptr_;
      rhs.ptr_ = nullptr;
      if (old_ptr) {
        old_ptr->Release();
      }
    }
    return *this;
  }

  //
  // Casting
  //

  template <typename CastToType>
  RefCountedPtr<T> Cast() const {
    return RefCountedPtr<CastToType>(dynamic_cast<CastToType*>(ptr_), true);
  }

  template <typename CastToType>
  RefCountedPtr<T> UnsafeCast() const {
    return RefCountedPtr<CastToType>(static_cast<CastToType*>(ptr_), true);
  }

  //
  // Dereferencings
  //

  T* operator->() {
    fun_check_ptr(ptr_);
    return ptr_;
  }

  const T* operator->() const {
    fun_check_ptr(ptr_);
    return ptr_;
  }

  T& operator*() {
    fun_check_ptr(ptr_);
    return *ptr_;
  }

  const T& operator*() const {
    fun_check_ptr(ptr_);
    return *ptr_;
  }

  operator T*() const { return ptr_; }

  T** GetInitReference() {
    *this = nullptr;
    return &ptr_;
  }

  T* GetReference() const { return ptr_; }

  T* Get() const { return ptr_; }

  //
  // Comparisons
  //

  friend bool operator==(const RefCountedPtr& lhs, const RefCountedPtr& rhs) {
    return lhs.ptr_ == rhs.ptr_;
  }

  friend bool operator==(const RefCountedPtr& lhs, T* rhs) {
    return lhs.ptr_ == rhs;
  }

  friend bool operator==(T* lhs, const RefCountedPtr& rhs) {
    return lhs == rhs.ptr_;
  }

  // TODO 대소 비교도 해야하나??

  bool IsValid() const { return ptr_ != nullptr; }

  explicit operator bool() const { return IsValid(); }

  bool operator!() const { return ptr_ == nullptr; }

  void SafeRelease() { *this = nullptr; }

  T* AddRef() {
    if (ptr_) {
      ptr_->AddRef();
    }
    return ptr_;
  }

  void Release() {
    if (ptr_) {
      ptr_->Release();
      ptr_ = nullptr;
    }
  }

  uint32 GetReferencedCount() const {
    uint32 count = 0;
    if (ptr_) {
      count = ptr_->GetReferencedCount();
      // you should never have a zero ref count
      // if there is a live ref counted pointer (*this is live)
      fun_check((int32)count > 0);
    }
    return count;
  }

  bool IsUnique() const { return GetReferencedCount() == 1; }

  void Swap(RefCountedPtr& other) { fun::Swap(ptr_, other.ptr_); }

 private:
  T* ptr_;
};

}  // namespace fun
