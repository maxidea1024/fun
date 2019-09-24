﻿/**
 * Auto-generated by IDL Compiler (1.0.2)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 */
#include "Emergency_EmergencyC2S_proxy.h"
#include "Emergency.h" // _Args, _PArgs

namespace fun {
namespace EmergencyC2S
{
  bool Proxy::EmergencyLogData_Begin(const fun::HostId* RpcRemoteIds, const fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& RpcCallOption, const fun::DateTime& logon_time, const fun::int32 connect_count, const fun::int32 remote_peer_count, const fun::int32 direct_p2p_enable_peer_count, const fun::String& nat_device_name, const fun::HostId& peer_id, const fun::int32 Iopendingcount, const fun::int32 total_tcp_issued_send_bytes_)
  {
    fun::MessageOut RpcMessage__;

    // Write RPC function id.
    fun::LiteFormat::Write(RpcMessage__, RpcIds::EmergencyLogData_Begin);

    // Write RPC header.
    fun::RpcHeader::WriteOk(RpcMessage__);

    // Write RPC function arguments.
    fun::FlexFormat::WriteDateTime(RpcMessage__, logon_time);
    fun::FlexFormat::WriteSInt32(RpcMessage__, connect_count);
    fun::FlexFormat::WriteSInt32(RpcMessage__, remote_peer_count);
    fun::FlexFormat::WriteSInt32(RpcMessage__, direct_p2p_enable_peer_count);
    fun::FlexFormat::WriteString(RpcMessage__, nat_device_name);
    EngineTypes_UserTypeHandlers::Write(RpcMessage__, peer_id);
    fun::FlexFormat::WriteSInt32(RpcMessage__, Iopendingcount);
    fun::FlexFormat::WriteSInt32(RpcMessage__, total_tcp_issued_send_bytes_);

    if (notify_send_by_proxy_enabled_ && !engine_specific_only_)
    {
      fun::String ArgsStr__ = TEXT("{");
      ArgsStr__ << TEXT("\"logon_time\":") << fun::ToString(logon_time);
      ArgsStr__ << TEXT(",\"connect_count\":") << fun::ToString(connect_count);
      ArgsStr__ << TEXT(",\"remote_peer_count\":") << fun::ToString(remote_peer_count);
      ArgsStr__ << TEXT(",\"direct_p2p_enable_peer_count\":") << fun::ToString(direct_p2p_enable_peer_count);
      ArgsStr__ << TEXT(",\"nat_device_name\":") << fun::ToString(nat_device_name);
      ArgsStr__ << TEXT(",\"peer_id\":") << fun::ToString(peer_id);
      ArgsStr__ << TEXT(",\"Iopendingcount\":") << fun::ToString(Iopendingcount);
      ArgsStr__ << TEXT(",\"total_tcp_issued_send_bytes_\":") << fun::ToString(total_tcp_issued_send_bytes_);
      ArgsStr__ << TEXT("}");

      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, RpcCallOption, RpcMessage__, RpcNames::EmergencyLogData_Begin(), RpcIds::EmergencyLogData_Begin, *ArgsStr__);
    }
    else
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, RpcCallOption, RpcMessage__, RpcNames::EmergencyLogData_Begin(), RpcIds::EmergencyLogData_Begin, nullptr);
    }
  }

  bool Proxy::EmergencyLogData_Error(const fun::HostId* RpcRemoteIds, const fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& RpcCallOption, const fun::int32 msg_size_error_count, const fun::int32 net_reset_error_count, const fun::int32 conn_reset_error_count, const fun::int32 last_error_completion_length)
  {
    fun::MessageOut RpcMessage__;

    // Write RPC function id.
    fun::LiteFormat::Write(RpcMessage__, RpcIds::EmergencyLogData_Error);

    // Write RPC header.
    fun::RpcHeader::WriteOk(RpcMessage__);

    // Write RPC function arguments.
    fun::FlexFormat::WriteSInt32(RpcMessage__, msg_size_error_count);
    fun::FlexFormat::WriteSInt32(RpcMessage__, net_reset_error_count);
    fun::FlexFormat::WriteSInt32(RpcMessage__, conn_reset_error_count);
    fun::FlexFormat::WriteSInt32(RpcMessage__, last_error_completion_length);

    if (notify_send_by_proxy_enabled_ && !engine_specific_only_)
    {
      fun::String ArgsStr__ = TEXT("{");
      ArgsStr__ << TEXT("\"msg_size_error_count\":") << fun::ToString(msg_size_error_count);
      ArgsStr__ << TEXT(",\"net_reset_error_count\":") << fun::ToString(net_reset_error_count);
      ArgsStr__ << TEXT(",\"conn_reset_error_count\":") << fun::ToString(conn_reset_error_count);
      ArgsStr__ << TEXT(",\"last_error_completion_length\":") << fun::ToString(last_error_completion_length);
      ArgsStr__ << TEXT("}");

      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, RpcCallOption, RpcMessage__, RpcNames::EmergencyLogData_Error(), RpcIds::EmergencyLogData_Error, *ArgsStr__);
    }
    else
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, RpcCallOption, RpcMessage__, RpcNames::EmergencyLogData_Error(), RpcIds::EmergencyLogData_Error, nullptr);
    }
  }

  bool Proxy::EmergencyLogData_Stats(const fun::HostId* RpcRemoteIds, const fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& RpcCallOption, const fun::int64 total_tcp_recv_bytes, const fun::int64 total_tcp_send_bytes, const fun::int64 total_udp_send_count, const fun::int64 total_udp_send_bytes, const fun::int64 total_udp_recv_count, const fun::int64 total_udp_recv_bytes)
  {
    fun::MessageOut RpcMessage__;

    // Write RPC function id.
    fun::LiteFormat::Write(RpcMessage__, RpcIds::EmergencyLogData_Stats);

    // Write RPC header.
    fun::RpcHeader::WriteOk(RpcMessage__);

    // Write RPC function arguments.
    fun::FlexFormat::WriteSInt64(RpcMessage__, total_tcp_recv_bytes);
    fun::FlexFormat::WriteSInt64(RpcMessage__, total_tcp_send_bytes);
    fun::FlexFormat::WriteSInt64(RpcMessage__, total_udp_send_count);
    fun::FlexFormat::WriteSInt64(RpcMessage__, total_udp_send_bytes);
    fun::FlexFormat::WriteSInt64(RpcMessage__, total_udp_recv_count);
    fun::FlexFormat::WriteSInt64(RpcMessage__, total_udp_recv_bytes);

    if (notify_send_by_proxy_enabled_ && !engine_specific_only_)
    {
      fun::String ArgsStr__ = TEXT("{");
      ArgsStr__ << TEXT("\"total_tcp_recv_bytes\":") << fun::ToString(total_tcp_recv_bytes);
      ArgsStr__ << TEXT(",\"total_tcp_send_bytes\":") << fun::ToString(total_tcp_send_bytes);
      ArgsStr__ << TEXT(",\"total_udp_send_count\":") << fun::ToString(total_udp_send_count);
      ArgsStr__ << TEXT(",\"total_udp_send_bytes\":") << fun::ToString(total_udp_send_bytes);
      ArgsStr__ << TEXT(",\"total_udp_recv_count\":") << fun::ToString(total_udp_recv_count);
      ArgsStr__ << TEXT(",\"total_udp_recv_bytes\":") << fun::ToString(total_udp_recv_bytes);
      ArgsStr__ << TEXT("}");

      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, RpcCallOption, RpcMessage__, RpcNames::EmergencyLogData_Stats(), RpcIds::EmergencyLogData_Stats, *ArgsStr__);
    }
    else
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, RpcCallOption, RpcMessage__, RpcNames::EmergencyLogData_Stats(), RpcIds::EmergencyLogData_Stats, nullptr);
    }
  }

  bool Proxy::EmergencyLogData_OSVersion(const fun::HostId* RpcRemoteIds, const fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& RpcCallOption, const fun::uint32 os_major_version, const fun::uint32 os_minor_version, const fun::uint8 product_type, const fun::uint16 processor_architecture)
  {
    fun::MessageOut RpcMessage__;

    // Write RPC function id.
    fun::LiteFormat::Write(RpcMessage__, RpcIds::EmergencyLogData_OSVersion);

    // Write RPC header.
    fun::RpcHeader::WriteOk(RpcMessage__);

    // Write RPC function arguments.
    fun::FlexFormat::WriteUInt32(RpcMessage__, os_major_version);
    fun::FlexFormat::WriteUInt32(RpcMessage__, os_minor_version);
    fun::FlexFormat::WriteFixed8(RpcMessage__, product_type);
    fun::FlexFormat::WriteFixed16(RpcMessage__, processor_architecture);

    if (notify_send_by_proxy_enabled_ && !engine_specific_only_)
    {
      fun::String ArgsStr__ = TEXT("{");
      ArgsStr__ << TEXT("\"os_major_version\":") << fun::ToString(os_major_version);
      ArgsStr__ << TEXT(",\"os_minor_version\":") << fun::ToString(os_minor_version);
      ArgsStr__ << TEXT(",\"product_type\":") << fun::ToString(product_type);
      ArgsStr__ << TEXT(",\"processor_architecture\":") << fun::ToString(processor_architecture);
      ArgsStr__ << TEXT("}");

      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, RpcCallOption, RpcMessage__, RpcNames::EmergencyLogData_OSVersion(), RpcIds::EmergencyLogData_OSVersion, *ArgsStr__);
    }
    else
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, RpcCallOption, RpcMessage__, RpcNames::EmergencyLogData_OSVersion(), RpcIds::EmergencyLogData_OSVersion, nullptr);
    }
  }

  bool Proxy::EmergencyLogData_LogEvent(const fun::HostId* RpcRemoteIds, const fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& RpcCallOption, const fun::LogCategory& Category, const fun::DateTime& added_time, const fun::String& text)
  {
    fun::MessageOut RpcMessage__;

    // Write RPC function id.
    fun::LiteFormat::Write(RpcMessage__, RpcIds::EmergencyLogData_LogEvent);

    // Write RPC header.
    fun::RpcHeader::WriteOk(RpcMessage__);

    // Write RPC function arguments.
    EngineTypes_UserTypeHandlers::Write(RpcMessage__, Category);
    fun::FlexFormat::WriteDateTime(RpcMessage__, added_time);
    fun::FlexFormat::WriteString(RpcMessage__, text);

    if (notify_send_by_proxy_enabled_ && !engine_specific_only_)
    {
      fun::String ArgsStr__ = TEXT("{");
      ArgsStr__ << TEXT("\"Category\":") << fun::ToString(Category);
      ArgsStr__ << TEXT(",\"added_time\":") << fun::ToString(added_time);
      ArgsStr__ << TEXT(",\"text\":") << fun::ToString(text);
      ArgsStr__ << TEXT("}");

      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, RpcCallOption, RpcMessage__, RpcNames::EmergencyLogData_LogEvent(), RpcIds::EmergencyLogData_LogEvent, *ArgsStr__);
    }
    else
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, RpcCallOption, RpcMessage__, RpcNames::EmergencyLogData_LogEvent(), RpcIds::EmergencyLogData_LogEvent, nullptr);
    }
  }

  bool Proxy::EmergencyLogData_End(const fun::HostId* RpcRemoteIds, const fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& RpcCallOption, const fun::int32 server_udp_addr_count, const fun::int32 remote_udp_addr_count)
  {
    fun::MessageOut RpcMessage__;

    // Write RPC function id.
    fun::LiteFormat::Write(RpcMessage__, RpcIds::EmergencyLogData_End);

    // Write RPC header.
    fun::RpcHeader::WriteOk(RpcMessage__);

    // Write RPC function arguments.
    fun::FlexFormat::WriteSInt32(RpcMessage__, server_udp_addr_count);
    fun::FlexFormat::WriteSInt32(RpcMessage__, remote_udp_addr_count);

    if (notify_send_by_proxy_enabled_ && !engine_specific_only_)
    {
      fun::String ArgsStr__ = TEXT("{");
      ArgsStr__ << TEXT("\"server_udp_addr_count\":") << fun::ToString(server_udp_addr_count);
      ArgsStr__ << TEXT(",\"remote_udp_addr_count\":") << fun::ToString(remote_udp_addr_count);
      ArgsStr__ << TEXT("}");

      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, RpcCallOption, RpcMessage__, RpcNames::EmergencyLogData_End(), RpcIds::EmergencyLogData_End, *ArgsStr__);
    }
    else
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, RpcCallOption, RpcMessage__, RpcNames::EmergencyLogData_End(), RpcIds::EmergencyLogData_End, nullptr);
    }
  }
} // end of namespace EmergencyC2S

} // end of namespace fun
