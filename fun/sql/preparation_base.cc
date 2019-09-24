#include "fun/sql/preparation_base.h"

namespace fun {
namespace sql {

PreparationBase::PreparationBase(PreparatorPtr preparator)
    : preparator_(preparator) {
  fun_check_ptr(preparator_);
}

PreparationBase::~PreparationBase() {}

}  // namespace sql
}  // namespace fun
