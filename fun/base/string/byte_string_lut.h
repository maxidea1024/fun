#pragma once

namespace fun {

//TODO 제거하도록 하자.
extern const uint8 latin1_uppercased[256];
extern const uint8 latin1_lowercased[256];

static inline char FoldCase(char ch) {
  return latin1_uppercased[uint8(ch)];
}

static inline uint8 FoldCase(uint8 ch) {
  return latin1_uppercased[ch];
}

} // namespace fun
