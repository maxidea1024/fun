#pragma once

#include "Net/Engine/Apps/EmergencyLogInterface.h"
#include "Net/Engine/Apps/EmergencyLogData.h"

#include "GeneratedRPCs/Emergency_EmergencyC2S_proxy.h"
#include "GeneratedRPCs/Emergency_EmergencyS2C_stub.h"

namespace fun {
namespace net {

/**
 * Net 또는 Lan클라이언트가 의도치않게 종료되었을때의 Log를 보내기 위한 Client
 */
class EmergencyLogClient
  : public EmergencyS2C::Stub
  , public INetClientCallbacks
{
public:
  enum class State {
      Connecting,
      Sending,
      Closing,
      Stopped
  };

  EmergencyLogClient();
  ~EmergencyLogClient();

  State GetState() const;

  void Start(const String& server_ip, int32 server_port, EmergencyLogData* log_data);

  void Tick();

private:
  UniquePtr<NetClient> client_;
  EmergencyC2S::Proxy c2s_proxy_;
  EmergencyLogData* log_data_;

  // INetClientCallbacks interface
  void OnJoinedToServer(const ResultInfo* result_info, const ByteArray& reply_from_server) override;
  void OnLeftFromServer(const ResultInfo* result_info) override;
  void OnP2PMemberJoined(HostId member_id, HostId group_id, int32 member_count, const ByteArray& custom_field) override;
  void OnP2PMemberLeft(HostId member_id, HostId group_id, int32 member_count) override;
  void OnError(const ResultInfo* result_info) override;
  void OnWarning(const ResultInfo* result_info) override {}
  void OnInformation(const ResultInfo* result_info) override {}
  void OnException(HostId host_id, const Exception& e) override;
  void OnUnhandledException() override {}
  void OnNoRpcProcessed(RpcId rpc_id) override {}

  void OnP2PStateChanged(HostId remote_id, ResultCode reason) override {}
  void OnSynchronizeServerTime() override {}

  // stub implementation
  DECRPC_EmergencyS2C_EmergencyLogData_AckComplete;

  State state_;
};

} // namespace net
} // namespace fun
