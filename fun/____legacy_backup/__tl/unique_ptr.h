#pragma once

#include "fun/base/base.h"
#include "fun/tl/template.h"
#include "fun/tl/is_array.h" // type_traits.h로 통합하는게 좋을듯도...

namespace fun {
namespace tl {

template <typename T>
struct DefaultDelete
{
  DefaultDelete() = default;
  DefaultDelete(const DefaultDelete&) = default;
  DefaultDelete& operator = (const DefaultDelete&) = default;
  ~DefaultDelete() = default;

  template <typename U, typename = typename EnableIf<PointerIsConvertibleFromTo<U,T>::Value>::Type>
  DefaultDelete(const DefaultDelete<U>&)
  {
  }

  template <typename U, typename = typename EnableIf<PointerIsConvertibleFromTo<U,T>::Value>::Type>
  DefaultDelete& operator = (const DefaultDelete<U>&)
  {
    return *this;
  }

  void operator()(T* ptr) const
  {
    delete ptr;
  }
};


template <typename T>
struct DefaultDelete<T[]>
{
  DefaultDelete() = default;
  DefaultDelete(const DefaultDelete&) = default;
  DefaultDelete& operator = (const DefaultDelete&) = default;
  ~DefaultDelete() = default;

  template <typename U, typename = typename EnableIf<PointerIsConvertibleFromTo<U[],T[]>::Value>::Type>
  DefaultDelete(const DefaultDelete<U>&)
  {
  }

  template <typename U, typename = typename EnableIf<PointerIsConvertibleFromTo<U[],T[]>::Value>::Type>
  DefaultDelete& operator = (const DefaultDelete<U>&)
  {
    return *this;
  }

  template <typename U>
  typename EnableIf<PointerIsConvertibleFromTo<U[],T[]>::Value>::Type
    operator()(U* ptr) const
  {
    delete[] ptr;
  }
};


/**
 * Single-ownership smart pointer in the vein of std::unique_ptr.
 * Use this when you need an object's lifetime to be strictly bound to the lifetime of a single smart pointer.
 * 
 * This class is non-copyable - ownership can only be transferred via a move operation, e.g.:
 * 
 * UniquePtr<MyClass> ptr1(new MyClass);    // The MyClass object is owned by ptr1.
 * UniquePtr<MyClass> ptr2(ptr1);           // Error - UniquePtr is not copyable
 * UniquePtr<MyClass> ptr3(MoveTemp(ptr1)); // ptr3 now owns the MyClass object - ptr1 is now nullptr.
 */
template <typename T, typename Deleter = DefaultDelete<T>>
class UniquePtr : private Deleter
{
  template <typename OtherT, typename OtherDeleter>
  friend class UniquePtr;

 public:
  inline UniquePtr()
    : reference_(nullptr)
  {
  }

  explicit inline UniquePtr(T* reference)
    : reference_(reference)
  {
  }

  inline UniquePtr(decltype(nullptr))
    : reference_(nullptr)
  {

  // Move만 지원함. UniquePtr은 copy가 아닌 move를 통해서
  // 하나의 소유권을 유지해야함.
  inline UniquePtr(UniquePtr&& rhs)
    : Deleter(MoveTemp(rhs.GetDeleter()))
    , reference_(rhs.reference_)
  {
    rhs.reference_ = nullptr;
  }

  template <typename OtherT,typename OtherDeleter,typename = typename EnableIf<!IsArray<OtherT>::Value>::Type>
  inline UniquePtr(UniquePtr<OtherT,OtherDeleter>&& other)
    : Deleter(MoveTemp(other.GetDeleter()))
    , reference_(other.reference_)
  {
    other.reference_ = nullptr;
  }

  inline UniquePtr& operator = (UniquePtr&& other)
  {
    if (FUN_LIKELY(&other != this)) {
      T* old_reference = reference_;
      reference_ = other.reference_;
      other.reference_ = nullptr;
      GetDeleter()(old_reference);
    }
    GetDeleter() = MoveTemp(other.GetDeleter());
    return *this;
  }

  template <typename OtherT,typename OtherDeleter>
  inline typename EnableIf<!IsArray<OtherT>::Value,UniquePtr&>::Type
      operator = (UniquePtr<OtherT,OtherDeleter>&& other)
  {
    T* old_reference = reference_;
    reference_ = other.reference_;
    other.reference_ = nullptr;
    GetDeleter()(old_reference);
    GetDeleter() = MoveTemp(other.GetDeleter());
    return *this;
  }

  inline UniquePtr& operator = (decltype(nullptr))
  {
    T* old_reference = reference_;
    reference_ = nullptr;
    GetDeleter()(old_reference);
    return *this;
  }

  inline ~UniquePtr()
  {
    GetDeleter()(reference_);
  }

  bool IsValid() const
  {
    return reference_ != nullptr;
  }

  inline explicit operator bool () const
  {
    return IsValid();
  }

  inline bool operator ! () const
  {
    return !IsValid();
  }

  inline T* operator -> () const
  {
    return reference_;
  }

  inline T& operator * () const
  {
    return *reference_;
  }

  inline T* Get() const
  {
    return reference_;
  }

  inline T* Detach()
  {
    T* result = reference_;
    reference_ = nullptr;
    return result;
  }

  inline void Reset(T* new_reference = nullptr)
  {
    T* old_reference = reference_;
    reference_ = new_reference;
    GetDeleter()(old_reference);
  }

  inline Deleter& GetDeleter()
  {
    return static_cast<Deleter&>(*this);
  }

  inline const Deleter& GetDeleter() const
  {
    return static_cast<const Deleter&>(*this);
  }

 private:
  UniquePtr(const UniquePtr&);
  UniquePtr& operator = (const UniquePtr&);

  T* reference_;
};


template <typename T, typename Deleter>
class UniquePtr<T[],Deleter> : private Deleter
{
  template <typename OtherT, typename OtherDeleter>
  friend class UniquePtr;

 public:
  inline UniquePtr()
    : reference_(nullptr)
  {
  }

  template <typename U,typename = typename EnableIf<PointerIsConvertibleFromTo<U[],T[]>::Value>::Type>
  explicit inline UniquePtr(T* InPtr)
    : reference_(InPtr)
  {
  }

  inline UniquePtr(decltype(nullptr))
    : reference_(nullptr)
  {
  }

  inline UniquePtr(UniquePtr&& other)
    : Deleter(MoveTemp(other.GetDeleter()))
    , reference_(other.reference_)
  {
    other.reference_ = nullptr;
  }

  template <typename OtherT,typename OtherDeleter,typename = typename EnableIf<PointerIsConvertibleFromTo<OtherT[],T[]>::Value>::Type>
  inline UniquePtr(UniquePtr<OtherT,OtherDeleter>&& other)
    : Deleter(MoveTemp(other.GetDeleter()))
    , reference_(other.reference_)
  {
    other.reference_ = nullptr;
  }

  inline UniquePtr& operator = (UniquePtr&& other)
  {
    if (FUN_LIKELY(&other != this)) {
      T* old_reference = reference_;
      reference_ = other.reference_;
      other.reference_ = nullptr;
      GetDeleter()(old_reference);
    }
    GetDeleter() = MoveTemp(other.GetDeleter());
    return *this;
  }

  template <typename OtherT,typename OtherDeleter>
  inline typename EnableIf<PointerIsConvertibleFromTo<OtherT[],T[]>::Value>::Type operator = (UniquePtr<OtherT,OtherDeleter>&& other)
  {
    T* old_reference = reference_;
    reference_ = other.reference_;
    other.reference_ = nullptr;
    GetDeleter()(old_reference);
    GetDeleter() = MoveTemp(other.GetDeleter());
    return *this;
  }

  inline UniquePtr& operator = (decltype(nullptr))
  {
    T* old_reference = reference_;
    reference_ = nullptr;
    GetDeleter()(old_reference);
    return *this;
  }

  inline ~UniquePtr()
  {
    GetDeleter()(reference_);
  }

  bool IsValid() const
  {
    return reference_ != nullptr;
  }

  inline explicit operator bool () const
  {
    return IsValid();
  }

  inline bool operator !() const
  {
    return !IsValid();
  }

  inline T* operator -> () const
  {
    return reference_;
  }

  inline T& operator * () const
  {
    return *reference_;
  }

  inline T& operator[] (size_t index) const
  {
    return reference_[index];
  }

  inline T* Get() const
  {
    return reference_;
  }

  inline T* Detach()
  {
    T* result = reference_;
    reference_ = nullptr;
    return result;
  }

  template <typename U>
  inline typename EnableIf<PointerIsConvertibleFromTo<U[],T[]>::Value>::Type Reset(T* new_reference = nullptr)
  {
    T* old_reference = reference_;
    reference_ = new_reference;
    GetDeleter()(old_reference);
  }

  inline Deleter& GetDeleter()
  {
    return static_cast<Deleter&>(*this);
  }

  inline const Deleter& GetDeleter() const
  {
    return static_cast<const Deleter&>(*this);
  }

 private:
  // Non-copyable
  UniquePtr(const UniquePtr&);
  UniquePtr& operator = (const UniquePtr&);

  T* reference_;
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
template <typename LhsT, typename RhsT>
inline bool operator == (const UniquePtr<LhsT>& lhs, const UniquePtr<RhsT>& rhs)
{
  return lhs.Get() == rhs.Get();
}

template <typename T>
inline bool operator == (const UniquePtr<T>& lhs, const UniquePtr<T>& rhs)
{
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
template <typename LhsT, typename RhsT>
inline bool operator != (const UniquePtr<LhsT>& lhs, const UniquePtr<RhsT>& rhs)
{
  return lhs.Get() != rhs.Get();
}

template <typename T>
inline bool operator != (const UniquePtr<T>& lhs, const UniquePtr<T>& rhs)
{
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
inline bool operator == (const UniquePtr<T>& lhs, decltype(nullptr))
{
  return !lhs.IsValid();
}

template <typename T>
inline bool operator == (decltype(nullptr), const UniquePtr<T>& rhs)
{
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
inline bool operator != (const UniquePtr<T>& lhs, decltype(nullptr))
{
  return lhs.IsValid();
}

template <typename T>
inline bool operator != (decltype(nullptr), const UniquePtr<T>& rhs)
{
  return rhs.IsValid();
}

// Trait which allows UniquePtr to be default constructed by memsetting to zero.
template <typename T>
struct IsZeroConstructible<UniquePtr<T>>
{
  enum { value = true };
};

// Trait which allows UniquePtr to be memcpy'able from pointers.
template <typename T>
struct IsBitwiseConstructible<UniquePtr<T>, T*>
{
  enum { value = true };
};

/**
 * Constructs a new object with the given arguments and returns it as a UniquePtr.
 * 
 * \param args - The arguments to pass to the constructor of T.
 * 
 * \return A UniquePtr which points to a newly-constructed T with the specified args.
 */
template <typename T, typename... Args>
inline typename EnableIf<!IsArray<T>::Value,UniquePtr<T>>::Type MakeUnique(Args&&... args)
{
  return UniquePtr<T>(new T(Forward<Args>(args)...));
}

template <typename T>
inline typename EnableIf<IsBoundedArray<T>::Value,UniquePtr<T>>::Type MakeUnique(size_t size)
{
  typedef typename RemoveExtent<T>::Type ElementType;
  return UniquePtr<T>(new ElementType[size]());
}

template <typename T, typename... Args>
typename EnableIf<IsBoundedArray<T>::Value,UniquePtr<T>>::Type MakeUnique(Args&&... args) = delete;

} // namespace tl
} // namespace fun
