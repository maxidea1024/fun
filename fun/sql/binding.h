#pragma once

#include <cstddef>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <vector>
#include "fun/base/shared_ptr.h"
#include "fun/debug.h"
#include "fun/meta_programming.h"
#include "fun/sql/binding_base.h"
#include "fun/sql/sql.h"
#include "fun/sql/sql_exception.h"
#include "fun/sql/type_handler.h"

namespace fun {
namespace sql {

/**
 * Binding maps a value or multiple values (see Binding specializations for STL
 * containers as well as type handlers) to database column(s). Values to be
 * bound can be either mapped directly (by reference) or a copy can be created,
 * depending on the value of the copy argument. To pass a reference to a
 * variable, it is recommended to pass it to the intermediate utility function
 * use(), which will create the proper binding. In cases when a reference is
 * passed to binding, the storage it refers to must be valid at the statement
 * execution time. To pass a copy of a variable, constant or string literal, use
 * utility function Bind(). Variables can be passed as either copies or
 * references (i.e. using either use() or Bind()). Constants, however, can only
 * be passed as copies. this is best achieved using Bind() utility function. An
 * attempt to pass a constant by reference shall result in compile-time error.
 */
template <typename T>
class Binding : public BindingBase {
 public:
  typedef T ValType;
  typedef SharedPtr<ValType> ValPtr;
  typedef Binding<ValType> Type;
  typedef SharedPtr<Type> Ptr;

  /**
   * Creates the Binding using the passed reference as bound value.
   * If copy is true, a copy of the value referred to is created.
   */
  explicit Binding(T& val, const String& name = "", Direction direction = PD_IN)
      : BindingBase(name, direction), val_(val), bound_(false) {}

  /**
   * Destroys the Binding.
   */
  ~Binding() {}

  size_t HandledColumnsCount() const { return TypeHandler<T>::size(); }

  size_t HandledRowsCount() const { return 1u; }

  bool CanBind() const { return !bound_; }

  void Bind(size_t pos) {
    fun_check_dbg(!GetBinder().IsNull());
    TypeHandler<T>::Bind(pos, val_, GetBinder(), GetDirection());
    bound_ = true;
  }

  void Reset() {
    bound_ = false;
    BinderBase::Ptr binder = GetBinder();
    fun_check_dbg(!binder.IsNull());
    binder->Reset();
  }

 private:
  const T& val_;
  bool bound_;
};

/**
 * Binding maps a value or multiple values (see Binding specializations for STL
 * containers as well as type handlers) to database column(s). Values to be
 * bound can be either mapped directly (by reference) or a copy can be created,
 * depending on the value of the copy argument. To pass a reference to a
 * variable, it is recommended to pass it to the intermediate utility function
 * use(), which will create the proper binding. In cases when a reference is
 * passed to binding, the storage it refers to must be valid at the statement
 * execution time. To pass a copy of a variable, constant or string literal, use
 * utility function Bind(). Variables can be passed as either copies or
 * references (i.e. using either use() or Bind()).
 */
template <typename T>
class CopyBinding : public BindingBase {
 public:
  typedef T ValType;
  typedef SharedPtr<ValType> ValPtr;
  typedef CopyBinding<ValType> Type;
  typedef SharedPtr<Type> Ptr;

  /**
   * Creates the Binding using the passed reference as bound value.
   * If copy is true, a copy of the value referred to is created.
   */
  explicit CopyBinding(T& val, const String& name = "",
                       Direction direction = PD_IN)
      : BindingBase(name, direction), val_ptr_(new T(val)), bound_(false) {}

  /**
   * Destroys the CopyBinding.
   */
  ~CopyBinding() {}

  size_t HandledColumnsCount() const { return TypeHandler<T>::size(); }

  size_t HandledRowsCount() const { return 1; }

  bool CanBind() const { return !bound_; }

  void Bind(size_t pos) {
    fun_check_dbg(!GetBinder().IsNull());
    TypeHandler<T>::Bind(pos, *val_ptr_, GetBinder(), GetDirection());
    bound_ = true;
  }

  void Reset() {
    bound_ = false;
    BinderBase::Ptr binder = GetBinder();
    fun_check_dbg(!binder.IsNull());
    binder->Reset();
  }

 private:
  // typedef typename TypeWrapper<T>::TYPE ValueType;
  ValPtr val_ptr_;
  bool bound_;
};

/**
 * Binding const char* specialization wraps char pointer into string.
 */
template <>
class Binding<const char*> : public BindingBase {
 public:
  typedef const char* ValType;
  typedef SharedPtr<ValType> ValPtr;
  typedef Binding<const char*> Type;
  typedef SharedPtr<Type> Ptr;

  /**
   * Creates the Binding by copying the passed string.
   */
  explicit Binding(const char* pVal, const String& name = "",
                   Direction direction = PD_IN)
      : BindingBase(name, direction),
        val_(pVal ? pVal : throw NullPointerException()),
        bound_(false) {}

  /**
   * Destroys the Binding.
   */
  ~Binding() {}

  size_t HandledColumnsCount() const { return 1u; }

  size_t HandledRowsCount() const { return 1u; }

  bool CanBind() const { return !bound_; }

  void Bind(size_t pos) {
    fun_check_dbg(!GetBinder().IsNull());
    TypeHandler<String>::Bind(pos, val_, GetBinder(), GetDirection());
    bound_ = true;
  }

  void Reset() {
    bound_ = false;
    BinderBase::Ptr binder = GetBinder();
    fun_check_dbg(!binder.IsNull());
    binder->Reset();
  }

 private:
  String val_;
  bool bound_;
};

/**
 * Binding const char* specialization wraps char pointer into string.
 */
template <>
class CopyBinding<const char*> : public BindingBase {
 public:
  typedef const char* ValType;
  typedef SharedPtr<ValType> ValPtr;
  typedef CopyBinding<const char*> Type;
  typedef SharedPtr<Type> Ptr;

  /**
   * Creates the Binding by copying the passed string.
   */
  explicit CopyBinding(const char* pVal, const String& name = "",
                       Direction direction = PD_IN)
      : BindingBase(name, direction),
        val_(pVal ? pVal : throw NullPointerException()),
        bound_(false) {}

  /**
   * Destroys the CopyBinding.
   */
  ~CopyBinding() {}

  size_t HandledColumnsCount() const { return 1u; }

  size_t HandledRowsCount() const { return 1u; }

  bool CanBind() const { return !bound_; }

  void Bind(size_t pos) {
    fun_check_dbg(!GetBinder().IsNull());
    TypeHandler<String>::Bind(pos, val_, GetBinder(), GetDirection());
    bound_ = true;
  }

  void Reset() {
    bound_ = false;
    BinderBase::Ptr binder = GetBinder();
    fun_check_dbg(!binder.IsNull());
    binder->Reset();
  }

 private:
  String val_;
  bool bound_;
};

/**
 * Specialization for std::vector.
 */
template <typename T>
class Binding<std::vector<T> > : public BindingBase {
 public:
  typedef std::vector<T> ValType;
  typedef SharedPtr<ValType> ValPtr;
  typedef SharedPtr<Binding<ValType> > Ptr;
  typedef typename ValType::const_iterator Iterator;

  /**
   * Creates the Binding.
   */
  explicit Binding(std::vector<T>& val, const String& name = "",
                   Direction direction = PD_IN)
      : BindingBase(name, direction), val_(val), begin_(), end_() {
    if (PD_IN == direction && HandledRowsCount() == 0) {
      throw BindingException(
          "It is illegal to Bind to an empty data collection");
    }

    Reset();
  }

  /**
   * Destroys the Binding.
   */
  ~Binding() {}

  size_t HandledColumnsCount() const { return TypeHandler<T>::size(); }

  size_t HandledRowsCount() const { return static_cast<size_t>(val_.size()); }

  bool CanBind() const { return begin_ != end_; }

  void Bind(size_t pos) {
    fun_check_dbg(!GetBinder().IsNull());
    fun_check_dbg(CanBind());

    TypeHandler<T>::Bind(pos, *begin_, GetBinder(), GetDirection());
    ++begin_;
  }

  void Reset() {
    begin_ = val_.begin();
    end_ = val_.end();
  }

 private:
  const ValType& val_;
  Iterator begin_;
  Iterator end_;
};

/**
 * Specialization for std::vector.
 */
template <typename T>
class CopyBinding<std::vector<T> > : public BindingBase {
 public:
  typedef std::vector<T> ValType;
  typedef SharedPtr<ValType> ValPtr;
  typedef SharedPtr<CopyBinding<ValType> > Ptr;
  typedef typename ValType::const_iterator Iterator;

  /**
   * Creates the Binding.
   */
  explicit CopyBinding(std::vector<T>& val, const String& name = "",
                       Direction direction = PD_IN)
      : BindingBase(name, direction),
        val_ptr_(new std::vector<T>(val)),
        begin_(),
        end_() {
    if (PD_IN == direction && HandledRowsCount() == 0) {
      throw BindingException(
          "It is illegal to Bind to an empty data collection");
    }

    Reset();
  }

  /**
   * Destroys the CopyBinding.
   */
  ~CopyBinding() {}

  size_t HandledColumnsCount() const { return TypeHandler<T>::size(); }

  size_t HandledRowsCount() const { return val_ptr_->size(); }

  bool CanBind() const { return begin_ != end_; }

  void Bind(size_t pos) {
    fun_check_dbg(!GetBinder().IsNull());
    fun_check_dbg(CanBind());

    TypeHandler<T>::Bind(pos, *begin_, GetBinder(), GetDirection());
    ++begin_;
  }

  void Reset() {
    begin_ = val_ptr_->begin();
    end_ = val_ptr_->end();
  }

 private:
  ValPtr val_ptr_;
  Iterator begin_;
  Iterator end_;
};

/**
 * Specialization for std::vector<bool>.
 * This specialization is necessary due to the nature of std::vector<bool>.
 * For details, see the standard library implementation of std::vector<bool>
 * or
 * S. Meyers: "Effective STL" (Copyright Addison-Wesley 2001),
 * Item 18: "Avoid using vector<bool>."
 *
 * The workaround employed here is using std::deque<bool> as an
 * internal replacement container.
 *
 * IMPORTANT:
 * Only IN binding is supported.
 */
template <>
class Binding<std::vector<bool> > : public BindingBase {
 public:
  typedef std::vector<bool> ValType;
  typedef SharedPtr<ValType> ValPtr;
  typedef SharedPtr<Binding<ValType> > Ptr;
  typedef ValType::const_iterator Iterator;

  /**
   * Creates the Binding.
   */
  explicit Binding(const std::vector<bool>& val, const String& name = "",
                   Direction direction = PD_IN)
      : BindingBase(name, direction),
        val_(val),
        _deq(val_.begin(), val_.end()),
        begin_(),
        end_() {
    if (PD_IN != direction) {
      throw BindingException(
          "Only IN direction is legal for std:vector<bool> binding.");
    }

    if (HandledRowsCount() == 0) {
      throw BindingException(
          "It is illegal to Bind to an empty data collection");
    }

    Reset();
  }

  /**
   * Destroys the Binding.
   */
  ~Binding() {}

  size_t HandledColumnsCount() const { return 1u; }

  size_t HandledRowsCount() const { return static_cast<size_t>(val_.size()); }

  bool CanBind() const { return begin_ != end_; }

  void Bind(size_t pos) {
    fun_check_dbg(!GetBinder().IsNull());
    fun_check_dbg(CanBind());
    TypeHandler<bool>::Bind(pos, *begin_, GetBinder(), GetDirection());
    ++begin_;
  }

  void Reset() {
    begin_ = _deq.begin();
    end_ = _deq.end();
  }

 private:
  const std::vector<bool>& val_;
  std::deque<bool> _deq;
  std::deque<bool>::const_iterator begin_;
  std::deque<bool>::const_iterator end_;
};

/**
 * Specialization for std::vector<bool>.
 * This specialization is necessary due to the nature of std::vector<bool>.
 * For details, see the standard library implementation of std::vector<bool>
 * or
 * S. Meyers: "Effective STL" (Copyright Addison-Wesley 2001),
 * Item 18: "Avoid using vector<bool>."
 *
 * The workaround employed here is using std::deque<bool> as an
 * internal replacement container.
 *
 * IMPORTANT:
 * Only IN binding is supported.
 */
template <>
class CopyBinding<std::vector<bool> > : public BindingBase {
 public:
  typedef std::vector<bool> ValType;
  typedef SharedPtr<ValType> ValPtr;
  typedef SharedPtr<CopyBinding<ValType> > Ptr;
  typedef ValType::const_iterator Iterator;

  /**
   * Creates the Binding.
   */
  explicit CopyBinding(const std::vector<bool>& val, const String& name = "",
                       Direction direction = PD_IN)
      : BindingBase(name, direction),
        _deq(val.begin(), val.end()),
        begin_(),
        end_() {
    if (PD_IN != direction) {
      throw BindingException(
          "Only IN direction is legal for std:vector<bool> binding.");
    }

    if (HandledRowsCount() == 0) {
      throw BindingException(
          "It is illegal to Bind to an empty data collection");
    }

    Reset();
  }

  /**
   * Destroys the CopyBinding.
   */
  ~CopyBinding() {}

  size_t HandledColumnsCount() const { return 1u; }

  size_t HandledRowsCount() const { return static_cast<size_t>(_deq.size()); }

  bool CanBind() const { return begin_ != end_; }

  void Bind(size_t pos) {
    fun_check_dbg(!GetBinder().IsNull());
    fun_check_dbg(CanBind());
    TypeHandler<bool>::Bind(pos, *begin_, GetBinder(), GetDirection());
    ++begin_;
  }

  void Reset() {
    begin_ = _deq.begin();
    end_ = _deq.end();
  }

 private:
  std::deque<bool> _deq;
  std::deque<bool>::const_iterator begin_;
  std::deque<bool>::const_iterator end_;
};

/**
 * Specialization for std::list.
 */
template <typename T>
class Binding<std::list<T> > : public BindingBase {
 public:
  typedef std::list<T> ValType;
  typedef SharedPtr<ValType> ValPtr;
  typedef SharedPtr<Binding<ValType> > Ptr;
  typedef typename ValType::const_iterator Iterator;

  /**
   * Creates the Binding.
   */
  explicit Binding(std::list<T>& val, const String& name = "",
                   Direction direction = PD_IN)
      : BindingBase(name, direction), val_(val), begin_(), end_() {
    if (PD_IN == direction && HandledRowsCount() == 0) {
      throw BindingException(
          "It is illegal to Bind to an empty data collection");
    }

    Reset();
  }

  /**
   * Destroys the Binding.
   */
  ~Binding() {}

  size_t HandledColumnsCount() const { return TypeHandler<T>::size(); }

  size_t HandledRowsCount() const { return val_.size(); }

  bool CanBind() const { return begin_ != end_; }

  void Bind(size_t pos) {
    fun_check_dbg(!GetBinder().IsNull());
    fun_check_dbg(CanBind());
    TypeHandler<T>::Bind(pos, *begin_, GetBinder(), GetDirection());
    ++begin_;
  }

  void Reset() {
    begin_ = val_.begin();
    end_ = val_.end();
  }

 private:
  const ValType& val_;
  Iterator begin_;
  Iterator end_;
};

/**
 * Specialization for std::list.
 */
template <typename T>
class CopyBinding<std::list<T> > : public BindingBase {
 public:
  typedef typename std::list<T> ValType;
  typedef SharedPtr<ValType> ValPtr;
  typedef SharedPtr<CopyBinding<ValType> > Ptr;
  typedef typename ValType::const_iterator Iterator;

  /**
   * Creates the Binding.
   */
  explicit CopyBinding(ValType& val, const String& name = "",
                       Direction direction = PD_IN)
      : BindingBase(name, direction),
        val_ptr_(new std::list<T>(val)),
        begin_(),
        end_() {
    if (PD_IN == direction && HandledRowsCount() == 0) {
      throw BindingException(
          "It is illegal to Bind to an empty data collection");
    }

    Reset();
  }

  /**
   * Destroys the CopyBinding.
   */
  ~CopyBinding() {}

  size_t HandledColumnsCount() const { return TypeHandler<T>::size(); }

  size_t HandledRowsCount() const { return val_ptr_->size(); }

  bool CanBind() const { return begin_ != end_; }

  void Bind(size_t pos) {
    fun_check_dbg(!GetBinder().IsNull());
    fun_check_dbg(CanBind());
    TypeHandler<T>::Bind(pos, *begin_, GetBinder(), GetDirection());
    ++begin_;
  }

  void Reset() {
    begin_ = val_ptr_->begin();
    end_ = val_ptr_->end();
  }

 private:
  ValPtr val_ptr_;
  Iterator begin_;
  Iterator end_;
};

/**
 * Specialization for std::deque.
 */
template <typename T>
class Binding<std::deque<T> > : public BindingBase {
 public:
  typedef std::deque<T> ValType;
  typedef SharedPtr<ValType> ValPtr;
  typedef SharedPtr<Binding<ValType> > Ptr;
  typedef typename ValType::const_iterator Iterator;

  /**
   * Creates the Binding.
   */
  explicit Binding(std::deque<T>& val, const String& name = "",
                   Direction direction = PD_IN)
      : BindingBase(name, direction), val_(val), begin_(), end_() {
    if (PD_IN == direction && HandledRowsCount() == 0) {
      throw BindingException(
          "It is illegal to Bind to an empty data collection");
    }

    Reset();
  }

  /**
   * Destroys the Binding.
   */
  ~Binding() {}

  size_t HandledColumnsCount() const { return TypeHandler<T>::size(); }

  size_t HandledRowsCount() const { return val_.size(); }

  bool CanBind() const { return begin_ != end_; }

  void Bind(size_t pos) {
    fun_check_dbg(!GetBinder().IsNull());
    fun_check_dbg(CanBind());
    TypeHandler<T>::Bind(pos, *begin_, GetBinder(), GetDirection());
    ++begin_;
  }

  void Reset() {
    begin_ = val_.begin();
    end_ = val_.end();
  }

 private:
  const ValType& val_;
  Iterator begin_;
  Iterator end_;
};

/**
 * Specialization for std::deque.
 */
template <typename T>
class CopyBinding<std::deque<T> > : public BindingBase {
 public:
  typedef std::deque<T> ValType;
  typedef SharedPtr<ValType> ValPtr;
  typedef SharedPtr<CopyBinding<ValType> > Ptr;
  typedef typename ValType::const_iterator Iterator;

  /**
   * Creates the Binding.
   */
  explicit CopyBinding(std::deque<T>& val, const String& name = "",
                       Direction direction = PD_IN)
      : BindingBase(name, direction),
        val_ptr_(new std::deque<T>(val)),
        begin_(),
        end_() {
    if (PD_IN == direction && HandledRowsCount() == 0) {
      throw BindingException(
          "It is illegal to Bind to an empty data collection");
    }

    Reset();
  }

  /**
   * Destroys the CopyBinding.
   */
  ~CopyBinding() {}

  size_t HandledColumnsCount() const { return TypeHandler<T>::size(); }

  size_t HandledRowsCount() const { return val_ptr_->size(); }

  bool CanBind() const { return begin_ != end_; }

  void Bind(size_t pos) {
    fun_check_dbg(!GetBinder().IsNull());
    fun_check_dbg(CanBind());
    TypeHandler<T>::Bind(pos, *begin_, GetBinder(), GetDirection());
    ++begin_;
  }

  void Reset() {
    begin_ = val_ptr_->begin();
    end_ = val_ptr_->end();
  }

 private:
  ValPtr val_ptr_;
  Iterator begin_;
  Iterator end_;
};

/**
 * Specialization for std::set.
 */
template <typename T>
class Binding<std::set<T> > : public BindingBase {
 public:
  typedef std::set<T> ValType;
  typedef SharedPtr<ValType> ValPtr;
  typedef SharedPtr<Binding<ValType> > Ptr;
  typedef typename ValType::const_iterator Iterator;

  /**
   * Creates the Binding.
   */
  explicit Binding(std::set<T>& val, const String& name = "",
                   Direction direction = PD_IN)
      : BindingBase(name, direction), val_(val), begin_(), end_() {
    if (PD_IN == direction && HandledRowsCount() == 0) {
      throw BindingException(
          "It is illegal to Bind to an empty data collection");
    }

    Reset();
  }

  /**
   * Destroys the Binding.
   */
  ~Binding() {}

  size_t HandledColumnsCount() const { return TypeHandler<T>::size(); }

  size_t HandledRowsCount() const { return static_cast<size_t>(val_.size()); }

  bool CanBind() const { return begin_ != end_; }

  void Bind(size_t pos) {
    fun_check_dbg(!GetBinder().IsNull());
    fun_check_dbg(CanBind());
    TypeHandler<T>::Bind(pos, *begin_, GetBinder(), GetDirection());
    ++begin_;
  }

  void Reset() {
    begin_ = val_.begin();
    end_ = val_.end();
  }

 private:
  const ValType& val_;
  Iterator begin_;
  Iterator end_;
};

/**
 * Specialization for std::set.
 */
template <typename T>
class CopyBinding<std::set<T> > : public BindingBase {
 public:
  typedef std::set<T> ValType;
  typedef SharedPtr<ValType> ValPtr;
  typedef SharedPtr<CopyBinding<ValType> > Ptr;
  typedef typename ValType::const_iterator Iterator;

  /**
   * Creates the Binding.
   */
  explicit CopyBinding(std::set<T>& val, const String& name = "",
                       Direction direction = PD_IN)
      : BindingBase(name, direction),
        val_ptr_(new std::set<T>(val)),
        begin_(),
        end_() {
    if (PD_IN == direction && HandledRowsCount() == 0) {
      throw BindingException(
          "It is illegal to Bind to an empty data collection");
    }

    Reset();
  }

  /**
   * Destroys the CopyBinding.
   */
  ~CopyBinding() {}

  size_t HandledColumnsCount() const { return TypeHandler<T>::size(); }

  size_t HandledRowsCount() const { return val_ptr_->size(); }

  bool CanBind() const { return begin_ != end_; }

  void Bind(size_t pos) {
    fun_check_dbg(!GetBinder().IsNull());
    fun_check_dbg(CanBind());
    TypeHandler<T>::Bind(pos, *begin_, GetBinder(), GetDirection());
    ++begin_;
  }

  void Reset() {
    begin_ = val_ptr_->begin();
    end_ = val_ptr_->end();
  }

 private:
  ValPtr val_ptr_;
  Iterator begin_;
  Iterator end_;
};

/**
 * Specialization for std::multiset.
 */
template <typename T>
class Binding<std::multiset<T> > : public BindingBase {
 public:
  typedef std::multiset<T> ValType;
  typedef SharedPtr<ValType> ValPtr;
  typedef SharedPtr<Binding<ValType> > Ptr;
  typedef typename ValType::const_iterator Iterator;

  /**
   * Creates the Binding.
   */
  explicit Binding(std::multiset<T>& val, const String& name = "",
                   Direction direction = PD_IN)
      : BindingBase(name, direction), val_(val), begin_(), end_() {
    if (PD_IN == direction && HandledRowsCount() == 0) {
      throw BindingException(
          "It is illegal to Bind to an empty data collection");
    }

    Reset();
  }

  /**
   * Destroys the Binding.
   */
  ~Binding() {}

  size_t HandledColumnsCount() const { return TypeHandler<T>::size(); }

  size_t HandledRowsCount() const { return static_cast<size_t>(val_.size()); }

  bool CanBind() const { return begin_ != end_; }

  void Bind(size_t pos) {
    fun_check_dbg(!GetBinder().IsNull());
    fun_check_dbg(CanBind());
    TypeHandler<T>::Bind(pos, *begin_, GetBinder(), GetDirection());
    ++begin_;
  }

  void Reset() {
    begin_ = val_.begin();
    end_ = val_.end();
  }

 private:
  const ValType& val_;
  Iterator begin_;
  Iterator end_;
};

/**
 * Specialization for std::multiset.
 */
template <typename T>
class CopyBinding<std::multiset<T> > : public BindingBase {
 public:
  typedef std::multiset<T> ValType;
  typedef SharedPtr<ValType> ValPtr;
  typedef SharedPtr<CopyBinding<ValType> > Ptr;
  typedef typename ValType::const_iterator Iterator;

  /**
   * Creates the Binding.
   */
  explicit CopyBinding(std::multiset<T>& val, const String& name = "",
                       Direction direction = PD_IN)
      : BindingBase(name, direction),
        val_ptr_(new std::multiset<T>(val)),
        begin_(),
        end_() {
    if (PD_IN == direction && HandledRowsCount() == 0) {
      throw BindingException(
          "It is illegal to Bind to an empty data collection");
    }

    Reset();
  }

  /**
   * Destroys the CopyBinding.
   */
  ~CopyBinding() {}

  size_t HandledColumnsCount() const { return TypeHandler<T>::size(); }

  size_t HandledRowsCount() const { return val_ptr_->size(); }

  bool CanBind() const { return begin_ != end_; }

  void Bind(size_t pos) {
    fun_check_dbg(!GetBinder().IsNull());
    fun_check_dbg(CanBind());
    TypeHandler<T>::Bind(pos, *begin_, GetBinder(), GetDirection());
    ++begin_;
  }

  void Reset() {
    begin_ = val_ptr_->begin();
    end_ = val_ptr_->end();
  }

 private:
  ValPtr val_ptr_;
  Iterator begin_;
  Iterator end_;
};

/**
 * Specialization for std::map.
 */
template <class K, class V>
class Binding<std::map<K, V> > : public BindingBase {
 public:
  typedef std::map<K, V> ValType;
  typedef SharedPtr<ValType> ValPtr;
  typedef SharedPtr<Binding<ValType> > Ptr;
  typedef typename ValType::const_iterator Iterator;

  /**
   * Creates the Binding.
   */
  explicit Binding(std::map<K, V>& val, const String& name = "",
                   Direction direction = PD_IN)
      : BindingBase(name, direction), val_(val), begin_(), end_() {
    if (PD_IN == direction && HandledRowsCount() == 0) {
      throw BindingException(
          "It is illegal to Bind to an empty data collection");
    }

    Reset();
  }

  /**
   * Destroys the Binding.
   */
  ~Binding() {}

  size_t HandledColumnsCount() const { return TypeHandler<V>::size(); }

  size_t HandledRowsCount() const { return static_cast<size_t>(val_.size()); }

  bool CanBind() const { return begin_ != end_; }

  void Bind(size_t pos) {
    fun_check_dbg(!GetBinder().IsNull());
    fun_check_dbg(CanBind());
    TypeHandler<V>::Bind(pos, begin_->second, GetBinder(), GetDirection());
    ++begin_;
  }

  void Reset() {
    begin_ = val_.begin();
    end_ = val_.end();
  }

 private:
  const ValType& val_;
  Iterator begin_;
  Iterator end_;
};

/**
 * Specialization for std::map.
 */
template <class K, class V>
class CopyBinding<std::map<K, V> > : public BindingBase {
 public:
  typedef std::map<K, V> ValType;
  typedef SharedPtr<ValType> ValPtr;
  typedef SharedPtr<CopyBinding<ValType> > Ptr;
  typedef typename ValType::const_iterator Iterator;

  /**
   * Creates the Binding.
   */
  explicit CopyBinding(std::map<K, V>& val, const String& name = "",
                       Direction direction = PD_IN)
      : BindingBase(name, direction),
        val_ptr_(new std::map<K, V>(val)),
        begin_(),
        end_() {
    if (PD_IN == direction && HandledRowsCount() == 0) {
      throw BindingException(
          "It is illegal to Bind to an empty data collection");
    }

    Reset();
  }

  /**
   * Destroys the CopyBinding.
   */
  ~CopyBinding() {}

  size_t HandledColumnsCount() const { return TypeHandler<V>::size(); }

  size_t HandledRowsCount() const { return val_ptr_->size(); }

  bool CanBind() const { return begin_ != end_; }

  void Bind(size_t pos) {
    fun_check_dbg(!GetBinder().IsNull());
    fun_check_dbg(CanBind());
    TypeHandler<V>::Bind(pos, begin_->second, GetBinder(), GetDirection());
    ++begin_;
  }

  void Reset() {
    begin_ = val_ptr_->begin();
    end_ = val_ptr_->end();
  }

 private:
  ValPtr val_ptr_;
  Iterator begin_;
  Iterator end_;
};

/**
 * Specialization for std::multimap.
 */
template <class K, class V>
class Binding<std::multimap<K, V> > : public BindingBase {
 public:
  typedef std::multimap<K, V> ValType;
  typedef SharedPtr<ValType> ValPtr;
  typedef SharedPtr<Binding<ValType> > Ptr;
  typedef typename ValType::const_iterator Iterator;

  /**
   * Creates the Binding.
   */
  explicit Binding(std::multimap<K, V>& val, const String& name = "",
                   Direction direction = PD_IN)
      : BindingBase(name, direction), val_(val), begin_(), end_() {
    if (PD_IN == direction && HandledRowsCount() == 0) {
      throw BindingException(
          "It is illegal to Bind to an empty data collection");
    }

    Reset();
  }

  /**
   * Destroys the Binding.
   */
  ~Binding() {}

  size_t HandledColumnsCount() const { return TypeHandler<V>::size(); }

  size_t HandledRowsCount() const { return static_cast<size_t>(val_.size()); }

  bool CanBind() const { return begin_ != end_; }

  void Bind(size_t pos) {
    fun_check_dbg(!GetBinder().IsNull());
    fun_check_dbg(CanBind());
    TypeHandler<V>::Bind(pos, begin_->second, GetBinder(), GetDirection());
    ++begin_;
  }

  void Reset() {
    begin_ = val_.begin();
    end_ = val_.end();
  }

 private:
  const ValType& val_;
  Iterator begin_;
  Iterator end_;
};

/**
 * Specialization for std::multimap.
 */
template <class K, class V>
class CopyBinding<std::multimap<K, V> > : public BindingBase {
 public:
  typedef std::multimap<K, V> ValType;
  typedef SharedPtr<ValType> ValPtr;
  typedef SharedPtr<CopyBinding<ValType> > Ptr;
  typedef typename ValType::const_iterator Iterator;

  /**
   * Creates the Binding.
   */
  explicit CopyBinding(std::multimap<K, V>& val, const String& name = "",
                       Direction direction = PD_IN)
      : BindingBase(name, direction),
        val_ptr_(new std::multimap<K, V>(val)),
        begin_(),
        end_() {
    if (PD_IN == direction && HandledRowsCount() == 0) {
      throw BindingException(
          "It is illegal to Bind to an empty data collection");
    }

    Reset();
  }

  /**
   * Destroys the CopyBinding.
   */
  ~CopyBinding() {}

  size_t HandledColumnsCount() const { return TypeHandler<V>::size(); }

  size_t HandledRowsCount() const { return val_ptr_->size(); }

  bool CanBind() const { return begin_ != end_; }

  void Bind(size_t pos) {
    fun_check_dbg(!GetBinder().IsNull());
    fun_check_dbg(CanBind());
    TypeHandler<V>::Bind(pos, begin_->second, GetBinder(), GetDirection());
    ++begin_;
  }

  void Reset() {
    begin_ = val_ptr_->begin();
    end_ = val_ptr_->end();
  }

 private:
  ValPtr val_ptr_;
  Iterator begin_;
  Iterator end_;
};

namespace Keywords {

/**
 * Convenience function for a more compact Binding creation.
 */
template <typename T>
inline BindingBase::Ptr use(T& t, const String& name = "") {
  // If this fails to compile, a const ref was passed to use().
  // This can be resolved by either (a) using Bind (which will copy the value),
  // or (b) if the const ref is guaranteed to exist when execute is called
  // (which can be much later!), by using the "useRef" keyword instead
  fun_static_assert(!IsConst<T>::Value);
  return new Binding<T>(t, name, BindingBase::PD_IN);
}

/**
 * Convenience function for a more compact Binding creation.
 */
template <>
inline BindingBase::Ptr use(long& t, const String& name) {
#ifndef FUN_LONG_IS_64_BIT
  return new Binding<long>(t, name, BindingBase::PD_IN);
#else
  return new Binding<int64>(reinterpret_cast<int64&>(t), name,
                            BindingBase::PD_IN);
#endif
}

/**
 * Convenience function for a more compact Binding creation.
 */
template <>
inline BindingBase::Ptr use(unsigned long& t, const String& name) {
#ifndef FUN_LONG_IS_64_BIT
  return new Binding<unsigned long>(t, name, BindingBase::PD_IN);
#else
  return new Binding<uint64>(reinterpret_cast<uint64&>(t), name,
                             BindingBase::PD_IN);
#endif
}

/**
 * NullData overload.
 */
inline BindingBase::Ptr use(const NullData& t, const String& name = "") {
  return new Binding<NullData>(const_cast<NullData&>(t), name,
                               BindingBase::PD_IN);
}

/**
 * NullData overload.
 */
inline BindingBase::Ptr use(const NullValue& /*t*/, const String& name = "") {
  return use(NullValue::NullCode<void>(), name);
}

/**
 * Convenience function for a more compact Binding creation.
 */
template <typename T>
inline BindingBase::Ptr useRef(T& t, const String& name = "") {
  return new Binding<T>(t, name, BindingBase::PD_IN);
}

/**
 * Convenience function for a more compact Binding creation.
 */
template <typename T>
inline BindingBase::Ptr in(T& t, const String& name = "") {
  return use(t, name);
}

/**
 * NullData overload.
 */
inline BindingBase::Ptr in(const NullData& t, const String& name = "") {
  return use(t, name);
}

/**
 * Convenience function for a more compact Binding creation.
 */
template <typename T>
inline BindingBase::Ptr out(T& t) {
  fun_static_assert(!IsConst<T>::Value);
  return new Binding<T>(t, "", BindingBase::PD_OUT);
}

/**
 * Convenience function for a more compact Binding creation.
 */
template <typename T>
inline BindingBase::Ptr io(T& t) {
  fun_static_assert(!IsConst<T>::Value);
  return new Binding<T>(t, "", BindingBase::PD_IN_OUT);
}

/**
 * Convenience dummy function (for syntax purposes only).
 */
inline BindingBaseVec& use(BindingBaseVec& bv) { return bv; }

/**
 * Convenience dummy function (for syntax purposes only).
 */
inline BindingBaseVec& in(BindingBaseVec& bv) { return bv; }

/**
 * Convenience dummy function (for syntax purposes only).
 */
inline BindingBaseVec& out(BindingBaseVec& bv) { return bv; }

/**
 * Convenience dummy function (for syntax purposes only).
 */
inline BindingBaseVec& io(BindingBaseVec& bv) { return bv; }

/**
 * Convenience function for a more compact Binding creation.
 * This function differs from use() in its value copy semantics.
 */
template <typename T>
inline BindingBase::Ptr Bind(T t, const String& name) {
  return new CopyBinding<T>(t, name, BindingBase::PD_IN);
}

/**
 * Convenience function for a more compact Binding creation.
 * This function differs from use() in its value copy semantics.
 */
template <typename T>
inline BindingBase::Ptr Bind(T t) {
  return fun::sql::Keywords::Bind(t, "");
}

}  // namespace Keywords

}  // namespace sql
}  // namespace fun
