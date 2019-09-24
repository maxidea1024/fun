#pragma once

#include "Net/Engine/App/DumpInterface.h"

#include "GeneratedRPCs/Dump_DumpS2C_stub.h"
#include "GeneratedRPCs/Dump_DumpC2S_proxy.h"

namespace fun {
namespace net {

class DumpClientImpl
  : public DumpClient
  , public DumpS2C::Stub
  , public INetClientCallbacks
{
 public:
  DumpClientImpl(IDumpClientDelegate* delegate);
  ~DumpClientImpl();

  void Start(const String& server_ip, int32 server_port, const String& filename);
  void Tick();

  State GetState() override;
  int32 GetSendProgress() override;
  int32 GetSendTotal() override;

 private:
  VOLATILE_ALIGN State state_;
  VOLATILE_ALIGN bool should_stop_;

  IDumpClientDelegate* delegate_;

  int32 send_progress_;
  int32 send_total_;

  DumpC2S::Proxy c2s_proxy_;

  String filename_;
  UniquePtr<IFile> file_;

  bool SendNextChunk();

  // INetClientCallbacks interface
  void OnJoinedToServer(const ResultInfo* result_info, const ByteArray& reply_from_server) override;
  void OnLeftFromServer(const ResultInfo* result_info) override;
  void OnP2PMemberJoined(HostId member_id, HostId group_id, int32 member_count, const ByteArray& custom_field) override;
  void OnP2PMemberLeft(HostId member_id, HostId group_id, int32 member_count) override;
  void OnError(const ResultInfo* result_info) override;
  void OnWarning(const ResultInfo* result_info) override {}
  void OnInformation(const ResultInfo* result_info) override {}
  void OnException(HostId host_id, const Exception& e) override {}
  void OnUnhandledException() override {}
  void OnNoRpcProcessed(RpcId rpc_id) override {}
  void OnP2PStateChanged(HostId remote_id, ResultCode reason) override {}
  void OnSynchronizeServerTime() override {}

  // stub implementation
  DECRPC_DumpS2C_Dump_ChunkAck;

  UniquePtr<NetClient> client_;
};

} // namespace net
} // namespace fun
