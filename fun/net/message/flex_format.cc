#include "fun/net/message/flex_format.h"

namespace fun {
namespace net {

//
// Tagging
//

bool FlexFormat::SkipField(IMessageIn& input, uint32 tag) {
  switch (GetTagWireType(tag)) {
    case WireType::Varint: {
      uint64 dummy;
      return input.ReadVarint64(dummy);
    }

    case WireType::Fixed8:
      return input.SkipRead(1);

    case WireType::Fixed16:
      return input.SkipRead(2);

    case WireType::Fixed32:
      return input.SkipRead(4);

    case WireType::Fixed64:
      return input.SkipRead(8);

    case WireType::LengthPrefixed: {
      int32 length;
      if (!MessageFormat::ReadCounter(input, length)) {
        return false;
      }

      return input.SkipRead(length);
    }
  }

  // couldn't reached at here!
  fun_check(0);
  return false;
}

bool FlexFormat::SkipStruct(IMessageIn& input) {
  while (true) {
    uint32 tag;
    if (!ReadTag(input, tag)) {
      return false;
    }

    if (tag == 0) {
      // end of input.  this is a valid place to end, so return true.
      return true;
    }

    if (!SkipField(input, tag)) {
      return false;
    }
  }

  // couldn't reached at here.
  fun_check(0);
  return false;
}

#undef FUN_DO_CHECKED

} // namespace net
} // namespace fun
