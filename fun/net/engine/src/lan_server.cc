#include "CorePrivatePCH.h"
#include "fun/net/net.h"

#include "ReportError.h"

#include "LanListener_S.h"
#include "LanServer.h"
#include "server_socket_pool_.h"

// TODO
//#include "Tools/Visualizer/viz_agent_.h"

#include "LeanDynamicCast.h"

#include "GeneratedRPCs/lan_LanC2S_stub.cc"
#include "GeneratedRPCs/lan_LanS2C_proxy.cc"

// TODO 딱히 구현내용이 없는데 말이지???
#include "GeneratedRPCs/lan.cc"  //클라이언트/서버 구분이 안되는 문제가 있음.. 헤더화할수는 없는건가??

namespace fun {

using lf = LiteFormat;

#define ASSERT_OR_HACKED(X)                                 \
  {                                                         \
    if (!(X)) {                                             \
      EnqueueHackSuspectEvent(lc, #X, HackType::PacketRig); \
    }                                                       \
  }

IMPLEMENT_RPCSTUB_LanC2S_P2PGroup_MemberJoin_Ack(LanServerImpl::C2SStub) {
  CScopedLock2 main_guard(owner_->GetMutex());
  owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

  // FUN_TRACE("LanSrv - MemberJoin_Ack %d add:%d\n", (int)rpc_recvfrom,
  // (int)added_member_id);
  // ack-waiter를 삭제한다.
  if (auto group = owner_->GetP2PGroupByHostId_NOLOCK(group_id)) {
    //같은 경우 커넥트된것으로 간주.
    if (added_member_id == rpc_recvfrom) {
      if (auto p2p_state = owner_->lan_p2p_connection_pair_list_.GetPair(
              rpc_recvfrom, added_member_id)) {
        p2p_state->p2p_connect_state =
            LanP2PConnectionState::P2PConnectState::P2PConnected;
      }

      // FUN_TRACE("LanSrv - MemberJoin_Ack state Change Connected %d add:%d\n",
      // (int)rpc_recvfrom, (int)added_member_id);
    }

    // 제대로 지운 다음...
    // ack-waiter for new member became empty?
    if (group->add_member_ack_waiters_.RemoveEqualItem(
            rpc_recvfrom, added_member_id, event_id) == true &&
        group->add_member_ack_waiters_.AckWaitingItemExists(
            added_member_id, event_id) == false) {
      // FUN_TRACE("LanSrv - MemberJoin_AckComplete %d add:%d\n",
      // (int)rpc_recvfrom, (int)added_member_id);
      owner_->EnqueueP2PAddMemberAckCompleteEvent(group_id, added_member_id,
                                                  ResultCode::Ok);

      // ack가 완료되었으니 P2P연결을 시작해라.
      for (auto& old_member_pair : group->members_) {
        LanClient_S* old_member = nullptr;
        try {
          old_member = dynamic_cast<LanClient_S*>(
              old_member_pair.value.ptr);  // 기존 그룹의 각 멤버
        } catch (std::bad_cast& e) {
          OutputDebugString(UTF8_TO_TCHAR(e.what()));
          int32* X = nullptr;
          *X = 1;
        }

        if (old_member == nullptr) {
          continue;
        }

        auto p2p_state = owner_->lan_p2p_connection_pair_list_.GetPair(
            old_member->host_id_, added_member_id);

        // 자기보다 큰 호스트아이디, 혹은 이미 연결되어있다면 건너뛴다.
        if (old_member->host_id_ >= added_member_id || !p2p_state.IsValid() ||
            p2p_state->dup_count > 1) {
          continue;
        }

        // 이미 연결중이라면 다시 보내지 않는다.
        if (p2p_state->p2p_connect_state !=
            LanP2PConnectionState::P2PConnectState::P2PJoining) {
          continue;
        }

        //@maxidea: 첫번째 remote-Peer에게만 보내주는 걸까??

        // OldMember에게 Connection을 걸라고 알린다.
        InetAddress external_addr =
            p2p_state->GetExternalAddr(old_member->host_id_);
        owner_->s2c_proxy_.P2PConnectStart(added_member_id,
                                           GReliableSend_INTERNAL,
                                           old_member->host_id_, external_addr);

        // P2P connect state 바꿈
        p2p_state->p2p_connect_state =
            LanP2PConnectionState::P2PConnectState::P2PConnecting;
      }
    }
  } else {
    // FUN_TRACE("경고 : 대응하는 ack가 없습니다.\n");
  }

  return true;
}

IMPLEMENT_RPCSTUB_LanC2S_ReliablePing(LanServerImpl::C2SStub) {
  CScopedLock2 main_guard(owner_->GetMutex());
  owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

  if (auto lc = owner_->GetAuthedLanClientByHostId_NOLOCK(rpc_recvfrom)) {
    lc->last_tcp_stream_recv_time_ = owner_->GetAbsoluteTime();

    owner_->s2c_proxy_.ReliablePong(rpc_recvfrom, GReliableSend_INTERNAL);

    if (owner_->intra_logger_) {
      const String text = String::Format(
          "Reliable ping receive.  %d host %lf last_tcp_stream_recv_time_",
          (int32)rpc_recvfrom, lc->last_tcp_stream_recv_time_);
      owner_->intra_logger_->WriteLine(LogCategory::System, *text);
    }
  }

  return true;
}

IMPLEMENT_RPCSTUB_LanC2S_ReportP2PPeerPing(LanServerImpl::C2SStub) {
  CScopedLock2 main_guard(owner_->GetMutex());
  owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

  // TODO uint32이므로 아래 조건 체크는 의미 없음.
  // if (recent_ping >= 0) {
  if (auto lc = owner_->GetAuthedLanClientByHostId_NOLOCK(rpc_recvfrom)) {
    for (auto& pair : lc->lan_p2p_connection_pairs_) {
      pair->recent_ping_ = double(recent_ping) / 1000;
    }
  }
}

return true;
}  // namespace fun

IMPLEMENT_RPCSTUB_LanC2S_ShutdownTcp(LanServerImpl::C2SStub) {
  CScopedLock2 main_guard(owner_->GetMutex());
  owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

  if (auto lc = owner_->GetAuthedLanClientByHostId_NOLOCK(rpc_recvfrom)) {
    lc->shutdown_comment_ = comment;

    owner_->s2c_proxy_.ShutdownTcpAck(rpc_recvfrom, GReliableSend_INTERNAL);
  }

  return true;
}

IMPLEMENT_RPCSTUB_LanC2S_ShutdownTcpHandshake(LanServerImpl::C2SStub) {
  CScopedLock2 main_guard(owner_->GetMutex());
  owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

  if (auto lc = owner_->GetAuthedLanClientByHostId_NOLOCK(rpc_recvfrom)) {
    owner_->IssueDisposeLanClient(
        lc, ResultCode::DisconnectFromRemote, ResultCode::TCPConnectFailure,
        lc->shutdown_comment_, __FUNCTION__, SocketErrorCode::Ok);
  }

  return true;
}

bool LanServerImpl::GetJoinedP2PGroups(HostId client_id, HostIdArray& output) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (auto lc = authed_lan_clients_.FindRef(client_id)) {
    output.Clear(lc->joined_lan_p2p_groups_.Count());  // just in case
    for (const auto& pair : lc->joined_lan_p2p_groups_) {
      const HostId group_id = pair.key;
      output.Add(group_id);
    }
    return true;
  } else {
    output.Clear();  // just in case
    return false;
  }
}

void LanServerImpl::GetP2PGroups(P2PGroupInfos& out_result) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  out_result.Clear(lan_p2p_groups_.Count());  // just in case
  for (auto& group_pair : lan_p2p_groups_) {
    auto group = group_pair.value;
    auto group_info = group->GetInfo();
    out_result.Add(group_info->group_id_, group_info);
  }
}

int32 LanServerImpl::GetP2PGroupCount() {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  return lan_p2p_groups_.Count();
}

//@todo CStartLanServerParameter는 내부에서 변경될 여지가 있음. (in and out)
bool LanServerImpl::Start(const LanStartServerArgs& InParams,
                          SharedPtr<ResultInfo>& out_error) {
  CScopedLock2 start_stop_phase_guard(start_stop_phase_mutex_);

  // 이미 시작한 적이 있으면 에러 처리한다.
  if (tcp_listening_socket_) {
    out_error = ResultInfo::From(ResultCode::AlreadyConnected, HostId_None,
                                 "Already listening socket exists.");
    return false;
  }

  //@note 내부에서 적절한 셋팅을 변경될 수 있으므로, 카피해야함.
  LanStartServerArgs args = InParams;

  if (args.thread_count == 0) {
    args.thread_count = CPlatformMisc::NumberOfCoresIncludingHyperthreads();
  }

  if (args.thread_count < 0) {
    throw Exception("Invalid thread count");
  }

  // 파라메터 validation을 체크한다.
  const bool has_multiple_nics = NetUtil::LocalAddresses().Count() > 1;
  if (has_multiple_nics && args.local_nic_addr_.IsEmpty()) {
    ShowWarning_NOLOCK(ResultInfo::From(
        ResultCode::Unexpected, HostId_None,
        "Server has multiple network devices, however, no device is specified "
        "for listening. Is it your intention?"));

    const String text = String::Format(
        "No NIC binding though multiple NIC detected##Process=%s",
        CPlatformProcess::ExecutableName());
    ErrorReporter::Report(text);
  }

  settings_.server_as_p2p_group_member_allowed =
      args.server_as_p2p_group_member_allowed;
  settings_.p2p_encrypted_messaging_enabled =
      args.p2p_encrypted_messaging_enabled;
  settings_.look_ahead_p2p_send_enabled = args.look_ahead_p2p_send_enabled;
  settings_.bEnableNagleAlgorithm = args.bEnableNagleAlgorithm;

  // LAN환경이므로, UPnP는 사용안함
  settings_.upnp_detect_nat_device = false;
  settings_.upnp_tcp_addr_port_mapping = false;

  settings_.strong_encrypted_message_key_length =
      args.strong_encrypted_message_key_length;

  //@warning 아예 길이를 정의로 맞추어 놓은 사례..
  if (settings_.strong_encrypted_message_key_length !=
          (int32)StrongEncryptionLevel::Low &&
      settings_.strong_encrypted_message_key_length !=
          (int32)StrongEncryptionLevel::Middle &&
      settings_.strong_encrypted_message_key_length !=
          (int32)StrongEncryptionLevel::High) {
    throw Exception("invalid key length.");
  }

  // RC4 키 길이 Bit 안쓸 떄에는 0
  //@warning 아예 길이를 정의로 맞추어 놓은 사례..
  settings_.weak_encrypted_message_key_length =
      args.weak_encrypted_message_key_length;
  if (settings_.weak_encrypted_message_key_length !=
          (int32)WeakEncryptionLevel::None &&
      settings_.weak_encrypted_message_key_length !=
          (int32)WeakEncryptionLevel::Low &&
      settings_.weak_encrypted_message_key_length !=
          (int32)WeakEncryptionLevel::Middle &&
      settings_.weak_encrypted_message_key_length !=
          (int32)WeakEncryptionLevel::High) {
    throw Exception(
        "LanStartServerArgs::weak_encrypted_message_key_length incorrect key "
        "length, MaxSize is 2048, MinSize is 8");
  }

  // 공개키와 개인키를 미리 생성하고 루프백용 aes 키를 생성합니다.
  ByteArray random_block;
  ByteArray rc4_random_block;
  if (!CryptoRSA::CreatePublicAndPrivateKey(self_xchg_key_, public_key_blob_) ||
      !CryptoRSA::CreateRandomBlock(
          random_block, settings_.strong_encrypted_message_key_length) ||
      !CryptoAES::ExpandFrom(
          self_session_key_.aes_key, (const uint8*)random_block.ConstData(),
          settings_.strong_encrypted_message_key_length / 8) ||

      !CryptoRSA::CreateRandomBlock(
          rc4_random_block, settings_.weak_encrypted_message_key_length) ||
      !CryptoRC4::ExpandFrom(self_session_key_.rc4_key,
                             (const uint8*)rc4_random_block.ConstData(),
                             settings_.weak_encrypted_message_key_length / 8)) {
    throw Exception("Failed to create session-key");
  }

  self_encrypt_count_ = 0;
  self_decrypt_count_ = 0;

  // create IOCP & user worker IOCP
  // completion_port_.Reset(new CompletionPort(this, true)); // AcceptEx때문에
  // 무조건 true
  tcp_accept_cp_.Reset(new CompletionPort(this, true, 1));

  clock_.Reset();
  clock_.Start();
  AbsoluteTime_USE_GetAbsoluteTime = clock_.AbsoluteSeconds();
  // elapsed_time = 0;

  // 서버 인스턴스 GUID 생성
  instance_tag_ = Uuid::NewUuid();

  //@maxidea: todo: 폴트를 여러개 지정할 수 있도록 해보자.

  if (!CreateTcpListenSocketAndInit(args.tcp_port, out_error)) {
    // 앞서 시행했던 것들을 모두 롤백한다.
    //@maxidea: 앞서하지 말고, 이후에 하면 다시 돌려야하는 부담도 없을듯
    //싶은데...
    timer_.Stop();
    tcp_accept_cp_.Reset();
    instance_tag_ = Uuid::None;
    out_error =
        ResultInfo::From(ResultCode::ServerPortListenFailure, HostId_None,
                         "Listening socket create failed.");
    return false;
  }

  // Copy some parameter values
  protocol_version_ = args.protocol_version;
  server_ip_alias_ = args.server_addr_at_client;
  local_nic_addr_ = args.local_nic_addr;
  timer_callback_interval_ = args.timer_callback_interval;
  timer_callback_context_ = args.timer_callback_context;

  // Initialize host_id factory
  if (args.HostIdGenerationPolicy == HostIdGenerationPolicy::NoRecycle) {
    host_id_factory_.Reset(new HostIdFactory());
  } else if (args.HostIdGenerationPolicy == HostIdGenerationPolicy::Recycle) {
    host_id_factory_.Reset(
        new RecycleHostIdFactory(NetConfig::host_id_recycle_allow_time_sec));
  } else {
    throw Exception(
        "Invalid parameter not HostIdGenerationPolicy::Assign in Lanserver");
  }

  // Determine threa count for thread pool
  args.network_thread_count =
      MathBase::Clamp(args.network_thread_count, 0,
                      CPlatformMisc::NumberOfCoresIncludingHyperthreads());
  if (args.network_thread_count == 0) {
    args.network_thread_count =
        CPlatformMisc::NumberOfCoresIncludingHyperthreads();
  }

  // Initialize thread pool
  if (args.external_net_worker_thread_pool) {
    auto thread_pool_impl =
        dynamic_cast<ThreadPoolImpl*>(args.external_net_worker_thread_pool);
    if (thread_pool_impl == nullptr || !thread_pool_impl->IsActive()) {
      throw Exception("LanWorerThreadPool not start.");
    }

    net_thread_pool_.Reset(thread_pool_impl);
    net_thread_external_use_ = true;
  } else {
    net_thread_pool_.Reset((ThreadPoolImpl*)ThreadPool2::New());
    net_thread_pool_->Start(args.network_thread_count);
    net_thread_external_use_ = false;
  }

  if (args.external_user_worker_thread_pool) {
    auto thread_pool_impl =
        dynamic_cast<ThreadPoolImpl*>(args.external_user_worker_thread_pool);
    if (thread_pool_impl == nullptr || !thread_pool_impl->IsActive()) {
      throw Exception("LanUserWorkerThreadPool not start.");
    }

    user_thread_pool_.Reset(thread_pool_impl);
    user_thread_external_use_ = true;
  } else {
    user_thread_pool_.Reset((ThreadPoolImpl*)ThreadPool2::New());
    user_thread_pool_->SetCallbacks(this);  // 내장일 경우 eventsink는 this가
                                            // 된다. (스레드풀 셋팅전에 해야함)
    user_thread_pool_->Start(args.thread_count);
    user_thread_external_use_ = false;
  }

  // Register referrer
  net_thread_pool_->RegistReferer(this);
  net_thread_pool_unregisted_ = false;
  user_thread_pool_->RegistReferer(this);
  user_thread_pool_unregisted_ = false;

  // TCP를 받아 처리하는 스레드 풀을 만든다.
  listener_thread_.Reset(new LanListener_S(this));
  listener_thread_->StartListening();

  pure_too_old_unmature_client_alarm_ =
      IntervalAlaram(NetConfig::purge_too_old_add_member_ack_timeout_sec);
  purge_too_old_add_member_ack_item_alarm_ =
      IntervalAlaram(NetConfig::purge_too_old_add_member_ack_timeout_sec);
  dispose_issued_remote_clients_alarm_ =
      IntervalAlaram(NetConfig::dispose_issued_remote_clients_timeout_sec);
  remove_too_old_tcp_send_packet_queue_alarm_ =
      IntervalAlaram(NetConfig::tcp_packet_board_long_interval_sec);
  disconnect_remote_client_on_timeout_alarm_ =
      IntervalAlaram(NetConfig::cs_ping_interval_sec);

  timer_.Start();

  if (timer_callback_interval_ > 0) {
    tick_timer_id_ = timer_.Schedule(
        Timespan::FromMilliseconds(100),
        Timespan::FromMilliseconds(timer_callback_interval_),
        [&](const TimerTaskContext&) { PostOnTick(); }, "LanServer.Tick");
  } else {
    tick_timer_id_ = 0;
  }

  heartbeat_timer_id_ = timer_.Schedule(
      Timespan::FromMilliseconds(100),
      Timespan::FromMilliseconds(NetConfig::server_heartbeat_interval_msec),
      [&](const TimerTaskContext&) { PostHeartbeatIssue(); },
      "LanServer.Heartbeat");

  issue_send_on_need_timer_id_ = timer_.Schedule(
      Timespan::FromMilliseconds(100),
      Timespan::FromMilliseconds(
          NetConfig::every_remote_issue_send_on_need_interval_sec * 1000),
      [&](const TimerTaskContext&) { PostEveryRemote_IssueSend(); },
      "LanServer.ConditionalIssueSend");

  CPlatformProcess::Sleep(0.1f);

  return true;
}

//@todo 뭔가 종료 처리가 매우 단순 무식하다...
void LanServerImpl::Stop() {
  CScopedLock2 start_stop_phase_guard(start_stop_phase_mutex_);

  AssertIsNotLockedByCurrentThread();

  if (!tcp_listening_socket_ /*|| tear_down_*/) {
    return;
  }

  // threadpool내부에 lock가 있으므로 여기서 criticalsection을 걸면 안됨.
  if (user_thread_pool_ && user_thread_pool_->IsCurrentThread()) {
    // 지우지 말것.
    // 이상하게도 userworkerthread내에서 try catch로 잡으면 파괴자가 호출되지
    // 않는 현상이 생긴다.
    //@note CScopedLock2 StartStopLock의 소멸자가 호출안되는 경우가 있다라고..
    start_stop_phase_guard.Unlock();

    throw Exception("Call Stop() in UserWorker");
  }
  {
    // tear_down_ = true;

    // 더 이상 accept은 없다.
    // 이게 선결되지 않으면 클라를 쫓는 과정에서 신규 클라가 들어오니 즐
    // listener스레드가 먼저 죽어야 한다. 그래야 accpetex가 실패로 나오는 것이
    // 없다.
    listener_thread_.Reset();

    if (tcp_listening_socket_) {
      tcp_listening_socket_->CloseSocketHandleOnly();

      if (intra_logger_) {
        intra_logger_->WriteLine(
            LogCategory::System,
            *String::Format("CloseSocketHandleOnly() called at %s",
                            __FUNCTION__));
      }
    }

    // 모든 클라이언트를 쫓아낸다.
    CScopedLock2 main_guard(main_mutex_);
    CheckCriticalSectionDeadLock(__FUNCTION__);

    for (auto& pair : candidate_lan_clients_) {
      auto lc = pair.value;
      IssueDisposeLanClient(lc, ResultCode::DisconnectFromLocal,
                            ResultCode::TCPConnectFailure, ByteArray(),
                            __FUNCTION__, SocketErrorCode::Ok);
    }

    for (auto& pair : authed_lan_clients_) {
      auto lc = pair.value;
      IssueDisposeLanClient(lc, ResultCode::DisconnectFromLocal,
                            ResultCode::TCPConnectFailure, ByteArray(),
                            __FUNCTION__, SocketErrorCode::Ok);
    }
  }

  // 모든 클라이언트의 비동기 io가 끝날 때까지 대기
  for (int32 i = 0; i < 10000; ++i) {
    CScopedLock2 main_guard(main_mutex_);
    CheckCriticalSectionDeadLock(__FUNCTION__);

    if (authed_lan_clients_.Count() == 0 &&
        candidate_lan_clients_.Count() == 0 &&
        dispose_issued_lan_clients_map_.Count() == 0) {
      break;
    }

    main_guard.Unlock();

    CPlatformProcess::Sleep(0.1f);
  }

  // 이제 비동기 io가 없으므로

  // timer종료 후
  // HeartbeatTimer.Reset();
  // IssueSendOnNeedTimer.Reset();
  // OnTickTimer.Reset();

  timer_.Stop(true);  // 내부 처리가 완료될때까지 대기함.

  // TODO 구태여 타이머 ID를 유지할 필요가 없어보이는데...??
  tick_timer_id_ = 0;
  heartbeat_timer_id_ = 0;
  issue_send_on_need_timer_id_ = 0;

  // listener_thread_.Reset();

  // timerfree후 thread종료.
  //종료를 알림후 대기...
  net_thread_pool_->PostCompletionStatus(this, (UINT_PTR)IocpCustomValue::End);

  if (net_thread_pool_ != user_thread_pool_) {
    user_thread_pool_->PostCompletionStatus(this,
                                            (UINT_PTR)IocpCustomValue::End);
  }

  for (int32 i = 0; i < 10000; ++i) {
    if (net_thread_pool_unregisted_ && user_thread_pool_unregisted_) {
      break;
    }

    CPlatformProcess::Sleep(0.1f);
  }

  if (!net_thread_pool_unregisted_ || !user_thread_pool_unregisted_) {
    fun_check(0);
  }

  if (!net_thread_external_use_) {
    if (net_thread_pool_ == user_thread_pool_) {
      net_thread_pool_.Reset();
      user_thread_pool_.Detach();
    } else {
      net_thread_pool_.Reset();
    }
  } else {
    net_thread_pool_.Detach();
  }

  if (!user_thread_external_use_) {
    user_thread_pool_.Reset();
  } else {
    user_thread_pool_.Detach();
  }

  // 이제 모든 스레드가 종료됐으니 객체들을 다 쎄려부리
  {
    CScopedLock2 main_guard(main_mutex_);
    CheckCriticalSectionDeadLock(__FUNCTION__);

    tcp_accept_cp_.Reset();
    timer_.Stop();

    tcp_listening_socket_.Reset();

    // Map에서 첫번째 엔트르를 가져오려면??
    // while (authed_lan_clients_.Count() > 0) {
    //  ProcessOnClientDisposeCanSafe(authed_lan_clients_.GetValueAt(authed_lan_clients_.GetStartPosition()));
    //}
    for (auto& pair : authed_lan_clients_) {
      ProcessOnClientDisposeCanSafe(pair.value);
    }
    authed_lan_clients_.Clear();

    // Map에서 첫번째 엔트르를 가져오려면??
    // while (candidate_lan_clients_.Count() > 0) {
    //  ProcessOnClientDisposeCanSafe(candidate_lan_clients_.begin()->Value);
    //}
    for (auto& pair : candidate_lan_clients_) {
      ProcessOnClientDisposeCanSafe(pair.value);
    }
    candidate_lan_clients_.Clear();

    // Destroy all lc instances.
    for (auto lc_instance : lan_client_instances_) {
      delete lc_instance;
    }
    lan_client_instances_.Clear();

    // Clear P2P groups.
    lan_p2p_groups_.Clear();
  }

  // 아래 과정은 unsafe heap을 쓰는 것들을 main_mutex_ lock 상태에서 청소하지
  // 않으면 괜한 assert fail이 발생하므로 필요
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  host_id_factory_.Reset();
  final_user_work_queue_.Clear();
  authed_lan_clients_.Clear();
  candidate_lan_clients_.Clear();
  // TODO
  // viz_agent_.Reset();
  // tear_down_ = false;
}

bool LanServerImpl::CreateTcpListenSocketAndInit(
    int32 tcp_port, SharedPtr<ResultInfo>& out_error) {
  tcp_listening_socket_.Reset(new InternalSocket(SocketType::Tcp, this));

  tcp_listening_socket_->ignore_not_socket_error_ = true;

  if (!tcp_listening_socket_->Bind(*local_nic_addr_, tcp_port)) {
    // TCP 리스닝 자체를 시작할 수 없는 상황이다. 이런 경우 서버는 제 기능을 할
    // 수 없는 심각한 상황이다.
    out_error = ResultInfo::From(ResultCode::ServerPortListenFailure,
                                 HostId_Server, "");
    tcp_listening_socket_.Reset();
    return false;
  } else {
    tcp_listening_socket_->Listen();
    tcp_listening_socket_->SetCompletionContext((ICompletionContext*)this);
    tcp_accept_cp_->AssociateSocket(tcp_listening_socket_.Get());
  }

  return true;
}

void LanServerImpl::PurgeTooOldAddMemberAckItem() {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  // Clear ack-info if it is too old.
  const double absolute_time = GetAbsoluteTime();

  for (auto& group_pair : lan_p2p_groups_) {
    auto group = group_pair.value;

    for (int32 member_index = 0;
         member_index < group->add_member_ack_waiters_.Count();
         ++member_index) {
      const auto& ack = group->add_member_ack_waiters_[member_index];

      if ((absolute_time - ack.event_time) >
          NetConfig::purge_too_old_add_member_ack_timeout_sec) {
        const HostId joining_peer_id = ack.joining_member_host_id;

        // 이거 하나만 지우고 리턴. 나머지는 다음 기회에서 지워도 충분하니까.
        group->add_member_ack_waiters_.RemoveAt(member_index);

        // 너무 오랫동안 ack가 안와서 제거되는 항목이 영구적 콜백 없음으로
        // 이어져서는 안되므로 아래가 필요
        if (!group->add_member_ack_waiters_.AckWaitingItemExists(
                joining_peer_id)) {
          EnqueueP2PAddMemberAckCompleteEvent(group->group_id_, joining_peer_id,
                                              ResultCode::ConnectServerTimeout);
        }

        return;
      }
    }
  }
}

void LanServerImpl::PurgeTooOldUnmatureClient() {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  const int32 list_count = candidate_lan_clients_.Count();
  if (list_count <= 0) {
    return;
  }

  const double absolute_time = GetAbsoluteTime();

  // 중국의 경우 tcp_socket_connect_timeout_sec * 2 + 3 일때 63이 나오므로
  // 상향조정함.
  fun_check(NetConfig::client_connect_server_timeout_sec < 70);

  int32 timeout_count = 0;
  //@todo 대상이 너무 많으면 어똫게 되는가.. 다른 메모리 할당자를 쓰는게 좋을듯
  //싶음. Array<LanClient_S*,InlineAllocator<256>> timeout_lc_list(list_count,
  // NoInit);
  Array<LanClient_S*> timeout_lc_list(list_count, NoInit);

  for (auto& pair : candidate_lan_clients_) {
    auto lc = pair.value;

    if (!lc->purge_requested_ &&
        (absolute_time - lc->created_time_) >
            NetConfig::client_connect_server_timeout_sec) {
      lc->purge_requested_ = true;
      lc->IncreaseUseCount();
      timeout_lc_list[timeout_count++] = lc;
    }
  }

  main_guard.Unlock();

  //@todo 아래의 부분은 좀더 확실하게 처리해야할듯 싶음. 뭔가 어수선함..

  MessageOut msg_to_send;
  lf::Write(msg_to_send, MessageType::ConnectServerTimedout);

  while (timeout_count > 0) {
#if USE_PARALLEL_FOR
#if (_MSC_VER >= 1400)
#pragma omp parallel for
#endif
#endif
    for (int32 timeout_index = 0; timeout_index < timeout_count;
         ++timeout_index) {
      auto lc = timeout_lc_list[timeout_index];

      CScopedLock2 lc_tcp_send_queue_guard(
          lc->to_client_tcp_->GetSendQueueMutex(), false);

      if (timeout_index != 0) {
        if (const bool lock_ok = lc_tcp_send_queue_guard.TryLock()) {
          lc->to_client_tcp_->SendWhenReady(SendFragRefs(msg_to_send),
                                            TcpSendOption());
          lc_tcp_send_queue_guard.Unlock();

          lc->DecreaseUseCount();
          timeout_lc_list[timeout_index] = timeout_lc_list[--timeout_count];
        }
      } else {
        lc_tcp_send_queue_guard.Lock();
        lc->to_client_tcp_->SendWhenReady(SendFragRefs(msg_to_send),
                                          TcpSendOption());
        lc_tcp_send_queue_guard.Unlock();

        lc->DecreaseUseCount();
        timeout_lc_list[timeout_index] = timeout_lc_list[--timeout_count];
      }
    }
  }
}

void LanServerImpl::ProcessOnClientDisposeCanSafe(LanClient_S* lc) {
  AssertIsLockedByCurrentThread();
  {
    CScopedLock2 tcp_issue_queue_guard(tcp_issue_queue_mutex_);
    lc->UnlinkSelf();  // 송신 이슈 큐에서도 제거
  }

  // dispose Client object (including sockets)
  if (lc->to_client_tcp_->socket_.IsValid()) {
    lc->to_client_tcp_->socket_->CloseSocketHandleOnly();

    if (intra_logger_) {
      intra_logger_->WriteLine(
          LogCategory::System,
          *String::Format("CloseSocketHandleOnly() called at %s",
                          __FUNCTION__));
    }
  }

  // Remove a lan-client from list
  candidate_lan_clients_.Remove(lc);
  authed_lan_clients_.Remove(lc->host_id_);
  host_id_factory_->Drop(GetAbsoluteTime(), lc->host_id_);
}

SessionKey* LanServerImpl::GetCryptSessionKey(HostId remote_id,
                                              String& out_error) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  SessionKey* key = nullptr;
  auto lc = GetAuthedLanClientByHostId_NOLOCK(remote_id);
  if (lc) {
    key = &lc->session_key_;
  } else if (remote_id == HostId_Server) {
    key = &self_session_key_;
  }

  if (key && !key->KeyExists()) {
    out_error = "Key is not exists.";
    return nullptr;
  }

  if (key == nullptr) {
    out_error =
        String::Format("%d remote LanClient is %s in LanServer.",
                       (int32)remote_id, lc == nullptr ? "NULL" : "not NULL");
  }

  return key;
}

bool LanServerImpl::Send(const SendFragRefs& data_to_send,
                         const SendOption& send_opt, const HostId* sendto_list,
                         int32 sendto_count) {
  // 네트워킹 비활성 상태이면 무조건 그냥 아웃.
  // 여기서 사전 검사를 하는 이유는, 아래 하위 callee들은 많은 validation
  // check를 하는데, 그걸 다 거친 후 안보내는 것 보다는 앗싸리 여기서 먼저
  // 안보내는 것이 성능상 장점이니까.
  if (!listener_thread_) {
    return false;
  }

  // 메시지 압축 레이어를 통하여 메시지에 압축 여부 관련 헤더를 삽입한다.
  // 암호화 된 후에는 데이터의 규칙성이 없어져서 압축이 재대로 되지 않기 때문에
  // 반드시 암호화 전에 한다.
  return Send_CompressLayer(data_to_send, send_opt, sendto_list, sendto_count);
}

bool LanServerImpl::Send_BroadcastLayer(const SendFragRefs& payload,
                                        const SendOption& send_opt,
                                        const HostId* sendto_list,
                                        int32 sendto_count) {
  // 이제 여기서 main lock을 건다.
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  // P2P 그룹을 host_id 리스트로 변환, 즉 ungroup한다.
  HostIdArray send_dest_list0;
  ConvertGroupToIndividualsAndUnion(sendto_count, sendto_list, send_dest_list0);

  // 각 수신 대상의 remote host object를 얻는다.
  LanSendDestInfoList_S send_dest_list;
  Convert_NOLOCK(send_dest_list, send_dest_list0);

  int32 send_dest_list_count = send_dest_list.Count();
  // Array<LanClient_S*,InlineAllocator<256>> send_list(send_dest_list_count,
  // NoInit);
  Array<LanClient_S*> send_list(send_dest_list_count, NoInit);

  // 각 수신자에 대해...
  int32 send_count = 0;
  for (int32 send_dest_index = 0; send_dest_index < send_dest_list_count;
       ++send_dest_index) {
    auto& send_dest_info = send_dest_list[send_dest_index];
    auto send_dest = send_dest_info.object;

    // if loopback
    if (send_dest == this && send_opt.bounce) {
      // 즉시 final user work로 넣어버리자. 루프백 메시지는 사용자가 발생하는 것
      // 말고는 없는 것을 전제한다.
      //메시지 타입을 제거한 후 페이로드만 온전히 남겨준다음 넘겨줌.

      const ByteArray payload_data = payload.ToBytes();  // copy

      // EMssageType은 제거한 상태로 넘어가야함.
      MessageType msg_type;
      MessageIn payload_without_msg_type(payload_data);
      lf::Read(payload_without_msg_type, msg_type);

      FinalUserWorkItemType final_work_type;
      switch (msg_type) {
        case MessageType::RPC:
          final_work_type = FinalUserWorkItemType::RPC;
          break;
        case MessageType::FreeformMessage:
          final_work_type = FinalUserWorkItemType::FreeformMessage;
          break;
        default:
          fun_check(0);
          break;
      }

      // Post(이건 별도의 함수로 만들어도 좋을듯 싶은데...)
      final_user_work_queue_.Enqueue(
          FinalUserWorkItem_S(payload_without_msg_type, final_work_type));
      user_task_queue_.AddTaskSubject(this);
    }
    // 수신자가 클라인 경우
    // (참고: dynamic_cast<LanClient_S*>(send_dest)는 매우 느리다!)
    else if (send_dest && LeanDynamicCastForLanClient(send_dest)) {
      auto lc = (LanClient_S*)send_dest;

      lc->IncreaseUseCount();
      send_list[send_count++] = lc;
    }
  }

  // main lock unlock
  main_guard.Unlock();

  for (int32 lc_index = 0; lc_index < send_count; ++lc_index) {
    auto lc = send_list[lc_index];
    {
      CScopedLock2 lc_tcp_send_queue_guard(
          lc->to_client_tcp_->GetSendQueueMutex());
      lc->to_client_tcp_->SendWhenReady(payload, TcpSendOption());
    }

    lc->DecreaseUseCount();
  }

  // 일단 다 보내고 난뒤 MessageReliability::Unreliable라면 Error임을 노티한다.
  if (send_opt.reliability == MessageReliability::Unreliable) {
    // EnqueueError() 내부에서 락을 걸기 때문에 구태여 여기서 할 필요는 없음.
    // CScopedLock2 main_guard(main_mutex_);
    // CheckCriticalSectionDeadLock(__FUNCTION__);

    EnqueueError(ResultInfo::From(
        ResultCode::Unexpected, HostId_Server,
        "An unreliable message cannot use LanServer and LanClient."));
    return false;
  }

  return true;
}

bool LanServerImpl::CloseConnection(HostId client_id) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (auto lc = GetAuthedLanClientByHostId_NOLOCK(client_id)) {
    // 바로 연결을 끊지 않고 클라이언트에게 자진탈퇴 요청을 한다.
    if (intra_logger_) {
      intra_logger_->WriteLine(LogCategory::System,
                               "CloseConnection() called.");
    }

    // Request auto-prune(self-unregisterring)
    RequestAutoPrune(lc);

    return true;
  } else {
    return false;
  }
}

void LanServerImpl::CloseAllConnections() {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (intra_logger_) {
    intra_logger_->WriteLine(LogCategory::System,
                             "CloseAllConnections() called.");
  }

  for (auto& pair : authed_lan_clients_) {
    auto lc = pair.value;

    RequestAutoPrune(lc);
  }
}

LanClient_S* LanServerImpl::GetLanClientByHostId_NOLOCK(HostId client_id) {
  AssertIsLockedByCurrentThread();

  return authed_lan_clients_.FindRef(client_id);
}

LanClient_S* LanServerImpl::GetAuthedLanClientByHostId_NOLOCK(
    HostId client_id) {
  AssertIsLockedByCurrentThread();

  LanClient_S* lc = authed_lan_clients_.FindRef(client_id);
  return (lc && lc->dispose_waiter_.IsValid() == false)
             ? lc
             : nullptr;  // disposing 중인것은 제외함.
}

double LanServerImpl::GetLastPingSec(HostId peer_id) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  auto lc = GetAuthedLanClientByHostId_NOLOCK(peer_id);
  return lc ? lc->last_ping_ : -1;
}

double LanServerImpl::GetRecentPingSec(HostId peer_id) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  auto lc = GetAuthedLanClientByHostId_NOLOCK(peer_id);
  return lc ? lc->recent_ping_ : -1;
}

bool LanServerImpl::DestroyP2PGroup(HostId group_id) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (auto group = GetP2PGroupByHostId_NOLOCK(group_id)) {
    // TODO 굳이 이런식으로 비워야할까?
    // 내부에서 목록이 추가되는 경우나 이렇게 하는걸텐데...
    while (!group->members_.IsEmpty()) {
      auto it = group->members_.CreateIterator();
      LeaveP2PGroup(it->key, group->group_id_);
    }

    // 다 끝났다. 이제 P2P 그룹 자체를 파괴해버린다.
    if (lan_p2p_groups_.Remove(group_id)) {
      host_id_factory_->Drop(GetAbsoluteTime(), group_id);
      EnqueueP2PGroupRemoveEvent(group_id);
    }

    return true;
  }

  return false;
}

bool LanServerImpl::LeaveP2PGroup(HostId member_id, HostId group_id) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  // 체크
  auto group = GetP2PGroupByHostId_NOLOCK(group_id);
  if (!group) {
    return false;
  }

  // 2009.11.06 MemberId가 서버가 멤버인경우 처리 수정 이미 나갔음 즐
  LanP2PGroupMember_S member_lc0;
  LanP2PGroupMemberBase_S* member_lc;

  if (!group->members_.TryGetValue(member_id, member_lc0)) {
    return false;
  }

  // 유효한 객체인지 체크
  if (member_id == HostId_Server) {
    member_lc = this;
  } else {
    member_lc = GetAuthedLanClientByHostId_NOLOCK(member_id);
    if (member_lc == nullptr) {
      return false;
    }
  }

  // notify P2PGroup_MemberLeave to related members
  // notify it to the banished member
  for (const auto& member_pair : group->members_) {
    if (member_pair.key != HostId_Server) {
      s2c_proxy_.P2PGroup_MemberLeave(member_pair.key, GReliableSend_INTERNAL,
                                      member_id, group_id);
    }

    if (member_id != HostId_Server && member_id != member_pair.key) {
      s2c_proxy_.P2PGroup_MemberLeave(member_id, GReliableSend_INTERNAL,
                                      member_pair.key, group_id);
    }
  }

  // P2P 연결 쌍 리스트에서도 제거하되 중복 카운트를 감안한다.
  if (member_id != HostId_Server) {
    for (const auto& member_pair : group->members_) {
      if (member_pair.value.ptr->GetHostId() != HostId_Server &&
          member_lc->GetHostId() != HostId_Server) {
        lan_p2p_connection_pair_list_.ReleasePair(
            this, (LanClient_S*)member_lc, (LanClient_S*)member_pair.value.ptr);
      }
    }
  }

  // 멤버 목록에서 삭제
  group->members_.Remove(member_id);

  member_lc->joined_lan_p2p_groups_.Remove(group_id);

  // remove from every P2PGroup's add-member-ack list
  AddMemberAckWaiters_RemoveRelated_MayTriggerJoinP2PMemberCompleteEvent(
      group.Get(), member_id, ResultCode::UserRequested);

  // 그룹을 파괴하던지 재정비하던지, 옵션에 따라.
  if (group->members_.IsEmpty() && !empty_p2p_group_allowed_) {
    const HostId group_id_to_remove = group->group_id_;

    if (lan_p2p_groups_.Remove(group_id_to_remove)) {
      host_id_factory_->Drop(GetAbsoluteTime(), group_id_to_remove);
      EnqueueP2PGroupRemoveEvent(group_id_to_remove);
    }
  } else {
    // 멤버가 모두 나갔어도 P2P group을 파괴하지는 않는다. 이는 명시적으로
    // 파괴되는 것이 정책이다.
    P2PGroup_CheckConsistency();
  }

  return true;
}

HostId LanServerImpl::CreateP2PGroup(const HostId* ClientHostIdList0,
                                     int32 count,
                                     const ByteArray& custom_field) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  // 빈 그룹도 만드는 것을 허용한다. (옵션에 따라)
  if (!empty_p2p_group_allowed_ && Count == 0) {
    return HostId_None;
  }

  // Array<HostId, InlineAllocator<128>> client_host_id_list;
  Array<HostId> client_host_id_list;
  if (count > 0) {
    // 클라 목록에서 중복 항목을 제거한다.
    client_host_id_list.Append(ClientHostIdList0, count);

    Algo::UnionDuplicateds(client_host_id_list);

    count = client_host_id_list.Count();

    // 클라이언트 유효성 체크
    for (int32 i = 0; i < count; ++i) {
      // Server인경우 클라이언트가 아니기때문에 유효성 체크 넘어감
      if (client_host_id_list[i] == HostId_Server) {
        continue;
      }

      auto lc = GetAuthedLanClientByHostId_NOLOCK(client_host_id_list[i]);
      if (lc == nullptr) {
        return HostId_None;
      }

      if (!lc->session_key_.KeyExists()) {
        EnqueueError(
            ResultInfo::From(ResultCode::Unexpected, lc->host_id_,
                             "CreateP2P 실패: session key 관련 오류."));
      }
    }
  }

  // 일단 빈 P2P group을 만든다.
  // 그리고 즉시 member들을 하나씩 추가한다.
  LanP2PGroupPtr_S new_group(new LanP2PGroup_S());
  new_group->group_id_ = host_id_factory_->Create(GetAbsoluteTime());
  lan_p2p_groups_.Add(new_group->group_id_, new_group);
  joined_p2p_group_key_gen_++;

  for (int32 i = 0; i < count; ++i) {
    JoinP2PGroup_INTERNAL(client_host_id_list[i], new_group->group_id_,
                          custom_field, joined_p2p_group_key_gen_);
  }

  return new_group->group_id_;
}

bool LanServerImpl::JoinP2PGroup_INTERNAL(HostId member_id, HostId group_id,
                                          const ByteArray& custom_field,
                                          uint32 joined_p2p_group_key_gen) {
  AssertIsLockedByCurrentThread();

  // 그룹 유효성 체크
  auto group = GetP2PGroupByHostId_NOLOCK(group_id);
  if (!group) {
    return false;
  }

  const uint32 ack_key_serial = joined_p2p_group_key_gen;

  // 클라 유효성 체크
  LanP2PGroupMember_S member_lc0;
  LanP2PGroupMemberBase_S* member_lc;
  if (group->members_.TryGetValue(member_id, member_lc0)) {
    // 이미 그룹내에 들어가 있다고 이벤트를 한다.
    EnqueueP2PAddMemberAckCompleteEvent(group_id, member_id,
                                        ResultCode::AlreadyExists);
    return true;
  }

  // 새로 넣으려는 멤버가 유효한 값인지?
  if (settings_.server_as_p2p_group_member_allowed &&
      member_id == HostId_Server) {
    member_lc = this;
  } else {
    member_lc = GetAuthedLanClientByHostId_NOLOCK(member_id);
    if (member_lc == nullptr ||
        IsDisposeLanClient_NOLOCK((LanClient_S*)member_lc)) {
      return false;
    }
  }

  const double absolute_time = clock_.AbsoluteSeconds();

  bool add_member_ack_waiter_done = false;

  // 그룹데이터 업데이트
  member_lc0.ptr = member_lc;
  member_lc0.joined_time = absolute_time;
  group->members_.Add(member_id, member_lc0);

  JoinedLanP2PGroupInfo group_info;
  group_info.lan_group_ptr = group;

  member_lc->joined_lan_p2p_groups_.Add(group_id, group_info);

  // add ack-waiters(with event time) for current members to new member
  for (auto& old_member_pair : group->members_) {
    auto old_member = old_member_pair.value.ptr;  // 기존 그룹의 각 멤버

    if (old_member != this) {  // 서버로부터 join ack 메시지 수신은 없으므로
      LanP2PGroup_S::AddMemberAckWaiter ack_waiter;
      ack_waiter.joining_member_host_id = member_id;
      ack_waiter.old_member_host_id = old_member->GetHostId();
      ack_waiter.event_id = ack_key_serial;
      ack_waiter.event_time = absolute_time;
      group->add_member_ack_waiters_.Add(ack_waiter);

      add_member_ack_waiter_done = true;
    }

    // 새 멤버로부터 기 멤버에 대한 join ack들이 일괄 와야 하므로 이것도
    // 추가해야. 단, 서버로부터는 join ack가 안오므로 제낀다.
    if (member_lc != this && member_lc != old_member) {
      LanP2PGroup_S::AddMemberAckWaiter ack_waiter;
      ack_waiter.joining_member_host_id = old_member->GetHostId();
      ack_waiter.old_member_host_id = member_id;
      ack_waiter.event_id = ack_key_serial;
      ack_waiter.event_time = absolute_time;
      group->add_member_ack_waiters_.Add(ack_waiter);

      add_member_ack_waiter_done = true;
    }

    // P2P connection pair를 갱신
    LanP2PConnectionStatePtr p2p_state;
    if (old_member->GetHostId() != HostId_Server &&
        member_lc->GetHostId() != HostId_Server) {
      auto old_member_as_lc = (LanClient_S*)old_member;
      auto new_member_as_lc = (LanClient_S*)member_lc;

      // 이미 P2P 직간접 통신중이 아니었다면 새로 쌍을 만들어 등록한다.
      // 이미 통신중이었다면 그냥 카운트만 증가시킨다.
      p2p_state = lan_p2p_connection_pair_list_.GetPair(
          old_member_as_lc->host_id_, new_member_as_lc->host_id_);
      if (!p2p_state.IsValid()) {
        p2p_state.Reset(new LanP2PConnectionState(this));
        p2p_state->p2p_connect_state =
            LanP2PConnectionState::P2PConnectState::P2PJoining;
        p2p_state->dup_count = 1;
        // 각 피어간 외부주소를 입력해준다.
        p2p_state->SetExternalAddr(old_member_as_lc->host_id_,
                                   old_member_as_lc->external_addr);
        p2p_state->SetExternalAddr(new_member_as_lc->host_id_,
                                   new_member_as_lc->external_addr);

        if (settings_.p2p_encrypted_messaging_enabled) {
          if (!CryptoRSA::CreateRandomBlock(
                  p2p_state->p2p_aes_session_key,
                  settings_.strong_encrypted_message_key_length) ||
              !CryptoRSA::CreateRandomBlock(
                  p2p_state->p2p_rc4_session_key,
                  settings_.weak_encrypted_message_key_length)) {
            if (intra_logger_) {
              const String text =
                  String::Format("%d와 %d간의 P2P session key make failed",
                                 (int32)old_member->GetHostId(),
                                 (int32)member_lc->GetHostId());
              intra_logger_->WriteLine(LogCategory::LP2P, *text);
            }
          }
        } else {
          p2p_state->p2p_aes_session_key = ByteArray();
          p2p_state->p2p_rc4_session_key = ByteArray();
        }

        p2p_state->holepunch_tag_ = random_.NextUuid();

        lan_p2p_connection_pair_list_.AddPair(old_member_as_lc,
                                              new_member_as_lc, p2p_state);
        // FUN_TRACE("%d-%d pair Add\n", old_member_as_lc->HostID,
        // new_member_as_lc->HostID);

        old_member_as_lc->lan_p2p_connection_pairs_.Add(p2p_state);
        new_member_as_lc->lan_p2p_connection_pairs_.Add(p2p_state);

        if (intra_logger_) {
          const String text = String::Format("Prepare P2P pair for %d <-> %d",
                                             (int32)old_member->GetHostId(),
                                             (int32)member_lc->GetHostId());
          intra_logger_->WriteLine(LogCategory::LP2P, *text);
        }
      } else {
        p2p_state->dup_count++;
      }
    }

    // 이게 있어야 받는 쪽에서는 member join noti를 받은 후에야
    // P2P RPC(relayed)를 받는 순서가 보장된다.
    AssertIsLockedByCurrentThread();

    if (old_member != this) {
      if (settings_.p2p_encrypted_messaging_enabled) {
        // P2P_AddMember RPC with heterogeneous session keys and event time
        // session key와 1st frame 번호를 보내는 것이 필요한 이유:
        // 릴레이 모드에서도 어쨌거나 frame number와 보안 통신이 필요하니껭.
        s2c_proxy_.P2PGroup_MemberJoin(
            old_member->GetHostId(), GSecureReliableSend_INTERNAL,
            group->group_id_, member_id, custom_field, ack_key_serial,
            p2p_state.IsValid() ? p2p_state->p2p_aes_session_key : ByteArray(),
            p2p_state.IsValid() ? p2p_state->p2p_rc4_session_key : ByteArray(),
            p2p_state.IsValid() ? p2p_state->holepunch_tag_ : Uuid::None);
      } else {
        // P2P_AddMember RPC with heterogeneous session keys and event time
        // session key와 1st frame 번호를 보내는 것이 필요한 이유:
        // 릴레이 모드에서도 어쨌거나 frame number와 보안 통신이 필요하니껭.
        s2c_proxy_.P2PGroup_MemberJoin_Unencrypted(
            old_member->GetHostId(), GReliableSend_INTERNAL, group->group_id_,
            member_id, custom_field, ack_key_serial,
            p2p_state.IsValid() ? p2p_state->holepunch_tag_ : Uuid::None);
      }
    }

    // 자기 자신에 대해서의 경우 이미 한번은 위 라인에서 보냈으므로
    // 이번에는 또 보내지는 않는다.
    if (old_member != member_lc && member_id != HostId_Server) {
      if (settings_.p2p_encrypted_messaging_enabled) {
        s2c_proxy_.P2PGroup_MemberJoin(
            member_id, GSecureReliableSend_INTERNAL, group->group_id_,
            old_member->GetHostId(), custom_field, ack_key_serial,
            p2p_state.IsValid() ? p2p_state->p2p_aes_session_key : ByteArray(),
            p2p_state.IsValid() ? p2p_state->p2p_rc4_session_key : ByteArray(),
            p2p_state.IsValid() ? p2p_state->holepunch_tag_ : Uuid::None);
      } else {
        s2c_proxy_.P2PGroup_MemberJoin_Unencrypted(
            member_id, GReliableSend_INTERNAL, group->group_id_,
            old_member->GetHostId(), custom_field, ack_key_serial,
            p2p_state.IsValid() ? p2p_state->holepunch_tag_ : Uuid::None);
      }
    }

    if (intra_logger_ && p2p_state) {
      const String text = String::Format("서버에서 발급한 holepunch tag: %s",
                                         *p2p_state->holepunch_tag_.ToString());
      intra_logger_->WriteLine(LogCategory::LP2P, *text);
    }
  }

  P2PGroup_CheckConsistency();

  // 성공적으로 멤버를 넣었으나 ack waiter가 추가되지 않은 경우(예: 빈 그룹에
  // 서버 1개만 추가) 바로 완료 콜백을 때린다.
  if (!add_member_ack_waiter_done) {
    EnqueueP2PAddMemberAckCompleteEvent(group_id, HostId_Server,
                                        ResultCode::Ok);
  }

  return true;
}

bool LanServerImpl::JoinP2PGroup(HostId member_id, HostId group_id,
                                 const ByteArray& custom_field) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  return JoinP2PGroup_INTERNAL(member_id, group_id, custom_field,
                               ++joined_p2p_group_key_gen_);
}

void LanServerImpl::EnqueueClientLeaveEvent(LanClient_S* lc,
                                            ResultCode result_code,
                                            ResultCode detail_code,
                                            const ByteArray& comment,
                                            SocketErrorCode socket_error) {
  AssertIsLockedByCurrentThread();

  if (callbacks_ && lc->host_id_ != HostId_None) {
    LocalEvent event(LocalEventType::ClientLeaveAfterDispose);
    event.result_info.Reset(new ResultInfo());
    event.result_info->result_code = result_code;
    event.result_info->detail_code = detail_code;
    event.NetClientInfo = lc->GetClientInfo();
    event.comment = comment;
    event.remote_id = lc->host_id_;
    lc->EnqueueLocalEvent(event);
  }

  // if (viz_agent_) {
  //  CScopedLock2 viz_agent_guard(viz_agent_->main_mutex_);
  //  viz_agent_->C2SProxy.NotifySrv_Clients_Remove(HostId_Server,
  //  GReliableSend_INTERNAL, lc->host_id_);
  //}
}

INetCoreCallbacks* LanServerImpl::GetCallbacks_NOLOCK() {
  AssertIsNotLockedByCurrentThread();
  return callbacks_;
}

LanServerImpl::LanServerImpl()
    : main_mutex_(),
      settings_(),
      pure_too_old_unmature_client_alarm_(
          NetConfig::purge_too_old_add_member_ack_timeout_sec),
      purge_too_old_add_member_ack_item_alarm_(
          NetConfig::purge_too_old_add_member_ack_timeout_sec),
      dispose_issued_remote_clients_alarm_(
          NetConfig::dispose_issued_remote_clients_timeout_sec),
      remove_too_old_tcp_send_packet_queue_alarm_(
          NetConfig::tcp_packet_board_long_interval_sec),
      disconnect_remote_client_on_timeout_alarm_(
          NetConfig::cs_ping_interval_sec),
      user_task_queue_(this) {
  server_socket_pool_ = ServerSocketPool::GetSharedPtr();

  internal_version_ = NetConfig::InternalLanVersion;

  settings_.message_max_length = NetConfig::MessageMaxLengthInServerLan;

  // tear_down_ = false;
  callbacks_ = nullptr;

  // timer_.Start();

  AttachProxy(&s2c_proxy_);
  AttachStub(&c2s_stub_);

  c2s_stub_.owner = this;

  user_task_is_running_ = false;

  empty_p2p_group_allowed_ = true;

  s2c_proxy_.engine_specific_only_ = true;
  c2s_stub_.engine_specific_only_ = true;

  joined_p2p_group_key_gen_ = 0;

  timer_callback_interval_ = 0;
  timer_callback_context_ = nullptr;

  total_tcp_recv_count_ = 0;
  total_tcp_recv_bytes_ = 0;
  total_tcp_send_count = 0;
  total_tcp_send_bytes = 0;
}

int32 LanServerImpl::GetClientHostIds(HostId* output, int32 output_length) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  int32 output_count = 0;
  for (auto& pair : authed_lan_clients_) {
    auto lc = pair.value;

    if (output_count < output_length) {
      output[output_count++] = lc->host_id_;
    } else {
      break;
    }
  }

  return output_count;
}

bool LanServerImpl::GetClientInfo(HostId client_id, NetClientInfo& out_info) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (auto lc = GetLanClientByHostId_NOLOCK(client_id)) {
    lc->GetClientInfo(out_info);
    return true;
  } else {
    return false;
  }
}

LanP2PGroupPtr_S LanServerImpl::GetP2PGroupByHostId_NOLOCK(HostId group_id) {
  AssertIsLockedByCurrentThread();

  return lan_p2p_groups_.FindRef(group_id);
}

bool LanServerImpl::GetP2PGroupInfo(HostId group_id, P2PGroupInfo& out_info) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (auto group = GetP2PGroupByHostId_NOLOCK(group_id)) {
    group->GetInfo(out_info);
    return true;
  } else {
    return false;
  }
}

// P2P 그룹을 host id list로 변환 후 배열에 이어붙인다.
// TODO 이어붙인다고???
void LanServerImpl::ConvertAndAppendP2PGroupToPeerList(
    HostId sendto, HostIdArray& additive_output) {
  AssertIsLockedByCurrentThread();

  // ConvertP2PGroupToPeerList는 한번만 호출되는 것이 아니다. 추가하는 형태로
  // 동작하므로, 여기서 리셋하면 안됨.
  if (auto group = GetP2PGroupByHostId_NOLOCK(sendto)) {
    /// group->members_.GenerateKeyArray(additive_output);
    for (const auto& member_pair : group->members_) {
      const HostId member_id = member_pair.key;
      additive_output.Add(member_id);
    }
  } else {
    //이미 dispose로 들어간 remote는 추가 하지 말자.
    if (HostId_Server == sendto || GetAuthedLanClientByHostId_NOLOCK(sendto)) {
      additive_output.Add(sendto);
    }
  }
}

bool LanServerImpl::GetP2PConnectionStats(HostId remote_id,
                                          P2PConnectionStats& out_stats) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (auto lc = GetAuthedLanClientByHostId_NOLOCK(remote_id)) {
    out_stats.total_p2p_count = 0;
    out_stats.direct_p2p_count = 0;
    out_stats.to_remote_peer_send_udp_message_attempt_count = 0;
    out_stats.to_remote_peer_send_udp_message_success_count = 0;

    for (auto pair : lc->lan_p2p_connection_pairs_) {
      if (pair.IsValid()) {
        out_stats.total_p2p_count++;
        out_stats.direct_p2p_count++;
      }
    }

    return true;
  }

  if (auto group = GetP2PGroupByHostId_NOLOCK(remote_id)) {
    // 그룹아이디라면 그룹에서 얻는다.
    out_stats.total_p2p_count = 0;
    out_stats.direct_p2p_count = 0;
    out_stats.to_remote_peer_send_udp_message_attempt_count = 0;
    out_stats.to_remote_peer_send_udp_message_success_count = 0;

    Array<LanP2PConnectionState*> visited_connection_states;
    visited_connection_states.Reserve((int32)MathBase::Max(
        pow((double)group->members_.Count(), 2) * 0.5, 1.0));
    for (auto member_pair : group->members_) {
      // 서버와의 연결은 통계에서 제외. (서버와 P2P가 허용된 경우에는 대상이
      // 서버가 될 수 있음.)
      if (member_pair.value.ptr->GetHostId() == HostId_Server) {
        continue;
      }

      auto member_as_lc = (LanClient_S*)member_pair.value.ptr;
      for (auto p2p_connection : member_as_lc->lan_p2p_connection_pairs_) {
        // 무효화된 경우는 제외.
        if (!p2p_connection.IsValid()) {
          continue;
        }

        // 다른 그룹일 경우에는 제외.
        if (!group->members_.Contains(p2p_connection->first_client->host_id_) ||
            !group->members_.Contains(
                p2p_connection->second_client->host_id_)) {
          continue;
        }

        // 이미 체크한 커넥션은 제외.
        if (visited_connection_states.Contains(p2p_connection.Get())) {
          continue;
        }

        out_stats.total_p2p_count++;
        out_stats.direct_p2p_count++;

        visited_connection_states.Add(p2p_connection.Get());
      }
    }

    return true;
  }

  return false;
}

ISendDest_S* LanServerImpl::GetSendDestByHostId_NOLOCK(HostId peer_id) {
  AssertIsLockedByCurrentThread();

  if (peer_id == HostId_Server) {
    return this;
  } else if (peer_id == HostId_None) {
    return &ISendDest_S::None;
  } else {
    return GetAuthedLanClientByHostId_NOLOCK(peer_id);
  }
}

bool LanServerImpl::IsFinalReceiveQueueEmpty() {
  AssertIsLockedByCurrentThread();
  return final_user_work_queue_.Count() == 0;
}

bool LanServerImpl::IsTaskRunning() {
  AssertIsLockedByCurrentThread();
  return user_task_is_running_;
}

void LanServerImpl::OnSetTaskRunningFlag(bool running) {
  AssertIsLockedByCurrentThread();
  user_task_is_running_ = running;
}

bool LanServerImpl::PopFirstUserWorkItem(FinalUserWorkItem& out_item) {
  AssertIsLockedByCurrentThread();
  if (!final_user_work_queue_.IsEmpty()) {
    out_item.From(final_user_work_queue_.Front(), HostId_Server);
    final_user_work_queue_.RemoveFront();
    return true;
  }

  return false;
}

// 이 함수는 마구 호출해도 괜찮다.
void LanServerImpl::IssueDisposeLanClient(
    LanClient_S* lc, ResultCode result_code, ResultCode detail_code,
    const ByteArray& comment, const char* where, SocketErrorCode socket_error) {
  AssertIsLockedByCurrentThread();

  if (intra_logger_) {
    intra_logger_->WriteLine(
        LogCategory::System,
        *String::Format("IssueDisposeLanClient() called at %s", where));
  }

  // 내부 절차는 아래와 같다.
  // 먼저 TCP socket을 close한다.
  // then issue중이던 것들은 에러가 발생한다. 혹은 issuerecv/send에서 이벤트
  // 없이 에러가 발생한다. 이때 recv on progress, send on progress가 중지될
  // 것이다. 물론 TCP에 한해서 말이다. 양쪽 모두 중지 확인되면 즉시 dispose를
  // 한다. group info 등을 파괴하는 것은 즉시 하지 않고 이 객체가 파괴되는 즉시
  // 하는게 맞다.
  if (!lc->dispose_waiter_.IsValid()) {
    lc->dispose_waiter_.Reset(new LanClient_S::DisposeWaiter());
    lc->dispose_waiter_->reason = result_code;
    lc->dispose_waiter_->detail = detail_code;
    lc->dispose_waiter_->comment = comment;
    lc->dispose_waiter_->socket_error = socket_error;
  }

  if (!dispose_issued_lan_clients_map_.Contains(lc)) {
    // authed에서 빼지 않음.
    // if (remove_from_collection) {
    //  LanClient_RemoveFromCollections(lc);
    //}

    //  이제는 여기서 disevent를 enque한다.
    if (lc->dispose_waiter_.IsValid()) {
      EnqueueClientLeaveEvent(
          lc, lc->dispose_waiter_->reason, lc->dispose_waiter_->detail,
          lc->dispose_waiter_->comment, lc->dispose_waiter_->socket_error);
    } else {
      EnqueueClientLeaveEvent(lc, ResultCode::DisconnectFromLocal,
                              ResultCode::ConnectServerTimeout, ByteArray(),
                              SocketErrorCode::Ok);
    }

    dispose_issued_lan_clients_map_.Add(lc, GetAbsoluteTime());
    //#ifdef TRACE_NEW_DISPOSE_ROUTINE
    //    FUN_TRACE("%d Client dispose add\n", (int)lc->host_id_);
    //#endif
  } else {
    //#ifdef TRACE_NEW_DISPOSE_ROUTINE
    //    FUN_TRACE("%d ContainsKey\n", (int)lc->host_id_);
    //#endif
    return;  // 이렇게 하면 아래 CloseSocketOnly가 자주호출되는 문제를 피할 듯.
  }

  // 이렇게 소켓을 닫으면 issue중이던 것들이 모두 종료한다. 그리고 나서 재
  // 시도를 하려고 해도 DisposeWaiter가 있으므로 되지 않을 것이다. 그러면
  // 안전하게 객체를 파괴 가능.
  lc->to_client_tcp_->socket_->CloseSocketHandleOnly();

  if (intra_logger_) {
    intra_logger_->WriteLine(
        LogCategory::System,
        *String::Format("CloseSocketHandleOnly() called at %s", __FUNCTION__));
  }

  lc->WarnTooShortDisposal(where);

  for (auto& group_pair : lc->joined_lan_p2p_groups_) {
    // remove from P2PGroup_Add ack info
    auto group = group_pair.value.lan_group_ptr;

    AddMemberAckWaiters_RemoveRelated_MayTriggerJoinP2PMemberCompleteEvent(
        group.Get(), lc->host_id_, ResultCode::DisconnectFromRemote);

    // notify member leave to related group members
    // Server는 P2PGroup_MemberLeave를 받을 필요가 없다.
    for (const auto& member_pair : group->members_) {
      if (member_pair.key != HostId_Server) {
        s2c_proxy_.P2PGroup_MemberLeave(member_pair.key, GReliableSend_INTERNAL,
                                        lc->host_id_, group->group_id_);
      }
    }

    // P2P그룹에서 제명
    group->members_.Remove(lc->host_id_);

    //@todo
    // 해당 그룹이 더이상 잔존할 이유가 없다면, 제거.
    // bAllowEmptyGroup을 통해서 남겨두는 이유는 무엇일까??
    // 일단 여러개의 그룹을 만든 후 Pool 처럼 사용하려는 이유가 아닐런지??
    if (group->members_.IsEmpty() && !empty_p2p_group_allowed_) {
      // P2P그룹이 파괴되어야 한다면...
      const HostId id_to_delete = group->group_id_;

      if (lan_p2p_groups_.Remove(id_to_delete)) {
        host_id_factory_->Drop(GetAbsoluteTime(), id_to_delete);
        EnqueueP2PGroupRemoveEvent(id_to_delete);
      }
    }

    // 클라 나감 이벤트에서 '나가기 처리 직전까지 잔존했던 종속 P2P그룹'을 줘야
    // 하므로 여기서 백업.
    lc->had_joined_p2p_groups_.Add(group->group_id_);
  }
  lc->joined_lan_p2p_groups_.Clear();

  // 클라가 P2P 연결이 되어 있는 상대방 목록을 찾아서 모두 제거
  lan_p2p_connection_pair_list_.RemovePairOfAnySide(lc);
  //#ifdef TRACE_NEW_DISPOSE_ROUTINE
  //  //FUN_TRACE("%d RemovePair Call!!\n", (int)lc->host_id_);
  //#endif

  // return false;
  // FUN_TRACE(LogCategory::System, "%s(this=%p, WSAGetLastError=%d)\n",
  // __FUNCTION__, this, WSAGetLastError());
}

void LanServerImpl::DisposeIssuedLanClients() {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  for (auto it = dispose_issued_lan_clients_map_.CreateIterator(); it; ++it) {
    auto lc = it->key;

    HardDisconnect_AutoPruneGoesTooLongClient(lc);

    // lc lock
    CScopedLock2 tcp_guard(lc->to_client_tcp_->GetMutex());
    CScopedLock2 tcp_issue_queue_guard(tcp_issue_queue_mutex_);

    const bool use_count_safe = lc->GetUseCount() == 0;

    // Tcp 중지 확인되면 즉시 dispose를 한다.
    // 300초가 넘으면 무조건 dispose를 한다. 300에 훨씬 못미치는 초 안에
    // 충분히 recv, send는 completion이 발생하기 때문이다.
    // 서버가 과부하가 걸리면 closesocket후 몇십초가 지나도 recv completion 이
    // 안올 가능성은 존재하기 마련. 하지만 300초씩이나 걸릴 정도면 막장
    // 상황이므로 차라리 서버를 끄는게 낫다.
    const bool tcp_close_safe =
        !lc->to_client_tcp_->recv_issued_ && !lc->to_client_tcp_->send_issued_;

    // 일거리가 남아있다면 지우지 아니 한다.
    const bool works_remain = (lc->task_subject_node_.GetListOwner() ||
                               lc->IsTaskRunning() || lc->GetListOwner());

    tcp_guard.Unlock();

    if (use_count_safe && tcp_close_safe && !works_remain) {
      ProcessOnClientDisposeCanSafe(lc);
      it.RemoveCurrent();
      lan_client_instances_.Remove(lc);
      delete lc;  // 실제로 여기서 제거함
    }
  }
}

double LanServerImpl::GetTime() {
  return !clock_.IsStopped() ? clock_.AbsoluteSeconds() : -1;
}

int32 LanServerImpl::GetClientCount() {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  return authed_lan_clients_.Count();
}

LanServerImpl::~LanServerImpl() {
  Stop();

  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  // RZ 내부에서도 쓰는 RPC까지 더 이상 참조되지 않음을 확인해야 하므로 여기서
  // 시행해야 한다.
  CleanupEveryProxyAndStub();  // 꼭 이걸 호출해서 미리 청소해놔야 한다.
}

NamedInetAddress LanServerImpl::GetRemoteIdentifiableLocalAddr() {
  if (tcp_listening_socket_.IsValid()) {
    NamedInetAddress result =
        NamedInetAddress(tcp_listening_socket_->GetSockName());

    // 3순위
    result.OverwriteHostNameIfExists(NetUtil::LocalAddressAt(0).ToString());

    // 2순위
    result.OverwriteHostNameIfExists(local_nic_addr_);

    // 1순위
    result.OverwriteHostNameIfExists(server_ip_alias_);

    // 최종적으로 설정된 주소가 unicast 주소가 아니라면, 그냥 첫번째 로컬 ip를
    // 넣도록 한다. 하지만 이는 fallback 처리이므로 로그를 남겨준다던지 하는것도
    // 좋을듯 싶다.
    if (!result.IsUnicast()) {
      result.Address = NetUtil::LocalAddressAt(0).ToString();
    }

    return result;
  }

  return NamedInetAddress::None;
}

void LanServerImpl::Tcp_LongTick() {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  int32 count = 0;
  const int32 total_count =
      authed_lan_clients_.Count() + candidate_lan_clients_.Count();
  if (total_count <= 0) {
    return;
  }

  // TODO inline allocator를 사용하는 array로 대체하는게 좋을듯함.
  // LanClient_S** lan_clients = (LanClient_S**)ALLOCA(sizeof(LanClient_S*) *
  // total_count); Array<LanClient_S*,InlineAllocator<256>> lan_clients;
  Array<LanClient_S*> lan_clients;
  lan_clients.ResizeUninitialized(total_count);

  // TCP에 대해서만 처리하기
  for (auto& pair : authed_lan_clients_) {
    auto lc = pair.value;

    lc->IncreaseUseCount();
    lan_clients[count++] = lc;
  }

  for (auto& pair : candidate_lan_clients_) {
    auto lc = pair.value;

    lc->IncreaseUseCount();
    lan_clients[count++] = lc;
  }

  const double absolute_time = GetAbsoluteTime();

  main_guard.Unlock();

  while (count > 0) {
#if USE_PARALLEL_FOR
#if (_MSC_VER >= 1400)
#pragma omp parallel for
#endif
#endif
    for (int32 lc_index = 0; lc_index < count; ++lc_index) {
      auto lc = lan_clients[lc_index];

      CScopedLock2 lc_tcp_send_queue_guard(
          lc->to_client_tcp_->GetSendQueueMutex(), false);

      if (lc_index != 0) {
        if (const bool lock_ok = lc_tcp_send_queue_guard.TryLock()) {
          lc->to_client_tcp_->LongTick(absolute_time);
          lc_tcp_send_queue_guard.Unlock();

          lc->DecreaseUseCount();
          lan_clients[lc_index] = lan_clients[--count];
        }
      } else {
        lc_tcp_send_queue_guard.Lock();
        lc->to_client_tcp_->LongTick(absolute_time);
        lc_tcp_send_queue_guard.Unlock();

        lc->DecreaseUseCount();
        lan_clients[lc_index] = lan_clients[--Count];
      }
    }
  }
}

void LanServerImpl::EnableIntraLog(const char* log_filename) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (intra_logger_) {
    return;
  }

  intra_logger_.Reset(LogWriter::New(log_filename));

  //@maxidea: NetServer에서는 지원하고 있지만, LanServer에서는 구지 필요 없어서
  //뺀듯..?
  // 모든 클라이언트들에게 로그를 보내라는 명령을 한다.
  // for (auto client_it = authed_lan_clients_.begin(); client_it !=
  // authed_lan_clients_.end(); ++client_it) {
  //  s2c_proxy_.EnableIntraLog(client_it->key, GReliableSend_INTERNAL);
  //}
}

void LanServerImpl::DisableIntraLog() {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  intra_logger_.Reset();

  //@maxidea: NetServer에서는 지원하고 있지만, LanServer에서는 구지 필요 없어서
  //뺀듯..?
  // 모든 클라이언트들에게 로그를 보내지 말라는 명령을 한다.
  // for (auto client_it = authed_lan_clients_.begin(); client_it !=
  // authed_lan_clients_.end(); ++client_it) {
  //  s2c_proxy_.DisableIntraLogging(i->key, GReliableSend_INTERNAL);
  //}
}

void LanServerImpl::Heartbeat_PerClient() {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  const double absolute_time = GetAbsoluteTime();

  for (auto pair : authed_lan_clients_) {
    auto lc = pair.value;

    if ((absolute_time - lc->last_tcp_stream_recv_time_) >
        settings_.default_timeout_sec) {
      if (intra_logger_) {
        const String text = String::Format(
            "Timeout %d host %lf delta %lf default_timeout_sec",
            (int32)lc->host_id_, absolute_time - lc->last_tcp_stream_recv_time_,
            settings_.default_timeout_sec);
        intra_logger_->WriteLine(LogCategory::System, *text);
      }

      IssueDisposeLanClient(lc, ResultCode::DisconnectFromRemote,
                            ResultCode::ConnectServerTimeout, ByteArray(),
                            __FUNCTION__, SocketErrorCode::Ok);
      continue;
    }

    RefreshSendQueuedAmountStat(lc);
  }
}

void LanServerImpl::EnqueueHackSuspectEvent(LanClient_S* lc,
                                            const char* statement,
                                            HackType hack_type) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (callbacks_) {
    LocalEvent event(LocalEventType::HackSuspected);
    event.result_info.Reset(new ResultInfo());
    event.hack_type = hack_type;
    event.remote_id = lc ? lc->host_id_ : HostId_None;
    event.result_info->comment = statement;
    EnqueueLocalEvent(event);
  }
}

void LanServerImpl::SetMessageMaxLength(int32 max_length) {
  if (max_length <= NetConfig::MessageMinLength) {
    throw InvalidArgumentException();
  }

  // 아예 전역 값도 수정하도록 한다.
  {
    CScopedLock2 config_write_guard(NetConfig::GetWriteMutex());
    NetConfig::message_max_length =
        MathBase::Max(NetConfig::message_max_length, max_length);
  }

  settings_.message_max_length = max_length;
}

void LanServerImpl::SetDefaultTimeoutTimeMilisec(uint32 timeout_msec) {
  SetDefaultTimeoutTimeSec(((double)timeout_msec) / 1000);
}

void LanServerImpl::SetDefaultTimeoutTimeSec(double timeout_sec) {
  AssertIsNotLockedByCurrentThread();

  if (timeout_sec < 1) {
    if (callbacks_) {
      LOG(LogNetEngine, Warning,
          "Too short timeout value. it may cause unfair disconnection.");
      return;
    }
  }

#ifndef _DEBUG
  if (timeout_sec > 240) {
    if (callbacks_) {
      LOG(LogNetEngine, Warning,
          "Too long timeout value. it may take a lot of time to detect lost "
          "connection.");
      return;
    }
  }
#endif

  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  CheckDefaultTimeoutTimeValidation(timeout_sec);
  settings_.default_timeout_sec = timeout_sec;
}

bool LanServerImpl::SetDirectP2PStartCondition(
    DirectP2PStartCondition condition) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (condition >= DirectP2PStartCondition::Last) {
    throw InvalidArgumentException();
  }

  settings_.direct_p2p_start_condition = condition;

  return true;
}

void LanServerImpl::GetStats(LanServerStats& out_stats) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  out_stats.Reset();

  out_stats.total_tcp_recv_bytes = total_tcp_recv_bytes_;
  out_stats.total_tcp_recv_count = total_tcp_recv_count_;
  out_stats.total_tcp_send_bytes = total_tcp_send_bytes_;
  out_stats.total_tcp_send_count = total_tcp_send_count_;

  out_stats.client_count = authed_lan_clients_.Count();

  for (auto& pair : lan_p2p_connection_pair_list_.List) {
    if (auto state = pair.value) {
      out_stats.p2p_connection_pair_count++;
      out_stats.p2p_direct_connection_pair_count++;
    }
  }
}

InetAddress LanServerImpl::GetTcpListenerLocalAddr() {
  return tcp_listening_socket_.IsValid() ? tcp_listening_socket_->GetSockName()
                                         : InetAddress::None;
}

void LanServerImpl::ConvertGroupToIndividualsAndUnion(
    int32 sendto_count, const HostId* sendto_list,
    HostIdArray& out_send_dest_list) {
  AssertIsLockedByCurrentThread();

  for (int32 sendto_index = 0; sendto_index < sendto_count; ++sendto_index) {
    if (sendto_list[sendto_index] != HostId_None) {
      ConvertAndAppendP2PGroupToPeerList(sendto_list[sendto_index],
                                         out_send_dest_list);
    }
  }

  // TODO
  // 이 구문은 아래 p2p relay 정리 전에 필요할 듯
  algo::UnionDuplicateds(out_send_dest_list);
}

// 특별한 처리 없이 바로 클라이언트에게 TCP로 데이터를 보낸다.
bool LanServerImpl::SendWithSplitter_DirectlyToClient_Copy(
    HostId client_id, const SendFragRefs& data_to_send,
    MessageReliability reliability, const SendOption& send_opt) {
  AssertIsLockedByCurrentThread();

  if (auto lc = GetAuthedLanClientByHostId_NOLOCK(client_id)) {
    CScopedLock2 lc_tcp_send_queue_guard(
        lc->to_client_tcp_->GetSendQueueMutex());

    lc->to_client_tcp_->SendWhenReady(data_to_send, TcpSendOption());

    if (reliability != MessageReliability::Reliable) {
      EnqueueError(ResultInfo::From(
          ResultCode::Unexpected, HostId_Server,
          "An unreliable message connot use LanServer and LanClient."));
      return false;
    }

    return true;
  }

  return false;
}

bool LanServerImpl::IsValidHostId(HostId host_id) {
  CScopedLock2 main_guard(main_mutex_);

  return IsValidHostId_NOLOCK(host_id);
}

void LanServerImpl::Convert_NOLOCK(LanSendDestInfoList_S& to,
                                   HostIdArray& from) {
  AssertIsLockedByCurrentThread();

  to.Resize(from.Count());

  for (int32 i = 0; i < from.Count(); ++i) {
    to[i].host_id = from[i];
    to[i].object = GetSendDestByHostId_NOLOCK(from[i]);
  }
}

String LanServerImpl::DumpGroupStatus() {
  //@todo ???
  return String();
}

bool LanServerImpl::NextEncryptCount(HostId remote_id,
                                     CryptoCountType& out_count) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (remote_id == HostId_Server) {  // server
    out_count = self_encrypt_count_;
    ++self_encrypt_count_;
    return true;
  } else {  // lan client
    if (auto lc = GetAuthedLanClientByHostId_NOLOCK(remote_id)) {
      out_count = lc->encrypt_count;
      ++lc->encrypt_count;
      return true;
    } else {
      // Unknown host id
      return false;
    }
  }
}

void LanServerImpl::PrevEncryptCount(HostId remote_id) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (remote_id == HostId_Server) {  // server
    --self_encrypt_count_;
  } else {  // lan client
    if (auto lc = GetAuthedLanClientByHostId_NOLOCK(remote_id)) {
      --lc->encrypt_count;
    }
  }
}

bool LanServerImpl::GetExpectedDecryptCount(HostId remote_id,
                                            CryptoCountType& out_count) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (remote_id == HostId_Server) {  // server
    out_count = self_decrypt_count_;
    return true;
  } else {  // lan client
    if (auto lc = GetAuthedLanClientByHostId_NOLOCK(remote_id)) {
      out_count = lc->decrypt_count_;
      return true;
    } else {
      return false;
    }
  }
}

bool LanServerImpl::NextDecryptCount(HostId remote_id) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (remote_id == HostId_Server) {  // server
    ++self_decrypt_count_;
    return true;
  } else {  // lan client
    if (auto lc = GetAuthedLanClientByHostId_NOLOCK(remote_id)) {
      ++lc->decrypt_count_;
      return true;
    } else {
      // Unknown host id
      return false;
    }
  }
}

bool LanServerImpl::IsConnectedClient(HostId client_id) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  return GetAuthedLanClientByHostId_NOLOCK(client_id) != nullptr;
}

void LanServerImpl::P2PGroup_CheckConsistency() {}

void LanServerImpl::DestroyEmptyP2PGroups() {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  for (auto it = lan_p2p_groups_.CreateIterator(); it; ++it) {
    auto group = it->value;

    if (group->members_.IsEmpty()) {
      const HostId group_id = group->group_id_;
      it.RemoveCurrent();
      host_id_factory_->Drop(GetAbsoluteTime(), group_id);
      EnqueueP2PGroupRemoveEvent(group_id);
    }
  }
}

void LanServerImpl::EnqueueP2PGroupRemoveEvent(HostId group_id) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (callbacks_) {
    LocalEvent event(LocalEventType::P2PGroupRemoved);
    event.remote_id = group_id;
    EnqueueLocalEvent(event);
  }
}

void LanServerImpl::EnqueueP2PDisconnectEvent(HostId member_id,
                                              HostId remote_id,
                                              ResultCode result_code) {
  AssertIsLockedByCurrentThread();

  if (callbacks_) {
    LocalEvent event(LocalEventType::P2PDisconnected);
    event.result_info = ResultInfo::From(result_code);
    event.member_id = member_id;
    event.remote_id = remote_id;
    EnqueueLocalEvent(event);
  }
}

// void LanServerImpl::EnqueueUnitTestFailEvent(const String& text) {
//  CScopedLock2 main_guard(main_mutex_);
//  CheckCriticalSectionDeadLock(__FUNCTION__);
//
//  if (callbacks_) {
//    LocalEvent event(LocalEventType::UnitTestFail);
//    event.result_info.Reset(new ResultInfo());
//    event.result_info->comment = text;
//    EnqueueLocalEvent(event);
//  }
//}

void LanServerImpl::OnSocketWarning(InternalSocket* socket,
                                    const String& text) {
  //@warning Deadlock
  // CScopedLock2 main_guard(main_mutex_);

  if (intra_logger_) {
    intra_logger_->WriteLine(LogCategory::System, *text);
  }
}

uint32 LanServerImpl::GetInternalVersion() { return internal_version_; }

void LanServerImpl::ShowError_NOLOCK(SharedPtr<ResultInfo> result_info) {
  if (intra_logger_) {
    intra_logger_->WriteLine(LogCategory::System, *result_info->ToString());
  }

  NetCoreImpl::ShowError_NOLOCK(result_info);
}

void LanServerImpl::ShowWarning_NOLOCK(SharedPtr<ResultInfo> result_info) {
  if (intra_logger_) {
    const String text =
        *String::Format("WARNING: %s", *result_info->ToString());
    intra_logger_->WriteLine(LogCategory::System, *text);
  }

  if (callbacks_ /*&& !tear_down_*/) {
    callbacks_->OnWarning(result_info.Get());
  }
}

void LanServerImpl::SetCallbacks(ILanServerCallbacks* callbacks) {
  if (AsyncCallbackMayOccur()) {
    // TODO Exception 클래스를 특수화하는게 좋을듯함.
    // CAsyncCallbackOccurException() 같은...?  이름은 좀더 생각을 해봐야할듯..
    throw Exception(
        "Already async callback may occur.  Server start or client connection "
        "should have not been done before here.");
  }

  AssertIsNotLockedByCurrentThread();
  callbacks_ = callbacks;
}

void LanServerImpl::EnqueueError(SharedPtr<ResultInfo> result_info) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (callbacks_) {
    LocalEvent event(LocalEventType::error);
    event.result_info.Reset(new ResultInfo());
    event.result_info->result_code = result_info->result_code;
    event.result_info->comment = result_info->comment;
    event.remote_id = result_info->remote;
    event.remote_addr = result_info->remote_addr;
    EnqueueLocalEvent(event);
  }
}

void LanServerImpl::EnqueueWarning(SharedPtr<ResultInfo> result_info) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (callbacks_) {
    LocalEvent event(LocalEventType::Warning);
    event.result_info.Reset(new ResultInfo());
    event.result_info->result_code = result_info->result_code;
    event.result_info->comment = result_info->comment;
    event.remote_id = result_info->remote;
    event.remote_addr = result_info->remote_addr;
    EnqueueLocalEvent(event);
  }
}

void LanServerImpl::EnqueueWarning(const InetAddress& sender,
                                   const char* text) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  //@todo Tcp어드레스가 유니크할까??
  auto lc = GetCandidateLanClientByTcpAddr(sender);
  EnqueueWarning(ResultInfo::From(ResultCode::InvalidPacketFormat,
                                  lc ? lc->host_id_ : HostId_None, text));
}

void LanServerImpl::EnqueueClientJoinApproveDetermine(
    const InetAddress& client_tcp_addr, const ByteArray& request) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  fun_check(client_tcp_addr.IsUnicast());

  if (callbacks_) {
    LocalEvent event(LocalEventType::ClientJoinCandidate);
    event.remote_addr = client_tcp_addr;
    event.connection_request = request;
    EnqueueLocalEvent(event);
  }
}

void LanServerImpl::ProcessOnClientJoinRejected(LanClient_S* lc,
                                                const ByteArray& response) {
  AssertIsLockedByCurrentThread();
  lc->to_client_tcp_->AssertIsSendQueueNotLockedByCurrentThread();

  MessageOut msg;
  lf::Write(msg, MessageType::NotifyServerDeniedConnection);
  lf::Write(msg, response);  // TODO 참조로도 넘길 수 있을터인데,... 뭐 호출
                             // 빈도는 크지 않을테니 상관 없으려나??
  {
    CScopedLock2 tcp_send_queue_guard(lc->to_client_tcp_->GetSendQueueMutex());
    lc->to_client_tcp_->SendWhenReady(SendFragRefs(msg), TcpSendOption());
  }
}

void LanServerImpl::ProcessOnClientJoinApproved(LanClient_S* lc,
                                                const ByteArray& response) {
  // CScopedLock2 main_guard(main_mutex_);
  AssertIsLockedByCurrentThread();

  if (lc == nullptr) {
    throw Exception("Unexpected at candidate remote client removal.");
  }

  // Promote unmature Client to mature, give new host_id
  const HostId new_host_id = host_id_factory_->Create(GetAbsoluteTime());
  lc->host_id_ = new_host_id;

  //@note 인증이 완료 되었으므로, 후보 목록에서 제거하고, 인증된 목록에 추가.
  candidate_lan_clients_.Remove(lc);
  authed_lan_clients_.Add(new_host_id, lc);

  // Send weLanClientome with host_id
  {
    fun_check(lc->to_client_tcp_->remote_addr_.IsUnicast());

    MessageOut msg;
    lf::Write(msg, MessageType::NotifyServerConnectSuccess);
    lf::Write(msg, new_host_id);
    lf::Write(msg, instance_tag_);
    lf::Write(msg, response);
    lf::Write(msg, lc->to_client_tcp_->remote_addr_);

    CScopedLock2 lc_tcp_send_queue_guard(
        lc->to_client_tcp_->GetSendQueueMutex());
    lc->to_client_tcp_->SendWhenReady(SendFragRefs(msg), TcpSendOption());
  }

  {
    LocalEvent event(LocalEventType::ClientJoinApproved);
    event.NetClientInfo = lc->GetClientInfo();
    event.remote_id = lc->host_id_;
    lc->EnqueueLocalEvent(event);
  }
}

LanClient_S* LanServerImpl::GetCandidateLanClientByTcpAddr(
    const InetAddress& client_addr) {
  AssertIsLockedByCurrentThread();
  return candidate_lan_clients_.FindRef(client_addr);
}

//이미 서버가 기동되었는지 체크
bool LanServerImpl::AsyncCallbackMayOccur() {
  return listener_thread_.IsValid();
}

void LanServerImpl::EnqueueLocalEvent(LocalEvent& event) {
  AssertIsLockedByCurrentThread();

  if (listener_thread_.IsValid()) {
    final_user_work_queue_.Enqueue(event);
    user_task_queue_.AddTaskSubject(this);
  }
}

void LanServerImpl::ProcessOneLocalEvent(LocalEvent& event) {
  AssertIsNotLockedByCurrentThread();

  if (callbacks_ /* && !tear_down_*/) {
    try {
      switch (event.type) {
        case LocalEventType::ClientLeaveAfterDispose:
          callbacks_->OnClientLeft(event.NetClientInfo.Get(),
                                   event.result_info.Get(), event.comment);
          break;

        case LocalEventType::ClientJoinApproved:
          callbacks_->OnClientJoined(event.NetClientInfo.Get());
          break;

        case LocalEventType::P2PAddMemberAckComplete:
          callbacks_->OnP2PGroupJoinMemberAckComplete(
              event.group_id, event.member_id, event.result_info->result_code);
          break;

        case LocalEventType::GroupP2PEnabled:
          callbacks_->OnGroupP2PConnectionComplete(event.group_id);
          break;

        case LocalEventType::HackSuspected:
          callbacks_->OnClientHackSuspected(event.remote_id, event.hack_type);
          break;

        case LocalEventType::P2PGroupRemoved:
          callbacks_->OnP2PGroupRemoved(event.remote_id);
          break;

        case LocalEventType::DirectP2PEnabled:
          callbacks_->OnP2PConnectionEstablished(event.member_id,
                                                 event.remote_id);
          break;

        case LocalEventType::P2PDisconnected:
          callbacks_->OnP2PDisconnected(event.member_id, event.remote_id,
                                        event.result_info->result_code);
          break;

        case LocalEventType::TcpListenFail:
          ShowError_NOLOCK(ResultInfo::From(ResultCode::ServerPortListenFailure,
                                            HostId_Server));
          break;

          // case LocalEventType::UnitTestFail:
          //  ShowError_NOLOCK(ResultInfo::From(ResultCode::UnitTestFailed,
          //  HostId_Server, event.result_info->comment)); break;

        case LocalEventType::error:
          ShowError_NOLOCK(ResultInfo::From(event.result_info->result_code,
                                            event.remote_id,
                                            event.result_info->comment));
          break;

        case LocalEventType::Warning:
          ShowWarning_NOLOCK(ResultInfo::From(event.result_info->result_code,
                                              event.remote_id,
                                              event.result_info->comment));
          break;

        case LocalEventType::ClientJoinCandidate: {
          ByteArray response;
          const bool is_approved = callbacks_->OnConnectionRequest(
              event.remote_addr, event.connection_request, response);

          CScopedLock2 main_guard(main_mutex_);
          CheckCriticalSectionDeadLock(__FUNCTION__);

          if (auto lc = GetCandidateLanClientByTcpAddr(event.remote_addr)) {
            if (is_approved) {
              ProcessOnClientJoinApproved(lc, response);
            } else {
              ProcessOnClientJoinRejected(lc, response);
            }
          }
        }
      }
    } catch (Exception& e) {
      if (callbacks_ /*&& !tear_down_*/) {
        callbacks_->OnException(event.remote_id, e);
      }
    } catch (std::exception& e) {
      if (callbacks_ /*&& !tear_down_*/) {
        callbacks_->OnException(event.remote_id, Exception(e));
      }
    }  // catch (_com_error& e) {
    //  if (callbacks_/* && !tear_down_*/) {
    //    callbacks_->OnException(event.remote_id, Exception(e));
    //  }
    //} catch (void* e) {
    //  if (callbacks_/* && !tear_down_*/) {
    //    callbacks_->OnException(event.remote_id, Exception(e));
    //  }
    //}
#ifdef ALLOW_CATCH_UNHANDLED_EVEN_FOR_USER_ROUTINE
    catch (...) {
      if (callbacks_ /*&& !tear_down_*/) {
        Exception e;
        e.exception_type = ExceptionType::Unhandled;
        callbacks_->OnException(event.remote_id, e);
      }
    }
#endif
  }
}

String LanServerImpl::GetConfigString() {
  return String::Format("Listen=%s", *local_nic_addr_);
}

int32 LanServerImpl::GetMessageMaxLength() {
  return settings_.message_max_length;
}

HostId LanServerImpl::GetLocalHostId() { return HostId_Server; }

void LanServerImpl::RequestAutoPrune(LanClient_S* lc) {
  // 클라 추방시 소켓을 바로 닫으면 직전에 보낸 RPC가 안갈 수 있다.
  // 따라서 클라에게 자진 탈퇴를 종용하되 시한을 둔다.

  if (lc->request_auto_prune_start_time_ == 0) {  // Once!
    lc->request_auto_prune_start_time_ =
        GetAbsoluteTime();  //@note 설정 여부에 시간을 기록해두는게 좀더
                            //좋을듯..

    //@note 서버에서 바로 끊기 보다는, 클라이언트들에게 대기중인 메시지들을
    //      온전하게 처리한 후 안전하게 연결을 수 있도록 한다.
    s2c_proxy_.RequestAutoPrune(lc->host_id_, GReliableSend_INTERNAL);
  }
}

void LanServerImpl::HardDisconnect_AutoPruneGoesTooLongClient(LanClient_S* lc) {
  // 5초로 잡아야 서버가 클라에게 많이 보내고 있던 중에도 직전 RPC의 확실한
  // 송신이 되므로
  if (lc->request_auto_prune_start_time_ != 0.0 &&
      (GetAbsoluteTime() - lc->request_auto_prune_start_time_) > 5) {
    lc->request_auto_prune_start_time_ = 0.0;

    // Caller에서는 FunNet.LanServerImpl.AuthedLanClients를 iter중이 아니므로
    // 안전
    IssueDisposeLanClient(lc, ResultCode::DisconnectFromLocal,
                          ResultCode::ConnectServerTimeout, ByteArray(),
                          __FUNCTION__, SocketErrorCode::Ok);
  }
}

bool LanServerImpl::IsNagleAlgorithmEnabled() {
  return settings_.bEnableNagleAlgorithm;
}

bool LanServerImpl::IsValidHostId_NOLOCK(HostId host_id) {
  AssertIsLockedByCurrentThread();
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (host_id == HostId_Server) {
    return true;
  } else if (GetLanClientByHostId_NOLOCK(host_id)) {
    return true;
  } else if (GetP2PGroupByHostId_NOLOCK(host_id).IsValid()) {
    return true;
  } else {
    return false;
  }
}

void LanServerImpl::RefreshSendQueuedAmountStat(LanClient_S* lc) {
  // 송신큐 잔여 총량을 산출한다. (TCP만)
  uint32 tcp_queued_amount = 0;
  {
    CScopedLock2 lc_tcp_send_queue_guard(
        lc->to_client_tcp_->GetSendQueueMutex());
    tcp_queued_amount = lc->to_client_tcp_->send_queue_.GetLength();
  }

  lc->send_queued_amount_in_byte_ = tcp_queued_amount;
}

void LanServerImpl::AllowEmptyP2PGroup(bool allow) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (AsyncCallbackMayOccur()) {
    throw Exception(
        "Cannot set AllowEmptyP2PGroup after the server has started.");
  }

  empty_p2p_group_allowed_ = allow;
}

void LanServerImpl::GetUserWorkerThreadInfo(Array<ThreadInfo>& output) {
  // CScopedLock2 main_guard(main_mutex_);
  // CheckCriticalSectionDeadLock(__FUNCTION__);

  if (user_thread_pool_.IsValid()) {
    user_thread_pool_->GetThreadInfos(output);
  }
}

void LanServerImpl::GetNetWorkerThreadInfo(Array<ThreadInfo>& output) {
  // CScopedLock2 main_guard(main_mutex_);
  // CheckCriticalSectionDeadLock(__FUNCTION__);

  if (net_thread_pool_.IsValid()) {
    net_thread_pool_->GetThreadInfos(output);
  }
}

void LanServerImpl::LanClient_RemoveFromCollections(LanClient_S* lc) {
  // 목록에서 제거.

  // Remove candidate list
  candidate_lan_clients_.Remove(lc);

  // Remove authed list
  authed_lan_clients_.Remove(lc->host_id_);

  // Free hostid
  host_id_factory_->Drop(GetAbsoluteTime(), lc->host_id_);
}

void LanServerImpl::EnqueueP2PAddMemberAckCompleteEvent(HostId group_id,
                                                        HostId added_member_id,
                                                        ResultCode result) {
  // Enqueue 'completed join' event
  LocalEvent event(LocalEventType::P2PAddMemberAckComplete);
  event.group_id = group_id;
  event.member_id = added_member_id;
  event.remote_id = added_member_id;
  event.result_info = ResultInfo::From(result);
  EnqueueLocalEvent(event);
}

// MemberId이 들어있는 AddMemberAckWaiter 객체를 목록에서 찾아서 모두 제거한다.
void LanServerImpl::
    AddMemberAckWaiters_RemoveRelated_MayTriggerJoinP2PMemberCompleteEvent(
        LanP2PGroup_S* group, HostId member_id, ResultCode reason) {
  Array<int32> DelList;
  Set<HostId> JoiningMemberDelList;

  for (int32 i = group->add_member_ack_waiters_.Count() - 1; i >= 0; --i) {
    auto& ack = group->add_member_ack_waiters_[i];

    if (ack.joining_member_host_id == member_id ||
        ack.old_member_host_id == member_id) {
      DelList.Add(i);

      // 제거된 항목이 추가완료대기를 기다리는 피어에 대한 것일테니
      // 추가완료대기중인 신규진입피어 목록만 따로 모아둔다.
      JoiningMemberDelList.Add(ack.joining_member_host_id);
    }
  }

  for (int32 i = 0; i < DelList.Count(); ++i) {
    group->add_member_ack_waiters_.RemoveAtSwap(DelList[i]);
  }

  // MemberId에 대한 OnP2PGroupJoinMemberAckComplete 대기에 대한 콜백에 대한
  // 정리. 중도 실패되어도 OnP2PGroupJoinMemberAckComplete 콜백을 되게 해주어야
  // 하니까.
  for (auto JoiningMemberId : JoiningMemberDelList) {
    if (!group->add_member_ack_waiters_.AckWaitingItemExists(JoiningMemberId)) {
      EnqueueP2PAddMemberAckCompleteEvent(group->group_id_, JoiningMemberId,
                                          reason);
    }
  }
}

bool LanServerImpl::UpdateGroupP2PConnection(HostId group_id) {
  AssertIsLockedByCurrentThread();

  if (auto group = GetP2PGroupByHostId_NOLOCK(group_id)) {
    for (auto& member_pair1 : group->members_) {
      // 서버일 경우에는 이미 연결된 걸로 간주함.
      if (member_pair1.key == HostId_Server) {
        continue;
      }

      for (auto& member_pair2 : group->members_) {
        // 서버일 경우에는 이미 연결된 걸로 간주함.
        if (member_pair2.key == HostId_Server) {
          continue;
        }

        auto state = lan_p2p_connection_pair_list_.GetPair(
            member_pair1.value.ptr->GetHostId(),
            member_pair2.value.ptr->GetHostId());
        if (state->p2p_connect_state !=
            LanP2PConnectionState::P2PConnectState::P2PConnected) {
          return false;
        }
      }
    }

    // 그룹의 모든 p2p연결이 성사되었다.
    group->all_peers_connected = true;

    // 그룹 connection 완료 Event를 띄운다.
    EnqueueGroupP2PConnectionCompleteEvent(group_id);

    // 해당 그룹의 각 클라이언트에게 알린다.
    for (auto& member_pair : group->members_) {
      s2c_proxy_.GroupP2PConnectionComplete(member_pair.value.ptr->GetHostId(),
                                            GReliableSend_INTERNAL, group_id);
    }
  }

  return true;
}

void LanServerImpl::EnqueueGroupP2PConnectionCompleteEvent(HostId group_id) {
  // Enqueue 'group p2p completed' event
  LocalEvent event(LocalEventType::GroupP2PEnabled);
  event.group_id = group_id;
  EnqueueLocalEvent(event);
}

void LanServerImpl::EnqueueP2PConnectionEstablishedEvent(HostId member_id,
                                                         HostId remote_id) {
  // Enqueue event
  LocalEvent event(LocalEventType::DirectP2PEnabled);
  event.member_id = member_id;
  event.remote_id = remote_id;
  EnqueueLocalEvent(event);
}

bool LanServerImpl::IsDisposeLanClient_NOLOCK(LanClient_S* lc) {
  return dispose_issued_lan_clients_map_.Contains(lc);
}

ITaskSubject* LanServerImpl::GetTaskSubjectByHostId_NOLOCK(
    HostId subject_host_id) {
  AssertIsLockedByCurrentThread();
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (subject_host_id == HostId_Server) {
    return this;
  } else {
    return GetLanClientByHostId_NOLOCK(subject_host_id);
  }
}

bool LanServerImpl::SetHostTag(HostId host_id, void* host_tag) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (auto subject = GetTaskSubjectByHostId_NOLOCK(host_id)) {
    subject->host_tag_ = host_tag;
    return true;
  } else {
    return false;
  }
}

void LanServerImpl::CheckCriticalSectionDeadLock_INTERNAL(const char* where) {
  for (auto lc : lan_client_instances_) {
    if (lc->IsLockedByCurrentThread() == true) {
      LOG(LogNetEngine, Warning, "LanClient_S DeadLock!! - %s", where);
    }
  }

  if (tcp_issue_queue_mutex_.IsLockedByCurrentThread() == true) {
    LOG(LogNetEngine, Warning, "LanServer TcpSendIssueQueueCS DeadLock!! - %s",
        where);
  }
}

void LanServerImpl::OnSocketIoCompletion(Array<IHostObject*>& send_issued_pool,
                                         ReceivedMessageList& msg_list,
                                         CompletionStatus& completion) {
  AssertIsNotLockedByCurrentThread();

  LanClient_S* lc = nullptr;
  try {
    lc = LeanDynamicCast2ForLanClient(completion.completion_context);
    if (lc) {
      if (completion.type == CompletionType::Receive) {  // Receive
        IoCompletion_TcpRecvCompletionCase(completion, lc, msg_list);
      } else if (completion.type == CompletionType::Send) {  // Send
        IoCompletion_TcpSendCompletionCase(completion, lc);
      } else if (completion.type ==
                 CompletionType::ReferCustomValue) {  // Custom(User)
        IoCompletion_TcpCustomValueCase(completion, lc);
      }
    }
  } catch (std::exception&) {
    CatchThreadExceptionAndPurgeClient(lc, __FUNCTION__, "std.exception");
  }  // catch (_com_error&) {
  //  CatchThreadExceptionAndPurgeClient(lc, __FUNCTION__, "_com_error");
  //} catch (void*) {
  //  CatchThreadExceptionAndPurgeClient(lc, __FUNCTION__, "void*");
  //}
}

void LanServerImpl::IoCompletion_TcpCustomValueCase(
    CompletionStatus& completion, LanClient_S* lc) {
  AssertIsNotLockedByCurrentThread();

  switch ((IocpCustomValue)completion.custom_value) {
    case IocpCustomValue::NewClient:  // 처음 시작 조건
      // 서버 종료 조건이 아니면 클라를 IOCP에 엮고 다음 송신 절차를 시작한다.
      IoCompletion_NewClientCase(lc);
      break;
  }
}

void LanServerImpl::IoCompletion_NewClientCase(LanClient_S* lc) {
  // associate TCP socket with IOCP
  AssertIsNotLockedByCurrentThread();
  lc->AssertIsZeroUseCount();

  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  net_thread_pool_->AssociateSocket(lc->to_client_tcp_->socket_.Get());

  // Send public key, one of server UDP ports
  MessageOut msg;
  lf::Write(msg, MessageType::NotifyServerConnectionHint);
  lf::Write(msg, intra_logger_.IsValid());
  lf::Write(msg, settings_);
  lf::Write(msg, public_key_blob_);

  main_guard.Unlock();
  {
    CScopedLock2 lc_tcp_send_queue_guard(
        lc->to_client_tcp_->GetSendQueueMutex());
    lc->to_client_tcp_->SendWhenReady(SendFragRefs(msg), TcpSendOption());
  }

  CScopedLock2 lc_tcp_guard(lc->to_client_tcp_->GetMutex());

  const SocketErrorCode socket_error = lc->to_client_tcp_->IssueRecvAndCheck();
  if (socket_error != SocketErrorCode::Ok) {
    lc_tcp_guard.Unlock();

    lc->AssertIsNotLockedByCurrentThread();
    main_guard.Lock();

    lc->IssueDispose(ResultCode::DisconnectFromRemote,
                     ResultCode::TCPConnectFailure, ByteArray(), __FUNCTION__,
                     socket_error);
    lc->WarnTooShortDisposal(__FUNCTION__);
  }

  lc_tcp_guard.Unlock();

  // new accept case에 대한 decrease
  lc->DecreaseUseCount();
}

void LanServerImpl::IoCompletion_TcpSendCompletionCase(
    CompletionStatus& completion, LanClient_S* lc) {
  AssertIsNotLockedByCurrentThread();
  lc->AssertIsNotLockedByCurrentThread();

  ScopedUseCounter counter(*lc);
  CScopedLock2 lc_tcp_guard(lc->to_client_tcp_->GetMutex());

  lc->to_client_tcp_->send_issued_ = false;

  // 송신의 경우 0바이트를 보내는 경우도 있으므로 <=가 아닌 < 비교이다.
  if (completion.completed_length < 0) {  // Don't care completion.socket_error
    lc_tcp_guard.Unlock();

    lc->to_client_tcp_->AssertIsNotLockedByCurrentThread();

    CScopedLock2 main_guard(main_mutex_);
    CheckCriticalSectionDeadLock(__FUNCTION__);

    // TCP 연결이 끊어졌음을 의미한다. 따라서 에러 처리한다.
    const String comment = String::Format("%s: error: %d", __FUNCTION__,
                                          (int32)completion.socket_error);
    IssueDisposeLanClient(lc, ResultCode::DisconnectFromRemote,
                          ResultCode::TCPConnectFailure, ByteArray(), *comment,
                          completion.socket_error);
  } else {
    // 송신 큐에서 완료된 만큼의 데이터를 제거한다. 그리고 다음 송신을 건다.
    {
      CScopedLock2 lc_tcp_send_queue_guard(
          lc->to_client_tcp_->GetSendQueueMutex());
      lc->to_client_tcp_->send_queue_.DequeueNoCopy(
          completion.completed_length);
    }

    const SocketErrorCode socket_error =
        lc->to_client_tcp_->ConditionalIssueSend(GetAbsoluteTime());
    if (socket_error != SocketErrorCode::Ok) {
      lc_tcp_guard.Unlock();

      lc->to_client_tcp_->AssertIsNotLockedByCurrentThread();

      CScopedLock2 main_guard(main_mutex_);
      CheckCriticalSectionDeadLock(__FUNCTION__);

      lc->IssueDispose(ResultCode::DisconnectFromRemote,
                       ResultCode::TCPConnectFailure, ByteArray(), __FUNCTION__,
                       socket_error);
      lc->WarnTooShortDisposal(__FUNCTION__);
    }

    // Update stats
    total_tcp_send_count++;
    total_tcp_send_bytes += completion.completed_length;
  }
}

void LanServerImpl::IoCompletion_TcpRecvCompletionCase(
    CompletionStatus& completion, LanClient_S* lc,
    ReceivedMessageList& msg_list) {
  AssertIsNotLockedByCurrentThread();
  ScopedUseCounter counter(*lc);
  CScopedLock2 lc_tcp_guard(lc->to_client_tcp_->GetMutex());

  // 수신의 경우 0바이트 수신했음 혹은 음수바이트 수신했음이면 연결에 문제가
  // 발생한 것이므로 디스해야 한다.
  if (completion.completed_length <= 0) {
    lc->to_client_tcp_->recv_issued_ = false;

    lc_tcp_guard.Unlock();

    CScopedLock2 main_guard(main_mutex_);
    CheckCriticalSectionDeadLock(__FUNCTION__);

    // TCP socket에서 연결이 실패했으므로 연결 해제 처리를 한다.
    IssueDisposeLanClient(lc, ResultCode::DisconnectFromRemote,
                          ResultCode::TCPConnectFailure, ByteArray(),
                          __FUNCTION__, completion.socket_error);
    lc->WarnTooShortDisposal(__FUNCTION__);

    if (intra_logger_) {
      const String text = String::Format(
          "서버: 클라이언트 연결 해제.  host_id: %d, addr: %s, socket_error: "
          "%d",
          (int32)lc->host_id_, *lc->to_client_tcp_->remote_addr_.ToString(),
          (int32)completion.socket_error);
      intra_logger_->WriteLine(LogCategory::System, *text);
    }
  } else {
    // 수신 큐에서 받은 데이터를 꺼낸 후 ...
    lc->to_client_tcp_->recv_stream_.EnqueueCopy(
        lc->to_client_tcp_->socket_->GetRecvBufferPtr(),
        completion.completed_length);

    // msg_list.EmptyAndKeepCapacity();
    msg_list.Reset();  // keep capacity

    const ResultCode extract_result =
        lc->ExtractMessagesFromTcpStream(msg_list);
    if (extract_result != ResultCode::Ok) {
      lc->to_client_tcp_->recv_issued_ = false;

      lc_tcp_guard.Unlock();

      CScopedLock2 main_guard(main_mutex_);
      CheckCriticalSectionDeadLock(__FUNCTION__);

      IssueDisposeLanClient(lc, extract_result, ResultCode::TCPConnectFailure,
                            ByteArray(), __FUNCTION__, SocketErrorCode::Ok);
      return;
    }

    lc_tcp_guard.Unlock();

    // volatile이다. 크게 상관있는 값이 아니므로 이렇게 처리.
    // Update stats
    lc->last_tcp_stream_recv_time_ = GetAbsoluteTime();
    total_tcp_recv_count_++;
    total_tcp_recv_bytes_ += completion.completed_length;

    IoCompletion_ProcessMessageOrMoveToFinalRecvQueue(lc, msg_list);

    // 다음 recv를 건다.
    lc_tcp_guard.Lock();

    lc->to_client_tcp_->recv_issued_ = false;
    const SocketErrorCode socket_error =
        lc->to_client_tcp_->IssueRecvAndCheck();
    if (socket_error != SocketErrorCode::Ok) {
      lc_tcp_guard.Unlock();

      CScopedLock2 main_guard(main_mutex_);
      CheckCriticalSectionDeadLock(__FUNCTION__);

      lc->IssueDispose(ResultCode::DisconnectFromRemote,
                       ResultCode::TCPConnectFailure, ByteArray(), __FUNCTION__,
                       socket_error);
      lc->WarnTooShortDisposal(__FUNCTION__);
    }
  }
}

void LanServerImpl::IoCompletion_ProcessMessageOrMoveToFinalRecvQueue(
    LanClient_S* lc, ReceivedMessageList& extracted_msg_list) {
  AssertIsNotLockedByCurrentThread();

  // Final recv queue로 옮기거나 여기서 처리한다.
  for (auto& received_msg : extracted_msg_list) {
    ASSERT_OR_HACKED(received_msg.unsafe_message.AtBegin());
    IoCompletion_ProcessMessage_EngineLayer(received_msg, lc);
  }
}

bool LanServerImpl::IoCompletion_ProcessMessage_EngineLayer(
    ReceivedMessage& received_msg, LanClient_S* lc) {
  AssertIsNotLockedByCurrentThread();

  auto& msg = received_msg.unsafe_message;
  const int32 saved_read_pos = msg.Tell();

  MessageType msg_type;
  if (!lf::Read(msg, msg_type)) {
    msg.Seek(saved_read_pos);
    return false;
  }

  bool msg_processed = false;
  switch (msg_type) {
    case MessageType::NotifyCSEncryptedSessionKey:
      IoCompletion_ProcessMessage_NotifyCSEncryptedSessionKey(msg, lc);
      msg_processed = true;
      break;

    case MessageType::NotifyServerConnectionRequestData:
      IoCompletion_ProcessMessage_NotifyServerConnectionRequestData(msg, lc);
      msg_processed = true;
      break;

    case MessageType::RequestServerTimeAndKeepAlive:
      IoCompletion_ProcessMessage_RequestServerTimeAndKeepAlive(msg, lc);
      msg_processed = true;
      break;

    case MessageType::NotifyCSConnectionPeerSuccess:
      IoCompletion_ProcessMessage_NotifyCSConnectionPeerSuccess(msg, lc);
      msg_processed = true;
      break;

    case MessageType::NotifyCSP2PDisconnected:
      IoCompletion_ProcessMessage_NotifyCSP2PDisconnected(msg, lc);
      msg_processed = true;
      break;

    case MessageType::RPC:
      IoCompletion_ProcessMessage_RPC(received_msg, msg_processed, lc);
      break;

    case MessageType::FreeformMessage:
      IoCompletion_ProcessMessage_FreeformMessage(received_msg, msg_processed,
                                                  lc);
      break;

    case MessageType::Encrypted_Reliable:
    case MessageType::Encrypted_Unreliable: {
      ReceivedMessage decrypted_msg;
      if (DecryptMessage(msg_type, received_msg,
                         decrypted_msg.unsafe_message)) {
        decrypted_msg.relayed = received_msg.relayed;
        decrypted_msg.remote_addr_udp_only = received_msg.remote_addr_udp_only;
        decrypted_msg.remote_id = received_msg.remote_id;

        msg_processed |=
            IoCompletion_ProcessMessage_EngineLayer(decrypted_msg, lc);
      }
    } break;

    case MessageType::Compressed: {
      ReceivedMessage decompressed_msg;
      if (DecompressMessage(received_msg, decompressed_msg.unsafe_message)) {
        decompressed_msg.relayed = received_msg.relayed;
        decompressed_msg.remote_addr_udp_only =
            received_msg.remote_addr_udp_only;
        decompressed_msg.remote_id = received_msg.remote_id;

        msg_processed |=
            IoCompletion_ProcessMessage_EngineLayer(decompressed_msg, lc);
      }
      break;
    }

    default:
      break;
  }

  // 만약 잘못된 메시지가 도착한 것이면 이미 FunNet 계층에서 처리한 것으로
  // 간주하고 메시지를 폐기한다. 그리고 예외 발생 이벤트를 던진다. 단, C++
  // 예외를 발생시키지 말자. 디버깅시 혼란도 생기며 fail over 처리에도
  // 애매해진다.
  if (msg_processed &&
      received_msg.unsafe_message.GetLength() !=
          received_msg.unsafe_message.Tell() &&
      msg_type != MessageType::Encrypted_Reliable &&
      msg_type !=
          MessageType::Encrypted_Unreliable) {  // 암호화된 메시지는 별도
                                                // 버퍼에서 복호화된 후
                                                // 처리되므로
    //@todo 이게 맞는건지??
    msg_processed = true;

    // 에러시에 마지막 메세지를 저장한다.
    EnqueueError(ResultInfo::From(ResultCode::InvalidPacketFormat,
                                  received_msg.remote_id, __FUNCTION__,
                                  msg.ToReadableBytesCopy()));  // copy
  }

  // AssureMessageReadOkToEnd(msg_processed, msg_type, received_msg, lc);
  if (!msg_processed) {
    msg.Seek(saved_read_pos);
    return false;
  }

  return true;
}

void LanServerImpl::IoCompletion_ProcessMessage_NotifyCSEncryptedSessionKey(
    MessageIn& msg, LanClient_S* lc) {
  lc->AssertIsZeroUseCount();
  AssertIsNotLockedByCurrentThread();
  lc->LockMain_AssertIsNotLockedByCurrentThread();

  // AES/RC4 key를 얻는다.
  ByteArray encrypted_aes_key_blob;
  ByteArray encrypted_rc4_key_blob;
  if (!lf::Reads(msg, encrypted_aes_key_blob, encrypted_rc4_key_blob)) {
    return;
  }

  // 암호화된 세션키를 복호화해서 클라이언트 세션키에 넣는다.
  ByteArray out_random_block;

  // self_xchg_key_ 는 서버 시작시에 한번만 만드므로 락을 안걸어도 괜찮을것
  // 같습니다.
  if (SharedPtr<ResultInfo> error = CryptoRSA::DecryptSessionKeyByPrivateKey(
          out_random_block, encrypted_aes_key_blob, self_xchg_key_)) {
    return;  //@todo Logging을 해야하나?
  }

  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  // 미인증 클라인지 확인.
  // 해킹된 클라에서 이게 반복적으로 오는 것일 수 있다. 그런 경우 무시하도록
  // 하자.
  //@todo 인증 대기중인 놈한테 패킷이 온거란말이지..?
  if (!candidate_lan_clients_.FindKey(lc)) {
    return;
  }

  // AES 키 세팅 / RC4 키 세팅
  ByteArray out_rc4_random_block;
  if (!CryptoAES::ExpandFrom(
          lc->session_key_.aes_key, (const uint8*)out_random_block.ConstData(),
          settings_.strong_encrypted_message_key_length / 8) ||
      !CryptoAES::Decrypt(lc->session_key_.aes_key, encrypted_rc4_key_blob,
                          out_rc4_random_block) ||
      !CryptoRC4::ExpandFrom(lc->session_key_.rc4_key,
                             (const uint8*)out_rc4_random_block.ConstData(),
                             settings_.weak_encrypted_message_key_length / 8)) {
    callbacks_->OnException(HostId_None,
                            Exception("Create session-key failed"));

    MessageOut msg_to_send;
    lf::Write(msg_to_send, MessageType::NotifyServerDeniedConnection);
    lf::Write(msg_to_send, ByteArray());
    {
      CScopedLock2 lc_tcp_send_queue_guard(
          lc->to_client_tcp_->GetSendQueueMutex());
      lc->to_client_tcp_->SendWhenReady(SendFragRefs(msg_to_send),
                                        TcpSendOption());
    }
    return;
  }

  lc->session_key_received_ = true;

  // unlock를 하지 않아도 되지만...기왕이면 하자.
  main_guard.Unlock();

  MessageOut msg_to_send;
  lf::Write(msg_to_send, MessageType::NotifyCSSessionKeySuccess);
  {
    CScopedLock2 lc_tcp_send_queue_guard(
        lc->to_client_tcp_->GetSendQueueMutex());
    lc->to_client_tcp_->SendWhenReady(SendFragRefs(msg_to_send),
                                      TcpSendOption());
  }
}

void LanServerImpl::
    IoCompletion_ProcessMessage_NotifyServerConnectionRequestData(
        MessageIn& msg, LanClient_S* lc) {
  AssertIsNotLockedByCurrentThread();
  lc->AssertIsZeroUseCount();
  lc->AssertIsNotLockedByCurrentThread();

  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  // 인증전 클라 여부 체크
  if (!candidate_lan_clients_.FindKey(lc)) {
    NotifyProtocolVersionMismatch(lc);
    return;
  }

  ByteArray msg_user_data;
  Uuid msg_protocol_version;
  uint32 msg_internal_version;
  InetAddress msg_external_addr;

  if (!lf::Reads(msg, msg_user_data, msg_protocol_version, msg_internal_version,
                 msg_external_addr)) {
    NotifyProtocolVersionMismatch(lc);
    return;
  }

  // Protocol version match and assign received session key
  if (msg_protocol_version != protocol_version_ ||
      msg_internal_version != internal_version_) {
    NotifyProtocolVersionMismatch(lc);
    return;
  }

  // 암호화 키가 교환이 완료됐나 체크한다.
  if (!lc->session_key_.KeyExists()) {
    MessageOut msg_to_send;
    lf::Write(msg_to_send, MessageType::NotifyServerDeniedConnection);
    lf::Write(msg_to_send, ByteArray());

    // 전송(SendQueue에 추가)
    CScopedLock2 lc_tcp_send_queue_guard(
        lc->to_client_tcp_->GetSendQueueMutex());
    lc->to_client_tcp_->SendWhenReady(SendFragRefs(msg_to_send),
                                      TcpSendOption());

    return;
  }

  // listensocket는 getsockname를 해도 binaryaddress가 0이 나오기 때문에
  //서버에서 인식한 주소를 넣는다.

  // TODO 호스트만 변경하는 기능을 CIPEndPoint에 넣어 주어야함.
  // msg_external_addr.BinaryAddress =
  // lc->to_client_tcp_->remote_addr_.BinaryAddress; //포트 번호는???
  msg_external_addr = InetAddress(
      lc->to_client_tcp_->remote_addr_.GetHost(),
      msg_external_addr.GetPort());  // TODO SetHost 함수를 하나 만들어야할듯...

  // EndPoint가 유효한지 체크한다.
  if (!msg_external_addr.IsUnicast()) {
    MessageOut msg_to_send;
    lf::Write(msg_to_send, MessageType::NotifyServerDeniedConnection);
    lf::Write(msg_to_send, ByteArray());

    CScopedLock2 lc_tcp_send_queue_guard(
        lc->to_client_tcp_->GetSendQueueMutex());
    lc->to_client_tcp_->SendWhenReady(SendFragRefs(msg_to_send),
                                      TcpSendOption());

    return;
  }

  //이미 유효한 addr이 있는지 체크할 필요가 있을까?
  // if (lc->listen_addr_.IsUnicast()) {
  //
  //}

  lc->external_addr = msg_external_addr;

  // 접속 허가를 해도 되는지 유저에게 묻는다.
  // 이 스레드는 이미 main_mutex_ locked이므로 여기서 하지 말고 스레드 풀에서
  // 콜백받은 후 후반전으로 ㄱㄱ

  if (callbacks_) {
    EnqueueClientJoinApproveDetermine(lc->to_client_tcp_->remote_addr_,
                                      msg_user_data);
  } else {
    // 무조건 성공 처리
    ProcessOnClientJoinApproved(lc, ByteArray());
  }
}

void LanServerImpl::IoCompletion_ProcessMessage_RequestServerTimeAndKeepAlive(
    MessageIn& msg, LanClient_S* lc) {
  // Read client_local_time, MesauredLasLag
  double client_local_time;
  double measured_last_lag;
  if (!lf::Reads(msg, client_local_time, measured_last_lag)) {
    return;
  }

  lc->AssertIsNotLockedByCurrentThread();
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  // tcp recvcompletioncase에서 하지만 혹여나.
  //가장 마지막에 reliable ping을 받은 시간을 체크해둔다.
  lc->last_tcp_stream_recv_time_ = GetAbsoluteTime();

  if (intra_logger_) {
    const String text = String::Format(
        "RequestServerTimeAndKeepAlive %d host %lf last_tcp_stream_recv_time_",
        (int32)lc->host_id_, lc->last_tcp_stream_recv_time_);
    intra_logger_->WriteLine(LogCategory::System, *text);
  }

  // 측정된 랙도 저장해둔다.
  lc->last_ping_ = measured_last_lag;
  lc->recent_ping_ = MathBase::Lerp(lc->recent_ping_, measured_last_lag, 0.5);

  // 에코를 reliable하게 보낸다.
  MessageOut msg_to_send;
  lf::Write(msg_to_send, MessageType::ReplyServerTime);
  lf::Write(msg_to_send, client_local_time);
  lf::Write(msg_to_send, GetAbsoluteTime());
  {
    CScopedLock2 lc_tcp_send_queue_guard(
        lc->to_client_tcp_->GetSendQueueMutex());
    lc->to_client_tcp_->SendWhenReady(SendFragRefs(msg_to_send),
                                      TcpSendOption());
  }
}

void LanServerImpl::IoCompletion_ProcessMessage_NotifyCSConnectionPeerSuccess(
    MessageIn& msg, LanClient_S* lc) {
  AssertIsNotLockedByCurrentThread();
  lc->AssertIsZeroUseCount();

  HostId peer_id;
  if (!lf::Read(msg, peer_id)) {
    // error
    return;
  }

  lc->AssertIsNotLockedByCurrentThread();
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  // P2P 연결 Pair를 찾아 등록되어있는 놈들인지 확인한다.
  if (auto p2p_state =
          lan_p2p_connection_pair_list_.GetPair(peer_id, lc->host_id_)) {
    // 연결이 성사되었다.
    p2p_state->p2p_connect_state =
        LanP2PConnectionState::P2PConnectState::P2PConnected;

    // 연결 성사 이벤트를 알린다.
    EnqueueP2PConnectionEstablishedEvent(lc->host_id_, peer_id);

    HostIdArray groups1, groups2, groups3;
    // peer와 LanClient가 소속된 P2P Group을 찾는다.
    GetJoinedP2PGroups(peer_id, groups1);
    GetJoinedP2PGroups(lc->host_id_, groups2);

    // 두 피어가 소속된 그룹을 검색하여 둘다일치하는 그룹을 찾아 arr3에 옮긴다.
    for (int32 i = 0; i < groups1.Count(); ++i) {
      for (int32 j = 0; j < groups2.Count(); ++j) {
        if (groups1[i] == groups2[j]) {
          groups3.Add(groups1[i]);
        }
      }
    }

    // group Connection상태를 업데이트한다.
    for (int32 i = 0; i < groups3.Count(); ++i) {
      UpdateGroupP2PConnection(groups3[i]);
    }
  }
}

void LanServerImpl::IoCompletion_ProcessMessage_NotifyCSP2PDisconnected(
    MessageIn& msg, LanClient_S* lc) {
  AssertIsNotLockedByCurrentThread();

  const int32 saved_read_pos = msg.Tell();

  ResultCode result_code;
  HostId peer_id;

  if (!lf::Reads(msg, result_code, peer_id)) {
    msg.Seek(saved_read_pos);
    return;
  }

  lc->AssertIsNotLockedByCurrentThread();
  lc->AssertIsZeroUseCount();

  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  // P2P 상태변경
  if (auto p2p_state =
          lan_p2p_connection_pair_list_.GetPair(lc->host_id_, peer_id)) {
    p2p_state->p2p_connect_state = LanP2PConnectionState::P2PConnectState::None;
  }

  // P2P Tcp Connection이 실패하였으므로 peer가 속한 모든 p2pgroup에서 쫏아낸다.
  // FUN_TRACE("Notify P2P Disconnected! peer_id : %d\n", (int)peer_id);

  HostIdArray groups;
  HostId leave_id = HostId_None;
  if (peer_id > lc->host_id_) {  //무조건 HostId가 큰것을 내보낸다.
    leave_id = peer_id;
  } else {
    leave_id = lc->host_id_;
  }

  GetJoinedP2PGroups(leave_id, groups);

  for (int32 group_index = 0; group_index < groups.Count(); ++group_index) {
    LeaveP2PGroup(leave_id, groups[group_index]);
  }

  // 이벤트 노티
  EnqueueP2PDisconnectEvent(lc->host_id_, peer_id, (ResultCode)result_code);
}

void LanServerImpl::IoCompletion_ProcessMessage_RPC(
    ReceivedMessage& received_msg, bool msg_processed, LanClient_S* lc) {
  // FunNet layer의 RPC이면 아래 구문에서 true가 리턴되고 결국 user thread로
  // 넘어가지 않는다. 따라서 아래처럼 하면 된다.
  {
    auto& payload = received_msg.unsafe_message;

    AssertIsNotLockedByCurrentThread();
    lc->AssertIsZeroUseCount();
    lc->AssertIsNotLockedByCurrentThread();

    CScopedLock2 main_guard(main_mutex_);
    CheckCriticalSectionDeadLock(__FUNCTION__);

    const int32 rpc_header_pos =
        payload.Tell();  // 이 값은 MessageType::RPC 다음의 offset이다.

    // 내부 RPC Stub에 먼저 처리 기회를 주고, 처리되지 않았다면 유저 RPC Stub로
    // 전달하도록 함.
    msg_processed |=
        c2s_stub_.ProcessReceivedMessage(received_msg, lc->host_tag_);

    if (!msg_processed) {
      // 유저 스레드에서 RPC를 처리하도록 enque한다.
      payload.Seek(rpc_header_pos);

      ReceivedMessage received_msg2;
      received_msg2.unsafe_message = payload;  // share
      received_msg2.relayed = received_msg.relayed;
      received_msg2.remote_addr_udp_only = received_msg.remote_addr_udp_only;
      received_msg2.remote_id = received_msg.remote_id;

      UserTaskQueue_Add(lc, received_msg2, FinalUserWorkItemType::RPC);
    }
  }
}

void LanServerImpl::IoCompletion_ProcessMessage_FreeformMessage(
    ReceivedMessage& received_msg, bool msg_processed, LanClient_S* lc) {
  AssertIsNotLockedByCurrentThread();
  lc->AssertIsZeroUseCount();
  lc->AssertIsNotLockedByCurrentThread();

  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  // FunNet layer의 RPC이면 아래 구문에서 true가 리턴되고 결국 user thread로
  // 넘어가지 않는다. 따라서 아래처럼 하면 된다.
  // block 삭제 금지.!!!
  // TODO 딱히 블럭을 유지해야할 이유가 없어보이는데??
  {
    auto& payload = received_msg.unsafe_message;

    ReceivedMessage received_msg2;
    received_msg2.unsafe_message = payload;  // share
    received_msg2.relayed = received_msg.relayed;
    received_msg2.remote_addr_udp_only = received_msg.remote_addr_udp_only;
    received_msg2.remote_id = received_msg.remote_id;

    UserTaskQueue_Add(lc, received_msg2,
                      FinalUserWorkItemType::FreeformMessage);
  }
}

void LanServerImpl::UserTaskQueue_Add(LanClient_S* lc,
                                      ReceivedMessage& received_msg,
                                      FinalUserWorkItemType type) {
  AssertIsLockedByCurrentThread();
  lc->AssertIsNotLockedByCurrentThread();

  lc->final_user_work_queue_.Enqueue(
      FinalUserWorkItem_S(received_msg.unsafe_message, type));
  user_task_queue_.AddTaskSubject(lc);
}

void LanServerImpl::CatchThreadExceptionAndPurgeClient(LanClient_S* lc,
                                                       const char* where,
                                                       const char* reason) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (lc) {
    if (callbacks_) {
      const String text = String::Format(
          "LanServerImpl.CatchThreadExceptionAndPurgeClient: where: %s, "
          "reason: %s, client_id: %d",
          where, reason, (int32)lc->host_id_);
      EnqueueError(
          ResultInfo::From(ResultCode::Unexpected, lc->host_id_, text));
    }

    IssueDisposeLanClient(lc, ResultCode::Unexpected,
                          ResultCode::TCPConnectFailure, ByteArray(),
                          __FUNCTION__, SocketErrorCode::Ok);
  } else {
    if (callbacks_) {
      const String text = String::Format(
          "%s에서 %s 오류가 발생했으나 클라이언트를 식별 불가.", where, reason);
      EnqueueError(ResultInfo::From(ResultCode::Unexpected, HostId_None, text));
    }
  }
}

void LanServerImpl::NotifyProtocolVersionMismatch(LanClient_S* lc) {
  MessageOut msg_to_send;
  lf::Write(msg_to_send, MessageType::NotifyProtocolVersionMismatch);
  {
    CScopedLock2 lc_tcp_send_queue_guard(
        lc->to_client_tcp_->GetSendQueueMutex());
    lc->to_client_tcp_->SendWhenReady(SendFragRefs(msg_to_send),
                                      TcpSendOption());
  }
}

void LanServerImpl::PostHeartbeatIssue() {
  net_thread_pool_->PostCompletionStatus(this,
                                         (uintptr_t)IocpCustomValue::Heartbeat);
}

//중첩호출안됨.
void LanServerImpl::Heartbeat() {
  const double elapsed_sec = clock_.elapsed_sec();
  AbsoluteTime_USE_GetAbsoluteTime = clock_.AbsoluteSeconds();

  Heartbeat_One(elapsed_sec);
}

void LanServerImpl::Heartbeat_One(double elapsed_sec) {
  // TODO TickableScheduler로 처리하자.
  if (pure_too_old_unmature_client_alarm_.TakeElapsedTime(elapsed_sec)) {
    PurgeTooOldUnmatureClient();
  }

  if (purge_too_old_add_member_ack_item_alarm_.TakeElapsedTime(elapsed_sec)) {
    PurgeTooOldAddMemberAckItem();
  }

  if (dispose_issued_remote_clients_alarm_.TakeElapsedTime(elapsed_sec)) {
    DisposeIssuedLanClients();
  }

  if (remove_too_old_tcp_send_packet_queue_alarm_.TakeElapsedTime(
          elapsed_sec)) {
    Tcp_LongTick();
  }

  if (disconnect_remote_client_on_timeout_alarm_.TakeElapsedTime(elapsed_sec)) {
    // heartbeat_perClient에서 같이 처리. 그게 더 경제적인듯.
    // owner_->DisconnectLanClientOnTimeout();
    Heartbeat_PerClient();
  }
}

//중첩호출안됨.
void LanServerImpl::EveryRemote_IssueSendOnNeed(Array<IHostObject*>& pool) {
  //한 스레드만 작업하는것을 개런티
  CScopedLock2 tcp_issue_send_guard(tcp_issue_queue_mutex_);

  int32 collected_parallel_count = 0;
  int32 total_parallel_count = tcp_issue_send_ready_remote_clients_.Count();
  pool.Resize(total_parallel_count);

  auto parallel_queue = pool.MutableData();
  while (true) {
    if (auto lc = tcp_issue_send_ready_remote_clients_.Front()) {
      lc->UnlinkSelf();
      lc->IncreaseUseCount();
      parallel_queue[collected_parallel_count++] = lc;
    } else {
      break;
    }
  }

  //수집이 덜될수도 있으려나??
  //아래 코드를 보니 실질적으로 불필요한 코드인듯... 나중에 좀더 살펴보고
  //삭제하도록 하자.
  for (int32 i = collected_parallel_count; i < total_parallel_count; ++i) {
    parallel_queue[i] = nullptr;
  }

  tcp_guard.Unlock();

  const double absolute_time = GetAbsoluteTime();

  while (total_parallel_count > 0) {
#if USE_PARALLEL_FOR
#if (_MSC_VER >= 1400)
#pragma omp parallel for
#endif
#endif
    for (int32 parallel_index = 0; parallel_index < total_parallel_count;
         ++parallel_index) {
      auto object = parallel_queue[parallel_index];

      CScopedLock2 lc_guard(object->GetMutex(), false);

      if (parallel_index != 0) {
        if (const bool lock_ok = lc_guard.TryLock()) {
          const SocketErrorCode socket_error = object->IssueSend(absolute_time);
          lc_guard.Unlock();

          if (socket_error != SocketErrorCode::Ok) {
            CScopedLock2 main_guard(main_mutex_);
            CheckCriticalSectionDeadLock(__FUNCTION__);
            object->OnIssueSendFail(__FUNCTION__, socket_error);
          }

          object->Decrease();
          parallel_queue[parallel_index] =
              parallel_queue[--total_parallel_count];
        }
      } else {
        lc_guard.Lock();
        const SocketErrorCode socket_error = object->IssueSend(absolute_time);
        lc_guard.Unlock();

        if (socket_error != SocketErrorCode::Ok) {
          CScopedLock2 main_guard(main_mutex_);
          CheckCriticalSectionDeadLock(__FUNCTION__);
          object->OnIssueSendFail(__FUNCTION__, socket_error);
        }

        object->Decrease();
        parallel_queue[parallel_index] = parallel_queue[--total_parallel_count];
      }
    }
  }
}

void LanServerImpl::PostEveryRemote_IssueSend() {
  net_thread_pool_->PostCompletionStatus(
      this, (uintptr_t)IocpCustomValue::SendEnqueued);
}

void LanServerImpl::OnIoCompletion(Array<IHostObject*>& send_issued_pool,
                                   ReceivedMessageList& msg_list,
                                   CompletionStatus& completion) {
  fun_check(completion.type == CompletionType::ReferCustomValue);

  switch ((IocpCustomValue)completion.custom_value) {
    case IocpCustomValue::Heartbeat:
      Heartbeat();
      break;

    case IocpCustomValue::SendEnqueued:
      EveryRemote_IssueSendOnNeed(send_issued_pool);
      break;

    case IocpCustomValue::OnTick:
      OnTick();
      break;

    case IocpCustomValue::DoUserTask:
      DoUserTask();
      break;

    case IocpCustomValue::End:
      EndCompletion();
      break;

    default:
      fun_unexpected();
      break;
  }
}

void LanServerImpl::PostOnTick() {
  fun_check_ptr(user_thread_pool_);
  user_thread_pool_->PostCompletionStatus(this,
                                          (UINT_PTR)IocpCustomValue::OnTick);
}

// 중첩호출 안됨.
void LanServerImpl::OnTick() {
  if (callbacks_ /*&& !owner_->tear_down_*/) {
    callbacks_->OnTick(timer_callback_context_);
  }
}

void LanServerImpl::PostUserTask() {
  fun_check(user_thread_pool_);
  user_thread_pool_->PostCompletionStatus(
      this, (UINT_PTR)IocpCustomValue::DoUserTask);
}

void LanServerImpl::DoUserTask() {
  UserWorkerThreadCallbackContext context;
  FinalUserWorkItem item;

  bool running_state = false;
  do {
    void* host_tag = nullptr;
    {
      CScopedLock2 main_guard(main_mutex_);
      CheckCriticalSectionDeadLock(__FUNCTION__);

      running_state = user_task_queue_.PopAnyTaskNotRunningAndMarkAsRunning(
          item, &host_tag);
    }

    if (running_state) {
      if (callbacks_) {
        callbacks_->OnUserWorkerThreadCallbackBegin(&context);
      }

      switch (item.type) {
        case FinalUserWorkItemType::RPC:
          UserWork_FinalReceiveRPC(item, host_tag);
          break;

        case FinalUserWorkItemType::FreeformMessage:
          UserWork_FinalReceiveFreeformMessage(item, host_tag);
          break;

        case FinalUserWorkItemType::UserTask:
          UserWork_FinalUserTask(item, host_tag);
          break;

        default:
          UserWork_LocalEvent(item);
          break;
      }

      if (callbacks_ /*&& !owner_->tear_down_*/) {
        callbacks_->OnUserWorkerThreadCallbackEnd(&context);
      }
    }
  } while (running_state);
}

void LanServerImpl::UserWork_FinalReceiveRPC(FinalUserWorkItem& uwi,
                                             void* host_tag) {
  AssertIsNotLockedByCurrentThread();

  auto& content = uwi.unsafe_message.unsafe_message;
  const int32 saved_read_pos = content.Tell();

  if (saved_read_pos != 0) {
    EnqueueHackSuspectEvent(nullptr, __FUNCTION__,
                            HackType::PacketRig);  // bug or hacked
  }

  bool processed = false;
  RpcId rpc_id = RpcId_None;

  if (lf::Read(content, rpc_id)) {
    // 각 stub에 대한 처리를 수행한다.
    const int32 stub_count = stubs_nolock_.Count();
    for (int32 stub_index = 0; stub_index < stub_count; ++stub_index) {
      auto stub = stubs_nolock_[stub_index];

      content.Seek(saved_read_pos);

      try {
        // if (!owner_->tear_down_) {
        processed |= stub->ProcessReceivedMessage(uwi.unsafe_message, host_tag);
        //}
      } catch (Exception& e) {
        if (callbacks_ /*&& !owner_->tear_down_*/) {
          callbacks_->OnException(uwi.unsafe_message.remote_id, e);
        }
      } catch (std::exception& e) {
        if (callbacks_ /*&& !owner_->tear_down_*/) {
          callbacks_->OnException(uwi.unsafe_message.remote_id, Exception(e));
        }
      }  // catch (_com_error& e) {
      //  if (callbacks_ /*&& !owner_->tear_down_*/) {
      //    callbacks_->OnException(uwi.unsafe_message.remote_id, Exception(e));
      //  }
      //} catch (void* e) {
      //  if (callbacks_/*&& !owner_->tear_down_*/) {
      //    callbacks_->OnException(uwi.unsafe_message.remote_id, Exception(e));
      //  }
      //}
    }

    if (!processed) {
      content.Seek(saved_read_pos);

      if (callbacks_ /*&& !owner_->tear_down_*/) {
        callbacks_->OnNoRpcProcessed(rpc_id);
      }
    }

    user_task_queue_.SetTaskRunningFlagByHostId(uwi.unsafe_message.remote_id,
                                                false);
  }
}

void LanServerImpl::UserWork_FinalReceiveFreeformMessage(FinalUserWorkItem& uwi,
                                                         void* host_tag) {
  AssertIsNotLockedByCurrentThread();

  auto& content = uwi.unsafe_message.unsafe_message;
  const int32 saved_read_pos = content.Tell();

  if (saved_read_pos != 0) {
    EnqueueHackSuspectEvent(nullptr, __FUNCTION__,
                            HackType::PacketRig);  // bug or hacked
  }

  if (callbacks_ /* && !owner_->tear_down_*/) {
    RpcHint rpc_hint;
    rpc_hint.relayed = uwi.unsafe_message.relayed;
    rpc_hint.host_tag = host_tag;

    try {
      int32 payload_length;
      if (!lf::Read(content, payload_length) ||
          payload_length != content.GetReadableLength()) {
        SharedPtr<ResultInfo> result_info(new ResultInfo);
        result_info->result_code = ResultCode::InvalidPacketFormat;
        result_info->comment = "Invalid payload size in user message.";
        EnqueueError(result_info);
      } else {
        callbacks_->OnReceiveFreeform(uwi.unsafe_message.remote_id, rpc_hint,
                                      content.ToReadableBytesCopy());  // copy
      }
    } catch (Exception& e) {
      if (callbacks_ /* && !owner_->tear_down_*/) {
        callbacks_->OnException(uwi.unsafe_message.remote_id, e);
      }
    } catch (std::exception& e) {
      if (callbacks_ /* && !owner_->tear_down_*/) {
        callbacks_->OnException(uwi.unsafe_message.remote_id, Exception(e));
      }
    }  // catch (_com_error& e) {
    //  if (callbacks_/* && !owner_->tear_down_*/) {
    //    callbacks_->OnException(uwi.unsafe_message.remote_id, Exception(e));
    //  }
    //} catch (void* e) {
    //  if (callbacks_/*&& !owner_->tear_down_*/) {
    //    callbacks_->OnException(uwi.unsafe_message.remote_id, Exception(e));
    //  }
    //}
  }

  user_task_queue_.SetTaskRunningFlagByHostId(uwi.unsafe_message.remote_id,
                                              false);
}

void LanServerImpl::UserWork_FinalUserTask(FinalUserWorkItem& uwi,
                                           void* host_tag) {
  AssertIsNotLockedByCurrentThread();

  if (callbacks_ /* && !owner_->tear_down_*/) {
    try {
      uwi.UserFunc();
    } catch (Exception& e) {
      if (callbacks_ /* && !owner_->tear_down_*/) {
        callbacks_->OnException(uwi.unsafe_message.remote_id, e);
      }
    } catch (std::exception& e) {
      if (callbacks_ /* && !owner_->tear_down_*/) {
        callbacks_->OnException(uwi.unsafe_message.remote_id, Exception(e));
      }
    }  // catch (_com_error& e) {
    //  if (callbacks_/* && !owner_->tear_down_*/) {
    //    callbacks_->OnException(uwi.unsafe_message.remote_id, Exception(e));
    //  }
    //} catch (void* e) {
    //  if (callbacks_/*&& !owner_->tear_down_*/) {
    //    callbacks_->OnException(uwi.unsafe_message.remote_id, Exception(e));
    //  }
    //}
  }

  user_task_queue_.SetTaskRunningFlagByHostId(uwi.unsafe_message.remote_id,
                                              false);
}

void LanServerImpl::UserWork_LocalEvent(FinalUserWorkItem& uwi) {
  ProcessOneLocalEvent(uwi.Event);

  user_task_queue_.SetTaskRunningFlagByHostId(uwi.unsafe_message.remote_id,
                                              false);
}

void LanServerImpl::OnThreadBegin() {
  if (callbacks_) {
    callbacks_->OnUserWorkerThreadBegin();
  }
}

void LanServerImpl::OnThreadEnd() {
  if (callbacks_) {
    callbacks_->OnUserWorkerThreadEnd();
  }
}

void LanServerImpl::EndCompletion() {
  if (net_thread_pool_->IsCurrentThread()) {
    net_thread_pool_->UnregisterReferer(this);
    net_thread_pool_unregisted_ = true;
  }

  if (user_thread_pool_->IsCurrentThread()) {
    user_thread_pool_->UnregisterReferer(this);
    user_thread_pool_unregisted_ = true;
  }
}

LanP2PConnectionStatePtr LanP2PPairList::GetPair(HostId a, HostId b) {
  // TODO 이건 왜 하는거지??
  if (a > b) {
    Swap(a, b);
  }

  return list.FindRef(RCPair(a, b));
}

void LanP2PPairList::RemovePairOfAnySide(LanClient_S* lc) {
  for (auto it = list.CreateIterator(); it; ++it) {
#if 1  // HostId가 미지정되면(그럴리는 없겠지만) 어쩔라구
    auto state = it->value;

    if (state->first_client == lc || state->second_client == lc) {
#ifdef TRACE_NEW_DISPOSE_ROUTINE
      // FUN_TRACE("%d-%d pair Remove...\n", (int)state->first_client->host_id_,
      // (int)state->second_client->host_id_);
#endif
      state->first_client->lan_p2p_connection_pairs_.Remove(state);
      state->second_client->lan_p2p_connection_pairs_.Remove(state);

      it.RemoveCurrent();
    }
#else
    const RCPair& pair = it->key;

    if (pair.first == lc->host_id_ || pair.second == lc->host_id_) {
      auto state = it->value;

      state->first_client->lan_p2p_connection_pairs_.Remove(state);
      state->second_client->lan_p2p_connection_pairs_.Remove(state);

      it.RemoveCurrent();
    }
#endif
  }
}

void LanP2PPairList::AddPair(LanClient_S* lc_a, LanClient_S* lc_b,
                             LanP2PConnectionStatePtr state) {
  HostId a_id = lc_a->host_id_;
  HostId b_id = lc_b->host_id_;

  // 서버, 잘못된 아이디, 서로 같은 아이디는 추가 불가능. P2P 직빵연결된 것들의
  // 인덱스니까.
  // fun_check(a != b);  loopback도 pair에 들어갈 수 있으므로 이건 체크하지
  // 말자.
  fun_check(a_id != HostId_Server);
  fun_check(a_id != HostId_None);
  fun_check(b_id != HostId_Server);
  fun_check(b_id != HostId_None);

  if (a_id > b_id) {
    Swap(a_id, b_id);
    Swap(lc_a, lc_b);
  }

  RCPair pair(a_id, b_id);

  state->first_client = lc_a;
  state->second_client = lc_b;

  list.Add(pair, state);
}

void LanP2PPairList::ReleasePair(LanServerImpl* owner, LanClient_S* lc_a,
                                 LanClient_S* lc_b) {
  // LanP2PConnectionStatePtr pair;
  // LCPairMap::iterator pair_it;
  // if (GetPair(lc_a->host_id_, lc_b->host_id_, pair, pair_it)) {
  //  --pair->dup_count;
  //
  //  if (pair->dup_count == 0) {
  //    pair->first_client->lan_p2p_connection_pairs_.Remove(pair);
  //    pair->second_client->lan_p2p_connection_pairs_.Remove(pair);
  //
  //    List.erase(pair_it); // 이게 여기 있어야 한다. 위에 있으면 안되고.
  //
  //    if (owner_->intra_logger_) {
  //      const String text = String::Format("[서버] Client %d-Client %d간의 P2P
  //      연결 쌍 제거", lc_a->host_id_, lc_b->host_id_);
  //      owner_->intra_logger_->WriteLine(LogCategory::LP2P, *text);
  //    }
  //  }
  //}

  RCPair pair_key(lc_a->host_id_, lc_b->host_id_);
  if (pair_key.first > pair_key.second) {
    Swap(pair_key.first, pair_key.second);
  }

  if (auto pair = list.FindRef(pair_key)) {
    --pair->dup_count;

    if (pair->dup_count == 0) {
      pair->first_client->lan_p2p_connection_pairs_.Remove(pair);
      pair->second_client->lan_p2p_connection_pairs_.Remove(pair);

      list.Remove(pair_key);

      if (owner_->intra_logger_) {
        const String text = String::Format(
            "Remove pair of connections between client %d and %d.",
            (int32)lc_a->host_id_, (int32)lc_b->host_id_);
        owner_->intra_logger_->WriteLine(LogCategory::LP2P, *text);
      }
    }
  }
}

void LanServerImpl::CandidateLanClients::Remove(LanClient_S* lc) {
  Map<InetAddress, LanClient_S*>::Remove(lc->to_client_tcp_->remote_addr_);
}

bool LanServerImpl::RunAsync(HostId task_owner_id, Function<void()> func) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  // TODO group에도 넣을 수 있다며...

  // 각 커넥션별로 큐가 있으므로, 해당 커넥션의 큐에 넣어준다면 순서는 지켜지게
  // 된다.
  if (auto lc = GetLanClientByHostId_NOLOCK(task_owner_id)) {
    lc->EnqueueUserTask(func);
    user_task_queue_.AddTaskSubject(lc);
    return true;
  } else {
    if (listener_thread_) {
      final_user_work_queue_.Enqueue(func);
      user_task_queue_.AddTaskSubject(this);
      return true;
    }
  }

  return false;
}

LanServer* LanServer::New() { return new LanServerImpl(); }

void LanP2PConnectionState::SetExternalAddr(HostId peer_id,
                                            const InetAddress& external_addr) {
  for (int32 i = 0; i < 2; ++i) {
    if (peers[i].id == peer_id) {
      peers[i].external_addr = external_addr;
      return;
    }
  }

  for (int32 i = 0; i < 2; ++i) {
    if (peers[i].id == HostId_None) {
      peers[i].external_addr = external_addr;
      peers[i].id = peer_id;
      return;
    }
  }
}

InetAddress LanP2PConnectionState::GetExternalAddr(HostId peer_id) {
  for (int32 i = 0; i < 2; ++i) {
    if (peers[i].id == peer_id) {
      return peers[i].external_addr;
    }
  }

  return InetAddress::None;
}

LanP2PConnectionState::LanP2PConnectionState(LanServerImpl* owner)
    : owner(owner),
      dup_count(0),
      recent_ping(0),
      p2p_connect_state(P2PConnectState::None),
      first_client(nullptr),
      second_client(nullptr) {}

LanP2PConnectionState::~LanP2PConnectionState() {}

}  // namespace net
}  // namespace fun
