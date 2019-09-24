#include "fun/base/shared_memory_dummy.h"

namespace fun {

SharedMemoryImpl::SharedMemoryImpl(const String&, size_t,
                                   SharedMemory::AccessMode, const void*,
                                   bool) {
  // NOOP
}

SharedMemoryImpl::SharedMemoryImpl(const File&, SharedMemory::AccessMode,
                                   const void*) {
  // NOOP
}

SharedMemoryImpl::~SharedMemoryImpl() {
  // NOOP
}

}  // namespace fun
