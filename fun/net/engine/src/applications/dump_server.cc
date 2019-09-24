#include "../NetEnginePrivate.h"

#include "DumpServer.h"

#include "GenertatedRPCs/Dump_DumpC2S_stub.cc"
#include "GenertatedRPCs/Dump_Dumps2c_proxy.cc"

namespace fun {
namespace net {

//
// DumpServerImpl
//

DumpServerImpl::DumpServerImpl(IDumpServerDelegate* delegate)
{
  delegate_ = delegate;

  clock_.Start();

  server_.Attach(NetServer::New());
  server_->AttachProxy(&s2c_proxy_);
  server_->AttachStub(this);
  server_->SetCallbacks(this);
}

DumpServerImpl::~DumpServerImpl()
{
}

void DumpServerImpl::Serve()
{
  //CCoInitializer COI;

  // 시작
  StartServerArgs start_server_args;
  delegate_->OnStartServer(start_server_args); //@note OnStartServer를 OnPreStartServer로 바꾸는게 좋을듯..(StartParams이 이곳에서 셋업됨?)

  start_server_args.protocol_version = Uuid::From(DUMP_PROTOCOL_VERSION);
  // 스레드 풀 갯수를 충분히 둔다. 덤프 파일을 기록하는 루틴이 오래 걸리기 때문이다.
  start_server_args.thread_count = GetNoofProcessors() * 3;

  // dump server에서는 udp를 사용하지 않는다.
  server_->SetDefaultFallbackMethod(FallbackMethod::ServerUdpToTcp);

  SharedPtr<ResultInfo> result_info;
  bool result = server_->Start(start_server_args, result_info);
  if (result) {
    delegate_->OnServerStartComplete(SharedPtr<ResultInfo>());
  }
  else {
    delegate_->OnServerStartComplete(result_info);
  }

  while (!delegate_->ShouldStop()) {
    //try {
      elapsed_time_ = (float)clock_.ElapsedSeconds();

      Tick();
    //}
    //catch (_com_error& e) {
    //  e;
      //WARN_COMEXCEPTION(e);
    //}

    CPlatformProcess::Sleep(0.001f);
  }
}

void DumpServerImpl::Tick()
{
  CScopedLock2 guard(delegate_->GetMutex());

  delegate_->OnTick();
}

void DumpServerImpl::OnClientJoined(const NetClientInfo* client_info)
{
  CScopedLock2 guard(delegate_->GetMutex());

  DumpClientPtr_S new_client(new DumpClient_S(this, client_info->host_id_));
  clients_.insert(DumpClients::value_type(client_info->host_id_, new_client));
}

void DumpServerImpl::OnClientLeft(const NetClientInfo* client_info,
                                  const ResultInfo* result_info,
                                  const ByteArray& comment)
{
  CScopedLock2 guard(delegate_->GetMutex());

  clients_.erase(client_info->host_id_);
}

DumpClientPtr_S DumpServerImpl::GetClientByHostId(HostId client_id)
{
  CScopedLock2 guard(delegate_->GetMutex());

  return clients_.FindRef(client_id);
}

float DumpServerImpl::GetElapsedTime()
{
  return elapsed_time_;
}

int32 DumpServerImpl::GetUserCount()
{
  CScopedLock2 Lock(delegate_->GetMutex());

  return clients_.Count();
}


//
// DumpClient_S
//

DumpClient_S::DumpClient_S(DumpServerImpl* server, HostId host_id)
{
  server_ = server;
  host_id_ = host_id;
}

DumpClient_S::~DumpClient_S()
{
}

bool DumpServerImpl::Dump_Start(HostId rpc_recvfrom, const RpcHint& rpc_hint)
{
  CScopedLock2 guard(delegate_->GetMutex());

  // 아직 파일 생성중이 아니면 파일을 생성한다.
  DumpClientPtr_S client = GetClientByHostId(rpc_recvfrom);
  if (client && client->file_ == nullptr) {
    NetClientInfo client_info;
    if (server_->GetClientInfo(rpc_recvfrom, client_info)) {
      // lock은 피한다. 오래 걸리니까.
      guard.Unlock();

      string filename = delegate_->GetDumpFilePath(rpc_recvfrom, client_info.tcp_addr_from_server, DateTime::Now());

      // non MFC class를 이용해서 파일에 기록한다.
      SharedPtr<IFile> file(IPlatformFS::GetPlatformPhysical().OpenWrite(filename));
      if (file == nullptr) {
        // 특별히 에러를 발생하지는 않고, 그냥 리턴한다. 이후의 메시지는 모두 버린다.
        return true;
      }

      guard.Lock();

      // 생성한 파일을 연결시킨다.
      client = GetClientByHostId(rpc_recvfrom);
      if (client) {
        client->file_ = file;
      }
    }
  }
  return true;
}

bool DumpServerImpl::Dump_Chunk(HostId rpc_recvfrom, const RpcHint& rpc_hint, const ByteArray& chunk)
{
  CScopedLock2 guard(delegate_->GetMutex());

  // 이미 파일을 생성한 상태이어야 한다.
  auto client = GetClientByHostId(rpc_recvfrom);
  if (client && client->file_) {
    NetClientInfo client_info;
    if (server_->GetClientInfo(rpc_recvfrom, client_info)) {
      SharedPtr<IFile> file = client->file_;
      if (file) {
        // Send ack.
        s2c_proxy_.Dump_ChunkAck(rpc_recvfrom, GReliableSend_INTERNAL);
      }

      // lock은 피한다. 오래 걸리니까.
      guard.Unlock();

      file->Write(chunk.GetData(), chunk.Count());
    }
  }
  return true;
}

bool DumpServerImpl::Dump_End(HostId rpc_recvfrom, const RpcHint& rpc_hint)
{
  CScopedLock2 guard(delegate_->GetMutex());

  // 파일을 닫는다. 더 이상 처리할 것이 없다.
  auto client = GetClientByHostId(rpc_recvfrom);
  if (client && client->file_) {
    NetClientInfo client_info;
    if (server_->GetClientInfo(rpc_recvfrom, client_info)) {
      client->file_ = SharedPtr<IFile>(); // 파일을 닫는다.
    }
  }
  return true;
}

DumpServer* DumpServer::New(IDumpServerDelegate* delegate)
{
  return new DumpServerImpl(delegate);
}

} // namespace net
} // namespace fun
