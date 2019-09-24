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
namespace NetC2S
{
  struct Proxy : public fun::RpcProxy
  {
    fun::int32 GetDeclaredRpcCount() const override { return NetC2S::NumRpcFunctions; }
    const fun::RpcId* GetDeclaredRpcIds() const override { return NetC2S::RpcIds::RpcIdList(); }
    const char** GetDeclaredRpcNames() const override { return NetC2S::RpcNames::RpcNameList(); }

    inline bool ReliablePing(fun::HostId rpc_recvfrom, const double recent_frame_rate) { return ReliablePing(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable, recent_frame_rate); }
    inline bool ReliablePing(fun::HostId rpc_recvfrom, const fun::RpcCallOption& rpc_call_opt, const double recent_frame_rate) { return ReliablePing(&rpc_recvfrom, 1, rpc_call_opt, recent_frame_rate); }
    template <typename Allocator>
    inline bool ReliablePing(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const double recent_frame_rate) { return ReliablePing(RpcRemoteIds, fun::RpcCallOption::Reliable, recent_frame_rate); }
    template <typename Allocator>
    inline bool ReliablePing(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& rpc_call_opt, const double recent_frame_rate) { return ReliablePing(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), rpc_call_opt, recent_frame_rate); }
    inline bool ReliablePing(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const double recent_frame_rate) { return ReliablePing(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable, recent_frame_rate); }
    bool ReliablePing(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const double recent_frame_rate);

    inline bool P2P_NotifyDirectP2PDisconnected(fun::HostId rpc_recvfrom, const fun::HostId& peer_id, const fun::ResultCode& reason) { return P2P_NotifyDirectP2PDisconnected(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable, peer_id, reason); }
    inline bool P2P_NotifyDirectP2PDisconnected(fun::HostId rpc_recvfrom, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& peer_id, const fun::ResultCode& reason) { return P2P_NotifyDirectP2PDisconnected(&rpc_recvfrom, 1, rpc_call_opt, peer_id, reason); }
    template <typename Allocator>
    inline bool P2P_NotifyDirectP2PDisconnected(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::HostId& peer_id, const fun::ResultCode& reason) { return P2P_NotifyDirectP2PDisconnected(RpcRemoteIds, fun::RpcCallOption::Reliable, peer_id, reason); }
    template <typename Allocator>
    inline bool P2P_NotifyDirectP2PDisconnected(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& peer_id, const fun::ResultCode& reason) { return P2P_NotifyDirectP2PDisconnected(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), rpc_call_opt, peer_id, reason); }
    inline bool P2P_NotifyDirectP2PDisconnected(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::HostId& peer_id, const fun::ResultCode& reason) { return P2P_NotifyDirectP2PDisconnected(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable, peer_id, reason); }
    bool P2P_NotifyDirectP2PDisconnected(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& peer_id, const fun::ResultCode& reason);

    inline bool NotifyUdpToTcpFallbackByClient(fun::HostId rpc_recvfrom) { return NotifyUdpToTcpFallbackByClient(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable); }
    inline bool NotifyUdpToTcpFallbackByClient(fun::HostId rpc_recvfrom, const fun::RpcCallOption& rpc_call_opt) { return NotifyUdpToTcpFallbackByClient(&rpc_recvfrom, 1, rpc_call_opt); }
    template <typename Allocator>
    inline bool NotifyUdpToTcpFallbackByClient(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds) { return NotifyUdpToTcpFallbackByClient(RpcRemoteIds, fun::RpcCallOption::Reliable); }
    template <typename Allocator>
    inline bool NotifyUdpToTcpFallbackByClient(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& rpc_call_opt) { return NotifyUdpToTcpFallbackByClient(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), rpc_call_opt); }
    inline bool NotifyUdpToTcpFallbackByClient(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount) { return NotifyUdpToTcpFallbackByClient(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable); }
    bool NotifyUdpToTcpFallbackByClient(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt);

    inline bool P2PGroup_MemberJoin_Ack(fun::HostId rpc_recvfrom, const fun::HostId& group_id, const fun::HostId& added_member_id, const fun::uint32 event_id, const bool bLocalPortReuseOk) { return P2PGroup_MemberJoin_Ack(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable, group_id, added_member_id, event_id, bLocalPortReuseOk); }
    inline bool P2PGroup_MemberJoin_Ack(fun::HostId rpc_recvfrom, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& group_id, const fun::HostId& added_member_id, const fun::uint32 event_id, const bool bLocalPortReuseOk) { return P2PGroup_MemberJoin_Ack(&rpc_recvfrom, 1, rpc_call_opt, group_id, added_member_id, event_id, bLocalPortReuseOk); }
    template <typename Allocator>
    inline bool P2PGroup_MemberJoin_Ack(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::HostId& group_id, const fun::HostId& added_member_id, const fun::uint32 event_id, const bool bLocalPortReuseOk) { return P2PGroup_MemberJoin_Ack(RpcRemoteIds, fun::RpcCallOption::Reliable, group_id, added_member_id, event_id, bLocalPortReuseOk); }
    template <typename Allocator>
    inline bool P2PGroup_MemberJoin_Ack(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& group_id, const fun::HostId& added_member_id, const fun::uint32 event_id, const bool bLocalPortReuseOk) { return P2PGroup_MemberJoin_Ack(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), rpc_call_opt, group_id, added_member_id, event_id, bLocalPortReuseOk); }
    inline bool P2PGroup_MemberJoin_Ack(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::HostId& group_id, const fun::HostId& added_member_id, const fun::uint32 event_id, const bool bLocalPortReuseOk) { return P2PGroup_MemberJoin_Ack(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable, group_id, added_member_id, event_id, bLocalPortReuseOk); }
    bool P2PGroup_MemberJoin_Ack(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& group_id, const fun::HostId& added_member_id, const fun::uint32 event_id, const bool bLocalPortReuseOk);

    inline bool NotifyP2PHolepunchSuccess(fun::HostId rpc_recvfrom, const fun::HostId& A, const fun::HostId& B, const fun::InetAddress a2b_send_addr, const fun::InetAddress a2b_recv_addr, const fun::InetAddress b2a_send_addr, const fun::InetAddress b2a_recv_addr) { return NotifyP2PHolepunchSuccess(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable, A, B, a2b_send_addr, a2b_recv_addr, b2a_send_addr, b2a_recv_addr); }
    inline bool NotifyP2PHolepunchSuccess(fun::HostId rpc_recvfrom, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& A, const fun::HostId& B, const fun::InetAddress a2b_send_addr, const fun::InetAddress a2b_recv_addr, const fun::InetAddress b2a_send_addr, const fun::InetAddress b2a_recv_addr) { return NotifyP2PHolepunchSuccess(&rpc_recvfrom, 1, rpc_call_opt, A, B, a2b_send_addr, a2b_recv_addr, b2a_send_addr, b2a_recv_addr); }
    template <typename Allocator>
    inline bool NotifyP2PHolepunchSuccess(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::HostId& A, const fun::HostId& B, const fun::InetAddress a2b_send_addr, const fun::InetAddress a2b_recv_addr, const fun::InetAddress b2a_send_addr, const fun::InetAddress b2a_recv_addr) { return NotifyP2PHolepunchSuccess(RpcRemoteIds, fun::RpcCallOption::Reliable, A, B, a2b_send_addr, a2b_recv_addr, b2a_send_addr, b2a_recv_addr); }
    template <typename Allocator>
    inline bool NotifyP2PHolepunchSuccess(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& A, const fun::HostId& B, const fun::InetAddress a2b_send_addr, const fun::InetAddress a2b_recv_addr, const fun::InetAddress b2a_send_addr, const fun::InetAddress b2a_recv_addr) { return NotifyP2PHolepunchSuccess(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), rpc_call_opt, A, B, a2b_send_addr, a2b_recv_addr, b2a_send_addr, b2a_recv_addr); }
    inline bool NotifyP2PHolepunchSuccess(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::HostId& A, const fun::HostId& B, const fun::InetAddress a2b_send_addr, const fun::InetAddress a2b_recv_addr, const fun::InetAddress b2a_send_addr, const fun::InetAddress b2a_recv_addr) { return NotifyP2PHolepunchSuccess(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable, A, B, a2b_send_addr, a2b_recv_addr, b2a_send_addr, b2a_recv_addr); }
    bool NotifyP2PHolepunchSuccess(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& A, const fun::HostId& B, const fun::InetAddress a2b_send_addr, const fun::InetAddress a2b_recv_addr, const fun::InetAddress b2a_send_addr, const fun::InetAddress b2a_recv_addr);

    inline bool ShutdownTcp(fun::HostId rpc_recvfrom, const fun::ByteArray& comment) { return ShutdownTcp(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable, comment); }
    inline bool ShutdownTcp(fun::HostId rpc_recvfrom, const fun::RpcCallOption& rpc_call_opt, const fun::ByteArray& comment) { return ShutdownTcp(&rpc_recvfrom, 1, rpc_call_opt, comment); }
    template <typename Allocator>
    inline bool ShutdownTcp(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::ByteArray& comment) { return ShutdownTcp(RpcRemoteIds, fun::RpcCallOption::Reliable, comment); }
    template <typename Allocator>
    inline bool ShutdownTcp(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& rpc_call_opt, const fun::ByteArray& comment) { return ShutdownTcp(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), rpc_call_opt, comment); }
    inline bool ShutdownTcp(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::ByteArray& comment) { return ShutdownTcp(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable, comment); }
    bool ShutdownTcp(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const fun::ByteArray& comment);

    inline bool ShutdownTcpHandshake(fun::HostId rpc_recvfrom) { return ShutdownTcpHandshake(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable); }
    inline bool ShutdownTcpHandshake(fun::HostId rpc_recvfrom, const fun::RpcCallOption& rpc_call_opt) { return ShutdownTcpHandshake(&rpc_recvfrom, 1, rpc_call_opt); }
    template <typename Allocator>
    inline bool ShutdownTcpHandshake(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds) { return ShutdownTcpHandshake(RpcRemoteIds, fun::RpcCallOption::Reliable); }
    template <typename Allocator>
    inline bool ShutdownTcpHandshake(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& rpc_call_opt) { return ShutdownTcpHandshake(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), rpc_call_opt); }
    inline bool ShutdownTcpHandshake(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount) { return ShutdownTcpHandshake(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable); }
    bool ShutdownTcpHandshake(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt);

    inline bool NotifyLog(fun::HostId rpc_recvfrom, const fun::LogCategory& Category, const fun::String& text) { return NotifyLog(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable, Category, text); }
    inline bool NotifyLog(fun::HostId rpc_recvfrom, const fun::RpcCallOption& rpc_call_opt, const fun::LogCategory& Category, const fun::String& text) { return NotifyLog(&rpc_recvfrom, 1, rpc_call_opt, Category, text); }
    template <typename Allocator>
    inline bool NotifyLog(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::LogCategory& Category, const fun::String& text) { return NotifyLog(RpcRemoteIds, fun::RpcCallOption::Reliable, Category, text); }
    template <typename Allocator>
    inline bool NotifyLog(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& rpc_call_opt, const fun::LogCategory& Category, const fun::String& text) { return NotifyLog(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), rpc_call_opt, Category, text); }
    inline bool NotifyLog(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::LogCategory& Category, const fun::String& text) { return NotifyLog(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable, Category, text); }
    bool NotifyLog(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const fun::LogCategory& Category, const fun::String& text);

    inline bool NotifyLogHolepunchFreqFail(fun::HostId rpc_recvfrom, const fun::int32 Rank, const fun::String& text) { return NotifyLogHolepunchFreqFail(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable, Rank, text); }
    inline bool NotifyLogHolepunchFreqFail(fun::HostId rpc_recvfrom, const fun::RpcCallOption& rpc_call_opt, const fun::int32 Rank, const fun::String& text) { return NotifyLogHolepunchFreqFail(&rpc_recvfrom, 1, rpc_call_opt, Rank, text); }
    template <typename Allocator>
    inline bool NotifyLogHolepunchFreqFail(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::int32 Rank, const fun::String& text) { return NotifyLogHolepunchFreqFail(RpcRemoteIds, fun::RpcCallOption::Reliable, Rank, text); }
    template <typename Allocator>
    inline bool NotifyLogHolepunchFreqFail(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& rpc_call_opt, const fun::int32 Rank, const fun::String& text) { return NotifyLogHolepunchFreqFail(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), rpc_call_opt, Rank, text); }
    inline bool NotifyLogHolepunchFreqFail(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::int32 Rank, const fun::String& text) { return NotifyLogHolepunchFreqFail(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable, Rank, text); }
    bool NotifyLogHolepunchFreqFail(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const fun::int32 Rank, const fun::String& text);

    inline bool NotifyNatDeviceName(fun::HostId rpc_recvfrom, const fun::String& DeviceName) { return NotifyNatDeviceName(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable, DeviceName); }
    inline bool NotifyNatDeviceName(fun::HostId rpc_recvfrom, const fun::RpcCallOption& rpc_call_opt, const fun::String& DeviceName) { return NotifyNatDeviceName(&rpc_recvfrom, 1, rpc_call_opt, DeviceName); }
    template <typename Allocator>
    inline bool NotifyNatDeviceName(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::String& DeviceName) { return NotifyNatDeviceName(RpcRemoteIds, fun::RpcCallOption::Reliable, DeviceName); }
    template <typename Allocator>
    inline bool NotifyNatDeviceName(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& rpc_call_opt, const fun::String& DeviceName) { return NotifyNatDeviceName(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), rpc_call_opt, DeviceName); }
    inline bool NotifyNatDeviceName(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::String& DeviceName) { return NotifyNatDeviceName(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable, DeviceName); }
    bool NotifyNatDeviceName(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const fun::String& DeviceName);

    inline bool NotifyPeerUdpSocketRestored(fun::HostId rpc_recvfrom, const fun::HostId& PeerBID) { return NotifyPeerUdpSocketRestored(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable, PeerBID); }
    inline bool NotifyPeerUdpSocketRestored(fun::HostId rpc_recvfrom, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& PeerBID) { return NotifyPeerUdpSocketRestored(&rpc_recvfrom, 1, rpc_call_opt, PeerBID); }
    template <typename Allocator>
    inline bool NotifyPeerUdpSocketRestored(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::HostId& PeerBID) { return NotifyPeerUdpSocketRestored(RpcRemoteIds, fun::RpcCallOption::Reliable, PeerBID); }
    template <typename Allocator>
    inline bool NotifyPeerUdpSocketRestored(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& PeerBID) { return NotifyPeerUdpSocketRestored(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), rpc_call_opt, PeerBID); }
    inline bool NotifyPeerUdpSocketRestored(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::HostId& PeerBID) { return NotifyPeerUdpSocketRestored(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable, PeerBID); }
    bool NotifyPeerUdpSocketRestored(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& PeerBID);

    inline bool NotifyJitDirectP2PTriggered(fun::HostId rpc_recvfrom, const fun::HostId& PeerBID) { return NotifyJitDirectP2PTriggered(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable, PeerBID); }
    inline bool NotifyJitDirectP2PTriggered(fun::HostId rpc_recvfrom, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& PeerBID) { return NotifyJitDirectP2PTriggered(&rpc_recvfrom, 1, rpc_call_opt, PeerBID); }
    template <typename Allocator>
    inline bool NotifyJitDirectP2PTriggered(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::HostId& PeerBID) { return NotifyJitDirectP2PTriggered(RpcRemoteIds, fun::RpcCallOption::Reliable, PeerBID); }
    template <typename Allocator>
    inline bool NotifyJitDirectP2PTriggered(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& PeerBID) { return NotifyJitDirectP2PTriggered(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), rpc_call_opt, PeerBID); }
    inline bool NotifyJitDirectP2PTriggered(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::HostId& PeerBID) { return NotifyJitDirectP2PTriggered(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable, PeerBID); }
    bool NotifyJitDirectP2PTriggered(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& PeerBID);

    inline bool NotifyNatDeviceNameDetected(fun::HostId rpc_recvfrom, const fun::String& DeviceName) { return NotifyNatDeviceNameDetected(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable, DeviceName); }
    inline bool NotifyNatDeviceNameDetected(fun::HostId rpc_recvfrom, const fun::RpcCallOption& rpc_call_opt, const fun::String& DeviceName) { return NotifyNatDeviceNameDetected(&rpc_recvfrom, 1, rpc_call_opt, DeviceName); }
    template <typename Allocator>
    inline bool NotifyNatDeviceNameDetected(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::String& DeviceName) { return NotifyNatDeviceNameDetected(RpcRemoteIds, fun::RpcCallOption::Reliable, DeviceName); }
    template <typename Allocator>
    inline bool NotifyNatDeviceNameDetected(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& rpc_call_opt, const fun::String& DeviceName) { return NotifyNatDeviceNameDetected(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), rpc_call_opt, DeviceName); }
    inline bool NotifyNatDeviceNameDetected(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::String& DeviceName) { return NotifyNatDeviceNameDetected(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable, DeviceName); }
    bool NotifyNatDeviceNameDetected(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const fun::String& DeviceName);

    inline bool NotifySendSpeed(fun::HostId rpc_recvfrom, const double Speed) { return NotifySendSpeed(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable, Speed); }
    inline bool NotifySendSpeed(fun::HostId rpc_recvfrom, const fun::RpcCallOption& rpc_call_opt, const double Speed) { return NotifySendSpeed(&rpc_recvfrom, 1, rpc_call_opt, Speed); }
    template <typename Allocator>
    inline bool NotifySendSpeed(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const double Speed) { return NotifySendSpeed(RpcRemoteIds, fun::RpcCallOption::Reliable, Speed); }
    template <typename Allocator>
    inline bool NotifySendSpeed(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& rpc_call_opt, const double Speed) { return NotifySendSpeed(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), rpc_call_opt, Speed); }
    inline bool NotifySendSpeed(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const double Speed) { return NotifySendSpeed(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable, Speed); }
    bool NotifySendSpeed(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const double Speed);

    inline bool ReportP2PPeerPing(fun::HostId rpc_recvfrom, const fun::HostId& peer_id, const fun::uint32 recent_ping) { return ReportP2PPeerPing(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable, peer_id, recent_ping); }
    inline bool ReportP2PPeerPing(fun::HostId rpc_recvfrom, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& peer_id, const fun::uint32 recent_ping) { return ReportP2PPeerPing(&rpc_recvfrom, 1, rpc_call_opt, peer_id, recent_ping); }
    template <typename Allocator>
    inline bool ReportP2PPeerPing(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::HostId& peer_id, const fun::uint32 recent_ping) { return ReportP2PPeerPing(RpcRemoteIds, fun::RpcCallOption::Reliable, peer_id, recent_ping); }
    template <typename Allocator>
    inline bool ReportP2PPeerPing(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& peer_id, const fun::uint32 recent_ping) { return ReportP2PPeerPing(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), rpc_call_opt, peer_id, recent_ping); }
    inline bool ReportP2PPeerPing(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::HostId& peer_id, const fun::uint32 recent_ping) { return ReportP2PPeerPing(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable, peer_id, recent_ping); }
    bool ReportP2PPeerPing(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& peer_id, const fun::uint32 recent_ping);

    inline bool C2S_RequestCreateUdpSocket(fun::HostId rpc_recvfrom) { return C2S_RequestCreateUdpSocket(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable); }
    inline bool C2S_RequestCreateUdpSocket(fun::HostId rpc_recvfrom, const fun::RpcCallOption& rpc_call_opt) { return C2S_RequestCreateUdpSocket(&rpc_recvfrom, 1, rpc_call_opt); }
    template <typename Allocator>
    inline bool C2S_RequestCreateUdpSocket(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds) { return C2S_RequestCreateUdpSocket(RpcRemoteIds, fun::RpcCallOption::Reliable); }
    template <typename Allocator>
    inline bool C2S_RequestCreateUdpSocket(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& rpc_call_opt) { return C2S_RequestCreateUdpSocket(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), rpc_call_opt); }
    inline bool C2S_RequestCreateUdpSocket(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount) { return C2S_RequestCreateUdpSocket(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable); }
    bool C2S_RequestCreateUdpSocket(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt);

    inline bool C2S_CreateUdpSocketAck(fun::HostId rpc_recvfrom, const bool bOk) { return C2S_CreateUdpSocketAck(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable, bOk); }
    inline bool C2S_CreateUdpSocketAck(fun::HostId rpc_recvfrom, const fun::RpcCallOption& rpc_call_opt, const bool bOk) { return C2S_CreateUdpSocketAck(&rpc_recvfrom, 1, rpc_call_opt, bOk); }
    template <typename Allocator>
    inline bool C2S_CreateUdpSocketAck(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const bool bOk) { return C2S_CreateUdpSocketAck(RpcRemoteIds, fun::RpcCallOption::Reliable, bOk); }
    template <typename Allocator>
    inline bool C2S_CreateUdpSocketAck(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& rpc_call_opt, const bool bOk) { return C2S_CreateUdpSocketAck(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), rpc_call_opt, bOk); }
    inline bool C2S_CreateUdpSocketAck(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const bool bOk) { return C2S_CreateUdpSocketAck(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable, bOk); }
    bool C2S_CreateUdpSocketAck(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const bool bOk);

    inline bool ReportC2CUdpMessageCount(fun::HostId rpc_recvfrom, const fun::HostId& peer_id, const fun::uint32 udp_message_attempt_count, const fun::uint32 udp_message_success_count) { return ReportC2CUdpMessageCount(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable, peer_id, udp_message_attempt_count, udp_message_success_count); }
    inline bool ReportC2CUdpMessageCount(fun::HostId rpc_recvfrom, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& peer_id, const fun::uint32 udp_message_attempt_count, const fun::uint32 udp_message_success_count) { return ReportC2CUdpMessageCount(&rpc_recvfrom, 1, rpc_call_opt, peer_id, udp_message_attempt_count, udp_message_success_count); }
    template <typename Allocator>
    inline bool ReportC2CUdpMessageCount(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::HostId& peer_id, const fun::uint32 udp_message_attempt_count, const fun::uint32 udp_message_success_count) { return ReportC2CUdpMessageCount(RpcRemoteIds, fun::RpcCallOption::Reliable, peer_id, udp_message_attempt_count, udp_message_success_count); }
    template <typename Allocator>
    inline bool ReportC2CUdpMessageCount(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& peer_id, const fun::uint32 udp_message_attempt_count, const fun::uint32 udp_message_success_count) { return ReportC2CUdpMessageCount(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), rpc_call_opt, peer_id, udp_message_attempt_count, udp_message_success_count); }
    inline bool ReportC2CUdpMessageCount(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::HostId& peer_id, const fun::uint32 udp_message_attempt_count, const fun::uint32 udp_message_success_count) { return ReportC2CUdpMessageCount(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable, peer_id, udp_message_attempt_count, udp_message_success_count); }
    bool ReportC2CUdpMessageCount(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const fun::HostId& peer_id, const fun::uint32 udp_message_attempt_count, const fun::uint32 udp_message_success_count);

    inline bool ReportC2SUdpMessageTrialCount(fun::HostId rpc_recvfrom, const fun::uint32 to_server_udp_attempt_count) { return ReportC2SUdpMessageTrialCount(&rpc_recvfrom, 1, fun::RpcCallOption::Reliable, to_server_udp_attempt_count); }
    inline bool ReportC2SUdpMessageTrialCount(fun::HostId rpc_recvfrom, const fun::RpcCallOption& rpc_call_opt, const fun::uint32 to_server_udp_attempt_count) { return ReportC2SUdpMessageTrialCount(&rpc_recvfrom, 1, rpc_call_opt, to_server_udp_attempt_count); }
    template <typename Allocator>
    inline bool ReportC2SUdpMessageTrialCount(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::uint32 to_server_udp_attempt_count) { return ReportC2SUdpMessageTrialCount(RpcRemoteIds, fun::RpcCallOption::Reliable, to_server_udp_attempt_count); }
    template <typename Allocator>
    inline bool ReportC2SUdpMessageTrialCount(const fun::Array<fun::HostId,Allocator>& RpcRemoteIds, const fun::RpcCallOption& rpc_call_opt, const fun::uint32 to_server_udp_attempt_count) { return ReportC2SUdpMessageTrialCount(RpcRemoteIds.ConstData(), RpcRemoteIds.Count(), rpc_call_opt, to_server_udp_attempt_count); }
    inline bool ReportC2SUdpMessageTrialCount(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::uint32 to_server_udp_attempt_count) { return ReportC2SUdpMessageTrialCount(RpcRemoteIds, RpcRemoteIdCount, fun::RpcCallOption::Reliable, to_server_udp_attempt_count); }
    bool ReportC2SUdpMessageTrialCount(const fun::HostId* RpcRemoteIds, fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& rpc_call_opt, const fun::uint32 to_server_udp_attempt_count);
  };
} // end of namespace NetC2S

} // end of namespace fun