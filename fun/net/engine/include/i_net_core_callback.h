#pragma once

#include "fun/net/net.h"

namespace fun {
namespace net {

class ResultInfo;
class Exception;
class RpcHint;

//TODO Function<...>으로 대체하도록 하자.
class INetCoreCallbacks {
 public:
  virtual ~INetCoreCallbacks() {}

  virtual void OnError(const ResultInfo* result_info) = 0;
  virtual void OnWarning(const ResultInfo* result_info) = 0;
  virtual void OnInformation(const ResultInfo* result_info) = 0;
  virtual void OnException(HostId host_id, const Exception& e) = 0;

  virtual void OnNoRpcProcessed(RpcId rpc_id) = 0;

  virtual void OnReceiveFreeform(
        HostId sender,
        const RpcHint& rpc_hint,
        const ByteArray& payload) {}
};

} // namespace net
} // namespace fun
