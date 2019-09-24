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
namespace EmergencyC2S
{
  struct Stub : public fun::RpcStub
  {
    fun::int32 GetDeclaredRpcCount() const override { return EmergencyC2S::NumRpcFunctions; }
    const fun::RpcId* GetDeclaredRpcIds() const override { return EmergencyC2S::RpcIds::RpcIdList(); }
    const char** GetDeclaredRpcNames() const override { return EmergencyC2S::RpcNames::RpcNameList(); }

    // EmergencyLogData_Begin
    virtual bool EmergencyLogData_Begin(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::DateTime& logon_time, const fun::int32 connect_count, const fun::int32 remote_peer_count, const fun::int32 direct_p2p_enable_peer_count, const fun::String& nat_device_name, const fun::HostId& peer_id, const fun::int32 Iopendingcount, const fun::int32 total_tcp_issued_send_bytes_) { return false; }
    #define DECLARE_RPCSTUB_EmergencyC2S_EmergencyLogData_Begin bool EmergencyLogData_Begin(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::DateTime& logon_time, const fun::int32 connect_count, const fun::int32 remote_peer_count, const fun::int32 direct_p2p_enable_peer_count, const fun::String& nat_device_name, const fun::HostId& peer_id, const fun::int32 Iopendingcount, const fun::int32 total_tcp_issued_send_bytes_) override;
    #define IMPLEMENT_RPCSTUB_EmergencyC2S_EmergencyLogData_Begin(TClass) bool TClass::EmergencyLogData_Begin(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::DateTime& logon_time, const fun::int32 connect_count, const fun::int32 remote_peer_count, const fun::int32 direct_p2p_enable_peer_count, const fun::String& nat_device_name, const fun::HostId& peer_id, const fun::int32 Iopendingcount, const fun::int32 total_tcp_issued_send_bytes_)
    typedef fun::Function<bool(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::DateTime& /* logon_time */, const fun::int32 /* connect_count */, const fun::int32 /* remote_peer_count */, const fun::int32 /* direct_p2p_enable_peer_count */, const fun::String& /* nat_device_name */, const fun::HostId& /* peer_id */, const fun::int32 /* Iopendingcount */, const fun::int32 /* total_tcp_issued_send_bytes_ */)> EmergencyLogData_BeginFunctionType;
    EmergencyLogData_BeginFunctionType OnEmergencyLogData_Begin;

    // EmergencyLogData_Error
    virtual bool EmergencyLogData_Error(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::int32 msg_size_error_count, const fun::int32 net_reset_error_count, const fun::int32 conn_reset_error_count, const fun::int32 last_error_completion_length) { return false; }
    #define DECLARE_RPCSTUB_EmergencyC2S_EmergencyLogData_Error bool EmergencyLogData_Error(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::int32 msg_size_error_count, const fun::int32 net_reset_error_count, const fun::int32 conn_reset_error_count, const fun::int32 last_error_completion_length) override;
    #define IMPLEMENT_RPCSTUB_EmergencyC2S_EmergencyLogData_Error(TClass) bool TClass::EmergencyLogData_Error(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::int32 msg_size_error_count, const fun::int32 net_reset_error_count, const fun::int32 conn_reset_error_count, const fun::int32 last_error_completion_length)
    typedef fun::Function<bool(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::int32 /* msg_size_error_count */, const fun::int32 /* net_reset_error_count */, const fun::int32 /* conn_reset_error_count */, const fun::int32 /* last_error_completion_length */)> EmergencyLogData_ErrorFunctionType;
    EmergencyLogData_ErrorFunctionType OnEmergencyLogData_Error;

    // EmergencyLogData_Stats
    virtual bool EmergencyLogData_Stats(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::int64 total_tcp_recv_bytes, const fun::int64 total_tcp_send_bytes, const fun::int64 total_udp_send_count, const fun::int64 total_udp_send_bytes, const fun::int64 total_udp_recv_count, const fun::int64 total_udp_recv_bytes) { return false; }
    #define DECLARE_RPCSTUB_EmergencyC2S_EmergencyLogData_Stats bool EmergencyLogData_Stats(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::int64 total_tcp_recv_bytes, const fun::int64 total_tcp_send_bytes, const fun::int64 total_udp_send_count, const fun::int64 total_udp_send_bytes, const fun::int64 total_udp_recv_count, const fun::int64 total_udp_recv_bytes) override;
    #define IMPLEMENT_RPCSTUB_EmergencyC2S_EmergencyLogData_Stats(TClass) bool TClass::EmergencyLogData_Stats(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::int64 total_tcp_recv_bytes, const fun::int64 total_tcp_send_bytes, const fun::int64 total_udp_send_count, const fun::int64 total_udp_send_bytes, const fun::int64 total_udp_recv_count, const fun::int64 total_udp_recv_bytes)
    typedef fun::Function<bool(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::int64 /* total_tcp_recv_bytes */, const fun::int64 /* total_tcp_send_bytes */, const fun::int64 /* total_udp_send_count */, const fun::int64 /* total_udp_send_bytes */, const fun::int64 /* total_udp_recv_count */, const fun::int64 /* total_udp_recv_bytes */)> EmergencyLogData_StatsFunctionType;
    EmergencyLogData_StatsFunctionType OnEmergencyLogData_Stats;

    // EmergencyLogData_OSVersion
    virtual bool EmergencyLogData_OSVersion(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::uint32 os_major_version, const fun::uint32 os_minor_version, const fun::uint8 product_type, const fun::uint16 processor_architecture) { return false; }
    #define DECLARE_RPCSTUB_EmergencyC2S_EmergencyLogData_OSVersion bool EmergencyLogData_OSVersion(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::uint32 os_major_version, const fun::uint32 os_minor_version, const fun::uint8 product_type, const fun::uint16 processor_architecture) override;
    #define IMPLEMENT_RPCSTUB_EmergencyC2S_EmergencyLogData_OSVersion(TClass) bool TClass::EmergencyLogData_OSVersion(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::uint32 os_major_version, const fun::uint32 os_minor_version, const fun::uint8 product_type, const fun::uint16 processor_architecture)
    typedef fun::Function<bool(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::uint32 /* os_major_version */, const fun::uint32 /* os_minor_version */, const fun::uint8 /* product_type */, const fun::uint16 /* processor_architecture */)> EmergencyLogData_OSVersionFunctionType;
    EmergencyLogData_OSVersionFunctionType OnEmergencyLogData_OSVersion;

    // EmergencyLogData_LogEvent
    virtual bool EmergencyLogData_LogEvent(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::LogCategory& Category, const fun::DateTime& added_time, const fun::String& text) { return false; }
    #define DECLARE_RPCSTUB_EmergencyC2S_EmergencyLogData_LogEvent bool EmergencyLogData_LogEvent(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::LogCategory& Category, const fun::DateTime& added_time, const fun::String& text) override;
    #define IMPLEMENT_RPCSTUB_EmergencyC2S_EmergencyLogData_LogEvent(TClass) bool TClass::EmergencyLogData_LogEvent(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::LogCategory& Category, const fun::DateTime& added_time, const fun::String& text)
    typedef fun::Function<bool(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::LogCategory& /* Category */, const fun::DateTime& /* added_time */, const fun::String& /* text */)> EmergencyLogData_LogEventFunctionType;
    EmergencyLogData_LogEventFunctionType OnEmergencyLogData_LogEvent;

    // EmergencyLogData_End
    virtual bool EmergencyLogData_End(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::int32 server_udp_addr_count, const fun::int32 remote_udp_addr_count) { return false; }
    #define DECLARE_RPCSTUB_EmergencyC2S_EmergencyLogData_End bool EmergencyLogData_End(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::int32 server_udp_addr_count, const fun::int32 remote_udp_addr_count) override;
    #define IMPLEMENT_RPCSTUB_EmergencyC2S_EmergencyLogData_End(TClass) bool TClass::EmergencyLogData_End(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::int32 server_udp_addr_count, const fun::int32 remote_udp_addr_count)
    typedef fun::Function<bool(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::int32 /* server_udp_addr_count */, const fun::int32 /* remote_udp_addr_count */)> EmergencyLogData_EndFunctionType;
    EmergencyLogData_EndFunctionType OnEmergencyLogData_End;

    // RpcStub interface
    bool ProcessReceivedMessage(fun::ReceivedMessage& received_msg, void* host_tag) override;
  };

} // end of namespace EmergencyC2S

} // end of namespace fun
