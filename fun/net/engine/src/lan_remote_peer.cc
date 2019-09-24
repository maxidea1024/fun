#include "CorePrivatePCH.h"
#include "LanClient.h"
#include "LanRemotePeer.h"
#include "fun/net/net.h"

namespace fun {
namespace net {

LanRemotePeer_C::LanRemotePeer_C(LanClientImpl* owner)
    : tcp_transport_(this, nullptr, InetAddress::None), owner_(owner) {
  dispose_caller_ = nullptr;
  created_time_ =
      owner_->GetAbsoluteTime();  // owner_->clock_.AbsoluteSeconds();
  last_ping_ = 0;
  //접속이 몰릴 경우(서버 재기동 같은 경우) 한번에 몰려서 전송이 되는 것을
  //방지하기 위해서 랜덤으로 간격을 흩어놓음.
  sync_indirect_server_time_diff_cooltime_ = MathBase::Lerp(
      NetConfig::p2p_ping_interval_sec * 0.5, NetConfig::p2p_ping_interval_sec,
      owner_->random_.NextDouble());
  // send_queued_amount_in_byte_ = 0;
  recent_ping_ = 0;
  last_report_p2p_peer_ping_cooltime_ = 0;
  last_tcp_stream_recv_time_ = 0;
  task_running_ = false;
  dispose_requested_ = false;
  encrypt_count = 0;
  decrypt_count_ = 0;
}

LanRemotePeer_C::~LanRemotePeer_C() {
  LockMain_AssertIsLockedByCurrentThread();
}

void LanRemotePeer_C::CreateP2PHolepunchAttemptContext() {
  // NOTHING TO DO
}

void LanRemotePeer_C::Heartbeat(double absolute_time) {
  (void)absolute_time;

  // NOTHING TO DO
}

double LanRemotePeer_C::GetIndirectServerTimeDiff() {
  if (host_id_ == HostId_Server) {
    return owner_->GetServerTimeDiff();
  } else {
    if (indirect_server_time_diff_ == 0) {
      indirect_server_time_diff_ = owner_->GetServerTimeDiff();
    }

    return indirect_server_time_diff_;
  }
}

int32 LanRemotePeer_C::GetOverSendSuspectingThresholdInByte() {
  return owner_->settings_.over_send_suspecting_threshold_in_byte;
}

ResultCode LanRemotePeer_C::ExtractMessagesFromTcpStream(
    ReceivedMessageList& out_result) {
  out_result.Clear();  // just in case

  tcp_transport_.AssertIsLockedByCurrentThread();

  ResultCode rc = ResultCode::Ok;
  const int32 added_count = MessageStream::ExtractMessagesAndFlushStream(
      tcp_transport_.recv_stream_, out_result, host_id_,
      owner_->settings_.message_max_length, rc);

  if (added_count < 0) {
    return rc;
  }

  return ResultCode::Ok;
}

bool LanRemotePeer_C::IsFinalReceiveQueueEmpty() {
  owner_->LockMain_AssertIsLockedByCurrentThread();
  return final_user_work_queue_.IsEmpty();
}

bool LanRemotePeer_C::IsTaskRunning() {
  owner_->LockMain_AssertIsLockedByCurrentThread();
  return task_running_;
}

void LanRemotePeer_C::OnSetTaskRunningFlag(bool running) {
  owner_->LockMain_AssertIsLockedByCurrentThread();
  task_running_ = running;
}

bool LanRemotePeer_C::PopFirstUserWorkItem(FinalUserWorkItem& out_item) {
  owner_->LockMain_AssertIsLockedByCurrentThread();

  if (!final_user_work_queue_.IsEmpty()) {
    // TODO From이라는걸 꼭 써야하나??
    out_item.From(final_user_work_queue_.Front(), host_id_);
    final_user_work_queue_.RemoveFront();
    return true;
  }
  return false;
}

void LanRemotePeer_C::GetPeerInfo(PeerInfo& out_info) {
  out_info.host_id = host_id_;
  out_info.recent_ping = recent_ping;
  out_info.host_tag = host_tag;
  // out_info.send_queued_amount_in_byte = send_queued_amount_in_byte_;

  out_info.joined_p2p_groups_.Clear(joined_p2p_groups_.Count());
  for (const auto& pair : joined_p2p_groups_) {
    const HostId group_id = pair.key;
    out_info.joined_p2p_groups_.Add(group_id);
  }

  // TODO TSet대응
  // joined_p2p_groups_.GenerateKeyArray(out_info.joined_p2p_groups);
}

NetPeerInfoPtr LanRemotePeer_C::GetPeerInfo() {
  NetPeerInfoPtr info(new PeerInfo);
  GetPeerInfo(*info);
  return info;
}

void LanRemotePeer_C::WarnTooShortDisposal(const char* where) {
  if (dispose_caller_ == nullptr) {
    dispose_caller_ = where;

    if ((owner_->GetAbsoluteTime() - created_time_) <
        0.3) {  // TODO 수치가 하드코딩 되어 있음, 별도로 빼주는것도 좋을듯...
      if (owner_->intra_logger_) {
        const String text = String::Format(
            "A suspicious part has been found.  "
            "The connected client disconnected in "
            "a short time after connecting. : %s",
            where);
        owner_->intra_logger_->WriteLine(LogCategory::System, *text);
      }
    }
  }
}

void LanRemotePeer_C::LockMain_AssertIsLockedByCurrentThread() {
  owner_->LockMain_AssertIsLockedByCurrentThread();
}

void LanRemotePeer_C::LockMain_AssertIsNotLockedByCurrentThread() {
  owner_->LockMain_AssertIsNotLockedByCurrentThread();
}

void LanRemotePeer_C::EnqueueIssueSendReadyRemotes() {
  CScopedLock2 tcp_issue_queue_guard(owner_->tcp_issue_queue_mutex_);

  if (GetListOwner() == nullptr) {
    owner_->tcp_issue_send_ready_remote_peers_.Append(this);
  }
}

bool LanRemotePeer_C::IsDispose() { return dispose_waiter_.IsValid(); }

double LanRemotePeer_C::GetAbsoluteTime() { return owner_->GetAbsoluteTime(); }

void LanRemotePeer_C::IssueDispose(ResultCode result_code,
                                   ResultCode detail_code,
                                   const ByteArray& comment, const char* where,
                                   SocketErrorCode socket_error) {
  CScopedLock2 owner_guard(owner_->GetMutex());
  owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

  owner_->NotifyP2PDisconnected(this, result_code);
  dispose_requested_ = true;
}

bool LanRemotePeer_C::IsValidEnd() {
  return owner_->tcp_listening_socket_.IsValid() == false;
}

void LanRemotePeer_C::EnqueueLocalEvent(LocalEvent& event) {
  owner_->LockMain_AssertIsLockedByCurrentThread();

  if (owner_ && owner_->listener_thread_.IsValid()) {
    final_user_work_queue_.Enqueue(event);
    owner_->user_task_queue_.AddTaskSubject(this);
  }
}

void LanRemotePeer_C::EnqueueUserTask(Function<void()> func) {
  owner_->LockMain_AssertIsLockedByCurrentThread();

  if (owner_ && owner_->listener_thread_.IsValid()) {
    final_user_work_queue_.Enqueue(func);
    owner_->user_task_queue_.AddTaskSubject(this);
  }
}

bool LanRemotePeer_C::IsLockedByCurrentThread() {
  return tcp_transport_.IsLockedByCurrentThread() ||
         tcp_transport_.IsSendQueueLockedByCurrentThread();
}

SocketErrorCode LanRemotePeer_C::IssueSend(double absolute_time) {
  tcp_transport_.AssertIsLockedByCurrentThread();

  return tcp_transport_.ConditionalIssueSend(absolute_time);
}

CCriticalSection2& LanRemotePeer_C::GetSendMutex() {
  return tcp_transport_.GetSendQueueMutex();
}

CCriticalSection2& LanRemotePeer_C::GetMutex() {
  return tcp_transport_.GetMutex();
}

void LanRemotePeer_C::Decrease() { DecreaseUseCount(); }

void LanRemotePeer_C::OnIssueSendFail(const char* where,
                                      SocketErrorCode socket_error) {
  LockMain_AssertIsLockedByCurrentThread();

  IssueDispose(ResultCode::DisconnectFromRemote, ResultCode::TCPConnectFailure,
               ByteArray(), __FUNCTION__, socket_error);

  WarnTooShortDisposal(where);
}

void LanRemotePeer_C::SendWhenReady(HostId sender_id,
                                    const InetAddress& sender_addr,
                                    HostId dest_id, const SendFragRefs& data,
                                    const UdpSendOption& option) {
  fun_check(0);  //이함수는 사용하면 안됩니다.
}

void AcceptedInfo::ExtractMessagesFromTcpStream(
    ReceivedMessageList& out_result) {
  owner_->LockMain_AssertIsNotLockedByCurrentThread();
  AssertIsLockedByCurrentThread();

  out_result.Clear();  // Just in case

  ResultCode rc;
  const int32 added_count = MessageStream::ExtractMessagesAndFlushStream(
      recv_stream_, out_result, HostId_None,
      owner_->settings_.message_max_length, rc);
  if (added_count < 0) {
    // 해당 클라와의 TCP 통신이 더 이상 불가능한 상황이다.
    // owner_->(this, rc, ResultCode::TCPConnectFailure, ByteArray(),
    // __FUNCTION__, SocketErrorCode::Ok); //InduceDisconnect();
  }
}

AcceptedInfo::AcceptedInfo(LanClientImpl* owner)
    : owner_(owner),
      recv_stream_(NetConfig::stream_grow_by),
      is_timedout_(false),
      recv_issued_(false),
      accepted_time(0) {}

void AcceptedInfo::IssueRecvAndCheck() {
  owner_->LockMain_AssertIsNotLockedByCurrentThread();
  AssertIsLockedByCurrentThread();

  // 이미 닫힌 소켓이면 issue를 하지 않는다.
  // 만약 닫힌 소켓에 issue하면 completion이 어쨌거나 또 발생한다.
  // 메모리도 긁으면서.
  fun_check(!recv_issued_);

  if (!recv_issued_ && !socket_->IsClosedOrClosing()) {
    if (!is_timedout_) {
      recv_issued_ = true;
      const SocketErrorCode socket_error =
          socket_->IssueRecv(NetConfig::tcp_issue_recv_length);
      if (socket_error != SocketErrorCode::Ok) {
        recv_issued_ = false;
        socket_->CloseSocketHandleOnly();
        // owner_->WarnTooShortDisposal(__FUNCTION__);

        if (owner_->intra_logger_) {
          const String text = String::Format(
              "CloseSocketHandleOnly called at %s", __FUNCTION__);
          owner_->intra_logger_->WriteLine(LogCategory::System, *text);
        }
      }
    }
  }
}

}  // namespace net
}  // namespace fun
