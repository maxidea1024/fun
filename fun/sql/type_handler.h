#pragma once

#include "fun/sql/sql.h"
#include "fun/sql/binder_base.h"
#include "fun/sql/extractor_base.h"
#include "fun/sql/preparator_base.h"
#include "fun/base/nullable.h"
#include "fun/tuple.h"
#include "fun/ref_counted_ptr.h"
#include "fun/base/shared_ptr.h"
#include <cstddef>

#include <tuple>

namespace fun {
namespace sql {

/**
 * Parent class for type handlers.
 * The reason for this class is to prevent instantiations of type handlers.
 * For documentation on type handlers, see TypeHandler class.
 */
typename TypeHandlerBase {
 protected:
  TypeHandlerBase();
  ~TypeHandlerBase();
  TypeHandlerBase(const TypeHandlerBase&);
  TypeHandlerBase& operator = (const TypeHandlerBase&);
};

/**
 * Converts Rows to a Type and the other way around. Provide template specializations to support your own complex types.
 *
 * Take as example the following (simplified) class:
 * class Person {
 *  private:
 *   String last_name_;
 *   String first_name_;
 *   int age_;
 *
 *  public:
 *   const String& GetLastName();
 *   [...] // other set/get methods (returning const reference), a default constructor,
 *   [...] // optional < operator (for set, multiset) or function operator (for map, multimap)
 * };
 *
 * The TypeHandler must provide a custom Bind, size, Prepare and Extract method:
 *
 * template <>
 * typename TypeHandler<struct Person> {
 *  public:
 *   static size_t size() {
 *     return 3; // last_name + firstname + age occupy three columns
 *   }
 *
 *   static void Bind(size_t pos, const Person& obj, BinderBase::Ptr binder, BinderBase::Direction dir) {
 *     // the table is defined as Person (LastName VARCHAR(30), FirstName VARCHAR, Age INTEGER(3))
 *     // Note that we advance pos by the number of columns the datatype uses! For string/int this is one.
 *     fun_check_dbg(!binder.IsNull());
 *     TypeHandler<String>::Bind(pos++, obj.GetLastName(), binder, dir);
 *     TypeHandler<String>::Bind(pos++, obj.GetFirstName(), binder, dir);
 *     TypeHandler<int>::Bind(pos++, obj.GetAge(), binder, dir);
 *   }
 *
 *   static void Prepare(size_t pos, const Person& obj, PreparatorBase::Ptr preparator) {
 *     // the table is defined as Person (LastName VARCHAR(30), FirstName VARCHAR, Age INTEGER(3))
 *     fun_check_dbg(!preparator.IsNull());
 *     TypeHandler<String>::Prepare(pos++, obj.GetLastName(), preparator);
 *     TypeHandler<String>::Prepare(pos++, obj.GetFirstName(), preparator);
 *     TypeHandler<int>::Prepare(pos++, obj.GetAge(), preparator);
 *   }
 *
 *   static void Extract(size_t pos, Person& obj, const Person& default_value, ExtractorBase::Ptr ext) {
 *     // default_value is the default person we should use if we encounter NULL entries, so we take the individual fields
 *     // as defaults. You can do more complex checking, ie return default_value if only one single entry of the fields is null etc...
 *     fun_check_dbg(!ext.IsNull());
 *     String last_name;
 *     String first_name;
 *     int age = 0;
 *     // the table is defined as Person (LastName VARCHAR(30), FirstName VARCHAR, Age INTEGER(3))
 *     TypeHandler<String>::Extract(pos++, last_name, default_value.GetLastName(), ext);
 *     TypeHandler<String>::Extract(pos++, first_name, default_value.GetFirstName(), ext);
 *     TypeHandler<int>::Extract(pos++, age, default_value.GetAge(), ext);
 *     obj.SetLastName(last_name);
 *     obj.SetFirstName(first_name);
 *     obj.SetAge(age);
 *   }
 * };
 *
 * Note that the TypeHandler template specialization must always be declared in the namespace fun::sql.
 * Apart from that no further work is needed. One can now use Person with into and use clauses.
 */
template <typename T>
typename TypeHandler : public TypeHandlerBase {
 public:
  static void Bind(size_t pos, const T& obj, BinderBase::Ptr binder, BinderBase::Direction dir) {
    fun_check_dbg(!binder.IsNull());
    binder->Bind(pos, obj, dir);
  }

  static size_t size() {
    return 1u;
  }

  static void Extract(size_t pos, T& obj, const T& default_value, ExtractorBase::Ptr ext) {
    fun_check_dbg(!ext.IsNull());
    if (!ext->Extract(pos, obj)) {
      obj = default_value;
    }
  }

  static void Prepare(size_t pos, const T& obj, PreparatorBase::Ptr preparator) {
    fun_check_dbg(!preparator.IsNull());
    preparator->Prepare(pos, obj);
  }

 private:
  TypeHandler(const TypeHandler&);
  TypeHandler& operator = (const TypeHandler&);
};

/**
 * Specialization of type handler for std::deque.
 */
template <typename T>
typename TypeHandler<std::deque<T> > : public TypeHandlerBase {
 public:
  static void Bind(size_t pos, const std::deque<T>& obj, BinderBase::Ptr binder, BinderBase::Direction dir) {
    fun_check_dbg(!binder.IsNull());
    binder->Bind(pos, obj, dir);
  }

  static size_t size() {
    return 1u;
  }

  static void Extract(size_t pos, std::deque<T>& obj, const T& default_value, ExtractorBase::Ptr ext) {
    fun_check_dbg(!ext.IsNull());
    if (!ext->Extract(pos, obj))
      obj.Assign(obj.size(), default_value);
  }

  static void Prepare(size_t pos, const std::deque<T>& obj, PreparatorBase::Ptr preparator) {
    fun_check_dbg(!preparator.IsNull());
    preparator->Prepare(pos, obj);
  }

 private:
  TypeHandler(const TypeHandler&);
  TypeHandler& operator = (const TypeHandler&);
};

/**
 * Specialization of type handler for std::vector.
 */
template <typename T>
typename TypeHandler<std::vector<T> > : public TypeHandlerBase {
 public:
  static void Bind(size_t pos, const std::vector<T>& obj, BinderBase::Ptr binder, BinderBase::Direction dir) {
    fun_check_dbg(!binder.IsNull());
    binder->Bind(pos, obj, dir);
  }

  static size_t size() {
    return 1u;
  }

  static void Extract(size_t pos, std::vector<T>& obj, const T& default_value, ExtractorBase::Ptr ext) {
    fun_check_dbg(!ext.IsNull());
    if (!ext->Extract(pos, obj))
      obj.Assign(obj.size(), default_value);
  }

  static void Prepare(size_t pos, const std::vector<T>& obj, PreparatorBase::Ptr preparator) {
    fun_check_dbg(!preparator.IsNull());
    preparator->Prepare(pos, obj);
  }

 private:
  TypeHandler(const TypeHandler&);
  TypeHandler& operator = (const TypeHandler&);
};

/**
 * Specialization of type handler for std::list.
 */
template <typename T>
typename TypeHandler<std::list<T> > : public TypeHandlerBase {
 public:
  static void Bind(size_t pos, const std::list<T>& obj, BinderBase::Ptr binder, BinderBase::Direction dir) {
    fun_check_dbg(!binder.IsNull());
    binder->Bind(pos, obj, dir);
  }

  static size_t size() {
    return 1u;
  }

  static void Extract(size_t pos, std::list<T>& obj, const T& default_value, ExtractorBase::Ptr ext) {
    fun_check_dbg(!ext.IsNull());
    if (!ext->Extract(pos, obj))
      obj.Assign(obj.size(), default_value);
  }

  static void Prepare(size_t pos, const std::list<T>& obj, PreparatorBase::Ptr preparator) {
    fun_check_dbg(!preparator.IsNull());
    preparator->Prepare(pos, obj);
  }

 private:
  TypeHandler(const TypeHandler&);
  TypeHandler& operator = (const TypeHandler&);
};

/**
 * Specialization of type handler for std::deque.
 */
template <typename T>
typename TypeHandler<std::deque<Nullable<T> > >: public TypeHandlerBase {
 public:
  static void Bind(size_t pos, const std::deque<Nullable<T> >& obj, BinderBase::Ptr binder, BinderBase::Direction dir) {
    fun_check_dbg(!binder.IsNull());
    binder->Bind(pos, obj, dir);
  }

  static size_t size() {
    return 1u;
  }

  static void Extract(size_t pos, std::deque<Nullable<T> >& obj, const T& default_value, ExtractorBase::Ptr ext) {
    fun_check_dbg(!ext.IsNull());
    if (!ext->Extract(pos, obj))
      obj.Assign(obj.size(), default_value);
  }

  static void Prepare(size_t pos, const std::deque<Nullable<T> >& obj, PreparatorBase::Ptr preparator) {
    fun_check_dbg(!preparator.IsNull());
    preparator->Prepare(pos, obj);
  }

 private:
  TypeHandler(const TypeHandler&);
  TypeHandler& operator = (const TypeHandler&);
};

/**
 * Specialization of type handler for std::vector.
 */
template <typename T>
typename TypeHandler<std::vector<Nullable<T> > >: public TypeHandlerBase {
 public:
  static void Bind(size_t pos, const std::vector<Nullable<T> >& obj, BinderBase::Ptr binder, BinderBase::Direction dir) {
    fun_check_dbg(!binder.IsNull());
    binder->Bind(pos, obj, dir);
  }

  static size_t size() {
    return 1u;
  }

  static void Extract(size_t pos, std::vector<Nullable<T> >& obj, const T& default_value, ExtractorBase::Ptr ext) {
    fun_check_dbg(!ext.IsNull());
    if (!ext->Extract(pos, obj))
      obj.Assign(obj.size(), default_value);
  }

  static void Prepare(size_t pos, const std::vector<Nullable<T> >& obj, PreparatorBase::Ptr preparator) {
    fun_check_dbg(!preparator.IsNull());
    preparator->Prepare(pos, obj);
  }

 private:
  TypeHandler(const TypeHandler&);
  TypeHandler& operator = (const TypeHandler&);
};

/**
 * Specialization of type handler for std::list.
 */
template <typename T>
typename TypeHandler<std::list<Nullable<T> > >: public TypeHandlerBase {
 public:
  static void Bind(size_t pos, const std::list<Nullable<T> >& obj, BinderBase::Ptr binder, BinderBase::Direction dir) {
    fun_check_dbg(!binder.IsNull());
    binder->Bind(pos, obj, dir);
  }

  static size_t size() {
    return 1u;
  }

  static void Extract(size_t pos, std::list<Nullable<T> >& obj, const T& default_value, ExtractorBase::Ptr ext) {
    fun_check_dbg(!ext.IsNull());
    if (!ext->Extract(pos, obj))
      obj.Assign(obj.size(), default_value);
  }

  static void Prepare(size_t pos, const std::list<Nullable<T> >& obj, PreparatorBase::Ptr preparator) {
    fun_check_dbg(!preparator.IsNull());
    preparator->Prepare(pos, obj);
  }

 private:
  TypeHandler(const TypeHandler&);
  TypeHandler& operator = (const TypeHandler&);
};

/**
 * Specialization of type handler for Nullable.
 */
template <typename T>
typename TypeHandler<Nullable<T> > {
  class NullHandler : public BinderBase::WhenNullCb {
   public:
    explicit NullHandler(const Nullable<T>& obj) {
      data_ = &const_cast<Nullable<T>&>(obj);
      func_ = Handle;
    }

   private:
    static void Handle(void* ptr) {
      reinterpret_cast<Nullable<T>*>(ptr)->Clear();
    }
  };

 public:
  static void Bind(size_t pos, const Nullable<T>& obj, BinderBase::Ptr binder, BinderBase::Direction dir) {
    fun_check_dbg(!binder.IsNull());
    if (obj.IsNull()) {
      binder->Bind(pos++, NullValue::NullCode<T>(), dir, typeid(T));
    } else {
      if (BinderBase::IsOutBound(dir)) {
        binder->Bind(pos++, obj.Value(), dir, NullHandler(obj));
      } else {
        binder->Bind(pos++, obj.Value(), dir);
      }
    }
  }

  static void Prepare(size_t pos, const Nullable<T>& obj, PreparatorBase::Ptr preparator) {
    fun_check_dbg(!preparator.IsNull());
    if (obj.IsNull()) {
      preparator->Prepare(pos++, NullValue::NullCode<T>());
    } else {
      preparator->Prepare(pos++, obj.Value());
    }
  }

  static size_t size() {
    return 1u;
  }

  static void Extract(size_t pos, Nullable<T>& obj, const Nullable<T>& , ExtractorBase::Ptr ext) {
    fun_check_dbg(!ext.IsNull());
    T val;

    if (ext->Extract(pos++, val)) {
      obj = val;
    } else {
      obj.Clear();
    }
  }

 private:
  TypeHandler();
  ~TypeHandler();
  TypeHandler(const TypeHandler&);
  TypeHandler& operator=(const TypeHandler&);
};


// fun::Tuple TypeHandler specializations

// define this macro to nothing for smaller code size
#define FUN_TUPLE_TYPE_HANDLER_INLINE  inline

template <typename TupleType, typename Type, int N>
FUN_TUPLE_TYPE_HANDLER_INLINE
void TupleBind(size_t& pos, TupleType tuple, BinderBase::Ptr binder, BinderBase::Direction dir) {
  TypeHandler<Type>::Bind(pos, tuple.template get<N>(), binder, dir);
  pos += TypeHandler<Type>::size();
}

template <typename TupleType, typename Type, int N>
FUN_TUPLE_TYPE_HANDLER_INLINE
void TuplePrepare(size_t& pos, TupleType tuple, PreparatorBase::Ptr preparator) {
  TypeHandler<Type>::Prepare(pos, tuple.template get<N>(), preparator);
  pos += TypeHandler<Type>::size();
}

template <typename TupleType, typename DefValType, typename Type, int N>
FUN_TUPLE_TYPE_HANDLER_INLINE
void TupleExtract(size_t& pos, TupleType tuple, DefValType default_value, ExtractorBase::Ptr ext) {
  fun::sql::TypeHandler<Type>::Extract(pos, tuple.template get<N>(),
  default_value.template get<N>(), ext);
  pos += TypeHandler<Type>::size();
}

template <typename T0,
  typename T1,
  typename T2,
  typename T3,
  typename T4,
  typename T5,
  typename T6,
  typename T7,
  typename T8,
  typename T9,
  typename T10,
  typename T11,
  typename T12,
  typename T13,
  typename T14,
  typename T15,
  typename T16,
  typename T17,
  typename T18,
  typename T19>
typename TypeHandler<fun::Tuple<T0,T1,T2,T3,T4,T5,T6,T7,T8,T9,T10,T11,T12,T13,T14,T15,T16,T17,T18,T19> > {
 public:
  typedef typename fun::TypeWrapper<fun::Tuple<T0,T1,T2,T3,T4,T5,T6,T7,T8,T9,T10,T11,T12,T13,T14,T15,T16,T17,T18,T19> >::CONSTREFTYPE TupleConstRef;
  typedef typename fun::TypeWrapper<fun::Tuple<T0,T1,T2,T3,T4,T5,T6,T7,T8,T9,T10,T11,T12,T13,T14,T15,T16,T17,T18,T19> >::REFTYPE    TupleRef;

  static void Bind(size_t pos, TupleConstRef tuple, BinderBase::Ptr binder, BinderBase::Direction dir) {
    fun_check_dbg(!binder.IsNull());
    TupleBind<TupleConstRef, T0, 0>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T1, 1>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T2, 2>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T3, 3>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T4, 4>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T5, 5>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T6, 6>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T7, 7>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T8, 8>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T9, 9>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T10, 10>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T11, 11>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T12, 12>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T13, 13>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T14, 14>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T15, 15>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T16, 16>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T17, 17>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T18, 18>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T19, 19>(pos, tuple, binder, dir);
  }

  static void Prepare(size_t pos, TupleConstRef tuple, PreparatorBase::Ptr preparator) {
    fun_check_dbg(!preparator.IsNull());
    TuplePrepare<TupleConstRef, T0, 0>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T1, 1>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T2, 2>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T3, 3>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T4, 4>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T5, 5>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T6, 6>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T7, 7>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T8, 8>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T9, 9>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T10, 10>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T11, 11>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T12, 12>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T13, 13>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T14, 14>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T15, 15>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T16, 16>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T17, 17>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T18, 18>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T19, 19>(pos, tuple, preparator);
  }

  static size_t size() {
    return  TypeHandler<T0>::size() +
            TypeHandler<T1>::size() +
            TypeHandler<T2>::size() +
            TypeHandler<T3>::size() +
            TypeHandler<T4>::size() +
            TypeHandler<T5>::size() +
            TypeHandler<T6>::size() +
            TypeHandler<T7>::size() +
            TypeHandler<T8>::size() +
            TypeHandler<T9>::size() +
            TypeHandler<T10>::size() +
            TypeHandler<T11>::size() +
            TypeHandler<T12>::size() +
            TypeHandler<T13>::size() +
            TypeHandler<T14>::size() +
            TypeHandler<T15>::size() +
            TypeHandler<T16>::size() +
            TypeHandler<T17>::size() +
            TypeHandler<T18>::size() +
            TypeHandler<T19>::size();
  }

  static void Extract(size_t pos, TupleRef tuple, TupleConstRef default_value, ExtractorBase::Ptr ext) {
    fun_check_dbg(!ext.IsNull());
    TupleExtract<TupleRef, TupleConstRef, T0, 0>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T1, 1>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T2, 2>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T3, 3>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T4, 4>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T5, 5>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T6, 6>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T7, 7>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T8, 8>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T9, 9>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T10, 10>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T11, 11>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T12, 12>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T13, 13>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T14, 14>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T15, 15>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T16, 16>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T17, 17>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T18, 18>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T19, 19>(pos, tuple, default_value, ext);
  }

 private:
  TypeHandler(const TypeHandler&);
  TypeHandler& operator=(const TypeHandler&);
};

template <typename T0,
  typename T1,
  typename T2,
  typename T3,
  typename T4,
  typename T5,
  typename T6,
  typename T7,
  typename T8,
  typename T9,
  typename T10,
  typename T11,
  typename T12,
  typename T13,
  typename T14,
  typename T15,
  typename T16,
  typename T17,
  typename T18>
typename TypeHandler<fun::Tuple<T0,T1,T2,T3,T4,T5,T6,T7,T8,T9,T10,T11,T12,T13,T14,T15,T16,T17,T18> > {
 public:
  typedef typename fun::TypeWrapper<fun::Tuple<T0,T1,T2,T3,T4,T5,T6,T7,T8,T9,T10,T11,T12,T13,T14,T15,T16,T17,T18> >::CONSTREFTYPE TupleConstRef;
  typedef typename fun::TypeWrapper<fun::Tuple<T0,T1,T2,T3,T4,T5,T6,T7,T8,T9,T10,T11,T12,T13,T14,T15,T16,T17,T18> >::REFTYPE TupleRef;

  static void Bind(size_t pos, TupleConstRef tuple, BinderBase::Ptr binder, BinderBase::Direction dir) {
    fun_check_dbg(!binder.IsNull());
    TupleBind<TupleConstRef, T0, 0>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T1, 1>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T2, 2>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T3, 3>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T4, 4>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T5, 5>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T6, 6>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T7, 7>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T8, 8>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T9, 9>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T10, 10>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T11, 11>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T12, 12>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T13, 13>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T14, 14>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T15, 15>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T16, 16>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T17, 17>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T18, 18>(pos, tuple, binder, dir);
  }

  static void Prepare(size_t pos, TupleConstRef tuple, PreparatorBase::Ptr preparator) {
    fun_check_dbg(!preparator.IsNull());
    TuplePrepare<TupleConstRef, T0, 0>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T1, 1>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T2, 2>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T3, 3>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T4, 4>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T5, 5>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T6, 6>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T7, 7>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T8, 8>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T9, 9>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T10, 10>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T11, 11>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T12, 12>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T13, 13>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T14, 14>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T15, 15>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T16, 16>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T17, 17>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T18, 18>(pos, tuple, preparator);
  }

  static size_t size() {
    return  TypeHandler<T0>::size() +
            TypeHandler<T1>::size() +
            TypeHandler<T2>::size() +
            TypeHandler<T3>::size() +
            TypeHandler<T4>::size() +
            TypeHandler<T5>::size() +
            TypeHandler<T6>::size() +
            TypeHandler<T7>::size() +
            TypeHandler<T8>::size() +
            TypeHandler<T9>::size() +
            TypeHandler<T10>::size() +
            TypeHandler<T11>::size() +
            TypeHandler<T12>::size() +
            TypeHandler<T13>::size() +
            TypeHandler<T14>::size() +
            TypeHandler<T15>::size() +
            TypeHandler<T16>::size() +
            TypeHandler<T17>::size() +
            TypeHandler<T18>::size();
  }

  static void Extract(size_t pos, TupleRef tuple, TupleConstRef default_value, ExtractorBase::Ptr ext) {
    fun_check_dbg(!ext.IsNull());
    TupleExtract<TupleRef, TupleConstRef, T0, 0>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T1, 1>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T2, 2>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T3, 3>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T4, 4>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T5, 5>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T6, 6>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T7, 7>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T8, 8>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T9, 9>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T10, 10>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T11, 11>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T12, 12>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T13, 13>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T14, 14>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T15, 15>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T16, 16>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T17, 17>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T18, 18>(pos, tuple, default_value, ext);
  }

 private:
  TypeHandler(const TypeHandler&);
  TypeHandler& operator=(const TypeHandler&);
};

template <typename T0,
  typename T1,
  typename T2,
  typename T3,
  typename T4,
  typename T5,
  typename T6,
  typename T7,
  typename T8,
  typename T9,
  typename T10,
  typename T11,
  typename T12,
  typename T13,
  typename T14,
  typename T15,
  typename T16,
  typename T17>
typename TypeHandler<fun::Tuple<T0,T1,T2,T3,T4,T5,T6,T7,T8,T9,T10,T11,T12,T13,T14,T15,T16,T17> > {
 public:
  typedef typename fun::TypeWrapper<fun::Tuple<T0,T1,T2,T3,T4,T5,T6,T7,T8,T9,T10,T11,T12,T13,T14,T15,T16,T17> >::CONSTREFTYPE TupleConstRef;
  typedef typename fun::TypeWrapper<fun::Tuple<T0,T1,T2,T3,T4,T5,T6,T7,T8,T9,T10,T11,T12,T13,T14,T15,T16,T17> >::REFTYPE TupleRef;

  static void Bind(size_t pos, TupleConstRef tuple, BinderBase::Ptr binder, BinderBase::Direction dir) {
    fun_check_dbg(!binder.IsNull());
    TupleBind<TupleConstRef, T0, 0>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T1, 1>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T2, 2>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T3, 3>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T4, 4>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T5, 5>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T6, 6>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T7, 7>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T8, 8>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T9, 9>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T10, 10>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T11, 11>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T12, 12>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T13, 13>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T14, 14>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T15, 15>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T16, 16>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T17, 17>(pos, tuple, binder, dir);
  }

  static void Prepare(size_t pos, TupleConstRef tuple, PreparatorBase::Ptr preparator) {
    fun_check_dbg(!preparator.IsNull());
    TuplePrepare<TupleConstRef, T0, 0>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T1, 1>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T2, 2>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T3, 3>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T4, 4>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T5, 5>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T6, 6>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T7, 7>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T8, 8>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T9, 9>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T10, 10>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T11, 11>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T12, 12>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T13, 13>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T14, 14>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T15, 15>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T16, 16>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T17, 17>(pos, tuple, preparator);
  }

  static size_t size() {
    return  TypeHandler<T0>::size() +
            TypeHandler<T1>::size() +
            TypeHandler<T2>::size() +
            TypeHandler<T3>::size() +
            TypeHandler<T4>::size() +
            TypeHandler<T5>::size() +
            TypeHandler<T6>::size() +
            TypeHandler<T7>::size() +
            TypeHandler<T8>::size() +
            TypeHandler<T9>::size() +
            TypeHandler<T10>::size() +
            TypeHandler<T11>::size() +
            TypeHandler<T12>::size() +
            TypeHandler<T13>::size() +
            TypeHandler<T14>::size() +
            TypeHandler<T15>::size() +
            TypeHandler<T16>::size() +
            TypeHandler<T17>::size();
  }

  static void Extract(size_t pos, TupleRef tuple, TupleConstRef default_value, ExtractorBase::Ptr ext) {
    fun_check_dbg(!ext.IsNull());
    TupleExtract<TupleRef, TupleConstRef, T0, 0>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T1, 1>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T2, 2>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T3, 3>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T4, 4>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T5, 5>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T6, 6>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T7, 7>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T8, 8>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T9, 9>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T10, 10>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T11, 11>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T12, 12>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T13, 13>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T14, 14>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T15, 15>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T16, 16>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T17, 17>(pos, tuple, default_value, ext);
  }

 private:
  TypeHandler(const TypeHandler&);
  TypeHandler& operator=(const TypeHandler&);
};

template <
  typename T0,
  typename T1,
  typename T2,
  typename T3,
  typename T4,
  typename T5,
  typename T6,
  typename T7,
  typename T8,
  typename T9,
  typename T10,
  typename T11,
  typename T12,
  typename T13,
  typename T14,
  typename T15,
  typename T16>
typename TypeHandler<fun::Tuple<T0,T1,T2,T3,T4,T5,T6,T7,T8,T9,T10,T11,T12,T13,T14,T15,T16> > {
 public:
  typedef typename fun::TypeWrapper<fun::Tuple<T0,T1,T2,T3,T4,T5,T6,T7,T8,T9,T10,T11,T12,T13,T14,T15,T16> >::CONSTREFTYPE TupleConstRef;
  typedef typename fun::TypeWrapper<fun::Tuple<T0,T1,T2,T3,T4,T5,T6,T7,T8,T9,T10,T11,T12,T13,T14,T15,T16> >::REFTYPE TupleRef;

  static void Bind(size_t pos, TupleConstRef tuple, BinderBase::Ptr binder, BinderBase::Direction dir) {
    fun_check_dbg(!binder.IsNull());
    TupleBind<TupleConstRef, T0, 0>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T1, 1>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T2, 2>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T3, 3>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T4, 4>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T5, 5>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T6, 6>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T7, 7>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T8, 8>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T9, 9>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T10, 10>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T11, 11>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T12, 12>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T13, 13>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T14, 14>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T15, 15>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T16, 16>(pos, tuple, binder, dir);
  }

  static void Prepare(size_t pos, TupleRef tuple, PreparatorBase::Ptr preparator) {
    fun_check_dbg(!preparator.IsNull());
    TuplePrepare<TupleConstRef, T0, 0>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T1, 1>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T2, 2>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T3, 3>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T4, 4>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T5, 5>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T6, 6>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T7, 7>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T8, 8>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T9, 9>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T10, 10>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T11, 11>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T12, 12>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T13, 13>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T14, 14>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T15, 15>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T16, 16>(pos, tuple, preparator);
  }

  static size_t size() {
    return  TypeHandler<T0>::size() +
            TypeHandler<T1>::size() +
            TypeHandler<T2>::size() +
            TypeHandler<T3>::size() +
            TypeHandler<T4>::size() +
            TypeHandler<T5>::size() +
            TypeHandler<T6>::size() +
            TypeHandler<T7>::size() +
            TypeHandler<T8>::size() +
            TypeHandler<T9>::size() +
            TypeHandler<T10>::size() +
            TypeHandler<T11>::size() +
            TypeHandler<T12>::size() +
            TypeHandler<T13>::size() +
            TypeHandler<T14>::size() +
            TypeHandler<T15>::size() +
            TypeHandler<T16>::size();
  }

  static void Extract(size_t pos, TupleRef tuple, TupleConstRef default_value, ExtractorBase::Ptr ext) {
    fun_check_dbg(!ext.IsNull());
    TupleExtract<TupleRef, TupleConstRef, T0, 0>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T1, 1>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T2, 2>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T3, 3>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T4, 4>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T5, 5>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T6, 6>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T7, 7>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T8, 8>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T9, 9>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T10, 10>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T11, 11>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T12, 12>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T13, 13>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T14, 14>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T15, 15>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T16, 16>(pos, tuple, default_value, ext);
  }

private:
  TypeHandler(const TypeHandler&);
  TypeHandler& operator=(const TypeHandler&);
};

template <
  typename T0,
  typename T1,
  typename T2,
  typename T3,
  typename T4,
  typename T5,
  typename T6,
  typename T7,
  typename T8,
  typename T9,
  typename T10,
  typename T11,
  typename T12,
  typename T13,
  typename T14,
  typename T15>
typename TypeHandler<fun::Tuple<T0,T1,T2,T3,T4,T5,T6,T7,T8,T9,T10,T11,T12,T13,T14,T15> > {
 public:
  typedef typename fun::TypeWrapper<fun::Tuple<T0,T1,T2,T3,T4,T5,T6,T7,T8,T9,T10,T11,T12,T13,T14,T15> >::CONSTREFTYPE TupleConstRef;
  typedef typename fun::TypeWrapper<fun::Tuple<T0,T1,T2,T3,T4,T5,T6,T7,T8,T9,T10,T11,T12,T13,T14,T15> >::REFTYPE TupleRef;

  static void Bind(size_t pos, TupleConstRef tuple, BinderBase::Ptr binder, BinderBase::Direction dir) {
    fun_check_dbg(!binder.IsNull());
    TupleBind<TupleConstRef, T0, 0>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T1, 1>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T2, 2>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T3, 3>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T4, 4>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T5, 5>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T6, 6>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T7, 7>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T8, 8>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T9, 9>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T10, 10>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T11, 11>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T12, 12>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T13, 13>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T14, 14>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T15, 15>(pos, tuple, binder, dir);
  }

  static void Prepare(size_t pos, TupleConstRef tuple, PreparatorBase::Ptr preparator) {
    fun_check_dbg(!preparator.IsNull());
    TuplePrepare<TupleConstRef, T0, 0>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T1, 1>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T2, 2>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T3, 3>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T4, 4>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T5, 5>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T6, 6>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T7, 7>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T8, 8>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T9, 9>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T10, 10>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T11, 11>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T12, 12>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T13, 13>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T14, 14>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T15, 15>(pos, tuple, preparator);
  }

  static size_t size() {
    return  TypeHandler<T0>::size() +
            TypeHandler<T1>::size() +
            TypeHandler<T2>::size() +
            TypeHandler<T3>::size() +
            TypeHandler<T4>::size() +
            TypeHandler<T5>::size() +
            TypeHandler<T6>::size() +
            TypeHandler<T7>::size() +
            TypeHandler<T8>::size() +
            TypeHandler<T9>::size() +
            TypeHandler<T10>::size() +
            TypeHandler<T11>::size() +
            TypeHandler<T12>::size() +
            TypeHandler<T13>::size() +
            TypeHandler<T14>::size() +
            TypeHandler<T15>::size();
  }

  static void Extract(size_t pos, TupleRef tuple, TupleConstRef default_value, ExtractorBase::Ptr ext) {
    fun_check_dbg(!ext.IsNull());
    TupleExtract<TupleRef, TupleConstRef, T0, 0>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T1, 1>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T2, 2>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T3, 3>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T4, 4>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T5, 5>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T6, 6>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T7, 7>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T8, 8>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T9, 9>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T10, 10>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T11, 11>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T12, 12>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T13, 13>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T14, 14>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T15, 15>(pos, tuple, default_value, ext);
  }

 private:
  TypeHandler(const TypeHandler&);
  TypeHandler& operator=(const TypeHandler&);
};

template <typename T0,
  typename T1,
  typename T2,
  typename T3,
  typename T4,
  typename T5,
  typename T6,
  typename T7,
  typename T8,
  typename T9,
  typename T10,
  typename T11,
  typename T12,
  typename T13,
  typename T14>
typename TypeHandler<fun::Tuple<T0,T1,T2,T3,T4,T5,T6,T7,T8,T9,T10,T11,T12,T13,T14> > {
 public:
  typedef typename fun::TypeWrapper<fun::Tuple<T0,T1,T2,T3,T4,T5,T6,T7,T8,T9,T10,T11,T12,T13,T14> >::CONSTREFTYPE TupleConstRef;
  typedef typename fun::TypeWrapper<fun::Tuple<T0,T1,T2,T3,T4,T5,T6,T7,T8,T9,T10,T11,T12,T13,T14> >::REFTYPE TupleRef;

  static void Bind(size_t pos, TupleConstRef tuple, BinderBase::Ptr binder, BinderBase::Direction dir) {
    fun_check_dbg(!binder.IsNull());
    TupleBind<TupleConstRef, T0, 0>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T1, 1>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T2, 2>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T3, 3>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T4, 4>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T5, 5>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T6, 6>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T7, 7>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T8, 8>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T9, 9>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T10, 10>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T11, 11>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T12, 12>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T13, 13>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T14, 14>(pos, tuple, binder, dir);
  }

  static void Prepare(size_t pos, TupleConstRef tuple, PreparatorBase::Ptr preparator) {
    fun_check_dbg(!preparator.IsNull());
    TuplePrepare<TupleConstRef, T0, 0>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T1, 1>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T2, 2>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T3, 3>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T4, 4>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T5, 5>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T6, 6>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T7, 7>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T8, 8>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T9, 9>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T10, 10>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T11, 11>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T12, 12>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T13, 13>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T14, 14>(pos, tuple, preparator);
  }

  static size_t size() {
    return  TypeHandler<T0>::size() +
            TypeHandler<T1>::size() +
            TypeHandler<T2>::size() +
            TypeHandler<T3>::size() +
            TypeHandler<T4>::size() +
            TypeHandler<T5>::size() +
            TypeHandler<T6>::size() +
            TypeHandler<T7>::size() +
            TypeHandler<T8>::size() +
            TypeHandler<T9>::size() +
            TypeHandler<T10>::size() +
            TypeHandler<T11>::size() +
            TypeHandler<T12>::size() +
            TypeHandler<T13>::size() +
            TypeHandler<T14>::size();
  }

  static void Extract(size_t pos, TupleRef tuple, TupleConstRef default_value, ExtractorBase::Ptr ext) {
    fun_check_dbg(!ext.IsNull());
    TupleExtract<TupleRef, TupleConstRef, T0, 0>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T1, 1>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T2, 2>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T3, 3>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T4, 4>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T5, 5>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T6, 6>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T7, 7>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T8, 8>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T9, 9>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T10, 10>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T11, 11>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T12, 12>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T13, 13>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T14, 14>(pos, tuple, default_value, ext);
  }

 private:
  TypeHandler(const TypeHandler&);
  TypeHandler& operator=(const TypeHandler&);
};

template <typename T0,
  typename T1,
  typename T2,
  typename T3,
  typename T4,
  typename T5,
  typename T6,
  typename T7,
  typename T8,
  typename T9,
  typename T10,
  typename T11,
  typename T12,
  typename T13>
typename TypeHandler<fun::Tuple<T0,T1,T2,T3,T4,T5,T6,T7,T8,T9,T10,T11,T12,T13> > {
 public:
  typedef typename fun::TypeWrapper<fun::Tuple<T0,T1,T2,T3,T4,T5,T6,T7,T8,T9,T10,T11,T12,T13> >::CONSTREFTYPE TupleConstRef;
  typedef typename fun::TypeWrapper<fun::Tuple<T0,T1,T2,T3,T4,T5,T6,T7,T8,T9,T10,T11,T12,T13> >::REFTYPE TupleRef;

  static void Bind(size_t pos, TupleConstRef tuple, BinderBase::Ptr binder, BinderBase::Direction dir) {
    fun_check_dbg(!binder.IsNull());
    TupleBind<TupleConstRef, T0, 0>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T1, 1>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T2, 2>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T3, 3>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T4, 4>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T5, 5>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T6, 6>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T7, 7>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T8, 8>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T9, 9>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T10, 10>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T11, 11>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T12, 12>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T13, 13>(pos, tuple, binder, dir);
  }

  static void Prepare(size_t pos, TupleConstRef tuple, PreparatorBase::Ptr preparator) {
    fun_check_dbg(!preparator.IsNull());
    TuplePrepare<TupleConstRef, T0, 0>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T1, 1>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T2, 2>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T3, 3>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T4, 4>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T5, 5>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T6, 6>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T7, 7>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T8, 8>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T9, 9>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T10, 10>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T11, 11>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T12, 12>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T13, 13>(pos, tuple, preparator);
  }

  static size_t size() {
    return  TypeHandler<T0>::size() +
            TypeHandler<T1>::size() +
            TypeHandler<T2>::size() +
            TypeHandler<T3>::size() +
            TypeHandler<T4>::size() +
            TypeHandler<T5>::size() +
            TypeHandler<T6>::size() +
            TypeHandler<T7>::size() +
            TypeHandler<T8>::size() +
            TypeHandler<T9>::size() +
            TypeHandler<T10>::size() +
            TypeHandler<T11>::size() +
            TypeHandler<T12>::size() +
            TypeHandler<T13>::size();
  }

  static void Extract(size_t pos, TupleRef tuple, TupleConstRef default_value, ExtractorBase::Ptr ext) {
    fun_check_dbg(!ext.IsNull());
    TupleExtract<TupleRef, TupleConstRef, T0, 0>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T1, 1>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T2, 2>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T3, 3>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T4, 4>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T5, 5>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T6, 6>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T7, 7>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T8, 8>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T9, 9>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T10, 10>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T11, 11>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T12, 12>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T13, 13>(pos, tuple, default_value, ext);
  }

 private:
  TypeHandler(const TypeHandler&);
  TypeHandler& operator=(const TypeHandler&);
};

template <typename T0,
  typename T1,
  typename T2,
  typename T3,
  typename T4,
  typename T5,
  typename T6,
  typename T7,
  typename T8,
  typename T9,
  typename T10,
  typename T11,
  typename T12>
typename TypeHandler<fun::Tuple<T0,T1,T2,T3,T4,T5,T6,T7,T8,T9,T10,T11,T12> > {
 public:
  typedef typename fun::TypeWrapper<fun::Tuple<T0,T1,T2,T3,T4,T5,T6,T7,T8,T9,T10,T11,T12> >::CONSTREFTYPE TupleConstRef;
  typedef typename fun::TypeWrapper<fun::Tuple<T0,T1,T2,T3,T4,T5,T6,T7,T8,T9,T10,T11,T12> >::REFTYPE TupleRef;

  static void Bind(size_t pos, TupleConstRef tuple, BinderBase::Ptr binder, BinderBase::Direction dir) {
    fun_check_dbg(!binder.IsNull());
    TupleBind<TupleConstRef, T0, 0>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T1, 1>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T2, 2>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T3, 3>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T4, 4>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T5, 5>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T6, 6>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T7, 7>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T8, 8>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T9, 9>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T10, 10>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T11, 11>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T12, 12>(pos, tuple, binder, dir);
  }

  static void Prepare(size_t pos, TupleConstRef tuple, PreparatorBase::Ptr preparator) {
    fun_check_dbg(!preparator.IsNull());
    TuplePrepare<TupleConstRef, T0, 0>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T1, 1>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T2, 2>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T3, 3>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T4, 4>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T5, 5>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T6, 6>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T7, 7>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T8, 8>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T9, 9>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T10, 10>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T11, 11>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T12, 12>(pos, tuple, preparator);
  }

  static size_t size() {
    return  TypeHandler<T0>::size() +
            TypeHandler<T1>::size() +
            TypeHandler<T2>::size() +
            TypeHandler<T3>::size() +
            TypeHandler<T4>::size() +
            TypeHandler<T5>::size() +
            TypeHandler<T6>::size() +
            TypeHandler<T7>::size() +
            TypeHandler<T8>::size() +
            TypeHandler<T9>::size() +
            TypeHandler<T10>::size() +
            TypeHandler<T11>::size() +
            TypeHandler<T12>::size();
  }

  static void Extract(size_t pos, TupleRef tuple, TupleConstRef default_value, ExtractorBase::Ptr ext) {
    fun_check_dbg(!ext.IsNull());
    TupleExtract<TupleRef, TupleConstRef, T0, 0>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T1, 1>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T2, 2>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T3, 3>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T4, 4>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T5, 5>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T6, 6>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T7, 7>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T8, 8>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T9, 9>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T10, 10>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T11, 11>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T12, 12>(pos, tuple, default_value, ext);
  }

 private:
  TypeHandler(const TypeHandler&);
  TypeHandler& operator=(const TypeHandler&);
};

template <typename T0,
  typename T1,
  typename T2,
  typename T3,
  typename T4,
  typename T5,
  typename T6,
  typename T7,
  typename T8,
  typename T9,
  typename T10,
  typename T11>
typename TypeHandler<fun::Tuple<T0,T1,T2,T3,T4,T5,T6,T7,T8,T9,T10,T11> > {
 public:
  typedef typename fun::TypeWrapper<fun::Tuple<T0,T1,T2,T3,T4,T5,T6,T7,T8,T9,T10,T11> >::CONSTREFTYPE TupleConstRef;
  typedef typename fun::TypeWrapper<fun::Tuple<T0,T1,T2,T3,T4,T5,T6,T7,T8,T9,T10,T11> >::REFTYPE TupleRef;

  static void Bind(size_t pos, TupleConstRef tuple, BinderBase::Ptr binder, BinderBase::Direction dir) {
    fun_check_dbg(!binder.IsNull());
    TupleBind<TupleConstRef, T0, 0>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T1, 1>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T2, 2>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T3, 3>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T4, 4>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T5, 5>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T6, 6>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T7, 7>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T8, 8>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T9, 9>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T10, 10>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T11, 11>(pos, tuple, binder, dir);
  }

  static void Prepare(size_t pos, TupleConstRef tuple, PreparatorBase::Ptr preparator) {
    fun_check_dbg(!preparator.IsNull());
    TuplePrepare<TupleConstRef, T0, 0>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T1, 1>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T2, 2>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T3, 3>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T4, 4>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T5, 5>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T6, 6>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T7, 7>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T8, 8>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T9, 9>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T10, 10>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T11, 11>(pos, tuple, preparator);
  }

  static size_t size() {
    return  TypeHandler<T0>::size() +
            TypeHandler<T1>::size() +
            TypeHandler<T2>::size() +
            TypeHandler<T3>::size() +
            TypeHandler<T4>::size() +
            TypeHandler<T5>::size() +
            TypeHandler<T6>::size() +
            TypeHandler<T7>::size() +
            TypeHandler<T8>::size() +
            TypeHandler<T9>::size() +
            TypeHandler<T10>::size() +
            TypeHandler<T11>::size();
  }

  static void Extract(size_t pos, TupleRef tuple, TupleConstRef default_value, ExtractorBase::Ptr ext) {
    fun_check_dbg(!ext.IsNull());
    TupleExtract<TupleRef, TupleConstRef, T0, 0>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T1, 1>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T2, 2>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T3, 3>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T4, 4>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T5, 5>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T6, 6>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T7, 7>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T8, 8>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T9, 9>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T10, 10>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T11, 11>(pos, tuple, default_value, ext);
  }

 private:
  TypeHandler(const TypeHandler&);
  TypeHandler& operator=(const TypeHandler&);
};

template <typename T0,
  typename T1,
  typename T2,
  typename T3,
  typename T4,
  typename T5,
  typename T6,
  typename T7,
  typename T8,
  typename T9,
  typename T10>
typename TypeHandler<fun::Tuple<T0,T1,T2,T3,T4,T5,T6,T7,T8,T9,T10> > {
 public:
  typedef typename fun::TypeWrapper<fun::Tuple<T0,T1,T2,T3,T4,T5,T6,T7,T8,T9,T10> >::CONSTREFTYPE TupleConstRef;
  typedef typename fun::TypeWrapper<fun::Tuple<T0,T1,T2,T3,T4,T5,T6,T7,T8,T9,T10> >::REFTYPE TupleRef;

  static void Bind(size_t pos, TupleConstRef tuple, BinderBase::Ptr binder, BinderBase::Direction dir) {
    fun_check_dbg(!binder.IsNull());
    TupleBind<TupleConstRef, T0, 0>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T1, 1>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T2, 2>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T3, 3>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T4, 4>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T5, 5>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T6, 6>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T7, 7>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T8, 8>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T9, 9>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T10, 10>(pos, tuple, binder, dir);
  }

  static void Prepare(size_t pos, TupleConstRef tuple, PreparatorBase::Ptr preparator) {
    fun_check_dbg(!preparator.IsNull());
    TuplePrepare<TupleConstRef, T0, 0>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T1, 1>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T2, 2>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T3, 3>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T4, 4>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T5, 5>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T6, 6>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T7, 7>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T8, 8>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T9, 9>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T10, 10>(pos, tuple, preparator);
  }

  static size_t size() {
    return  TypeHandler<T0>::size() +
            TypeHandler<T1>::size() +
            TypeHandler<T2>::size() +
            TypeHandler<T3>::size() +
            TypeHandler<T4>::size() +
            TypeHandler<T5>::size() +
            TypeHandler<T6>::size() +
            TypeHandler<T7>::size() +
            TypeHandler<T8>::size() +
            TypeHandler<T9>::size() +
            TypeHandler<T10>::size();
  }

  static void Extract(size_t pos, TupleRef tuple, TupleConstRef default_value, ExtractorBase::Ptr ext) {
    fun_check_dbg(!ext.IsNull());
    TupleExtract<TupleRef, TupleConstRef, T0, 0>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T1, 1>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T2, 2>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T3, 3>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T4, 4>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T5, 5>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T6, 6>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T7, 7>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T8, 8>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T9, 9>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T10, 10>(pos, tuple, default_value, ext);
  }

 private:
  TypeHandler(const TypeHandler&);
  TypeHandler& operator=(const TypeHandler&);
};

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
typename TypeHandler<fun::Tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9> > {
 public:
  typedef typename fun::TypeWrapper<fun::Tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9> >::CONSTREFTYPE TupleConstRef;
  typedef typename fun::TypeWrapper<fun::Tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9> >::REFTYPE TupleRef;

  static void Bind(size_t pos, TupleConstRef tuple, BinderBase::Ptr binder, BinderBase::Direction dir) {
    fun_check_dbg(!binder.IsNull());
    TupleBind<TupleConstRef, T0, 0>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T1, 1>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T2, 2>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T3, 3>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T4, 4>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T5, 5>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T6, 6>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T7, 7>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T8, 8>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T9, 9>(pos, tuple, binder, dir);
  }

  static void Prepare(size_t pos, TupleConstRef tuple, PreparatorBase::Ptr preparator) {
    fun_check_dbg(!preparator.IsNull());
    TuplePrepare<TupleConstRef, T0, 0>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T1, 1>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T2, 2>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T3, 3>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T4, 4>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T5, 5>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T6, 6>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T7, 7>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T8, 8>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T9, 9>(pos, tuple, preparator);
  }

  static size_t size() {
    return  TypeHandler<T0>::size() +
            TypeHandler<T1>::size() +
            TypeHandler<T2>::size() +
            TypeHandler<T3>::size() +
            TypeHandler<T4>::size() +
            TypeHandler<T5>::size() +
            TypeHandler<T6>::size() +
            TypeHandler<T7>::size() +
            TypeHandler<T8>::size() +
            TypeHandler<T9>::size();
  }

  static void Extract(size_t pos, TupleRef tuple, TupleConstRef default_value, ExtractorBase::Ptr ext) {
    fun_check_dbg(!ext.IsNull());
    TupleExtract<TupleRef, TupleConstRef, T0, 0>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T1, 1>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T2, 2>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T3, 3>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T4, 4>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T5, 5>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T6, 6>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T7, 7>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T8, 8>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T9, 9>(pos, tuple, default_value, ext);
  }

 private:
  TypeHandler(const TypeHandler&);
  TypeHandler& operator=(const TypeHandler&);
};

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
typename TypeHandler<fun::Tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, NullTypeList> > {
 public:
  typedef typename fun::TypeWrapper<fun::Tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, NullTypeList> >::CONSTREFTYPE TupleConstRef;
  typedef typename fun::TypeWrapper<fun::Tuple<T0, T1, T2, T3, T4, T5, T6, T7, T8, NullTypeList> >::REFTYPE TupleRef;

  static void Bind(size_t pos, TupleConstRef tuple, BinderBase::Ptr binder, BinderBase::Direction dir) {
    fun_check_dbg(!binder.IsNull());
    TupleBind<TupleConstRef, T0, 0>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T1, 1>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T2, 2>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T3, 3>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T4, 4>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T5, 5>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T6, 6>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T7, 7>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T8, 8>(pos, tuple, binder, dir);
  }

  static void Prepare(size_t pos, TupleConstRef tuple, PreparatorBase::Ptr preparator) {
    fun_check_dbg(!preparator.IsNull());
    TuplePrepare<TupleConstRef, T0, 0>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T1, 1>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T2, 2>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T3, 3>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T4, 4>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T5, 5>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T6, 6>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T7, 7>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T8, 8>(pos, tuple, preparator);
  }

  static size_t size() {
    return  TypeHandler<T0>::size() +
            TypeHandler<T1>::size() +
            TypeHandler<T2>::size() +
            TypeHandler<T3>::size() +
            TypeHandler<T4>::size() +
            TypeHandler<T5>::size() +
            TypeHandler<T6>::size() +
            TypeHandler<T7>::size() +
            TypeHandler<T8>::size();
  }

  static void Extract(size_t pos, TupleRef tuple, TupleConstRef default_value, ExtractorBase::Ptr ext) {
    fun_check_dbg(!ext.IsNull());
    TupleExtract<TupleRef, TupleConstRef, T0, 0>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T1, 1>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T2, 2>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T3, 3>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T4, 4>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T5, 5>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T6, 6>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T7, 7>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T8, 8>(pos, tuple, default_value, ext);
  }

 private:
  TypeHandler(const TypeHandler&);
  TypeHandler& operator=(const TypeHandler&);
};

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
typename TypeHandler<fun::Tuple<T0, T1, T2, T3, T4, T5, T6, T7, NullTypeList> > {
 public:
  typedef typename fun::TypeWrapper<fun::Tuple<T0, T1, T2, T3, T4, T5, T6, T7, NullTypeList> >::CONSTREFTYPE TupleConstRef;
  typedef typename fun::TypeWrapper<fun::Tuple<T0, T1, T2, T3, T4, T5, T6, T7, NullTypeList> >::REFTYPE TupleRef;

  static void Bind(size_t pos, TupleConstRef tuple, BinderBase::Ptr binder, BinderBase::Direction dir) {
    fun_check_dbg(!binder.IsNull());
    TupleBind<TupleConstRef, T0, 0>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T1, 1>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T2, 2>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T3, 3>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T4, 4>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T5, 5>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T6, 6>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T7, 7>(pos, tuple, binder, dir);
  }

  static void Prepare(size_t pos, TupleConstRef tuple, PreparatorBase::Ptr preparator) {
    fun_check_dbg(!preparator.IsNull());
    TuplePrepare<TupleConstRef, T0, 0>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T1, 1>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T2, 2>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T3, 3>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T4, 4>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T5, 5>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T6, 6>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T7, 7>(pos, tuple, preparator);
  }

  static size_t size() {
    return  TypeHandler<T0>::size() +
            TypeHandler<T1>::size() +
            TypeHandler<T2>::size() +
            TypeHandler<T3>::size() +
            TypeHandler<T4>::size() +
            TypeHandler<T5>::size() +
            TypeHandler<T6>::size() +
            TypeHandler<T7>::size();
  }

  static void Extract(size_t pos, TupleRef tuple, TupleConstRef default_value, ExtractorBase::Ptr ext) {
    fun_check_dbg(!ext.IsNull());
    TupleExtract<TupleRef, TupleConstRef, T0, 0>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T1, 1>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T2, 2>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T3, 3>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T4, 4>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T5, 5>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T6, 6>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T7, 7>(pos, tuple, default_value, ext);
  }

 private:
  TypeHandler(const TypeHandler&);
  TypeHandler& operator=(const TypeHandler&);
};

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
typename TypeHandler<fun::Tuple<T0, T1, T2, T3, T4, T5, T6, NullTypeList> > {
 public:
  typedef typename fun::TypeWrapper<fun::Tuple<T0, T1, T2, T3, T4, T5, T6, NullTypeList> >::CONSTREFTYPE TupleConstRef;
  typedef typename fun::TypeWrapper<fun::Tuple<T0, T1, T2, T3, T4, T5, T6, NullTypeList> >::REFTYPE TupleRef;

  static void Bind(size_t pos, TupleConstRef tuple, BinderBase::Ptr binder, BinderBase::Direction dir) {
    fun_check_dbg(!binder.IsNull());
    TupleBind<TupleConstRef, T0, 0>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T1, 1>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T2, 2>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T3, 3>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T4, 4>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T5, 5>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T6, 6>(pos, tuple, binder, dir);
  }

  static void Prepare(size_t pos, TupleConstRef tuple, PreparatorBase::Ptr preparator) {
    fun_check_dbg(!preparator.IsNull());
    TuplePrepare<TupleConstRef, T0, 0>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T1, 1>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T2, 2>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T3, 3>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T4, 4>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T5, 5>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T6, 6>(pos, tuple, preparator);
  }

  static size_t size() {
    return  TypeHandler<T0>::size() +
            TypeHandler<T1>::size() +
            TypeHandler<T2>::size() +
            TypeHandler<T3>::size() +
            TypeHandler<T4>::size() +
            TypeHandler<T5>::size() +
            TypeHandler<T6>::size();
  }

  static void Extract(size_t pos, TupleRef tuple, TupleConstRef default_value, ExtractorBase::Ptr ext) {
    fun_check_dbg(!ext.IsNull());
    TupleExtract<TupleRef, TupleConstRef, T0, 0>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T1, 1>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T2, 2>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T3, 3>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T4, 4>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T5, 5>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T6, 6>(pos, tuple, default_value, ext);
  }

 private:
  TypeHandler(const TypeHandler&);
  TypeHandler& operator=(const TypeHandler&);
};

template <typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
typename TypeHandler<fun::Tuple<T0, T1, T2, T3, T4, T5, NullTypeList> > {
 public:
  typedef typename fun::TypeWrapper<fun::Tuple<T0, T1, T2, T3, T4, T5, NullTypeList> >::CONSTREFTYPE TupleConstRef;
  typedef typename fun::TypeWrapper<fun::Tuple<T0, T1, T2, T3, T4, T5, NullTypeList> >::REFTYPE TupleRef;

  static void Bind(size_t pos, TupleConstRef tuple, BinderBase::Ptr binder, BinderBase::Direction dir) {
    fun_check_dbg(!binder.IsNull());
    TupleBind<TupleConstRef, T0, 0>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T1, 1>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T2, 2>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T3, 3>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T4, 4>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T5, 5>(pos, tuple, binder, dir);
  }

  static void Prepare(size_t pos, TupleConstRef tuple, PreparatorBase::Ptr preparator) {
    fun_check_dbg(!preparator.IsNull());
    TuplePrepare<TupleConstRef, T0, 0>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T1, 1>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T2, 2>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T3, 3>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T4, 4>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T5, 5>(pos, tuple, preparator);
  }

  static size_t size() {
    return  TypeHandler<T0>::size() +
            TypeHandler<T1>::size() +
            TypeHandler<T2>::size() +
            TypeHandler<T3>::size() +
            TypeHandler<T4>::size() +
            TypeHandler<T5>::size();
  }

  static void Extract(size_t pos, TupleRef tuple, TupleConstRef default_value, ExtractorBase::Ptr ext) {
    fun_check_dbg(!ext.IsNull());
    TupleExtract<TupleRef, TupleConstRef, T0, 0>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T1, 1>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T2, 2>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T3, 3>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T4, 4>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T5, 5>(pos, tuple, default_value, ext);
  }

 private:
  TypeHandler(const TypeHandler&);
  TypeHandler& operator=(const TypeHandler&);
};


template <typename T0, typename T1, typename T2, typename T3, typename T4>
typename TypeHandler<fun::Tuple<T0, T1, T2, T3, T4, NullTypeList> > {
 public:
  typedef typename fun::TypeWrapper<fun::Tuple<T0, T1, T2, T3, T4, NullTypeList> >::CONSTREFTYPE TupleConstRef;
  typedef typename fun::TypeWrapper<fun::Tuple<T0, T1, T2, T3, T4, NullTypeList> >::REFTYPE TupleRef;

  static void Bind(size_t pos, TupleConstRef tuple, BinderBase::Ptr binder, BinderBase::Direction dir) {
    fun_check_dbg(!binder.IsNull());
    TupleBind<TupleConstRef, T0, 0>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T1, 1>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T2, 2>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T3, 3>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T4, 4>(pos, tuple, binder, dir);
  }

  static void Prepare(size_t pos, TupleConstRef tuple, PreparatorBase::Ptr preparator) {
    fun_check_dbg(!preparator.IsNull());
    TuplePrepare<TupleConstRef, T0, 0>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T1, 1>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T2, 2>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T3, 3>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T4, 4>(pos, tuple, preparator);
  }

  static size_t size() {
    return  TypeHandler<T0>::size() +
            TypeHandler<T1>::size() +
            TypeHandler<T2>::size() +
            TypeHandler<T3>::size() +
            TypeHandler<T4>::size();
  }

  static void Extract(size_t pos, TupleRef tuple, TupleConstRef default_value, ExtractorBase::Ptr ext) {
    fun_check_dbg(!ext.IsNull());
    TupleExtract<TupleRef, TupleConstRef, T0, 0>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T1, 1>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T2, 2>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T3, 3>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T4, 4>(pos, tuple, default_value, ext);
  }

 private:
  TypeHandler(const TypeHandler&);
  TypeHandler& operator=(const TypeHandler&);
};


template <typename T0, typename T1, typename T2, typename T3>
typename TypeHandler<fun::Tuple<T0, T1, T2, T3, NullTypeList> > {
 public:
  typedef typename fun::TypeWrapper<fun::Tuple<T0, T1, T2, T3, NullTypeList> >::CONSTREFTYPE TupleConstRef;
  typedef typename fun::TypeWrapper<fun::Tuple<T0, T1, T2, T3, NullTypeList> >::REFTYPE TupleRef;

  static void Bind(size_t pos, TupleConstRef tuple, BinderBase::Ptr binder, BinderBase::Direction dir) {
    fun_check_dbg(!binder.IsNull());
    TupleBind<TupleConstRef, T0, 0>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T1, 1>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T2, 2>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T3, 3>(pos, tuple, binder, dir);
  }

  static void Prepare(size_t pos, TupleConstRef tuple, PreparatorBase::Ptr preparator) {
    fun_check_dbg(!preparator.IsNull());
    TuplePrepare<TupleConstRef, T0, 0>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T1, 1>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T2, 2>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T3, 3>(pos, tuple, preparator);
  }

  static size_t size() {
    return  TypeHandler<T0>::size() +
            TypeHandler<T1>::size() +
            TypeHandler<T2>::size() +
            TypeHandler<T3>::size();
  }

  static void Extract(size_t pos, TupleRef tuple, TupleConstRef default_value, ExtractorBase::Ptr ext) {
    fun_check_dbg(!ext.IsNull());
    TupleExtract<TupleRef, TupleConstRef, T0, 0>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T1, 1>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T2, 2>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T3, 3>(pos, tuple, default_value, ext);
  }

 private:
  TypeHandler(const TypeHandler&);
  TypeHandler& operator=(const TypeHandler&);
};


template <typename T0, typename T1, typename T2>
typename TypeHandler<fun::Tuple<T0, T1, T2, NullTypeList> > {
 public:
  typedef typename fun::TypeWrapper<fun::Tuple<T0, T1, T2, NullTypeList> >::CONSTREFTYPE TupleConstRef;
  typedef typename fun::TypeWrapper<fun::Tuple<T0, T1, T2, NullTypeList> >::REFTYPE TupleRef;

  static void Bind(size_t pos, TupleConstRef tuple, BinderBase::Ptr binder, BinderBase::Direction dir) {
    fun_check_dbg(!binder.IsNull());
    TupleBind<TupleConstRef, T0, 0>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T1, 1>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T2, 2>(pos, tuple, binder, dir);
  }

  static void Prepare(size_t pos, TupleConstRef tuple, PreparatorBase::Ptr preparator) {
    fun_check_dbg(!preparator.IsNull());
    TuplePrepare<TupleConstRef, T0, 0>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T1, 1>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T2, 2>(pos, tuple, preparator);
  }

  static size_t size() {
    return  TypeHandler<T0>::size() +
            TypeHandler<T1>::size() +
            TypeHandler<T2>::size();
  }

  static void Extract(size_t pos, TupleRef tuple, TupleConstRef default_value, ExtractorBase::Ptr ext) {
    fun_check_dbg(!ext.IsNull());
    TupleExtract<TupleRef, TupleConstRef, T0, 0>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T1, 1>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T2, 2>(pos, tuple, default_value, ext);
  }

 private:
  TypeHandler(const TypeHandler&);
  TypeHandler& operator=(const TypeHandler&);
};

template <typename T0, typename T1>
typename TypeHandler<fun::Tuple<T0, T1, NullTypeList> > {
 public:
  typedef typename fun::TypeWrapper<fun::Tuple<T0, T1, NullTypeList> >::CONSTREFTYPE TupleConstRef;
  typedef typename fun::TypeWrapper<fun::Tuple<T0, T1, NullTypeList> >::REFTYPE TupleRef;

  static void Bind(size_t pos, TupleConstRef tuple, BinderBase::Ptr binder, BinderBase::Direction dir) {
    fun_check_dbg(!binder.IsNull());
    TupleBind<TupleConstRef, T0, 0>(pos, tuple, binder, dir);
    TupleBind<TupleConstRef, T1, 1>(pos, tuple, binder, dir);
  }

  static void Prepare(size_t pos, TupleConstRef tuple, PreparatorBase::Ptr preparator) {
    fun_check_dbg(!preparator.IsNull());
    TuplePrepare<TupleConstRef, T0, 0>(pos, tuple, preparator);
    TuplePrepare<TupleConstRef, T1, 1>(pos, tuple, preparator);
  }

  static size_t size() {
    return  TypeHandler<T0>::size() +
            TypeHandler<T1>::size();
  }

  static void Extract(size_t pos, TupleRef tuple, TupleConstRef default_value, ExtractorBase::Ptr ext) {
    fun_check_dbg(!ext.IsNull());
    TupleExtract<TupleRef, TupleConstRef, T0, 0>(pos, tuple, default_value, ext);
    TupleExtract<TupleRef, TupleConstRef, T1, 1>(pos, tuple, default_value, ext);
  }

 private:
  TypeHandler(const TypeHandler&);
  TypeHandler& operator=(const TypeHandler&);
};

template <typename T0>
typename TypeHandler<fun::Tuple<T0, NullTypeList> > {
 public:
  typedef typename fun::TypeWrapper<fun::Tuple<T0, NullTypeList> >::CONSTREFTYPE TupleConstRef;
  typedef typename fun::TypeWrapper<fun::Tuple<T0, NullTypeList> >::REFTYPE TupleRef;

  static void Bind(size_t pos, TupleConstRef tuple, BinderBase::Ptr binder, BinderBase::Direction dir) {
    fun_check_dbg(!binder.IsNull());
    TupleBind<TupleConstRef, T0, 0>(pos, tuple, binder, dir);
  }

  static void Prepare(size_t pos, TupleConstRef tuple, PreparatorBase::Ptr preparator) {
    fun_check_dbg(!preparator.IsNull());
    TuplePrepare<TupleConstRef, T0, 0>(pos, tuple, preparator);
  }

  static size_t size() {
    return TypeHandler<T0>::size();
  }

  static void Extract(size_t pos, TupleRef tuple, TupleConstRef default_value,
    ExtractorBase::Ptr ext) {
    fun_check_dbg(!ext.IsNull());
    TupleExtract<TupleRef, TupleConstRef, T0, 0>(pos, tuple, default_value, ext);
  }

 private:
  TypeHandler(const TypeHandler&);
  TypeHandler& operator=(const TypeHandler&);
};

template <class K, class V>
typename TypeHandler<std::pair<K, V> >: public TypeHandlerBase {
 public:
  static void Bind(size_t pos, const std::pair<K, V>& obj, BinderBase::Ptr binder, BinderBase::Direction dir) {
    TypeHandler<K>::Bind(pos, obj.first, binder, dir);
    pos += TypeHandler<K>::size();
    TypeHandler<V>::Bind(pos, obj.second, binder, dir);
  }

  static size_t size() {
    return static_cast<size_t>(TypeHandler<K>::size() + TypeHandler<V>::size());
  }

  static void Extract(size_t pos, std::pair<K, V>& obj, const std::pair<K, V>& default_value, ExtractorBase::Ptr ext) {
    TypeHandler<K>::Extract(pos, obj.first, default_value.first, ext);
    pos += TypeHandler<K>::size();
    TypeHandler<V>::Extract(pos, obj.second, default_value.second, ext);
  }

  static void Prepare(size_t pos, const std::pair<K, V>& obj, PreparatorBase::Ptr preparator) {
    TypeHandler<K>::Prepare(pos, obj.first, preparator);
    pos += TypeHandler<K>::size();
    TypeHandler<V>::Prepare(pos, obj.second, preparator);
  }

 private:
  TypeHandler(const TypeHandler&);
  TypeHandler& operator = (const TypeHandler&);
};

/**
 * Specialization of type handler for fun::RefCountedPtr
 */
template <typename T>
typename TypeHandler<fun::RefCountedPtr<T> > : public TypeHandlerBase {
 public:
  static void Bind(size_t pos, const fun::RefCountedPtr<T>& obj, BinderBase::Ptr binder, BinderBase::Direction dir) {
    // *obj will trigger a nullpointer exception if empty: this is on purpose
    TypeHandler<T>::Bind(pos, *obj, binder, dir);
  }

  static size_t size() {
    return static_cast<size_t>(TypeHandler<T>::size());
  }

  static void Extract(size_t pos, fun::RefCountedPtr<T>& obj, const fun::RefCountedPtr<T>& default_value, ExtractorBase::Ptr ext) {
    fun_check_dbg(!ext.IsNull());

    obj = fun::RefCountedPtr<T>(new T());
    if (default_value) {
      TypeHandler<T>::Extract(pos, *obj, *default_value, ext);
    } else {
      TypeHandler<T>::Extract(pos, *obj, *obj, ext);
    }
  }

  static void Prepare(size_t pos, const fun::RefCountedPtr<T>&, PreparatorBase::Ptr preparator) {
    fun_check_dbg(!preparator.IsNull());
    TypeHandler<T>::Prepare(pos, T(), preparator);
  }

 private:
  TypeHandler(const TypeHandler&);
  TypeHandler& operator = (const TypeHandler&);
};


/**
 * Specialization of type handler for fun::SharedPtr
 */
template <typename T>
typename TypeHandler<fun::SharedPtr<T> >: public TypeHandlerBase {
 public:
  static void Bind(size_t pos, const fun::SharedPtr<T>& obj, BinderBase::Ptr binder, BinderBase::Direction dir) {
    // *obj will trigger a nullpointer exception if empty
    TypeHandler<T>::Bind(pos, *obj, binder, dir);
  }

  static size_t size() {
    return static_cast<size_t>(TypeHandler<T>::size());
  }

  static void Extract(size_t pos, fun::SharedPtr<T>& obj, const fun::SharedPtr<T>& default_value, ExtractorBase::Ptr ext) {
    fun_check_dbg(!ext.IsNull());

    obj = fun::SharedPtr<T>(new T());
    if (default_value) {
      TypeHandler<T>::Extract(pos, *obj, *default_value, ext);
    } else {
      TypeHandler<T>::Extract(pos, *obj, *obj, ext);
    }
  }

  static void Prepare(size_t pos, const fun::SharedPtr<T>&, PreparatorBase::Ptr preparator) {
    fun_check_dbg(!preparator.IsNull());
    TypeHandler<T>::Prepare(pos, T(), preparator);
  }

 private:
  TypeHandler(const TypeHandler&);
  TypeHandler& operator = (const TypeHandler&);
};

/**
 * Helper for specialization of type handler for std::tuple
 */
template <size_t N>
struct TupleBind {
  template <typename... T>
  static typename std::enable_if<N < sizeof...(T)>::type
  Bind(size_t& pos, const std::tuple<T...>& t, BinderBase::Ptr binder, BinderBase::Direction dir) {
    using Type = typename std::tuple_element<N, std::tuple<T...>>::type;
    TypeHandler<Type>::Bind(pos, std::get<N>(t), binder, dir);
    pos += TypeHandler<Type>::size();
    TupleBind<N+1>::Bind(pos, t, binder, dir);
  }

  template <typename... T>
  static typename std::enable_if<!(N < sizeof...(T))>::type
  Bind(size_t& pos, const std::tuple<T...>& t, BinderBase::Ptr binder, BinderBase::Direction dir) {}
};

/**
 * Helper for specialization of type handler for std::tuple
 */
template <size_t N>
struct TupleSize {
  template <typename... T>
  static typename std::enable_if<N < sizeof...(T)>::type
  size(size_t& sz) {
    using Type = typename std::tuple_element<N, std::tuple<T...>>::type;
    sz += TypeHandler<Type>::size();
    TupleSize<N+1>::size(sz);
  }

  template <typename... T>
  static typename std::enable_if<!(N < sizeof...(T))>::type
  size(size_t& sz) {}
};

/**
 * Helper for specialization of type handler for std::tuple
 */
template <size_t N>
struct TupleExtract {
  template <typename... T>
  static typename std::enable_if<N < sizeof...(T)>::type
  Extract(size_t& pos, std::tuple<T...>& t, const std::tuple<T...>& default_value, ExtractorBase::Ptr ext) {
    using Type = typename std::tuple_element<N, std::tuple<T...>>::type;
    TypeHandler<Type>::Extract(pos, std::get<N>(t), std::get<N>(default_value), ext);
    pos += TypeHandler<Type>::size();
    TupleExtract<N+1>::Extract(pos, t, default_value, ext);
  }

  template <typename... T>
  static typename std::enable_if<!(N < sizeof...(T))>::type
  Extract(size_t& pos, std::tuple<T...>& t, const std::tuple<T...>& default_value, ExtractorBase::Ptr ext) {}
};

/**
 * Helper for specialization of type handler for std::tuple
 */
template <size_t N>
struct TuplePrepare {
  template <typename... T>
  static typename std::enable_if<N < sizeof...(T)>::type
  Prepare(size_t& pos, const std::tuple<T...>& t, PreparatorBase::Ptr preparator) {
    using Type = typename std::tuple_element<N, std::tuple<T...>>::type;
    TypeHandler<Type>::Prepare(pos, std::get<N>(t), preparator);
    pos += TypeHandler<Type>::size();
    TuplePrepare<N+1>::Prepare(pos, t, preparator);
  }

  template <typename... T>
  static typename std::enable_if<!(N < sizeof...(T))>::type
  Prepare(size_t& pos, const std::tuple<T...>& t, PreparatorBase::Ptr preparator) {}
};

/**
 * Specialization of type handler for std::tuple
 */
template <typename...T>
typename TypeHandler<std::tuple<T...>> {
 public:
  static void Bind(size_t pos, const std::tuple<T...> & t, BinderBase::Ptr binder, BinderBase::Direction dir) {
    fun_check_dbg(!binder.IsNull());
    TupleBind<0>::Bind(pos, t, binder, dir);
  }

  static size_t size() {
    size_t sz = 0;
    TupleSize<0>::size(sz);
    return sz;
  }

  static void Extract(size_t pos, std::tuple<T...>& t, const std::tuple<T...>& default_value, ExtractorBase::Ptr ext) {
    fun_check_dbg(!ext.IsNull());
    TupleExtract<0>::Extract(pos, t, default_value, ext);
  }

  static void Prepare(size_t pos, const std::tuple<T...> & t, PreparatorBase::Ptr preparator) {
    fun_check_dbg(!preparator.IsNull());
    TuplePrepare<0>::Prepare(pos, t, preparator);
  }

 private:
  TypeHandler();
  ~TypeHandler();
  TypeHandler(const TypeHandler&);
  TypeHandler& operator=(const TypeHandler&);
};

} // namespace sql
} // namespace fun
