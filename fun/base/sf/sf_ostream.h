//TODO 지원하지 말자..

#pragma once

#include "fun/base/base.h"

#include "fun/base/sf/sf_format.h"
#include <ostream>

namespace fun {
namespace sf {

namespace internal {

template <typename Char>
class FormatBuf : public std::basic_streambuf<Char> {
 private:
  
};

} // namespace internal

} // namespace sf
} // namespace fun
