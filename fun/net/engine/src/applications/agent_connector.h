#pragma once

#include "AgentInterface.h"

#include "GeneratedRPCs/Agent_AgentC2S_proxy.h"
#include "GeneratedRPCs/Agent_AgentS2C_stub.h"

namespace fun {
namespace net {

class AgentConnectorImpl
  : public AgentConnector
  , public ILanClientCallbacks
  , public AgentS2C::Stub
{
 public:
  AgentConnectorImpl(IAgentConnectorDelegate* delegate);
  ~AgentConnectorImpl();

  void GetCpuTime(float& out_user_time, float& out_kernel_time);

 private:
  // stub implementation
  DECRPC_AgentS2C_NotifyCredential;
  DECRPC_AgentS2C_RequestServerAppStop;

  THeldPtr<LanClient> client_;
  AgentC2S::Proxy proxy_;
  IAgentConnectorDelegate* delegate_;

  // Agent에게 발급받은 cookie(Agent내에서 serverapp을 식별하기 위한값)
  uint32 cookie_;

  // 상태 정보를 보내는 delay
  uint32 delay_time_about_send_agent_status_;
  uint32 last_send_status_time_;

  uint32 last_try_connect_time_;

  // 커널 정보에 대한 필요한 함수
  uint64 prev_kernel_time_;
  uint64 prev_user_time_;
  double prev_instrumented_time_;

  //ILanClientCallbacks interface
  void OnJoinedToServer(const ResultInfo* result_info, const ByteArray& reply_from_server) override;
  void OnLeftFromServer(const ResultInfo* result_info) override;

  void OnP2PMemberJoined(HostId member_id, HostId group_id, int32 member_count, const ByteArray& custom_field) override {}
  void OnP2PConnectionEstablished(HostId remote_id) override {}
  void OnP2PDisconnected(HostId remote_id, ResultCode result_code) override {}
  void OnGroupP2PConnectionComplete(HostId group_id) override {}
  void OnP2PMemberLeft(HostId member_id, HostId group_id, int32 member_count) override {}

  void OnSynchronizeServerTime() override {}
  void OnUserWorkerThreadBegin() override {}
  void OnUserWorkerThreadEnd() override {}

  void OnError(const ResultInfo* result_info) override {}
  void OnWarning(const ResultInfo* result_info) override {}
  void OnInformation(const ResultInfo* result_info) override {}
  void OnException(HostId host_id, const Exception& e) override {}
  void OnNoRpcProcessed(RpcId rpc_id) override {}

 public:
  // AgentConnector interface
  bool Start() override;
  bool SendReportStatus(const ReportStatus& status) override;
  bool EventLog(ReportStatus::EStatusType log_type, const char* text) override;

  void Tick() override;
  void SetDelayTimeAboutSendAgentStatus(uint32 delay_msec) override;
};

} // namespace net
} // namespace fun
