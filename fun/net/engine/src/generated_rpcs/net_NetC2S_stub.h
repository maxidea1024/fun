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
  struct Stub : public fun::RpcStub
  {
    fun::int32 GetDeclaredRpcCount() const override { return NetC2S::NumRpcFunctions; }
    const fun::RpcId* GetDeclaredRpcIds() const override { return NetC2S::RpcIds::RpcIdList(); }
    const char** GetDeclaredRpcNames() const override { return NetC2S::RpcNames::RpcNameList(); }

    // ReliablePing
    virtual bool ReliablePing(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const double recent_frame_rate) { return false; }
    #define DECLARE_RPCSTUB_NetC2S_ReliablePing bool ReliablePing(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const double recent_frame_rate) override;
    #define IMPLEMENT_RPCSTUB_NetC2S_ReliablePing(TClass) bool TClass::ReliablePing(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const double recent_frame_rate)
    typedef fun::Function<bool(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const double /* recent_frame_rate */)> ReliablePingFunctionType;
    ReliablePingFunctionType OnReliablePing;

    // P2P_NotifyDirectP2PDisconnected
    virtual bool P2P_NotifyDirectP2PDisconnected(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::HostId& peer_id, const fun::ResultCode& reason) { return false; }
    #define DECLARE_RPCSTUB_NetC2S_P2P_NotifyDirectP2PDisconnected bool P2P_NotifyDirectP2PDisconnected(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::HostId& peer_id, const fun::ResultCode& reason) override;
    #define IMPLEMENT_RPCSTUB_NetC2S_P2P_NotifyDirectP2PDisconnected(TClass) bool TClass::P2P_NotifyDirectP2PDisconnected(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::HostId& peer_id, const fun::ResultCode& reason)
    typedef fun::Function<bool(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::HostId& /* peer_id */, const fun::ResultCode& /* reason */)> P2P_NotifyDirectP2PDisconnectedFunctionType;
    P2P_NotifyDirectP2PDisconnectedFunctionType OnP2P_NotifyDirectP2PDisconnected;

    // NotifyUdpToTcpFallbackByClient
    virtual bool NotifyUdpToTcpFallbackByClient(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint) { return false; }
    #define DECLARE_RPCSTUB_NetC2S_NotifyUdpToTcpFallbackByClient bool NotifyUdpToTcpFallbackByClient(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint) override;
    #define IMPLEMENT_RPCSTUB_NetC2S_NotifyUdpToTcpFallbackByClient(TClass) bool TClass::NotifyUdpToTcpFallbackByClient(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint)
    typedef fun::Function<bool(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint)> NotifyUdpToTcpFallbackByClientFunctionType;
    NotifyUdpToTcpFallbackByClientFunctionType OnNotifyUdpToTcpFallbackByClient;

    // P2PGroup_MemberJoin_Ack
    virtual bool P2PGroup_MemberJoin_Ack(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::HostId& group_id, const fun::HostId& added_member_id, const fun::uint32 event_id, const bool bLocalPortReuseOk) { return false; }
    #define DECLARE_RPCSTUB_NetC2S_P2PGroup_MemberJoin_Ack bool P2PGroup_MemberJoin_Ack(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::HostId& group_id, const fun::HostId& added_member_id, const fun::uint32 event_id, const bool bLocalPortReuseOk) override;
    #define IMPLEMENT_RPCSTUB_NetC2S_P2PGroup_MemberJoin_Ack(TClass) bool TClass::P2PGroup_MemberJoin_Ack(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::HostId& group_id, const fun::HostId& added_member_id, const fun::uint32 event_id, const bool bLocalPortReuseOk)
    typedef fun::Function<bool(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::HostId& /* group_id */, const fun::HostId& /* added_member_id */, const fun::uint32 /* event_id */, const bool /* bLocalPortReuseOk */)> P2PGroup_MemberJoin_AckFunctionType;
    P2PGroup_MemberJoin_AckFunctionType OnP2PGroup_MemberJoin_Ack;

    // NotifyP2PHolepunchSuccess
    virtual bool NotifyP2PHolepunchSuccess(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::HostId& A, const fun::HostId& B, const fun::InetAddress a2b_send_addr, const fun::InetAddress a2b_recv_addr, const fun::InetAddress b2a_send_addr, const fun::InetAddress b2a_recv_addr) { return false; }
    #define DECLARE_RPCSTUB_NetC2S_NotifyP2PHolepunchSuccess bool NotifyP2PHolepunchSuccess(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::HostId& A, const fun::HostId& B, const fun::InetAddress a2b_send_addr, const fun::InetAddress a2b_recv_addr, const fun::InetAddress b2a_send_addr, const fun::InetAddress b2a_recv_addr) override;
    #define IMPLEMENT_RPCSTUB_NetC2S_NotifyP2PHolepunchSuccess(TClass) bool TClass::NotifyP2PHolepunchSuccess(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::HostId& A, const fun::HostId& B, const fun::InetAddress a2b_send_addr, const fun::InetAddress a2b_recv_addr, const fun::InetAddress b2a_send_addr, const fun::InetAddress b2a_recv_addr)
    typedef fun::Function<bool(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::HostId& /* A */, const fun::HostId& /* B */, const fun::InetAddress /* a2b_send_addr */, const fun::InetAddress /* a2b_recv_addr */, const fun::InetAddress /* b2a_send_addr */, const fun::InetAddress /* b2a_recv_addr */)> NotifyP2PHolepunchSuccessFunctionType;
    NotifyP2PHolepunchSuccessFunctionType OnNotifyP2PHolepunchSuccess;

    // ShutdownTcp
    virtual bool ShutdownTcp(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::ByteArray& comment) { return false; }
    #define DECLARE_RPCSTUB_NetC2S_ShutdownTcp bool ShutdownTcp(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::ByteArray& comment) override;
    #define IMPLEMENT_RPCSTUB_NetC2S_ShutdownTcp(TClass) bool TClass::ShutdownTcp(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::ByteArray& comment)
    typedef fun::Function<bool(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::ByteArray& /* comment */)> ShutdownTcpFunctionType;
    ShutdownTcpFunctionType OnShutdownTcp;

    // ShutdownTcpHandshake
    virtual bool ShutdownTcpHandshake(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint) { return false; }
    #define DECLARE_RPCSTUB_NetC2S_ShutdownTcpHandshake bool ShutdownTcpHandshake(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint) override;
    #define IMPLEMENT_RPCSTUB_NetC2S_ShutdownTcpHandshake(TClass) bool TClass::ShutdownTcpHandshake(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint)
    typedef fun::Function<bool(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint)> ShutdownTcpHandshakeFunctionType;
    ShutdownTcpHandshakeFunctionType OnShutdownTcpHandshake;

    // NotifyLog
    virtual bool NotifyLog(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::LogCategory& Category, const fun::String& text) { return false; }
    #define DECLARE_RPCSTUB_NetC2S_NotifyLog bool NotifyLog(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::LogCategory& Category, const fun::String& text) override;
    #define IMPLEMENT_RPCSTUB_NetC2S_NotifyLog(TClass) bool TClass::NotifyLog(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::LogCategory& Category, const fun::String& text)
    typedef fun::Function<bool(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::LogCategory& /* Category */, const fun::String& /* text */)> NotifyLogFunctionType;
    NotifyLogFunctionType OnNotifyLog;

    // NotifyLogHolepunchFreqFail
    virtual bool NotifyLogHolepunchFreqFail(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::int32 Rank, const fun::String& text) { return false; }
    #define DECLARE_RPCSTUB_NetC2S_NotifyLogHolepunchFreqFail bool NotifyLogHolepunchFreqFail(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::int32 Rank, const fun::String& text) override;
    #define IMPLEMENT_RPCSTUB_NetC2S_NotifyLogHolepunchFreqFail(TClass) bool TClass::NotifyLogHolepunchFreqFail(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::int32 Rank, const fun::String& text)
    typedef fun::Function<bool(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::int32 /* Rank */, const fun::String& /* text */)> NotifyLogHolepunchFreqFailFunctionType;
    NotifyLogHolepunchFreqFailFunctionType OnNotifyLogHolepunchFreqFail;

    // NotifyNatDeviceName
    virtual bool NotifyNatDeviceName(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::String& DeviceName) { return false; }
    #define DECLARE_RPCSTUB_NetC2S_NotifyNatDeviceName bool NotifyNatDeviceName(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::String& DeviceName) override;
    #define IMPLEMENT_RPCSTUB_NetC2S_NotifyNatDeviceName(TClass) bool TClass::NotifyNatDeviceName(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::String& DeviceName)
    typedef fun::Function<bool(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::String& /* DeviceName */)> NotifyNatDeviceNameFunctionType;
    NotifyNatDeviceNameFunctionType OnNotifyNatDeviceName;

    // NotifyPeerUdpSocketRestored
    virtual bool NotifyPeerUdpSocketRestored(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::HostId& PeerBID) { return false; }
    #define DECLARE_RPCSTUB_NetC2S_NotifyPeerUdpSocketRestored bool NotifyPeerUdpSocketRestored(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::HostId& PeerBID) override;
    #define IMPLEMENT_RPCSTUB_NetC2S_NotifyPeerUdpSocketRestored(TClass) bool TClass::NotifyPeerUdpSocketRestored(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::HostId& PeerBID)
    typedef fun::Function<bool(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::HostId& /* PeerBID */)> NotifyPeerUdpSocketRestoredFunctionType;
    NotifyPeerUdpSocketRestoredFunctionType OnNotifyPeerUdpSocketRestored;

    // NotifyJitDirectP2PTriggered
    virtual bool NotifyJitDirectP2PTriggered(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::HostId& PeerBID) { return false; }
    #define DECLARE_RPCSTUB_NetC2S_NotifyJitDirectP2PTriggered bool NotifyJitDirectP2PTriggered(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::HostId& PeerBID) override;
    #define IMPLEMENT_RPCSTUB_NetC2S_NotifyJitDirectP2PTriggered(TClass) bool TClass::NotifyJitDirectP2PTriggered(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::HostId& PeerBID)
    typedef fun::Function<bool(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::HostId& /* PeerBID */)> NotifyJitDirectP2PTriggeredFunctionType;
    NotifyJitDirectP2PTriggeredFunctionType OnNotifyJitDirectP2PTriggered;

    // NotifyNatDeviceNameDetected
    virtual bool NotifyNatDeviceNameDetected(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::String& DeviceName) { return false; }
    #define DECLARE_RPCSTUB_NetC2S_NotifyNatDeviceNameDetected bool NotifyNatDeviceNameDetected(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::String& DeviceName) override;
    #define IMPLEMENT_RPCSTUB_NetC2S_NotifyNatDeviceNameDetected(TClass) bool TClass::NotifyNatDeviceNameDetected(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::String& DeviceName)
    typedef fun::Function<bool(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::String& /* DeviceName */)> NotifyNatDeviceNameDetectedFunctionType;
    NotifyNatDeviceNameDetectedFunctionType OnNotifyNatDeviceNameDetected;

    // NotifySendSpeed
    virtual bool NotifySendSpeed(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const double Speed) { return false; }
    #define DECLARE_RPCSTUB_NetC2S_NotifySendSpeed bool NotifySendSpeed(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const double Speed) override;
    #define IMPLEMENT_RPCSTUB_NetC2S_NotifySendSpeed(TClass) bool TClass::NotifySendSpeed(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const double Speed)
    typedef fun::Function<bool(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const double /* Speed */)> NotifySendSpeedFunctionType;
    NotifySendSpeedFunctionType OnNotifySendSpeed;

    // ReportP2PPeerPing
    virtual bool ReportP2PPeerPing(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::HostId& peer_id, const fun::uint32 recent_ping) { return false; }
    #define DECLARE_RPCSTUB_NetC2S_ReportP2PPeerPing bool ReportP2PPeerPing(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::HostId& peer_id, const fun::uint32 recent_ping) override;
    #define IMPLEMENT_RPCSTUB_NetC2S_ReportP2PPeerPing(TClass) bool TClass::ReportP2PPeerPing(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::HostId& peer_id, const fun::uint32 recent_ping)
    typedef fun::Function<bool(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::HostId& /* peer_id */, const fun::uint32 /* recent_ping */)> ReportP2PPeerPingFunctionType;
    ReportP2PPeerPingFunctionType OnReportP2PPeerPing;

    // C2S_RequestCreateUdpSocket
    virtual bool C2S_RequestCreateUdpSocket(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint) { return false; }
    #define DECLARE_RPCSTUB_NetC2S_C2S_RequestCreateUdpSocket bool C2S_RequestCreateUdpSocket(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint) override;
    #define IMPLEMENT_RPCSTUB_NetC2S_C2S_RequestCreateUdpSocket(TClass) bool TClass::C2S_RequestCreateUdpSocket(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint)
    typedef fun::Function<bool(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint)> C2S_RequestCreateUdpSocketFunctionType;
    C2S_RequestCreateUdpSocketFunctionType OnC2S_RequestCreateUdpSocket;

    // C2S_CreateUdpSocketAck
    virtual bool C2S_CreateUdpSocketAck(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const bool bOk) { return false; }
    #define DECLARE_RPCSTUB_NetC2S_C2S_CreateUdpSocketAck bool C2S_CreateUdpSocketAck(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const bool bOk) override;
    #define IMPLEMENT_RPCSTUB_NetC2S_C2S_CreateUdpSocketAck(TClass) bool TClass::C2S_CreateUdpSocketAck(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const bool bOk)
    typedef fun::Function<bool(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const bool /* bOk */)> C2S_CreateUdpSocketAckFunctionType;
    C2S_CreateUdpSocketAckFunctionType OnC2S_CreateUdpSocketAck;

    // ReportC2CUdpMessageCount
    virtual bool ReportC2CUdpMessageCount(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::HostId& peer_id, const fun::uint32 udp_message_attempt_count, const fun::uint32 udp_message_success_count) { return false; }
    #define DECLARE_RPCSTUB_NetC2S_ReportC2CUdpMessageCount bool ReportC2CUdpMessageCount(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::HostId& peer_id, const fun::uint32 udp_message_attempt_count, const fun::uint32 udp_message_success_count) override;
    #define IMPLEMENT_RPCSTUB_NetC2S_ReportC2CUdpMessageCount(TClass) bool TClass::ReportC2CUdpMessageCount(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::HostId& peer_id, const fun::uint32 udp_message_attempt_count, const fun::uint32 udp_message_success_count)
    typedef fun::Function<bool(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::HostId& /* peer_id */, const fun::uint32 /* udp_message_attempt_count */, const fun::uint32 /* udp_message_success_count */)> ReportC2CUdpMessageCountFunctionType;
    ReportC2CUdpMessageCountFunctionType OnReportC2CUdpMessageCount;

    // ReportC2SUdpMessageTrialCount
    virtual bool ReportC2SUdpMessageTrialCount(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::uint32 to_server_udp_attempt_count) { return false; }
    #define DECLARE_RPCSTUB_NetC2S_ReportC2SUdpMessageTrialCount bool ReportC2SUdpMessageTrialCount(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::uint32 to_server_udp_attempt_count) override;
    #define IMPLEMENT_RPCSTUB_NetC2S_ReportC2SUdpMessageTrialCount(TClass) bool TClass::ReportC2SUdpMessageTrialCount(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::uint32 to_server_udp_attempt_count)
    typedef fun::Function<bool(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::uint32 /* to_server_udp_attempt_count */)> ReportC2SUdpMessageTrialCountFunctionType;
    ReportC2SUdpMessageTrialCountFunctionType OnReportC2SUdpMessageTrialCount;

    // RpcStub interface
    bool ProcessReceivedMessage(fun::ReceivedMessage& received_msg, void* host_tag) override;
  };

} // end of namespace NetC2S

} // end of namespace fun
