﻿/**
 * Auto-generated by IDL Compiler (2.1.3)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 */
#include "viz_VizS2C_proxy.h"
#include "viz.h" // _Args, _PArgs

namespace fun {
namespace VizS2C {

bool Proxy::NotifyLoginOk(const fun::HostId* RpcRemoteIds,
                          const fun::int32 RpcRemoteIdCount,
                          const fun::RpcCallOption& rpc_call_opt) {
  fun::MessageOut RpcMessage__;

  // Write RPC function id.
  fun::LiteFormat::Write(RpcMessage__, RpcIds::NotifyLoginOk);

  // Write RPC header.
  fun::RpcHeader::WriteOk(RpcMessage__);

  // No RPC function arguments.

  if (notify_send_by_proxy_enabled_ && !engine_specific_only_) {
    return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::NotifyLoginOk(), RpcIds::NotifyLoginOk, TEXT("{}"));
  }
  else {
    return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::NotifyLoginOk(), RpcIds::NotifyLoginOk, nullptr);
  }
}

bool Proxy::NotifyLoginFailed(const fun::HostId* RpcRemoteIds,
                              const fun::int32 RpcRemoteIdCount,
                              const fun::RpcCallOption& rpc_call_opt,
                              const fun::ResultCode& reason) {
  fun::MessageOut RpcMessage__;

  // Write RPC function id.
  fun::LiteFormat::Write(RpcMessage__, RpcIds::NotifyLoginFailed);

  // Write RPC header.
  fun::RpcHeader::WriteOk(RpcMessage__);

  // Write RPC function arguments.
  EngineTypes_UserTypeHandlers::Write(RpcMessage__, reason);

  if (notify_send_by_proxy_enabled_ && !engine_specific_only_) {
    fun::String ArgsStr__ = TEXT("{");
    ArgsStr__ << TEXT("\"reason\":") << fun::ToString(reason);
    ArgsStr__ << TEXT("}");
    return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::NotifyLoginFailed(), RpcIds::NotifyLoginFailed, *ArgsStr__);
  }
  else {
    return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::NotifyLoginFailed(), RpcIds::NotifyLoginFailed, nullptr);
  }
}

} // end of namespace VizS2C
} // end of namespace fun