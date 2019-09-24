#include "fun/base/pipe_impl_dummy.h"

namespace fun {

PipeImpl::PipeImpl() {
  // NOOP
}

PipeImpl::~PipeImpl() {
  // NOOP
}

int PipeImpl::WriteBytes(const void* data, int len) {
  // NOOP
  return 0;
}

int PipeImpl::ReadBytes(void* buf, int len) {
  // NOOP
  return 0;
}

PipeImpl::Handle PipeImpl::ReadHandle() const {
  // NOOP
  return 0;
}

PipeImpl::Handle PipeImpl::WriteHandle() const {
  // NOOP
  return 0;
}

void PipeImpl::CloseRead() {
  // NOOP
}

void PipeImpl::CloseWrite() {
  // NOOP
}

}  // namespace fun
