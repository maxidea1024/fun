#pragma once

#include "Net/Engine/Apps/EmergencyLogData.h"

#include "GeneratedRPCs/Emergency_EmergencyC2S_stub.h"
#include "GeneratedRPCs/Emergency_EmergencyS2C_proxy.h"

namespace fun {
namespace net {

class EmergencyLogServerImpl;

class EmergencyClient_S
{
 public:
  EmergencyLogData log_data_;

  EmergencyLogServerImpl* GetServer() const { return server_; }

  EmergencyClient_S(EmergencyLogServerImpl* server, HostId host_id);
  ~EmergencyClient_S();

 private:
  EmergencyLogServerImpl* server_;
  HostId host_id_;
};

typedef SharedPtr<EmergencyClient_S> EmergencyPtr_S;
typedef Map<HostId, EmergencyPtr_S> EmergencyClients;


class EmergencyLogServerImpl
  : public EmergencyLogServer
  , public EmergencyC2S::Stub
  , public INetServerCallbacks
{
 public:
  EmergencyLogServerImpl(IEmergencyLogServerDelegate* delegate);
  ~EmergencyLogServerImpl();

  void Serve();

 private:
  EmergencyS2C::Proxy s2c_proxy_;

  float elapsed_time_;
  IEmergencyLogServerDelegate* delegate_;

  Clock clock_;
  UniquePtr<NetServer> server_;
  EmergencyClients clients_;
  UniquePtr<IFile> log_file_;
  int32 log_file_count_;

  void Tick();

  float GetElapsedTime();

  EmergencyPtr_S GetClientByHostId_NOLOCK(HostId host_id);

  bool CreateLogFile();

  void WriteClientLog(HostId host_id, const EmergencyLogData& data);

  // INetServerCallbacks interface
  virtual void OnClientJoined(const NetClientInfo* client_info) override;
  virtual void OnClientLeft(const NetClientInfo* client_info, const ResultInfo* result_info, const ByteArray& comment) override;
  virtual void OnError(const ResultInfo* result_info) override {}
  virtual void OnWarning(const ResultInfo* result_info) override {}
  virtual void OnInformation(const ResultInfo* result_info) override {}
  virtual void OnException(HostId host_id, const Exception& e) override {}
  //virtual void OnUnhandledException() override {}
  virtual void OnNoRpcProcessed(RpcId rpc_id) override {}
  virtual bool OnConnectionRequest(const InetAddress& client_addr, const ByteArray& user_data_from_client, ByteArray& reply) override { return true; }
  virtual void OnP2PGroupJoinMemberAckComplete(HostId group_id, HostId member_id, ResultCode result) override {}
  virtual void OnUserWorkerThreadBegin() override {}
  virtual void OnUserWorkerThreadEnd() override {}

  // stub implementation
  DECRPC_EmergencyC2S_EmergencyLogData_Begin;
  DECRPC_EmergencyC2S_EmergencyLogData_Error;
  DECRPC_EmergencyC2S_EmergencyLogData_Stats;
  DECRPC_EmergencyC2S_EmergencyLogData_OSVersion;
  DECRPC_EmergencyC2S_EmergencyLogData_LogEvent;
  DECRPC_EmergencyC2S_EmergencyLogData_End;
};

} // namespace net
} // namespace fun
