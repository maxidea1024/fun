#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * The purpose of the EventArgs class is to be used as parameter
 * when one doesn't want to send any data.
 *
 * One can use EventArgs as a base class for one's own event arguments
 * but with the arguments being a template parameter this is not
 * necessary.
 */
class FUN_BASE_API EventArgs {
 public:
  EventArgs();
  virtual ~EventArgs();
};

}  // namespace fun
