#pragma once

#include "Net/Engine/Apps/VizInterface.h"
#include "VizMessageSummary.h"

#include "GeneratedRPCs/Viz_Viz_proxy.h"
#include "GeneratedRPCs/Viz_Viz_stub.h"

#include "Misc/Timer.h"

namespace fun {
namespace net {

class IVizAgentDelegate;
class NetClientImpl;

class VizAgent
  : public INetClientCallbacks
  , public VizS2C::Stub
{
 public:
  static const Uuid PROTOCOL_VERSION;

  VizAgent( IVizAgentDelegate* delegate,
            const char* server_ip,
            int32 server_port,
            const String& login_key);
  ~VizAgent();

  // IHeartbeatWork interface
  virtual void Heartbeat() override;

 private:
  void Heartbeat_DisconnectedCase();
  void Heartbeat_ConnectingCase();
  void Heartbeat_ConnectedCase();

  void NotifyInitState();
  HostId GetOwnerHostId();

  DECRPC_VizS2C_NotifyLoginOk;
  DECRPC_VizS2C_NotifyLoginFailed;

  IVizAgentDelegate* delegate_;
  UniquePtr<NetClientImpl> net_client_;

  /** Server ip */
  String server_ip_;

  /** Server port number */
  uint16 server_port_;

  /** Login key for logon */
  String login_key_;

  Timer timer_;

 public:
  VizC2S::Proxy c2s_proxy_;

 private:
  enum class State {
    Disconnected,
    Connecting,
    Connected
  };
  State state_;
  double try_connect_begin_time_;

 public:
  // **주의** vizagent를 가진 객체는 자기 상태를 viz에 노티하는 RPC를 콜 하기 전에 반드시 CS 걸고 움직여야 한다!
  // 안그러면 보내는 순서가 꼬여서 받는 쪽에서 상태가 교란될 수 있다.
  CCriticalSection2 mutex_;

 private:
  // INetClientCallbacks interface
  void OnJoinedToServer(const ResultInfo* result_info, const ByteArray& reply_from_server) override;
  void OnLeftFromServer(const ResultInfo* result_info) override;
  void OnP2PMemberJoined(HostId member_id, HostId group_id, int32 member_count, const ByteArray& custom_field) override {}
  void OnP2PMemberLeft(HostId member_id, HostId group_id, int32 member_count) override {}
  void OnError(const ResultInfo* result_info) override {}
  void OnWarning(const ResultInfo* result_info) override {}
  void OnInformation(const ResultInfo* result_info) override {}
  void OnException(HostId host_id, const Exception& e) override {}
  void OnNoRpcProcessed(RpcId rpc_id) override {}
  void OnP2PStateChanged(HostId remote_id, ResultCode reason) override {}
  void OnSynchronizeServerTime() override {}
};

} // namespace net
} // namespace fun
