﻿/**
 * Auto-generated by IDL Compiler (1.0.2)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 */
#pragma once

#include "FUN.h"
#include "Viz_defs.h"
#include "Viz_types.h"

namespace fun {
namespace VizS2C
{
  struct Proxy : public fun::RpcProxy
  {
    fun::int32 GetDeclaredRpcCount() const override { return VizS2C::NumRpcFunctions; }
    const fun::RpcId* GetDeclaredRpcIds() const override { return VizS2C::RpcIds::RpcIdList(); }
    const char** GetDeclaredRpcNames() const override { return VizS2C::RpcNames::RpcNameList(); }

    inline bool NotifyLoginOk(fun::HostId rpc_recvfrom) { return NotifyLoginOk(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable); }
    inline bool NotifyLoginOk(fun::HostId rpc_recvfrom, const fun::RpcCallOption& RpcCallOption) { return NotifyLoginOk(&rpc_recvfrom, 1, RpcCallOption); }
    template <typename Allocator>
    inline bool NotifyLoginOk(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds) { return NotifyLoginOk(RpcRemoteIds, fun::RpcCallOption::Reliable); }
    template <typename Allocator>
    inline bool NotifyLoginOk(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& RpcCallOption) { return NotifyLoginOk(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), RpcCallOption); }
    inline bool NotifyLoginOk(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount) { return NotifyLoginOk(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable); }
    bool NotifyLoginOk(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& RpcCallOption);

    inline bool NotifyLoginFailed(fun::HostId rpc_recvfrom, const fun::ResultCode& reason) { return NotifyLoginFailed(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable, reason); }
    inline bool NotifyLoginFailed(fun::HostId rpc_recvfrom, const fun::RpcCallOption& RpcCallOption, const fun::ResultCode& reason) { return NotifyLoginFailed(&rpc_recvfrom, 1, RpcCallOption, reason); }
    template <typename Allocator>
    inline bool NotifyLoginFailed(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::ResultCode& reason) { return NotifyLoginFailed(RpcRemoteIds, fun::RpcCallOption::Reliable, reason); }
    template <typename Allocator>
    inline bool NotifyLoginFailed(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& RpcCallOption, const fun::ResultCode& reason) { return NotifyLoginFailed(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), RpcCallOption, reason); }
    inline bool NotifyLoginFailed(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::ResultCode& reason) { return NotifyLoginFailed(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable, reason); }
    bool NotifyLoginFailed(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& RpcCallOption, const fun::ResultCode& reason);
  };
} // end of namespace VizS2C

} // end of namespace fun
