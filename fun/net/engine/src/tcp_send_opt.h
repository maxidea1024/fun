#pragma once

#include "fun/net/net.h"

namespace fun {
namespace net {

class TcpSendOption {
 public:
  /** 실제로 사용되지 않는 값임. (전처리로 금지되어 있는 상태임) */
  uint64 unique_id;
  /** Lookback 허용 여부. */
  bool bounce;
  //bool no_coalesce;

 public:
  TcpSendOption()
    : unique_id(0),
      bounce(true)
    //, no_coalesce(false)
  {
  }

  TcpSendOption(const SendOption& src)
    : unique_id(src.unique_id),
      bounce(src.bounce)
    //, no_coalesce(src.no_coalesce)
  {
  }

  TcpSendOption(const RpcCallOption& src)
    : unique_id(src.unique_id),
      bounce(src.bounce)
    //, no_coalesce(src.no_coalesce)
  {
  }

  TcpSendOption(const UdpSendOption& src)
    : unique_id(src.unique_id),
      bounce(src.bounce)
    //, no_coalesce(src.no_coalesce)
  {
  }
};

} // namespace net
} // namespace fun
