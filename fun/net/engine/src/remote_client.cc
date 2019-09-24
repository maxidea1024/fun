#include "NetServer.h"
#include "RemoteClient.h"
#include "TcpTransport_S.h"
#include "fun/net/net.h"

namespace fun {
namespace net {

RemoteClient_S::RemoteClient_S(NetServerImpl* owner, InternalSocket* new_socket,
                               const InetAddress& tcp_remote_addr)
    : to_client_udp_fallbackable_(this) {
  fun_check_ptr(owner);
  fun_check_ptr(new_socket);

  owner_ = owner;

  borrowed_port_number_ = 0;

  // Per RC UDP socket이라면 이 RC가 직접 소유권을 가진다.
  // if (owner_->udp_assign_mode == ServerUdpAssignMode::PerClient) {
  //  owned_udp_socket_ = assigned_udp_socket;
  //}

  last_request_measure_send_speed_time_ = 0;
  send_speed_ = 0;

  dispose_caller_ = nullptr;
  created_time_ = owner_->GetAbsoluteTime();

  max_direct_p2p_connection_count_ =
      NetConfig::default_max_direct_p2p_multicast_count;
  host_id_ = HostId_None;
  purge_requested_ = false;
  task_running_ = false;
  last_ping_ = 0;
  recent_ping_ = 0;
  send_queued_amount_in_byte_ = 0;
  last_tcp_stream_recv_time_ = created_time_;
  last_udp_packet_recv_time_ = created_time_;
  arbitrary_udp_touched_time_ = created_time_;
  last_udp_ping_recv_time_ = created_time_;

  encrypt_count_ = 0;
  decrypt_count_ = 0;
  session_key_received_ = false;

  super_peer_rating_ = 0;
  last_application_hint_.recent_frame_rate = 0;

  request_auto_prune_start_time_ = 0;

  to_remote_peer_send_udp_message_attempt_count_ = 0;
  to_remote_peer_send_udp_message_success_count_ = 0;
  to_server_send_udp_message_attempt_count_ = 0;
  to_server_send_udp_message_success_count_ = 0;

  send_queue_warning_start_time_ = 0;

  if (NetConfig::speedhack_detector_enabled_by_default) {
    speed_hack_detector_.Reset(new SpeedHackDetector);
  }

  to_client_tcp = new TcpTransport_S(this, new_socket, tcp_remote_addr);
  to_client_tcp_->SetEnableNagleAlgorithm(
      owner_->settings_.bEnableNagleAlgorithm);
}

ResultCode RemoteClient_S::ExtractMessagesFromTcpStream(
    ReceivedMessageList& out_result) {
  owner_->AssertIsNotLockedByCurrentThread();
  to_client_tcp_->AssertIsLockedByCurrentThread();

  out_result.Reset();  // Just in case

  ResultCode extract_result;
  const int32 added_count = MessageStream::ExtractMessagesAndFlushStream(
      to_client_tcp_->recv_stream_, out_result, host_id_,
      owner_->settings_.message_max_length, extract_result);
  if (added_count < 0) {
    return extract_result;
  }

  return ResultCode::Ok;
}

void RemoteClient_S::ExtractMessagesFromUdpRecvQueue(
    const uint8* udp_packet, int32 udp_packet_length,
    const InetAddress& udp_addr_from_here, int32 message_max_length,
    ReceivedMessageList& out_result) {
  out_result.Reset();  // just in case

  // coalesce를 감안해서 처리한다.
  const int32 last_extractee = out_result.Count();

  MessageStreamExtractor extractor;
  extractor.input = udp_packet;
  extractor.input_length = udp_packet_length;
  extractor.output = &out_result;
  extractor.message_max_length = message_max_length;
  extractor.sender_id = HostId_None;

  ResultCode extract_result;
  const int32 added_count = extractor.Extract(extract_result);
  if (added_count >= 0) {
    for (int32 i = 0; i < added_count; ++i) {
      auto& received_msg = out_result[last_extractee + i];

      received_msg.remote_addr_udp_only = udp_addr_from_here_;
      received_msg.remote_id = host_id_;
    }
  } else {
    // 잘못된 스트림 데이터이다. UDP인 경우에는 모두 처리된 것처럼 간주하고 그냥
    // 무시해버린다.
    int32 X = 0;
  }
}

void RemoteClient_S::GetClientInfo(NetClientInfo& out_info) {
  owner_->AssertIsLockedByCurrentThread();

  out_info.host_id = host_id_;
  out_info.udp_addr_from_server =
      to_client_udp_fallbackable_.GetUdpAddrFromHere();
  out_info.tcp_addr_from_server = to_client_tcp_->remote_addr_;
  out_info.udp_addr_internal = to_client_udp_fallbackable_.udp_addr_internal_;
  out_info.recent_ping = recent_ping_;
  out_info.send_queued_amount_in_byte = send_queued_amount_in_byte_;
  out_info.real_udp_enabled = to_client_udp_fallbackable_.real_udp_enabled_;

  for (auto& pair : joined_p2p_groups_) {
    out_info.joined_p2p_groups_.Add(pair.key);
  }

  for (auto group : had_joined_p2p_groups_) {
    out_info.joined_p2p_groups_.Add(group);
  }

  out_info.relayed_p2p = false;
  out_info.behind_nat = IsBehindNAT();
  out_info.nat_device_name = nat_device_name_;
  out_info.host_tag = host_tag_;
  out_info.recent_frame_rate = last_application_hint_.recent_frame_rate;

  out_info.host_id_recycle_count =
      owner_->host_id_factory_->GetRecycleCount(host_id_);
  out_info.to_server_send_udp_message_attempt_count =
      to_server_send_udp_message_attempt_count_;
  out_info.to_server_send_udp_message_success_count =
      to_server_send_udp_message_success_count_;
}

bool RemoteClient_S::IsBehindNAT() {
  return to_client_udp_fallbackable_.GetUdpAddrFromHere().GetHost() !=
         to_client_udp_fallbackable_.udp_addr_internal_.GetHost();
}

NetClientInfoPtr RemoteClient_S::GetClientInfo() {
  NetClientInfoPtr ret(new NetClientInfo);
  GetClientInfo(*ret);
  return ret;
}

bool RemoteClient_S::IsFinalReceiveQueueEmpty() {
  owner_->AssertIsLockedByCurrentThread();

  return final_user_work_queue_.IsEmpty();
}

bool RemoteClient_S::IsTaskRunning() {
  owner_->AssertIsLockedByCurrentThread();

  return task_running_;
}

bool RemoteClient_S::PopFirstUserWorkItem(FinalUserWorkItem& out_item) {
  owner_->AssertIsLockedByCurrentThread();

  if (!final_user_work_queue_.IsEmpty()) {
    out_item.From(final_user_work_queue_.Front(), host_id_);
    final_user_work_queue_.RemoveFront();
    return true;
  }

  return false;
}

void RemoteClient_S::OnSetTaskRunningFlag(bool running) {
  owner_->AssertIsLockedByCurrentThread();

  task_running_ = running;
}

RemoteClient_S::~RemoteClient_S() {
  owner_->AssertIsLockedByCurrentThread();

  p2p_connection_pairs_.Clear();

  UnlinkSelf();

  task_subject_node_.UnlinkSelf();

  if (owned_udp_socket_) {
    fun_check(owner_->local_addr_to_udp_socket_map_.Contains(
        owned_udp_socket_->cached_local_addr_));
    owner_->local_addr_to_udp_socket_map_.Remove(
        owned_udp_socket_->cached_local_addr_);

    // 포트를 빌려다가 사용했었다면, 포트풀에 반납해야합니다.
    if (borrowed_port_number_ > 0) {
      owner_->free_udp_ports_.Add(borrowed_port_number_);
    }

    owned_udp_socket_.Reset();
  }

  delete to_client_tcp_;

  final_user_work_queue_.Clear();
}

// 핑의 주기가 얼마나 빠른지로 스피드핵 여부를 감지합니다.
void RemoteClient_S::DetectSpeedHack() {
  owner_->AssertIsLockedByCurrentThread();

  if (speed_hack_detector_) {
    const double last_ping_ = speed_hack_detector_->last_ping_recv_time_;
    const double current_ping = owner_->GetAbsoluteTime();

    if (speed_hack_detector_->first_ping_recv_time_ == 0) {
      // 처음에는 이전 상태가 없으므로, 그대로 설정합니다.
      speed_hack_detector_->first_ping_recv_time_ = current_ping;
      speed_hack_detector_->last_ping_recv_time_ = current_ping;
    } else if (!speed_hack_detector_->hack_suspected_) {
      const double reckness =
          MathBase::Clamp(owner_->speed_hack_detector_reck_ratio_, 0.2, 0.9);

      speed_hack_detector_->recent_ping_interval_ =
          MathBase::Lerp(current_ping - last_ping_,
                         speed_hack_detector_->recent_ping_interval_, reckness);

      // 클라 생성 후 충분한 시간이 지났으며

      // 현재 스핵은 주기가 짧아지는 경우만 체크한다. 주기가 긴 경우는 TCP
      // fallback의 의심 상황일 수 있으므로 넘어간다.
      if ((current_ping - speed_hack_detector_->first_ping_recv_time_) >
          NetConfig::speedhack_detector_ping_interval_sec * 10) {
        if (speed_hack_detector_->recent_ping_interval_ <
            NetConfig::speedhack_detector_ping_interval_sec * 0.7) {
          speed_hack_detector_->hack_suspected_ = true;
        }
      }

      // 디스를 시키는건 오인에 민감하다. 따라서 경고만 날린다. 디스를 시키거나
      // 통계를 수집하는건 엔진 유저의 몫이다. 단, 1회만 날린다.
      if (speed_hack_detector_->hack_suspected_) {
        owner_->EnqueueHackSuspectEvent(this, "Speedhack", HackType::SpeedHack);
      }

      speed_hack_detector_->last_ping_recv_time_ = current_ping;
    }
  }
}

void RemoteClient_S::WarnTooShortDisposal(const char* where) {
  owner_->AssertIsLockedByCurrentThread();

  if (dispose_caller_ == nullptr) {
    dispose_caller_ = where;

    if ((owner_->GetAbsoluteTime() - created_time_) <
        0.3) {  // TODO 0.3초는 NetConfig 쪽으로 빼주도록 하자.
      if (owner_->intra_logger_) {
        const String text = String::Format(
            "WARNING: As soon as the client (remote: %d) comes in, it is "
            "banned immediately.  Is it intent or bug ? : %s",
            (int32)host_id_, where);
        owner_->intra_logger_->WriteLine(LogCategory::System, *text);
      }
    }
  }
}

void RemoteClient_S::EnqueueLocalEvent(LocalEvent& event) {
  if (owner_) {
    owner_->AssertIsLockedByCurrentThread();

    if (owner_->listener_) {
      final_user_work_queue_.Enqueue(event);
      owner_->UserTaskQueue.AddTaskSubject(this);
    }
  }
}

void RemoteClient_S::EnqueueUserTask(Function<void()> UserFunc) {
  if (owner_) {
    owner_->AssertIsLockedByCurrentThread();

    if (owner_->listener_) {
      final_user_work_queue_.Enqueue(UserFunc);
      owner_->UserTaskQueue.AddTaskSubject(this);
    }
  }
}

double RemoteClient_S::GetP2PGroupTotalRecentPing(HostId group_id) {
  // p2p group을 얻으려고 하는 경우 모든 멤버들의 평균 핑을 구한다.
  CScopedLock2 owner_guard(owner_->GetMutex());
  owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

  if (auto group = owner_->GetP2PGroupByHostId_NOLOCK(group_id)) {
    // touch jit p2p & get recent ping ave
    int32 count = 0;
    double sum = 0;
    for (const auto& member_pair : group->members_) {
      const double ping =
          owner_->GetRecentPingSec(member_pair.key);  // Touch JIT P2P
      if (ping >= 0) {
        count++;
        sum += ping;
      }
    }

    if (count > 0) {
      return (sum / count) * group->members_.Count();
    }
  }
  return -1;
}

void RemoteClient_S::LockMain_AssertIsLockedByCurrentThread() {
  owner_->AssertIsLockedByCurrentThread();
}

void RemoteClient_S::LockMain_AssertIsNotLockedByCurrentThread() {
  owner_->AssertIsNotLockedByCurrentThread();
}

void RemoteClient_S::EnqueueIssueSendReadyRemotes() {
  CScopedLock2 owner_guard(owner_->tcp_issue_queue_mutex_);

  if (GetListOwner() == nullptr) {
    owner_->tcp_issue_send_ready_remote_clients_.Append(this);
  }
}

bool RemoteClient_S::IsDispose() { return dispose_waiter_.IsValid(); }

double RemoteClient_S::GetAbsoluteTime() { return owner_->GetAbsoluteTime(); }

void RemoteClient_S::IssueDispose(ResultCode result_code,
                                  ResultCode detail_code,
                                  const ByteArray& comment, const char* where,
                                  SocketErrorCode socket_error) {
  owner_->IssueDisposeRemoteClient(this, result_code, detail_code, comment,
                                   where, socket_error);
}

// CanTearDown으로 바꾸는게 어떨런지? 아니면, IsSafeToClose?
bool RemoteClient_S::IsValidEnd() {
  return owner_->tcp_listening_socket_.IsValid() == false;
}

SocketErrorCode RemoteClient_S::IssueSend(double absolute_time) {
  return to_client_tcp_->ConditionalIssueSend(absolute_time);
}

void RemoteClient_S::Decrease() { DecreaseUseCount(); }

void RemoteClient_S::OnIssueSendFail(const char* where,
                                     SocketErrorCode socket_error) {
  IssueDispose(ResultCode::DisconnectFromRemote, ResultCode::TCPConnectFailure,
               ByteArray(), where, socket_error);

  WarnTooShortDisposal(where);
}

CCriticalSection2& RemoteClient_S::GetSendMutex() {
  return to_client_tcp_->GetSendQueueMutex();
}

CCriticalSection2& RemoteClient_S::GetMutex() {
  return to_client_tcp_->GetMutex();
}

bool RemoteClient_S::IsLockedByCurrentThread() {
  return to_client_tcp_->IsLockedByCurrentThread() ||
         to_client_tcp_->IsSendQueueLockedByCurrentThread();
}

void RemoteClient_S::SendWhenReady(HostId sender_id,
                                   const InetAddress& sender_addr,
                                   HostId dest_id,
                                   const SendFragRefs& data_to_send,
                                   const UdpSendOption& send_opt) {
  to_client_tcp_->SendWhenReady(data_to_send, send_opt);
}

}  // namespace net
}  // namespace fun
