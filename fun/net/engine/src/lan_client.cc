//@deprecated
//@note NetClient, LanClient 모두 Disconnect()호출하여 접속을 끊을시에는 "LeftFromServer" 콜백이 호출되지 않음.
//      호출되게 해줄까??

#include "CorePrivatePCH.h"
#include "fun/net/net.h"

#include "LanClient.h"

#include "LanRemotePeer.h"
#include "LeanDynamicCast.h"

#include "ReportError.h"

//TODO
//#include "Tools/Visualizer/viz_agent_.h"

#include "GeneratedRPCs/lan_LanS2C_stub.cc"
#include "GeneratedRPCs/lan_LanC2S_proxy.cc"
#include "GeneratedRPCs/lan_LanC2C_proxy.cc"
#include "GeneratedRPCs/lan_LanC2C_stub.cc"

namespace fun {
namespace net {

using lf = LiteFormat;

extern const char* NoServerConnectionErrorText;

IMPLEMENT_RPCSTUB_LanS2C_P2PGroup_MemberJoin(LanClientImpl::S2CStub) {
  CScopedLock2 main_guard(owner_->GetMutex());
  owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

  // P2P Group에 새로운 member가 들어온것을 update한다
  owner_->UpdateP2PGroup_MemberJoin(group_id,
                                    member_id,
                                    custom_field,
                                    event_id,
                                    p2p_aes_session_key,
                                    p2p_rc4_session_key,
                                    connection_tag);
  return true;
}

IMPLEMENT_RPCSTUB_LanS2C_P2PGroup_MemberJoin_Unencrypted(LanClientImpl::S2CStub) {
  CScopedLock2 main_guard(owner_->GetMutex());
  owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

  // P2P Group에 새로운 member가 들어온것을 update한다
  owner_->UpdateP2PGroup_MemberJoin(group_id,
                                    member_id,
                                    custom_field,
                                    event_id,
                                    ByteArray(),
                                    ByteArray(),
                                    connection_tag);
  return true;
}

IMPLEMENT_RPCSTUB_LanS2C_P2PGroup_MemberLeave(LanClientImpl::S2CStub) {
  CScopedLock2 main_guard(owner_->GetMutex());
  owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

  //if (owner_->bbIntraLoggingOn) {
  //  owner_->Log(LogCategory::LP2P,
  //      "[Client %d] P2PGroup_MemberLeave 도착: remote peer=%d, group=%d\n",
  //      owner_->local_host_id_,
  //      member_id,
  //      group_id);
  //}
  //TODO group / member 둘중 하나라도 무효하다면 의미없는걸텐데...??

  // 여기서는 candidate에도 들어가야함.
  auto member_lp = owner_->GetPeerByHostId_NOLOCK(member_id);
  if (member_lp == nullptr) {
    owner_->candidate_remote_peers_.TryGetValue(member_id, member_lp);
  }

  auto group = owner_->GetP2PGroupByHostId_INTERNAL(group_id);
  if (group) {
    // 자신인 경우라도 그룹 객체에서 제거는 해야 한다.
    group->members_.Remove(member_id);
  }

  if (member_lp) {
    // 해당 멤버와 관련된 큐를 정리한다.
    if (owner_->settings_.look_ahead_p2p_send_enabled) {
      owner_->RemoveLookaheadP2PSendQueueMap(member_lp);
    }

    member_lp->joined_p2p_groups_.Remove(group_id);

    owner_->RemoveRemotePeerIfNoGroupRelationDetected(member_lp);
  }

  if (member_id == owner_->local_host_id_) { // 자신인 경우
    owner_->p2p_groups_.Remove(group_id);
  }

  // P2PLeaveMember
  LocalEvent event(LocalEventType::P2PLeaveMember);
  event.member_id = member_id;
  event.remote_id = member_id;
  event.group_id = group_id;
  event.member_count = group.IsValid() ? group->members_.Count() : 0;

  if (member_lp) {
    member_lp->EnqueueLocalEvent(event);
  }
  else {
    owner_->EnqueueLocalEvent(event);
  }

  return true;
}

IMPLEMENT_RPCSTUB_LanS2C_ReliablePong(LanClientImpl::S2CStub) {
  CScopedLock2 main_guard(owner_->GetMutex());
  owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

  // 받은 시간을 키핑한다.
  //owner_->last_reliable_pong_received_time_ = owner_->clock_.AbsoluteSeconds();

  return true;
}

//@maxidea:
// 다른 remote-peer의 접속을 받을 수 있도록 Listening 개시함.
IMPLEMENT_RPCSTUB_LanS2C_P2PConnectStart(LanClientImpl::S2CStub) {
  CScopedLock2 main_guard(owner_->GetMutex());
  owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

  // 미인증 클라 여부 체크
  // 미인증 클라이언트이면, 거부해야하는거 아닌가???
  // 체크 방법이 잘못된듯 싶은데...
  // 미인증 목록에서 찾는게 아니라 인증목록에서 찾은 결과를 토대로 처리해야하는거 아닌지???
  auto lp = owner_->candidate_remote_peers_.FindRef(peer_id);
  if (!lp) {
    // 서버에게 없는 넘이라고 노티하자.
    //NotifyProtocolVersionMismatch(lp);
    return true;
  }

  // 어드레스 체크
  if (!external_addr.IsUnicast()) {
    // 서버에게 에러리턴.
    lp->listen_addr_from_server_ = InetAddress::None;
    return true;
  }

  lp->listen_addr_from_server_ = external_addr;

  // 소켓 생성하자.임시로 생성해보자...다른 방법이 있을듯도 하다.
  lp->tcp_transport_.socket_.Reset(owner_->server_socket_pool_->NewTcpSocket(owner_, owner_));

  // Nagle Algorithm 적용
  lp->tcp_transport_.SetEnableNagleAlgorithm(owner_->settings_.bEnableNagleAlgorithm);

  // Set lp as completion_context
  lp->tcp_transport_.socket_->SetCompletionContext((ICompletionContext*)lp);


  //@maxidea: Connect 하는건데 binding을 해주어야 하나??

  // 상대방에게 커넥션을 걸자. 커넥션 플래그는 커넥트 overlapped안의 issued를 사용하자.
  // 로컬 어드레스로 바인딩... NIC가 여러개일경우를 지칭한다.
  if (!lp->tcp_transport_.socket_->Bind(*owner_->local_nic_addr_, 0)) {
    // 서버에게도 노티하자.
    owner_->EnqueueError(ResultInfo::From(ResultCode::TCPConnectFailure, owner_->local_host_id_, "ConnectSocket bind failure."));
    return true;
  }
  //@maxidea: debugging
  //else {
  //  _tprintf("lp->tcp_transport_.socket_ : bound to %s\n", *lp->tcp_transport_.socket_->GetSockName().ToString());
  //}

  if (owner_->net_thread_pool_ == nullptr) {
    // 서버에게도 노티하자.
    owner_->EnqueueError(ResultInfo::From(ResultCode::TCPConnectFailure, owner_->local_host_id_, "net_thread_pool_ is NULL."));
    return true;
  }

  owner_->net_thread_pool_->AssociateSocket(lp->tcp_transport_.socket_.Get());

  // ConnextEx 하기 전에 IncreaseUseCount 완료 후 decrease
  lp->IncreaseUseCount();

  // Issue connect
  CScopedLock2 lp_tcp_guard(lp->tcp_transport_.GetMutex());
  const SocketErrorCode socket_error = lp->tcp_transport_.socket_->ConnectEx(lp->listen_addr_from_server_);
  fun_check(socket_error == SocketErrorCode::Ok);

  //@maxidea: debugging
  //_tprintf("[!] Connecting to remote-peer (%s)    socket_error=%s\n", *lp->listen_addr_from_server_.ToString(), SocketErrorCode::ToString(socket_error));
  return true;
}

/** P2P 연결 구성이 완료 되었음. */
IMPLEMENT_RPCSTUB_LanS2C_GroupP2PConnectionComplete(LanClientImpl::S2CStub) {
  CScopedLock2 main_guard(owner_->GetMutex());
  owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

  if (auto group = owner_->GetP2PGroupByHostId_INTERNAL(group_id)) {
    LocalEvent event(LocalEventType::GroupP2PEnabled);
    event.group_id = group_id;
    owner_->EnqueueLocalEvent(event);

    // 그룹내 통신이 가능한지의 여부를 넣을 필요가 있다면 여기서...
  }

  return true;
}

IMPLEMENT_RPCSTUB_LanS2C_ShutdownTcpAck(LanClientImpl::S2CStub)
{
  CScopedLock2 main_guard(owner_->GetMutex());
  owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

  // shutdown ack를 받으면 바로 종료 처리를 진행하도록 한다.
  if (owner_->listener_thread_ &&
      owner_->shutdown_issued_time_ > 0 &&
      owner_->graceful_disconnect_timeout_ != 0.0) {
    owner_->shutdown_issued_time_ = owner_->GetAbsoluteTime() - (owner_->graceful_disconnect_timeout_ * 2);
  }

  // 서버에게 shutdown요청에 대한 응답을 보냄.
  owner_->c2s_proxy_.ShutdownTcpHandshake(HostId_Server, GReliableSend_INTERNAL);
  return true;
}

IMPLEMENT_RPCSTUB_LanS2C_RequestAutoPrune(LanClientImpl::S2CStub)
{
  CScopedLock2 main_guard(owner_->GetMutex());
  owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

  if (owner_->intra_logger_) {
    owner_->intra_logger_->WriteLine(LogCategory::System, "RequestAutoPrune receive.");
  }

  // 서버와의 연결을 당장 끊는다.
  // Shutdown-shake 과정을 할 필요가 없다. 클라는 디스가 불특정 시간에 일어나는 셈이므로.
  if (owner_->GetState() <= ConnectionState::Connected) {
    // Disconnect
    owner_->EnqueueDisconnectionEvent(ResultCode::DisconnectFromRemote, ResultCode::TCPConnectFailure);
    owner_->SetState(ConnectionState::Disconnecting);
  }

  return true;
}

void LanClientImpl::RemoveRemotePeerIfNoGroupRelationDetected(LanRemotePeer_C* member_lp) {
  LockMain_AssertIsLockedByCurrentThread();

  // 모든 그룹을 뒤져서 local과 제거하려는 Remote가 모두 들어있는 P2P Group이 하나라도 존재하면
  // P2P 연결 해제를 하지 않는다.
  for (auto& pair : p2p_groups_) {
    auto group = pair.value;

    for (auto& member_pair : group->members_) {
      auto member = member_pair.value;

      if (member == member_lp) {
        return;
      }
    }
  }

  IssueDisposeRemotePeer( member_lp,
                          ResultCode::DisconnectFromRemote,
                          ResultCode::Ok,
                          ByteArray(),
                          __FUNCTION__,
                          SocketErrorCode::Ok);
}

LanClientImpl::LanClientImpl()
  : server_as_send_dest_(this),
    reliable_ping_alarm_(NetConfig::GetDefaultNoPingTimeoutTime()),
    purge_too_old_unmature_peer_alarm_(NetConfig::purge_too_old_joining_timeout_interval_sec),
    remove_tool_old_tcp_send_packet_queue_alarm_(NetConfig::udp_packet_board_long_interval_sec), //@temp
    dispose_issued_remote_peers_alarm_(NetConfig::dispose_issued_remote_clients_timeout_sec),
    disconnect_remote_peer_on_timeout_alarm_(NetConfig::cs_ping_interval_sec),
    accepted_peer_dispost_alarm_(NetConfig::dispose_issued_remote_clients_timeout_sec),
    remove_lookahead_message_alarm_(NetConfig::remove_lookahead_msg_timeout_interval_sec),
    user_task_queue_(this),
    state_(ConnectionState::Disconnected)
  //, tear_down_(false)
{
  to_server_tcp_ = nullptr;

  server_socket_pool_ = ServerSocketPool::GetSharedPtr();

  internal_version_ = NetConfig::InternalLanVersion;
  user_task_is_running_ = false;
  callbacks_ = nullptr;

  //intra_logging_on_ = false;

  AttachProxy(&c2c_proxy_);
  AttachProxy(&c2s_proxy_);
  AttachStub(&s2c_stub_);
  AttachStub(&c2c_stub_);

  c2c_proxy_.engine_specific_only_ = true;
  c2s_proxy_.engine_specific_only_ = true;
  s2c_stub_.engine_specific_only_ = true;
  c2c_stub_.engine_specific_only_ = true;

  c2c_stub_.owner_ = this;
  s2c_stub_.owner_ = this;

  total_tcp_recv_bytes_ = 0;
  total_tcp_send_bytes_ = 0;

  shutdown_issued_time_ = 0;
  graceful_disconnect_timeout_ = 0;

  stepped_absolute_time_ = 0.0;
  stepped_elapsed_time_ = 0.0;
}

LanClientImpl::~LanClientImpl() {
  LockMain_AssertIsNotLockedByCurrentThread();

  Disconnect();

  // RZ 내부에서도 쓰는 RPC까지 더 이상 참조되지 않음을 확인해야 하므로 여기서 시행해야 한다.
  CleanupEveryProxyAndStub(); // 꼭 이걸 호출해서 미리 청소해놔야 한다.

  // 아래 과정은 unsafe heap을 쓰는 것들을 mutex_ lock 상태에서 청소하지 않으면 괜한 assert fail이 발생하므로 필요
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  // CompletionPort와 ToServerTcp는 어차피 Disconnect내부에서 삭제되므로 여기서 따로 하지 않음.
  //completion_port_.Reset();
  p2p_groups_.Clear(); // 확인사살
  //to_server_tcp_.Reset(); // 이것들도 unsafe heap을 참조하는 것들을 보유하므로

  final_user_work_queue_.Clear();

  main_guard.Unlock(); // 이게 없으면 assert fail

  //TODO
  //viz_agent_.Reset();

  //networker_thread_.Reset();
  {
    CScopedLock2 PreFinalRecvLock(pre_final_recv_queue_mutex_);
    pre_final_recv_queue_.Clear();
  }

  to_server_tcp_.Reset();
}

bool LanClientImpl::Connect(LanConnectionArgs& args, SharedPtr<ResultInfo>& out_error) {
  CScopedLock2 connect_disconnect_phase_guard(connect_disconnect_phase_mutex_);

  LockMain_AssertIsNotLockedByCurrentThread();
  {
    // UserWorkerThread에서 Connect를 사용할수 없다.
    CScopedLock2 main_guard(mutex_);
    CheckCriticalSectionDeadLock(__FUNCTION__);

    const ConnectionState state = GetServerConnectionState();
    if (state != ConnectionState::Disconnected) {
      // already connected, retry is treat as failure
      const String text = String::Format("Already connected.  (current_state: %s)", *ToString(state));
      out_error = ResultInfo::From(ResultCode::AlreadyConnected, HostId_None, text);
      return false;
    }
  }

  if (user_thread_pool_ && user_thread_pool_->IsCurrentThread()) {
    // 이상하게도 userworkerthread내에서 try catch로 잡으면 파괴자가 호출되지 않는 현상이 생긴다.
    connect_disconnect_phase_guard.Unlock();

    throw Exception("Call Connect() in UserWorker.");
  }

  connect_count_.Increment();

  //connect가 userworkerthread에서 불릴일이 없을것이라 가정하고 코딩되었다.
  //DisconnectNotWait함수가 호출된후 connect를 하게 되면 문제가 생길수 있는경우가 있어 코딩되었다.
  listener_thread_.Reset();

  CScopedLock2 main_guard(mutex_); // for atomic oper
  CheckCriticalSectionDeadLock(__FUNCTION__);

  suppress_subsequenct_disconnection_events_ = false;

  //@todo
  //일단 임시로 Cleanup()함수를 직접 호출해주어 정상 동작하도록 하긴 했으나,
  //이렇게 하므로써, 바로 재접속을 할 수는 있게 되었지만,
  //큐에 대기중이었던 메시지들은 어떻게 되는지?
  //ConnectionState::Disconnecting단계에서, 처리가 완료되면 ConnectionState::Disconnected로 넘어가는것인지 확인할 필요가
  //있어보임.
  //이곳에서 호출하면 문제가 발생함.
  //ConnectionState::Disconnected로 바뀌는 시점들에서 처리하는게 바람직할듯함!
  //TcpListeningSocket을 닫아주고, TcpListenCompletionPort만 해제해주면 될려나?
  //현재로서는, Heartbeat()루틴과의 간섭이 발생하고 있음.
  //Cleanup();

  //@todo 이게 해제되는게 Cleanup()함수에 있는데, 명시적으로 Disconnect()하기 전에는
  //      Cleanup()함수가 호출되지 않는다. 자세히 살펴볼 필요가 있을듯 싶다.

  // 이전에 Listen이 끝나야 한다.

  if (tcp_listening_socket_) {
    out_error = ResultInfo::From(ResultCode::AlreadyConnected, HostId_None, "Already listening.");
    return false;
  }

  if (args.thread_count == 0) {
    args.thread_count = CPlatformMisc::NumberOfCoresIncludingHyperthreads();
    //_tprintf("thread_count : %d\n", args.thread_count);
  }

  if (args.thread_count < 0) {
    throw Exception("Invalid thread count");
  }

  const bool has_multiple_nics = NetUtil::LocalAddresses().Count() > 1;
  if (has_multiple_nics && args.local_nic_addr_.IsEmpty()) {
    //ShowWarning_NOLOCK(ResultInfo::From(ResultCode::Unexpected, HostId_None, "Server has multiple network devices, however, no device is specified for listening. Is it your intention?"));

    //TODO 이걸 보고해야할 오류라고 봐야하나??
    //경고정도로 끝내도 좋을듯 싶은데...
    ErrorReporter::Report(String::Format("LanClientImpl.Connect - No NIC binding though multiple NIC detected##Process=%s", CPlatformProcess::ExecutableName()));
  }

  // create IOCP & user worker IOCP
  tcp_accept_cp_.Reset(new CompletionPort(this, true, 1));

  clock_.Start();
  UpdateFixedSteppedTimeVars();

  local_nic_addr_ = args.local_nic_addr_;

  args.network_thread_count = MathBase::Clamp(args.network_thread_count, 0, CPlatformMisc::NumberOfCoresIncludingHyperthreads());
  if (args.network_thread_count == 0) {
    args.network_thread_count = CPlatformMisc::NumberOfCoresIncludingHyperthreads();
  }

  connection_params_.timer_callback_interval = args.timer_callback_interval;
  connection_params_.timer_callback_context = args.timer_callback_context;
  connection_params_.p2p_listening_port = args.p2p_listening_port;

  // tcplisten 초기화

  if (!CreateTcpListenSocketAndInit(out_error)) {
    // 앞서 시행했던 것들을 모두 롤백한다.

    fun_check(tcp_listening_socket_.IsValid() == false);
    clock_.Stop();
    tcp_accept_cp_.Reset();

    out_error = ResultInfo::From(ResultCode::ServerPortListenFailure, HostId_None, "Listening socket create failed.");
    return false;
  }

  const ConnectionState server_conn_state = GetServerConnectionState();
  if (server_conn_state != ConnectionState::Disconnected) {
    throw Exception("Wrong state(%s).  Disconnect() or GetServerConnectionState() may be required.", ToString(server_conn_state));
  }

  // 파라메터 정당성 체크
  //TODO CIPEndPoint에 넣어서 체크하자.
  //if (args.server_ip == "0.0.0.0" ||
  //  args.server_port == 0 ||
  //  args.server_port == 0xFFFF ||
  //  args.server_ip == "255.255.255.255")
  if (!InetAddress(args.server_ip, args.server_port).IsUnicast()) {
    // 이상하게도 userworkerthread내에서 try catch로 잡으면 파괴자가 호출되지 않는 현상이 생긴다.
    connect_disconnect_phase_guard.Unlock();
    main_guard.Unlock();

    throw Exception(ResultInfo::TypeToString(ResultCode::UnknownEndPoint));
  }

  // clean clear and naked! 서버 접속에 관련된 모든 값들을 초기화한다.
  // (disconnected state에서도 이게 안 비어있을 수 있다. 왜냐하면 서버에서 추방직전 쏜 RPC를
  // 클라가 모두 처리하려면 disconnected state에서도 미처리 항목을 유지해야 하기 때문이다.)
  reliable_ping_alarm_.Reset();

  final_user_work_queue_.Clear();

  last_tcp_stream_recv_time_ = GetAbsoluteTime();

  // reset it
  total_tcp_send_bytes_ = 0;
  total_tcp_recv_bytes_ = 0;

  //m_lastTickInvokedTime = GetAbsoluteTime();

  to_server_encrypt_count_ = 0;
  to_server_decrypt_count_ = 0;

  last_request_server_time_time_ = 0;
  request_server_time_count_ = 0;

  dx_server_time_diff_ = 0;
  server_tcp_recent_ping_ = 0;
  server_tcp_last_ping_ = 0;
  local_host_id_ = HostId_None;

  self_encrypt_count_ = 0;
  self_decrypt_count_ = 0;
  user_task_is_running_ = false;

  disconnecting_mode_heartbeat_count_ = 0;
  disconnecting_mode_start_time_ = 0;
  disconnecting_mode_warned_ = false;

  purge_too_old_unmature_peer_alarm_ = IntervalAlaram(NetConfig::purge_too_old_joining_timeout_interval_sec);
  remove_tool_old_tcp_send_packet_queue_alarm_ = IntervalAlaram(NetConfig::udp_packet_board_long_interval_sec);
  dispose_issued_remote_peers_alarm_ = IntervalAlaram(NetConfig::dispose_issued_remote_clients_timeout_sec);
  disconnect_remote_peer_on_timeout_alarm_ = IntervalAlaram(NetConfig::cs_ping_interval_sec);
  accepted_peer_dispost_alarm_ = IntervalAlaram(NetConfig::dispose_issued_remote_clients_timeout_sec);
  remove_lookahead_message_alarm_ = IntervalAlaram(NetConfig::remove_lookahead_msg_timeout_interval_sec);

  shutdown_issued_time_ = 0;
  graceful_disconnect_timeout_ = 0;

  //tear_down_ = false;

  connection_params_ = args;

  lookahead_p2p_send_queue_map_.Clear();

  // 빈 문자열 들어가면 localhost로 자동으로 채움
  connection_params_.server_ip = connection_params_.server_ip.Trim();
  if (connection_params_.server_ip.IsEmpty()) {
    connection_params_.server_ip = "localhost";
  }

  // threadpool
  args.network_thread_count = MathBase::Clamp(args.network_thread_count, 0, CPlatformMisc::NumberOfCoresIncludingHyperthreads());
  if (args.network_thread_count == 0) {
    args.network_thread_count = CPlatformMisc::NumberOfCoresIncludingHyperthreads(); //@todo 이게 좀 맞는 처리인지??
  }

  // threadpool 초기화.
  if (args.external_net_worker_thread_pool) {
    auto thread_pool_impl = dynamic_cast<ThreadPoolImpl*>(args.external_net_worker_thread_pool);
    if (thread_pool_impl == nullptr || !thread_pool_impl->IsActive()) {
      throw Exception("LanWorkerThreadPool not start.");
    }

    net_thread_pool_.Reset(thread_pool_impl);
    net_thread_external_use_ = true;
  } else {
    net_thread_pool_.Reset((thread_pool_impl*)ThreadPool2::New());
    net_thread_pool_->Start(args.network_thread_count);
    net_thread_external_use_ = false;
  }

  if (args.external_user_worker_thread_pool) {
    auto thread_pool_impl = dynamic_cast<ThreadPoolImpl*>(args.external_user_worker_thread_pool);
    if (thread_pool_impl == nullptr || !thread_pool_impl->IsActive()) {
      throw Exception("LanUserWorkerThreadpool not start.");
    }

    user_thread_pool_.Reset(thread_pool_impl);
    user_thread_external_use_ = true;
  } else {
    user_thread_pool_.Reset((ThreadPoolImpl*)ThreadPool2::New());
    user_thread_pool_->SetCallbacks(this); // 내장일 경우 eventsink는 this가 된다. (스레드풀 셋팅전에 해야함)
    user_thread_pool_->Start(args.thread_count);
    user_thread_external_use_ = false;
  }

  // 이 에러 처리가 필요 하다.
  if (!CreateTcpConnectSocketAndInit(connection_params_.server_ip, connection_params_.server_port, out_error)) {
    //@todo 정리 차원에서 할뿐..
    state_ = ConnectionState::Disconnected;

    clock_.Stop();

    if (!net_thread_external_use_) {
      net_thread_pool_.Reset();
    } else {
      net_thread_pool_.Detach();
    }

    if (!user_thread_external_use_) {
      user_thread_pool_.Reset();
    } else {
      user_thread_pool_.Detach();
    }

    CScopedLock2 main_guard(mutex_);

    tcp_accept_cp_.Reset();
    to_server_tcp_.Reset();

    out_error = ResultInfo::From(ResultCode::Unexpected, HostId_None, "Connect socket create failed.");
    return false;
  }

  // referrer등록.
  net_thread_pool_->RegistReferer(this);
  net_thread_pool_unregisted_ = false;
  user_thread_pool_->RegistReferer(this);
  user_thread_pool_unregisted_ = false;

  // TCP를 받아 처리하는 스레드 풀을 만든다.
  listener_thread_.Reset(new LanListener_C(this));
  listener_thread_->StartListening();

  timer_.Start();

  // 주의 : 모든 타이머는 초기화 이슈등을 피하기 위해서, 0.1초 이후부터 작동하기 시작함.
  // 구지 이렇게 해야할까 싶긴한데...

  // Tick timer는 필요시에만 구동함.
  if (connection_params_.timer_callback_interval > 0) {
    tick_timer_id_ = timer_.Schedule(
        Timespan::FromMilliseconds(100),
        Timespan::FromMilliseconds(connection_params_.timer_callback_interval),
        [&](const TimerTaskContext&) { PostOnTick(); },
        "LanServer.Tick");
  } else {
    tick_timer_id_ = 0;
  }

  // 매우 자주 호출됨. 매 5ms마다 호출됨.
  heartbeat_timer_id_ = timer_.Schedule(
      Timespan::FromMilliseconds(100),
      Timespan::FromMilliseconds(NetConfig::server_heartbeat_interval_msec),
      [&](const TimerTaskContext&) { PostHeartbeatIssue(); },
      "LanServer.Heartbeat");

  // 3ms ?
  // 좀 짧은 감이 없지 않아 있는데, 즉 0.003초안의 coalescing을 수행하겠다는 얘기임.
  // (0.003초 안의 전송은 모아서 하겠다는 거고, 대기 시간을 너무 길게 잡을 경우에는 반응성이 떨어질 수 있음.)
  issue_send_on_need_timer_id_ = timer_.Schedule(
      Timespan::FromMilliseconds(100),
      Timespan::FromMilliseconds(NetConfig::every_remote_issue_send_on_need_interval_sec * 1000),
      [&](const TimerTaskContext&) { PostEveryRemote_IssueSend(); },
      "LanServer.ConditionalIssueSend");

  //@todo 나중에는 이런 초딩짓이 아니라 모든 스레드의 시작이 준비되는 순간에 리턴하게 만들자.
  //CountdownLatch를 사용하면 좋을듯...
  CPlatformProcess::Sleep(0.1f);

  return true;
}

double LanClientImpl::GetAbsoluteTime() {
  if (stepped_absolute_time_ == 0.0) {
    stepped_absolute_time_ = clock_.AbsoluteSeconds();
  }

  return stepped_absolute_time_;
}

double LanClientImpl::GetElapsedTime() {
  if (stepped_elapsed_time_ == 0.0) {
    stepped_elapsed_time_ = clock_.ElapsedSeconds();
  }

  return stepped_elapsed_time_;
}

bool LanClientImpl::CreateTcpListenSocketAndInit(SharedPtr<ResultInfo>& out_error) {
  tcp_listening_socket_.Reset(new InternalSocket(SocketType::Tcp, this));
  tcp_listening_socket_->ignore_not_socket_error_ = true;

  // listening port를 사용자가 지정할수 있게 수정.
  if (!tcp_listening_socket_->Bind(*local_nic_addr_, connection_params_.p2p_listening_port)) {
    // TCP 리스닝 자체를 시작할 수 없는 상황이다. 이런 경우 서버는 제 기능을 할 수 없는 심각한 상황이다.
    out_error = ResultInfo::From(ResultCode::ServerPortListenFailure, local_host_id_, "");
    return false;
  }

  tcp_listening_socket_->Listen();

  tcp_listening_socket_->SetCompletionContext((ICompletionContext*)this);
  tcp_accept_cp_->AssociateSocket(tcp_listening_socket_.Get());

  return true;
}

bool LanClientImpl::CreateTcpConnectSocketAndInit(const String& ip, int32 port, SharedPtr<ResultInfo>& out_error)
{
  to_server_tcp_.Reset(new TcpTransport_C(this));

  // 로컬 어드레스로 바인딩...NIC가 여러개일경우를 지칭한다.
  if (!to_server_tcp_->socket_->Bind(*local_nic_addr_, 0)) {
    out_error = ResultInfo::From(ResultCode::TCPConnectFailure, local_host_id_, "ConnectSocket bind failure.");
    return false;
  }

  to_server_tcp_->socket_->ConnectEx(ip, port);

  state_ = ConnectionState::Connecting;
  return true;
}

bool LanClientImpl::Send_BroadcastLayer(const SendFragRefs& payload,
                                        const SendOption& send_opt,
                                        const HostId* sendto_list,
                                        int32 sendto_count)
{
  // lock을 이제 여기서 따로 건다.
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (!to_server_tcp_.IsValid() || local_host_id_ == HostId_None) { // 이미 서버와 연결 해제된 상태면 즐
    EnqueueError(ResultInfo::From(ResultCode::PermissionDenied, HostId_None, NoServerConnectionErrorText));
    return false;
  }

  // 수신 대상을 unGroup한다.
  ILanSendDestList_C send_dest_list;
  ConvertGroupToIndividualsAndUnion(sendto_count, sendto_list, send_dest_list);

  const int32 send_dest_list_count = send_dest_list.Count();
  Array<LanRemotePeer_C*,InlineAllocator<256>> p2p_send_list(send_dest_list_count, NoInit);

  LanRemotePeer_C* lp = nullptr;

  int32 p2p_send_count = 0;
  // for each send_to items BEGIN
  for (int32 send_dest_index = 0; send_dest_index < send_dest_list_count; ++send_dest_index) {
    auto send_dest = send_dest_list[send_dest_index];

    // if loopback
    if (send_dest == this && send_opt.bounce) {
      // 즉시 final user work로 넣어버리자. 루프백 메시지는 사용자가 발생하는 것 말고는 없는 것을 전제한다.
      //메시지 타입을 제거한 후 페이로드만 온전히 남겨준다음 넘겨줌.

      ByteArray PayloadBytes = payload.ToBytes(); // copy

      //EMssageType은 제거한 상태로 넘어가야함.
      MessageType msg_type;
      MessageIn payload_without_msg_type(PayloadBytes);
      lf::Read(payload_without_msg_type, msg_type);

      FinalUserWorkItemType final_user_work_item_type;
      switch (msg_type) {
      case MessageType::RPC: final_user_work_item_type = FinalUserWorkItemType::RPC; break;
      case MessageType::FreeformMessage: final_user_work_item_type = FinalUserWorkItemType::FreeformMessage; break;
      default: fun_check(0); break;
      }

      // Post(이건 별도의 함수로 만들어도 좋을듯 싶은데...)
      final_user_work_queue_.Enqueue(FinalUserWorkItem_S(payload_without_msg_type, final_user_work_item_type));
      user_task_queue_.AddTaskSubject(this);
    }

    // check if send_to is server
    else if (send_dest == &server_as_send_dest_) {
      Send_ToServer_Directly_Copy(send_opt.reliability, payload);
    }

    // 아직 P2P Connect 되진 않았으나 보내야하는경우 큐에 저장한다.
    // NOTE: truely lookahead는 아니지만 어쨌거나 서버에서 P2P member join 요청에 대한 반응을 한 적이 있다면 유효한 remote Peer로 처리한다.
    // P2P member join 요청이 안온 것에 대해서까지 lookahead하지는 않고 있다.
    else if ( settings_.look_ahead_p2p_send_enabled &&
              send_dest &&
              candidate_remote_peers_.Contains(send_dest->GetHostId())) {
      // SendFragRefs를 그대로 저장하면 안된다.
      // (메모리를 가지고있지 않으므로 ByteBufferPtr로 복사해 저장한다.)
      //ByteArray data_to_send;
      //data_to_send.UseInternalBuffer(); // *COPY*
      //payload.CopyTo(data_to_send);
      //AddLookaheadP2PSendQueueMap(send_dest->GetHostId(), data_to_send);
      AddLookaheadP2PSendQueueMap(send_dest->GetHostId(), payload.ToBytes()); // copy
#ifdef TRACE_LOOKAHEADSEND
      //FUN_TRACE("LanCli - Add LookaheadP2PQueMap. host_id : %d\n", (int)send_dest->GetHostId());
#endif
    }

    // P2P로 보내는 메시지인 경우
    else if (send_dest) {
      try {
        lp = dynamic_cast<LanRemotePeer_C*>(send_dest);
      } catch (std::bad_cast& e) {
        OutputDebugString(UTF8_TO_TCHAR(e.what()));
        int32* X = nullptr; *X = 1;
      }

      if (lp) {
        lp->IncreaseUseCount();
        p2p_send_list[p2p_send_count++] = lp;
      }
    }
  }

  // main unlock
  main_guard.Unlock();

  // 모아진 RP들에게 전부 send를 하자.
  for (int32 send_index = 0; send_index < p2p_send_count; ++send_index) {
    // RP가 있음을 항상 보장하므로 따로 검사를 하지않는다.
    auto lp = p2p_send_list[send_index];
    {
      CScopedLock2 lp_tcp_send_queue_guard(lp->tcp_transport_.GetSendQueueMutex());
      lp->tcp_transport_.SendWhenReady(payload, TcpSendOption());
    }

    lp->DecreaseUseCount();
  }

  return true;
}

void LanClientImpl::Send_ToServer_Directly_Copy(MessageReliability Reliability, const SendFragRefs& data_to_send)
{
  CScopedLock2 to_server_tcp_guard(to_server_tcp_mutex_);
  to_server_tcp_->SendWhenReady(data_to_send, TcpSendOption());
}

void LanClientImpl::ConvertGroupToIndividualsAndUnion(int32 sendto_count, const HostId* sendto_list, HostIdArray& output)
{
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  ILanSendDestList_C list;
  ConvertGroupToIndividualsAndUnion(sendto_count, sendto_list, list);
  output.ResizeUninitialized(list.Count());
  for (int32 dst_index = 0; dst_index < list.Count(); ++dst_index) {
    auto dst = list[dst_index];
    output[dst_index] = dst ? dst->GetHostId() : HostId_None;
  }
}

// P2P group HostId를 개별 HostId로 바꾼 후 중복되는 것들을 병합한다.
void LanClientImpl::ConvertGroupToIndividualsAndUnion(int32 sendto_count, const HostId* sendto_list, ILanSendDestList_C& SendDestList)
{
  for (int32 sendto_index = 0; sendto_index < sendto_count; ++sendto_index) {
    if (sendto_list[sendto_index] != HostId_None) {
      ConvertAndAppendP2PGroupToPeerList(sendto_list[sendto_index], SendDestList);
    }
  }

  algo::UnionDuplicateds(SendDestList);
}

bool LanClientImpl::ConvertAndAppendP2PGroupToPeerList(HostId sendto, ILanSendDestList_C& SendTo2)
{
  LockMain_AssertIsLockedByCurrentThread();

  // convert sendto group to remote hosts
  if (auto group = GetP2PGroupByHostId_INTERNAL(sendto)) {
    for (const auto& member_pair : group->members_) {
      const HostId member_id = member_pair.key;
      SendTo2.Add(GetSendDestByHostId(member_id));
    }
  } else {
    SendTo2.Add(GetSendDestByHostId(sendto));
  }

  return true;
}

ISendDest_C* LanClientImpl::GetSendDestByHostId(HostId peer_id) {
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (peer_id == HostId_Server) {
    return &server_as_send_dest_;
  } else if (peer_id == HostId_None) {
    return &ISendDest_C::None;
  } else if (peer_id == local_host_id_) {
    return this;
  } else {
    auto peer = GetPeerByHostId_NOLOCK(peer_id);
    if (peer == nullptr && settings_.look_ahead_p2p_send_enabled) {
      // Lookahead P2P를 위해 candidate에 있는 놈이라도 빼서준다.
      auto candidate = candidate_remote_peers_.FindRef(peer_id);
      if (candidate) {
        return candidate;
      }
    }

    return peer;
  }
}

double LanClientImpl::GetIndirectServerTime(HostId peer_id) {
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (clock_.IsStopped()) {
    return -1;
  }

  const double absolute_time = clock_.AbsoluteSeconds();

  if (auto lp = GetPeerByHostId_NOLOCK(peer_id)) {
    return absolute_time - lp->GetIndirectServerTimeDiff();
  } else {
    return absolute_time - dx_server_time_diff_;
  }
}

LanRemotePeer_C* LanClientImpl::GetPeerByHostId_NOLOCK(HostId peer_id) {
  LockMain_AssertIsLockedByCurrentThread();

  return authed_remote_peers_.FindRef(peer_id);
}

P2PGroupPtr_C LanClientImpl::GetP2PGroupByHostId_INTERNAL(HostId group_id) {
  //CScopedLock2 main_guard(mutex_);

  P2PGroupPtr_C group;
  p2p_groups_.TryGetValue(group_id, group);
  return group;
}

ResultCode LanClientImpl::ExtractMessagesFromTcpStream(ReceivedMessageList& output) {
  output.Clear();

  LockMain_AssertIsNotLockedByCurrentThread();
  to_server_tcp_mutex_.AssertIsLockedByCurrentThread();

  ResultCode extract_result = ResultCode::Ok;
  const int32 added_count = MessageStream::ExtractMessagesAndFlushStream(
                                  to_server_tcp_->recv_stream_,
                                  output,
                                  HostId_Server,
                                  settings_.message_max_length,
                                  extract_result);
  if (added_count < 0) {
    // 서버와의 TCP 연결에 문제가 있다는 뜻이므로 연결 해제를 유도한다.
    // 데드락의 문제가 있어 삭제.
    //EnqueueError(ResultInfo::From(extract_result, HostId_Server, "Received stream from TCP server became inconsistent.")); //InduceDisconnect();연결 해제 유도.
    return extract_result;
  }

  return extract_result;
}

bool LanClientImpl::IsValidHostId_NOLOCK(HostId host_id) {
  CheckCriticalSectionDeadLock(__FUNCTION__);
  LockMain_AssertIsLockedByCurrentThread();

  if (host_id == HostId_Server) {
    return true;
  } else if (host_id == local_host_id_) {
    return true;
  } else if (GetPeerByHostId_NOLOCK(host_id)) {
    return true;
  }

  if (candidate_remote_peers_.Contains(host_id)) {
    return true;
  }

  if (dispose_issued_remote_peers_map_.Contains(host_id)) {
    return true;
  }

  return false;
}

void LanClientImpl::EnqueueDisconnectionEvent(ResultCode result_code, ResultCode detail_code) {
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (callbacks_ && !suppress_subsequenct_disconnection_events_) {
    suppress_subsequenct_disconnection_events_ = true;

    LocalEvent event(LocalEventType::ClientServerDisconnect);
    event.result_info.Reset(new ResultInfo());
    event.result_info->result_code = result_code;
    event.result_info->detail_code = detail_code;
    event.remote_id = HostId_Server;
    EnqueueLocalEvent(event);

    //if (viz_agent_) {
    //  CScopedLock2 viz_agent_guard(viz_agent_->mutex_);
    //  viz_agent_->c2s_proxy_.NotifyClient_ConnectionState(HostId_Server, GReliableSend_INTERNAL, GetServerConnectionState());
    //}
  }
}

INetCoreCallbacks* LanClientImpl::GetCallbacks_NOLOCK()
{
  return callbacks_;
}

void LanClientImpl::ProcessOneLocalEvent(LocalEvent& event)
{
  LockMain_AssertIsNotLockedByCurrentThread();

  if (callbacks_) {
    try {
      //@maxidea: LanClient는 싱글 스레드가 아니므로, 아래의 플래그들에 해당하는 동작들은 유효하지 않을듯 싶은데??

      callbacks_->bHolsterMoreCallback_FORONETHREADEDMODEL = false;// one thread model인 클라라서 안전
      callbacks_->bPostponeThisCallback_FORONETHREADEDMODEL = false; //역시 안전

      switch (event.type) {
      case LocalEventType::ConnectServerSuccess: {
          //TODO 구지 생성해서 넘겨주고, 바로 파괴되어야 하는걸까??
          //아래처럼 NULL 객체를 하나 만들어서 사용하는것도 좋을듯 싶은데...
          //SharedPtr<ResultInfo> result_info = ResultInfo::EmptyPtr;
          SharedPtr<ResultInfo> result_info = SharedPtr<ResultInfo>(new ResultInfo());
          callbacks_->OnJoinedToServer(result_info.Get(), event.user_data);
          break;
        }

      case LocalEventType::ConnectServerFail: {
          SharedPtr<ResultInfo> result_info = ResultInfo::From(event.result_info->result_code, HostId_Server, "");
          result_info->remote_addr = event.remote_addr;
          result_info->socket_error = event.socket_error;
          callbacks_->OnJoinedToServer(result_info.Get(), event.user_data);
        }
        break;

      case LocalEventType::ClientServerDisconnect:
        callbacks_->OnLeftFromServer(event.result_info.Get());
        break;

      case LocalEventType::P2PAddMember:
        callbacks_->OnP2PMemberJoined(event.member_id, event.group_id, event.member_count, event.custom_field);
        break;

      case LocalEventType::P2PLeaveMember:
        callbacks_->OnP2PMemberLeft(event.member_id, event.group_id, event.member_count);
        break;

      case LocalEventType::SynchronizeServerTime:
        callbacks_->OnSynchronizeServerTime();
        break;

      case LocalEventType::Error:
        callbacks_->OnError(ResultInfo::From(event.result_info->result_code, event.remote_id, event.result_info->comment).Get());
        break;

      case LocalEventType::Warning:
        callbacks_->OnWarning(ResultInfo::From(event.result_info->result_code, event.remote_id, event.result_info->comment).Get());
        break;

      case LocalEventType::DirectP2PEnabled:
        callbacks_->OnP2PConnectionEstablished(event.remote_id);
        break;

      case LocalEventType::GroupP2PEnabled:
        callbacks_->OnGroupP2PConnectionComplete(event.group_id);
        break;

      case LocalEventType::P2PDisconnected:
        callbacks_->OnP2PDisconnected(event.remote_id, event.result_info->result_code);
        break;

      //case LocalEventType::ClientLeaveAfterDispose:
      //  break;
      }

      //out_holster_more_callback = callbacks_->bHolsterMoreCallback_FORONETHREADEDMODEL;
      //out_postpone_this_callback = callbacks_->bPostponeThisCallback_FORONETHREADEDMODEL;
    }
    catch (Exception& e) {
      if (callbacks_) {
        callbacks_->OnException(event.remote_id, e);
      }
    }
    catch (std::exception& e) {
      if (callbacks_) {
        callbacks_->OnException(event.remote_id, Exception(e));
      }
    }
    //catch (_com_error& e) {
    //  if (callbacks_) {
    //    callbacks_->OnException(event.remote_id, Exception(e));
    //  }
    //}
    //catch (void* e) {
    //  if (callbacks_) {
    //    callbacks_->OnException(event.remote_id, Exception(e));
    //  }
    //}
#ifdef ALLOW_CATCH_UNHANDLED_EVEN_FOR_USER_ROUTINE
    catch (...) {
      if (callbacks_) {
        Exception e;
        e.exception_type = ExceptionType::Unhandled;
        callbacks_->OnException(event.remote_id, e);
      }
    }
#endif
  }
}

bool LanClientImpl::NextEncryptCount(HostId remote_id, CryptoCountType& out_count)
{
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  // 아직 서버와 연결된 상태가 아니라면...encryptcount는 존재하지 않는다.
  if (!to_server_tcp_.IsValid() || local_host_id_ == HostId_None) {
    return false;
  }

  if (local_host_id_ == remote_id) {
    out_count = self_encrypt_count_;
    ++self_encrypt_count_;
    return true;
  }
  else if (remote_id == HostId_Server) {
    out_count = to_server_encrypt_count_;
    ++to_server_encrypt_count_;
    return true;
  }
  else if (auto lp = GetPeerByHostId_NOLOCK(remote_id)) {
    out_count = lp->encrypt_count;
    ++lp->encrypt_count;
    return true;
  }

  return false;
}

void LanClientImpl::PrevEncryptCount(HostId remote_id)
{
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  // 아직 서버와 연결된 상태가 아니라면...encryptcount는 존재하지 않는다.
  if (!to_server_tcp_.IsValid() || local_host_id_ == HostId_None) {
    return;
  }

  if (remote_id == local_host_id_) {
    --self_encrypt_count_;
  }
  else if (remote_id == HostId_Server) {
    --to_server_encrypt_count_;
  }
  else if (auto lp = GetPeerByHostId_NOLOCK(remote_id)) {
    --lp->encrypt_count;
  }
}

bool LanClientImpl::GetExpectedDecryptCount(HostId remote_id, CryptoCountType& out_count)
{
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (remote_id == local_host_id_) {
    out_count = self_decrypt_count_;
    return true;
  }
  else if (remote_id == HostId_Server) {
    out_count = to_server_decrypt_count_;
    return true;
  }
  else if (auto lp = GetPeerByHostId_NOLOCK(remote_id)) {
    out_count = lp->decrypt_count;
    return true;
  }

  return false;
}

bool LanClientImpl::NextDecryptCount(HostId remote_id)
{
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (remote_id == local_host_id_) {
    ++self_decrypt_count_;
    return true;
  }
  else if (remote_id == HostId_Server) {
    ++to_server_decrypt_count_;
    return true;
  }
  else if (auto lp = GetPeerByHostId_NOLOCK(remote_id)) {
    ++lp->decrypt_count;
    return true;
  }

  return false;
}

SessionKey* LanClientImpl::GetCryptSessionKey(HostId remote_id, String& out_error)
{
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  SessionKey* key = nullptr;

  if (local_host_id_ == remote_id) {
    key = &self_p2p_session_key_;
  }
  else if (remote_id == HostId_Server) {
    key = &to_server_session_key_;
  }
  else {
    auto lp = GetPeerByHostId_NOLOCK(remote_id);
    if (lp) {
      key = &lp->p2p_session_key;
    }

    if (key == nullptr) {
      out_error = String::Format("%d remote lp is %s in LanClient.", (int32)remote_id, lp == nullptr ? "NULL" : "not NULL");
    }
  }

  if (key && !key->KeyExists()) {
    out_error = "key is not exists.";
    return nullptr;
  }

  return key;
}

void LanClientImpl::EnqueueError(SharedPtr<ResultInfo> result_info)
{
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (callbacks_) {
    LocalEvent event(LocalEventType::Error);
    event.result_info.Reset(new ResultInfo());
    event.result_info->result_code = result_info->result_code;
    event.result_info->comment = result_info->comment;
    event.remote_id = result_info->remote;
    event.remote_addr = result_info->remote_addr;
    EnqueueLocalEvent(event);
  }
}

void LanClientImpl::EnqueueLocalEvent(LocalEvent& event)
{
  LockMain_AssertIsLockedByCurrentThread();

  if (listener_thread_) {
    final_user_work_queue_.Enqueue(event);
    user_task_queue_.AddTaskSubject(this);
  }
}

void LanClientImpl::EnqueueUserTask(Function<void()> func)
{
  LockMain_AssertIsLockedByCurrentThread();

  if (listener_thread_) {
    final_user_work_queue_.Enqueue(func);
    user_task_queue_.AddTaskSubject(this);
  }
}

void LanClientImpl::EnqueueWarning(SharedPtr<ResultInfo> result_info)
{
  CScopedLock2 main_guard(mutex_);
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

bool LanClientImpl::AsyncCallbackMayOccur()
{
  return listener_thread_;
}

void LanClientImpl::Disconnect()
{
  Disconnect(NetConfig::default_graceful_disconnect_timeout_sec, ByteArray());
}

void LanClientImpl::IssueDisposeRemotePeer(
      LanRemotePeer_C* lp,
      ResultCode result_code,
      ResultCode detail_code,
      const ByteArray& comment,
      const char* where,
      SocketErrorCode socket_error,
      bool remove_from_collection)
{
  LockMain_AssertIsLockedByCurrentThread();

  if (!lp->dispose_waiter_) { // once
    lp->dispose_waiter_.Reset(new LanRemotePeer_C::DisposeWaiter());
    lp->dispose_waiter_->reason = result_code;
    lp->dispose_waiter_->detail = detail_code;
    lp->dispose_waiter_->comment = comment;
    lp->dispose_waiter_->socket_error = socket_error;
  }

  if (!dispose_issued_remote_peers_map_.Contains(lp->host_id_)) {
    //authed에서 빼지 않는다.

    if (remove_from_collection) {
      RemotePeer_RemoveFromCollections(lp);
    }

    dispose_issued_remote_peers_map_.Add(lp->host_id_, lp);

    //FUN_TRACE("%d peer dispose add\n", (int)lp->host_id_);
  }
  else {
    //FUN_TRACE("%d RPContainsKey\n", (int)lp->host_id_);
    return; // 이렇게 하면 아래 CloseSocketOnly가 자주호출되는 문제를 피할 듯.
  }

  // 이렇게 소켓을 닫으면 issue중이던 것들이 모두 종료한다. 그리고 나서 재 시도를 하려고 해도
  // DisposeWaiter가 있으므로 되지 않을 것이다. 그러면 안전하게 객체를 파괴 가능.
  if (lp->tcp_transport_.socket_) {
    lp->tcp_transport_.socket_->CloseSocketHandleOnly();

    if (intra_logger_) {
      const String text = String::Format("CloseSocketHandleOnly() called at %s", __FUNCTION__);
      intra_logger_->WriteLine(LogCategory::System, *text);
    }
  }

  lp->WarnTooShortDisposal(where);

  for (auto& pair : lp->joined_p2p_groups_) {
    // Remove from P2PGroup_Add ack info
    auto p2p_group = pair.value;

    // P2P그룹에서 제거
    p2p_group->members_.Remove(lp->host_id_);

    // 제명한 후 P2P그룹이 잔존해야 한다면...
    if (p2p_group->members_.IsEmpty()) {
      // P2P그룹이 파괴되어야 한다면...
      //EnqueueP2PGroupRemoveEvent(p2p_group->group_id_);
      p2p_groups_.Remove(p2p_group->group_id_);
    }
  }

  lp->joined_p2p_groups_.Clear();
}

void LanClientImpl::RemotePeer_RemoveFromCollections(LanRemotePeer_C* lp)
{
  candidate_remote_peers_.Remove(lp->host_id_);

  authed_remote_peers_.Remove(lp->host_id_);
}

void LanClientImpl::DisconnectRemotePeerOnTimeout()
{
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  const double absolute_time = GetAbsoluteTime();

  for (auto it = authed_remote_peers_.CreateIterator(); it; ++it) {
    auto lp = it->value;

    if (lp->last_tcp_stream_recv_time_ == 0 || lp->dispose_requested_) {
      continue;
    }

    if ((absolute_time - lp->last_tcp_stream_recv_time_) > settings_.default_timeout_sec) {
      NotifyP2PDisconnected(lp, ResultCode::TimeOut);

      //if (IssueDisposeRemotePeer(lp, ResultCode::DisconnectFromRemote, ResultCode::ConnectServerTimeout, ByteArray(), __FUNCTION__, SocketErrorCode::Ok, false)) {
      //  //dispose에 추가되었으므로 authed와 candidate에서는 제거를 한다.
      //  it.RemoveCurrent();
      //  RemotePeer_RemoveFromCollections(lp);
      //}
      //else {
      //  ++it;
      //}
    }
  }
}

void LanClientImpl::DisposeIssuedRemotePeers()
{
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  for (auto it = dispose_issued_remote_peers_map_.CreateIterator(); it; ++it) {
    auto lp = it->value;

    CScopedLock2 lp_tcp_guard(lp->tcp_transport_.GetMutex());
    CScopedLock2 to_server_tcp_guard(tcp_issue_queue_mutex_);

    // UseCount 검사.
    const bool use_count_safe = lp->GetUseCount() == 0;

    // 소켓을 안전하게 닫을 수 있는지 검사.
    const bool tcp_close_safe = !lp->tcp_transport_.recv_issued_ && !lp->tcp_transport_.send_issued_;

    // 아직 처리중이라면, 대기를 해야함.
    const bool works_remain = (lp->task_subject_node_.GetListOwner() || lp->IsTaskRunning());

    lp_tcp_guard.Unlock();

    if (use_count_safe && tcp_close_safe && !works_remain) {
      ProcessOnPeerDisposeCanSafe(lp);
      it.RemoveCurrent();
      remote_peer_instances_.Remove(lp);

      // destroy remote-peer instance.
      delete lp;
    }
  }

  for (auto it = remote_peer_garbages_.CreateIterator(); it;) {
    auto lp = *it;

    CScopedLock2 lp_tcp_guard(lp->tcp_transport_.GetMutex());
    const bool use_count_safe = lp->GetUseCount() == 0;
    const bool tcp_close_safe = !lp->tcp_transport_.recv_issued_ && !lp->tcp_transport_.send_issued_;
    lp_tcp_guard.Unlock();

    const bool should_remove = use_count_safe && tcp_close_safe;
    if (should_remove) {
      ProcessOnPeerDisposeCanSafe(lp);
      remote_peer_garbages_.Remove(it);
    }
    else {
      ++it;
    }
  }
}

void LanClientImpl::ProcessOnPeerDisposeCanSafe(LanRemotePeer_C* lp)
{
  LockMain_AssertIsLockedByCurrentThread();

  // 송신 이슈 큐에서도 제거
  lp->UnlinkSelf();

  // 이벤트 노티
  // 원래는 RemoteClient에 enquelocalevent를 처리 하려 했으나.
  // delete되기 전에 enque하는 것이므로 다른 쓰레드에서 이 RemoteClient에 대한 이벤트가
  // 나오지 않을 것이므로, 이렇게 처리해도 무방하다.
  //if (lp->dispose_waiter_) {
  //  EnqueuePeerLeaveEvent(lp, lp->dispose_waiter_->reason, lp->dispose_waiter_->detail, lp->dispose_waiter_->comment, lp->dispose_waiter_->socket_error);
  //}
  //else {
  //  EnqueuePeerLeaveEvent(lp, ResultCode::DisconnectFromLocal, ResultCode::ConnectServerTimeout, ByteArray(), SocketErrorCode::Ok);
  //}

  // dispose peer object (including Sockets)
  if (lp->tcp_transport_.socket_) {
    lp->tcp_transport_.socket_->CloseSocketHandleOnly();

    if (intra_logger_) {
      const String text = String::Format("CloseSocketHandleOnly() called at %s", __FUNCTION__);
      intra_logger_->WriteLine(LogCategory::System, *text);
    }
  }

  //RemotePeer_RemoveFromCollections(lp);
}

void LanClientImpl::EnqueuePeerLeaveEvent(
      LanRemotePeer_C* lp,
      ResultCode result_code,
      ResultCode detail_code,
      const ByteArray& comment,
      SocketErrorCode socket_error)
{
  LockMain_AssertIsLockedByCurrentThread();

  if (callbacks_ && lp->host_id_ != HostId_None) {
    LocalEvent event(LocalEventType::ClientLeaveAfterDispose);
    event.result_info.Reset(new ResultInfo());
    event.result_info->result_code = result_code;
    event.result_info->detail_code = detail_code;
    event.peer_info = lp->GetPeerInfo();
    event.remote_id = lp->host_id_;
    event.comment = comment;
    EnqueueLocalEvent(event);
  }
}

void LanClientImpl::EnqueuePeerConnectionEstablishEvent(LanRemotePeer_C* lp)
{
  LockMain_AssertIsLockedByCurrentThread();

  if (callbacks_ && lp->host_id_ != HostId_None) {
    LocalEvent event(LocalEventType::DirectP2PEnabled);
    event.peer_info = lp->GetPeerInfo();
    event.remote_id = lp->host_id_;
    EnqueueLocalEvent(event);
  }
}

void LanClientImpl::EnqueuePeerDisconnectEvent(LanRemotePeer_C* lp, ResultCode result_code)
{
  LockMain_AssertIsLockedByCurrentThread();

  if (callbacks_ && lp->host_id_ != HostId_None) {
    LocalEvent event(LocalEventType::P2PDisconnected);
    event.result_info = ResultInfo::From(result_code);
    event.peer_info = lp->GetPeerInfo();
    event.remote_id = lp->host_id_;
    EnqueueLocalEvent(event);
  }
}

void LanClientImpl::Disconnect(double graceful_disconnect_timeout, const ByteArray& comment)
{
  CScopedLock2 connect_disconnect_phase_guard(connect_disconnect_phase_mutex_);

  if (user_thread_pool_ && user_thread_pool_->IsCurrentThread()) {
    // 이상하게도 userworkerthread내에서 try catch로 잡으면
    // 파괴자가 호출되지 않는 현상이 생긴다.
    connect_disconnect_phase_guard.Unlock();
    throw Exception("Call Disconnect() in UserWorker");
  }

  Disconnect_INTERNAL(graceful_disconnect_timeout, comment);

  LockMain_AssertIsNotLockedByCurrentThread();

  listener_thread_.Reset();

  fun_check(state_ == ConnectionState::Disconnected);
}

//void LanClientImpl::DisconnectNoWait()
//{
//  DisconnectNoWait(NetConfig::default_graceful_disconnect_timeout_sec, ByteArray());
//}
//
//
//
//void LanClientImpl::DisconnectNoWait(double graceful_disconnect_timeout_sec, const ByteArray& comment)
//{
//  CScopedLock2 PhaseLock(connect_disconnect_phase_mutex_, true);
//
//  Disconnect_INTERNAL(graceful_disconnect_timeout_sec, comment);
//
//  Cleanup();
//}

void LanClientImpl::Disconnect_INTERNAL(double graceful_disconnect_timeout_sec, const ByteArray& comment)
{
  LockMain_AssertIsNotLockedByCurrentThread();

  bool issue_disconnect_done = false;

  disconnection_invoke_count_.Increment();

  const uint32 t0 = Clock::Milliseconds();
  const uint32 timeout = MathBase::Max<uint32>((uint32)(graceful_disconnect_timeout_sec * 2 * 1000), 10000);

  int32 wait_count = 0;
  {
    CScopedLock2 main_guard(mutex_);
    CheckCriticalSectionDeadLock(__FUNCTION__);

    if (intra_logger_) {
      intra_logger_->WriteLine("LanClient Disconnect() call");
    }
  }

  //TODO 하트비트가 안불리므로, 시간 갱신이 안될터인데... 문제점을 좀 짚어볼 필요가 있음.
  bool safe_disconnect = false;
  // 완전히 디스될 때까지 루프를 돈다. 디스 이슈는 필요시 하고.
  while (true) {
    CScopedLock2 main_guard(mutex_);
    CheckCriticalSectionDeadLock(__FUNCTION__);

    // 이미 디스된 상황이면 그냥 나간다.
    //if (listener_thread_ == nullptr) {
    //  // 어쨌건 나가도 되는 상황
    //  safe_disconnect = true;
    //  break;
    //}

    if (GetState() == ConnectionState::Disconnected && remote_peer_instances_.IsEmpty()) {
      safe_disconnect = true;
      break;
    }

    //TODO
    // 프로세스 종료중이면 아무것도 하지 않는다.
    //if (CThread::bDllProcessDetached_INTERNAL) {
    //  safe_disconnect = true;
    //  break;
    //}

    // 이게 있으면, Heartbeat가 돌아가는 상황에서 break가 걸려 버린다.
    //if (networker_thread_->should_stop_ == true) {
    //  safe_disconnect = true;
    //  break;
    //}

    // completion port가 이미 파괴되었음(그러면 안되겠지만) 그냥 루프를 나간다.
    if (!tcp_accept_cp_) {
      SetState(ConnectionState::Disconnected);
      safe_disconnect = true;
      break;
    }

    if ((Clock::Milliseconds() - t0) > timeout) {
      const String text = String::Format("LanClient.Disconnect seems to be freezed ## process: %s", CPlatformProcess::ExecutableName());
      ErrorReporter::Report(text);

      SetState(ConnectionState::Disconnected);
      safe_disconnect = true;
      break;
    }

    // 디스 이슈를 건다. 1회만.
    if (!issue_disconnect_done) {
      issue_disconnect_done = true;

      if (GetState() == ConnectionState::Connected) {
        // 언제 디스 작업을 시작할건가를 지정
        shutdown_issued_time_ = GetAbsoluteTime(); // 이거에 의해 Networker State가 disconnecting으로 곧 바뀐다.

        // 서버와의 연결 해제를 서버에 먼저 알린다.
        // 바로 소켓을 닫고 클라 프로세스가 바로 종료하면 shutdown 신호가 TCP 서버로 넘어가지 못하는 경우가 있다.
        // 따라서 서버에서 연결 해제를 주도시킨 후 클라에서 종료하되 시간 제한을 두는 형태로 한다.
        // (즉 TCP의 graceful shutdown을 대신한다.)
        graceful_disconnect_timeout_ = graceful_disconnect_timeout_sec;

        c2s_proxy_.ShutdownTcp(HostId_Server, GReliableSend_INTERNAL, comment);
      }
      else if (GetState() < ConnectionState::Connected) {
        // 서버와 연결중이었으면 당장 디스모드로 전환
        SetState(ConnectionState::Disconnecting);
      }
    }
    // graceful_disconnect_timeout_ 지정 시간이 지난 후 실 종료 과정이 시작된다
    // 실 종료 과정이 완전히 끝날 때까지 대기

    fun_check(listener_thread_.IsValid());

    if (wait_count > 0) { // 보다 빠른 응답성을 위해
      main_guard.Unlock();
      CPlatformProcess::Sleep(0.001f);
    }

    wait_count++;
  }

  timer_.Stop(); // 모든 작업이 완료될때까지 대기함.

  heartbeat_timer_id_ = 0;
  issue_send_on_need_timer_id_ = 0;
  tick_timer_id_ = 0;

  //@fixme
  // 명시적으로 Disconnect()로 끊은게 아니고, 서버에서 연결을 끊은 경우
  // user_thread_pool_, NetThreadPool을 어떻게 해주어야하는가??
  // 대기를 타면 되는건가?? 현재는 오류가 나는데 말이지..
  // Heartbeat_Disconnecting()에서 처리하는 곳에 아래 부분을 카피해서 넣어볼까??

  if (net_thread_pool_.IsValid()) {
    //timerfree후 thread종료.
    //종료를 알림후 대기...
    net_thread_pool_->PostCompletionStatus(this, (UINT_PTR)IocpCustomValue::End);
    if (net_thread_pool_ != user_thread_pool_) {
      user_thread_pool_->PostCompletionStatus(this, (UINT_PTR)IocpCustomValue::End);
    }

    // 여기서 무한 대기를 타는 경우가 있는것 같음...
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
      }
      else {
        net_thread_pool_.Reset();
      }
    }
    else {
      net_thread_pool_.Detach();
    }

    if (!user_thread_external_use_) {
      user_thread_pool_.Reset();
    }
    else {
      user_thread_pool_.Detach();
    }
  }

  Cleanup();
}

void LanClientImpl::ConditionalIssueSend_ToServerTcp()
{
  CScopedLock2 to_server_tcp_guard(to_server_tcp_mutex_);

  if (to_server_tcp_) {
    to_server_tcp_->ConditionalIssueSend();
  }
}

void LanClientImpl::EnqueueConnectFailEvent(ResultCode result_code, SharedPtr<ResultInfo> result_info)
{
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (!suppress_subsequenct_disconnection_events_) {
    suppress_subsequenct_disconnection_events_ = true;

    LocalEvent event(LocalEventType::ConnectServerFail);
    event.result_info.Reset(new ResultInfo());
    event.result_info->result_code = result_code;
    event.result_info->comment = result_info->comment;
    event.remote_id = HostId_Server;
    event.remote_addr = InetAddress(connection_params_.server_ip, connection_params_.server_port);
    event.socket_error = SocketErrorCode::Ok;
    EnqueueLocalEvent(event);
  }
}

void LanClientImpl::EnqueueConnectFailEvent(ResultCode result_code, SocketErrorCode socket_error)
{
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (!suppress_subsequenct_disconnection_events_) {
    suppress_subsequenct_disconnection_events_ = true;

    LocalEvent event(LocalEventType::ConnectServerFail);
    event.result_info.Reset(new ResultInfo());
    event.result_info->result_code = result_code;
    event.remote_id = HostId_Server;
    event.remote_addr = InetAddress(connection_params_.server_ip, connection_params_.server_port);
    event.socket_error = socket_error;
    EnqueueLocalEvent(event);
  }
}

void LanClientImpl::EnqueueHackSuspectEvent(LanRemotePeer_C* lp, const char* Statement, HackType hack_type)
{
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (callbacks_) {
    LocalEvent event(LocalEventType::HackSuspected);
    event.result_info.Reset(new ResultInfo());
    event.hack_type = hack_type;
    event.remote_id = lp ? lp->host_id_ : HostId_None;
    event.result_info->comment = Statement;
    EnqueueLocalEvent(event);
  }
}

LanClientStats::LanClientStats()
{
  total_tcp_recv_bytes_ = 0;
  total_tcp_send_bytes_ = 0;
  remote_peer_count = 0;
}

String LanClientStats::ToString() const
{
  return String::Format("remote_peer_count: %d, total_recv_bytes: %I64d, total_send_bytes: %I64d",
                        remote_peer_count, total_tcp_recv_bytes_, total_tcp_send_bytes_);
}

void LanClientImpl::UpdateP2PGroup_MemberJoin(
      HostId group_id,
      HostId member_id,
      const ByteArray& custom_field,
      uint32 event_id,
      const ByteArray& p2p_aes_session_key,
      const ByteArray& p2p_rc4_session_key,
      const Uuid& connection_tag)
{
  LockMain_AssertIsLockedByCurrentThread();

  //FUN_TRACE("LanClient - MemberJoin Recv add:%d\n", (int)member_id);

  // 서버와 연결이 끊어지고 있는 상황이면 무시해야 한다.
  if (!listener_thread_.IsValid() || GetState() != ConnectionState::Connected) {
    return;
  }

  // create or update P2P group
  // 이 그룹은 local이 소속된 그룹이기도 하다. 어차피 이 RPC는 local이 소속된 그룹에 대해서 호출되니까.
  P2PGroupPtr_C group = GetP2PGroupByHostId_INTERNAL(group_id);
  if (!group.IsValid()) {
    group = CreateP2PGroupObject_INTERNAL(group_id);
  }

  // Server인경우와 아닌경우를 나눈다.
  LanRemotePeer_C* member_lp = nullptr;
  if (member_id != HostId_Server) {
    // create or update the peer
    member_lp = GetPeerByHostId_NOLOCK(member_id);

    if (member_lp == nullptr) {
      candidate_remote_peers_.TryGetValue(member_id, member_lp);
    }

    if (member_id != local_host_id_ && member_lp == nullptr) {
      // instance 를 생성하여 등록한다.
      member_lp = new LanRemotePeer_C(this);
      member_lp->host_id_ = member_id;
      member_lp->holepunch_tag_ = connection_tag;

      remote_peer_instances_.Add(member_lp);
      candidate_remote_peers_.Add(member_id, member_lp);

      // assign session key
      // and first frame number of reliable UDP

      // AES
      if (!p2p_aes_session_key.IsEmpty()) {
        if (!CryptoAES::ExpandFrom(member_lp->p2p_session_key.aes_key, (const uint8*)p2p_aes_session_key.ConstData(), settings_.strong_encrypted_message_key_length / 8)) {
          throw Exception("Failed to create session-key");
        }
      }
      else {
        member_lp->p2p_session_key.aes_key.Reset();
      }

      // RC4
      if (!p2p_rc4_session_key.IsEmpty()) {
        if (!CryptoRC4::ExpandFrom(member_lp->p2p_session_key.rc4_key, (const uint8*)p2p_rc4_session_key.ConstData(), settings_.weak_encrypted_message_key_length / 8)) {
          throw Exception("Failed to create session-key");
        }
      }
      else {
        // Fast Encrypt는 안사용할 때도 있다.
        member_lp->p2p_session_key.rc4_key.key_exists = true;
      }

      ConditionalAssignRemotePeerInfo(member_lp);
    }

    if (member_id != local_host_id_) {
      // update peer's joined Groups
      member_lp->joined_p2p_groups_.Add(group->group_id_, group);

      group->members_.Add(member_id, member_lp);
    }
    else {
      group->members_.Add(member_id, this);
    }
  }
  else {
    // 서버도 그룹에 추가한다.
    group->members_.Add(member_id, &server_as_send_dest_);
  }

  // P2P-member-add-ack RPC with received event time
  c2s_proxy_.P2PGroup_MemberJoin_Ack(HostId_Server, GReliableSend_INTERNAL, group_id, member_id, event_id);

  // P2PAddMember
  LocalEvent event(LocalEventType::P2PAddMember);
  event.group_id = group_id;
  event.member_id = member_id;
  event.remote_id = member_id;
  event.member_count = group->members_.Count();
  event.custom_field = custom_field;

  if (member_lp) {
    member_lp->EnqueueLocalEvent(event);
  }
  else {
    EnqueueLocalEvent(event);
  }
}

//안쓰임???
void LanClientImpl::ConditionalAssignRemotePeerInfo(LanRemotePeer_C* lp)
{
  LockMain_AssertIsLockedByCurrentThread();

  if (auto dispose_lp = dispose_issued_remote_peers_map_.FindRef(lp->host_id_)) {
    lp->task_running_ = dispose_lp->task_running_;

    // 일거리를 옮긴다.
    //DisposeRP의 FinalUserworkItemList를 lp->FinalUserworkItemList쪽으로 옮겨 주는건가???
    lp->final_user_work_queue_.Append(dispose_lp->final_user_work_queue_);

    // 태스크는 실행중이 아님으로 표시.
    dispose_lp->task_running_ = false;

    // 삭제후.
    dispose_issued_remote_peers_map_.Remove(lp->host_id_);
    remote_peer_instances_.Remove(dispose_lp);

    // 가비지로 옮김.
    remote_peer_garbages_.Append(dispose_lp);
  }
}

P2PGroupPtr_C LanClientImpl::CreateP2PGroupObject_INTERNAL(HostId group_id)
{
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  P2PGroupPtr_C new_group(new P2PGroup_C());
  new_group->group_id_ = group_id;
  p2p_groups_.Add(group_id, new_group);
  return new_group;
}

bool LanClientImpl::Send( const SendFragRefs& data_to_send,
                          const SendOption& send_opt,
                          const HostId* sendto_list,
                          int32 sendto_count)
{
  if (!listener_thread_.IsValid()) {
    return false;
  }

  //@note
  // Send_CompressLayer -> Send_SecureLayer -> Send_BroadcastLayer 순으로 전파됨.
  // 이름을 살짝 변경해주는게 좋지 않을까?
  // Send_RootLayer? 좋은 이름이 생각이 안난다???

  // 메시지 압축 레이어를 통하여 메시지에 압축 여부 관련 헤더를 삽입한다.
  // 암호화 된 후에는 데이터의 규칙성이 없어져서 압축이 재대로 되지 않기 때문에 반드시 암호화 전에 한다.
  return Send_CompressLayer(data_to_send, send_opt, sendto_list, sendto_count);
}

void LanClientImpl::EnableVizAgent(const char* viz_server_ip, int32 viz_server_port, const String& login_key)
{
  //if (viz_agent == nullptr) { // only one
  //  viz_agent_.Reset(new VizAgent(this, viz_server_ip, viz_server_port, login_key));
  //}
}

void LanClientImpl::Viz_NotifySendByProxy(const HostId* sento_list,
                                          int32 sendto_count,
                                          const MessageSummary& summary,
                                          const RpcCallOption& rpc_call_opt)
{
  //if (viz_agent_) {
  //  Array<host_id> targets;
  //  targets.ResizeUninitialized(sendto_count);
  //  UnsafeMemory::Memcpy(targets.MutableData(), sento_list, sizeof(HostId) * sendto_count);
  //
  //  viz_agent_->c2s_proxy_.NotifyCommon_SendRpc(HostId_Server, GReliableSend_INTERNAL, targets, rpc_name, rpc_id);
  //}
}

void LanClientImpl::Viz_NotifyRecvToStub( HostId rpc_recvfrom,
                                          RpcId rpc_id,
                                          const char* rpc_name,
                                          const char* params_as_string)
{
  //if (viz_agent_) {
  //  viz_agent_->c2s_proxy_.NotifyCommon_ReceiveRpc(HostId_Server, GReliableSend_INTERNAL, rpc_recvfrom, rpc_name, rpc_id);
  //}
}

//@todo 왜 이래해야 하는걸까??
void LanClientImpl::ShowError_NOLOCK(SharedPtr<ResultInfo> result_info)
{
  if (intra_logger_) {
    intra_logger_->WriteLine(LogCategory::System, *result_info->ToString());
  }

  NetCoreImpl::ShowError_NOLOCK(result_info);
}

//@todo 왜 이래해야 하는걸까??
void LanClientImpl::ShowNotImplementedRpcWarning(RpcId rpc_id, const char* rpc_name)
{
  NetCoreImpl::ShowNotImplementedRpcWarning(rpc_id, rpc_name);
}

//@todo 왜 이래해야 하는걸까??
void LanClientImpl::PostCheckReadMessage(IMessageIn& msg, RpcId rpc_id, const char* rpc_name)
{
  NetCoreImpl::PostCheckReadMessage(msg, rpc_id, rpc_name);
}

String LanClientImpl::DumpGroupStatus()
{
  //@todo ???
  return String();
}

void LanClientImpl::GetGroupMembers(HostId group_id, HostIdArray& output)
{
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (auto group = GetP2PGroupByHostId_INTERNAL(group_id)) {
    group->members_.GenerateKeyArray(output);
  }
  else {
    output.Clear(); // just in case
  }
}

HostId LanClientImpl::GetLocalHostId()
{
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  return local_host_id_;
}

void LanClientImpl::GetLocalJoinedP2PGroups(HostIdArray& output)
{
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  p2p_groups_.GenerateKeyArray(output);
}

void LanClientImpl::GetStats(LanClientStats& out_stats)
{
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  out_stats.Reset();
  out_stats.total_tcp_recv_bytes_ = total_tcp_recv_bytes_;
  out_stats.total_tcp_send_bytes_ = total_tcp_send_bytes_;
  out_stats.remote_peer_count = authed_remote_peers_.Count();
}

double LanClientImpl::GetP2PServerTime(HostId group_id)
{
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  // 서버와 그룹 멤버의 서버 시간의 평균을 구한다.
  int32 count = 1;
  double sum = GetServerTimeDiff();
  if (auto group = GetP2PGroupByHostId_INTERNAL(group_id)) {
    if (clock_.IsStopped()) {
      return -1;
    }

    for (auto& member_pair : group->members_) {
      if (auto peer = member_pair.value) {
        fun_check(peer->GetHostId() != HostId_Server);
        count++;
        sum += peer->GetIndirectServerTimeDiff();
      }
    }

    const double group_server_time_diff = sum / ((double)count);
    const double absolute_time = clock_.AbsoluteSeconds();

    return absolute_time - group_server_time_diff;
  }
  else {
    return GetServerTime();
  }
}

InetAddress LanClientImpl::GetServerAddress()
{
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  return to_server_tcp_.IsValid() ? to_server_tcp_->socket_->GetPeerName() : InetAddress::None;
}

bool LanClientImpl::GetPeerInfo(HostId peer_id, PeerInfo& out_info)
{
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  LanRemotePeer_C* lp = nullptr;
  if (!authed_remote_peers_.TryGetValue(peer_id, lp)) {
    if (!candidate_remote_peers_.TryGetValue(peer_id, lp)) {
      return false;
    }
  }

  lp->GetPeerInfo(out_info);
  return true;
}

double LanClientImpl::GetServerTime()
{
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (clock_.IsStopped()) {
    return -1;
  }

  const double absolute_time = clock_.AbsoluteSeconds();
  return absolute_time - dx_server_time_diff_;
}

double LanClientImpl::GetServerTimeDiff()
{
  return dx_server_time_diff_;
}

ConnectionState LanClientImpl::GetServerConnectionState()
{
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  return GetState();
}

void LanClientImpl::SetCallbacks(ILanClientCallbacks* callbacks)
{
  if (AsyncCallbackMayOccur()) {
    //TODO Exception 클래스를 특수화하는게 좋을듯함.
    //CAsyncCallbackOccurException() 같은...?  이름은 좀더 생각을 해봐야할듯..
    throw Exception("Already async callback may occur.  Server start or client connection should have not been done before here.");
  }

  LockMain_AssertIsNotLockedByCurrentThread();
  callbacks_ = callbacks;
}

double LanClientImpl::GetLastPingSec(HostId remote_id)
{
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (remote_id == HostId_Server) {
    return server_tcp_last_ping_;
  }

  if (auto lp = GetPeerByHostId_NOLOCK(remote_id)) {
    return lp->last_ping_;
  }

  // P2P Group을 얻으려고 하는 경우 모든 멤버들의 평균 핑을 구한다.
  if (auto group = GetP2PGroupByHostId_INTERNAL(remote_id)) {
    // Touch jit P2P & get recent ping ave
    int32 count = 0;
    double sum = 0;
    for (const auto& member_pair : group->members_) {
      //@maxidea: 핑만 조회해도 JIT P2P를 시도함...(뭐 나쁜 선택은 아닌듯 보이긴 한데...)
      const double ping = GetLastPingSec(member_pair.key); // Touch JIT P2P
      if (ping >= 0) // include 0 {
        count++;
        sum += ping;
      }
    }

    if (count > 0) {
      return sum / (double)count;
    }
  }

  return -1;
}

double LanClientImpl::GetRecentPingSec(HostId remote_id)
{
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (remote_id == HostId_Server) {
    return server_tcp_recent_ping_;
  }

  if (LanRemotePeer_C* lp = GetPeerByHostId_NOLOCK(remote_id)) {
    return lp->recent_ping_;
  }

  // P2P Group을 얻으려고 하는 경우 모든 멤버들의 평균 핑을 구한다.
  if (auto group = GetP2PGroupByHostId_INTERNAL(remote_id)) {
    // Touch JIT P2P & get recent ping ave
    int32 count = 0;
    double sum = 0;
    for (const auto& member_pair : group->members_) {
      //@maxidea: 핑만 조회해도 JIT P2P를 시도함...(뭐 나쁜 선택은 아닌듯 보이긴 한데...)
      const double ping = GetRecentPingSec(member_pair.key); // Touch JIT P2P
      if (ping >= 0) {
        count++;
        sum += ping;
      }
    }

    if (count > 0) {
      return sum / (double)count;
    }
  }

  return -1;
}

double LanClientImpl::GetSendToServerSpeed()
{
  // Tcp로 속도를 재야 할지도...
  //@maxidea: LAN 내부 망에서도 구동될것이므로 구태여 측정할 필요는 없어보임..
  return 0;
}

uint32 LanClientImpl::GetInternalVersion()
{
  return internal_version_;
}

InetAddress LanClientImpl::GetPublicAddress()
{
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  return to_server_tcp_ ? to_server_tcp_->local_addr_at_server_ : InetAddress::None;
}

void LanClientImpl::GetNetWorkerThreadInfo(Array<ThreadInfo>& output)
{
  if (net_thread_pool_) {
    net_thread_pool_->GetThreadInfos(output);
  }
}

void LanClientImpl::GetUserWorkerThreadInfo(Array<ThreadInfo>& output)
{
  if (user_thread_pool_) {
    user_thread_pool_->GetThreadInfos(output);
  }
}

void LanClientImpl::OnSocketWarning(InternalSocket* socket, const String& msg)
{
  //CScopedLock2 main_guard(mutex_);

  if (intra_logger_) {
    intra_logger_->WriteLine(LogCategory::System, *msg);
  }
}

void LanClientImpl::OnCompletionPortWarning(CompletionPort* port, const char* msg)
{
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (intra_logger_) {
    intra_logger_->WriteLine(LogCategory::System, msg);
  }
}

HostId LanClientImpl::GetHostId() const
{
  return local_host_id_;
}

bool LanClientImpl::IsFinalReceiveQueueEmpty()
{
  LockMain_AssertIsLockedByCurrentThread();

  return final_user_work_queue_.IsEmpty();
}

bool LanClientImpl::IsTaskRunning()
{
  LockMain_AssertIsLockedByCurrentThread();

  return user_task_is_running_;
}

void LanClientImpl::OnSetTaskRunningFlag(bool running)
{
  LockMain_AssertIsLockedByCurrentThread();

  user_task_is_running_ = running;
}

bool LanClientImpl::PopFirstUserWorkItem(FinalUserWorkItem& out_item)
{
  LockMain_AssertIsLockedByCurrentThread();

  if (!final_user_work_queue_.IsEmpty()) {
    out_item.From(final_user_work_queue_.Front(), local_host_id_);
    final_user_work_queue_.RemoveFront();
    return true;
  }
  else {
    return false;
  }
}

void LanClientImpl::ConditionalRequestServerTime()
{
  if (local_host_id_ != HostId_None) {
    CScopedLock2 main_guard(mutex_);
    CheckCriticalSectionDeadLock(__FUNCTION__);

    const double absolute_time = GetAbsoluteTime();

    if ((absolute_time - last_request_server_time_time_) > NetConfig::cs_ping_interval_sec) {
      request_server_time_count_++;
      last_request_server_time_time_ = absolute_time;

      // 핑으로도 쓰인다. (TCP로 보낸다.)
      MessageOut msg_to_send;
      lf::Write(msg_to_send, MessageType::RequestServerTimeAndKeepAlive);
      lf::Write(msg_to_send, absolute_time);
      lf::Write(msg_to_send, server_tcp_recent_ping_);
      {
        CScopedLock2 to_server_tcp_guard(to_server_tcp_mutex_);
        to_server_tcp_->SendWhenReady(SendFragRefs(msg_to_send), TcpSendOption());
      }

      if (intra_logger_) {
        intra_logger_->WriteLine(LogCategory::System, "SendRequestServerTimeAndKeepAlive");
      }
    }

    reliable_ping_alarm_.SetInterval(GetReliablePingTimerInterval());
    if (reliable_ping_alarm_.TakeElapsedTime(GetElapsedTime())) {
      c2s_proxy_.ReliablePing(HostId_Server, GReliableSend_INTERNAL);

      if (intra_logger_) {
        intra_logger_->WriteLine(LogCategory::System, "SendReliablePing");
      }
    }
  }
  else {
    // 서버에 연결 상태가 아니면 아무것도 하지 않는다.
    //last_reliable_pong_received_time_ = last_server_udp_packet_recv_time_;
  }
}

void LanClientImpl::ConditionalSyncIndirectServerTime()
{
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  fun_check(NetConfig::p2p_ping_interval_sec > 0);

  const int32 peer_count = authed_remote_peers_.Count();
  if (peer_count <= 0) {
    return;
  }

  const double elapsed_time = GetElapsedTime();
  const double absolute_time = clock_.AbsoluteSeconds();

  Array<LanRemotePeer_C*,InlineAllocator<256>> collected_lp_list(peer_count, NoInit);
  int32 request_count = 0;
  for (auto& pair : authed_remote_peers_) {
    auto& lp = pair.value;

    if (lp && lp->host_id_ != HostId_Server) {
      lp->sync_indirect_server_time_diff_cooltime_ -= elapsed_time;

      if (lp->sync_indirect_server_time_diff_cooltime_ <= 0) {
        lp->last_ping_send_time_ = absolute_time;
        lp->sync_indirect_server_time_diff_cooltime_ = NetConfig::p2p_ping_interval_sec;

        lp->IncreaseUseCount();
        collected_lp_list[request_count++] = lp;
      }
    }
  }

  MessageOut header;
  lf::Write(header, MessageType::P2PIndirectServerTimeAndPing);
  lf::Write(header, absolute_time);

  SendFragRefs frags(header);

  // main unlock
  main_guard.Unlock();

  int32 request_index = 0;
  while (request_count > 0) {
#if USE_PARALLEL_FOR
#if (_MSC_VER >= 1400)
#pragma omp parallel for
#endif
#endif
    for (request_index = 0; request_index < request_count; ++request_index) {
      auto lp = collected_lp_list[request_index];

      CScopedLock2 lp_tcp_send_queue_guard(lp->tcp_transport_.GetSendQueueMutex(), false);

      if (request_index != 0) {
        if (const bool lock_ok = lp_tcp_send_queue_guard.TryLock()) {
          // Peer가 가진 server time을 다른 Peer에게 전송한다. 즉 간접 서버 시간을 동기화하고자 한다.
          //
          // 일정 시간마다 각 Peer에게 P2P_SyncIndirectServerTime(서버에 의해 동기화된 시간, 랙)을 보낸다.
          // 이걸 받은 상대는 해당 peer 기준으로의 time diff 값을 갖고 있는다. 모든 Peer로부터
          // 이값을 받으면 그리고 Peer가 속한 각 P2P group 범위 내에서의 time diff 평균값을 계산한다.
          //
          // 또한 이 메시지는 P2P 간 keep alive check를 하는 용도로도 쓰인다.

          lp->tcp_transport_.SendWhenReady(frags, TcpSendOption());
          lp_tcp_send_queue_guard.Unlock();

          lp->DecreaseUseCount();
          collected_lp_list[request_index] = collected_lp_list[--request_count]; // 맨 끝에 있는 것을 앞으로 땡겨온다.
        }
      }
      else {
        lp_tcp_send_queue_guard.Lock();
        lp->tcp_transport_.SendWhenReady(frags, TcpSendOption());
        lp_tcp_send_queue_guard.Unlock();

        lp->DecreaseUseCount();
        collected_lp_list[request_index] = collected_lp_list[--request_count]; // 맨 끝에 있는 것을 앞으로 땡겨온다.
      }
    }
  }
}

void LanClientImpl::Tcp_LongTick()
{
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  const int32 total_remote_peer_count = authed_remote_peers_.Count() + candidate_remote_peers_.Count();
  if (total_remote_peer_count <= 0) {
    return;
  }

  Array<LanRemotePeer_C*,InlineAllocator<256>> collected_lp_list(total_remote_peer_count, NoInit);

  int32 total_count = 0;
  for (auto& pair : authed_remote_peers_) {
    auto lp = pair.value;

    lp->IncreaseUseCount();
    collected_lp_list[total_count++] = lp;
  }

  for (auto& pair : candidate_remote_peers_) {
    auto lp = pair.value;

    lp->IncreaseUseCount();
    collected_lp_list[total_count++] = lp;
  }

  const double absolute_time = GetAbsoluteTime();

  // main unlock
  main_guard.Unlock();

  // ToServerTcp의 LongTick 시행
  {
    CScopedLock2 to_server_tcp_guard(to_server_tcp_mutex_);
    to_server_tcp_->LongTick(GetAbsoluteTime());
  }

  while (total_count > 0) {
#if USE_PARALLEL_FOR
#if (_MSC_VER >= 1400)
#pragma omp parallel for
#endif
#endif
    for (int32 lp_index = 0; lp_index < total_count; ++lp_index) {
      auto lp = collected_lp_list[lp_index];

      CScopedLock2 lp_tcp_send_queue_guard(lp->tcp_transport_.GetSendQueueMutex(), false);

      if (lp_index != 0) {
        if (const bool lock_ok = lp_tcp_send_queue_guard.TryLock()) {
          lp->tcp_transport_.LongTick(absolute_time);
          lp_tcp_send_queue_guard.Unlock();

          lp->DecreaseUseCount();
          collected_lp_list[lp_index] = collected_lp_list[--total_count];
        }
      }
      else {
        lp_tcp_send_queue_guard.Lock();
        lp->tcp_transport_.LongTick(absolute_time);
        lp_tcp_send_queue_guard.Unlock();

        lp->DecreaseUseCount();
        collected_lp_list[lp_index] = collected_lp_list[--total_count];
      }
    }
  }
}

void LanClientImpl::ConditionalReportLanP2PPeerPing()
{
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  for (auto& pair : authed_remote_peers_) {
    auto lp = pair.value;
    fun_check_ptr(lp);

    lp->last_report_p2p_peer_ping_cooltime_ -= GetElapsedTime();
    if (lp->last_report_p2p_peer_ping_cooltime_ <= 0) {
      lp->last_report_p2p_peer_ping_cooltime_ += NetConfig::report_lan_p2p_peer_ping_interval_sec;
      c2s_proxy_.ReportP2PPeerPing(HostId_Server, GReliableSend_INTERNAL, lp->host_id_, (uint32)(lp->recent_ping_ * 1000));
    }
  }
}

void LanClientImpl::PurgeTooOldUnmaturePeer()
{
  // 오랫동안 아무 변화가 없는 Peer들을 서버에 리포트 한다.

  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  //const double absolute_time = clock_.AbsoluteSeconds();
  const double absolute_time = GetAbsoluteTime();

  // 너무 길게 잡혀있으면 사용자가 쓰기 불편하므로
  fun_check(NetConfig::lan_peer_connect_peer_timeout_sec < 30);

  for (auto& pair : candidate_remote_peers_) {
    auto lp = pair.value;
    fun_check_ptr(lp);

    // 서버에 노티하도록 하자.
    if ((absolute_time - lp->created_time_) > NetConfig::lan_peer_connect_peer_timeout_sec && !lp->dispose_requested_) {
      NotifyP2PDisconnected(lp, ResultCode::TCPConnectFailure);
      lp->dispose_requested_ = true; // 차후에 제거되겠군..
      EnqueuePeerDisconnectEvent(lp, ResultCode::TCPConnectFailure);
    }
  }
}

//@maxidea: AcceptedPeers는 어디서 삭제하는걸까??
//타임아웃 된것들은 아래서 제거를 하긴 하는데, 정상적으로 처리된건 어디에서 삭제를 하는거지??
void LanClientImpl::PurgeTooOldUnmatureAcceptedPeers()
{
  // 오랫동안 아무 변화가 없는 acceptlist들을 삭제 한다.
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  // 너무 길게 잡혀있으면 사용자가 쓰기 불편하므로
  fun_check(NetConfig::lan_peer_connect_peer_timeout_sec < 30);

  if (accepted_peers_.IsEmpty()) {
    return;
  }

  //_tprintf("[!] LanClientImpl::PurgeTooOldUnmatureAcceptedPeers()\n");

  //const double absolute_time = clock_.AbsoluteSeconds();
  const double absolute_time = GetAbsoluteTime();

  int32 removed_count = 0;
  for (auto it = accepted_peers_.CreateIterator(); it; ++it) {
    auto accepted_info = it->value;

    bool should_destroy = false;

    //@maxidea:
    // 정상적으로 인증이 완료된 경우임..
    if (!accepted_info->socket_) {
      // 소켓 객체가 해제 되어서 제거한다??  그런데, 소켓 객체가 왜 무효화 된걸까??

      if (accepted_info->GetUseCount() == 0) {
        //_tprintf("[!] Purge successfully accepted AcceptedInfo.\n");
        should_destroy = true;
      }
      else {
        //@maxidea: 이경우, 계속 처리가 안되고 대기를 하게 된다면, 누수로 이어질듯 한데???
        //_tprintf("    Pended: UseCount=%d\n"), accepted_info->GetUseCount();
      }
    }

    //@maxidea:
    // 타임아웃된 AcceptedInfo이므로, 폐기하도록 함.
    else if (accepted_info->is_timedout_) {
      // 타임아웃 되어서, 제거한다??
      if (!accepted_info->recv_issued_ && accepted_info->GetUseCount() == 0) {
        should_destroy = true;
      }
    }

    //@maxidea:
    // 20초가 지나도록 연결을 받아주지 않는다면, 폐기하도록 함.
    // 바로 폐기하지는 않고, 다음 턴에 폐기되도록 표시만 해둔다.(is_timedout = true)
    else if ((absolute_time - accepted_info->accepted_time_) > NetConfig::lan_peer_connect_peer_timeout_sec) {
      accepted_info->is_timedout_ = true;
    }

    //@maxidea:
    // 아직 정상적으로 사용되고 있는 경우 이므로, 다음 엔트리로 이동.
    else {
    }

    if (should_destroy) {
      delete accepted_info;
      it.RemoveCurrent();
      removed_count++;
    }
  }
}

void LanClientImpl::NotifyP2PDisconnected(LanRemotePeer_C* lp, ResultCode reason)
{
  LockMain_AssertIsLockedByCurrentThread();

  if (!lp->dispose_requested_ && !dispose_issued_remote_peers_map_.Contains(lp->host_id_)) {
    MessageOut reply;
    lf::Write(reply, MessageType::NotifyCSP2PDisconnected);
    lf::Write(reply, reason);
    lf::Write(reply, lp->host_id_);

    {
      CScopedLock2 to_server_tcp_guard(to_server_tcp_mutex_);
      to_server_tcp_->SendWhenReady(SendFragRefs(reply), TcpSendOption());
    }

    lp->dispose_requested_ = true;
  }
}

double LanClientImpl::GetReliablePingTimerInterval()
{
  // 연속 두세번 보낸 것이 일정 시간 안에 도착 안하면 사실상 막장이므로 이정도 주기면 OK.
  return settings_.default_timeout_sec * 0.3;
}

//NetThread는 종료안해주나??
//AcceptPeers는 클리어 안해주나??
void LanClientImpl::Cleanup()
{
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);
  {
    CScopedLock2 to_server_tcp_guard(to_server_tcp_mutex_);
    to_server_tcp_.Reset();
  }

  tcp_accept_cp_.Reset();
  tcp_listening_socket_.Reset();

  self_p2p_session_key_.Reset();
  self_encrypt_count_ = 0;
  self_decrypt_count_ = 0;
  suppress_subsequenct_disconnection_events_ = false;
  request_server_time_count_ = 0;
  dx_server_time_diff_ = 0;
  last_request_server_time_time_ = 0;
  p2p_groups_.Clear(); // 확인사살.
  total_tcp_recv_bytes_ = 0;
  total_tcp_send_bytes_ = 0;
  internal_version_ = NetConfig::InternalLanVersion;
  settings_ = NetSettings();
  server_instance_tag_ = Uuid::None;
  to_server_encrypt_count_ = 0;
  to_server_decrypt_count_ = 0;
  //min_extra_ping_ = 0;
  //extra_ping_variance_ = 0;
  local_host_id_ = HostId_None;
  connection_params_ = LanConnectionArgs();
  final_user_work_queue_.Clear();
  clock_.Stop();
  authed_remote_peers_.Clear();
  candidate_remote_peers_.Clear();
  dispose_issued_remote_peers_map_.Clear(); // connect, disconnect시에 이것을 clear하는게 맞는듯하다.
  CleanupPeerGet();
  last_tcp_stream_recv_time_ = 0;
  last_request_server_time_time_ = 0;
  local_nic_addr_ = "";
  user_task_is_running_ = false;
  server_instance_tag_ = Uuid::None;
  server_tcp_recent_ping_ = 0;
  server_tcp_last_ping_ = 0;
  //tear_down_ = false;
  suppress_subsequenct_disconnection_events_ = false;
  p2p_groups_.Clear();
  tcp_issue_send_ready_remote_peers_.Clear();
  lookback_final_recv_message_queue_.Clear();
  lookahead_p2p_send_queue_map_.Clear();

  //@todo 도대체 어디에서 삭제되는거지??

  // Clear accepted peer infos.
  //for (auto& pair : accepted_peers_) {
  //  delete pair.value;
  //}
  //accepted_peers_.Clear();

  //@maxidea: todo
  // 완료되지 않은 AcceptedPeer들을 안전하게 제거하는 코드를 넣어주어야 할듯함.

  // main unlock
  main_guard.Unlock();

  {
    CScopedLock2 pre_final_recv_queue_guard(pre_final_recv_queue_mutex_);
    pre_final_recv_queue_.Clear();
  }
}

void LanClientImpl::CleanupPeerGet()
{
  for (auto instance : remote_peer_instances_) {
    delete instance; // delete operator accept null
  }
  remote_peer_instances_.Clear();
}

void LanClientImpl::EveryClientDispose()
{
  LockMain_AssertIsLockedByCurrentThread();

  //모든 피어를 내보낸다.

  for (auto& pair : candidate_remote_peers_) {
    auto lp = pair.value;

    IssueDisposeRemotePeer( lp,
                            ResultCode::DisconnectFromLocal,
                            ResultCode::TCPConnectFailure,
                            ByteArray(),
                            __FUNCTION__,
                            SocketErrorCode::Ok,
                            false);
  }
  candidate_remote_peers_.Clear();

  for (auto& pair : authed_remote_peers_) {
    auto lp = pair.value;

    IssueDisposeRemotePeer( lp,
                            ResultCode::DisconnectFromLocal,
                            ResultCode::TCPConnectFailure,
                            ByteArray(),
                            __FUNCTION__,
                            SocketErrorCode::Ok,
                            false);
  }
  authed_remote_peers_.Clear();
}

void LanClientImpl::AddLookaheadP2PSendQueueMap(HostId peer_id, const ByteArray& data_to_send)
{
  LockMain_AssertIsLockedByCurrentThread();

  if (peer_id == GetLocalHostId() || peer_id == HostId_Server) {
    return;
  }

  if (auto found = lookahead_p2p_send_queue_map_.Find(peer_id)) {
    found->Append(data_to_send);
  }
  else {
    // 큐를 새로만들어 추가
    ByteArrayQueue send_queue_;
    send_queue_.Append(data_to_send);

    lookahead_p2p_send_queue_map_.Add(peer_id, send_queue_);
  }
}

void LanClientImpl::RemoveLookaheadP2PSendQueueMap(LanRemotePeer_C* lp)
{
  LockMain_AssertIsLockedByCurrentThread();

  if (lp) {
    //TODO TMap에서 Iterator로 찾는게 안되남??
    //Iterator말고 조회와 삭제가 동시에 되는 함수가 있으니 그걸 사용하자~!!!
    if (auto found = lookahead_p2p_send_queue_map_.Find(lp->GetHostId())) {
      found->Clear(); //TODO 구지 필요 없어보이는데...???
      lookahead_p2p_send_queue_map_.Remove(lp->GetHostId());
    }
  }
}

void LanClientImpl::RemoveAllLookaheadP2PSendQueueMap()
{
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  for (auto& pair : authed_remote_peers_) {
    auto lp = pair.value;
    RemoveLookaheadP2PSendQueueMap(lp);
  }
}

void LanClientImpl::SendLookaheadP2PMessage(LanRemotePeer_C* lp)
{
  LockMain_AssertIsLockedByCurrentThread();

  if (lp) {
    CScopedLock2 lp_tcp_guard(lp->tcp_transport_.GetSendQueueMutex());

    if (auto found = lookahead_p2p_send_queue_map_.Find(lp->GetHostId())) {
      const int32 dequeue_length = found->Count();
      for (int32 i = 0; i < dequeue_length; ++i) {
        // SendFragRefs로 변환하여 메세지 송신
        ByteArray data;
        found->PopFront(data);

        SendFragRefs data_to_send;
        data_to_send.Add(data);

        lp->tcp_transport_.SendWhenReady(data_to_send, TcpSendOption());

#ifdef TRACE_LOOKAHEADSEND
        //FUN_TRACE("LookaheadP2PSend!! local_host_id_ : %d, peer_id : %d\n", GetLocalHostId(), lp->GetHostId());
#endif
      }
    }
  }
}

ITaskSubject* LanClientImpl::GetTaskSubjectByHostId_NOLOCK(HostId subject_host_id)
{
  LockMain_AssertIsLockedByCurrentThread();
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (subject_host_id == local_host_id_) {
    return this;
  }

  auto subject = GetPeerByHostId_NOLOCK(subject_host_id);

   //아래의 경우에는 candidate도 Subject를 구해준다.
  if (subject == nullptr)// && settings_.look_ahead_p2p_send_enabled) //HostTag에서 candidate를 못빼서 문제가 생김...
  {
    if (auto candidate = candidate_remote_peers_.FindRef(subject_host_id)) {
      return candidate;
    }

    if (auto dispose = dispose_issued_remote_peers_map_.FindRef(subject_host_id)) {
      return dispose;
    }
  }

  return subject;
}

void LanClientImpl::AssociateSocket(InternalSocket* socket)
{
  net_thread_pool_->AssociateSocket(socket);
}

bool LanClientImpl::SetHostTag(HostId host_id, void* host_tag)
{
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (host_id == HostId_Server) {
    server_as_send_dest_.host_tag = host_tag;
    return true;
  }
  else if (auto subject = GetTaskSubjectByHostId_NOLOCK(host_id)) {
    subject->host_tag_ = host_tag;
    return true;
  }
  else {
    return false;
  }
}

int32 LanClientImpl::GetMessageMaxLength()
{
  return settings_.message_max_length;
}

void LanClientImpl::CheckCriticalSectionDeadLock_INTERNAL(const char* where)
{
  LockMain_AssertIsLockedByCurrentThread();

  for (auto lp : remote_peer_instances_) {
    if (lp->tcp_transport_.GetMutex().IsLockedByCurrentThread()) {
      LOG(LogNetEngine, Warning, "LanClient LanRemtePeer DeadLock! - %s", where);
    }
  }

  for (auto& pair : accepted_peers_) {
    auto accepted_info = pair.value;

    if (accepted_info->GetMutex().IsLockedByCurrentThread()) {
      LOG(LogNetEngine, Warning, "LanClient AcceptedInfo DeadLock! - %s", where);
    }
  }

  if (to_server_tcp_mutex_.IsLockedByCurrentThread()) {
    LOG(LogNetEngine, Warning, "LanClient to_server_tcp_mutex_ DeadLock! - %s", where);
  }

  if (tcp_issue_queue_mutex_.IsLockedByCurrentThread()) {
    LOG(LogNetEngine, Warning, "LanClient tcpissuequeuecs DeadLock! - %s", where);
  }
}

void LanClientImpl::OnSocketIoCompletion( Array<IHostObject*>& send_issued_pool,
                                          ReceivedMessageList& msg_list,
                                          CompletionStatus& completion)
{
  IoCompletion_ToServerTcp(completion, msg_list);
  IoCompletion_PerRemotePeer(completion, msg_list);
  IoCompletion_AcceptedInfo(completion, msg_list);
}

void LanClientImpl::IoCompletion_ToServerTcp( CompletionStatus& completion,
                                              ReceivedMessageList& received_msg_list)
{
  LockMain_AssertIsNotLockedByCurrentThread();
  to_server_tcp_mutex_.AssertIsNotLockedByCurrentThread();

  // CTcpLayer_C로 캐스팅이 성공하면 ToServer. CLanRemotePeer_C로 캐스팅이 성공하면 remote peer.
  auto tcp_transport = LeanDynamicCastTcpTransport_C(completion.completion_context);

  CScopedLock2 to_server_tcp_guard(to_server_tcp_mutex_);

  if (tcp_transport) {
    if (completion.type == CompletionType::Send) {
      tcp_transport->send_issued_ = false;

      // 송신의 경우 0바이트를 보내는 경우도 있으므로 <=가 아닌 < 비교이다.
      if (completion.completed_length < 0) {
        // 이상황이 두번올수가 있는걸까??

        to_server_tcp_guard.Unlock();

        CScopedLock2 main_guard(mutex_);
        CheckCriticalSectionDeadLock(__FUNCTION__);

        if (GetState() < ConnectionState::Disconnecting) {
          // 서버와의 TCP 연결이 끊어졌음을 의미한다. 따라서 에러 처리한다.
          EnqueueDisconnectionEvent(ResultCode::DisconnectFromRemote, ResultCode::TCPConnectFailure);

          SetState(ConnectionState::Disconnecting);

          if (intra_logger_) {
            const String text = String::Format("Send CompletedDatalength<=0.  host_id: %d, socket_error: %d", (int32)GetLocalHostId(), (int32)completion.socket_error);
            intra_logger_->WriteLine(LogCategory::System, *text);
          }
        }
      }
      else {
        // 보내기 완료
        if (completion.completed_length > 0) {
          // 송신 큐에서 완료된 만큼의 데이터를 제거한다. 그리고 다음 송신을 건다.
          // (최종 completion 상황이라 하더라도 이 과정은 필수!)
          // DequeueNoCopy 함수 이름은 Drain으로 바꿔주는게 좋을듯도...
          tcp_transport->send_queue_.DequeueNoCopy(completion.completed_length);

          // Update stats
          total_tcp_send_bytes_ += completion.completed_length;
        }

        // 이 루틴이 실행중일 때는 아직 owner가 건재함을 전제한다.
        //if (GetState() == ConnectionState::Connected) {
          tcp_transport->ConditionalIssueSend();
        //}
      }
    }
    else if (completion.type == CompletionType::Receive) {
      try {
        tcp_transport->recv_issued_ = false;

        if (completion.completed_length <= 0) {
          to_server_tcp_guard.Unlock();

          CScopedLock2 main_guard(mutex_);
          CheckCriticalSectionDeadLock(__FUNCTION__);

          if (GetState() < ConnectionState::Disconnecting) {
            // TCP 연결이 끊어졌음을 의미한다. 따라서 에러 처리한다.
            EnqueueDisconnectionEvent(ResultCode::DisconnectFromRemote, ResultCode::TCPConnectFailure);

            // disconnect
            SetState(ConnectionState::Disconnecting);

            if (intra_logger_) {
              const String text = String::Format("Recv CompletedDatalength<=0.  host_id: %d, socket_error: %d", (int32)GetLocalHostId(), (int32)completion.socket_error);
              intra_logger_->WriteLine(LogCategory::System, *text);
            }
          }
        }
        else {
          // 받기 완료.
          fun_check(to_server_tcp_.IsValid());

          // 타임스탬핑
          last_tcp_stream_recv_time_ = GetAbsoluteTime();

          // 수신 큐에서 받은 데이터를 꺼낸 후 ...
          tcp_transport->recv_stream_.EnqueueCopy(tcp_transport->socket_->GetRecvBufferPtr(), completion.completed_length);

          fun_check(to_server_tcp_.IsValid());

          // 통계
          total_tcp_recv_bytes_ += completion.completed_length;

          received_msg_list.Reset(); // keep capacity

          const ResultCode result_code = ExtractMessagesFromTcpStream(received_msg_list);

          to_server_tcp_guard.Unlock();

          if (result_code != ResultCode::Ok) {
            EnqueueError(ResultInfo::From(result_code, HostId_Server, "Received stream from TCP server became inconsistent.")); //InduceDisconnect();연결 해제 유도.
          }

          {
            CScopedLock2 pre_final_recv_queue_guard(pre_final_recv_queue_mutex_);

            for (auto& received_msg : received_msg_list) {
              // PreFinalRecvQueue는 따로 lock을 사용
              //received_msg.ActionName = owner_->GetAbsoluteTime() + owner_->GetSimLag();
              pre_final_recv_queue_.Enqueue(received_msg);
            }

            fun_check(to_server_tcp_.IsValid());
            ProcessEveryMessageOrMoveToFinalRecvQueue_ToServerTcp();
            fun_check(to_server_tcp_.IsValid());
          }
        }

        // 다음 recv를 건다.
        // 여기까지 왔을떄는 시스템이 파괴되지 않고 건재함을 전제해야 한다.
        if (GetState() < ConnectionState::Disconnecting) {
          CScopedLock2 to_server_tcp_guard(to_server_tcp_mutex_);

          const SocketErrorCode socket_error = to_server_tcp_->IssueRecvAndCheck();
          if (socket_error != SocketErrorCode::Ok) {
            const String text = String::Format("%d TCP IssueRecv failed socket_error", (int32)socket_error);
            EnqueueDisconnectionEvent(ResultCode::TCPConnectFailure, ResultCode::Unexpected);

            // disconnect.
            SetState(ConnectionState::Disconnecting);
          }
        }
      }
      catch (std::exception& e) {
        CatchThreadUnexpectedExit(__FUNCTION__, *String::Format("std.exception(%s)", (const char*)UTF8_TO_TCHAR((e.what()))));
      }
      //catch (_com_error& e) {
      //  CatchThreadUnexpectedExit(__FUNCTION__, *String::Format("_com_error(%s)", (const char*)e.Description()));
      //}
      //catch (void*) {
      //  CatchThreadUnexpectedExit(__FUNCTION__, "void*");
      //}
    }
    else if (completion.type == CompletionType::ConnectEx) { // 커넥트 완료.
      // 커넥트 완료 되었음
      const SocketErrorCode socket_error = tcp_transport->socket_->ConnectExComplete();
      if (completion.socket_error != SocketErrorCode::Ok) {
        to_server_tcp_guard.Unlock();

        // TCP 연결이 실패했음을 의미
        Heartbeat_ConnectFailCase(completion.socket_error);
      }
      else {
        if (socket_error != SocketErrorCode::Ok) {
          to_server_tcp_guard.Unlock();

          Heartbeat_ConnectFailCase(socket_error);
        }
        else {
          to_server_tcp_->IssueRecvAndCheck();
        }
      }
    }
    else {
      // main_guard.Unlock();
      ErrorReporter::Report("Unexpected at client IOCP TCP completion.");
    }
  }
}

void LanClientImpl::IoCompletion_PerRemotePeer( CompletionStatus& completion,
                                                ReceivedMessageList& received_msg_list)
{
  LockMain_AssertIsNotLockedByCurrentThread();

  LanRemotePeer_C* lp = nullptr;
  try {
    lp = LeanDynamicCastRemotePeer_C(completion.completion_context);
    if (lp) {
      if (completion.type == CompletionType::Receive) {
        IoCompletion_TcpRecvCompletionCase(completion, received_msg_list, lp);
      }
      else if (completion.type == CompletionType::Send) {
        IoCompletion_TcpSendCompletionCase(completion, lp);
      }
      else if (completion.type == CompletionType::ConnectEx) {
        IoCompletion_TcpConnectExCompletionCase(completion, lp);
      }
    }
  }
  catch (std::exception&) {
    CatchThreadExceptionAndPurgeClient(lp, __FUNCTION__, "std.exception");
  }
  //catch (_com_error&) {
  //  CatchThreadExceptionAndPurgeClient(lp, __FUNCTION__, "_com_error");
  //}
  //catch (void*) {
  //  CatchThreadExceptionAndPurgeClient(lp, __FUNCTION__, "void*");
  //}
}

/*
@maxidea

P2P Group을 만든 직후 접속이 끊어지는데, 그냥 귾어지는 것인지 서버에서 끊는건지
확인을 해봐야할 듯 싶다.
*/
void LanClientImpl::IoCompletion_TcpRecvCompletionCase( CompletionStatus& completion,
                                                        ReceivedMessageList& received_msg_list,
                                                        LanRemotePeer_C* lp)
{
  lp->tcp_transport_.AssertIsNotLockedByCurrentThread();
  LockMain_AssertIsNotLockedByCurrentThread();

  // lp lock 사용
  ScopedUseCounter counter(*lp);
  CScopedLock2 lp_guard(lp->tcp_transport_.GetMutex());

  lp->tcp_transport_.recv_issued_ = false;

  // TCP 스트림을 받은 시간을 키핑한다.
  lp->last_tcp_stream_recv_time_ = GetAbsoluteTime();

  if (completion.completed_length <= 0) {
    //const String comment = String::Format("%s:%d", __FUNCTION__, completion.socket_error);

    // TCP 연결이 끊어졌음을 의미한다. 따라서 에러 처리한다.
    // 서버에 노티후 컴플리션 또는 다른쪽에 저장해둔다.

    lp_guard.Unlock();

    CScopedLock2 main_guard(mutex_);
    CheckCriticalSectionDeadLock(__FUNCTION__);

    NotifyP2PDisconnected(lp, ResultCode::DisconnectFromRemote);

    //// TCP Socket에서 연결이 실패했으므로 연결 해제 처리를 한다.
    //owner_->IssueDisposeRemotePeer(lp, ResultCode::DisconnectFromRemote, ResultCode::TCPConnectFailure, ByteArray(), __FUNCTION__, completion.socket_error);
    //lp->WarnTooShortDisposal(__FUNCTION__);

    //if (owner_->intra_logger_) {
    //  const String text = String::Format("서버: 클라이언트 연결 해제.  host_id: %d, addr: %s, error_code: %d",
    //              lp->host_id_,
    //              *lp->tcp_transport_->remote_addr_.ToString(),
    //              completion.socket_error);
    //  owner_->intra_logger_->WriteLine(LogCategory::System, text);
    //}
  }
  else {
    //Console.WriteLine(String.Format("서버: {0}바이트 받았음.", completion.completed_length));

    //owner_->m_stats.m_totalTcpReceiveCount++;
    total_tcp_recv_bytes_ += completion.completed_length;

    // 수신 큐에서 받은 데이터를 꺼낸 후 ...
    lp->tcp_transport_.recv_stream_.EnqueueCopy(lp->tcp_transport_.socket_->GetRecvBufferPtr(), completion.completed_length);

    // 완전한 Msg가 도착한 것들을 모두 추려서 final recv queue로 옮기거나 여기서 처리한다.
    received_msg_list.Reset(); // keep capacity

    const ResultCode result_code = lp->ExtractMessagesFromTcpStream(received_msg_list);

#ifdef TRACE_LOOKAHEADSEND
    //FUN_TRACE("Receive lp %d->%d\n", (int)lp->GetHostId(), (int)owner_->GetLocalHostId());
#endif

    lp_guard.Unlock();

    if (result_code != ResultCode::Ok) {
      CScopedLock2 main_guard(mutex_);
      CheckCriticalSectionDeadLock(__FUNCTION__);

      EnqueueError(ResultInfo::From(ResultCode::TCPConnectFailure, lp->host_id_, "Extraction failed"));

      // 해당 클라와의 TCP 통신이 더 이상 불가능한 상황이다.
      NotifyP2PDisconnected(lp, result_code);
      return;
    }

    // 이구문은 lp lock 상태이면 안된다.. 내부적으로 MainLock이 걸리기 때문
    IoCompletion_ProcessMessageOrMoveToFinalRecvQueue(lp, received_msg_list);

    // 다시 RPLock을 건다.
    lp_guard.Lock();

    // 다음 recv를 건다.
    lp->tcp_transport_.IssueRecvAndCheck();
  }
}

void LanClientImpl::IoCompletion_TcpSendCompletionCase(CompletionStatus& completion, LanRemotePeer_C* lp)
{
  LockMain_AssertIsNotLockedByCurrentThread();

  ScopedUseCounter counter(*lp);
  CScopedLock2 lp_tcp_guard(lp->tcp_transport_.GetMutex());

  lp->tcp_transport_.send_issued_ = false;

  // 송신의 경우 0바이트를 보내는 경우도 있으므로 <=가 아닌 < 비교이다.
  if (completion.completed_length < 0) {
    // TCP 연결이 끊어졌음을 의미한다. 따라서 에러 처리한다.
    // 서버에 알린다.
    lp_tcp_guard.Unlock();

    CScopedLock2 main_guard(mutex_);
    CheckCriticalSectionDeadLock(__FUNCTION__);

    NotifyP2PDisconnected(lp, ResultCode::DisconnectFromRemote);
  }
  else {
    // 송신 큐에서 완료된 만큼의 데이터를 제거한다. 그리고 다음 송신을 건다.
    {
      CScopedLock2 lp_tcp_send_queue_guard(lp->tcp_transport_.GetSendQueueMutex());
      lp->tcp_transport_.send_queue_.DequeueNoCopy(completion.completed_length);
    }

    lp->tcp_transport_.ConditionalIssueSend(GetAbsoluteTime());

    //owner_->m_stats.m_totalTcpSendCount++;
    total_tcp_send_bytes_ += completion.completed_length;
  }
}

void LanClientImpl::IoCompletion_TcpConnectExCompletionCase(CompletionStatus& completion, LanRemotePeer_C* lp)
{
  LockMain_AssertIsNotLockedByCurrentThread();
  //여기서는 MainLock로 진행해도 무방할듯.그리 많은 connectex가 일어나지 않을테니까.
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  //connectex하기전에 increasecount 완료후 decrease
  lp->DecreaseUseCount();

  ScopedUseCounter counter(*lp);

  if (completion.socket_error != SocketErrorCode::Ok) {
    //RemotePeer에 연결실패.

    //Heartbeat_ConnectFailCase(completion.socket_error);

    NotifyP2PDisconnected(lp, ResultCode::TCPConnectFailure);

    EnqueuePeerDisconnectEvent(lp, ResultCode::TCPConnectFailure);
  }
  else {
    // 커넥트 완료 되었음

    CScopedLock2 lp_tcp_guard(lp->tcp_transport_.GetMutex());

    const SocketErrorCode result_code = lp->tcp_transport_.socket_->ConnectExComplete();
    if (result_code != SocketErrorCode::Ok) {
      // 연결 실패.
      NotifyP2PDisconnected(lp, ResultCode::TCPConnectFailure);

      EnqueuePeerDisconnectEvent(lp, ResultCode::TCPConnectFailure);
    }
    else {
      // 일단 검증관련 패킷을 날리도록 하자.
      MessageOut msg_to_send;
      lf::Write(msg_to_send, MessageType::NotifyConnectionPeerRequestData);
      lf::Write(msg_to_send, local_host_id_);
      lf::Write(msg_to_send, server_instance_tag_);
      lf::Write(msg_to_send, internal_version_);
      lf::Write(msg_to_send, lp->holepunch_tag_);

      main_guard.Unlock();

      {
        CScopedLock2 lp_tcp_send_queue_guard(lp->tcp_transport_.GetSendQueueMutex());
        lp->tcp_transport_.SendWhenReady(SendFragRefs(msg_to_send), TcpSendOption());
      }

      // Issue first receive.
      lp->tcp_transport_.IssueRecvAndCheck();
    }
  }
}

void LanClientImpl::IoCompletion_AcceptedInfo(CompletionStatus& completion,
                                              ReceivedMessageList& received_msg_list)
{
  LockMain_AssertIsNotLockedByCurrentThread();

  AcceptedInfo* accepted_info = nullptr;
  try {
    accepted_info = LeanDynamicCastAcceptedInfo(completion.completion_context);
    if (accepted_info) {
      ScopedUseCounter counter(*accepted_info);

      if (completion.type == CompletionType::ReferCustomValue) {
        IoCompletion_TcpPeerAcceptedCase(completion, accepted_info);
      }
      else if (completion.type == CompletionType::Receive) {
        IoCompletion_TcpRecvCompletionCase(completion, received_msg_list, accepted_info);
      }
    }
  }
  catch (std::exception&) {
    CatchThreadExceptionAndPurgeClient(nullptr, __FUNCTION__, "std.exception");
  }
  //catch (_com_error&) {
  //  CatchThreadExceptionAndPurgeClient(nullptr, __FUNCTION__, "_com_error");
  //}
  //catch (void*) {
  //  CatchThreadExceptionAndPurgeClient(nullptr, __FUNCTION__, "void*");
  //}
}

void LanClientImpl::IoCompletion_TcpPeerAcceptedCase( CompletionStatus& completion,
                                                      AcceptedInfo* accepted_info)
{
  LockMain_AssertIsNotLockedByCurrentThread();

  switch ((IocpCustomValue)completion.custom_value) {
  case IocpCustomValue::NewPeerAccepted:
    IoCompletion_NewAcceptedPeerCase(accepted_info);
    break;
  }
}

void LanClientImpl::IoCompletion_NewAcceptedPeerCase(AcceptedInfo* accepted_info)
{
  LockMain_AssertIsNotLockedByCurrentThread();

  CScopedLock2 accepted_info_guard(accepted_info->GetMutex());

  // First receive issue.
  accepted_info->IssueRecvAndCheck();

  // DecreaseUseCount하기전에 acceptInfo의 lock을 해제해야한다.
  accepted_info_guard.Unlock();

  // completion 완료되었으므로 acceptInfo 의 UseCount를 1내림
  accepted_info->DecreaseUseCount();
}

void LanClientImpl::IoCompletion_TcpRecvCompletionCase(
      CompletionStatus& completion,
      ReceivedMessageList& received_msg_list,
      AcceptedInfo* accepted_info)
{
  LockMain_AssertIsNotLockedByCurrentThread();

  // 수신의 경우 0바이트 수신했음 혹은 음수바이트 수신했음이면
  // 연결에 문제가 발생한 것이므로 디스해야 한다.
  if (completion.completed_length <= 0) {
    // 어차피 RemotePeer로 넘어가지 않은 AcceptedInfo는
    // 따로 지우기때문에 여기서 에러처리를 하지 않고 넘어간다.
  }
  else {
    ScopedUseCounter counter(*accepted_info);
    CScopedLock2 accepted_info_guard(accepted_info->GetMutex());

    //owner_->stats_.total_tcp_recv_count++;
    total_tcp_recv_bytes_ += completion.completed_length;

    // 수신 큐에서 받은 데이터를 꺼낸 후 ...
    accepted_info->recv_stream_.EnqueueCopy(accepted_info->socket_->GetRecvBufferPtr(), completion.completed_length);

    // 완전한 Msg가 도착한 것들을 모두 추려서 final recv queue로 옮기거나 여기서 처리한다.
    received_msg_list.Reset(); // keep capacity
    accepted_info->ExtractMessagesFromTcpStream(received_msg_list);

    accepted_info_guard.Unlock();

    // final recv queue로 옮기거나 여기서 처리한다.
    for (auto& received_msg : received_msg_list) {
      fun_check(received_msg.unsafe_message.AtBegin());
      ProcessMessage_EngineLayer(received_msg, accepted_info); // AccpetedInfo는 TaskSubject를 사용치 않는다.
    }

    accepted_info_guard.Lock();

    accepted_info->recv_issued_ = false;

    if (accepted_info->socket_.IsValid()) {
      // 여기서 Socket에 대해 Issue Recv를 하게되면 CTcpLayer_S의 m_issueRecv가 갱신되지 않는 문제가 있다.
      // 따라서 CAcceptInfo -> CRemotePeer_S로 이양이 된뒤에 IssueRecv하는걸로 수정되었음.
      // 다음 recv를 건다.소켓이 RemotePeer이 인양될수 있으므로 이렇게 처리.
      const SocketErrorCode socket_error = accepted_info->socket_->IssueRecv(NetConfig::tcp_issue_recv_length);
      if (socket_error != SocketErrorCode::Ok) {
        auto socket = accepted_info->socket_.Get();

        accepted_info_guard.Unlock();

        CScopedLock2 main_guard(mutex_);
        CheckCriticalSectionDeadLock(__FUNCTION__);

        EnqueueError(ResultInfo::From(ResultCode::Unexpected, GetLocalHostId(), "FATAL: Cannot new peer accept socket receive issue."));

        socket_->CloseSocketHandleOnly();

        if (intra_logger_) {
          const String text = String::Format("CloseSocketHandleOnly() called at %s", __FUNCTION__);
          intra_logger_->WriteLine(LogCategory::System, *text);
        }
      }
    }
  }
}

bool LanClientImpl::ProcessMessage_EngineLayer( ReceivedMessage& received_msg,
                                                AcceptedInfo* accepted_info)
{
  LockMain_AssertIsNotLockedByCurrentThread();

  auto& msg = received_msg.unsafe_message;
  const int32 saved_read_pos = msg.Tell();

  MessageType msg_type;
  if (!lf::Read(msg, msg_type)) {
    msg.Seek(saved_read_pos);
    return false;
  }

  bool msg_processed = false;
  switch (msg_type) {
  case MessageType::NotifyConnectionPeerRequestData:
    ProcessMessage_NotifyConnectionPeerRequestData(msg, accepted_info);
    msg_processed = true;
    break;
  }

  // 만약 잘못된 메시지가 도착한 것이면 이미 FunNetNet 계층에서
  // 처리한 것으로 간주하고/ 메시지를 폐기한다.
  // 그리고 예외 발생 이벤트를 던진다.
  // 단, C++ 예외를 발생시키지 말자.
  // 디버깅시 혼란도 생기며 fail over 처리에도 애매해진다.
  const int32 l1 = msg.GetLength();
  const int32 l2 = msg.Tell();

  // 암호화된 메시지는 별도 버퍼에서 복호화된 후 처리되므로

  if (msg_processed &&
      l1 != l2 &&
      msg_type != MessageType::Encrypted_Reliable &&
      msg_type != MessageType::Encrypted_Unreliable) {
    CScopedLock2 main_guard(mutex_);
    CheckCriticalSectionDeadLock(__FUNCTION__);

    //@todo 왜 두번하지??
    msg_processed = true;

    // 에러가 난시점의 Msg를 기록한다.
    EnqueueError(ResultInfo::From(ResultCode::InvalidPacketFormat, received_msg.remote_id, __FUNCTION__, msg.ToAllBytesCopy()));
  }

  //AssureMessageReadOkToEnd(msg_processed, msg_type, RI);

  if (!msg_processed) {
    msg.Seek(saved_read_pos);
    return false;
  }

  return true;
}

void LanClientImpl::ProcessMessage_NotifyConnectionPeerRequestData(MessageIn& msg, AcceptedInfo* accepted_info)
{
  //_tprintf("ProcessMessage_NotifyConnectionPeerRequestData()\n"); //@maxidea: debug: temp

  LockMain_AssertIsNotLockedByCurrentThread();

  HostId  msg_remote_host_id;
  Uuid    msg_server_instance_tag;
  uint32  msg_internal_version;
  Uuid    msg_signature;

  if (!lf::Reads(msg,  msg_remote_host_id, msg_server_instance_tag, msg_internal_version, msg_signature)) {
    //검증 실패를 노티한다.
    //SendP2PConnectFailed(ResultCode::TCPConnectFailure);
    //그냥 끊어 낸다.
    TRACE_SOURCE_LOCATION();
    return;
  }

  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  auto lp = candidate_remote_peers_.FindRef(msg_remote_host_id);
  if (!lp) {
    // 검증 할수 없다....
    //SendP2PConnectFailed(ResultCode::TCPConnectFailure);
    // 그냥 끊어 낸다.
    TRACE_SOURCE_LOCATION();
    return;
  }

  if (msg_server_instance_tag != server_instance_tag_ ||
      msg_internal_version != internal_version_ ||
      msg_signature != lp->holepunch_tag_) {
    // 같지 않으면 검증 실패..
    NotifyP2PDisconnected(lp, ResultCode::TCPConnectFailure);
    return;
  }

  CScopedLock2 lp_tcp_guard(lp->tcp_transport_.GetMutex());
  CScopedLock2 accepted_info_guard(accepted_info->GetMutex());

  // 검증이 성공
  // 우선 acceptedinfo의 정보를 RemotePeer로 옮긴다.
  // 일정시간 후 AcceptInfo 객체는 PurgeTooOldUnmatureAcceptedPeers()에서 제거됨.

  //lp->tcp_transport_.socket_.Reset(accepted_info->socket_.Get());
  //accepted_info->socket_.Reset();
  Swap(accepted_info->socket_, lp->tcp_transport_.socket_);

  // CompletionContext를 강제로 변경. (AcceptInfo -> RemotePeer)
  lp->tcp_transport_.socket_->ForceSetCompletionContext(lp);

  // authed에 새로 추가하기 전에 보내지 못했던 패킷들을 발송한다.
  if (settings_.look_ahead_p2p_send_enabled) {
    SendLookaheadP2PMessage(lp);
  }

  // 받은 내용이 있었다면, RecvStream에 넣어줌.
  if (accepted_info->recv_stream_.GetLength() > 0) {
    //TODO 어떤식으로 해야하는걸까????
    //어짜피 복사가 일어나므로, 별도의 메시지 클래스를 사용할 필요는 없음.
    //CMessage MoveStream;
    //MoveStream.UseExternalBuffer(accepted_info->recv_stream_.GetData(), accepted_info->recv_stream_.GetLength());
    //MoveStream.SetLength(accepted_info->recv_stream_.GetLength());
    //lp->tcp_transport_.recv_stream_.EnqueueCopy(SendFragRefs(MoveStream));

    SendFragRefs data;
    data.Add(accepted_info->recv_stream_.ConstData(), accepted_info->recv_stream_.GetLength());
    lp->tcp_transport_.recv_stream_.EnqueueCopy(data);
  }

  // authed로 변경....
  authed_remote_peers_.Add(msg_remote_host_id, lp);
  candidate_remote_peers_.Remove(msg_remote_host_id);

  // 연결 성공 이벤트를 알린다.
  EnqueuePeerConnectionEstablishEvent(lp);

  //lp_tcp_guard.Unlock();
  accepted_info_guard.Unlock();

  //@maxidea: AcceptedPeers에서 제거해주어야 할터???
  //for (auto it = accepted_peers_; it; ++it) {
  //  if (it->value == accepted_info) {
  //    delete it->value;
  //    it.RemoveCurrent();
  //    break;
  //  }
  //}

  main_guard.Unlock();

  //lp_tcp_guard.Lock();
  // 검증이 되었으므로 First issue를 건다.
  lp->tcp_transport_.IssueRecvAndCheck();
  lp_tcp_guard.Unlock();

  // 이제 검증한거 노티하자.
  MessageOut msg_to_send;
  lf::Write(msg_to_send, MessageType::NotifyConnectPeerRequestDataSucess);
  lf::Write(msg_to_send, lp->holepunch_tag_);

  {
    CScopedLock2 lp_tcp_send_queue_guard(lp->tcp_transport_.GetSendQueueMutex());
    lp->tcp_transport_.SendWhenReady(SendFragRefs(msg_to_send), TcpSendOption());
  }
}

void LanClientImpl::ProcessEveryMessageOrMoveToFinalRecvQueue_ToServerTcp()
{
  LockMain_AssertIsNotLockedByCurrentThread();
  pre_final_recv_queue_mutex_.AssertIsLockedByCurrentThread();

  // final recv queue로 옮기거나 여기서 처리한다.
  while (!pre_final_recv_queue_.IsEmpty()) {
    auto& received_msg = pre_final_recv_queue_.Front();

    // 시뮬 랙에 의해 당장 시행할 메시지가 아니면 그만 처리.
    // 일단 사용하지 않는다.
    //if (received_msg.action_time > owner_->GetAbsoluteTime()) {
    //  break;
    //}

    fun_check(received_msg.unsafe_message.AtBegin());
    ProcessMessage_EngineLayer(received_msg, this);

    pre_final_recv_queue_.RemoveFront();
  }
}

void LanClientImpl::IoCompletion_ProcessMessageOrMoveToFinalRecvQueue(
    LanRemotePeer_C* lp, ReceivedMessageList& extracted_msg_list)
{
  LockMain_AssertIsNotLockedByCurrentThread();

  // final recv queue로 옮기거나 여기서 처리한다.
  for (auto& msg : extracted_msg_list) {
    fun_check(msg.unsafe_message.AtBegin());
    ProcessMessage_EngineLayer(msg, lp);
  }
}

void LanClientImpl::CatchThreadExceptionAndPurgeClient(
    LanRemotePeer_C* lp, const char* where, const char* reason)
{
  if (lp) {
    if (callbacks_) {
      const String text = String::Format("%s에서 %s 이유로 클라이언트 %d를 추방합니다.", where, reason, (int32)lp->host_id_);
      EnqueueError(ResultInfo::From(ResultCode::Unexpected, lp->host_id_, text));
    }

    IssueDisposeRemotePeer(lp, ResultCode::Unexpected, ResultCode::TCPConnectFailure, ByteArray(), __FUNCTION__, SocketErrorCode::Ok);
  }
  else {
    if (callbacks_) {
      const String text = String::Format("%s에서 %s 오류가 발생했으나 클라이언트를 식별 불가.", where, reason);
      EnqueueError(ResultInfo::From(ResultCode::Unexpected, HostId_None, text));
    }
  }
}

void LanClientImpl::CatchThreadUnexpectedExit(const char* where, const char* reason)
{
  if (callbacks_) {
    LockMain_AssertIsNotLockedByCurrentThread();

    const String text = String::Format("(%s): Unexpected thread exit with (%s)", where, reason);
    SharedPtr<ResultInfo> result_info = ResultInfo::From(ResultCode::Unexpected, HostId_Server, text);
    ShowError_NOLOCK(result_info);
  }
}

void LanClientImpl::Heartbeat_ConnectFailCase(SocketErrorCode socket_error)
{
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  //@warning Disconnecting -> Disconnecting 두번 연달아 호출되는 경우가 있음.
  // 전반적으로 정리가 필요한 상황인건지??
  if (GetState() < ConnectionState::Disconnecting) {
    // CloseSocket 후 Disconnecting mode로 전환
    to_server_tcp_->socket_->SetBlockingMode(true);
    to_server_tcp_->socket_->SetVerboseFlag(true);

    // close, equeue notify event and stop thread
    EnqueueConnectFailEvent(ResultCode::TCPConnectFailure, socket_error);

    //@warning Send_BroadcastLayer에서 괜시리 경고만 나오는 경우가 있어서, 일단 막아둠.
    //local_host_id_ = HostId_None; // 서버와 연결이 해제됐으므로 초기화한다.

    SetState(ConnectionState::Disconnecting);

    if (intra_logger_) {
      intra_logger_->WriteLine("Heartbeat connectfailcase disconnectstate");
    }
  }
}

void LanClientImpl::PostUserTask()
{
  user_thread_pool_->PostCompletionStatus(this, (UINT_PTR)IocpCustomValue::DoUserTask);
}

void LanClientImpl::PostHeartbeatIssue()
{
  net_thread_pool_->PostCompletionStatus(this, (UINT_PTR)IocpCustomValue::Heartbeat);
}

void LanClientImpl::UpdateFixedSteppedTimeVars()
{
  CScopedLock2 main_guard(GetMutex());
  fun_check(!clock_.IsStopped());

  stepped_absolute_time_ = clock_.AbsoluteSeconds();
  stepped_elapsed_time_ = clock_.ElapsedSeconds();
}

void LanClientImpl::Heartbeat()
{
  UpdateFixedSteppedTimeVars();

  //TODO
  // 프로세스 종료중이면 아무것도 하지 않는다.
  //if (CThread::bDllProcessDetached_INTERNAL) {
  //  return;
  //}

  switch (GetState()) {
  case ConnectionState::Connected:
    Heartbeat_Connected();
    break;

  case ConnectionState::Disconnecting:
    Heartbeat_Disconnecting();
    break;

  case ConnectionState::Disconnected:
    // Do nothing
    break;

  default:
    break;
  }
}

//@todo 뭔가 처리가 이상한건지 맛이가는 경우가 많다...
//원래 소스코드를 확인해봐야 할까??
void LanClientImpl::Heartbeat_Disconnecting()
{
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  disconnecting_mode_heartbeat_count_++;

  // 첫 1회인 경우 모든 소켓을 닫는다.
  if (disconnecting_mode_heartbeat_count_ == 1) {
    // 더 이상 accept은 없다.
    // 이게 선결되지 않으면 클라를 쫓는 과정에서 신규 클라가 들어오니 즐

    // ConnectEx 가 Completion이 최소 1회는 이루어져야한다.
    if (tcp_listening_socket_.IsValid() &&
        tcp_listening_socket_->IsClosedOrClosing() == false &&
        tcp_listening_socket_->IsConnectExComplete() == true) {
      // 이게 먼저 죽어야 한다. 그래야, acceptex실패가 나오지 않는다.
      listener_thread_.Reset();

      tcp_listening_socket_->CloseSocketHandleOnly();

      if (intra_logger_) {
        const String text = String::Format("CloseSocketHandleOnly() called at %s (Listen)", __FUNCTION__);
        intra_logger_->WriteLine(LogCategory::System, *text);
      }
    }

    // TCP 소켓도 바로 끊어야 한다.
    if (to_server_tcp_.IsValid() && to_server_tcp_->socket_->IsConnectExComplete()) {
      to_server_tcp_->socket_->CloseSocketHandleOnly();

      if (intra_logger_) {
        const String text = String::Format("CloseSocketHandleOnly() called at %s (ToServer)", __FUNCTION__);
        intra_logger_->WriteLine(LogCategory::System, *text);
      }
    }

    EveryClientDispose();
  }

  //owner_->DoGarbageCollection();

  // 이게 없으면 disconnect가 매우 늦다...dispose를 처리해야 하는데...
  // heartbeatstate가 dis로 돌아서서 처리를 못하기 때문...
  DisposeIssuedRemotePeers();

  int32 unsafe_disconnect_reason = 0;

  // async io가 모두 끝난게 확인될 떄까지 disconnecting mode 지속.
  // 어차피 async io는 이 스레드 하나에서만 이슈하기 떄문에 이 스레드 issue 없는 상태인 것을
  // 확인하면 더 이상 async io가 진행중일 수 없다.
  bool safe_disconnect = true;
  if (remote_peer_instances_.Count() > 0 || remote_peer_garbages_.Count() > 0) {
    unsafe_disconnect_reason = 1;
    safe_disconnect = false;
  }
  else if (to_server_tcp_.IsValid() && to_server_tcp_->socket_.IsValid() &&
          (!to_server_tcp_->socket_->IsClosed() || to_server_tcp_->send_issued_ || to_server_tcp_->recv_issued_)) {
    unsafe_disconnect_reason = 3;
    safe_disconnect = false;
  }

  if (!IsFinalReceiveQueueEmpty()) {
    unsafe_disconnect_reason = 4;
    safe_disconnect = false;
  }

  if (safe_disconnect) {
    // 모든 컨텍스트를 초기화한다.

    if (!clock_.IsStopped()) {
      clock_.Stop();
    }

    // 여기서 ToServerTcp를 지우면 Completion발생시 뻑남..
    // owner_->to_server_tcp_.Reset();
    // fun_check(owner_->remote_peers_.Count() == 0);
    // FinalUserWorkItemList_RemoveReceivedMessagesOnly(); 이걸 하지 말자. 서버측에서 RPC직후추방시 클라에서는 수신RPC를 폴링해야하는데 그게 증발해버리니까. 대안을 찾자.
    // 이러면 disconnected 상황이 되어도 미수신 콜백을 유저가 폴링할 수 있다. 이래야 서버측에서 디스 직전 보낸 RPC를 클라가 처리할 수 있다.

    //@warning Send_BroadcastLayer에서 괜시리 경고만 나오는 경우가 있어서, 일단 막아둠.
    //local_host_id_ = HostId_None;

    // 모든 스레드 종료 지시 후 대기

    listener_thread_->should_stop_ = true;

    main_guard.Unlock();

    listener_thread_->StopListening();

    //tcp_listening_socket_.Reset(); //@maxidea

    main_guard.Lock();

    //@todo 이놈을 호출해줘야 하는데, Timer가 null이 되어서 문제가 되고 있음.
    //      왜일까?? 흐름상 문제가 있는것 같은데..
    //      차라리, 객체는 날리지 않는게 좋지 않을까??

    //@fixme
    // net_thread_pool_
    // UserThreadPool에 여전히 접근할 경우, 서버에서 접속을 강제로 끊을 경우 간혹 크래쉬가 발생함.

    //확인해보니 아래의 세 타이머가 여전히 동작중이므로, 타이밍에 따라서는
    //건드리면 안되는 부분을 건드리게 될수도 있음.
    //정리가 필요한 상황 같음.


    //----------------------------------------------------------------------
    //@maxidea: todo:
    //  이곳에서 ThreadPool이 종료된 상태인지???????????????????????????
    //  현재 종료가 안된건지 오류가 발생하고 있다..
    //----------------------------------------------------------------------

    //@maxidea:
    /*
    쓰레드풀 종료 루틴을 추가했음.  종료하지 않아서, Access Violation이 일어나는것으로 보여서..
    스레드 풀을 종료해버리니, 다시 재접속이 안되는 상황이 발생하네...
    이걸 어떤식으로 접근해서 수정해야 할지???
    */
    //if (!net_thread_external_use_)
    //{
    //  if (net_thread_pool_ == user_thread_pool_) {
    //    net_thread_pool_->UnregisterReferer(this);
    //    net_thread_pool_.Reset();
    //
    //    user_thread_pool_.Detach();
    //  }
    //  else {
    //    net_thread_pool_->UnregisterReferer(this);
    //    net_thread_pool_.Reset();
    //  }
    //}
    //else {
    //  net_thread_pool_.Detach();
    //}
    //
    //if (!user_thread_external_use_) {
    //  user_thread_pool_->UnregisterReferer(this);
    //  user_thread_pool_.Reset();
    //}
    //else {
    //  user_thread_pool_.Detach();
    //}

    timer_.Stop(true); // 완료시까지 대기후 종료.

    heartbeat_timer_id_ = 0;
    issue_send_on_need_timer_id_ = 0;
    tick_timer_id_ = 0;

    /*
    net_thread_pool_->PostCompletionStatus(this, IocpCustomValue::End);

    if (net_thread_pool_ != user_thread_pool_) {
      user_thread_pool_->PostCompletionStatus(this, IocpCustomValue::End);
    }

    for (int32 i = 0; i < 10000; ++i) {
      MemoryBarrier();

      if (net_thread_pool_unregisted_ && user_thread_pool_unregisted_) {
        break;
      }

      CPlatformProcess::Sleep(0.1f);
    }

    MemoryBarrier();
    if (!net_thread_pool_unregisted_ || !user_thread_pool_unregisted_) {
      // 10000회나 시도 했지만, 종료가 안되는 상황임.
      fun_check(0);
    }
    */


    /*
    //tcp_accept_cp_.Reset();
    //.Reset(); //@maxidea: 이걸 닫아줘야 ThreadPool에서 무한 대기를 하지 않을듯 싶은데??

    if (!net_thread_external_use_) {
      if (net_thread_pool_ == user_thread_pool_) {
        net_thread_pool_.Reset();
        user_thread_pool_.Detach();
      }
      else {
        OutputDebugString("+++net_thread_pool_.Reset()\n");
        net_thread_pool_.Reset();
        OutputDebugString("---net_thread_pool_.Reset()\n");
      }
    }
    else {
      net_thread_pool_.Detach();
    }

    if (!user_thread_external_use_) {
      user_thread_pool_.Reset();
    }
    else {
      user_thread_pool_.Detach();
    }
    */


    // Clear accepted peer infos.
    //for (auto& pair : accepted_peers_)
    //{
    //  delete pair.value;
    //}
    //accepted_peers_.Clear();

    Cleanup();

    SetState(ConnectionState::Disconnected);
  }

  // 종료 처리가 너무 오래걸렸음을, 경고 형태로 알려줌.
  if (safe_disconnect == false &&
      (GetAbsoluteTime() - disconnecting_mode_start_time_) > 5.0 &&
    disconnecting_mode_warned_ == false) {
    disconnecting_mode_warned_ = true;

    const String text = String::Format("Too long time elapsed since disconnecting mode.  unsafe_disconnect_reason: %d", unsafe_disconnect_reason);
    ErrorReporter::Report(*text);

    SharedPtr<ResultInfo> result_info = SharedPtr<ResultInfo>(new ResultInfo);
    result_info->result_code = ResultCode::DisconnectFromLocal;
    result_info->comment = text;
    EnqueueWarning(result_info);
  }
}

// 연결된 상태에서, 한번씩 호출되는 루틴.
void LanClientImpl::Heartbeat_Connected()
{
  // main lock은 이제 각자 함수에서 시행한다.
  //CScopedLock2 main_guard(owner_->GetMutex());

  const double elapsed_time = GetElapsedTime();

  if ((GetAbsoluteTime() - last_tcp_stream_recv_time_) > settings_.default_timeout_sec) {
    CScopedLock2 main_guard(mutex_);
    CheckCriticalSectionDeadLock(__FUNCTION__);

    EnqueueDisconnectionEvent(ResultCode::DisconnectFromLocal, ResultCode::ConnectServerTimeout);
    SetState(ConnectionState::Disconnecting);

    if (intra_logger_) {
      intra_logger_->WriteLine("timeout heartbeat disconnect");
    }
  }

  // Server에게 Ping요청을 한다.
  ConditionalRequestServerTime();

  // 피어에게 핑 요청.
  ConditionalSyncIndirectServerTime();

  // 서버에 Peerping report
  ConditionalReportLanP2PPeerPing();

  // 여기서 커넥션 이슈 이후 오랫동안 안오는 경우도 해야 할지 생각해보자.
  if (purge_too_old_unmature_peer_alarm_.TakeElapsedTime(elapsed_time)) {
    PurgeTooOldUnmaturePeer();
  }

  if (remove_tool_old_tcp_send_packet_queue_alarm_.TakeElapsedTime(elapsed_time)) {
    Tcp_LongTick();
  }

  if (dispose_issued_remote_peers_alarm_.TakeElapsedTime(elapsed_time)) {
    DisposeIssuedRemotePeers();
  }

  if (disconnect_remote_peer_on_timeout_alarm_.TakeElapsedTime(elapsed_time)) {
    DisconnectRemotePeerOnTimeout();
  }

  //@maxidea: debug: 테스트를 위해서 임시로 막음... 단지, 디버깅이 곤란한점을 해소하기 위해서 임시로 막은것
  //뿐이니 반듯이 원래대로 돌려주어야함!
  if (accepted_peer_dispost_alarm_.TakeElapsedTime(elapsed_time)) {
    PurgeTooOldUnmatureAcceptedPeers();
  }

  // 오래된 lookahead Message들을 삭제한다
  if (settings_.look_ahead_p2p_send_enabled) {
    if (remove_lookahead_message_alarm_.TakeElapsedTime(elapsed_time)) {
      RemoveAllLookaheadP2PSendQueueMap();
    }
  }

  // 내부적으로 actiontime 을 체크하여 PreFinal을 Final로 이동시켜 준다.
  //ProcessEveryMessageOrMoveToFinalRecvQueue_ToServerTcp();

  while (true) {
    bool anything_did = false;

    //TODO 오랜시간 대기를 할 수 있는건가???  시간 갱신을 이 루프 안에서 해야하는지????
    UpdateFixedSteppedTimeVars();

    const double absolute_time = GetAbsoluteTime();

    // 서버와의 연결 해제 시작 상황이 된 후 서버와 오랫동안 연결이 안끊어지면 강제 스레드 종료를 한다.
    if (shutdown_issued_time_ > 0 && (absolute_time - shutdown_issued_time_) > graceful_disconnect_timeout_) {
      CScopedLock2 main_guard(mutex_);
      CheckCriticalSectionDeadLock(__FUNCTION__);

      SetState(ConnectionState::Disconnecting);

      if (intra_logger_) {
        intra_logger_->WriteLine("Shutdown issued time in heartbeat");
      }
      return;
    }

    anything_did |= LoopbackRecvCompletionCase(); //@maxidea: 한번이라도 성공하면 계속 루푸를 돈다는 의미일까??
    if (GetState() == ConnectionState::Disconnecting) {
      return;
    }

    if (!anything_did) {
      return;
    }
  }
}

bool LanClientImpl::LoopbackRecvCompletionCase()
{
  LockMain_AssertIsNotLockedByCurrentThread();

  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (!lookback_final_recv_message_queue_.IsEmpty()) {
    ReceivedMessage received_msg;
    received_msg.remote_id = local_host_id_;
    received_msg.unsafe_message = lookback_final_recv_message_queue_.CutFront();
    received_msg.unsafe_message.Seek(0);

    // main_guard unlock
    main_guard.Unlock();

    // PreFinal lock
    CScopedLock2 pre_final_recv_queue_guard(pre_final_recv_queue_mutex_);

    pre_final_recv_queue_.Enqueue(received_msg);

    // final recv queue로 옮기거나 여기서 처리한다.
    while (!pre_final_recv_queue_.IsEmpty()) {
      auto& received_msg2 = pre_final_recv_queue_.Front();
      fun_check(received_msg2.unsafe_message.AtBegin());
      ProcessMessage_EngineLayer(received_msg2, this);
      pre_final_recv_queue_.RemoveFront();
    }

    return true;
  }

  return false;
}

bool LanClientImpl::ProcessMessage_EngineLayer(
    ReceivedMessage& received_msg, ITaskSubject* subject)
{
  LockMain_AssertIsNotLockedByCurrentThread();

  auto& msg = received_msg.unsafe_message;
  const int32 saved_read_pos = msg.Tell();

  MessageType msg_type;
  if (!lf::Read(msg, msg_type)) {
    msg.Seek(saved_read_pos);
    return false;
  }

  bool msg_processed = false;
  switch (msg_type) {
  case MessageType::NotifyServerConnectionHint:
    if (!IsFromRemoteClientPeer(received_msg)) {
      ProcessMessage_NotifyServerConnectionHint(msg);
    }
    else {
      msg.SeekToEnd(); // sink error
    }
    msg_processed = true;
    break;

  case MessageType::NotifyCSSessionKeySuccess:
    if (!IsFromRemoteClientPeer(received_msg)) {
      ProcessMessage_NotifyCSSessionKeySuccess(msg);
    }
    else {
      msg.SeekToEnd(); // sink error
    }
    msg_processed = true;
    break;

  case MessageType::NotifyProtocolVersionMismatch:
    if (!IsFromRemoteClientPeer(received_msg)) {
      ProcessMessage_NotifyProtocolVersionMismatch(msg);
    }
    else {
      msg.SeekToEnd(); // sink error
    }
    msg_processed = true;
    break;

  case MessageType::ReplyServerTime:
    if (!IsFromRemoteClientPeer(received_msg)) {
      ProcessMessage_ReplyServerTime(msg);
    }
    else {
      msg.SeekToEnd(); // sink error
    }
    msg_processed = true;
    break;

  case MessageType::NotifyServerDeniedConnection:
    if (!IsFromRemoteClientPeer(received_msg)) {
      ProcessMessage_NotifyServerDeniedConnection(msg);
    }
    else {
      msg.SeekToEnd(); // sink error
    }
    msg_processed = true;
    break;

  case MessageType::NotifyServerConnectSuccess:
    if (!IsFromRemoteClientPeer(received_msg)) {
      ProcessMessage_NotifyServerConnectSuccess(msg);
    }
    else {
      msg.SeekToEnd(); // sink error
    }
    msg_processed = true;
    break;

  case MessageType::NotifyConnectPeerRequestDataSucess:
    ProcessMessage_NotifyConnectPeerRequestSuccess(received_msg.remote_id, msg);
    msg_processed = true;
    break; //@todo 원래 break가 없었는데... 아래 루틴을 호출해주는게 맞는거였나???

  case MessageType::P2PIndirectServerTimeAndPing:
    ProcessMessage_P2PIndirectServerTimeAndPing(received_msg);
    msg_processed = true;
    break;

  case MessageType::P2PIndirectServerTimeAndPong:
    ProcessMessage_P2PIndirectServerTimeAndPong(received_msg);
    msg_processed = true;
    break;

  case MessageType::RPC:
    ProcessMessage_RPC(received_msg, subject, msg_processed);
    break;

  case MessageType::FreeformMessage:
    ProcessMessage_FreeformMessage(received_msg, subject, msg_processed);
    break;

  case MessageType::Encrypted_Reliable:
  case MessageType::Encrypted_Unreliable: {
      ReceivedMessage decrypted_received_msg;
      MessageIn decrypted_msg;
      if (DecryptMessage(msg_type, received_msg, decrypted_msg)) {
        decrypted_received_msg.unsafe_message = decrypted_msg;
        decrypted_received_msg.relayed = received_msg.relayed;
        decrypted_received_msg.remote_addr_udp_only = received_msg.remote_addr_udp_only;
        decrypted_received_msg.remote_id = received_msg.remote_id;
        msg_processed |= ProcessMessage_EngineLayer(decrypted_received_msg, subject);
      }
    }
    break;

  case MessageType::Compressed: {
      ReceivedMessage decompressed_received_msg;
      MessageIn decompressed_msg;
      if (DecompressMessage(received_msg, decompressed_msg)) {
        decompressed_received_msg.unsafe_message = decompressed_msg;
        decompressed_received_msg.relayed = received_msg.relayed;
        decompressed_received_msg.remote_addr_udp_only = received_msg.remote_addr_udp_only;
        decompressed_received_msg.remote_id = received_msg.remote_id;
        msg_processed |= ProcessMessage_EngineLayer(decompressed_received_msg, subject);
      }
      break;
    }

  default:
    //FUN_TRACE("type:%d %d->%d wrong type\n", (int32)msg_type, (int32)subject->GetHostId(), (int32)GetLocalHostId());
    break;
  }

  // 만약 잘못된 메시지가 도착한 것이면 이미 FunNetNet 계층에서 처리한 것으로 간주하고
  // 메시지를 폐기한다. 그리고 예외 발생 이벤트를 던진다.
  // 단, C++ 예외를 발생시키지 말자. 디버깅시 혼란도 생기며 fail over 처리에도 애매해진다.
  const int32 l1 = msg.GetLength();
  const int32 l2 = msg.Tell();

  // 암호화된 메시지는 별도 버퍼에서 복호화된 후 처리되므로
  if (msg_processed == true &&
      l1 != l2 &&
      msg_type != MessageType::Encrypted_Reliable &&
      msg_type != MessageType::Encrypted_Unreliable) {
    msg_processed = true;
    EnqueueError(ResultInfo::From(ResultCode::InvalidPacketFormat, received_msg.remote_id, __FUNCTION__, msg.ToAllBytesCopy())); //copy
  }

  //AssureMessageReadOkToEnd(msg_processed, msg_type, RI);

  if (!msg_processed) {
    msg.Seek(saved_read_pos);
    return false;
  }

  return true;
}

void LanClientImpl::ProcessMessage_NotifyServerConnectionHint(MessageIn& msg)
{
  LockMain_AssertIsNotLockedByCurrentThread();

  // public key, server UDP port 주소를 얻는다.
  bool intra_logging_on;
  if (!lf::Read(msg, intra_logging_on)) {
    EnqueueDisconnectionEvent(ResultCode::ProtocolVersionMismatch, ResultCode::TCPConnectFailure);
    SetState(ConnectionState::Disconnecting);
    return;
  }

  NetSettings msg_settings;
  if (!lf::Read(msg, msg_settings)) {
    EnqueueDisconnectionEvent(ResultCode::ProtocolVersionMismatch, ResultCode::TCPConnectFailure);
    SetState(ConnectionState::Disconnecting);
    return;
  }

  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  //@note Setting값들을 서버에서 받아옴. (서버의 설정을 사용)
  settings_ = msg_settings;

  ByteArray public_key_blob_;
  if (!lf::Read(msg, public_key_blob_)) {
    TRACE_SOURCE_LOCATION();

    EnqueueDisconnectionEvent(ResultCode::ProtocolVersionMismatch, ResultCode::TCPConnectFailure);
    SetState(ConnectionState::Disconnecting);
    return;
  }

  ByteArray strong_random_block;
  ByteArray weak_encrypt_random_block;

  if (!CryptoRSA::CreateRandomBlock(strong_random_block, settings_.strong_encrypted_message_key_length) ||
      !CryptoAES::ExpandFrom(self_p2p_session_key_.aes_key, (const uint8*)strong_random_block.ConstData(), settings_.strong_encrypted_message_key_length / 8) ||

      !CryptoRSA::CreateRandomBlock(weak_encrypt_random_block, settings_.weak_encrypted_message_key_length) ||
      !CryptoRC4::ExpandFrom(self_p2p_session_key_.rc4_key, (const uint8*)weak_encrypt_random_block.ConstData(), settings_.weak_encrypted_message_key_length / 8)) {
    EnqueueDisconnectionEvent(ResultCode::EncryptFail, ResultCode::TCPConnectFailure);

    if (callbacks_) {
      callbacks_->OnException(HostId_None, Exception("Failed to create session_key_"));
    }

    SetState(ConnectionState::Disconnecting);
    return;
  }
  {
    CScopedLock2 net_config_write_guard(NetConfig::GetWriteMutex());
    NetConfig::message_max_length = MathBase::Max<int32>(NetConfig::message_max_length, settings_.message_max_length); // 아예 전역 값도 수정하도록 한다.
  }

  //NamedInetAddress UdpServerAddr;
  //if (!msg.Read(UdpServerAddr)) {
  //  owner_->EnqueueDisconnectionEvent(ResultCode::ProtocolVersionMismatch, ResultCode::TCPConnectFailure);
  //  SetState(Disconnecting);
  //
  //  //XXAACheck(owner);
  //  return;
  //}

  if (!msg.AtEnd()) {
    TRACE_SOURCE_LOCATION();

    EnqueueDisconnectionEvent(ResultCode::ProtocolVersionMismatch, ResultCode::TCPConnectFailure);
    SetState(ConnectionState::Disconnecting);
    return;
  }

  // 서버 연결 시점 전에 이미 로그를 남겨야 하냐 여부도 갱신받아야 하므로.
  //owner_->intra_logging_on_ = intra_logging_on;

  // nagle 알고리즘을 켜거나 끈다
  to_server_tcp_->SetEnableNagleAlgorithm(settings_.bEnableNagleAlgorithm);

  //fun_check(owner_->to_server_udp_fallbackable_->server_addr_.IsUnicast());

  // 세션키를 만들어서 공개키로 암호화해서 보낸다.
  // AES Key는 공개키로 암호화하고 RC4 key 는 AES 키로 암호화한다.
  ByteArray encrypted_aes_key_blob;
  ByteArray encrypted_rc4_key;

  if (!CryptoRSA::CreateRandomBlock(strong_random_block, settings_.strong_encrypted_message_key_length) ||
      !CryptoAES::ExpandFrom(to_server_session_key_.aes_key, (const uint8*)strong_random_block.ConstData(), settings_.strong_encrypted_message_key_length / 8) ||
      !CryptoRSA::EncryptSessionKeyByPublicKey(encrypted_aes_key_blob, strong_random_block, public_key_blob_) ||

      !CryptoRSA::CreateRandomBlock(weak_encrypt_random_block, settings_.weak_encrypted_message_key_length) ||
      !CryptoRC4::ExpandFrom(to_server_session_key_.rc4_key, (const uint8*)weak_encrypt_random_block.ConstData(), settings_.weak_encrypted_message_key_length / 8) ||
      !CryptoAES::Encrypt(to_server_session_key_.aes_key, weak_encrypt_random_block, encrypted_rc4_key)) {
    EnqueueDisconnectionEvent(ResultCode::EncryptFail, ResultCode::TCPConnectFailure);

    if (callbacks_) {
      callbacks_->OnException(HostId_None, Exception("Failed to create session_key"));
    }

    SetState(ConnectionState::Disconnecting);
    return;
  }

  // main unlock
  main_guard.Unlock();

  // send session key, user data and protocol version via TCP, public-key encrypted
  MessageOut msg_to_send;
  lf::Write(msg_to_send, MessageType::NotifyCSEncryptedSessionKey);
  lf::Write(msg_to_send, encrypted_aes_key_blob);
  lf::Write(msg_to_send, encrypted_rc4_key);
  {
    CScopedLock2 to_server_tcp_guard(to_server_tcp_mutex_);
    to_server_tcp_->SendWhenReady(SendFragRefs(msg_to_send), TcpSendOption());
  }
}

void LanClientImpl::ProcessMessage_NotifyCSSessionKeySuccess(MessageIn& msg)
{
  LockMain_AssertIsNotLockedByCurrentThread();

  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  // 서버 연결 과정의 마지막 메시지를 쏜다.
  // send user data and protocol version via TCP
  MessageOut msg_to_send;
  lf::Write(msg_to_send, MessageType::NotifyServerConnectionRequestData);
  lf::Write(msg_to_send, connection_params_.user_data);
  lf::Write(msg_to_send, connection_params_.protocol_version);
  lf::Write(msg_to_send, internal_version_);
  //@maxidea: 서버에 접속할 P2P용 Listen 서버 접속 주소를 알려주어야함.  그래야 다른 Peer가 이 Peer에 접속할 수 있을테니..
  //TODO 캐싱된 값을 사용해야할까?  가끔 직접 구하면 주소값이 제대로 조회가 안되는 경우가 있는데.. (스트레스 테스트시)
  lf::Write(msg_to_send, tcp_listening_socket_->GetSockName());

  main_guard.Unlock();
  {
    CScopedLock2 to_server_tcp_guard(to_server_tcp_mutex_);
    to_server_tcp_->SendWhenReady(SendFragRefs(msg_to_send), TcpSendOption());
  }
}

void LanClientImpl::ProcessMessage_NotifyProtocolVersionMismatch(MessageIn& msg)
{
  LockMain_AssertIsNotLockedByCurrentThread();

  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  EnqueueConnectFailEvent(ResultCode::ProtocolVersionMismatch);

  SetState(ConnectionState::Disconnecting);
}

void LanClientImpl::ProcessMessage_ReplyServerTime(MessageIn& msg)
{
  double client_old_local_time;
  double server_local_time;
  if (!lf::Reads(msg,  client_old_local_time, server_local_time)) {
    return;
  }

  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  // 새로 server lag을 계산한 후 클라이언트와의 시간차도 구한다.
  const double absolute_time = clock_.AbsoluteSeconds();
  const double rtt = (absolute_time - client_old_local_time);
  const double lag = rtt / 2;
  server_tcp_last_ping_ = lag;

  if (server_tcp_recent_ping_ == 0) {
    server_tcp_recent_ping_ = lag;
  }
  else {
    server_tcp_recent_ping_ = MathBase::Lerp(server_tcp_recent_ping_, lag, NetConfig::log_linear_programming_factor); //! 급격하게 변하지 않도록 하는 완화 장치?
  }

  const double server_time = server_local_time + server_tcp_recent_ping_;
  dx_server_time_diff_ = absolute_time - server_time;

  // SynchronizeServerTime
  LocalEvent event(LocalEventType::SynchronizeServerTime);
  event.remote_id = HostId_Server;
  EnqueueLocalEvent(event);
}

void LanClientImpl::ProcessMessage_NotifyServerDeniedConnection(MessageIn& msg)
{
  LockMain_AssertIsNotLockedByCurrentThread();

  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  EnqueueConnectFailEvent(ResultCode::NotifyServerDeniedConnection);

  SetState(ConnectionState::Disconnecting);
}

void LanClientImpl::ProcessMessage_NotifyServerConnectSuccess(MessageIn& msg)
{
  LockMain_AssertIsNotLockedByCurrentThread();

  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  // set local host_id: connect established! but TCP fallback mode yet
  ByteArray msg_user_data;
  Uuid msg_server_instance_tag;
  HostId msg_local_host_id;
  InetAddress msg_local_addr_at_server;
  if (!lf::Reads(msg, msg_local_host_id, msg_server_instance_tag, msg_user_data, msg_local_addr_at_server)) {
    EnqueueConnectFailEvent(ResultCode::InvalidPacketFormat,
        ResultInfo::From(ResultCode::ProtocolVersionMismatch, HostId_Server, "Bad format in NotifyServerConnectSuccess"));
    SetState(ConnectionState::Disconnecting);
    return;
  }

  local_host_id_ = msg_local_host_id;

  fun_check(to_server_tcp_.IsValid());
  to_server_tcp_->local_addr_at_server_ = msg_local_addr_at_server;

  // Enqueue connect ok event
  LocalEvent event(LocalEventType::ConnectServerSuccess);
  event.user_data = msg_user_data;
  event.remote_id = HostId_Server;
  event.remote_addr = InetAddress(connection_params_.server_ip, connection_params_.server_port);
  EnqueueLocalEvent(event);

  server_instance_tag_ = msg_server_instance_tag;

  //TODO
  //if (viz_agent_) {
  //  CScopedLock2 viz_agent_guard(viz_agent_->mutex_);
  //  viz_agent_->c2s_proxy_.NotifyClient_ConnectionState(HostId_Server, GReliableSend_INTERNAL, GetServerConnectionState());
  //}

  if (intra_logger_) {
    intra_logger_->WriteLine(LogCategory::LP2P, *String::Format("서버와의 연결 성공.  host_id: %d", (int32)local_host_id_));
  }

  //@note 기본에는 ConnectEx가 성공하면(물리적으로 tcp 커넥션 연결됨) 연결됨으로 변경해주었는데,
  //      LocalHostId를 서버에서 받은 이후가 연결된 상황이라고 간주하는게 좋을듯하다.
  SetState(ConnectionState::Connected);
}

void LanClientImpl::ProcessMessage_NotifyConnectPeerRequestSuccess(HostId peer_id, MessageIn& msg)
{
  LockMain_AssertIsNotLockedByCurrentThread();

  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  auto lp = candidate_remote_peers_.FindRef(peer_id);
  if (!lp) {
    //검증 실패...
    //purge에서 걸릴것이므로...일단 넘어가자.
    //owner_->NotifyP2PConnectFailed(peer_id, ResultCode::TCPConnectFailure);
    return;
  }

  Uuid tag;
  if (!lf::Read(msg, tag) || tag != lp->holepunch_tag_) {
    if (!lp->dispose_requested_) {
      lp->dispose_requested_ = true;
      NotifyP2PDisconnected(lp, ResultCode::TCPConnectFailure);
      EnqueuePeerDisconnectEvent(lp, ResultCode::ProtocolVersionMismatch);
    }
    return;
  }

  // 검증 성공...

  // authed에 새로 추가하기 전에 보내지 못했던 패킷들을 발송한다.
  if (settings_.look_ahead_p2p_send_enabled) {
    SendLookaheadP2PMessage(lp);
  }

  // authed로 변경....
  authed_remote_peers_.Add(peer_id, lp);    //@todo 인증된 목록에 추가
  candidate_remote_peers_.Remove(peer_id);  //@todo 인증 대기 목록에서 제거

  // 연결 성공 이벤트를 알린다.
  EnqueuePeerConnectionEstablishEvent(lp);

  // main unlock
  main_guard.Unlock();

  // 이제 서버에 노티.
  MessageOut msg_to_send;
  lf::Write(msg_to_send, MessageType::NotifyCSConnectionPeerSuccess);
  lf::Write(msg_to_send, peer_id);
  {
    CScopedLock2 to_server_tcp_guard(to_server_tcp_mutex_);
    to_server_tcp_->SendWhenReady(SendFragRefs(msg_to_send), TcpSendOption());
  }
}

void LanClientImpl::ProcessMessage_P2PIndirectServerTimeAndPing(ReceivedMessage& received_msg)
{
  LockMain_AssertIsNotLockedByCurrentThread();

  auto& msg = received_msg.unsafe_message;

  double client_local_time;
  if (!lf::Read(msg, client_local_time)) {
    return;
  }

  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  auto lp = GetPeerByHostId_NOLOCK(received_msg.remote_id);
  if (lp == nullptr || lp->host_id_ == HostId_Server) {
    return;
  }

  main_guard.Unlock();

  // P2PIndirectServerTimeAndPong
  MessageOut reply;
  lf::Write(reply, MessageType::P2PIndirectServerTimeAndPong);
  lf::Write(reply, client_local_time);
  lf::Write(reply, GetServerTime());
  lf::Write(reply, server_tcp_recent_ping_);
  {
    CScopedLock2 tcp_send_queue_guard(lp->tcp_transport_.GetSendQueueMutex());
    lp->tcp_transport_.SendWhenReady(SendFragRefs(reply), TcpSendOption());
  }
}

void LanClientImpl::ProcessMessage_P2PIndirectServerTimeAndPong(ReceivedMessage& received_msg)
{
  auto& msg = received_msg.unsafe_message;

  double client_old_local_time;
  double server_local_time;
  double peer_to_server_tcp_ping;
  if (!lf::Reads(msg,  client_old_local_time, server_local_time, peer_to_server_tcp_ping)) {
    return;
  }

  peer_to_server_tcp_ping = MathBase::Max(peer_to_server_tcp_ping, 0.0);

  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  // 받아서 시간 계산한다.
  auto peer = GetPeerByHostId_NOLOCK(received_msg.remote_id);
  if (peer == nullptr || peer->host_id_ == HostId_Server) {
    return;
  }

  peer->peer_to_server_ping_ = peer_to_server_tcp_ping;

  const double absolute_time = clock_.AbsoluteSeconds();
  const double rtt = (absolute_time - client_old_local_time);
  const double lag = rtt / 2;

  peer->last_ping_ = lag;

  if (peer->recent_ping_ > 0) {
    peer->recent_ping_ = MathBase::Lerp(peer->recent_ping_, lag, NetConfig::log_linear_programming_factor);
  }
  else {
    peer->recent_ping_ = lag;
  }

  const double server_time = server_local_time + peer->recent_ping_;
  peer->indirect_server_time_diff_ = absolute_time - server_time;
}

void LanClientImpl::ProcessMessage_RPC(ReceivedMessage& received_msg, ITaskSubject* subject, bool& ref_msg_processed)
{
  LockMain_AssertIsNotLockedByCurrentThread();

  auto& payload = received_msg.unsafe_message;
  const int32 saved_read_pos = payload.Tell(); // 이 값은 MessageType::RPC 다음의 offset이다.

  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  // FunNetNet layer의 RPC이면 아래 구문에서 true가 리턴되고 결국 user thread로 넘어가지 않는다.
  // 따라서 아래처럼 하면 된다.

  // 1st
  ref_msg_processed |= s2c_stub_.ProcessReceivedMessage(received_msg, subject->host_tag_);

  // 2nd
  if (!ref_msg_processed) {
    payload.Seek(saved_read_pos);
    ref_msg_processed |= c2c_stub_.ProcessReceivedMessage(received_msg, subject->host_tag_);
  }

  // 3rd
  if (!ref_msg_processed) {
    // 유저 스레드에서 RPC 처리하도록 enque한다.
    payload.Seek(saved_read_pos);

    ReceivedMessage received_msg2;
    received_msg2.unsafe_message = payload; // share
    received_msg2.relayed = received_msg.relayed;
    received_msg2.remote_addr_udp_only = received_msg.remote_addr_udp_only;
    received_msg2.remote_id = received_msg.remote_id;

    if (subject == this) {
      final_user_work_queue_.Enqueue(FinalUserWorkItem_S(received_msg2.unsafe_message, FinalUserWorkItemType::RPC));
    }
    else {
      ((LanRemotePeer_C*)subject)->final_user_work_queue_.Enqueue(FinalUserWorkItem_S(received_msg2.unsafe_message, FinalUserWorkItemType::RPC));
    }

    user_task_queue_.AddTaskSubject(subject);
  }
}

void LanClientImpl::ProcessMessage_FreeformMessage(ReceivedMessage& received_msg, ITaskSubject* subject, bool& ref_msg_processed)
{
  LockMain_AssertIsNotLockedByCurrentThread();

  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  // 이거 치우거나 MainLock아래로 내리지 말것...no lock CReceivedMessage파괴되면서 refcount꼬임.
  {
    auto& payload = received_msg.unsafe_message;

    // 유저 스레드에서 사용자 정의 메시지를 처리하도록 enque한다.
    //TODO 복사를 하지 않아도 될터인데??
    ReceivedMessage received_msg2;
    received_msg2.unsafe_message = payload; // share
    received_msg2.relayed = received_msg.relayed;
    received_msg2.remote_addr_udp_only = received_msg.remote_addr_udp_only;
    received_msg2.remote_id = received_msg.remote_id;

    if (subject == this) {
      final_user_work_queue_.Enqueue(FinalUserWorkItem_S(received_msg2.unsafe_message, FinalUserWorkItemType::FreeformMessage));
    }
    else {
      ((LanRemotePeer_C*)subject)->final_user_work_queue_.Enqueue(FinalUserWorkItem_S(received_msg2.unsafe_message, FinalUserWorkItemType::FreeformMessage));
    }

    user_task_queue_.AddTaskSubject(subject);
  }
}

bool LanClientImpl::IsFromRemoteClientPeer(ReceivedMessage& received_msg)
{
  return  received_msg.remote_id != HostId_Server &&
          received_msg.remote_id != HostId_None;
}

void LanClientImpl::OnIoCompletion(Array<IHostObject*>& send_issued_pool, ReceivedMessageList& msg_list, CompletionStatus& completion)
{
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
    fun_check(0);
    break;
  }
}

void LanClientImpl::SetState(ConnectionState new_state)
{
  if (new_state > state_) { // 뒤 상태로는 이동할 수 없어야 한다.
    disconnecting_mode_heartbeat_count_ = 0; // 이것도 세팅해야!
    state_ = new_state;
    disconnecting_mode_start_time_ = GetAbsoluteTime(); //@todo Disconnecting이 아닐때도 불리는데 귀찮아서 그런걸까? 상관 없으려나??
    disconnecting_mode_warned_ = false;
  }
  else {
    //@todo 이전 상태로 돌아간다는건데... 의도한건지 어쩐건지 확인을 해봐야할듯함.

    // Disconnecting인데 Disconnecting하는건 또 머지??
    //_tprintf("SetState error: %d -> %d\n", (int32)state_, (int32)new_state);

    //fun_check(0);
  }
}

void LanClientImpl::PostEveryRemote_IssueSend()
{
  net_thread_pool_->PostCompletionStatus(this, (UINT_PTR)IocpCustomValue::SendEnqueued);
}

//이 후틴은 중첩해서 호출되지 않음.
void LanClientImpl::EveryRemote_IssueSendOnNeed(Array<IHostObject*>& pool)
{
  // 송신 큐 lock을 건다.
  CScopedLock2 tcp_issue_queue_guard(tcp_issue_queue_mutex_);

  int32 issue_count = 0;
  const int32 peers_count = tcp_issue_send_ready_remote_peers_.Count();

  pool.ResizeUninitialized(peers_count);

  auto peers_list = pool.MutableData();
  while (true) {
    auto lp = tcp_issue_send_ready_remote_peers_.Front();
    if (lp == nullptr) {
      break;
    }

#ifdef TRACE_LOOKAHEADSEND
    //FUN_TRACE("lp - %d->%d ConditionalIssueSend\n", (int)owner_->GetLocalHostId(), (int)lp->GetHostId());
#endif

    lp->IncreaseUseCount();
    peers_list[issue_count++] = lp;

    lp->UnlinkSelf();
  }

  const double absolute_time = GetAbsoluteTime();
  tcp_issue_queue_guard.Unlock();

  // Server에 대한 issue도 처리한다.
  ConditionalIssueSend_ToServerTcp();

  while (issue_count > 0) {
#if USE_PARALLEL_FOR
#if (_MSC_VER >= 1400)
#pragma omp parallel for
#endif
#endif
    for (int32 issue_index = 0; issue_index < issue_count; ++issue_index) {
      auto lp = peers_list[issue_index];

      CScopedLock2 lp_guard(lp->GetMutex(), false);

      if (issue_index != 0) {
        if (const bool lock_ok = lp_guard.TryLock()) {
          lp->IssueSend(absolute_time);
          lp_guard.Unlock();

          lp->Decrease();
          peers_list[issue_index] = peers_list[--issue_count];
        }
      }
      else {
        lp_guard.Lock();
        lp->IssueSend(absolute_time);
        lp_guard.Unlock();

        lp->Decrease();
        peers_list[issue_index] = peers_list[--issue_count];
      }
    }
  }
}

void LanClientImpl::PostOnTick()
{
  user_thread_pool_->PostCompletionStatus(this, (UINT_PTR)IocpCustomValue::OnTick);
}

void LanClientImpl::OnTick()
{
  if (callbacks_ /*&& !owner_->tear_down_*/) {
    callbacks_->OnTick(connection_params_.timer_callback_context);
  }
}

void LanClientImpl::DoUserTask()
{
  UserWorkerThreadCallbackContext context;
  FinalUserWorkItem item;

  bool running_state = false;
  do {
    void* host_tag = nullptr;
    {
      CScopedLock2 main_guard(mutex_);
      CheckCriticalSectionDeadLock(__FUNCTION__);

      running_state = user_task_queue_.PopAnyTaskNotRunningAndMarkAsRunning(item, &host_tag);
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
        UserWork_FinalReceiveFreeform(item, host_tag);
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

void LanClientImpl::UserWork_FinalReceiveRPC(FinalUserWorkItem& uwi, void* host_tag)
{
  LockMain_AssertIsNotLockedByCurrentThread();

  auto& msg_content = uwi.unsafe_message.unsafe_message;
  const int32 saved_read_pos = msg_content.Tell();

  if (saved_read_pos != 0) {
    EnqueueHackSuspectEvent(nullptr, __FUNCTION__, HackType::PacketRig);
  }

  bool processed = false;
  RpcId rpc_id = RpcId_None;

  if (lf::Read(msg_content, rpc_id)) {
    // 각 stub에 대한 처리를 수행한다.
    const int32 stub_count = stubs_nolock_.Count();
    for (int32 stub_index = 0; stub_index < stub_count; ++stub_index) {
      auto stub = stubs_nolock_[stub_index];

      //@note 여러개의 Stub에서 접근할 것이므로, Stub에서 접근할때마다 원래의 위치로 옮겨주어야함.
      msg_content.Seek(saved_read_pos);

      // 이렇게 해줘야 막판에 task running flag를 수정할 수 있으니까 try-catch 구문이 있는거다.
      try {
        //if (!owner_->tear_down_) {
          processed |= stub->ProcessReceivedMessage(uwi.unsafe_message, host_tag);
        //}
      }
      catch (Exception& e) {
        if (callbacks_ /*&& !owner_->tear_down_*/) {
          callbacks_->OnException(uwi.unsafe_message.remote_id, e);
        }
      }
      catch (std::exception& e) {
        if (callbacks_ /*&& !owner_->tear_down_*/) {
          callbacks_->OnException(uwi.unsafe_message.remote_id, Exception(e));
        }
      }
      //catch (_com_error& e)
      //{
      //  if (callbacks_/* && !owner_->tear_down_*/) {
      //    callbacks_->OnException(uwi.unsafe_message.remote_id, Exception(e));
      //  }
      //}
      //catch (void* e) {
      //  if (callbacks_/*&& !owner_->tear_down_*/) {
      //    callbacks_->OnException(uwi.unsafe_message.remote_id, Exception(e));
      //  }
      //}
      //catch (...) {
      //  if (owner_->callbacks_ /*&& !owner_->tear_down_*/) {
      //    Exception e;
      //    e.exception_type = ExceptionType::Unhandled;
      //    owner_->callbacks_->OnException(uwi.unsafe_message.remote_id, e);
      //  }
      //}
    }

    if (!processed) {
      msg_content.Seek(saved_read_pos);

      if (callbacks_ /*&& !owner_->tear_down_*/) {
        callbacks_->OnNoRpcProcessed(rpc_id);
      }
    }

    user_task_queue_.SetTaskRunningFlagByHostId(uwi.unsafe_message.remote_id, false);
  }
}

void LanClientImpl::UserWork_FinalReceiveFreeform(FinalUserWorkItem& uwi, void* host_tag)
{
  LockMain_AssertIsNotLockedByCurrentThread();

  auto& msg_content = uwi.unsafe_message.unsafe_message;
  const int32 saved_read_pos = msg_content.Tell();

  if (saved_read_pos != 0) {
    EnqueueHackSuspectEvent(nullptr, __FUNCTION__, HackType::PacketRig);
  }

  // 이렇게 해줘야 막판에 task running flag를 수정할 수 있으니까 try-catch 구문이 있는거다.
  if (callbacks_/* && !owner_->tear_down_*/) {
    RpcHint rpc_hint;
    rpc_hint.relayed = uwi.unsafe_message.relayed;
    rpc_hint.host_tag = host_tag;

    try {
      OptimalCounter32 payload_length;
      if (!lf::Read(msg_content, payload_length) || payload_length != msg_content.GetReadableLength()) {
        SharedPtr<ResultInfo> result_info(new ResultInfo);
        result_info->result_code = ResultCode::InvalidPacketFormat;
        result_info->comment = "Invalid payload size in user message.";
        EnqueueError(result_info);
      }
      else {
        //TODO Payload를 CByteString으로 보내주어도 문제 없을터인데??
        callbacks_->OnReceiveFreeform(uwi.unsafe_message.remote_id, rpc_hint, msg_content.ToReadableBytesCopy()); //copy
      }
    }
    catch (Exception& e) {
      if (callbacks_/* && !owner_->tear_down_*/) {
        callbacks_->OnException(uwi.unsafe_message.remote_id, e);
      }
    }
    catch (std::exception& e) {
      if (callbacks_/* && !owner_->tear_down_*/) {
        callbacks_->OnException(uwi.unsafe_message.remote_id, Exception(e));
      }
    }
    //catch (_com_error& e) {
    //  if (callbacks_ /*&& !owner_->tear_down_*/) {
    //    callbacks_->OnException(uwi.unsafe_message.remote_id, Exception(e));
    //  }
    //}
    //catch (void* e) {
    //  if (callbacks_/*&& !owner_->tear_down_*/) {
    //    callbacks_->OnException(uwi.unsafe_message.remote_id, Exception(e));
    //  }
    //}
    //catch (...) {
    //  if (owner_->callbacks_/* && !owner_->tear_down_*/) {
    //    Exception e;
    //    e.exception_type = ExceptionType::Unhandled;
    //    owner_->callbacks_->OnException(uwi.unsafe_message.remote_id, e);
    //  }
    //}
  }

  user_task_queue_.SetTaskRunningFlagByHostId(uwi.unsafe_message.remote_id, false);
}

void LanClientImpl::UserWork_FinalUserTask(FinalUserWorkItem& uwi, void* host_tag)
{
  LockMain_AssertIsNotLockedByCurrentThread();

  // 이렇게 해줘야 막판에 task running flag를 수정할 수 있으니까 try-catch 구문이 있는거다.
  if (callbacks_/* && !owner_->tear_down_*/) {
    try {
      uwi.func();
    }
    catch (Exception& e) {
      if (callbacks_/* && !owner_->tear_down_*/) {
        callbacks_->OnException(uwi.unsafe_message.remote_id, e);
      }
    }
    catch (std::exception& e) {
      if (callbacks_/* && !owner_->tear_down_*/) {
        callbacks_->OnException(uwi.unsafe_message.remote_id, Exception(e));
      }
    }
    //catch (_com_error& e)
    //{
    //  if (callbacks_ /*&& !owner_->tear_down_*/) {
    //    callbacks_->OnException(uwi.unsafe_message.remote_id, Exception(e));
    //  }
    //}
    //catch (void* e) {
    //  if (callbacks_/*&& !owner_->tear_down_*/) {
    //    callbacks_->OnException(uwi.unsafe_message.remote_id, Exception(e));
    //  }
    //}
    //catch (...) {
    //  if (owner_->callbacks_/* && !owner_->tear_down_*/) {
    //    Exception e;
    //    e.exception_type = ExceptionType::Unhandled;
    //    owner_->callbacks_->OnException(uwi.unsafe_message.remote_id, e);
    //  }
    //}
  }

  user_task_queue_.SetTaskRunningFlagByHostId(uwi.unsafe_message.remote_id, false);
}

void LanClientImpl::UserWork_LocalEvent(FinalUserWorkItem& uwi)
{
  ProcessOneLocalEvent(uwi.event);

  user_task_queue_.SetTaskRunningFlagByHostId(uwi.unsafe_message.remote_id, false);
}

void LanClientImpl::EndCompletion()
{
  if (net_thread_pool_->IsCurrentThread()) {
    net_thread_pool_->UnregisterReferer(this);
    net_thread_pool_unregisted_ = true;
  }

  if (user_thread_pool_->IsCurrentThread()) {
    user_thread_pool_->UnregisterReferer(this);
    user_thread_pool_unregisted_ = true;
  }
}

void LanClientImpl::OnThreadBegin()
{
  if (callbacks_) {
    callbacks_->OnUserWorkerThreadBegin();
  }
}

void LanClientImpl::OnThreadEnd()
{
  if (callbacks_) {
    callbacks_->OnUserWorkerThreadEnd();
  }
}

ISocketIoCompletionDelegate* LanClientImpl::GetIoCompletionDelegate()
{
  return this;
}

void LanClientImpl::EnableIntraLogging(const char* filename)
{
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (!intra_logger_) {
    intra_logger_.Reset(LogWriter::New(filename));
  }
}

bool LanClientImpl::RunAsync(HostId task_owner_id, Function<void()> func)
{
  CScopedLock2 main_guard(mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (auto peer = GetPeerByHostId_NOLOCK(task_owner_id)) {
    peer->EnqueueUserTask(func);
    user_task_queue_.AddTaskSubject(peer);
    return true;
  }
  else {
    if (listener_thread_) {
      final_user_work_queue_.Enqueue(func);
      user_task_queue_.AddTaskSubject(this);
      return true;
    }
  }

  return false;
}

LanClient* LanClient::New()
{
  return new LanClientImpl();
}

} // namespace net
} // namespace fun
