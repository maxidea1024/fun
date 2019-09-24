﻿/**
 * Auto-generated by IDL Compiler (1.0.2)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 */
#include "viz_S2C_proxy.h"
#include "viz.h" // _Args, _PArgs

namespace fun { namespace Viz {
namespace S2C
{
  bool Proxy::NotifyLoginOk(const fun::HostId* RpcRemoteIds, const fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& RpcCallOption)
  {
    fun::MessageOut RpcMessage__;

    // Write RPC function id.
    fun::LiteFormat::Write(RpcMessage__, RpcIds::NotifyLoginOk);

    // Write RPC header.
    fun::RpcHeader::WriteOk(RpcMessage__);

    // No RPC function arguments.

    if (notify_send_by_proxy_enabled_ && !engine_specific_only_)
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, RpcCallOption, RpcMessage__, RpcNames::NotifyLoginOk(), RpcIds::NotifyLoginOk, TEXT("{}"));
    }
    else
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, RpcCallOption, RpcMessage__, RpcNames::NotifyLoginOk(), RpcIds::NotifyLoginOk, nullptr);
    }
  }

  bool Proxy::NotifyLoginFailed(const fun::HostId* RpcRemoteIds, const fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& RpcCallOption, const fun::ResultCode& reason)
  {
    fun::MessageOut RpcMessage__;

    // Write RPC function id.
    fun::LiteFormat::Write(RpcMessage__, RpcIds::NotifyLoginFailed);

    // Write RPC header.
    fun::RpcHeader::WriteOk(RpcMessage__);

    // Write RPC function arguments.
    fun::EngineTypes_UserTypeHandlers::Write(RpcMessage__, reason);

    if (notify_send_by_proxy_enabled_ && !engine_specific_only_)
    {
      fun::String ArgsStr__ = TEXT("{");
      ArgsStr__ << TEXT("\"reason\":") << fun::ToString(reason);
      ArgsStr__ << TEXT("}");

      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, RpcCallOption, RpcMessage__, RpcNames::NotifyLoginFailed(), RpcIds::NotifyLoginFailed, *ArgsStr__);
    }
    else
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, RpcCallOption, RpcMessage__, RpcNames::NotifyLoginFailed(), RpcIds::NotifyLoginFailed, nullptr);
    }
  }
} // end of namespace S2C

}} // end of namespace fun::Viz
