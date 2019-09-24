#pragma once

#include <cstddef>
#include <deque>
#include <list>
#include <vector>
#include "fun/base/any.h"
#include "fun/ref_counted_object.h"
#include "fun/ref_counted_ptr.h"
#include "fun/sql/binder_base.h"
#include "fun/sql/sql.h"

namespace fun {
namespace sql {

/**
 * BindingBase connects a value with a placeholder via an BinderBase interface.
 */
class FUN_SQL_API BindingBase {
 public:
  typedef SharedPtr<BindingBase> Ptr;
  typedef BinderBase::Ptr BinderPtr;

  enum Direction {
    PD_IN = BinderBase::PD_IN,
    PD_OUT = BinderBase::PD_OUT,
    PD_IN_OUT = BinderBase::PD_IN_OUT
  };

  /**
   * Creates the BindingBase.
   */
  BindingBase(const String& name = "", Direction direction = PD_IN,
              uint32 bulk_size = 0);

  /**
   * Destroys the BindingBase.
   */
  virtual ~BindingBase();

  /**
   * Sets the object used for binding; object does NOT take ownership of the
   * pointer.
   */
  void SetBinder(BinderPtr binder);

  /**
   * Returns the BinderBase used for binding data.
   */
  BinderPtr GetBinder() const;

  /**
   * Returns the number of columns that the binding handles.
   *
   * The trivial case will be one single column but when
   * complex types are used this value can be larger than one.
   */
  virtual size_t HandledColumnsCount() const = 0;

  /**
   * Returns the number of rows that the binding handles.
   *
   * The trivial case will be one single row but
   * for collection data types it can be larger.
   */
  virtual size_t HandledRowsCount() const = 0;

  /**
   * Returns true if we have enough data to bind
   */
  virtual bool CanBind() const = 0;

  /**
   * Binds a value to the given column position
   */
  virtual void Bind(size_t pos) = 0;

  /**
   * Allows a binding to be reused.
   */
  virtual void Reset() = 0;

  /**
   * Returns the binding direction.
   */
  BinderBase::Direction GetDirection() const;

  /**
   * Returns the name for this binding.
   */
  const String& Name() const;

  /**
   * Returns true if extraction is bulk.
   */
  bool IsBulk() const;

  /**
   * Returns the size of the bulk binding.
   */
  uint32 BulkSize() const;

 private:
  BinderPtr binder_;
  String name_;
  Direction direction_;
  uint32 bulk_size_;
};

typedef std::vector<BindingBase::Ptr> BindingBaseVec;
typedef std::deque<BindingBase::Ptr> BindingBaseDeq;
typedef std::list<BindingBase::Ptr> BindingBaseLst;

//
// inlines
//

inline BinderBase::Ptr BindingBase::GetBinder() const { return binder_; }

inline const String& BindingBase::Name() const { return name_; }

inline BinderBase::Direction BindingBase::GetDirection() const {
  return (BinderBase::Direction)direction_;
}

inline bool BindingBase::IsBulk() const { return bulk_size_ > 0; }

inline uint32 BindingBase::BulkSize() const { return bulk_size_; }

}  // namespace sql
}  // namespace fun
