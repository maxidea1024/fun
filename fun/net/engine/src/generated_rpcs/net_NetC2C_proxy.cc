﻿/**
 * Auto-generated by IDL Compiler (2.1.3)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 */
#include "net_NetC2C_proxy.h"
#include "net.h" // _Args, _PArgs

namespace fun {
namespace NetC2C
{
  bool Proxy::SuppressP2PHolepunchTrial(const fun::HostId* RpcRemoteIds, const fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt)
  {
    fun::MessageOut RpcMessage__;

    // Write RPC function id.
    fun::LiteFormat::Write(RpcMessage__, RpcIds::SuppressP2PHolepunchTrial);

    // Write RPC header.
    fun::RpcHeader::WriteOk(RpcMessage__);

    // No RPC function arguments.

    if (notify_send_by_proxy_enabled_ && !engine_specific_only_)
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::SuppressP2PHolepunchTrial(), RpcIds::SuppressP2PHolepunchTrial, TEXT("{}"));
    }
    else
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::SuppressP2PHolepunchTrial(), RpcIds::SuppressP2PHolepunchTrial, nullptr);
    }
  }

  bool Proxy::ReportUdpMessageCount(const fun::HostId* RpcRemoteIds, const fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const fun::uint32 UdpSuccessCount)
  {
    fun::MessageOut RpcMessage__;

    // Write RPC function id.
    fun::LiteFormat::Write(RpcMessage__, RpcIds::ReportUdpMessageCount);

    // Write RPC header.
    fun::RpcHeader::WriteOk(RpcMessage__);

    // Write RPC function arguments.
    fun::FlexFormat::WriteUInt32(RpcMessage__, UdpSuccessCount);

    if (notify_send_by_proxy_enabled_ && !engine_specific_only_)
    {
      fun::String ArgsStr__ = TEXT("{");
      ArgsStr__ << TEXT("\"UdpSuccessCount\":") << fun::ToString(UdpSuccessCount);
      ArgsStr__ << TEXT("}");

      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::ReportUdpMessageCount(), RpcIds::ReportUdpMessageCount, *ArgsStr__);
    }
    else
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::ReportUdpMessageCount(), RpcIds::ReportUdpMessageCount, nullptr);
    }
  }

  bool Proxy::ReportServerTimeAndFrameRateAndPing(const fun::HostId* RpcRemoteIds, const fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const double client_local_time, const double recent_frame_rate)
  {
    fun::MessageOut RpcMessage__;

    // Write RPC function id.
    fun::LiteFormat::Write(RpcMessage__, RpcIds::ReportServerTimeAndFrameRateAndPing);

    // Write RPC header.
    fun::RpcHeader::WriteOk(RpcMessage__);

    // Write RPC function arguments.
    fun::FlexFormat::WriteDouble(RpcMessage__, client_local_time);
    fun::FlexFormat::WriteDouble(RpcMessage__, recent_frame_rate);

    if (notify_send_by_proxy_enabled_ && !engine_specific_only_)
    {
      fun::String ArgsStr__ = TEXT("{");
      ArgsStr__ << TEXT("\"client_local_time\":") << fun::ToString(client_local_time);
      ArgsStr__ << TEXT(",\"recent_frame_rate\":") << fun::ToString(recent_frame_rate);
      ArgsStr__ << TEXT("}");

      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::ReportServerTimeAndFrameRateAndPing(), RpcIds::ReportServerTimeAndFrameRateAndPing, *ArgsStr__);
    }
    else
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::ReportServerTimeAndFrameRateAndPing(), RpcIds::ReportServerTimeAndFrameRateAndPing, nullptr);
    }
  }

  bool Proxy::ReportServerTimeAndFrameRateAndPong(const fun::HostId* RpcRemoteIds, const fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const double OldClientLocalTime, const double server_local_time, const double server_udp_recent_ping, const double recent_frame_rate)
  {
    fun::MessageOut RpcMessage__;

    // Write RPC function id.
    fun::LiteFormat::Write(RpcMessage__, RpcIds::ReportServerTimeAndFrameRateAndPong);

    // Write RPC header.
    fun::RpcHeader::WriteOk(RpcMessage__);

    // Write RPC function arguments.
    fun::FlexFormat::WriteDouble(RpcMessage__, OldClientLocalTime);
    fun::FlexFormat::WriteDouble(RpcMessage__, server_local_time);
    fun::FlexFormat::WriteDouble(RpcMessage__, server_udp_recent_ping);
    fun::FlexFormat::WriteDouble(RpcMessage__, recent_frame_rate);

    if (notify_send_by_proxy_enabled_ && !engine_specific_only_)
    {
      fun::String ArgsStr__ = TEXT("{");
      ArgsStr__ << TEXT("\"OldClientLocalTime\":") << fun::ToString(OldClientLocalTime);
      ArgsStr__ << TEXT(",\"server_local_time\":") << fun::ToString(server_local_time);
      ArgsStr__ << TEXT(",\"server_udp_recent_ping\":") << fun::ToString(server_udp_recent_ping);
      ArgsStr__ << TEXT(",\"recent_frame_rate\":") << fun::ToString(recent_frame_rate);
      ArgsStr__ << TEXT("}");

      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::ReportServerTimeAndFrameRateAndPong(), RpcIds::ReportServerTimeAndFrameRateAndPong, *ArgsStr__);
    }
    else
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::ReportServerTimeAndFrameRateAndPong(), RpcIds::ReportServerTimeAndFrameRateAndPong, nullptr);
    }
  }
} // end of namespace NetC2C

} // end of namespace fun
