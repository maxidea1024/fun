#include "fun/base/thread_target.h"

namespace fun {

ThreadTarget::ThreadTarget(Callback method)
  : method_(method) {}

ThreadTarget::ThreadTarget(const ThreadTarget& rhs)
  : method_(rhs.method_) {}

ThreadTarget& ThreadTarget::operator = (const ThreadTarget& rhs) {
  method_ = rhs.method_;
  return *this;
}

ThreadTarget::~ThreadTarget() {}

} // namespace fun
