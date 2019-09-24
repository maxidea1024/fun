#pragma once

#include "fun/sql/binding_base.h"
#include "fun/sql/bulk.h"
#include "fun/sql/sql.h"
#include "fun/sql/sql_exception.h"
#include "fun/sql/type_handler.h"

#include <cstddef>
#include <deque>
#include <list>
#include <vector>

namespace fun {
namespace sql {

/**
 * A BulkBinding maps a value to a column.
 * Bulk binding support is provided only for std::vector.
 */
template <typename T>
class BulkBinding : public BindingBase {
 public:
  /**
   * Creates the BulkBinding.
   */
  BulkBinding(const T& val, uint32 bulk_size, const String& name = "",
              Direction direction = PD_IN)
      : BindingBase(name, direction, bulk_size), val_(val), bound_(false) {
    if (0 == val_.size()) {
      throw BindingException("Zero size containers not allowed.");
    }
  }

  /**
   * Destroys the BulkBinding.
   */
  ~BulkBinding() {}

  size_t HandledColumnsCount() const { return 1; }

  size_t HandledRowsCount() const { return val_.size(); }

  bool CanBind() const { return !bound_; }

  void Bind(size_t pos) {
    fun_check_dbg(!GetBinder().IsNull());
    TypeHandler<T>::Bind(pos, val_, GetBinder(), GetDirection());
    bound_ = true;
  }

  void Reset() {
    bound_ = false;
    GetBinder()->Reset();
  }

 private:
  const T& val_;
  bool bound_;
};

namespace Keywords {

/**
 * Convenience function for a more compact BulkBinding creation for std::vector.
 */
template <typename T>
BindingBase::Ptr use(const std::vector<T>& t, BulkFnType,
                     const String& name = "") {
  return new BulkBinding<std::vector<T> >(t, static_cast<uint32>(t.size()),
                                          name);
}

/**
 * Convenience function for a more compact BulkBinding creation for std::vector.
 */
template <typename T>
BindingBase::Ptr in(const std::vector<T>& t, BulkFnType,
                    const String& name = "") {
  return new BulkBinding<std::vector<T> >(t, static_cast<uint32>(t.size()),
                                          name);
}

/**
 * Convenience function for a more compact BulkBinding creation for std::deque.
 */
template <typename T>
BindingBase::Ptr use(const std::deque<T>& t, BulkFnType,
                     const String& name = "") {
  return new BulkBinding<std::deque<T> >(t, static_cast<uint32>(t.size()),
                                         name);
}

/**
 * Convenience function for a more compact BulkBinding creation for std::deque.
 */
template <typename T>
BindingBase::Ptr in(const std::deque<T>& t, BulkFnType,
                    const String& name = "") {
  return new BulkBinding<std::deque<T> >(t, static_cast<uint32>(t.size()),
                                         name);
}

/**
 * Convenience function for a more compact BulkBinding creation for std::list.
 */
template <typename T>
BindingBase::Ptr use(const std::list<T>& t, BulkFnType,
                     const String& name = "") {
  return new BulkBinding<std::list<T> >(t, static_cast<uint32>(t.size()), name);
}

/**
 * Convenience function for a more compact BulkBinding creation for std::list.
 */
template <typename T>
BindingBase::Ptr in(const std::list<T>& t, BulkFnType,
                    const String& name = "") {
  return new BulkBinding<std::list<T> >(t, static_cast<uint32>(t.size()), name);
}

/**
 * Convenience function for a more compact BulkBinding creation for std::vector.
 */
template <typename T>
BindingBase::Ptr use(const std::vector<Nullable<T> >& t, BulkFnType,
                     const String& name = "") {
  return new BulkBinding<std::vector<Nullable<T> > >(
      t, static_cast<uint32>(t.size()), name);
}

/**
 * Convenience function for a more compact BulkBinding creation for std::vector.
 */
template <typename T>
BindingBase::Ptr in(const std::vector<Nullable<T> >& t, BulkFnType,
                    const String& name = "") {
  return new BulkBinding<std::vector<Nullable<T> > >(
      t, static_cast<uint32>(t.size()), name);
}

/**
 * Convenience function for a more compact BulkBinding creation for std::deque.
 */
template <typename T>
BindingBase::Ptr use(const std::deque<Nullable<T> >& t, BulkFnType,
                     const String& name = "") {
  return new BulkBinding<std::deque<Nullable<T> > >(
      t, static_cast<uint32>(t.size()), name);
}

/**
 * Convenience function for a more compact BulkBinding creation for std::deque.
 */
template <typename T>
BindingBase::Ptr in(const std::deque<Nullable<T> >& t, BulkFnType,
                    const String& name = "") {
  return new BulkBinding<std::deque<Nullable<T> > >(
      t, static_cast<uint32>(t.size()), name);
}

/**
 * Convenience function for a more compact BulkBinding creation for std::list.
 */
template <typename T>
BindingBase::Ptr use(const std::list<Nullable<T> >& t, BulkFnType,
                     const String& name = "") {
  return new BulkBinding<std::list<Nullable<T> > >(
      t, static_cast<uint32>(t.size()), name);
}

/**
 * Convenience function for a more compact BulkBinding creation for std::list.
 */
template <typename T>
BindingBase::Ptr in(const std::list<Nullable<T> >& t, BulkFnType,
                    const String& name = "") {
  return new BulkBinding<std::list<Nullable<T> > >(
      t, static_cast<uint32>(t.size()), name);
}

}  // namespace Keywords

}  // namespace sql
}  // namespace fun
