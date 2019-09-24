#include "fun/base/pipe.h"

namespace fun {

Pipe::Pipe() : impl_(new PipeImpl) {}

Pipe::Pipe(const Pipe& rhs) : impl_(rhs.impl_) { impl_->AddRef(); }

Pipe::~Pipe() { impl_->Release(); }

Pipe& Pipe::operator=(const Pipe& rhs) {
  if (FUN_LIKELY(&rhs != this)) {
    impl_->Release();
    impl_ = rhs.impl_;
    impl_->AddRef();
  }
  return *this;
}

void Pipe::Close(CloseMode mode) {
  switch (mode) {
    case CLOSE_READ:
      impl_->CloseRead();
      break;
    case CLOSE_WRITE:
      impl_->CloseWrite();
      break;
    default:
      impl_->CloseRead();
      impl_->CloseWrite();
      break;
  }
}

}  // namespace fun
