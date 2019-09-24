#include "../NetEnginePrivate.h"

#include "Net/Engine/Apps/EmergencyLogClient.h"

#include "GeneratedRPCs/Emergency_EmergencyS2C_stub.cc"
#include "GeneratedRPCs/Emergency_EmergencyC2S_proxy.cc"

namespace fun {
namespace net {

EmergencyLogClient::EmergencyLogClient()
{
  log_data_ = nullptr;

  state_ = State::Connecting;

  client_.Attach(NetClient::New());
  client_->AttachProxy(&c2s_proxy_);
  client_->AttachStub(this);
  client_->SetCallbacks(this);
}

EmergencyLogClient::~EmergencyLogClient()
{
  client_.Reset();
}

void EmergencyLogClient::OnJoinedToServer(const ResultInfo* result_info, const ByteArray& reply_from_server)
{
  if (result_info->result_code != ResultCode::Ok) {
    //@todo 에러코드를 같이 알려줄 필요가 있을듯 싶은데..
    throw Exception("couldn't connect to emergency-log server.");
  }
  else {
    // 모아둔 로그를 보내자. 크기는 크지 않을테니 한방에 보낸다.
    state_ = State::Sending;

    if (log_data_) {
      //@todo DateTime -> int64
      //c2s_proxy_.EmergencyLogData_Begin(HostId_Server, GReliableSend_INTERNAL,
      //  log_data_->logon_time,
      //  log_data_->connect_count,
      //  log_data_->remote_peer_count,
      //  log_data_->direct_p2p_enable_peer_count,
      //  log_data_->nat_device_name,
      //  log_data_->host_id,
      //  log_data_->io_pending_count,
      //  log_data_->total_tcp_issued_send_bytes
      //  );

      c2s_proxy_.EmergencyLogData_Begin(HostId_Server, GReliableSend_INTERNAL,
                  0,
                  log_data_->connect_count,
                  log_data_->remote_peer_count,
                  log_data_->direct_p2p_enable_peer_count,
                  log_data_->nat_device_name,
                  log_data_->host_id_,log_data->io_pending_count,
                  (int32)log_data_->total_tcp_issued_send_bytes);

      c2s_proxy_.EmergencyLogData_Error(HostId_Server, GReliableSend_INTERNAL,
                  log_data_->msg_size_error_count,
                  log_data_->net_reset_error_count,
                  log_data_->conn_reset_error_count,
                  log_data_->last_error_completion_length);

      c2s_proxy_.EmergencyLogData_Stats(HostId_Server, GReliableSend_INTERNAL,
                  log_data_->total_tcp_recv_bytes,
                  log_data_->total_tcp_send_bytes,
                  log_data_->total_udp_send_count,
                  log_data_->total_udp_send_bytes,
                  log_data_->total_udp_recv_count,
                  log_data_->total_udp_recv_bytes);

      c2s_proxy_.EmergencyLogData_OSVersion(HostId_Server, GReliableSend_INTERNAL,
                  log_data_->os_major_version,
                  log_data_->os_minor_version,
                  log_data_->product_type,
                  log_data_->processor_architecture);

      for (const auto& log : log_data_->log_list) {
        //@todo DateTime -> int64
        //c2s_proxy_.EmergencyLogData_LogEvent(HostId_Server, GReliableSend_INTERNAL, log.category, log.added_time, log.text);
        c2s_proxy_.EmergencyLogData_LogEvent(HostId_Server, GReliableSend_INTERNAL, log.category, 0, log.text);
      }

      c2s_proxy_.EmergencyLogData_End(HostId_Server, GReliableSend_INTERNAL, log_data_->server_udp_addr_count, log_data_->remote_udp_addr_count);
    }
  }
}

DEFRPC_EmergencyS2C_EmergencyLogData_AckComplete(EmergencyLogClient)
{
  // log 도착한게 확인됐다. 클라를 닫자.
  state_ = State::Closing;
  return true;
}

void EmergencyLogClient::OnLeftFromServer(const ResultInfo* result_info)
{
  state_ = State::Stopped;
}

void EmergencyLogClient::OnP2PMemberJoined( HostId member_id,
                                            HostId group_id,
                                            int32 member_count,
                                            const ByteArray& custom_field)
{
  // NOOP
}

void EmergencyLogClient::OnP2PMemberLeft(HostId member_id, HostId group_id, int32 member_count)
{
  // NOOP
}

void EmergencyLogClient::OnException(HostId host_id, const Exception& e)
{
  // Emergencylog를 보낼수 없는 상황이므로 종료 하도록한다.
  state_ = State::Closing;
}

void EmergencyLogClient::OnError(const ResultInfo* result_info)
{
  // Emergencylog를 보낼수 없는 상황이므로 종료 하도록한다.
  state_ = State::Closing;
}

void EmergencyLogClient::Start(const String& server_ip, int32 server_port, EmergencyLogData* log_data)
{
  log_data_ = log_data;

  NetConnectionArgs args;
  args.protocol_version = Uuid(EMERGENCY_PROTOCOL_VERSION);
  args.server_ip = server_ip;
  args.server_port = server_port;

  if (!client_->Connect(args)) {
    throw Exception("couldn't connect to emergency-log server.");
  }
}

void EmergencyLogClient::Tick()
{
  client_->Tick();

  if (state_ == State::Closing) {
    client_->Disconnect();

    state_ = State::Stopped;
  }
}

EmergencyLogClient::State EmergencyLogClient::GetState() const
{
  return state_;
}

} // namespace net
} // namespace fun
