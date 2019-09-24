#pragma once

#include "fun/net/net.h"

namespace fun {
namespace net {

class RpcCallOption;
class MessageOut;
class RpcHost;

/**
TODO
*/
class FUN_NETX_API RpcProxy {
 public:
  RpcHost* core_;

  bool engine_specific_only_;

  // TODO 제거하도록 하자.
  int32 signature_;

  bool notify_send_by_proxy_enabled_;

  RpcProxy();
  virtual ~RpcProxy();

  //
  //
  //

  virtual const char* GetRpcProxyName() const = 0;
  virtual int32 GetDeclaredRpcCount() const = 0;
  virtual const RpcId* GetDeclaredRpcIds() const = 0;
  virtual const char** GetDeclaredRpcNames() const = 0;

  //
  // RpcCall
  //

  virtual bool RpcCall(const HostId* rpc_sendto_list, int32 rpc_sendto_count,
                       const RpcCallOption& rpc_call_opt, const MessageOut& msg,
                       const char* rpc_name, RpcId rpc_id,
                       const char* arguments);

  //
  // RpcFailure
  //

  bool RpcFailure(HostId remote, RpcId rpc_id, int32 result_code,
                  const char* error_message = nullptr);

  bool RpcFailure(HostId remote, const RpcCallOption& rpc_call_opt,
                  RpcId rpc_id, int32 result_code,
                  const char* error_message = nullptr);

  template <typename Allocator>
  bool RpcFailure(const Array<HostId, Allocator>& rpc_sendto_list, RpcId rpc_id,
                  int32 result_code, const char* error_message = nullptr);

  template <typename Allocator>
  bool RpcFailure(const Array<HostId, Allocator>& rpc_sendto_list,
                  const RpcCallOption& rpc_call_opt, RpcId rpc_id,
                  int32 result_code, const char* error_message = nullptr);

  bool RpcFailure(const HostId* rpc_sendto_list, int32 rpc_sendto_count,
                  RpcId rpc_id, int32 result_code,
                  const char* error_message = nullptr);

  bool RpcFailure(const HostId* rpc_sendto_list, int32 rpc_sendto_count,
                  const RpcCallOption& rpc_call_opt, RpcId rpc_id,
                  int32 result_code, const char* error_message = nullptr);
};

//
// inlines
//

inline bool RpcProxy::RpcFailure(HostId remote, RpcId rpc_id, int32 result_code,
                                 const char* error_message) {
  return RpcFailure(remote, RpcCallOption::Reliable, rpc_id, result_code,
                    error_message);
}

inline bool RpcProxy::RpcFailure(HostId remote,
                                 const RpcCallOption& rpc_call_opt,
                                 RpcId rpc_id, int32 result_code,
                                 const char* error_message) {
  return RpcFailure(&remote, 1, rpc_call_opt, rpc_id, result_code,
                    error_message);
}

template <typename Allocator>
inline bool RpcProxy::RpcFailure(
    const Array<HostId, Allocator>& rpc_sendto_list, RpcId rpc_id,
    int32 result_code, const char* error_message) {
  return RpcFailure(rpc_sendto_list, RpcCallOption::Reliable, rpc_id,
                    result_code, error_message);
}

template <typename Allocator>
inline bool RpcProxy::RpcFailure(
    const Array<HostId, Allocator>& rpc_sendto_list,
    const RpcCallOption& rpc_call_opt, RpcId rpc_id, int32 result_code,
    const char* error_message) {
  return RpcFailure(rpc_sendto_list.ConstData(), rpc_sendto_list.Count(),
                    rpc_call_opt, rpc_id, result_code, error_message);
}

inline bool RpcProxy::RpcFailure(const HostId* rpc_sendto_list,
                                 int32 rpc_sendto_count, RpcId rpc_id,
                                 int32 result_code, const char* error_message) {
  return RpcFailure(rpc_sendto_list, rpc_sendto_count, RpcCallOption::Reliable,
                    rpc_id, result_code, error_message);
}

}  // namespace net
}  // namespace fun
