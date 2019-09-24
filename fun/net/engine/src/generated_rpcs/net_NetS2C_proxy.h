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
namespace NetS2C {
  
  class Proxy : public fun::RpcProxy {
   public:
    fun::int32 GetDeclaredRpcCount() const override { return NetS2C::NumRpcFunctions; }
    const fun::RpcId* GetDeclaredRpcIds() const override { return NetS2C::RpcIds::RpcIdList(); }
    const char** GetDeclaredRpcNames() const override { return NetS2C::RpcNames::RpcNameList(); }

    inline bool P2PGroup_MemberJoin(fun::HostId rpc_recvfrom, const fun::HostId& group_id, const fun::HostId& member_id, const fun::ByteArray& custom_field, const fun::uint32 event_id, const fun::ByteArray& P2PAESSessionKey, const fun::ByteArray& P2PRC4SessionKey, const fun::FrameNumber& p2p_first_frame_number, const fun::Uuid& ConnectionTag, const bool direct_p2p_enabled, const fun::int32 BindPort) { return P2PGroup_MemberJoin(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable, group_id, member_id, custom_field, event_id, P2PAESSessionKey, P2PRC4SessionKey, p2p_first_frame_number, ConnectionTag, direct_p2p_enabled, BindPort); }
    inline bool P2PGroup_MemberJoin(fun::HostId rpc_recvfrom, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& group_id, const fun::HostId& member_id, const fun::ByteArray& custom_field, const fun::uint32 event_id, const fun::ByteArray& P2PAESSessionKey, const fun::ByteArray& P2PRC4SessionKey, const fun::FrameNumber& p2p_first_frame_number, const fun::Uuid& ConnectionTag, const bool direct_p2p_enabled, const fun::int32 BindPort) { return P2PGroup_MemberJoin(&rpc_recvfrom, 1, rpc_call_opt, group_id, member_id, custom_field, event_id, P2PAESSessionKey, P2PRC4SessionKey, p2p_first_frame_number, ConnectionTag, direct_p2p_enabled, BindPort); }
    template <typename Allocator>
    inline bool P2PGroup_MemberJoin(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::HostId& group_id, const fun::HostId& member_id, const fun::ByteArray& custom_field, const fun::uint32 event_id, const fun::ByteArray& P2PAESSessionKey, const fun::ByteArray& P2PRC4SessionKey, const fun::FrameNumber& p2p_first_frame_number, const fun::Uuid& ConnectionTag, const bool direct_p2p_enabled, const fun::int32 BindPort) { return P2PGroup_MemberJoin(RpcRemoteIds, fun::RpcCallOption::Reliable, group_id, member_id, custom_field, event_id, P2PAESSessionKey, P2PRC4SessionKey, p2p_first_frame_number, ConnectionTag, direct_p2p_enabled, BindPort); }
    template <typename Allocator>
    inline bool P2PGroup_MemberJoin(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& group_id, const fun::HostId& member_id, const fun::ByteArray& custom_field, const fun::uint32 event_id, const fun::ByteArray& P2PAESSessionKey, const fun::ByteArray& P2PRC4SessionKey, const fun::FrameNumber& p2p_first_frame_number, const fun::Uuid& ConnectionTag, const bool direct_p2p_enabled, const fun::int32 BindPort) { return P2PGroup_MemberJoin(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), rpc_call_opt, group_id, member_id, custom_field, event_id, P2PAESSessionKey, P2PRC4SessionKey, p2p_first_frame_number, ConnectionTag, direct_p2p_enabled, BindPort); }
    inline bool P2PGroup_MemberJoin(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::HostId& group_id, const fun::HostId& member_id, const fun::ByteArray& custom_field, const fun::uint32 event_id, const fun::ByteArray& P2PAESSessionKey, const fun::ByteArray& P2PRC4SessionKey, const fun::FrameNumber& p2p_first_frame_number, const fun::Uuid& ConnectionTag, const bool direct_p2p_enabled, const fun::int32 BindPort) { return P2PGroup_MemberJoin(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable, group_id, member_id, custom_field, event_id, P2PAESSessionKey, P2PRC4SessionKey, p2p_first_frame_number, ConnectionTag, direct_p2p_enabled, BindPort); }
    bool P2PGroup_MemberJoin(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& group_id, const fun::HostId& member_id, const fun::ByteArray& custom_field, const fun::uint32 event_id, const fun::ByteArray& P2PAESSessionKey, const fun::ByteArray& P2PRC4SessionKey, const fun::FrameNumber& p2p_first_frame_number, const fun::Uuid& ConnectionTag, const bool direct_p2p_enabled, const fun::int32 BindPort);

    inline bool P2PGroup_MemberJoin_Unencrypted(fun::HostId rpc_recvfrom, const fun::HostId& group_id, const fun::HostId& member_id, const fun::ByteArray& custom_field, const fun::uint32 event_id, const fun::FrameNumber& p2p_first_frame_number, const fun::Uuid& ConnectionTag, const bool direct_p2p_enabled, const fun::int32 BindPort) { return P2PGroup_MemberJoin_Unencrypted(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable, group_id, member_id, custom_field, event_id, p2p_first_frame_number, ConnectionTag, direct_p2p_enabled, BindPort); }
    inline bool P2PGroup_MemberJoin_Unencrypted(fun::HostId rpc_recvfrom, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& group_id, const fun::HostId& member_id, const fun::ByteArray& custom_field, const fun::uint32 event_id, const fun::FrameNumber& p2p_first_frame_number, const fun::Uuid& ConnectionTag, const bool direct_p2p_enabled, const fun::int32 BindPort) { return P2PGroup_MemberJoin_Unencrypted(&rpc_recvfrom, 1, rpc_call_opt, group_id, member_id, custom_field, event_id, p2p_first_frame_number, ConnectionTag, direct_p2p_enabled, BindPort); }
    template <typename Allocator>
    inline bool P2PGroup_MemberJoin_Unencrypted(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::HostId& group_id, const fun::HostId& member_id, const fun::ByteArray& custom_field, const fun::uint32 event_id, const fun::FrameNumber& p2p_first_frame_number, const fun::Uuid& ConnectionTag, const bool direct_p2p_enabled, const fun::int32 BindPort) { return P2PGroup_MemberJoin_Unencrypted(RpcRemoteIds, fun::RpcCallOption::Reliable, group_id, member_id, custom_field, event_id, p2p_first_frame_number, ConnectionTag, direct_p2p_enabled, BindPort); }
    template <typename Allocator>
    inline bool P2PGroup_MemberJoin_Unencrypted(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& group_id, const fun::HostId& member_id, const fun::ByteArray& custom_field, const fun::uint32 event_id, const fun::FrameNumber& p2p_first_frame_number, const fun::Uuid& ConnectionTag, const bool direct_p2p_enabled, const fun::int32 BindPort) { return P2PGroup_MemberJoin_Unencrypted(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), rpc_call_opt, group_id, member_id, custom_field, event_id, p2p_first_frame_number, ConnectionTag, direct_p2p_enabled, BindPort); }
    inline bool P2PGroup_MemberJoin_Unencrypted(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::HostId& group_id, const fun::HostId& member_id, const fun::ByteArray& custom_field, const fun::uint32 event_id, const fun::FrameNumber& p2p_first_frame_number, const fun::Uuid& ConnectionTag, const bool direct_p2p_enabled, const fun::int32 BindPort) { return P2PGroup_MemberJoin_Unencrypted(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable, group_id, member_id, custom_field, event_id, p2p_first_frame_number, ConnectionTag, direct_p2p_enabled, BindPort); }
    bool P2PGroup_MemberJoin_Unencrypted(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& group_id, const fun::HostId& member_id, const fun::ByteArray& custom_field, const fun::uint32 event_id, const fun::FrameNumber& p2p_first_frame_number, const fun::Uuid& ConnectionTag, const bool direct_p2p_enabled, const fun::int32 BindPort);

    inline bool P2PRecycleComplete(fun::HostId rpc_recvfrom, const fun::HostId& peer_id, const bool bRecycled, const fun::InetAddress internal_addr, const fun::InetAddress external_addr, const fun::InetAddress SendAddr, const fun::InetAddress RecvAddr) { return P2PRecycleComplete(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable, peer_id, bRecycled, internal_addr, external_addr, SendAddr, RecvAddr); }
    inline bool P2PRecycleComplete(fun::HostId rpc_recvfrom, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& peer_id, const bool bRecycled, const fun::InetAddress internal_addr, const fun::InetAddress external_addr, const fun::InetAddress SendAddr, const fun::InetAddress RecvAddr) { return P2PRecycleComplete(&rpc_recvfrom, 1, rpc_call_opt, peer_id, bRecycled, internal_addr, external_addr, SendAddr, RecvAddr); }
    template <typename Allocator>
    inline bool P2PRecycleComplete(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::HostId& peer_id, const bool bRecycled, const fun::InetAddress internal_addr, const fun::InetAddress external_addr, const fun::InetAddress SendAddr, const fun::InetAddress RecvAddr) { return P2PRecycleComplete(RpcRemoteIds, fun::RpcCallOption::Reliable, peer_id, bRecycled, internal_addr, external_addr, SendAddr, RecvAddr); }
    template <typename Allocator>
    inline bool P2PRecycleComplete(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& peer_id, const bool bRecycled, const fun::InetAddress internal_addr, const fun::InetAddress external_addr, const fun::InetAddress SendAddr, const fun::InetAddress RecvAddr) { return P2PRecycleComplete(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), rpc_call_opt, peer_id, bRecycled, internal_addr, external_addr, SendAddr, RecvAddr); }
    inline bool P2PRecycleComplete(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::HostId& peer_id, const bool bRecycled, const fun::InetAddress internal_addr, const fun::InetAddress external_addr, const fun::InetAddress SendAddr, const fun::InetAddress RecvAddr) { return P2PRecycleComplete(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable, peer_id, bRecycled, internal_addr, external_addr, SendAddr, RecvAddr); }
    bool P2PRecycleComplete(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& peer_id, const bool bRecycled, const fun::InetAddress internal_addr, const fun::InetAddress external_addr, const fun::InetAddress SendAddr, const fun::InetAddress RecvAddr);

    inline bool RequestP2PHolepunch(fun::HostId rpc_recvfrom, const fun::HostId& peer_id, const fun::InetAddress internal_addr, const fun::InetAddress external_addr) { return RequestP2PHolepunch(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable, peer_id, internal_addr, external_addr); }
    inline bool RequestP2PHolepunch(fun::HostId rpc_recvfrom, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& peer_id, const fun::InetAddress internal_addr, const fun::InetAddress external_addr) { return RequestP2PHolepunch(&rpc_recvfrom, 1, rpc_call_opt, peer_id, internal_addr, external_addr); }
    template <typename Allocator>
    inline bool RequestP2PHolepunch(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::HostId& peer_id, const fun::InetAddress internal_addr, const fun::InetAddress external_addr) { return RequestP2PHolepunch(RpcRemoteIds, fun::RpcCallOption::Reliable, peer_id, internal_addr, external_addr); }
    template <typename Allocator>
    inline bool RequestP2PHolepunch(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& peer_id, const fun::InetAddress internal_addr, const fun::InetAddress external_addr) { return RequestP2PHolepunch(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), rpc_call_opt, peer_id, internal_addr, external_addr); }
    inline bool RequestP2PHolepunch(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::HostId& peer_id, const fun::InetAddress internal_addr, const fun::InetAddress external_addr) { return RequestP2PHolepunch(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable, peer_id, internal_addr, external_addr); }
    bool RequestP2PHolepunch(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& peer_id, const fun::InetAddress internal_addr, const fun::InetAddress external_addr);

    inline bool P2P_NotifyDirectP2PDisconnected2(fun::HostId rpc_recvfrom, const fun::HostId& peer_id, const fun::ResultCode& reason) { return P2P_NotifyDirectP2PDisconnected2(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable, peer_id, reason); }
    inline bool P2P_NotifyDirectP2PDisconnected2(fun::HostId rpc_recvfrom, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& peer_id, const fun::ResultCode& reason) { return P2P_NotifyDirectP2PDisconnected2(&rpc_recvfrom, 1, rpc_call_opt, peer_id, reason); }
    template <typename Allocator>
    inline bool P2P_NotifyDirectP2PDisconnected2(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::HostId& peer_id, const fun::ResultCode& reason) { return P2P_NotifyDirectP2PDisconnected2(RpcRemoteIds, fun::RpcCallOption::Reliable, peer_id, reason); }
    template <typename Allocator>
    inline bool P2P_NotifyDirectP2PDisconnected2(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& peer_id, const fun::ResultCode& reason) { return P2P_NotifyDirectP2PDisconnected2(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), rpc_call_opt, peer_id, reason); }
    inline bool P2P_NotifyDirectP2PDisconnected2(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::HostId& peer_id, const fun::ResultCode& reason) { return P2P_NotifyDirectP2PDisconnected2(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable, peer_id, reason); }
    bool P2P_NotifyDirectP2PDisconnected2(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& peer_id, const fun::ResultCode& reason);

    inline bool P2PGroup_MemberLeave(fun::HostId rpc_recvfrom, const fun::HostId& member_id, const fun::HostId& group_id) { return P2PGroup_MemberLeave(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable, member_id, group_id); }
    inline bool P2PGroup_MemberLeave(fun::HostId rpc_recvfrom, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& member_id, const fun::HostId& group_id) { return P2PGroup_MemberLeave(&rpc_recvfrom, 1, rpc_call_opt, member_id, group_id); }
    template <typename Allocator>
    inline bool P2PGroup_MemberLeave(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::HostId& member_id, const fun::HostId& group_id) { return P2PGroup_MemberLeave(RpcRemoteIds, fun::RpcCallOption::Reliable, member_id, group_id); }
    template <typename Allocator>
    inline bool P2PGroup_MemberLeave(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& member_id, const fun::HostId& group_id) { return P2PGroup_MemberLeave(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), rpc_call_opt, member_id, group_id); }
    inline bool P2PGroup_MemberLeave(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::HostId& member_id, const fun::HostId& group_id) { return P2PGroup_MemberLeave(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable, member_id, group_id); }
    bool P2PGroup_MemberLeave(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& member_id, const fun::HostId& group_id);

    inline bool NotifyDirectP2PEstablish(fun::HostId rpc_recvfrom, const fun::HostId& A0, const fun::HostId& B0, const fun::InetAddress X0, const fun::InetAddress Y0, const fun::InetAddress Z0, const fun::InetAddress W0) { return NotifyDirectP2PEstablish(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable, A0, B0, X0, Y0, Z0, W0); }
    inline bool NotifyDirectP2PEstablish(fun::HostId rpc_recvfrom, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& A0, const fun::HostId& B0, const fun::InetAddress X0, const fun::InetAddress Y0, const fun::InetAddress Z0, const fun::InetAddress W0) { return NotifyDirectP2PEstablish(&rpc_recvfrom, 1, rpc_call_opt, A0, B0, X0, Y0, Z0, W0); }
    template <typename Allocator>
    inline bool NotifyDirectP2PEstablish(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::HostId& A0, const fun::HostId& B0, const fun::InetAddress X0, const fun::InetAddress Y0, const fun::InetAddress Z0, const fun::InetAddress W0) { return NotifyDirectP2PEstablish(RpcRemoteIds, fun::RpcCallOption::Reliable, A0, B0, X0, Y0, Z0, W0); }
    template <typename Allocator>
    inline bool NotifyDirectP2PEstablish(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& A0, const fun::HostId& B0, const fun::InetAddress X0, const fun::InetAddress Y0, const fun::InetAddress Z0, const fun::InetAddress W0) { return NotifyDirectP2PEstablish(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), rpc_call_opt, A0, B0, X0, Y0, Z0, W0); }
    inline bool NotifyDirectP2PEstablish(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::HostId& A0, const fun::HostId& B0, const fun::InetAddress X0, const fun::InetAddress Y0, const fun::InetAddress Z0, const fun::InetAddress W0) { return NotifyDirectP2PEstablish(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable, A0, B0, X0, Y0, Z0, W0); }
    bool NotifyDirectP2PEstablish(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& A0, const fun::HostId& B0, const fun::InetAddress X0, const fun::InetAddress Y0, const fun::InetAddress Z0, const fun::InetAddress W0);

    inline bool ReliablePong(fun::HostId rpc_recvfrom) { return ReliablePong(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable); }
    inline bool ReliablePong(fun::HostId rpc_recvfrom, const fun::RpcCallOption& rpc_call_opt) { return ReliablePong(&rpc_recvfrom, 1, rpc_call_opt); }
    template <typename Allocator>
    inline bool ReliablePong(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds) { return ReliablePong(RpcRemoteIds, fun::RpcCallOption::Reliable); }
    template <typename Allocator>
    inline bool ReliablePong(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& rpc_call_opt) { return ReliablePong(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), rpc_call_opt); }
    inline bool ReliablePong(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount) { return ReliablePong(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable); }
    bool ReliablePong(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt);

    inline bool EnableIntraLogging(fun::HostId rpc_recvfrom) { return EnableIntraLogging(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable); }
    inline bool EnableIntraLogging(fun::HostId rpc_recvfrom, const fun::RpcCallOption& rpc_call_opt) { return EnableIntraLogging(&rpc_recvfrom, 1, rpc_call_opt); }
    template <typename Allocator>
    inline bool EnableIntraLogging(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds) { return EnableIntraLogging(RpcRemoteIds, fun::RpcCallOption::Reliable); }
    template <typename Allocator>
    inline bool EnableIntraLogging(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& rpc_call_opt) { return EnableIntraLogging(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), rpc_call_opt); }
    inline bool EnableIntraLogging(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount) { return EnableIntraLogging(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable); }
    bool EnableIntraLogging(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt);

    inline bool DisableIntraLogging(fun::HostId rpc_recvfrom) { return DisableIntraLogging(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable); }
    inline bool DisableIntraLogging(fun::HostId rpc_recvfrom, const fun::RpcCallOption& rpc_call_opt) { return DisableIntraLogging(&rpc_recvfrom, 1, rpc_call_opt); }
    template <typename Allocator>
    inline bool DisableIntraLogging(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds) { return DisableIntraLogging(RpcRemoteIds, fun::RpcCallOption::Reliable); }
    template <typename Allocator>
    inline bool DisableIntraLogging(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& rpc_call_opt) { return DisableIntraLogging(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), rpc_call_opt); }
    inline bool DisableIntraLogging(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount) { return DisableIntraLogging(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable); }
    bool DisableIntraLogging(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt);

    inline bool NotifyUdpToTcpFallbackByServer(fun::HostId rpc_recvfrom) { return NotifyUdpToTcpFallbackByServer(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable); }
    inline bool NotifyUdpToTcpFallbackByServer(fun::HostId rpc_recvfrom, const fun::RpcCallOption& rpc_call_opt) { return NotifyUdpToTcpFallbackByServer(&rpc_recvfrom, 1, rpc_call_opt); }
    template <typename Allocator>
    inline bool NotifyUdpToTcpFallbackByServer(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds) { return NotifyUdpToTcpFallbackByServer(RpcRemoteIds, fun::RpcCallOption::Reliable); }
    template <typename Allocator>
    inline bool NotifyUdpToTcpFallbackByServer(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& rpc_call_opt) { return NotifyUdpToTcpFallbackByServer(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), rpc_call_opt); }
    inline bool NotifyUdpToTcpFallbackByServer(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount) { return NotifyUdpToTcpFallbackByServer(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable); }
    bool NotifyUdpToTcpFallbackByServer(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt);

    inline bool NotifySpeedHackDetectorEnabled(fun::HostId rpc_recvfrom, const bool bEnabled) { return NotifySpeedHackDetectorEnabled(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable, bEnabled); }
    inline bool NotifySpeedHackDetectorEnabled(fun::HostId rpc_recvfrom, const fun::RpcCallOption& rpc_call_opt, const bool bEnabled) { return NotifySpeedHackDetectorEnabled(&rpc_recvfrom, 1, rpc_call_opt, bEnabled); }
    template <typename Allocator>
    inline bool NotifySpeedHackDetectorEnabled(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const bool bEnabled) { return NotifySpeedHackDetectorEnabled(RpcRemoteIds, fun::RpcCallOption::Reliable, bEnabled); }
    template <typename Allocator>
    inline bool NotifySpeedHackDetectorEnabled(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& rpc_call_opt, const bool bEnabled) { return NotifySpeedHackDetectorEnabled(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), rpc_call_opt, bEnabled); }
    inline bool NotifySpeedHackDetectorEnabled(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const bool bEnabled) { return NotifySpeedHackDetectorEnabled(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable, bEnabled); }
    bool NotifySpeedHackDetectorEnabled(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const bool bEnabled);

    inline bool ShutdownTcpAck(fun::HostId rpc_recvfrom) { return ShutdownTcpAck(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable); }
    inline bool ShutdownTcpAck(fun::HostId rpc_recvfrom, const fun::RpcCallOption& rpc_call_opt) { return ShutdownTcpAck(&rpc_recvfrom, 1, rpc_call_opt); }
    template <typename Allocator>
    inline bool ShutdownTcpAck(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds) { return ShutdownTcpAck(RpcRemoteIds, fun::RpcCallOption::Reliable); }
    template <typename Allocator>
    inline bool ShutdownTcpAck(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& rpc_call_opt) { return ShutdownTcpAck(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), rpc_call_opt); }
    inline bool ShutdownTcpAck(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount) { return ShutdownTcpAck(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable); }
    bool ShutdownTcpAck(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt);

    inline bool RequestAutoPrune(fun::HostId rpc_recvfrom) { return RequestAutoPrune(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable); }
    inline bool RequestAutoPrune(fun::HostId rpc_recvfrom, const fun::RpcCallOption& rpc_call_opt) { return RequestAutoPrune(&rpc_recvfrom, 1, rpc_call_opt); }
    template <typename Allocator>
    inline bool RequestAutoPrune(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds) { return RequestAutoPrune(RpcRemoteIds, fun::RpcCallOption::Reliable); }
    template <typename Allocator>
    inline bool RequestAutoPrune(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& rpc_call_opt) { return RequestAutoPrune(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), rpc_call_opt); }
    inline bool RequestAutoPrune(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount) { return RequestAutoPrune(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable); }
    bool RequestAutoPrune(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt);

    inline bool RenewP2PConnectionState(fun::HostId rpc_recvfrom, const fun::HostId& peer_id) { return RenewP2PConnectionState(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable, peer_id); }
    inline bool RenewP2PConnectionState(fun::HostId rpc_recvfrom, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& peer_id) { return RenewP2PConnectionState(&rpc_recvfrom, 1, rpc_call_opt, peer_id); }
    template <typename Allocator>
    inline bool RenewP2PConnectionState(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::HostId& peer_id) { return RenewP2PConnectionState(RpcRemoteIds, fun::RpcCallOption::Reliable, peer_id); }
    template <typename Allocator>
    inline bool RenewP2PConnectionState(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& peer_id) { return RenewP2PConnectionState(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), rpc_call_opt, peer_id); }
    inline bool RenewP2PConnectionState(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::HostId& peer_id) { return RenewP2PConnectionState(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable, peer_id); }
    bool RenewP2PConnectionState(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& peer_id);

    inline bool NewDirectP2PConnection(fun::HostId rpc_recvfrom, const fun::HostId& peer_id) { return NewDirectP2PConnection(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable, peer_id); }
    inline bool NewDirectP2PConnection(fun::HostId rpc_recvfrom, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& peer_id) { return NewDirectP2PConnection(&rpc_recvfrom, 1, rpc_call_opt, peer_id); }
    template <typename Allocator>
    inline bool NewDirectP2PConnection(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::HostId& peer_id) { return NewDirectP2PConnection(RpcRemoteIds, fun::RpcCallOption::Reliable, peer_id); }
    template <typename Allocator>
    inline bool NewDirectP2PConnection(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& peer_id) { return NewDirectP2PConnection(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), rpc_call_opt, peer_id); }
    inline bool NewDirectP2PConnection(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::HostId& peer_id) { return NewDirectP2PConnection(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable, peer_id); }
    bool NewDirectP2PConnection(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& peer_id);

    inline bool RequestMeasureSendSpeed(fun::HostId rpc_recvfrom, const bool bEnabled) { return RequestMeasureSendSpeed(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable, bEnabled); }
    inline bool RequestMeasureSendSpeed(fun::HostId rpc_recvfrom, const fun::RpcCallOption& rpc_call_opt, const bool bEnabled) { return RequestMeasureSendSpeed(&rpc_recvfrom, 1, rpc_call_opt, bEnabled); }
    template <typename Allocator>
    inline bool RequestMeasureSendSpeed(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const bool bEnabled) { return RequestMeasureSendSpeed(RpcRemoteIds, fun::RpcCallOption::Reliable, bEnabled); }
    template <typename Allocator>
    inline bool RequestMeasureSendSpeed(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& rpc_call_opt, const bool bEnabled) { return RequestMeasureSendSpeed(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), rpc_call_opt, bEnabled); }
    inline bool RequestMeasureSendSpeed(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const bool bEnabled) { return RequestMeasureSendSpeed(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable, bEnabled); }
    bool RequestMeasureSendSpeed(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const bool bEnabled);

    inline bool S2C_RequestCreateUdpSocket(fun::HostId rpc_recvfrom, const fun::NamedInetAddress& ServerUdpAddr) { return S2C_RequestCreateUdpSocket(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable, ServerUdpAddr); }
    inline bool S2C_RequestCreateUdpSocket(fun::HostId rpc_recvfrom, const fun::RpcCallOption& rpc_call_opt, const fun::NamedInetAddress& ServerUdpAddr) { return S2C_RequestCreateUdpSocket(&rpc_recvfrom, 1, rpc_call_opt, ServerUdpAddr); }
    template <typename Allocator>
    inline bool S2C_RequestCreateUdpSocket(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::NamedInetAddress& ServerUdpAddr) { return S2C_RequestCreateUdpSocket(RpcRemoteIds, fun::RpcCallOption::Reliable, ServerUdpAddr); }
    template <typename Allocator>
    inline bool S2C_RequestCreateUdpSocket(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& rpc_call_opt, const fun::NamedInetAddress& ServerUdpAddr) { return S2C_RequestCreateUdpSocket(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), rpc_call_opt, ServerUdpAddr); }
    inline bool S2C_RequestCreateUdpSocket(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::NamedInetAddress& ServerUdpAddr) { return S2C_RequestCreateUdpSocket(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable, ServerUdpAddr); }
    bool S2C_RequestCreateUdpSocket(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const fun::NamedInetAddress& ServerUdpAddr);

    inline bool S2C_CreateUdpSocketAck(fun::HostId rpc_recvfrom, const bool bOk, const fun::NamedInetAddress& ServerUdpAddr) { return S2C_CreateUdpSocketAck(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable, bOk, ServerUdpAddr); }
    inline bool S2C_CreateUdpSocketAck(fun::HostId rpc_recvfrom, const fun::RpcCallOption& rpc_call_opt, const bool bOk, const fun::NamedInetAddress& ServerUdpAddr) { return S2C_CreateUdpSocketAck(&rpc_recvfrom, 1, rpc_call_opt, bOk, ServerUdpAddr); }
    template <typename Allocator>
    inline bool S2C_CreateUdpSocketAck(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const bool bOk, const fun::NamedInetAddress& ServerUdpAddr) { return S2C_CreateUdpSocketAck(RpcRemoteIds, fun::RpcCallOption::Reliable, bOk, ServerUdpAddr); }
    template <typename Allocator>
    inline bool S2C_CreateUdpSocketAck(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& rpc_call_opt, const bool bOk, const fun::NamedInetAddress& ServerUdpAddr) { return S2C_CreateUdpSocketAck(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), rpc_call_opt, bOk, ServerUdpAddr); }
    inline bool S2C_CreateUdpSocketAck(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const bool bOk, const fun::NamedInetAddress& ServerUdpAddr) { return S2C_CreateUdpSocketAck(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable, bOk, ServerUdpAddr); }
    bool S2C_CreateUdpSocketAck(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const bool bOk, const fun::NamedInetAddress& ServerUdpAddr);
  };
} // end of namespace NetS2C

} // end of namespace fun
