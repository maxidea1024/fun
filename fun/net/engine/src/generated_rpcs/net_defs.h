﻿/**
 * Auto-generated by IDL Compiler (2.1.3)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 */
#pragma once

#include "FUN.h"

namespace fun {

namespace NetC2C
{
  const fun::int32 NumRpcFunctions = 4;

  // Rpc function id constants.
  namespace RpcIds
  {
    const fun::RpcId SuppressP2PHolepunchTrial = (fun::RpcId)65000;
    const fun::RpcId ReportUdpMessageCount = (fun::RpcId)65001;
    const fun::RpcId ReportServerTimeAndFrameRateAndPing = (fun::RpcId)65002;
    const fun::RpcId ReportServerTimeAndFrameRateAndPong = (fun::RpcId)65003;

    inline const fun::RpcId* RpcIdList()
    {
      static const fun::RpcId IdList[NumRpcFunctions] = {SuppressP2PHolepunchTrial, ReportUdpMessageCount, ReportServerTimeAndFrameRateAndPing, ReportServerTimeAndFrameRateAndPong};
      return IdList;
    }
  }

  // Rpc function name string constants.
  namespace RpcNames
  {
    inline const char* SuppressP2PHolepunchTrial() { return TEXT("SuppressP2PHolepunchTrial"); }
    inline const char* ReportUdpMessageCount() { return TEXT("ReportUdpMessageCount"); }
    inline const char* ReportServerTimeAndFrameRateAndPing() { return TEXT("ReportServerTimeAndFrameRateAndPing"); }
    inline const char* ReportServerTimeAndFrameRateAndPong() { return TEXT("ReportServerTimeAndFrameRateAndPong"); }

    inline const char** RpcNameList()
    {
      static const char* NameList[NumRpcFunctions] = {SuppressP2PHolepunchTrial(), ReportUdpMessageCount(), ReportServerTimeAndFrameRateAndPing(), ReportServerTimeAndFrameRateAndPong()};
      return NameList;
    }
  }
} // end of namespace NetC2C

namespace NetC2S
{
  const fun::int32 NumRpcFunctions = 19;

  // Rpc function id constants.
  namespace RpcIds
  {
    const fun::RpcId ReliablePing = (fun::RpcId)64000;
    const fun::RpcId P2P_NotifyDirectP2PDisconnected = (fun::RpcId)64001;
    const fun::RpcId NotifyUdpToTcpFallbackByClient = (fun::RpcId)64002;
    const fun::RpcId P2PGroup_MemberJoin_Ack = (fun::RpcId)64003;
    const fun::RpcId NotifyP2PHolepunchSuccess = (fun::RpcId)64004;
    const fun::RpcId ShutdownTcp = (fun::RpcId)64005;
    const fun::RpcId ShutdownTcpHandshake = (fun::RpcId)64006;
    const fun::RpcId NotifyLog = (fun::RpcId)64007;
    const fun::RpcId NotifyLogHolepunchFreqFail = (fun::RpcId)64008;
    const fun::RpcId NotifyNatDeviceName = (fun::RpcId)64009;
    const fun::RpcId NotifyPeerUdpSocketRestored = (fun::RpcId)64010;
    const fun::RpcId NotifyJitDirectP2PTriggered = (fun::RpcId)64011;
    const fun::RpcId NotifyNatDeviceNameDetected = (fun::RpcId)64012;
    const fun::RpcId NotifySendSpeed = (fun::RpcId)64013;
    const fun::RpcId ReportP2PPeerPing = (fun::RpcId)64014;
    const fun::RpcId C2S_RequestCreateUdpSocket = (fun::RpcId)64015;
    const fun::RpcId C2S_CreateUdpSocketAck = (fun::RpcId)64016;
    const fun::RpcId ReportC2CUdpMessageCount = (fun::RpcId)64017;
    const fun::RpcId ReportC2SUdpMessageTrialCount = (fun::RpcId)64018;

    inline const fun::RpcId* RpcIdList()
    {
      static const fun::RpcId IdList[NumRpcFunctions] = {ReliablePing, P2P_NotifyDirectP2PDisconnected, NotifyUdpToTcpFallbackByClient, P2PGroup_MemberJoin_Ack, NotifyP2PHolepunchSuccess, ShutdownTcp, ShutdownTcpHandshake, NotifyLog, NotifyLogHolepunchFreqFail, NotifyNatDeviceName, NotifyPeerUdpSocketRestored, NotifyJitDirectP2PTriggered, NotifyNatDeviceNameDetected, NotifySendSpeed, ReportP2PPeerPing, C2S_RequestCreateUdpSocket, C2S_CreateUdpSocketAck, ReportC2CUdpMessageCount, ReportC2SUdpMessageTrialCount};
      return IdList;
    }
  }

  // Rpc function name string constants.
  namespace RpcNames
  {
    inline const char* ReliablePing() { return TEXT("ReliablePing"); }
    inline const char* P2P_NotifyDirectP2PDisconnected() { return TEXT("P2P_NotifyDirectP2PDisconnected"); }
    inline const char* NotifyUdpToTcpFallbackByClient() { return TEXT("NotifyUdpToTcpFallbackByClient"); }
    inline const char* P2PGroup_MemberJoin_Ack() { return TEXT("P2PGroup_MemberJoin_Ack"); }
    inline const char* NotifyP2PHolepunchSuccess() { return TEXT("NotifyP2PHolepunchSuccess"); }
    inline const char* ShutdownTcp() { return TEXT("ShutdownTcp"); }
    inline const char* ShutdownTcpHandshake() { return TEXT("ShutdownTcpHandshake"); }
    inline const char* NotifyLog() { return TEXT("NotifyLog"); }
    inline const char* NotifyLogHolepunchFreqFail() { return TEXT("NotifyLogHolepunchFreqFail"); }
    inline const char* NotifyNatDeviceName() { return TEXT("NotifyNatDeviceName"); }
    inline const char* NotifyPeerUdpSocketRestored() { return TEXT("NotifyPeerUdpSocketRestored"); }
    inline const char* NotifyJitDirectP2PTriggered() { return TEXT("NotifyJitDirectP2PTriggered"); }
    inline const char* NotifyNatDeviceNameDetected() { return TEXT("NotifyNatDeviceNameDetected"); }
    inline const char* NotifySendSpeed() { return TEXT("NotifySendSpeed"); }
    inline const char* ReportP2PPeerPing() { return TEXT("ReportP2PPeerPing"); }
    inline const char* C2S_RequestCreateUdpSocket() { return TEXT("C2S_RequestCreateUdpSocket"); }
    inline const char* C2S_CreateUdpSocketAck() { return TEXT("C2S_CreateUdpSocketAck"); }
    inline const char* ReportC2CUdpMessageCount() { return TEXT("ReportC2CUdpMessageCount"); }
    inline const char* ReportC2SUdpMessageTrialCount() { return TEXT("ReportC2SUdpMessageTrialCount"); }

    inline const char** RpcNameList()
    {
      static const char* NameList[NumRpcFunctions] = {ReliablePing(), P2P_NotifyDirectP2PDisconnected(), NotifyUdpToTcpFallbackByClient(), P2PGroup_MemberJoin_Ack(), NotifyP2PHolepunchSuccess(), ShutdownTcp(), ShutdownTcpHandshake(), NotifyLog(), NotifyLogHolepunchFreqFail(), NotifyNatDeviceName(), NotifyPeerUdpSocketRestored(), NotifyJitDirectP2PTriggered(), NotifyNatDeviceNameDetected(), NotifySendSpeed(), ReportP2PPeerPing(), C2S_RequestCreateUdpSocket(), C2S_CreateUdpSocketAck(), ReportC2CUdpMessageCount(), ReportC2SUdpMessageTrialCount()};
      return NameList;
    }
  }
} // end of namespace NetC2S

namespace NetS2C
{
  const fun::int32 NumRpcFunctions = 19;

  // Rpc function id constants.
  namespace RpcIds
  {
    const fun::RpcId P2PGroup_MemberJoin = (fun::RpcId)64500;
    const fun::RpcId P2PGroup_MemberJoin_Unencrypted = (fun::RpcId)64501;
    const fun::RpcId P2PRecycleComplete = (fun::RpcId)64502;
    const fun::RpcId RequestP2PHolepunch = (fun::RpcId)64503;
    const fun::RpcId P2P_NotifyDirectP2PDisconnected2 = (fun::RpcId)64504;
    const fun::RpcId P2PGroup_MemberLeave = (fun::RpcId)64505;
    const fun::RpcId NotifyDirectP2PEstablish = (fun::RpcId)64507;
    const fun::RpcId ReliablePong = (fun::RpcId)64508;
    const fun::RpcId EnableIntraLogging = (fun::RpcId)64510;
    const fun::RpcId DisableIntraLogging = (fun::RpcId)64511;
    const fun::RpcId NotifyUdpToTcpFallbackByServer = (fun::RpcId)64512;
    const fun::RpcId NotifySpeedHackDetectorEnabled = (fun::RpcId)64513;
    const fun::RpcId ShutdownTcpAck = (fun::RpcId)64514;
    const fun::RpcId RequestAutoPrune = (fun::RpcId)64515;
    const fun::RpcId RenewP2PConnectionState = (fun::RpcId)64516;
    const fun::RpcId NewDirectP2PConnection = (fun::RpcId)64517;
    const fun::RpcId RequestMeasureSendSpeed = (fun::RpcId)64518;
    const fun::RpcId S2C_RequestCreateUdpSocket = (fun::RpcId)64519;
    const fun::RpcId S2C_CreateUdpSocketAck = (fun::RpcId)64520;

    inline const fun::RpcId* RpcIdList()
    {
      static const fun::RpcId IdList[NumRpcFunctions] = {P2PGroup_MemberJoin, P2PGroup_MemberJoin_Unencrypted, P2PRecycleComplete, RequestP2PHolepunch, P2P_NotifyDirectP2PDisconnected2, P2PGroup_MemberLeave, NotifyDirectP2PEstablish, ReliablePong, EnableIntraLogging, DisableIntraLogging, NotifyUdpToTcpFallbackByServer, NotifySpeedHackDetectorEnabled, ShutdownTcpAck, RequestAutoPrune, RenewP2PConnectionState, NewDirectP2PConnection, RequestMeasureSendSpeed, S2C_RequestCreateUdpSocket, S2C_CreateUdpSocketAck};
      return IdList;
    }
  }

  // Rpc function name string constants.
  namespace RpcNames
  {
    inline const char* P2PGroup_MemberJoin() { return TEXT("P2PGroup_MemberJoin"); }
    inline const char* P2PGroup_MemberJoin_Unencrypted() { return TEXT("P2PGroup_MemberJoin_Unencrypted"); }
    inline const char* P2PRecycleComplete() { return TEXT("P2PRecycleComplete"); }
    inline const char* RequestP2PHolepunch() { return TEXT("RequestP2PHolepunch"); }
    inline const char* P2P_NotifyDirectP2PDisconnected2() { return TEXT("P2P_NotifyDirectP2PDisconnected2"); }
    inline const char* P2PGroup_MemberLeave() { return TEXT("P2PGroup_MemberLeave"); }
    inline const char* NotifyDirectP2PEstablish() { return TEXT("NotifyDirectP2PEstablish"); }
    inline const char* ReliablePong() { return TEXT("ReliablePong"); }
    inline const char* EnableIntraLogging() { return TEXT("EnableIntraLogging"); }
    inline const char* DisableIntraLogging() { return TEXT("DisableIntraLogging"); }
    inline const char* NotifyUdpToTcpFallbackByServer() { return TEXT("NotifyUdpToTcpFallbackByServer"); }
    inline const char* NotifySpeedHackDetectorEnabled() { return TEXT("NotifySpeedHackDetectorEnabled"); }
    inline const char* ShutdownTcpAck() { return TEXT("ShutdownTcpAck"); }
    inline const char* RequestAutoPrune() { return TEXT("RequestAutoPrune"); }
    inline const char* RenewP2PConnectionState() { return TEXT("RenewP2PConnectionState"); }
    inline const char* NewDirectP2PConnection() { return TEXT("NewDirectP2PConnection"); }
    inline const char* RequestMeasureSendSpeed() { return TEXT("RequestMeasureSendSpeed"); }
    inline const char* S2C_RequestCreateUdpSocket() { return TEXT("S2C_RequestCreateUdpSocket"); }
    inline const char* S2C_CreateUdpSocketAck() { return TEXT("S2C_CreateUdpSocketAck"); }

    inline const char** RpcNameList()
    {
      static const char* NameList[NumRpcFunctions] = {P2PGroup_MemberJoin(), P2PGroup_MemberJoin_Unencrypted(), P2PRecycleComplete(), RequestP2PHolepunch(), P2P_NotifyDirectP2PDisconnected2(), P2PGroup_MemberLeave(), NotifyDirectP2PEstablish(), ReliablePong(), EnableIntraLogging(), DisableIntraLogging(), NotifyUdpToTcpFallbackByServer(), NotifySpeedHackDetectorEnabled(), ShutdownTcpAck(), RequestAutoPrune(), RenewP2PConnectionState(), NewDirectP2PConnection(), RequestMeasureSendSpeed(), S2C_RequestCreateUdpSocket(), S2C_CreateUdpSocketAck()};
      return NameList;
    }
  }
} // end of namespace NetS2C

} // end of namespace fun