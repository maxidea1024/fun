﻿/**
 * Auto-generated by IDL Compiler (1.0.2)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 */
#pragma once

#include "FUN.h"
#include "viz_defs.h"
#include "viz_types.h"

namespace fun { namespace Viz {
namespace S2C
{
  struct Stub : public fun::RpcStub
  {
    fun::int32 GetDeclaredRpcCount() const override { return S2C::NumRpcFunctions; }
    const fun::RpcId* GetDeclaredRpcIds() const override { return S2C::RpcIds::RpcIdList(); }
    const char** GetDeclaredRpcNames() const override { return S2C::RpcNames::RpcNameList(); }

    // NotifyLoginOk
    virtual bool NotifyLoginOk(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint) { return false; }
    #define DECLARE_RPCSTUB_S2C_NotifyLoginOk bool NotifyLoginOk(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint) override;
    #define IMPLEMENT_RPCSTUB_S2C_NotifyLoginOk(TClass) bool TClass::NotifyLoginOk(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint)
    typedef fun::Function<bool(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint)> NotifyLoginOkFunctionType;
    NotifyLoginOkFunctionType OnNotifyLoginOk;

    // NotifyLoginFailed
    virtual bool NotifyLoginFailed(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::ResultCode& reason) { return false; }
    #define DECLARE_RPCSTUB_S2C_NotifyLoginFailed bool NotifyLoginFailed(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::ResultCode& reason) override;
    #define IMPLEMENT_RPCSTUB_S2C_NotifyLoginFailed(TClass) bool TClass::NotifyLoginFailed(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::ResultCode& reason)
    typedef fun::Function<bool(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::ResultCode& /* reason */)> NotifyLoginFailedFunctionType;
    NotifyLoginFailedFunctionType OnNotifyLoginFailed;

    // RpcStub interface
    bool ProcessReceivedMessage(fun::ReceivedMessage& received_msg, void* host_tag) override;
  };

} // end of namespace S2C

}} // end of namespace fun::Viz
