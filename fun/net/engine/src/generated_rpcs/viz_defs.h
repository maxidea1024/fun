﻿/**
 * Auto-generated by IDL Compiler (2.1.3)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 */
#pragma once

#include "FUN.h"

namespace fun {

namespace VizC2S {
  const fun::int32 NumRpcFunctions = 9;

  // Rpc function id constants.
  namespace RpcIds {
    const fun::RpcId RequestLogin = (fun::RpcId)600;
    const fun::RpcId NotifyCommon_SendRpc = (fun::RpcId)601;
    const fun::RpcId NotifyCommon_ReceiveRpc = (fun::RpcId)602;
    const fun::RpcId NotifyCli_ConnectionState = (fun::RpcId)603;
    const fun::RpcId NotifyCli_Peers_Clear = (fun::RpcId)604;
    const fun::RpcId NotifyCli_Peers_AddOrEdit = (fun::RpcId)605;
    const fun::RpcId NotifySrv_ClientEmpty = (fun::RpcId)606;
    const fun::RpcId NotifySrv_Clients_AddOrEdit = (fun::RpcId)607;
    const fun::RpcId NotifySrv_Clients_Remove = (fun::RpcId)608;

    inline const fun::RpcId* RpcIdList() {
      static const fun::RpcId IdList[NumRpcFunctions] = {RequestLogin, NotifyCommon_SendRpc, NotifyCommon_ReceiveRpc, NotifyCli_ConnectionState, NotifyCli_Peers_Clear, NotifyCli_Peers_AddOrEdit, NotifySrv_ClientEmpty, NotifySrv_Clients_AddOrEdit, NotifySrv_Clients_Remove};
      return IdList;
    }
  }

  // Rpc function name string constants.
  namespace RpcNames {
    inline const char* RequestLogin() { return TEXT("RequestLogin"); }
    inline const char* NotifyCommon_SendRpc() { return TEXT("NotifyCommon_SendRpc"); }
    inline const char* NotifyCommon_ReceiveRpc() { return TEXT("NotifyCommon_ReceiveRpc"); }
    inline const char* NotifyCli_ConnectionState() { return TEXT("NotifyCli_ConnectionState"); }
    inline const char* NotifyCli_Peers_Clear() { return TEXT("NotifyCli_Peers_Clear"); }
    inline const char* NotifyCli_Peers_AddOrEdit() { return TEXT("NotifyCli_Peers_AddOrEdit"); }
    inline const char* NotifySrv_ClientEmpty() { return TEXT("NotifySrv_ClientEmpty"); }
    inline const char* NotifySrv_Clients_AddOrEdit() { return TEXT("NotifySrv_Clients_AddOrEdit"); }
    inline const char* NotifySrv_Clients_Remove() { return TEXT("NotifySrv_Clients_Remove"); }

    inline const char** RpcNameList() {
      static const char* NameList[NumRpcFunctions] = {RequestLogin(), NotifyCommon_SendRpc(), NotifyCommon_ReceiveRpc(), NotifyCli_ConnectionState(), NotifyCli_Peers_Clear(), NotifyCli_Peers_AddOrEdit(), NotifySrv_ClientEmpty(), NotifySrv_Clients_AddOrEdit(), NotifySrv_Clients_Remove()};
      return NameList;
    }
  }
} // end of namespace VizC2S

namespace VizS2C {
  const fun::int32 NumRpcFunctions = 2;

  // Rpc function id constants.
  namespace RpcIds {
    const fun::RpcId NotifyLoginOk = (fun::RpcId)650;
    const fun::RpcId NotifyLoginFailed = (fun::RpcId)651;

    inline const fun::RpcId* RpcIdList() {
      static const fun::RpcId IdList[NumRpcFunctions] = {NotifyLoginOk, NotifyLoginFailed};
      return IdList;
    }
  }

  // Rpc function name string constants.
  namespace RpcNames {
    inline const char* NotifyLoginOk() { return TEXT("NotifyLoginOk"); }
    inline const char* NotifyLoginFailed() { return TEXT("NotifyLoginFailed"); }

    inline const char** RpcNameList() {
      static const char* NameList[NumRpcFunctions] = {NotifyLoginOk(), NotifyLoginFailed()};
      return NameList;
    }
  }
} // end of namespace VizS2C

} // end of namespace fun
