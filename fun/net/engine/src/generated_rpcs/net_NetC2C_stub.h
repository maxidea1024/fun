﻿/**
 * Auto-generated by IDL Compiler (2.1.3)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 */
#pragma once

#include "FUN.h"
#include "net_defs.h"
#include "net_types.h"

namespace fun {
namespace NetC2C
{
  struct Stub : public fun::RpcStub
  {
    fun::int32 GetDeclaredRpcCount() const override { return NetC2C::NumRpcFunctions; }
    const fun::RpcId* GetDeclaredRpcIds() const override { return NetC2C::RpcIds::RpcIdList(); }
    const char** GetDeclaredRpcNames() const override { return NetC2C::RpcNames::RpcNameList(); }

    // SuppressP2PHolepunchTrial
    virtual bool SuppressP2PHolepunchTrial(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint) { return false; }
    #define DECLARE_RPCSTUB_NetC2C_SuppressP2PHolepunchTrial bool SuppressP2PHolepunchTrial(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint) override;
    #define IMPLEMENT_RPCSTUB_NetC2C_SuppressP2PHolepunchTrial(TClass) bool TClass::SuppressP2PHolepunchTrial(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint)
    typedef fun::Function<bool(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint)> SuppressP2PHolepunchTrialFunctionType;
    SuppressP2PHolepunchTrialFunctionType OnSuppressP2PHolepunchTrial;

    // ReportUdpMessageCount
    virtual bool ReportUdpMessageCount(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::uint32 UdpSuccessCount) { return false; }
    #define DECLARE_RPCSTUB_NetC2C_ReportUdpMessageCount bool ReportUdpMessageCount(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::uint32 UdpSuccessCount) override;
    #define IMPLEMENT_RPCSTUB_NetC2C_ReportUdpMessageCount(TClass) bool TClass::ReportUdpMessageCount(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::uint32 UdpSuccessCount)
    typedef fun::Function<bool(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::uint32 /* UdpSuccessCount */)> ReportUdpMessageCountFunctionType;
    ReportUdpMessageCountFunctionType OnReportUdpMessageCount;

    // ReportServerTimeAndFrameRateAndPing
    virtual bool ReportServerTimeAndFrameRateAndPing(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const double client_local_time, const double recent_frame_rate) { return false; }
    #define DECLARE_RPCSTUB_NetC2C_ReportServerTimeAndFrameRateAndPing bool ReportServerTimeAndFrameRateAndPing(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const double client_local_time, const double recent_frame_rate) override;
    #define IMPLEMENT_RPCSTUB_NetC2C_ReportServerTimeAndFrameRateAndPing(TClass) bool TClass::ReportServerTimeAndFrameRateAndPing(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const double client_local_time, const double recent_frame_rate)
    typedef fun::Function<bool(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const double /* client_local_time */, const double /* recent_frame_rate */)> ReportServerTimeAndFrameRateAndPingFunctionType;
    ReportServerTimeAndFrameRateAndPingFunctionType OnReportServerTimeAndFrameRateAndPing;

    // ReportServerTimeAndFrameRateAndPong
    virtual bool ReportServerTimeAndFrameRateAndPong(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const double OldClientLocalTime, const double server_local_time, const double server_udp_recent_ping, const double recent_frame_rate) { return false; }
    #define DECLARE_RPCSTUB_NetC2C_ReportServerTimeAndFrameRateAndPong bool ReportServerTimeAndFrameRateAndPong(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const double OldClientLocalTime, const double server_local_time, const double server_udp_recent_ping, const double recent_frame_rate) override;
    #define IMPLEMENT_RPCSTUB_NetC2C_ReportServerTimeAndFrameRateAndPong(TClass) bool TClass::ReportServerTimeAndFrameRateAndPong(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const double OldClientLocalTime, const double server_local_time, const double server_udp_recent_ping, const double recent_frame_rate)
    typedef fun::Function<bool(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const double /* OldClientLocalTime */, const double /* server_local_time */, const double /* server_udp_recent_ping */, const double /* recent_frame_rate */)> ReportServerTimeAndFrameRateAndPongFunctionType;
    ReportServerTimeAndFrameRateAndPongFunctionType OnReportServerTimeAndFrameRateAndPong;

    // RpcStub interface
    bool ProcessReceivedMessage(fun::ReceivedMessage& received_msg, void* host_tag) override;
  };

} // end of namespace NetC2C

} // end of namespace fun
