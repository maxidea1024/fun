#pragma once

#include "fun/base/base.h"
#include "fun/base/string/string.h"
#include "fun/base/container/map.h"

namespace fun {

/** Holds generic memory stats, internally implemented as a map. */
struct CGenericMemoryStats {
  void Add(const char* desc, const size_t value) {
    Data.Add(String(desc), value);
  }

  Map<string, size_t> data;
};

} // namespace fun
