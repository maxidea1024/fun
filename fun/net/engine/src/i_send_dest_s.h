#pragma once

#include "LeanType.h"

namespace fun {
namespace net {

class ISendDest_S {
 public:
  static ISendDest_S None;

  virtual ~ISendDest_S() {}

  virtual LeanType GetLeanType() const { return LeanType::None; }
  virtual HostId GetHostId() const { return HostId_None; }
};

} // namespace net
} // namespace fun
