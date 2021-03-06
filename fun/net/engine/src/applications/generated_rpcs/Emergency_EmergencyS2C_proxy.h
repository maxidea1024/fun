﻿/**
 * Auto-generated by IDL Compiler (1.0.2)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 */
#pragma once

#include "FUN.h"
#include "Emergency_defs.h"
#include "Emergency_types.h"

namespace fun {
namespace EmergencyS2C
{
  struct Proxy : public fun::RpcProxy
  {
    fun::int32 GetDeclaredRpcCount() const override { return EmergencyS2C::NumRpcFunctions; }
    const fun::RpcId* GetDeclaredRpcIds() const override { return EmergencyS2C::RpcIds::RpcIdList(); }
    const char** GetDeclaredRpcNames() const override { return EmergencyS2C::RpcNames::RpcNameList(); }

    inline bool EmergencyLogData_AckComplete(fun::HostId rpc_recvfrom) { return EmergencyLogData_AckComplete(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable); }
    inline bool EmergencyLogData_AckComplete(fun::HostId rpc_recvfrom, const fun::RpcCallOption& RpcCallOption) { return EmergencyLogData_AckComplete(&rpc_recvfrom, 1, RpcCallOption); }
    template <typename Allocator>
    inline bool EmergencyLogData_AckComplete(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds) { return EmergencyLogData_AckComplete(RpcRemoteIds, fun::RpcCallOption::Reliable); }
    template <typename Allocator>
    inline bool EmergencyLogData_AckComplete(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& RpcCallOption) { return EmergencyLogData_AckComplete(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), RpcCallOption); }
    inline bool EmergencyLogData_AckComplete(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount) { return EmergencyLogData_AckComplete(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable); }
    bool EmergencyLogData_AckComplete(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& RpcCallOption);
  };
} // end of namespace EmergencyS2C

} // end of namespace fun
