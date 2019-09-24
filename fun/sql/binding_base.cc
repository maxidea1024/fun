#include "fun/sql/binding_base.h"

namespace fun {
namespace sql {

BindingBase::BindingBase( const String& name,
                          Direction direction,
                          uint32 bulk_size)
  : binder_(nullptr),
    name_(name),
    direction_(direction),
    bulk_size_(bulk_size) {}

BindingBase::~BindingBase() {}

void BindingBase::SetBinder(BinderPtr binder) {
  fun_check_ptr(binder);
  binder_ = binder;
}

} // namespace sql
} // namespace fun
