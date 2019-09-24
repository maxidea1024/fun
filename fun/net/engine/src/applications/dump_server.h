#pragma once

#include "Net/Engine/Apps/DumpInterface.h"

#include "GeneratedRPCs/Dump_DumpC2S_stub.h"
#include "GeneratedRPCs/Dump_Dumps2c_proxy.h"

namespace fun {

extern const GUID DUMP_PROTOCOL_VERSION;

class DumpServerImpl;

class DumpClient_S
{
 public:
  UniquePtr<IFile> file_;

  DumpClient_S(DumpServerImpl* server, HostId host_id);
  ~DumpClient_S();

  DumpServerImpl* GetServer() const
  {
    return server_;
  }

 private:
  DumpServerImpl* server_;
  HostId host_id_;
};

typedef SharedPtr<DumpClient_S> DumpClientPtr_S;
typedef Map<HostId, DumpClientPtr_S> DumpClients;


class DumpServerImpl
  : public DumpServer
  , public DumpC2S::Stub
  , public INetServerCallbacks
{
 public:
  DumpServerImpl(IDumpServerDelegate* delegate);
  ~DumpServerImpl();

  void Serve();

 private:
  void Tick();

  DumpS2C::Proxy s2c_proxy_;
  float elapsed_time_;
  IDumpServerDelegate* delegate_;
  Clock clock_;
  UniquePtr<NetServer> server_;
  DumpClients clients_;

  float GetElapsedTime();

 private:
  // INetServerCallbacks interface
  void OnError(const ResultInfo* result_info) override {}
  void OnWarning(const ResultInfo* result_info) override {}
  void OnInformation(const ResultInfo* result_info) override {}
  void OnException(HostId host_id, const Exception& e) override {}
  void OnUnhandledException() override {}
  void OnNoRpcProcessed(RpcId rpc_id) override {}
  void OnUserWorkerThreadBegin() override {}
  void OnUserWorkerThreadEnd() override {}

  void OnClientJoined(const NetClientInfo* client_info) override;
  void OnClientLeft(const NetClientInfo* client_info, const ResultInfo* result_info, const ByteArray& comment) override;
  bool OnConnectionRequest(const InetAddress& client_addr, const ByteArray& user_data_from_client, ByteArray& reply) override { return true; }
  void OnP2PGroupJoinMemberAckComplete(HostId group_id, HostId member_id, ResultCode result) override {}

  // stub implementation
  DECRPC_DumpC2S_Dump_Start;
  DECRPC_DumpC2S_Dump_Chunk;
  DECRPC_DumpC2S_Dump_End;

  int32 GetUserCount();

  DumpClientPtr_S GetClientByHostId(HostId client_id);
  DumpClientPtr_S GetClientByLogonTimeUUID(const Uuid& logon_time_uuid); //@todo CGuid로 대체하는게 어떨런지??
};

} // namespace net
} // namespace fun
