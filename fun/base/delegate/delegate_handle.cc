#include "fun/base/atomic_counter.h"
#include "fun/base/delegate/i_delegate_instance.h"

namespace fun {

namespace {
static AtomicCounter64 g_next_id(1);
}

uint64 DelegateHandle::GenerateNewId() {
  // Just increment a counter to generate an ID.
  uint64 result = ++g_next_id;

  // Check for the next-to-impossible event that we wrap round to 0,
  // because we reserve 0 for null delegates.
  if (result == 0) {
    // Increment it again - it might not be zero, so don't just assign it to 1.
    result = ++g_next_id;
  }

  return result;
}

}  // namespace fun
