#include "CorePrivatePCH.h"
#include "fun/net/net.h"

#include "LanRemoteClient.h"
#include "LanServer.h"

namespace fun {
namespace net {

LanClient_S::LanClient_S(LanServerImpl* owner, InternalSocket* new_socket,
                         const InetAddress& remote_addr) {
  owner_ = owner;

  last_request_measure_send_speed_time_ = 0;
  send_speed_ = 0;

  dispose_caller_ = nullptr;
  created_time_ = owner_->GetAbsoluteTime();

  max_direct_p2p_connect_count_ =
      NetConfig::default_max_direct_p2p_multicast_count;
  host_id_ = HostId_None;
  purge_requested_ = false;
  task_running_ = false;
  last_ping_ = 0;
  recent_ping = 0;
  send_queued_amount_in_byte_ = 0;
  last_tcp_stream_recv_time_ = created_time_;

  encrypt_count = 0;
  decrypt_count_ = 0;
  session_key_received_ = false;

  request_auto_prune_start_time_ = 0;

  to_client_tcp_ = new TcpTransport_S(this, new_socket, remote_addr);
  to_client_tcp_->SetEnableNagleAlgorithm(
      owner_->settings_.bEnableNagleAlgorithm);
}

ResultCode LanClient_S::ExtractMessagesFromTcpStream(
    ReceivedMessageList& out_result) {
  out_result.Clear();

  owner_->AssertIsNotLockedByCurrentThread();
  to_client_tcp_->AssertIsLockedByCurrentThread();

  ResultCode rc;
  const int32 added_count = MessageStream::ExtractMessagesAndFlushStream(
      to_client_tcp_->recv_stream_, out_result, host_id_,
      owner_->settings_.message_max_length, rc);

  if (added_count < 0) {
    return rc;
  }

  return ResultCode::Ok;
}

void LanClient_S::GetClientInfo(NetClientInfo& out_info) {
  owner_->AssertIsLockedByCurrentThread();

  out_info.host_id_ = host_id_;
  out_info.tcp_addr_from_server = to_client_tcp_->remote_addr_;
  out_info.recent_ping = recent_ping_;
  out_info.send_queued_amount_in_byte_ = send_queued_amount_in_byte_;
  out_info.host_tag = host_tag_;

  out_info.joined_p2p_groups_.Clear(joined_lan_p2p_groups_.Count() +
                                    had_joined_p2p_groups_.Count());

  for (const auto& pair : joined_lan_p2p_groups_) {
    out_info.joined_p2p_groups_.Add(pair.key);
  }

  for (int32 i = 0; i < had_joined_p2p_groups_.Count(); ++i) {
    out_info.joined_p2p_groups_.Add(had_joined_p2p_groups_[i]);
  }

  out_info.relayed_p2p = false;

  // TODO 이게 구태여 필요할까???
  out_info.host_id_recycle_count =
      owner_->host_id_factory_->GetRecycleCount(host_id_);
}

NetClientInfoPtr LanClient_S::GetClientInfo() {
  NetClientInfoPtr info(new NetClientInfo);
  GetClientInfo(*info);
  return info;
}

bool LanClient_S::IsFinalReceiveQueueEmpty() {
  owner_->AssertIsLockedByCurrentThread();

  return final_user_work_queue_.IsEmpty();
}

bool LanClient_S::IsTaskRunning() {
  owner_->AssertIsLockedByCurrentThread();
  return task_running_;
}

bool LanClient_S::PopFirstUserWorkItem(FinalUserWorkItem& out_item) {
  owner_->AssertIsLockedByCurrentThread();

  if (!final_user_work_queue_.IsEmpty()) {
    out_item.From(final_user_work_queue_.Front(),
                  host_id_);  // From이라는걸 써야하나... 진정..
    final_user_work_queue_.RemoveFront();
    return true;
  }
  return false;
}

void LanClient_S::OnSetTaskRunningFlag(bool running) {
  owner_->AssertIsLockedByCurrentThread();
  task_running_ = running;
}

LanClient_S::~LanClient_S() {
  owner_->AssertIsLockedByCurrentThread();

  lan_p2p_connection_pairs_.Clear();

  UnlinkSelf();
  task_subject_node_.UnlinkSelf();

  delete to_client_tcp_;
}

// 너무 단시간내에 끊기는 경우에는 churn상황인지 경고성으로 알려줌.
void LanClient_S::WarnTooShortDisposal(const char* where) {
  if (dispose_caller_ == nullptr) {
    dispose_caller_ = where;

    if ((owner_->GetAbsoluteTime() - created_time_) <
        0.3) {  // TODO 수치가 하드 코딩 되어 있음...
      if (owner_->intra_logger_) {
        const String text = String::Format(
            "A suspicious part has been found."
            "  The connected client disconnected in a short time after "
            "connecting. : %s",
            where);
        owner_->intra_logger_->WriteLine(LogCategory::System, *text);
      }
    }
  }
}

void LanClient_S::EnqueueLocalEvent(LocalEvent& event) {
  if (owner_) {
    owner_->AssertIsLockedByCurrentThread();

    if (owner_->listener_thread_) {
      final_user_work_queue_.Enqueue(event);
      owner_->user_task_queue_.AddTaskSubject(this);
    }
  }
}

void LanClient_S::EnqueueUserTask(Function<void()> func) {
  if (owner_) {
    owner_->AssertIsLockedByCurrentThread();

    if (owner_->listener_thread_) {
      final_user_work_queue_.Enqueue(func);
      owner_->user_task_queue_.AddTaskSubject(this);
    }
  }
}

void LanClient_S::LockMain_AssertIsLockedByCurrentThread() {
  owner_->AssertIsLockedByCurrentThread();
}

void LanClient_S::LockMain_AssertIsNotLockedByCurrentThread() {
  owner_->AssertIsNotLockedByCurrentThread();
}

void LanClient_S::EnqueueIssueSendReadyRemotes() {
  CScopedLock2 tcp_issue_queue_guard(owner_->tcp_issue_queue_mutex_);

  if (GetListOwner() == nullptr) {
    owner_->tcp_issue_send_ready_remote_clients_.Append(this);
  }
}

//@todo 이름을 살짝 변경해주는게 어떨런지..?
bool LanClient_S::IsDispose() { return dispose_waiter_ != nullptr; }

double LanClient_S::GetAbsoluteTime() { return owner_->GetAbsoluteTime(); }

void LanClient_S::IssueDispose(ResultCode result_code, ResultCode detail_code,
                               const ByteArray& comment, const char* where,
                               SocketErrorCode socket_error) {
  owner_->IssueDisposeLanClient(this, result_code, detail_code, comment, where,
                                socket_error);
}

//@todo 이름을 살짝 변경해주는게 어떨런지..?
bool LanClient_S::IsValidEnd() {
  return owner_->tcp_listening_socket_.IsValid() == false;
}

SocketErrorCode LanClient_S::IssueSend(double absolute_time) {
  to_client_tcp_->AssertIsLockedByCurrentThread();

  return to_client_tcp_->ConditionalIssueSend(absolute_time);
}

CCriticalSection2& LanClient_S::GetSendMutex() {
  return to_client_tcp_->GetSendQueueMutex();
}

CCriticalSection2& LanClient_S::GetMutex() {
  return to_client_tcp_->GetMutex();
}

void LanClient_S::Decrease() { DecreaseUseCount(); }

void LanClient_S::OnIssueSendFail(const char* where,
                                  SocketErrorCode socket_error) {
  IssueDispose(ResultCode::DisconnectFromRemote, ResultCode::TCPConnectFailure,
               ByteArray(), __FUNCTION__, socket_error);

  WarnTooShortDisposal(where);
}

void LanClient_S::SendWhenReady(HostId sender_id,
                                const InetAddress& sender_addr, HostId dest_id,
                                const SendFragRefs& data,
                                const UdpSendOptions& option) {
  fun_check(0);  //사용하지 말것!!
}

bool LanClient_S::IsLockedByCurrentThread() {
  // TODO 뭐지 이거 왜 두번이나 하는거지, 오타인가??
  return to_client_tcp_->GetMutex().IsLockedByCurrentThread() ||
         to_client_tcp_->GetMutex().IsLockedByCurrentThread();
}

}  // namespace net
}  // namespace fun
