﻿/**
 * Auto-generated by IDL Compiler (2.1.3)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 */
#include "lan_LanC2S_proxy.h"
#include "lan.h" // _Args, _PArgs

namespace fun {
namespace LanC2S
{
  bool Proxy::ReliablePing(const fun::HostId* RpcRemoteIds, const fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt)
  {
    fun::MessageOut RpcMessage__;

    // Write RPC function id.
    fun::LiteFormat::Write(RpcMessage__, RpcIds::ReliablePing);

    // Write RPC header.
    fun::RpcHeader::WriteOk(RpcMessage__);

    // No RPC function arguments.

    if (notify_send_by_proxy_enabled_ && !engine_specific_only_)
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::ReliablePing(), RpcIds::ReliablePing, TEXT("{}"));
    }
    else
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::ReliablePing(), RpcIds::ReliablePing, nullptr);
    }
  }

  bool Proxy::P2PGroup_MemberJoin_Ack(const fun::HostId* RpcRemoteIds, const fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& group_id, const fun::HostId& added_member_id, const fun::uint32 event_id)
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

    if (notify_send_by_proxy_enabled_ && !engine_specific_only_)
    {
      fun::String ArgsStr__ = TEXT("{");
      ArgsStr__ << TEXT("\"group_id\":") << fun::ToString(group_id);
      ArgsStr__ << TEXT(",\"added_member_id\":") << fun::ToString(added_member_id);
      ArgsStr__ << TEXT(",\"event_id\":") << fun::ToString(event_id);
      ArgsStr__ << TEXT("}");

      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::P2PGroup_MemberJoin_Ack(), RpcIds::P2PGroup_MemberJoin_Ack, *ArgsStr__);
    }
    else
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, rpc_call_opt, RpcMessage__, RpcNames::P2PGroup_MemberJoin_Ack(), RpcIds::P2PGroup_MemberJoin_Ack, nullptr);
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
} // end of namespace LanC2S

} // end of namespace fun