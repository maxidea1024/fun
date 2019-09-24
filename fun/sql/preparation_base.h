#pragma once

#include "fun/base/shared_ptr.h"
#include "fun/sql/preparator_base.h"
#include "fun/sql/sql.h"

#include <cstddef>

namespace fun {
namespace sql {

/**
 * Interface for calling the appropriate PreparatorBase method
 */
class FUN_SQL_API PreparationBase {
 public:
  typedef SharedPtr<PreparationBase> Ptr;
  typedef PreparatorBase::Ptr PreparatorPtr;

  /**
   * Creates the PreparationBase.
   */
  PreparationBase(PreparatorPtr preparator);

  /**
   * Destroys the PreparationBase.
   */
  virtual ~PreparationBase();

  /**
   * Prepares data.
   */
  virtual void Prepare() = 0;

 protected:
  PreparationBase();
  PreparationBase(const PreparationBase&);
  PreparationBase& operator=(const PreparationBase&);

  /**
   * Returns the preparation object
   */
  PreparatorPtr GetPreparation();

  PreparatorPtr preparator_;
};

//
// inlines
//

inline PreparationBase::PreparatorPtr PreparationBase::GetPreparation() {
  return preparator_;
}

}  // namespace sql
}  // namespace fun
