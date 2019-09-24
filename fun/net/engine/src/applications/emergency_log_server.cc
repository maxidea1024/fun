#include "../NetEnginePrivate.h"
#include "Net/Engine/Apps/EmergencyLogServer.h"

#include "GeneratedRPCs/Emergency_EmergencyC2S_stub.cc"
#include "GeneratedRPCs/Emergency_EmergencyS2C_proxy.cc"

namespace fun {
namespace net {

const int32 LOG_FILE_SIZE = 1024 * 1024 * 100;

EmergencyLogServerImpl::EmergencyLogServerImpl(IEmergencyLogServerDelegate* delegate)
{
  delegate_ = delegate;

  clock_.Start();

  server_.Attach(NetServer::New());
  server_->AttachProxy(&s2c_proxy_);
  server_->AttachStub(this);
  server_->SetCallbacks(this);

  log_file_count_ = 0;
  CreateLogFile(); // 미리 로그 파일을 생성
}

EmergencyLogServerImpl::~EmergencyLogServerImpl()
{

}

void EmergencyLogServerImpl::RunMainLoop()
{
  //CCoInitializer COI;

  // server start
  StartServerArgs args;
  delegate_->OnStartServer(args);

  args.protocol_version = Uuid::From(EMERGENCY_PROTCOL_VERSION);
  args.thread_count = GetNoofProcessors();

  // Disable UDP
  server_->SetDefaultFallbackMethod(FallbackMethod::ServerUdpToTcp);

  SharedPtr<ResultInfo> result_info;
  if (server_->Start(args, result_info)) {
    // Notify server is started successfully!
    delegate_->OnServerStartComplete(SharedPtr<ResultInfo>());

    // Run loop.
    while (!delegate_->ShouldStop()) {
      //try {
        elapsed_time_ = (float)clock_.ElapsedSeconds();
        Tick();
      //}
      //catch (_com_error& e) {
      //  (void)e;
      //}

      Thread::Sleep(100);
    }
  }
  else {
    // Start failed!
    delegate_->OnServerStartComplete(result_info);
  }
}

void EmergencyLogServerImpl::Tick()
{
  CScopedLock2 guard(delegate_->GetMutex());

  delegate_->OnTick();
}

float EmergencyLogServerImpl::GetElapsedTime()
{
  return elapsed_time_;
}

void EmergencyLogServerImpl::OnClientJoined(const NetClientInfo* client_info)
{
  CScopedLock2 guard(delegate_->GetMutex());

  EmergencyPtr_S new_client(new EmergencyClient_S(this, client_info->host_id_));
  clients_.Add(client_info->host_id_, new_client);
}

void EmergencyLogServerImpl::OnClientLeft(const NetClientInfo* client_info,
                                          const ResultInfo* result_info,
                                          const ByteArray& comment)
{
  CScopedLock2 guard(delegate_->GetMutex());

  clients_.Remove(client_info->host_id_);
}

DEFRPC_EmergencyC2S_EmergencyLogData_Begin(EmergencyLogServerImpl)
{
  CScopedLock2 guard(delegate_->GetMutex());

  if (auto client = GetClientByHostId_NOLOCK(rpc_recvfrom)) {
    client->log_data_.logon_time = DateTime(logon_time);
    client->log_data_.connect_count = connect_count;
    client->log_data_.remote_peer_count = remote_peer_count;
    client->log_data_.direct_p2p_enable_peer_count = direct_p2p_enable_peer_count;
    client->log_data_.nat_device_name = nat_device_name;
    client->log_data_.host_id = peer_id;
    client->log_data_.io_pending_count = io_pending_count;
    client->log_data_.total_tcp_issued_send_bytes = total_tcp_issued_send_bytes;
  }
  return true;
}

DEFRPC_EmergencyC2S_EmergencyLogData_Error(EmergencyLogServerImpl)
{
  CScopedLock2 guard(delegate_->GetMutex());

  if (auto client = GetClientByHostId_NOLOCK(rpc_recvfrom)) {
    client->log_data_.msg_size_error_count = msg_size_error_count;
    client->log_data_.conn_reset_error_count = conn_reset_error_count;
    client->log_data_.net_reset_error_count = net_reset_error_count;
    client->log_data_.last_error_completion_length = last_error_completion_length;
  }
  return true;
}

DEFRPC_EmergencyC2S_EmergencyLogData_Stats(EmergencyLogServerImpl)
{
  CScopedLock2 guard(delegate_->GetMutex());

  if (auto client = GetClientByHostId_NOLOCK(rpc_recvfrom)) {
    client->log_data_.total_tcp_recv_bytes = total_tcp_recv_bytes;
    client->log_data_.total_tcp_send_bytes = total_tcp_send_bytes;
    client->log_data_.total_udp_recv_bytes = total_udp_recv_bytes;
    client->log_data_.total_udp_recv_count = total_udp_recv_count;
    client->log_data_.total_udp_send_bytes = total_udp_send_bytes;
    client->log_data_.total_udp_send_count = total_udp_send_count;
  }
  return true;
}

DEFRPC_EmergencyC2S_EmergencyLogData_OSVersion(EmergencyLogServerImpl)
{
  CScopedLock2 guard(delegate_->GetMutex());

  if (auto client = GetClientByHostId_NOLOCK(rpc_recvfrom)) {
    client->log_data_.os_major_version = os_major_version;
    client->log_data_.os_minor_version = os_minor_version;
    client->log_data_.processor_architecture = processor_architecture;
    client->log_data_.product_type = product_type;
  }
  return true;
}

DEFRPC_EmergencyC2S_EmergencyLogData_LogEvent(EmergencyLogServerImpl)
{
  CScopedLock2 guard(delegate_->GetMutex());

  if (auto client = GetClientByHostId_NOLOCK(rpc_recvfrom)) {
    EmergencyLogData::Log log;
    log.category = (LogCategory)category;
    log.added_time = DateTime(added_time);
    log.text = text;

    client->log_data_.log_list.Add(log);
  }
  return true;
}

DEFRPC_EmergencyC2S_EmergencyLogData_End(EmergencyLogServerImpl)
{
  CScopedLock2 guard(delegate_->GetMutex());

  if (auto client = GetClientByHostId_NOLOCK(rpc_recvfrom)) {
    // 나머지 로그 갱신
    client->log_data_.server_udp_addr_count = server_udp_addr_count;
    client->log_data_.remote_udp_addr_count = remote_udp_addr_count;

    // replace logging file.
    int64 size = log_file_->Size();
    if (size >= LOG_FILE_SIZE) {
      log_file_.Reset();

      // create a new logging file.
      CreateLogFile();

      // write logging information into file.
      WriteClientLog(rpc_recvfrom, client->log_data);
    }
    else {
      WriteClientLog(rpc_recvfrom, client->log_data);
    }

    s2c_proxy_.EmergencyLogData_AckComplete(rpc_recvfrom, GReliableSend_INTERNAL);
  }

  return true;
}

bool EmergencyLogServerImpl::CreateLogFile()
{
  String filename = String::Format("FunNetEmergencyLog_%d", log_file_count_++);
  IFile* file = IPlatformFS::GetPlatformPhysical().OpenWrite(filename);
  if (file == nullptr) {
    return false;
  }

  log_file_.Free();
  log_file_ = THeldPtr<IFile>(file);

  // write unicode BOM.
#ifdef _UNICODE
  uint8 BOM[2];
  BOM[0] = 0xFF;
  BOM[1] = 0xFE;
  log_file_->Write(BOM, sizeof(BOM));
#endif

  // seek to end of file.
  log_file_->SeekFromEnd(0);

  return true;
}

void EmergencyLogServerImpl::WriteClientLog(HostId host_id, const EmergencyLogData& data)
{
  //@todo 너무 장황한 부분이 없지않아..

  // 클라이언트 하나의 정보를 순서대로 적는다.
  String text;
  text.Format("%s | %s | %s | client host_id:%d, iopendingcount:%d, totalTcpSendBytes:%u, connect_count:%u, nat_device_name:%s\r\n", (const char*)DateTime(data.logon_time).ToString(),
    "INFO", "FunNet.Clientinfo", data.host_id, data.io_pending_count, data.total_tcp_issued_send_bytes_, data.connect_count, data.nat_device_name);
  log_file_->Write((const char*)text, sizeof(char)*text.GetLength());

  text.Format("%s | %s | %s | remote_peer_count:%u, DirectPeerCount:%u\r\n", (const char*)DateTime(data.logon_time).ToString(),
    "INFO", "FunNet.Clientinfo.p2pstat", data.remote_peer_count, data.direct_p2p_enable_peer_count);
  log_file_->Write((const char*)text, sizeof(char)*text.GetLength());

  text.Format("%s | %s | %s | TotalTcpRecevieBytes:%u, total_tcp_send_bytes:%u, total_udp_recv_bytes:%u, total_udp_send_bytes:%u, total_udp_recv_count:%u, total_udp_send_count:%u\r\n",
    (const char*)DateTime(data.logon_time).ToString(),
    "INFO", "FunNet.Clientinfo.netstat", data.total_tcp_recv_bytes, data.total_tcp_send_bytes,
    data.total_udp_recv_bytes, data.total_udp_send_bytes, data.total_udp_recv_count, data.total_udp_send_count);
  log_file_->Write((const char*)text, sizeof(char)*text.GetLength());

  text.Format("%s | %s | %s | OSMajorVersion:%u, OSMinorVersion:%u, ProcessArchitecture:%u, product_type:%u \r\n", (const char*)DateTime(data.logon_time).ToString(),
    "INFO", "FunNet.Clientinfo.os", data.os_major_version, data.os_minor_version,
    data.processor_architecture, data.product_type);
  log_file_->Write((const char*)text, sizeof(char)*text.GetLength());

  text.Format("%s | %s | %s | server_udp_addr_count:%u, remote_udp_addr_count:%u\r\n", (const char*)DateTime(data.logon_time).ToString(),
    "INFO", "FunNet.Clientinfo.udpaddrcount", data.server_udp_addr_count, data.remote_udp_addr_count);
  log_file_->Write((const char*)text, sizeof(char)*text.GetLength());

  text.Format("%s | %s | %s | MsgsizeErrorCount:%u, conn_reset_error_count:%u, net_reset_error_count:%u, GetLastErrorToCompletionDataLength:%u\r\n", (const char*)DateTime(data.logon_time).ToString(),
    "ERROR", "FunNet.Clientinfo.error", data.msg_size_error_count, data.conn_reset_error_count, data.net_reset_error_count,
    data.last_error_completion_length);
  log_file_->Write((const char*)text, sizeof(char)*text.GetLength());

  for (auto i = data.LogList.begin(); i != data.LogList.end(); ++i) {
    text.Format("%s | %s | %s | Category:%d log:%s\r\n", (const char*)DateTime((*i).added_time).ToString(),
      "LOG", "FunNet.Clientinfo.emergencylog", (*i).Category, (*i).text);
    log_file_->Write((const char*)text, sizeof(WCHAR)*text.GetLength());
  }
}

EmergencyPtr_S EmergencyLogServerImpl::GetClientByHostId_NOLOCK(HostId host_id)
{
  CScopedLock2 guard(delegate_->GetMutex());
  return clients_.FindRef(host_id);
}

EmergencyLogServer* EmergencyLogServer::New(IEmergencyLogServerDelegate* delegate)
{
  return new EmergencyLogServerImpl(delegate);
}

EmergencyClient_S::EmergencyClient_S(EmergencyLogServerImpl* server, HostId host_id)
{
  server_ = server;
  host_id_ = host_id;
}

EmergencyClient_S::~EmergencyClient_S()
{
}

} // namespace net
} // namespace fun
