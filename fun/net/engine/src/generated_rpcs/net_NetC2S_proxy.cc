﻿/**
 * Auto-generated by IDL Compiler (2.1.3)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 */
#include "net_NetC2S_proxy.h"
#include "net.h" // _Args, _PArgs

namespace fun {
namespace NetC2S
{
  bool Proxy::ReliablePing(const fun::HostId* RpcRemoteIds, const fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const double recent_frame_rate)
  {
    fun::MessageOut RpcMessage__;

    // Write RPC function id.
    fun::LiteFormat::Write(RpcMessage__, RpcIds::ReliablePing);

    // Write RPC header.
    fun::RpcHeader::WriteOk(RpcMessage__);

    // Write RPC function arguments.
    fun::FlexFormat::WriteDouble(RpcMessage__, recent_frame_rate);

    if (notify_send_by_proxy_enabled_ && !engine_specific_only_)
    {
      fun::String ArgsStr__ = TEXT("{");
      ArgsStr__ << TEXT("\"recent_frame_rate\":") << fun::ToString(recent_frame_rate);
      ArgsStr__ << TEXT("}");

      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::ReliablePing(), RpcIds::ReliablePing, *ArgsStr__);
    }
    else
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::ReliablePing(), RpcIds::ReliablePing, nullptr);
    }
  }

  bool Proxy::P2P_NotifyDirectP2PDisconnected(const fun::HostId* RpcRemoteIds, const fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& peer_id, const fun::ResultCode& reason)
  {
    fun::MessageOut RpcMessage__;

    // Write RPC function id.
    fun::LiteFormat::Write(RpcMessage__, RpcIds::P2P_NotifyDirectP2PDisconnected);

    // Write RPC header.
    fun::RpcHeader::WriteOk(RpcMessage__);

    // Write RPC function arguments.
    EngineTypes_UserTypeHandlers::Write(RpcMessage__, peer_id);
    EngineTypes_UserTypeHandlers::Write(RpcMessage__, reason);

    if (notify_send_by_proxy_enabled_ && !engine_specific_only_)
    {
      fun::String ArgsStr__ = TEXT("{");
      ArgsStr__ << TEXT("\"peer_id\":") << fun::ToString(peer_id);
      ArgsStr__ << TEXT(",\"reason\":") << fun::ToString(reason);
      ArgsStr__ << TEXT("}");

      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::P2P_NotifyDirectP2PDisconnected(), RpcIds::P2P_NotifyDirectP2PDisconnected, *ArgsStr__);
    }
    else
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::P2P_NotifyDirectP2PDisconnected(), RpcIds::P2P_NotifyDirectP2PDisconnected, nullptr);
    }
  }

  bool Proxy::NotifyUdpToTcpFallbackByClient(const fun::HostId* RpcRemoteIds, const fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt)
  {
    fun::MessageOut RpcMessage__;

    // Write RPC function id.
    fun::LiteFormat::Write(RpcMessage__, RpcIds::NotifyUdpToTcpFallbackByClient);

    // Write RPC header.
    fun::RpcHeader::WriteOk(RpcMessage__);

    // No RPC function arguments.

    if (notify_send_by_proxy_enabled_ && !engine_specific_only_)
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::NotifyUdpToTcpFallbackByClient(), RpcIds::NotifyUdpToTcpFallbackByClient, TEXT("{}"));
    }
    else
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::NotifyUdpToTcpFallbackByClient(), RpcIds::NotifyUdpToTcpFallbackByClient, nullptr);
    }
  }

  bool Proxy::P2PGroup_MemberJoin_Ack(const fun::HostId* RpcRemoteIds, const fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& group_id, const fun::HostId& added_member_id, const fun::uint32 event_id, const bool bLocalPortReuseOk)
  {
    fun::MessageOut RpcMessage__;

    // Write RPC function id.
    fun::LiteFormat::Write(RpcMessage__, RpcIds::P2PGroup_MemberJoin_Ack);

    // Write RPC header.
    fun::RpcHeader::WriteOk(RpcMessage__);

    // Write RPC function arguments.
    EngineTypes_UserTypeHandlers::Write(RpcMessage__, group_id);
    EngineTypes_UserTypeHandlers::Write(RpcMessage__, added_member_id);
    fun::FlexFormat::WriteUInt32(RpcMessage__, event_id);
    fun::FlexFormat::WriteBool(RpcMessage__, bLocalPortReuseOk);

    if (notify_send_by_proxy_enabled_ && !engine_specific_only_)
    {
      fun::String ArgsStr__ = TEXT("{");
      ArgsStr__ << TEXT("\"group_id\":") << fun::ToString(group_id);
      ArgsStr__ << TEXT(",\"added_member_id\":") << fun::ToString(added_member_id);
      ArgsStr__ << TEXT(",\"event_id\":") << fun::ToString(event_id);
      ArgsStr__ << TEXT(",\"bLocalPortReuseOk\":") << fun::ToString(bLocalPortReuseOk);
      ArgsStr__ << TEXT("}");

      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::P2PGroup_MemberJoin_Ack(), RpcIds::P2PGroup_MemberJoin_Ack, *ArgsStr__);
    }
    else
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::P2PGroup_MemberJoin_Ack(), RpcIds::P2PGroup_MemberJoin_Ack, nullptr);
    }
  }

  bool Proxy::NotifyP2PHolepunchSuccess(const fun::HostId* RpcRemoteIds, const fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& A, const fun::HostId& B, const fun::InetAddress a2b_send_addr, const fun::InetAddress a2b_recv_addr, const fun::InetAddress b2a_send_addr, const fun::InetAddress b2a_recv_addr)
  {
    fun::MessageOut RpcMessage__;

    // Write RPC function id.
    fun::LiteFormat::Write(RpcMessage__, RpcIds::NotifyP2PHolepunchSuccess);

    // Write RPC header.
    fun::RpcHeader::WriteOk(RpcMessage__);

    // Write RPC function arguments.
    EngineTypes_UserTypeHandlers::Write(RpcMessage__, A);
    EngineTypes_UserTypeHandlers::Write(RpcMessage__, B);
    EngineTypes_UserTypeHandlers::Write(RpcMessage__, a2b_send_addr);
    EngineTypes_UserTypeHandlers::Write(RpcMessage__, a2b_recv_addr);
    EngineTypes_UserTypeHandlers::Write(RpcMessage__, b2a_send_addr);
    EngineTypes_UserTypeHandlers::Write(RpcMessage__, b2a_recv_addr);

    if (notify_send_by_proxy_enabled_ && !engine_specific_only_)
    {
      fun::String ArgsStr__ = TEXT("{");
      ArgsStr__ << TEXT("\"A\":") << fun::ToString(A);
      ArgsStr__ << TEXT(",\"B\":") << fun::ToString(B);
      ArgsStr__ << TEXT(",\"a2b_send_addr\":") << fun::ToString(a2b_send_addr);
      ArgsStr__ << TEXT(",\"a2b_recv_addr\":") << fun::ToString(a2b_recv_addr);
      ArgsStr__ << TEXT(",\"b2a_send_addr\":") << fun::ToString(b2a_send_addr);
      ArgsStr__ << TEXT(",\"b2a_recv_addr\":") << fun::ToString(b2a_recv_addr);
      ArgsStr__ << TEXT("}");

      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::NotifyP2PHolepunchSuccess(), RpcIds::NotifyP2PHolepunchSuccess, *ArgsStr__);
    }
    else
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::NotifyP2PHolepunchSuccess(), RpcIds::NotifyP2PHolepunchSuccess, nullptr);
    }
  }

  bool Proxy::ShutdownTcp(const fun::HostId* RpcRemoteIds, const fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const fun::ByteArray& comment)
  {
    fun::MessageOut RpcMessage__;

    // Write RPC function id.
    fun::LiteFormat::Write(RpcMessage__, RpcIds::ShutdownTcp);

    // Write RPC header.
    fun::RpcHeader::WriteOk(RpcMessage__);

    // Write RPC function arguments.
    fun::FlexFormat::WriteBytes(RpcMessage__, comment);

    if (notify_send_by_proxy_enabled_ && !engine_specific_only_)
    {
      fun::String ArgsStr__ = TEXT("{");
      ArgsStr__ << TEXT("\"comment\":") << fun::ToString(comment);
      ArgsStr__ << TEXT("}");

      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::ShutdownTcp(), RpcIds::ShutdownTcp, *ArgsStr__);
    }
    else
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::ShutdownTcp(), RpcIds::ShutdownTcp, nullptr);
    }
  }

  bool Proxy::ShutdownTcpHandshake(const fun::HostId* RpcRemoteIds, const fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt)
  {
    fun::MessageOut RpcMessage__;

    // Write RPC function id.
    fun::LiteFormat::Write(RpcMessage__, RpcIds::ShutdownTcpHandshake);

    // Write RPC header.
    fun::RpcHeader::WriteOk(RpcMessage__);

    // No RPC function arguments.

    if (notify_send_by_proxy_enabled_ && !engine_specific_only_)
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::ShutdownTcpHandshake(), RpcIds::ShutdownTcpHandshake, TEXT("{}"));
    }
    else
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::ShutdownTcpHandshake(), RpcIds::ShutdownTcpHandshake, nullptr);
    }
  }

  bool Proxy::NotifyLog(const fun::HostId* RpcRemoteIds, const fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const fun::LogCategory& Category, const fun::String& text)
  {
    fun::MessageOut RpcMessage__;

    // Write RPC function id.
    fun::LiteFormat::Write(RpcMessage__, RpcIds::NotifyLog);

    // Write RPC header.
    fun::RpcHeader::WriteOk(RpcMessage__);

    // Write RPC function arguments.
    EngineTypes_UserTypeHandlers::Write(RpcMessage__, Category);
    fun::FlexFormat::WriteString(RpcMessage__, text);

    if (notify_send_by_proxy_enabled_ && !engine_specific_only_)
    {
      fun::String ArgsStr__ = TEXT("{");
      ArgsStr__ << TEXT("\"Category\":") << fun::ToString(Category);
      ArgsStr__ << TEXT(",\"text\":") << fun::ToString(text);
      ArgsStr__ << TEXT("}");

      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::NotifyLog(), RpcIds::NotifyLog, *ArgsStr__);
    }
    else
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::NotifyLog(), RpcIds::NotifyLog, nullptr);
    }
  }

  bool Proxy::NotifyLogHolepunchFreqFail(const fun::HostId* RpcRemoteIds, const fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const fun::int32 Rank, const fun::String& text)
  {
    fun::MessageOut RpcMessage__;

    // Write RPC function id.
    fun::LiteFormat::Write(RpcMessage__, RpcIds::NotifyLogHolepunchFreqFail);

    // Write RPC header.
    fun::RpcHeader::WriteOk(RpcMessage__);

    // Write RPC function arguments.
    fun::FlexFormat::WriteSInt32(RpcMessage__, Rank);
    fun::FlexFormat::WriteString(RpcMessage__, text);

    if (notify_send_by_proxy_enabled_ && !engine_specific_only_)
    {
      fun::String ArgsStr__ = TEXT("{");
      ArgsStr__ << TEXT("\"Rank\":") << fun::ToString(Rank);
      ArgsStr__ << TEXT(",\"text\":") << fun::ToString(text);
      ArgsStr__ << TEXT("}");

      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::NotifyLogHolepunchFreqFail(), RpcIds::NotifyLogHolepunchFreqFail, *ArgsStr__);
    }
    else
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::NotifyLogHolepunchFreqFail(), RpcIds::NotifyLogHolepunchFreqFail, nullptr);
    }
  }

  bool Proxy::NotifyNatDeviceName(const fun::HostId* RpcRemoteIds, const fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const fun::String& DeviceName)
  {
    fun::MessageOut RpcMessage__;

    // Write RPC function id.
    fun::LiteFormat::Write(RpcMessage__, RpcIds::NotifyNatDeviceName);

    // Write RPC header.
    fun::RpcHeader::WriteOk(RpcMessage__);

    // Write RPC function arguments.
    fun::FlexFormat::WriteString(RpcMessage__, DeviceName);

    if (notify_send_by_proxy_enabled_ && !engine_specific_only_)
    {
      fun::String ArgsStr__ = TEXT("{");
      ArgsStr__ << TEXT("\"DeviceName\":") << fun::ToString(DeviceName);
      ArgsStr__ << TEXT("}");

      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::NotifyNatDeviceName(), RpcIds::NotifyNatDeviceName, *ArgsStr__);
    }
    else
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::NotifyNatDeviceName(), RpcIds::NotifyNatDeviceName, nullptr);
    }
  }

  bool Proxy::NotifyPeerUdpSocketRestored(const fun::HostId* RpcRemoteIds, const fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& PeerBID)
  {
    fun::MessageOut RpcMessage__;

    // Write RPC function id.
    fun::LiteFormat::Write(RpcMessage__, RpcIds::NotifyPeerUdpSocketRestored);

    // Write RPC header.
    fun::RpcHeader::WriteOk(RpcMessage__);

    // Write RPC function arguments.
    EngineTypes_UserTypeHandlers::Write(RpcMessage__, PeerBID);

    if (notify_send_by_proxy_enabled_ && !engine_specific_only_)
    {
      fun::String ArgsStr__ = TEXT("{");
      ArgsStr__ << TEXT("\"PeerBID\":") << fun::ToString(PeerBID);
      ArgsStr__ << TEXT("}");

      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::NotifyPeerUdpSocketRestored(), RpcIds::NotifyPeerUdpSocketRestored, *ArgsStr__);
    }
    else
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::NotifyPeerUdpSocketRestored(), RpcIds::NotifyPeerUdpSocketRestored, nullptr);
    }
  }

  bool Proxy::NotifyJitDirectP2PTriggered(const fun::HostId* RpcRemoteIds, const fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& PeerBID)
  {
    fun::MessageOut RpcMessage__;

    // Write RPC function id.
    fun::LiteFormat::Write(RpcMessage__, RpcIds::NotifyJitDirectP2PTriggered);

    // Write RPC header.
    fun::RpcHeader::WriteOk(RpcMessage__);

    // Write RPC function arguments.
    EngineTypes_UserTypeHandlers::Write(RpcMessage__, PeerBID);

    if (notify_send_by_proxy_enabled_ && !engine_specific_only_)
    {
      fun::String ArgsStr__ = TEXT("{");
      ArgsStr__ << TEXT("\"PeerBID\":") << fun::ToString(PeerBID);
      ArgsStr__ << TEXT("}");

      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::NotifyJitDirectP2PTriggered(), RpcIds::NotifyJitDirectP2PTriggered, *ArgsStr__);
    }
    else
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::NotifyJitDirectP2PTriggered(), RpcIds::NotifyJitDirectP2PTriggered, nullptr);
    }
  }

  bool Proxy::NotifyNatDeviceNameDetected(const fun::HostId* RpcRemoteIds, const fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const fun::String& DeviceName)
  {
    fun::MessageOut RpcMessage__;

    // Write RPC function id.
    fun::LiteFormat::Write(RpcMessage__, RpcIds::NotifyNatDeviceNameDetected);

    // Write RPC header.
    fun::RpcHeader::WriteOk(RpcMessage__);

    // Write RPC function arguments.
    fun::FlexFormat::WriteString(RpcMessage__, DeviceName);

    if (notify_send_by_proxy_enabled_ && !engine_specific_only_)
    {
      fun::String ArgsStr__ = TEXT("{");
      ArgsStr__ << TEXT("\"DeviceName\":") << fun::ToString(DeviceName);
      ArgsStr__ << TEXT("}");

      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::NotifyNatDeviceNameDetected(), RpcIds::NotifyNatDeviceNameDetected, *ArgsStr__);
    }
    else
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::NotifyNatDeviceNameDetected(), RpcIds::NotifyNatDeviceNameDetected, nullptr);
    }
  }

  bool Proxy::NotifySendSpeed(const fun::HostId* RpcRemoteIds, const fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const double Speed)
  {
    fun::MessageOut RpcMessage__;

    // Write RPC function id.
    fun::LiteFormat::Write(RpcMessage__, RpcIds::NotifySendSpeed);

    // Write RPC header.
    fun::RpcHeader::WriteOk(RpcMessage__);

    // Write RPC function arguments.
    fun::FlexFormat::WriteDouble(RpcMessage__, Speed);

    if (notify_send_by_proxy_enabled_ && !engine_specific_only_)
    {
      fun::String ArgsStr__ = TEXT("{");
      ArgsStr__ << TEXT("\"Speed\":") << fun::ToString(Speed);
      ArgsStr__ << TEXT("}");

      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::NotifySendSpeed(), RpcIds::NotifySendSpeed, *ArgsStr__);
    }
    else
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::NotifySendSpeed(), RpcIds::NotifySendSpeed, nullptr);
    }
  }

  bool Proxy::ReportP2PPeerPing(const fun::HostId* RpcRemoteIds, const fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& peer_id, const fun::uint32 recent_ping)
  {
    fun::MessageOut RpcMessage__;

    // Write RPC function id.
    fun::LiteFormat::Write(RpcMessage__, RpcIds::ReportP2PPeerPing);

    // Write RPC header.
    fun::RpcHeader::WriteOk(RpcMessage__);

    // Write RPC function arguments.
    EngineTypes_UserTypeHandlers::Write(RpcMessage__, peer_id);
    fun::FlexFormat::WriteUInt32(RpcMessage__, recent_ping);

    if (notify_send_by_proxy_enabled_ && !engine_specific_only_)
    {
      fun::String ArgsStr__ = TEXT("{");
      ArgsStr__ << TEXT("\"peer_id\":") << fun::ToString(peer_id);
      ArgsStr__ << TEXT(",\"recent_ping\":") << fun::ToString(recent_ping);
      ArgsStr__ << TEXT("}");

      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::ReportP2PPeerPing(), RpcIds::ReportP2PPeerPing, *ArgsStr__);
    }
    else
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::ReportP2PPeerPing(), RpcIds::ReportP2PPeerPing, nullptr);
    }
  }

  bool Proxy::C2S_RequestCreateUdpSocket(const fun::HostId* RpcRemoteIds, const fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt)
  {
    fun::MessageOut RpcMessage__;

    // Write RPC function id.
    fun::LiteFormat::Write(RpcMessage__, RpcIds::C2S_RequestCreateUdpSocket);

    // Write RPC header.
    fun::RpcHeader::WriteOk(RpcMessage__);

    // No RPC function arguments.

    if (notify_send_by_proxy_enabled_ && !engine_specific_only_)
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::C2S_RequestCreateUdpSocket(), RpcIds::C2S_RequestCreateUdpSocket, TEXT("{}"));
    }
    else
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::C2S_RequestCreateUdpSocket(), RpcIds::C2S_RequestCreateUdpSocket, nullptr);
    }
  }

  bool Proxy::C2S_CreateUdpSocketAck(const fun::HostId* RpcRemoteIds, const fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const bool bOk)
  {
    fun::MessageOut RpcMessage__;

    // Write RPC function id.
    fun::LiteFormat::Write(RpcMessage__, RpcIds::C2S_CreateUdpSocketAck);

    // Write RPC header.
    fun::RpcHeader::WriteOk(RpcMessage__);

    // Write RPC function arguments.
    fun::FlexFormat::WriteBool(RpcMessage__, bOk);

    if (notify_send_by_proxy_enabled_ && !engine_specific_only_)
    {
      fun::String ArgsStr__ = TEXT("{");
      ArgsStr__ << TEXT("\"bOk\":") << fun::ToString(bOk);
      ArgsStr__ << TEXT("}");

      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::C2S_CreateUdpSocketAck(), RpcIds::C2S_CreateUdpSocketAck, *ArgsStr__);
    }
    else
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::C2S_CreateUdpSocketAck(), RpcIds::C2S_CreateUdpSocketAck, nullptr);
    }
  }

  bool Proxy::ReportC2CUdpMessageCount(const fun::HostId* RpcRemoteIds, const fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& peer_id, const fun::uint32 udp_message_attempt_count, const fun::uint32 udp_message_success_count)
  {
    fun::MessageOut RpcMessage__;

    // Write RPC function id.
    fun::LiteFormat::Write(RpcMessage__, RpcIds::ReportC2CUdpMessageCount);

    // Write RPC header.
    fun::RpcHeader::WriteOk(RpcMessage__);

    // Write RPC function arguments.
    EngineTypes_UserTypeHandlers::Write(RpcMessage__, peer_id);
    fun::FlexFormat::WriteUInt32(RpcMessage__, udp_message_attempt_count);
    fun::FlexFormat::WriteUInt32(RpcMessage__, udp_message_success_count);

    if (notify_send_by_proxy_enabled_ && !engine_specific_only_)
    {
      fun::String ArgsStr__ = TEXT("{");
      ArgsStr__ << TEXT("\"peer_id\":") << fun::ToString(peer_id);
      ArgsStr__ << TEXT(",\"udp_message_attempt_count\":") << fun::ToString(udp_message_attempt_count);
      ArgsStr__ << TEXT(",\"udp_message_success_count\":") << fun::ToString(udp_message_success_count);
      ArgsStr__ << TEXT("}");

      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::ReportC2CUdpMessageCount(), RpcIds::ReportC2CUdpMessageCount, *ArgsStr__);
    }
    else
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::ReportC2CUdpMessageCount(), RpcIds::ReportC2CUdpMessageCount, nullptr);
    }
  }

  bool Proxy::ReportC2SUdpMessageTrialCount(const fun::HostId* RpcRemoteIds, const fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const fun::uint32 to_server_udp_attempt_count)
  {
    fun::MessageOut RpcMessage__;

    // Write RPC function id.
    fun::LiteFormat::Write(RpcMessage__, RpcIds::ReportC2SUdpMessageTrialCount);

    // Write RPC header.
    fun::RpcHeader::WriteOk(RpcMessage__);

    // Write RPC function arguments.
    fun::FlexFormat::WriteUInt32(RpcMessage__, to_server_udp_attempt_count);

    if (notify_send_by_proxy_enabled_ && !engine_specific_only_)
    {
      fun::String ArgsStr__ = TEXT("{");
      ArgsStr__ << TEXT("\"to_server_udp_attempt_count\":") << fun::ToString(to_server_udp_attempt_count);
      ArgsStr__ << TEXT("}");

      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::ReportC2SUdpMessageTrialCount(), RpcIds::ReportC2SUdpMessageTrialCount, *ArgsStr__);
    }
    else
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::ReportC2SUdpMessageTrialCount(), RpcIds::ReportC2SUdpMessageTrialCount, nullptr);
    }
  }
} // end of namespace NetC2S

} // end of namespace fun
