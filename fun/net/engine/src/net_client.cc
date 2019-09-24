//TODO 코드정리
#include "fun/net/net.h"

#include "NetClient.h"
#include "Networker_C.h"

#include "UdpSocket_C.h"

#include "ReportError.h"

//TODO
//#include "Apps/viz_agent_.h"
//TODO
//#include "Apps/EmergencyLogClient.h"

#include "GeneratedRPCs/net_NetS2C_stub.cc"
#include "GeneratedRPCs/net_NetC2C_stub.cc"
#include "GeneratedRPCs/net_NetC2S_proxy.cc"
#include "GeneratedRPCs/net_NetC2C_proxy.cc"

#pragma warning(disable:4996) // warning C4996 : 'GetVersionExW' : deprecated로 선언되었습니다.

namespace fun {
namespace net {

using lf = LiteFormat;

//@todo localization
const char* NoServerConnectionErrorText = "cannot send messages unless connection to server exists.";
extern ByteArray POLICY_FILE_TEXT;


//
// DisconnectArgs
//

const DisconnectArgs DisconnectArgs::Default;

DisconnectArgs::DisconnectArgs()
  : graceful_disconnect_timeout_msec((int64)(NetConfig::default_graceful_disconnect_timeout_sec * 1000)), //@maxidea: todo: adjust time values
    disconnect_sleep_interval_msec(10), //@maxidea: todo: CNetConfig로 빼주는게 좋을듯..
    comment() {}


//
// NetClientImpl
//

IMPLEMENT_RPCSTUB_NetS2C_P2PGroup_MemberJoin(NetClientImpl::S2CStub) {
  CScopedLock2 main_guard(owner->GetMutex());

  //TODO : real_udp_enabled 파라메터를 추가 검토.
  bool real_udp_enabled = false;

  // P2P group에 새로운 member가 들어온것을 update한다.
  owner->UpdateP2PGroup_MemberJoin( group_id,
                                    member_id,
                                    custom_field,
                                    event_id,
                                    p2p_first_frame_number,
                                    connection_tag,
                                    p2p_aes_session_key,
                                    p2p_rc4_session_key,
                                    direct_p2p_enabled,
                                    bind_port,
                                    real_udp_enabled);
  return true;
}

IMPLEMENT_RPCSTUB_NetS2C_P2PGroup_MemberJoin_Unencrypted(NetClientImpl::S2CStub) {
  CScopedLock2 main_guard(owner->GetMutex());

  //TODO : real_udp_enabled 파라메터를 추가 검토.
  bool real_udp_enabled = false;

  // Update the new member in the P2P group.
  owner->UpdateP2PGroup_MemberJoin( group_id,
                                    member_id,
                                    custom_field,
                                    event_id,
                                    p2p_first_frame_number,
                                    connection_tag,
                                    ByteArray(),
                                    ByteArray(),
                                    direct_p2p_enabled,
                                    bind_port,
                                    real_udp_enabled);
  return true;
}

/*
TODO

IMPLEMENT_RPCSTUB_NetS2C_RemotePeer_RealUdpEnabledChanged(NetClientImpl::S2CStub) {
  RemotePeer_C* peer = owner->GetPeerByHostId(remote_peer_id);
  if (peer == nullptr || peer->garbaged_) {
    return true;
  }

  peer->real_udp_enabled_ = real_udp_enabled;
  return true;
}
*/

IMPLEMENT_RPCSTUB_NetS2C_RequestP2PHolepunch(NetClientImpl::S2CStub) {
  CScopedLock2 main_guard(owner->GetMutex());

  auto peer = owner->GetPeerByHostId(peer_id);
  if (!peer || peer->garbaged_) {
    return true;
  }

  // If the P2P hole punching attempt is not in itself, progress is no longer possible.
  if (!peer->p2p_holepunch_attempt_context_) {
    return true;
  }

  // Put the peer's address.
  fun_check(external_addr.IsUnicast());
  fun_check(internal_addr.IsUnicast());
  peer->udp_addr_from_server_ = external_addr;
  peer->udp_addr_internal_ = internal_addr;

  // Start a bilateral hole punching transition.
  if (peer->p2p_holepunch_attempt_context_->State.IsValid() == false ||
      peer->p2p_holepunch_attempt_context_->State->Type != P2PHolepunchAttemptContext::EStateType::PeerHolepunch) {
    auto new_state = new P2PHolepunchAttemptContext::CPeerHolepunchState();
    new_state->shotgun_min_port_ = external_addr.GetPort();

    peer->p2p_holepunch_attempt_context_->State.Reset(new_state);
  }

  return true;
}

/*
홀펀칭이 완료 되었을때 호출됨.
*/
IMPLEMENT_RPCSTUB_NetS2C_NotifyDirectP2PEstablish(NetClientImpl::S2CStub) {
  HostId A = A0;
  HostId B = B0;

  InetAddress a2b_send_addr = X0;
  InetAddress a2b_recv_addr = Y0;
  InetAddress b2a_send_addr = Z0;
  InetAddress b2a_recv_addr = W0;

  // Establish virtual UDP connection between A and B
  //
  // Status;
  //  A can send to B via X
  //  B can send to A via Z
  //  A can choose B's message if it is received from W
  //  B can choose A's messages if it is received from Y
    //
  //  Swap A-B / X-Z / W-Y if local is B */
  {
    CScopedLock2 main_guard(owner->GetMutex());

    fun_check(owner->local_host_id_ == A || owner->local_host_id_ == B);
    if (owner->local_host_id_ == B) {
      Swap(A, B);
      Swap(a2b_send_addr, b2a_send_addr);
      Swap(b2a_recv_addr, a2b_recv_addr);
    }

    auto peer = owner->GetPeerByHostId(B);
    if (!peer || peer->garbaged_) {
      return true;
    }

    peer->p2p_holepunched_local_to_remote_addr_ = a2b_send_addr;
    peer->p2p_holepunched_remote_to_local_addr_ = b2a_recv_addr;

    peer->SetDirectP2P();

    // 연결 시도중이던 객체를 파괴한다.
    // (이미 이쪽에서 저쪽으로 연결 시도가 성공해서 이미 trial context를 지운 상태일 수 있지만 무관함)
    // Destroy the object that was trying to connect.
    // (it is possible that the trial context has already been cleared because the connection attempt has already been made from this side to the other side, but this is irrelevant)
    peer->p2p_holepunch_attempt_context_.Reset(); // 홀펀칭 중지.

    // Enqueue event
    LocalEvent LocalEvent(LocalEventType::DirectP2PEnabled);
    LocalEvent.remote_id = B;
    owner->EnqueueLocalEvent(LocalEvent);

    // FunNet.RemotePeer_C.m_P2PholepunchedLocalToRemoteAddr나 FunNet.RemotePeer_C.m_P2PholepunchedRemoteToLocalAddr가
    // 업데이트되되 이 값이 UDP local socket의 FunNet.UdpSocket_C.AddrOfHereAtServer와 같으면
    // WAN 여러 포트가 아닌 이상 문제가 있음을 의미한다. 이런 경우에도 로그를 남기게 만들자.

    //int32 ErrorCase = 0;
    //InetAddress ErrorAddr;
    //
    //if (peer->p2p_holepunched_local_to_remote_addr_.BinaryAddress != 0 && !peer->p2p_holepunched_local_to_remote_addr_.IsUnicast()) {
    //  ErrorCase |= 1;
    //  ErrorAddr = peer->p2p_holepunched_local_to_remote_addr_;
    //}
    //if (peer->p2p_holepunched_remote_to_local_addr_.BinaryAddress != 0 && !peer->p2p_holepunched_remote_to_local_addr_.IsUnicast()) {
    //  ErrorCase |= 2;
    //  ErrorAddr = peer->p2p_holepunched_remote_to_local_addr_;
    //} else if (X0.BinaryAddress != Y0.BinaryAddress) {
    //  if (peer->p2p_holepunched_local_to_remote_addr_.BinaryAddress == peer->udp_socket_->here_addr_at_server_.BinaryAddress) {
    //    ErrorAddr = peer->p2p_holepunched_local_to_remote_addr_;
    //    ErrorCase |= 4;
    //  }
    //  if (peer->p2p_holepunched_remote_to_local_addr_.BinaryAddress == peer->udp_socket_->here_addr_at_server_.BinaryAddress) {
    //    ErrorAddr = peer->p2p_holepunched_remote_to_local_addr_;
    //    ErrorCase |= 8;
    //  }
    //}

    // TODO: 잘 잡히지도 않으므로 일단 임시로 막았다

    //if (ErrorCase) {
    //  const String text = String::Format("ProblemAtFinalHolepunchPhase##ErrorCase=%d##ErrorAddr=%s", ErrorCase, *ErrorAddr.ToString());
    //  ErrorReporter::Report(text);
    //}

    //@maxidea: debugging
    //_tprintf("[%s] remote=%d, p2p_holepunched_remote_to_local_addr_=%s\n", "P2PHolePunched", B, *peer->p2p_holepunched_remote_to_local_addr_.ToString());
  }

  return true;
}

IMPLEMENT_RPCSTUB_NetS2C_P2PGroup_MemberLeave(NetClientImpl::S2CStub) {
  CScopedLock2 main_guard(owner->GetMutex());

  if (owner->IsIntraLoggingOn()) {
    const String text = *String::Format("Received P2PGroup_MemberLeave: remote_peer: %d, group: %d", (int32)member_id, (int32)group_id);
    owner->IntraLogToServer(LogCategory::PP2P, *text);
  }

  //TODO group 및 Member가 null일 경우에 의미가 있는걸까?
  //단순히 나간놈의 ID만이라도 알려주어야하나??

  auto member_rp = owner->GetPeerByHostId(member_id);
  auto group = owner->GetP2PGroupByHostId_INTERNAL(group_id);

  // local host에 대한 이벤트도 받아야하므로, 여기서 바로 나가면 무시하면 안됨.
  //if (!group || !member_rp) {
  //  return true;
  //}

  if (group) {
    // Even if you are yourself, you should remove it from the group object.
    group->members_.Remove(member_id);
  }

  if (member_rp) {
    member_rp->joined_p2p_groups_.Remove(group_id);
    owner->RemoveRemotePeerIfNoGroupRelationDetected(member_rp);
    member_rp->leave_event_count++;
  }

  if (member_id == owner->local_host_id_) { // local
    owner->p2p_groups_.Remove(group_id);
  }

  // 'P2PLeaveMember' event
  LocalEvent event(LocalEventType::P2PLeaveMember);
  event.member_id = member_id;
  event.remote_id = member_id;
  event.group_id = group_id;
  event.member_count = group ? group->members_.Count() : 0;
  owner->EnqueueLocalEvent(event);
  return true;
}

IMPLEMENT_RPCSTUB_NetS2C_P2P_NotifyDirectP2PDisconnected2(NetClientImpl::S2CStub) {
  CScopedLock2 main_guard(owner->GetMutex());

  // 이 RPC는 상대방과 P2P 그룹 연계가 있음에도 불구하고 P2P 연결이 끊어질 경우에도 도착한다.
  // 따라서 P2P relay mode로 전환해야 한다.

  // This RPC arrives even if P2P connection is broken even though there is P2P group connection with the other party.
  // So you have to switch to P2P relay mode.

  auto peer = owner->GetPeerByHostId(peer_id);
  if (peer && !peer->garbaged_) {
    if (peer->IsDirectP2P()) {
      peer->FallbackP2PToRelay(false, reason);
    }
  }

  return true;
}

IMPLEMENT_RPCSTUB_NetS2C_S2C_RequestCreateUdpSocket(NetClientImpl::S2CStub) {
  CScopedLock2 main_guard(owner->GetMutex());

  // UDP socket 생성
  const bool ok = owner->New_ToServerUdpSocket();

  if (ok) {
  // UPnP 기능을 켠다. connect에서 다시 하도록 옮김 //(Connect에서 하던걸 여기로 옮김)
  //owner->ConditionalStartupUPnP();

  // 홀펀칭 시도
    const InetAddress UdpServerIp = ServerUdpAddr.ToInetAddress();
    owner->to_server_udp_fallbackable_->server_addr_ = UdpServerIp;
    fun_check(owner->to_server_udp_fallbackable_->server_addr_.IsUnicast());
  }

  // UDP 소켓을 열겠다는 확인메세지를 보낸다.
  owner->c2s_proxy_.C2S_CreateUdpSocketAck(HostId_Server, GReliableSend_INTERNAL, ok);
  return true;
}

IMPLEMENT_RPCSTUB_NetS2C_S2C_CreateUdpSocketAck(NetClientImpl::S2CStub) {
  CScopedLock2 main_guard(owner->GetMutex());

  if (ok) {
    // UDP socket 생성
    if (owner->New_ToServerUdpSocket()) {
      // UPnP 기능을 켠다. connect에서 다시 하도록 옮김 //(Connect에서 하던걸 여기로 옮김)
      //owner->ConditionalStartupUPnP();

      // 홀펀칭 시도
      const InetAddress UdpServerIp = ServerUdpAddr.ToInetAddress();
      owner->to_server_udp_fallbackable_->server_addr_ = UdpServerIp;
      fun_check(owner->to_server_udp_fallbackable_->server_addr_.IsUnicast());
    }
  }

  // Request 초기화
  owner->to_server_udp_fallbackable_->server_udp_ready_waiting = false;
  return true;
}

IMPLEMENT_RPCSTUB_NetS2C_ReliablePong(NetClientImpl::S2CStub) {
  /*
  CScopedLock2 main_guard(owner->GetMutex());

  // 받은 시간을 키핑한다.
  owner->last_reliable_pong_received_time_ = owner->GetMorePrecisionAbsoluteTime();
  */

  return true;
}

/*
상대와의 P2P 홀펀칭 시도하던 것들을 모두 중지시킨다.
이미 한 경로가 성공했기 때문이다. hole punch race condition이 발생하면 안되니까.
*/
IMPLEMENT_RPCSTUB_NetC2C_SuppressP2PHolepunchTrial(NetClientImpl::C2CStub) {
  CScopedLock2 main_guard(owner->GetMutex());

  auto peer = owner->GetPeerByHostId(rpc_recvfrom);
  if (peer && !peer->garbaged_) {
    // Stop holepunching.
    peer->p2p_holepunch_attempt_context_.Reset();
  }

  return true;
}

IMPLEMENT_RPCSTUB_NetC2C_ReportUdpMessageCount(NetClientImpl::C2CStub) {
  CScopedLock2 main_guard(owner->GetMutex());

  // 상대방으로부터 상대방이 보낸 갯수와 받은 갯수를 업데이트한다.

  // Update the number of recipients and the number of recipients from the recipient.
  auto peer = owner->GetPeerByHostId(rpc_recvfrom);
  if (peer && !peer->garbaged_) {
    // Update stats
    peer->to_remote_peer_send_udp_message_success_count = UdpSuccessCount;

    owner->c2s_proxy_.ReportC2CUdpMessageCount(
        HostId_Server,      // -> server
        GReliableSend_INTERNAL, // reliable
        peer->host_id_,
        peer->to_remote_peer_send_udp_message_attempt_count,
        peer->to_remote_peer_send_udp_message_success_count);
  }

  return true;
}

IMPLEMENT_RPCSTUB_NetC2C_ReportServerTimeAndFrameRateAndPing(NetClientImpl::C2CStub) {
  CScopedLock2 main_guard(owner->GetMutex());

  auto peer = owner->GetPeerByHostId(rpc_recvfrom);
  if (peer && !peer->garbaged_) {
    peer->recent_frame_rate = recent_frame_rate;

    ApplicationHint hint;
    owner->GetApplicationHint(hint);

    owner->c2c_proxy_.ReportServerTimeAndFrameRateAndPong(
        rpc_recvfrom,           // -> peer
        GReliableSend_INTERNAL, // reliable
        client_local_time,
        owner->GetServerTime(),
        owner->server_udp_recent_ping_,
        hint.recent_frame_rate);
  }

  return true;
}

IMPLEMENT_RPCSTUB_NetC2C_ReportServerTimeAndFrameRateAndPong(NetClientImpl::C2CStub) {
  CScopedLock2 main_guard(owner->GetMutex());

  auto peer = owner->GetPeerByHostId(rpc_recvfrom);
  if (peer && !peer->garbaged_) {
    const double peer_to_server_udp_ping = MathBase::Max(server_udp_recent_ping_, 0.0);
    peer->peer_to_server_ping_ = peer_to_server_udp_ping;
    peer->recent_frame_rate = recent_frame_rate;

    const double absolute_time = owner->GetMorePrecisionAbsoluteTime();
    const double server_time = server_local_time + peer->recent_ping_;

    peer->indirect_server_time_diff_ = absolute_time - server_time;
  }

  return true;
}

IMPLEMENT_RPCSTUB_NetS2C_P2PRecycleComplete(NetClientImpl::S2CStub) {
  CScopedLock2 main_guard(owner->GetMutex());

  auto peer = owner->GetPeerByHostId(peer_id);
  if (peer && !peer->garbaged_) {
    peer->member_join_process_end_ = true;

    if (bRecycled) {
      // 상대측 피어의 주소를 넣는다.
      peer->udp_addr_from_server_ = external_addr;
      peer->udp_addr_internal_ = internal_addr;

      peer->p2p_holepunched_local_to_remote_addr_ = SendAddr;
      peer->p2p_holepunched_remote_to_local_addr_ = RecvAddr;

      peer->SetDirectP2P();

      // 연결 시도중이던 객체를 파괴한다.
      // (이미 이쪽에서 저쪽으로 연결 시도가 성공해서 이미 trial context를 지운 상태일 수 있지만 무관함)
      peer->p2p_holepunch_attempt_context_.Reset();

      //first issue
      // 여기서 하는 이유는 complete가 실패했을때를 대비해서 이다...
      //issue가 되었는데 밑의 로직처럼 지운다면 대략 난감...
      peer->Get_ToPeerUdpSocket()->ConditionalIssueRecvFrom();

      // Enqueue event
      LocalEvent event(LocalEventType::DirectP2PEnabled);
      event.remote_id = peer_id;
      owner->EnqueueLocalEvent(event);
    } else {
      // 과거 홀펀칭 재사용이 실패했다. 따라서 Relayed mode로 냅둔다.
      // JIT P2P가 활성화되면 이 과정까지 왔다면 heartbeat에서 피어간 홀펀칭 과정을 곧 시작할 것이고 그렇지 않다면
      // JIT P2P가 활성화되기 전까지 릴레이 모드로 그냥 냅둘 것이다.
      // P2P 통신이 아직 필요한 단계가 아닌데 홀펀칭을 해버리면 안되기에 이렇게 만들어져 있다.

      //peer->AssureUdpSocketNotUnderIssued(); // UDP socket 객체를 파괴했으나 이것에 대한 overlapped io를 걸어놓은 상황이면 막장 댕글링. 차라리 여기서 오류를 수면위로 노출하자.
      // 어차피 udpsocket를 garbage화 하므로 없어도 된다.

      if (peer->udp_socket_) {
        owner->GarbageSocket(peer->udp_socket_);
        peer->udp_socket_.Reset();
      }

      peer->SetRelayedP2P();
    }
  }

  return true;
}

bool NetClientImpl::S2CStub::EnableIntraLogging(HostId remote_id, const RpcHint& rpc_hint) {
  CScopedLock2 main_guard(owner->GetMutex());
  owner->intra_logging_on_ = true;
  return true;
}

bool NetClientImpl::S2CStub::DisableIntraLogging(HostId remote_id, const RpcHint& rpc_hint) {
  CScopedLock2 main_guard(owner->GetMutex());
  owner->intra_logging_on_ = false;
  return true;
}

// 서버로부터 TCP fallback의 필요함을 노티받을 때의 처리
bool NetClientImpl::S2CStub::NotifyUdpToTcpFallbackByServer(HostId remote_id, const RpcHint& rpc_hint) {
  CScopedLock2 main_guard(owner->GetMutex());
  owner->ConditionalFallbackServerUdpToTcp();
  return true;
}

NetClientImpl::NetClientImpl()
  : conditional_remove_too_old_udp_send_packet_queue_alarm_(NetConfig::udp_packet_board_long_interval_sec),
    process_send_ready_remotes_alarm_(NetConfig::every_remote_issue_send_on_need_interval_sec),
    reliable_ping_alarm_(NetConfig::GetDefaultNoPingTimeoutTime()), // 어차피 이 인터벌은 중간에 바뀜.
    server_as_send_dest_(this),
    to_server_udp_socket_failed_(false),
    host_tag(nullptr),
    last_check_send_queue_time_(0),
    last_update_net_client_stat_clone_time_(0),
    send_queue_heavy_started_time_(0),
    enable_ping_test_end_time_(0) {
  //FIXME 분명히 여기서 참조를 홀드했다가 해제하는데, 왜 소멸자에서 Manager가 파괴되는지??
  manager_ = NetClientManager::GetSharedPtr();

  internal_version_ = NetConfig::InternalNetVersion;

  every_remote_issue_send_on_need_interval_sec = NetConfig::every_remote_issue_send_on_need_interval_sec;

  nat_device_name_detected_ = false;

  //min_extra_ping = 0;
  //extra_ping_variance_ = 0;

  callbacks_ = nullptr;
  last_tick_invoked_time_ = 0;
  local_host_id_ = HostId_None;
  backuped_host_id_ = HostId_None;
  disconnection_invoke_count_.Set(0);
  connect_count_.Set(0);
  to_server_udp_send_count_ = 0;
  last_report_udp_count_time_ = GetAbsoluteTime() + NetConfig::report_real_udp_count_interval_sec;

  c2c_proxy_.engine_specific_only_ = true;
  c2s_proxy_.engine_specific_only_ = true;
  s2c_stub_.engine_specific_only_ = true;
  c2c_stub_.engine_specific_only_ = true;
  c2c_stub_.owner = this;
  s2c_stub_.owner = this;

  AttachProxy(&c2c_proxy_);
  AttachProxy(&c2s_proxy_);
  AttachStub(&s2c_stub_);
  AttachStub(&c2c_stub_);

  intra_logging_on_ = false;
  virtual_speed_hack_multiplication_ = 1;

  io_pending_count_ = 0;
  total_tcp_issued_send_bytes_ = 0;
}

//TODO 예외가 발생하지 않는 버젼을 하나 만들어주는게 좋을듯...
bool NetClientImpl::Connect(const NetConnectionArgs& args) {
  CScopedLock2 connect_disconnect_phase_guard(connect_disconnect_phase_mutex_);

  LockMain_AssertIsNotLockedByCurrentThread();

  connect_count_.Increment();

  CScopedLock2 main_guard(GetMutex()); // for atomic oper

  const ConnectionState server_conn_state = GetServerConnectionState();
  if (server_conn_state != ConnectionState::Disconnected) {
    throw Exception("Wrong state(%s)! Disconnect() or GetServerConnectionState() may be required.", ToString(server_conn_state));
  }

  //Networker에서 reset하므로, 여기서는 반듯이 null이어야함.
  if (worker) {
    ErrorReporter::Report(String::Format("NetClient.Connect - Unstability in Connect #3! Process=%s", CPlatformProcess::ExecutableName()));
  }

  //worker.Reset(); // 서버가 추방했던 클라를 재사용시 이게 존재하기 마련. 따라서 리셋해야 한다.

  if (to_server_udp_socket_ || to_server_udp_fallbackable_) { // 하지만 이건 이미 제거된 상태이어야 한다.
    ErrorReporter::Report(String::Format("NetClient.Connect - Unstability in Connect #1! Process=%s", CPlatformProcess::ExecutableName()));
  }

  // Copy parameters
  connection_args_ = args;

  // 파라메터 정당성 체크
  //CIPEndPoint로 변환한다음. IsUnicast()에 실패했을 경우로 체크하는게 바람직해보임...
  if (!InetAddress(connection_args_.server_ip, connection_args_.server_port).IsUnicast()) {
    throw Exception(ResultInfo::TypeToString(ResultCode::UnknownEndPoint));
  }

  //if (connection_args_.server_ip == "0.0.0.0" ||
  //  connection_args_.server_port == 0 ||
  //  connection_args_.server_port == 0xFFFF ||
  //  connection_args_.server_ip == "255.255.255.255") {
  //  throw Exception(ResultInfo::TypeToString(ResultCode::UnknownEndPoint));
  //}

  // 최초 policy file text를 건너띄기 위해서 필요한 변수 초기화.
  first_recv_disregarded_ = false;
  first_disregard_offset_ = 0;

  // UDP 포트를 지정할 경우 포트가 중복되거나 0을 지정하였는지 체크합니다.
  unused_udp_ports_.Clear();
  used_udp_ports_.Clear();

  for (int32 i = 0; i < connection_args_.local_udp_port_pool.Count(); ++i) {
    if (connection_args_.local_udp_port_pool[i] <= 0) {
      throw Exception(ResultInfo::TypeToString(ResultCode::InvalidPortPool));
    }

    if (unused_udp_ports_.Contains(connection_args_.local_udp_port_pool[i])) {
      throw Exception(ResultInfo::TypeToString(ResultCode::InvalidPortPool));
    }

    unused_udp_ports_.Add(connection_args_.local_udp_port_pool[i]);
  }

  // 이제, 상태 청소를 해도 된다!
  // 서버 접속에 관련된 모든 값들을 초기화한다.
  // (disconnected state에서도 이게 안 비어있을 수 있다.
  // 왜냐하면 서버에서 추방직전 쏜 RPC를 클라가 모두 처리하려면
  // disconnected state에서도 미처리 항목을 유지해야 하기 때문이다.)
  if (connection_args_.TunedNetworkerSendInterval_TEST > 0) {
    every_remote_issue_send_on_need_interval_sec = connection_args_.TunedNetworkerSendInterval_TEST;
  } else {
    every_remote_issue_send_on_need_interval_sec = NetConfig::every_remote_issue_send_on_need_interval_sec;
  }

  conditional_remove_too_old_udp_send_packet_queue_alarm_.Reset();
  process_send_ready_remotes_alarm_.SetInterval(every_remote_issue_send_on_need_interval_sec);
  process_send_ready_remotes_alarm_.Reset();
  reliable_ping_alarm_.Reset();

  final_user_work_queue_.Clear();
  postponed_final_user_work_item_list_.Clear();

  last_tcp_stream_recv_time_ = GetAbsoluteTime();

  nat_device_name_detected_ = false;

  p2p_connection_attempt_end_time_ = NetConfig::GetP2PHolepunchEndTime();
  p2p_holepunch_interval_ = NetConfig::p2p_holepunch_interval_;

  more_precision_clock_.Reset();
  more_precision_clock_.Start();

  stats_.Reset();

  last_tick_invoked_time_ = GetAbsoluteTime();

  to_server_encrypt_count_ = 0;
  to_server_decrypt_count_ = 0;

  last_request_server_time_time_ = 0;
  request_server_time_count_ = 0;

  dx_server_time_diff_ = 0;
  server_udp_recent_ping_ = 0;
  server_udp_last_ping_ = 0;

  local_host_id_ = HostId_None;

  speedhack_detect_ping_cooltime_ = NetConfig::speedhack_detector_enabled_by_default ? 0 : NetConfig::INFINITE_COOLTIME;

  self_encrypt_count_ = 0;
  self_decrypt_count_ = 0;

  //unreliable Rpc를 사용하려 할때 초기화 되어야 하겠다...
  //to_server_udp_socket_ = IHasOverlappedIoPtr(new UdpSocket_C(this, nullptr, args.LocalPort));

  to_server_tcp_.Reset(new TcpTransport_C(this));

  suppress_subsequent_disconnection_events_ = false;

  // 빈 문자열 들어가면 localhost로 자동으로 채움
  connection_args_.server_ip = connection_args_.server_ip.Trimmed();
  if (connection_args_.server_ip.IsEmpty()) {
    connection_args_.server_ip = "localhost";
  }

  to_server_udp_fallbackable_.Reset(new FallbackableUdpTransport_C(this));

  // 클라이언트 워커 스레드에 이 객체를 등록시킨다.
  worker.Reset(new NetClientWorker(this));

  return true;
}

void NetClientImpl::ExtractMessagesFromUdpRecvQueue(
      const uint8* udp_packet,
      int32 udp_packet_length,
      const InetAddress& remote_addr,
      ReceivedMessageList& out_result,
      ResultCode& out_error) {
  // 몽땅 꺼내서 대응하는 HostId를 가진 remote의 수신큐에 저장한다.
  out_result.Clear();

  // coalesce를 감안해서 처리한다.
  const int32 last_extracee_base = out_result.Count();

  MessageStreamExtractor extractor;
  extractor.input = udp_packet;
  extractor.input_length = udp_packet_length;
  extractor.output = &out_result;
  extractor.sender_id = HostId_None;
  extractor.message_max_length = settings_.message_max_length;
  const int32 extracted_msg_count = extractor.Extract(out_error);
  if (extracted_msg_count < 0) {
    // 잘못된 스트림 데이터이다. UDP는 제3자 해커로부터의 메시지가 오는 경우도 있으므로
    // 저쪽과의 연결을 끊지 말고 그냥 조용히 수신된 메시지들을 폐기해야 한다.
    // Warning을 남겨주자.
    EnqueueWarning(ResultInfo::From(out_error, local_host_id_, "ExtractMessagesFromUdpRecvQueue : extracted_msg_count < 0"));
  } else {
    const double absolute_time = GetAbsoluteTime();

    // 얻은 메시지 리스트의 나머지 정보를 세팅한다.
    auto peer = GetPeerByUdpAddr(remote_addr);
    for (int32 msg_index = 0; msg_index < extracted_msg_count; ++msg_index) {
      auto& received_msg = out_result[last_extracee_base + msg_index];

      received_msg.remote_addr_udp_only = remote_addr;

      // peer에서 온 것이면...
      if (peer) {
        received_msg.remote_id = peer->host_id_;

        // pong 체크를 했다고 처리하도록 하자.
        // 이게 없으면 대량 통신시 pong 수신 지연으로 인한 튕김이 발생하니까.
        const double interval = absolute_time - peer->last_direct_udp_packet_recv_time_;

        if (interval > 0) {
          peer->last_udp_packet_recv_interval_ = interval;
        }

        peer->last_direct_udp_packet_recv_time_ = absolute_time;
        peer->direct_udp_packet_recv_count_++;
      } else if (remote_addr == to_server_udp_fallbackable_->server_addr_) {
        // 서버에서 온 것이면...
        received_msg.remote_id = HostId_Server;
        LogLastServerUdpPacketReceived();
      } else {
        // unidentified peer이면...
        received_msg.remote_id = HostId_None;
      }
    }
  }
}

//@note 최초 수신받은 데이터는 policy-text 문자열이 들어 있으므로, 건너띄어 줘야함!
bool NetClientImpl::ExtractMessagesFromTcpStream(ReceivedMessageList& out_result) {
  out_result.Clear(); // just in case.

  LockMain_AssertIsLockedByCurrentThread();

  // 그냥 씹어주어야 하는 부분 처리.
  // 서버 접속시 최초에 policy-text를 받게 되는데, 이게 실질적으로는 패킷이 아니다.
  // 그냥 보안 처리상 필요해 보인다.
  // 이거에 대한 구체적인 이유는 서치를 좀 해봐야겠다.
#if SEND_POLICY_FILE_AT_FIRST
  if (!first_recv_disregarded_) {
    //@todo 최초 policy 문자열을 받는 부분을 별도로 처리하는게 좋을듯함!!!
    //@todo 별도로 처리하는게 바람직할듯함.
    // 첫번째일 경우에는 tcp 스트림에 있는 내용을 모두 비워준다.
    //@fixed 서버에서 보내줄때 널 문자를 포함해서 보내주는듯함.
    //       원래 이래야하는건지???
    const int32 PolicyFileTextLength = POLICY_FILE_TEXT.Len() + 1;

    const int32 RecvStreamLength = Get_ToServerTcp()->recv_stream_.GetLength();

    //@todo 여기서 오프셋 처리를 해주어야할듯함!
    if ((first_disregard_offset_ + RecvStreamLength) < PolicyFileTextLength) {
      // consume.
      first_disregard_offset_ += RecvStreamLength;
      Get_ToServerTcp()->recv_stream_.DequeueNoCopy(RecvStreamLength);

      //@note 아직 policy-text를 모두 받은 상태가 아닐 경우에는 밑으로 가봐야 의미가 없다. 바로 리턴!
      return;
    } else {
      // policy-text를 모두 받은 상태이므로, policy-text에 해당하는 부분 까지만 consume해야함.
      const int32 ConsumeAmount = PolicyFileTextLength - first_disregard_offset_;
      first_disregard_offset_ += ConsumeAmount;
      Get_ToServerTcp()->recv_stream_.DequeueNoCopy(ConsumeAmount);

      // 첫번째로 오는 policy-text는 모두 받았다.
      first_recv_disregarded_ = true;
    }
  }
#endif

  ResultCode extract_result;
  const int32 extracted_and_added_count =
    MessageStream::ExtractMessagesAndFlushStream(
        Get_ToServerTcp()->recv_stream_,
        out_result,
        HostId_Server,
        settings_.message_max_length,
        extract_result);
  if (extracted_and_added_count < 0) { // message stream에 문제가 있으므로, 더이상 진행하지 않고 종료 절차를 밟도록 한다.
    // 서버와의 TCP 연결에 문제가 있다는 뜻이므로 연결 해제를 유도한다.
    const String text = String::Format("received stream from TCP server became inconsistent. (reason=%s)", *ToString(extract_result));
    EnqueueError(ResultInfo::From(extract_result, HostId_Server, *text));

    // 바로 접속해제 처리를 하지 않고, 소켓 핸들을 닫아주어서 자연스럽게 종료 처리 되도록 함.
    InduceDisconnect();
    return false;
  }

  return true;
}

void NetClientImpl::EnqueueDisconnectionEvent(ResultCode result_code,
                                              ResultCode detail_code,
                                              const String& comment) {
  CScopedLock2 main_guard(GetMutex());

  if (!suppress_subsequent_disconnection_events_) {
    suppress_subsequent_disconnection_events_ = true;

    LocalEvent event(LocalEventType::ClientServerDisconnect);
    event.result_info.Reset(new ResultInfo());
    event.result_info->result_code = result_code;
    event.result_info->detail_code = detail_code;
    event.result_info->comment = comment;
    LocalEvent.remote_id = HostId_Server;
    EnqueueLocalEvent(event);

    //TODO
    //if (viz_agent_) {
    //  CScopedLock2 viz_agent_guard(viz_agent_->CS);
    //  viz_agent_->c2s_proxy_.NotifyClient_ConnectionState(HostId_Server, GReliableSend_INTERNAL, GetServerConnectionState());
    //}
  }
}

void NetClientImpl::EnqueueConnectFailEvent(ResultCode result_code, SharedPtr<ResultInfo> result_info) {
  CScopedLock2 main_guard(GetMutex());

  if (!suppress_subsequent_disconnection_events_) {
    suppress_subsequent_disconnection_events_ = true;

    LocalEvent event(LocalEventType::ConnectServerFail);
    event.result_info.Reset(new ResultInfo());
    event.result_info->result_code = result_code;
    event.result_info->comment = result_info->comment;
    event.remote_id = HostId_Server;
    event.remote_addr = InetAddress(connection_args_.server_ip, connection_args_.server_port);
    event.socket_error = SocketErrorCode::Ok;
    EnqueueLocalEvent(event);
  }
}

void NetClientImpl::EnqueueConnectFailEvent(ResultCode result_code, SocketErrorCode socket_error) {
  CScopedLock2 main_guard(GetMutex());

  if (suppress_subsequent_disconnection_events_ == false) {
    suppress_subsequent_disconnection_events_ = true;

    LocalEvent event(LocalEventType::ConnectServerFail);
    event.result_info.Reset(new ResultInfo());
    event.result_info->result_code = result_code;
    event.remote_id = HostId_Server;
    event.remote_addr = InetAddress(connection_args_.server_ip, connection_args_.server_port);
    event.socket_error = socket_error;
    EnqueueLocalEvent(event);
  }
}

void NetClientImpl::SetInitialTcpSocketParameters() {
  // TCP only & relay only
  to_server_udp_fallbackable_->SetRealUdpEnabled(false);
}

INetCoreCallbacks* NetClientImpl::GetCallbacks_NOLOCK() {
  return callbacks_;
}

//@maxidea: RPC stub에서 호출하면 문제가 발생하려나??
void NetClientImpl::Disconnect() {
  Disconnect(DisconnectArgs::Default);

  //@rpc 처리부내에서 호출할 경우, 표시만 해두고 바로 처리하지 않고, 기회가 있을때 처리하도록 유도하자.
}

IMPLEMENT_RPCSTUB_NetS2C_RequestAutoPrune(NetClientImpl::S2CStub) {
  CScopedLock2 main_guard(owner->GetMutex());

  // 서버와의 연결을 당장 끊는다. Shutdown-shake 과정을 할 필요가 없다.
  // 클라는 디스가 불특정 시간에 일어나는 셈이므로.
  if (owner->worker_ && owner->worker_->GetState() <= NetClientWorker::State::Connected) {
    owner->EnqueueDisconnectionEvent(ResultCode::DisconnectFromRemote, ResultCode::TCPConnectFailure, "AutoPrune");
    owner->worker_->SetState(NetClientWorker::State::Disconnecting);
  }

  return true;
}

IMPLEMENT_RPCSTUB_NetS2C_ShutdownTcpAck(NetClientImpl::S2CStub) {
  CScopedLock2 main_guard(owner->GetMutex());

  // shutdown ack를 받으면 바로 종료 처리를 진행하도록 한다.
  if (owner->worker_ &&
      owner->worker_->shutdown_issued_time_ > 0 &&
      owner->worker_->graceful_disconnect_timeout_ > 0) {
    //gracefultime에 따라서 처리한다. 바로 처리하면, gracefultime가 의미 없음.
    owner->worker_->shutdown_issued_time_ = owner->GetAbsoluteTime();// - owner->worker_->graceful_disconnect_timeout_ * 2;
  }

  owner->c2s_proxy_.ShutdownTcpHandshake(HostId_Server, GReliableSend_INTERNAL);
  return true;
}

IMPLEMENT_RPCSTUB_NetS2C_RequestMeasureSendSpeed(NetClientImpl::S2CStub) {
  /*
  CScopedLock2 main_guard(owner->GetMutex());

  owner->manager_->RequestMeasureSendSpeed(enable);
  */

  return true;
}

//TODO 여기서 종료시 지연이 과하게 발생하고 있는듯 싶다.  튜닝을 하도록 하자.
//disconnect 처리시에 지연이 발생하는데, 이를 해소하기 위해서
//객체들을 바로 파괴하면, 비동기 결과처리시 access violation이 일어날 수 있으므로,
//객체가 안전하게 파괴가 가능할때까지 대기했다가 파괴해야함.
//대기하지 않고 바로 처리하는 방법이 없을까?
//그냥 소켓 핸들만 닫아주고 리턴하는건?

/*
Harddisconnect를 지원하는게 좋을듯...

Harddisconnect라고 하여, 무조건 바로 객체들을 파괴하고 종료를 하게 되면, access violation일 발생하므로,
pending i/o가 없을때까지만 대기하도록 하자.

GracefulDisconnectTimeoutMSec이 0인 경우에는 hard-disconnect로 처리하자.

*/
void NetClientImpl::Disconnect(const DisconnectArgs& args) {
  CScopedLock2 connect_disconnect_phase_guard(connect_disconnect_phase_mutex_);
  LockMain_AssertIsNotLockedByCurrentThread();

  bool issue_disconnect_done = false;

  // Stats
  {
    CScopedLock2 manager_guard(manager_->CS);
    manager_->disconnection_invoke_count_.Increment();
    disconnection_invoke_count_.Increment();
  }

  // Conditional intra logging
  if (IsIntraLoggingOn()) {
    CScopedLock2 manager_guard(manager_->CS);
    IntraLogToServer(LogCategory::System, "User call NetClient.Disconnect()");
  }


  //TODO 원래 값대로 해도 될듯 싶음...  아니면 좀더 숙고해서 최적읠 값을 설정하도록 하던지...
  const uint32 T0 = Clock::Milliseconds();
  //const double timeout = MathBase::Max<uint32>((uint32)(graceful_disconnect_timeout_ * 2 * 1000), 100000); // 시간을 100초로 늘립니다.
  const int64 timeout = args.graceful_disconnect_timeout_msec;

  int32 wait_turn_counter = 0;

  while (true) {
    CScopedLock2 main_guard(GetMutex());

    // 이미 접속이 해제된 경우라면 바로 종료하도록 합니다.
    if (!worker) {
      if (to_server_udp_fallbackable_ || to_server_udp_socket_) { // Worker객체가 파괴되기전, 이 두 객체는 정리가 되었어야 합니다.
        ErrorReporter::Report(String::Format("Unstability in Disconnect #1! Process=%s", CPlatformProcess::ExecutableName()));
      }

      CleanupEvenUnstableSituation();
      return;
    }

    const auto worker_state = worker_->GetState();

    // 연결안됨 상태로 전환되었고, garbage가 남아있지 않다면 바로 종료하도록 합니다.
    if (worker_state == NetClientWorker::State::Disconnected && garbages_.IsEmpty()) {
      CleanupEvenUnstableSituation();
      return;
    }

    // 프로그램이 종료중인 상황에서는 곧바로 종료하도록 합니다.
    if (GIsRequestingExit) {
      CleanupEvenUnstableSituation();
      return;
    }

    // manager의 completion port가 이미 파괴되었음(그러면 안되겠지만) 그냥 루프를 나간다.
    if (!manager_->completion_port_) {
      ErrorReporter::Report(String::Format("Unstability in Disconnect #2! Process=%s", CPlatformProcess::ExecutableName()));

      worker_->SetState(NetClientWorker::State::Disconnected);
      CleanupEvenUnstableSituation();
      return;
    }

    //@note 너우 오랫동안 끊어지지 않을 경우, 경고 출력하고 강제로 끊어버림.
    if ((Clock::Milliseconds() - T0) > timeout) {
      // 오류 상황을 제보한다.
      const String text = String::Format("NetClient.Disconnect seems to be freezed ## State=%d##GarbageCount=%d##Process=%s",
                            (int32)worker_->GetState(), garbages_.Count(), CPlatformProcess::ExecutableName());
      ErrorReporter::Report(text);

      worker_->SetState(NetClientWorker::State::Disconnected);
      CleanupEvenUnstableSituation();
      return;
    }

    if (wait_turn_counter == 0) { // 첫번째에서만 접속해제 절차를 시작합니다.
      manager_->cs_.IsLockedByCurrentThread(); // 이 CS를 걸면 매니저의 CS도 걸리도록 만들어져 있으므로

      if (worker_state == NetClientWorker::State::Connected) { // 현재 접속된 상태라면
        // worker_->shutdown_issued_time_ 값이 0보다 큰값으로 설정되면,
        // Worker는 접속해제 절차를 전개할지 여부를 판단하여, 접속해제를 밟게 됩니다.
        worker_->shutdown_issued_time_ = GetAbsoluteTime();

        // 서버와의 연결 해제를 서버에 먼저 알린다.
        // 바로 소켓을 닫고 클라 프로세스가 바로 종료하면 shutdown 신호가 TCP 서버로 넘어가지 못하는 경우가 있다.
        // 따라서 서버에서 연결 해제를 주도시킨 후 클라에서 종료하되 시간 제한을 두는 형태로 한다.
        // (즉 TCP의 graceful shutdown을 대신한다.)
        worker_->graceful_disconnect_timeout_ = (double)args.graceful_disconnect_timeout_msec / 1000.0;

        // 서버에게 안전하게 접속을 끊어달라고 알립니다.  args.graceful_disconnect_timeout_msec 값이 0보다 클 경우에만 작동합니다.
        //TODO 0.1초 이상일때만 작동하는게 좋을듯 싶은데??
        if (worker_->graceful_disconnect_timeout_ > 0.0) {
          LOG(LogNetEngine,Info,"ShutdownTCP: graceful timeout=%fsec", worker_->graceful_disconnect_timeout_);
          c2s_proxy_.ShutdownTcp(HostId_Server, GReliableSend_INTERNAL, args.comment);
        }
      } else if (worker_state < NetClientWorker::State::Connected) {
        // 아직 서버와의 연결중이라면 바로 접속을 끊어도 되므로, 바로 disconnecting으로 전환합니다.
        worker_->SetState(NetClientWorker::State::Disconnecting);
      }
    } else {
      // graceful_disconnect_timeout_ 지정 시간이 지난 후 실 종료 과정이 시작된다
      // 실 종료 과정이 완전히 끝날 때까지 대기

      main_guard.Unlock();

      // 여기서 10ms 기다리는 동안, disconnect가 지연되는 문제가 있음.
      CPlatformProcess::Sleep(args.disconnect_sleep_interval_msec / 1000.0f);
    }

    wait_turn_counter++;
  }


  //TODO 위 루틴이 뭔가 이상한데, 여기까지 코드가 도달할 수 없을텐데???
  //TODO 위 루틴이 뭔가 이상한데, 여기까지 코드가 도달할 수 없을텐데???
  //TODO 위 루틴이 뭔가 이상한데, 여기까지 코드가 도달할 수 없을텐데???
  //legacy코드로 인해서 남아 있었던듯...

  fun_check(0);

  /*
  LockMain_AssertIsNotLockedByCurrentThread();

  // 더이상 worker thread가 안 건드리는게 확인됐으므로 안전히 제거
  if (worker_) {
    //ErrorReporter::Report(String::Format("Unstability in Disconnect #3! Process=%s", CPlatformProcess::ExecutableName()));
  }

  worker_.Reset();

  more_precision_clock_.Stop();

  // 스레드도 싹 종료. 이제 완전히 클리어하자.
  {
    CScopedLock2 main_guard(GetMutex()); // 이걸로 보호한 후 Worker등을 모두 체크하도록 하자.
    CleanupEvenUnstableSituation();
  }
  */
}

NetClientImpl::~NetClientImpl() {
  LockMain_AssertIsNotLockedByCurrentThread();

  Disconnect();

  // RZ 내부에서도 쓰는 RPC까지 더 이상 참조되지 않음을 확인해야 하므로 여기서 시행해야 한다.
  CleanupEveryProxyAndStub(); // 꼭 이걸 호출해서 미리 청소해놔야 한다.
  {
    CScopedLock2 main_guard(GetMutex());
    p2p_groups_.Clear();
    to_server_tcp_.Reset();
    to_server_udp_fallbackable_.Reset();
    final_user_work_queue_.Clear();
    postponed_final_user_work_item_list_.Clear();
    garbages_.Clear();
    lookback_final_received_message_queue_.Clear();
    pre_final_recv_queue.Clear();
    remote_peers_.Clear();
    to_server_udp_socket_.Reset();
    main_guard.Unlock();

    //TODO
    //viz_agent_.Reset();
  }
}

bool NetClientImpl::GetP2PGroupByHostId(HostId group_id, P2PGroupInfo& out_info) {
  CScopedLock2 main_guard(GetMutex());

  if (auto group = GetP2PGroupByHostId_INTERNAL(group_id)) {
    group->GetInfo(out_info);
    return true;
  }

  return false;
}

P2PGroupPtr_C NetClientImpl::GetP2PGroupByHostId_INTERNAL(HostId group_id) {
  CScopedLock2 main_guard(GetMutex());

  return p2p_groups_.FindRef(group_id);
}

/*
직접 보낼지, 릴레이를 통해서 보낼지는 내부에서 판단함.
*/
bool NetClientImpl::Send_BroadcastLayer(const SendFragRefs& payload,
                                        const SendOption& send_opt,
                                        const HostId* sendto_list,
                                        int32 sendto_count) {
  RemotePeer_C* peer = nullptr;

  CScopedLock2 main_guard(GetMutex());

  // 서버와의 연결이 해제된 상태에서는, 아무런 시도도 하지 않음.
  if (!to_server_tcp_ || local_host_id_ == HostId_None) {
    EnqueueError(ResultInfo::From(ResultCode::PermissionDenied, HostId_None, NoServerConnectionErrorText));
    return false;
  }

  // host_id list를 정렬할 array를 따로 만든다.
  HostIdArray sorted_host_id_list(sendto_count, NoInit);
  UnsafeMemory::Memcpy(sorted_host_id_list.MutableData(), sendto_list, sendto_count * sizeof(HostId));

  // 정렬&중복 제거 한다.
  Algo::UnionDuplicateds(sorted_host_id_list);

  // 수신 대상을 ungroup한다. 즉 P2P group은 모두 분해해서 개별 remote들로만 추려낸다.
  ISendDestList_C IndividualDestList;
  ConvertGroupToIndividualsAndUnion(sorted_host_id_list.Count(), sorted_host_id_list.ConstData(), IndividualDestList);

  // 릴레이 타야 하는 dst list. 단, 비압축.
  RelayDestList_C relay_dest_list;
  int32 direct_sendto_wan_peer_count = 0; // 다른 LAN 환경의 피어에게 메시징한 횟수

  // 릴레이 타야 하는 dst list. 단, 압축. 비압축 형태보다 더 커질 수 있는데 이런 경우 비압축 버전이 사용될 것이다.
  CompressedRelayDestList_C compressed_relay_dest_list;

  // 이 함수가 실행되는 동안 재사용될 것임. 그래서 여기서 선언을.
  HostIdArray subset_group_host_id_list;

  // for each sendto items BEGIN
  for (int32 dst_index = 0; dst_index < IndividualDestList.Count(); ++dst_index) {
    auto send_dest = IndividualDestList[dst_index];

    if (send_dest == this && send_opt.bounce) { // if loop-back
      // Enqueue final recv queue and signal
      MessageIn msg(payload.ToBytes()); // copy(소스가 Payload가 Refs이므로, 어쩔 수 없이 복사를 해야함.)
      lookback_final_received_message_queue_.Enqueue(msg);
    } else if (send_dest == &server_as_send_dest_) { // check if sendto is server
      Send_ToServer_Directly_Copy(HostId_Server, send_opt.reliability, payload, send_opt);
    } else if (send_dest) { // P2P로 보내는 메시지인 경우
      try {
        peer = dynamic_cast<RemotePeer_C*>(send_dest);
      }
      catch (std::bad_cast& e) {
        //@todo 깔끔한 방법을 찾아보도록 하자.
        OutputDebugString((const char*)e.what());
        int32* X = nullptr; *X = 1;
      }

      if (peer) {
        bool is_remote_same_lan_to_local = false;

        // 같은 랜 안쪽에 있을 경우에는, 최대한 P2P로 멀티캐스팅 한다...
        if (NetConfig::use_is_same_lan_to_local_for_max_direct_p2p_multicast) {
          is_remote_same_lan_to_local = peer->IsSameLanToLocal();
        }

        // check if sendto is directly P2P connected
        // relay ping(자신과 서버와의핑 + 서버와 피어와의핑) / direct P2P ping
        // 위의 값이 force_relay_threshold_ratio 보다 작은 경우엔 강제로 릴레이전송한다.
        if (peer->IsDirectP2P() &&
            peer->recent_ping_ > 0 && // 유효한 핑 값을 가진 경우(아직 계산이 안되었을 수 있음)
            peer->peer_to_server_ping_ > 0 && // 유효한 핑 값을 가진 경우(아직 계산이 안되었을 수 있음)
            (server_udp_recent_ping_ + peer->peer_to_server_ping_) / peer->recent_ping_ >= send_opt.force_relay_threshold_ratio &&
            (direct_sendto_wan_peer_count < send_opt.max_direct_broadcast_count || is_remote_same_lan_to_local)) {
          // 다이렉트 P2P 전송이 가능한 경우

          if (!is_remote_same_lan_to_local) {
            direct_sendto_wan_peer_count++;
          }

          // peer 마다 udp packet 전송 시도 횟수 를 기록한다.
          // (FunNet 내부 메시지는 제외)
          if (!send_opt.engine_only_specific) {
            peer->to_remote_peer_send_udp_message_attempt_count++;
          }

          // send_core to connected peer via reliable or unreliable UDP
          // Reliable일 경우에는 Reliable UDP로 처리...
          if (send_opt.reliability == MessageReliability::Reliable) {
            peer->to_peer_rudp_.SendWhenReady(payload);
          } else {
            // Unreliable을 UDP로 보내는 경우 MTU size에 제한해서 split하지 않는다.
            // 어차피 UDP 자체가 큰 메시지를 split하니까.
            // 하지만, 큰 메시지는 drop될 수 있으므로, 가능한 작은 데이터를 보내야만 한다?
            peer->to_peer_udp.SendWhenReady(payload, UdpSendOption(send_opt));

            // remote_id peer A에게 다이렉트로 쏜 경우, 그리고 A가 소속된 P2P group G이
            // 애당초 송신할 대상에 있다면, G와 ~A를 compressed relay dst list에 넣는다.
            // 어차피 루프를 계속 돌면서 G 안의 피어들이 direct send가 가능해지면 compressed relay dst list의 차집합
            // 으로서 계속 추가될 것이므로 안전.
            if (GetIntersectionOfHostIdListAndP2PGroupsOfRemotePeer(sorted_host_id_list, peer, &subset_group_host_id_list)) {
              compressed_relay_dest_list.AddSubset(subset_group_host_id_list, peer->host_id_);
            }
          }

          // 직접 P2P로 패킷을 쏘는 경우, JIT P2P 홀펀칭 조건이 된다.
          // 의미가 없지 않을까? 이미 홀펀칭이 됨을 가정하고 들어온 상태이므로...
          if (send_opt.enable_p2p_jit_trigger) {
            peer->jit_direct_p2p_needed_ = true;
          }
        } else {
          // 릴레이로 보내는 경우.

          // gather P2P unconnected sendto

          fun_check(peer->host_id_ != local_host_id_);

          if (send_opt.reliability == MessageReliability::Reliable) { // 이건 할당해줘야
            // remote의 reliable UDP에 stream 및 sender window에 뭔가가 이미 들어있을 수 있다.
            // 그것들을 먼저 UDP send queue로 flush를 해준 다음에야 정확한 reliable UDP용 next frame number를 얻을 수 있다.
            peer->to_peer_rudp_.Host->sender.ConditionalStreamToSenderWindow(true);

            // reliable 형태로 보내므로, ReliableUDP 처리 부에서는 한프레임 처리된걸로 간주하고 건너뛰어 주어야함.
            RelayDest_C rd;
            rd.frame_number = peer->to_peer_rudp_.NextFrameNumberForAnotherReliablySendingFrame();
            rd.remote_peer = peer;
            relay_dest_list.Add(rd);
          } else if (send_opt.allow_relayed_send) { // unreliable인 경우, bAllowRelaySend가 false일 경우 relay로도 보내지 않는다.
            // 자, 이제 릴레이로 unreliable 메시징을 하자.
            RelayDest_C rd;
            rd.frame_number = (FrameNumber)0; // 어차피 안쓰이니까
            rd.remote_peer = peer;
            relay_dest_list.Add(rd);

            // RP에게 릴레이로 쏴야 한다. 그런데 rp가 소속된 그룹이 애당초 보내야 할 리스트에 있다고 치자.
            // 그렇다면 그 그룹을 compressed relay dest에 모두 넣어야 한다.
            // 그래도 괜찮은 것이, 각 individual에 대한 루프를 돌면서 direct send를 한 경우 compressed relay dest에 릴레이 제외 대상
            // 으로서 계속 추가된다.
            if (GetIntersectionOfHostIdListAndP2PGroupsOfRemotePeer(sorted_host_id_list, peer, &subset_group_host_id_list)) {
              compressed_relay_dest_list.AddSubset(subset_group_host_id_list, HostId_None);
            } else {
              compressed_relay_dest_list.AddIndividual(peer->host_id_);
            }
          }

          //rd.remote_peer = peer;
          //relay_dest_list.Add(rd);

          // 직접 P2P가 아닌 상태니까 트리거하는게 맞는거 아닌가?
          // 직접 P2P로 패킷을 쏘는 경우, JIT P2P 홀펀칭 조건이 된다.

          // 다이렉트 P2P상태가 아니거나, 릴레이로 전환되어서 전송되는 상황이므로, 여기에서 P2P를 활성하 시키도록 하는게 맞을듯 싶다.
          if (send_opt.enable_p2p_jit_trigger) {
            peer->jit_direct_p2p_needed_ = true;
          }
        }
      }
    }
  }

  // 릴레이 송신 대상이 존재할 경우
  if (!relay_dest_list.IsEmpty()) {
    // 릴레이 송신 대상이 존재하는 상황. 이때 서버와 UDP 통신을 안하더라도 Per-Peer UDP socket 방식이고
    // 그게 자체적인 클라-서버간 UDP 홀펀칭을 시행하므로 굳이 별도의 클라-서버간 UDP 통신 개시는 불필요.
    //RequestServerUdpSocketReady_FirstTimeOnly();

    // if unreliable send
    if (send_opt.reliability == MessageReliability::Unreliable) {
      // send_core relayed message of gathered list to server via UDP or fake UDP
      HostIdArray RelayDestList2;
      RelayDestList2.Reserve(relay_dest_list.Count());

      // 보낼 메시지 헤더
      MessageOut header;

      // 압축된 relay dest를 쓸건지 말 건지를 파악한다.
      // NetConfig::force_compressed_relay_dest_list_only 이 플래그는 불필요하지 싶은데...
      if (NetConfig::force_compressed_relay_dest_list_only == false &&
        relay_dest_list.Count() <= (compressed_relay_dest_list.GetAllHostIdCount() + 1)) {
        // 비압축 버전이 압축 버전보다 더 경제적인 경우.

        for (int32 dst_index = 0; dst_index < relay_dest_list.Count(); ++dst_index) {
          const auto& rd = relay_dest_list[dst_index];
          RelayDestList2.Add(rd.remote_peer->host_id_);
        }

        lf::Write(header, MessageType::UnreliableRelay1);
        lf::Write(header, send_opt.priority);
        lf::Write(header, send_opt.unique_id);
        lf::Write(header, RelayDestList2);
      } else {
        // 압축 버전.

        lf::Write(header, MessageType::UnreliableRelay1_RelayDestListCompressed);
        lf::Write(header, send_opt.priority);
        lf::Write(header, send_opt.unique_id);
        lf::Write(header, compressed_relay_dest_list.includee_host_id_list); // 그룹에 들어가있지않은 호스트들의 릴레이 리스트

        // (그룹 및 그 그룹에서 제거되어야 할 호스트들)의 리스트
        const OptimalCounter32 group_list_count = compressed_relay_dest_list.p2p_group_list.Count();
        lf::Write(header, group_list_count);
        for (const auto& pair : compressed_relay_dest_list.p2p_group_list) {
          lf::Write(header, pair.key);
          lf::Write(header, pair.value.excludee_host_id_list);
        }
        //lf::Write(header, compressed_relay_dest_list.p2p_group_list);
      }

      lf::Write(header, OptimalCounter32(payload.GetTotalLength())); // 보낼 데이터 크기 (TODO Counter)

      SendFragRefs UnreliableRelayMsg;
      UnreliableRelayMsg.Add(header);
      UnreliableRelayMsg.Add(payload);

      // 릴레이 메시지를 보낸다.
      // 복사 없이 바로 보낸다.
      // HostId_None을 넣어도 무방하다.HostId_Server이랑만 겹치지 않으면 되니까...

      // 릴레이메시지도 내부메시지로 처리한다.
      UdpSendOption opt(send_opt);
      opt.engine_only_specific = true;
      fun_check(send_opt.conditional_fragging == true); // relay 패킷은 relay dest가 여럿일 수 있다. 상당히 커질 수 있으므로 fragging은 꼭 켜야.
      Send_ToServer_Directly_Copy(HostId_None, MessageReliability::Unreliable, UnreliableRelayMsg, opt);
    } else {
      // send_core relayed-long-frame with gathered list to server via *TCP* with each frame number
      SendFragRefs long_frame;

      MessageOut tmp_header;
      MessageStream::AddStreamHeader(payload, long_frame, tmp_header);

      RelayDestList rd_list2;
      relay_dest_list.ToSerializable(rd_list2);

      SendFragRefs relayed_long_frame;

      // 보낼 메시지 헤더
      MessageOut header;
      lf::Write(header, MessageType::ReliableRelay1);                   // 헤더 ID
      lf::Write(header, rd_list2);                                      // 릴레이 리스트 (각 수신자별 프레임 number 포함)
      lf::Write(header, OptimalCounter32(long_frame.GetTotalLength())); // 보낼 데이터 크기 (TODO Counter)

      relayed_long_frame.Add(header);
      relayed_long_frame.Add(long_frame);

      // 릴레이 메시지를 보낸다.
      // 복사 없이 바로 보낸다.
      // (수신자별 프레임 번호가 포함되어있으므로 받는 쪽에서는 reliable UDP 층에 보내서 정렬한다.
      // 따라서 relay/direct 전환중에도 안전하게 데이터가 송달될 것이다.)

      // 릴레이메시지도 내부메시지로 처리한다.
      UdpSendOption opt(send_opt);
      opt.engine_only_specific = true;
      Send_ToServer_Directly_Copy(HostId_None, MessageReliability::Reliable, relayed_long_frame, opt);
    }
  }

  return true;
}

//주의 : SendTo2를 리셋하면 안됨. 추가하는 형태로 동작함.
bool NetClientImpl::ConvertAndAppendP2PGroupToPeerList(HostId sendto, ISendDestList_C& sendto2) {
  LockMain_AssertIsLockedByCurrentThread();

  // convert sendto group to remote hosts
  if (auto group = GetP2PGroupByHostId_INTERNAL(sendto)) {
    for (const auto& pair : group->members_) {
      const HostId member_id = pair.key;
      sendto2.Add(GetSendDestByHostId(member_id));
    }
  } else {
    sendto2.Add(GetSendDestByHostId(sendto));
  }

  return true;
}

ISendDest_C* NetClientImpl::GetSendDestByHostId(HostId peer_id) {
  CScopedLock2 main_guard(GetMutex());

  if (peer_id == HostId_Server) { // server
    return &server_as_send_dest_;
  } else if (peer_id == HostId_None) { // none
    return &ISendDest_C::None;
  } else if (peer_id == local_host_id_) { // local(self)
    return this;
  } else {
    // find out in remote-peer which is not garbage
    auto peer = GetPeerByHostId(peer_id);
    if (peer && !peer->garbaged_) {
      return peer.Get();
    }
  }

  return nullptr;
}

RemotePeerPtr_C NetClientImpl::GetPeerByHostId(HostId peer_id) {
  CScopedLock2 main_guard(GetMutex());
  return remote_peers_.FindRef(peer_id);
}

SessionKey* NetClientImpl::GetCryptSessionKey(HostId remote_id, String& out_error) {
  SessionKey* key = nullptr;

  CScopedLock2 main_guard(GetMutex());

  RemotePeerPtr_C peer;
  if (remote_id == HostId_Server) { // Server
    key = &to_server_session_key_;
  } else if (remote_id == local_host_id_) { // Local
    key = &self_p2p_session_key_;
  } else if (peer = GetPeerByHostId(remote_id)) { // RPs
    //@maxidea: todo: 위의 루틴처럼 garbage는 제껴 주어야 하지 않으려나??
    key = &peer->p2p_session_key_;
  }

  if (key && !key->KeyExists()) {
    out_error = "key not exists.";
    return nullptr;
  }

  if (key == nullptr) {
    out_error = String::Format("%d remote peer is %s in NetClient.", (int32)remote_id, !peer ? "NULL" : "not NULL");
  }

  return key;
}

// <홀펀칭이 성공한 주소>를 근거로 peer를 찾는다.
//@maxidea: GetPeerByUdpAddr_NOLOCK으로 이름을 바꿔줘야 하지 않을까??
RemotePeerPtr_C NetClientImpl::GetPeerByUdpAddr(const InetAddress& udp_addr)
{
  //TODO 락을 걸지 않아도 되려나??
  //CScopedLock2 main_guard(GetMutex());

  for (auto& pair : remote_peers_) {
    auto peer = pair.value;

    if (!peer->garbaged_ && peer->p2p_holepunched_remote_to_local_addr_ == udp_addr)
      // || peer->p2p_holepunched_local_to_remote_addr_ == udp_addr) //@maxidea: 이럴 가능성도 있는건가??
    {
      return peer;
    }
  }
  return RemotePeerPtr_C();
}

void NetClientImpl::SendServerHolepunch() {
  if (IsIntraLoggingOn()) {
    const String text = String::Format("Sending ServerHolepunch: server_addr: %s, tag: %s",
                          *to_server_udp_fallbackable_->server_addr_.ToString(),
                          *to_server_udp_fallbackable_->holepunch_tag_.ToString());
    IntraLogToServer(LogCategory::SP2P, *text);
  }

  MessageOut msg;
  lf::Write(msg, MessageType::ServerHolepunch);
  lf::Write(msg, to_server_udp_fallbackable_->holepunch_tag_);

  Get_ToServerUdpSocket()->SendWhenReady(
      HostId_Server,
      FilterTag::Make(GetLocalHostId(), HostId_Server), // Local -> Server
      to_server_udp_fallbackable_->server_addr_,
      msg,
      GetAbsoluteTime(),
      UdpSendOption(MessagePriority::Holepunch, EngineOnlyFeature));
}

void NetClientImpl::ConditionalSendServerHolePunch() {
  CScopedLock2 main_guard(GetMutex());

  // UDP를 강제로 끈 상태라면 시도조차 하지 않도록 한다.
  if (settings_.FallbackMethod == FallbackMethod::ServerUdpToTcp) {
    return;
  }

  // 소켓이 준비 되지 않았다면 하지 않는다.
  if (!to_server_udp_socket_ || to_server_udp_socket_->IsSocketClosed()) {
    return;
  }

  if (!to_server_udp_fallbackable_) {
    return;
  }

  // 이미 서버와 실 UDP 통신중이라면 홀펀칭 시도가 무의미하다.
  if (to_server_udp_fallbackable_->IsRealUdpEnabled()) {
    return;
  }

  // 아직 HostId를 배정받지 않은 클라이면 서버 홀펀칭을 시도하지 않는다.
  // 어차피 서버로 시도해봤자 서버측에서 host_id=0이면 splitter에서 실패하기 때문에 무시된다.
  if (local_host_id_ == HostId_None) {
    return;
  }

  // UDP 서버 주소를 클라에서 인식 못할 경우
  // (예: NAT 뒤의 서버가 외부 인터넷에서 인식 가능한 주소가 파라메터로 지정되지 않은 경우)
  // 홀펀칭을 시도하지 않는다.
  if (!to_server_udp_fallbackable_->server_addr_.IsUnicast()) {
    return;
  }

  // 설정에서 아예 시도 안하는걸로 설정되어 있으니, 그냥 건너뛴다.
  // 서버 홀펀칭 자체를 하고 싶지 않을 경우가 있을수 있기 때문.
  if (NetConfig::server_udp_holepunch_max_attempt_count <= 0) {
    return;
  }

  // 자 이제 일정 시간마다 서버에게 서버 홀펀칭 요청을 보낸다.
  if (to_server_udp_fallbackable_->holepunch_cooltime_ != NetConfig::INFINITE_COOLTIME) {
    to_server_udp_fallbackable_->holepunch_cooltime_ -= GetElapsedTime();

    if (to_server_udp_fallbackable_->holepunch_cooltime_ < 0) {
      // Reset cooltime.
      to_server_udp_fallbackable_->holepunch_cooltime_ += NetConfig::server_holepunch_interval_sec;

      // 시도 횟수 제한에 걸리게 되면, 다시 하지 않도록 Cooltime을 무한대로 설정한다.
      // 이렇게 시도 횟수를 제한하는것은 무의미하게 시도를 하게 되는 경우를 방지하기
      // 위함이다.
      // 서버 홀펀칭 시도 카운터 증가. (일정 횟수까지만 시도하고 포기한다.)
      to_server_udp_fallbackable_->holepunch_attempt_count_++;

      // 서버 홀펀칭 시도 횟수 초과 확인.
      const bool reached_to_limit = to_server_udp_fallbackable_->holepunch_attempt_count_ > NetConfig::server_udp_holepunch_max_attempt_count;

      if (!reached_to_limit) {
        // 서버에 홀펀칭을 요쳥한다.
        SendServerHolepunch();
      } else {
        // 포기. (더이상의 시도가 무의미하다.)

        to_server_udp_fallbackable_->holepunch_cooltime_ = NetConfig::INFINITE_COOLTIME;

        if (IsIntraLoggingOn()) {
          const String text = String::Format("Give up holepunching to server. (max_attempt_count: %d)", NetConfig::server_udp_holepunch_max_attempt_count);
          IntraLogToServer(LogCategory::SP2P, *text);
        }
      }
    }
  }
}

void NetClientImpl::Tick_FinalUserWorkItem(TickResult* out_result) {
  // 최종 수신 이벤트를 모두 꺼내서 처리한다. 그리고 cs unlock을 한다.
  LockMain_AssertIsNotLockedByCurrentThread();

  int32 processed_rpc_count = 0;
  int32 processed_event_count = 0;

  while (true) {
    FinalUserWorkItem_HasLockedDtor uwi(&GetMutex());

    if (PopFinalUserWorkItem(uwi.GetUWI())) {
      //TODO Holster/Postpone은 제거하는게 좋을듯함..
      bool bHolsterMoreCallback = false;
      bool bPostponeThisCallback = false;
      DoOneUserWorkItem(uwi.GetUWI(), bHolsterMoreCallback, bPostponeThisCallback, processed_rpc_count, processed_event_count);

      if (bPostponeThisCallback) {
        PostponeFinalUserWorlItem(uwi.GetUWI());
      }

      if (bHolsterMoreCallback) {
        break;
      }
    } else {
      break;
    }
  }

  if (out_result) {
    out_result->processed_message_count = processed_rpc_count;
    out_result->processed_event_count = processed_event_count;
  }
}

bool NetClientImpl::GetPeerInfo(HostId remote_id, PeerInfo& out_info) {
  CScopedLock2 main_guard(GetMutex());

  if (auto peer = remote_peers_.FindRef(remote_id)) {
    peer->GetPeerInfo(out_info);
    return true;
  }
  return false;
}

void NetClientImpl::GetLocalJoinedP2PGroups(HostIdArray& output) {
  CScopedLock2 main_guard(GetMutex());
  p2p_groups_.GenerateKeyArray(output);
}

//#ifdef DEPRECATE_SIMLAG
//  void NetClientImpl::SimulateBadTraffic(DWORD min_extra_ping, DWORD extra_ping_variance_) {
//    min_extra_ping = ((double)min_extra_ping) / 1000;
//    extra_ping_variance_ = ((double)extra_ping_variance_) / 1000;
//  }
//#endif // DEPRECATE_SIMLAG

P2PGroupPtr_C NetClientImpl::CreateP2PGroupObject_INTERNAL(HostId group_id) {
  CScopedLock2 main_guard(GetMutex());

  P2PGroupPtr_C new_group(new P2PGroup_C());
  new_group->group_id_ = group_id;
  p2p_groups_.Add(group_id, new_group);
  return new_group;
}

void NetClientImpl::ConditionalRequestServerTime() {
  if (local_host_id_ != HostId_None) {
    // VirtualSpeedHackMultiplication이 1보다 크게 설정된 경우에는
    // 보내는 주기가 빨라질 것이므로, 스피드 핵을 흉내내는 역활을 하게된다.
    if ((GetAbsoluteTime() - last_request_server_time_time_) > (NetConfig::cs_ping_interval_sec / virtual_speed_hack_multiplication_)) {
      last_request_server_time_time_ = GetAbsoluteTime();

      request_server_time_count_++;

      // UDP로 주고 받는 메시지 - 핑으로도 쓰인다.
      MessageOut msg;
      lf::Write(msg, MessageType::RequestServerTimeAndKeepAlive);
      lf::Write(msg, GetAbsoluteTime());
      lf::Write(msg, server_udp_recent_ping_);
      const UdpSendOption send_opt(MessagePriority::Ring0, EngineOnlyFeature);
      to_server_udp_fallbackable_->SendWhenReady(HostId_Server, SendFragRefs(msg), send_opt);
    }

    reliable_ping_alarm_.SetInterval(GetReliablePingTimerInterval()); //@note interval이 가변적임!

    if (reliable_ping_alarm_.TakeElapsedTime(GetElapsedTime())) {
      c2s_proxy_.ReliablePing(HostId_Server, GReliableSend_INTERNAL, ApplicationHint.recent_frame_rate);
    }
  } else {
    // 서버에 연결 상태가 아니면 미리미리 이렇게 준비해둔다.
    LogLastServerUdpPacketReceived();
    //last_reliable_pong_received_time_ = last_server_udp_packet_recv_time_;
  }
}

void NetClientImpl::ConditionalSpeedHackPing() {
  if (local_host_id_ != HostId_None && speedhack_detect_ping_cooltime_ != NetConfig::INFINITE_COOLTIME) {
    speedhack_detect_ping_cooltime_ -= GetElapsedTime();

    if (speedhack_detect_ping_cooltime_ <= 0) {
      // Reset for next step.
      speedhack_detect_ping_cooltime_ += NetConfig::speedhack_detector_ping_interval_sec;

      // virtual speed hack
      speedhack_detect_ping_cooltime_ /= virtual_speed_hack_multiplication_; //@todo inverse 값을 미리 계산해두면, 나누기를 곱하기로 대체 가능.

      // UDP로 주고 받는 메시지. 핑으로도 쓰인다.
      MessageOut msg;
      lf::Write(msg, MessageType::SpeedHackDetectorPing);
      const UdpSendOption send_opt(MessagePriority::Ring0, EngineOnlyFeature);
      to_server_udp_fallbackable_->SendWhenReady(HostId_Server, SendFragRefs(msg), send_opt);
    }
  }
}

double NetClientImpl::GetReliablePingTimerInterval() {
  // 연속 두세번 보낸 것이 일정 시간 안에 도착 안하면
  // 사실상 막장이므로 이정도 주기면 OK.
  return settings_.default_timeout_sec * 3;
}

void NetClientImpl::ConditionalSyncIndirectServerTime() {
  LockMain_AssertIsLockedByCurrentThread();

  //for (auto& pair : remote_peers_) {
  //  auto peer = pair.value;
  for (auto it = remote_peers_.CreateIterator(); it; ++it) {
    auto& peer = it.value();

    if (peer && !peer->garbaged_ && peer->host_id_ != HostId_Server) {
      peer->sync_indirect_server_time_diff_cooltime_ -= GetElapsedTime();

      if (peer->sync_indirect_server_time_diff_cooltime_ <= 0) {
        fun_check(NetConfig::p2p_ping_interval_sec > 0);

        // Reset for next step.
        peer->sync_indirect_server_time_diff_cooltime_ += NetConfig::p2p_ping_interval_sec;

        peer->last_ping_send_time_ = GetAbsoluteTime();

        if (peer->IsDirectP2P()) {
          // peer가 가진 server time을 다른 peer에게 전송한다. 즉 간접 서버 시간을 동기화하고자 한다.
          //
          // 일정 시간마다 각 peer에게 P2P_SyncIndirectServerTime(서버에 의해 동기화된 시간, 랙, 프레임 레이트)을 보낸다.
          // 이걸 받은 상대는 해당 peer 기준으로의 time diff 값을 갖고 있는다. 모든 peer로부터
          // 이값을 받으면 그리고 peer가 속한 각 P2P group 범위 내에서의 time diff 평균값을 계산한다.
          //
          // 또한 이 메시지는 P2P 간 keep alive check를 하는 용도로도 쓰인다.

          // P2PIndirectServerTimeAndPing
          //    + client_time

          MessageOut msg_to_send;
          lf::Write(msg_to_send, MessageType::P2PIndirectServerTimeAndPing);
          lf::Write(msg_to_send, GetMorePrecisionAbsoluteTime());
          const UdpSendOption send_opt(MessagePriority::Ring0, EngineOnlyFeature);
          peer->to_peer_udp.SendWhenReady(SendFragRefs(msg_to_send), send_opt);
        } else {
          // peer와 relayed 통신을 하는 경우.
          // 이미 보내서 받은것처럼 fake한다.

          const double interval = GetAbsoluteTime() - peer->last_direct_udp_packet_recv_time_;
          if (interval > 0) {
            peer->last_udp_packet_recv_interval_ = interval;
          }

          peer->last_direct_udp_packet_recv_time_ = GetAbsoluteTime();
          peer->direct_udp_packet_recv_count_++;
          peer->indirect_server_time_diff_ = 0;

          // relay의 경로(A->서버->B)의 랙을 합산한다.
          peer->last_ping_ = server_udp_recent_ping_ + peer->peer_to_server_ping_;

          if (peer->set_to_relayed_but_last_ping_is_not_calculated_yet_) {
            peer->recent_ping_ = 0;
            peer->set_to_relayed_but_last_ping_is_not_calculated_yet_ = false;
          }

          if (peer->recent_ping_ > 0) {
            // 즉치를 대입하지 않고, 보간을 해서 완만하게 변하도록 한다.
            peer->recent_ping_ = MathBase::Lerp(peer->recent_ping_, peer->last_ping_, NetConfig::log_linear_programming_factor);
          } else {
            // 최초 갱신일 때는 대입.
            peer->recent_ping_ = peer->last_ping_;
          }
        }
        int XXX = 0;
      }
    }
  }
}

IMPLEMENT_RPCSTUB_NetS2C_NotifySpeedHackDetectorEnabled(NetClientImpl::S2CStub) {
  CScopedLock2 main_guard(owner->GetMutex());

  if (enabled) {
    if (owner->speedhack_detect_ping_cooltime_ == NetConfig::INFINITE_COOLTIME) {
      owner->speedhack_detect_ping_cooltime_ = 0;
    }
  } else {
    owner->speedhack_detect_ping_cooltime_ = NetConfig::INFINITE_COOLTIME;
  }

  return true;
}

double NetClientImpl::GetIndirectServerTime(HostId peer_id) {
  CScopedLock2 main_guard(GetMutex());

  if (more_precision_clock_.IsStopped()) {
    return -1;
  }

  const double absolute_time = GetMorePrecisionAbsoluteTime();

  if (auto peer = GetPeerByHostId(peer_id)) {
    //@todo 아니 여기서 하는거야??
    // GetIndirectServerTime을 호출했다는 것은 P2P를 전재로 했다는 얘기가 되므로 그런걸까??
    peer->jit_direct_p2p_needed_ = true;

    return absolute_time - peer->GetIndirectServerTimeDiff();
  } else {
    return absolute_time - dx_server_time_diff_;
  }
}

double NetClientImpl::GetServerTime() {
  CScopedLock2 main_guard(GetMutex());

  if (more_precision_clock_.IsStopped()) {
    return -1;
  }

  const double absolute_time = GetMorePrecisionAbsoluteTime();

  return absolute_time - dx_server_time_diff_;
}

double NetClientImpl::GetAbsoluteTime() {
  return manager_->GetCachedAbsoluteTime();
}

double NetClientImpl::GetElapsedTime() {
  return manager_->GetCachedElapsedTime();
}

void NetClientImpl::ConditionalFallbackServerUdpToTcp() {
  const double absolute_time = GetAbsoluteTime();

  // 너무 오랜 시간동안 서버에 대한 UDP ping이 실패하면 UDP도 TCP fallback mode로 전환한다.
  // [CaseCMN] 간혹 섭->클 UDP 핑은 되면서 반대로의 핑이 안되는 경우로 인해 UDP fallback이 계속 안되는 경우가 있는 듯.
  // 그러므로 서버에서도 클->섭 UDP 핑이 오래 안오면 fallback한다.
  if (to_server_udp_fallbackable_->IsRealUdpEnabled() &&
      (absolute_time - to_server_udp_fallbackable_->last_server_udp_packet_recv_time_) > NetConfig::GetFallbackServerUdpToTcpTimeout()) {
    // check last_server_udp_packet_recv_count_ here
    FirstChanceFallbackServerUdpToTcp(ResultCode::ServerUdpFailed);
  }
}

double NetClientImpl::GetP2PServerTime(HostId group_id) {
  CScopedLock2 main_guard(GetMutex());

  // 서버와 그룹 멤버의 서버 시간의 평균을 구한다.
  int32 count = 1;
  double sum_diff = GetServerTimeDiff();
  if (auto group = GetP2PGroupByHostId_INTERNAL(group_id)) {
    if (more_precision_clock_.IsStopped()) {
      return -1;
    }

    for (const auto& pair : group->members_) {
      if (auto& member = pair.value) {
        fun_check(member->GetHostId() != HostId_Server);
        sum_diff += member->GetIndirectServerTimeDiff();
        count++;
      }
    }

    const double group_server_time_diff = sum_diff / ((double)count);
    const double client_time = GetMorePrecisionAbsoluteTime();

    return client_time - group_server_time_diff;
  } else {
    return GetServerTime();
  }
}

void NetClientImpl::RemoveRemotePeerIfNoGroupRelationDetected(RemotePeerPtr_C member_rp) {
  LockMain_AssertIsLockedByCurrentThread();

  // 모든 그룹을 뒤져서 local과 제거하려는 remote가 모두 들어있는 P2P group이 하나라도 존재하면
  // P2P 연결 해제를 하지 않는다.

  //@maxidea:
  // 하나이상의 P2P그룹에 속해 있을 경우에는 연결을 해제하지 않음.
  // 그렇다면 도대체 언제 해제하는건가??
  for (const auto& group_pair : p2p_groups_) {
    auto group = group_pair.value;

    for (const auto& member_pair : group->members_) {
      auto& member = member_pair.value;

      if (member_rp.Get() == member) {
        return;
      }
    }
  }

  // 아래에서 없애긴 한다만 아직 다른 곳에서 참조할 수도 있으니 이렇게 dispose 유사 처리를 해야 한다.
  // (없어도 될 것 같긴 하다. 왜냐하면 remote_peers_.Remove를 한 마당인데 P2P 통신이 불가능하니까.)
  //member_rc->SetRelayedP2P(); // 여기서 하면 소켓 재활용이 되지 않는다.

  // repunch 예비 불필요
  // 상대측에도 P2P 직빵 연결이 끊어졌으니 relay mode로 전환하라고 알려야 한다.
  c2s_proxy_.P2P_NotifyDirectP2PDisconnected(HostId_Server, GReliableSend_INTERNAL, member_rp->host_id_, ResultCode::NoP2PGroupRelation);

  //EnqueueFallbackP2PToRelayEvent(member_rc->host_id_, ResultCode::NoP2PGroupRelation); P2P 연결 자체를 로직컬하게도 없애는건데, 이 이벤트를 보낼 필요는 없다.

  if (IsIntraLoggingOn()) {
    const String text = String::Format("Disconnected direct P2P to client %d.", (int32)member_rp->host_id_);
    IntraLogToServer(LogCategory::PP2P, *text);
  }

  // per-peer UDP socket만 닫고 제낀다.
  //if (member_rp->udp_socket_) {
  GarbagePeer(member_rp);
  //}
}

void NetClientImpl::GetGroupMembers(HostId group_id, HostIdArray& output) {
  // LockMain_AssertIsLockedByCurrentThread();왜 이게 있었지?

  CScopedLock2 main_guard(GetMutex());

  if (auto group = GetP2PGroupByHostId_INTERNAL(group_id)) {
    group->members_.GenerateKeyArray(output);
  } else {
    output.Clear(); // just in case
  }
}

/**
네트워크 상황에 따라서, 후퇴모드(fallback)로 전환되는데 이상황을 시험해보고자
임의로 대체모드로 전환하고자 할 경우에 사용됨.
즉, 테스트 목적의 함수임.
*/
void NetClientImpl::TEST_FallbackUdpToTcp(FallbackMethod mode) {
  if (!HasServerConnection()) {
    return;
  }

  CScopedLock2 main_guard(GetMutex());

  switch (mode) {
    case FallbackMethod::CloseUdpSocket:
      LockMain_AssertIsLockedByCurrentThread();

      if (Get_ToServerUdpSocket()) {
        Get_ToServerUdpSocket()->socket_->CloseSocketHandleOnly();
      }

      for (auto& RPPair : remote_peers_) {
        auto& peer = RPPair.value;

        // 강제로 소켓을 다 닫아버린다. 즉 UDP 소켓이 맛간 상태를 만들어버린다.
        if (peer->Get_ToPeerUdpSocket()) {
          peer->Get_ToPeerUdpSocket()->CloseSocketHandleOnly();
        }

        // Fallback이 이루어짐을 테스트해야 하므로
        // FirstChanceFallbackEveryPeerUdpToTcp, FirstChanceFallbackServerUdpToTcp 즐
      }
      break;

    case FallbackMethod::ServerUdpToTcp:
      FirstChanceFallbackServerUdpToTcp(ResultCode::UserRequested);
      break;

    case FallbackMethod::PeersUdpToTcp:
      FirstChanceFallbackEveryPeerUdpToTcp(ResultCode::UserRequested);
      break;
  }
}

double NetClientImpl::GetLastPingSec(HostId peer_id) {
  CScopedLock2 main_guard(GetMutex());

  if (peer_id == HostId_Server) { // Server
    return server_udp_last_ping_;
  }

  // RPs
  if (auto peer = GetPeerByHostId(peer_id)) {
    //@todo 아니 이걸 왜 여기서 하지?? 뭔가 제대로 된 방법은 아닐듯..
    // 해당 피어의 마지막 핑 지연값을 구하기 위함이니, 결국은 P2P가 필요한 상황이라는 걸까..
    peer->jit_direct_p2p_needed_ = true;
    return peer->last_ping_;
  }

  // P2P group을 얻으려고 하는 경우 모든 멤버들의 평균 핑을 구한다.
  if (auto group = GetP2PGroupByHostId_INTERNAL(peer_id)) {
    // touch JIT P2P & get recent ping ave
    int32 count = 0;
    double sum = 0.0;
    for (const auto& member_pair : group->members_) {
      const double ping = GetLastPingSec(member_pair.key); // Touch JIT P2P (이놈이 호출되면 P2P가 개시되게됨.. 명확하지는 않은듯 싶음!!)
      if (ping >= 0.0) {
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

double NetClientImpl::GetRecentPingSec(HostId peer_id) {
  CScopedLock2 main_guard(GetMutex());

  // Server
  if (peer_id == HostId_Server) {
    return server_udp_recent_ping_;
  }

  // RPs
  if (auto peer = GetPeerByHostId(peer_id)) {
    peer->jit_direct_p2p_needed_ = true;
    return peer->recent_ping_;
  }

  // P2P group을 얻으려고 하는 경우 모든 멤버들의 평균 핑을 구한다.
  if (auto group = GetP2PGroupByHostId_INTERNAL(peer_id)) {
    // touch JIT P2P & get recent ping average.
    double count = 0;
    double sum = 0.0;
    for (const auto& member_pair : group->members_) {
      const double ping = GetRecentPingSec(member_pair.key); // touch jit P2P. (@todo 이게 좀 뽀록 스럽다..)
      if (ping >= 0.0) {
        sum += ping;
        count++;
      }
    }

    if (count > 0) {
      return sum / count;
    }
  }

  return -1;
}

InetAddress NetClientImpl::GetServerAddress() {
  CScopedLock2 main_guard(GetMutex());

  if (to_server_tcp_) {
    return Get_ToServerTcp()->socket_->GetPeerName();
  } else {
    return InetAddress::None;
  }
}

void NetClientImpl::TcpAndUdp_LongTick() {
  CScopedLock2 main_guard(GetMutex());

  const double absolute_time = GetAbsoluteTime();

  if (conditional_remove_too_old_udp_send_packet_queue_alarm_.TakeElapsedTime(GetElapsedTime())) {
    if (to_server_udp_socket_) {
      Get_ToServerUdpSocket()->LongTick(absolute_time);

      // per-peer UDP socket에 대해서도 처리하기.
      for (auto& pair : remote_peers_) {
        auto& peer = pair.value;

        if (peer && !peer->garbaged_) {
          if (peer->udp_socket_) {
            peer->Get_ToPeerUdpSocket()->LongTick(absolute_time);
          }

          // 송신량 측정 결과도 여기다 업뎃한다.
          peer->send_queued_amount_in_byte = peer->to_peer_udp.GetUdpSendBufferPacketFilledCount();
        }
      }
    }

    // TCP에 대해서도 처리하기
    if (Get_ToServerTcp()) {
      Get_ToServerTcp()->LongTick(absolute_time);
    }
  }

  if ((absolute_time - tcp_and_udp_short_tick_last_time_) > NetConfig::rudp_heartbeat_interval_sec) {
    if (to_server_udp_socket_) {
      Get_ToServerUdpSocket()->packet_fragger_->ShortTick(absolute_time);

      // EndPointQueueMapCount 갱신
      if (settings_.emergency_log_line_count > 0) {
        EmergencyLogData.server_udp_addr_count = Get_ToServerUdpSocket()->packet_fragger_->GetEndPointToQueueMapKeyCount();
      }
    }

    // per-peer UDP socket에 대해서도 처리하기.
    //for (auto& pair : remote_peers_) {
    for (auto it = remote_peers_.CreateIterator(); it; ++it) {
      auto& peer = it->value;

      if (peer && !peer->garbaged_) {
        if (peer->udp_socket_) {
          peer->Get_ToPeerUdpSocket()->packet_fragger_->ShortTick(absolute_time);

          if (settings_.emergency_log_line_count > 0) {
            EmergencyLogData.server_udp_addr_count = peer->Get_ToPeerUdpSocket()->packet_fragger_->GetEndPointToQueueMapKeyCount();
          }
        }
      }
    }

    tcp_and_udp_short_tick_last_time_ = absolute_time;
  }
}

/**
자연스럽게 종료 플로우를 탈 수 있도록 소켓만 닫아준다.
소켓을 닫았으므로, recv/send시에 실패 할것이므로, 자연스럽게 종료 플로우를
타게 될것이다.
사전적 의미: 접속 해제를 유도하다. 쯤 되겠음.
*/
void NetClientImpl::InduceDisconnect() {
  if (to_server_tcp_ && Get_ToServerTcp()->socket_) {
      Get_ToServerTcp()->socket_->CloseSocketHandleOnly();

    if (IsIntraLoggingOn()) {
      IntraLogToServer(LogCategory::TCP, "InduceDisconnect, CloseSocketHandleOnly called.");
    }
  }
}

void NetClientImpl::ConvertGroupToIndividualsAndUnion(int32 sendto_count, const HostId* sendto, HostIdArray& output) {
  CScopedLock2 main_guard(GetMutex());

  ISendDestList_C list;
  ConvertGroupToIndividualsAndUnion(sendto_count, sendto, list);

  //output.ResizeUninitialized(list.Count());
  output.Resize(list.Count());
  for (int32 dst_index = 0; dst_index < list.Count(); ++dst_index) {
    auto dst = list[dst_index];
    output[dst_index] = dst ? dst->GetHostId() : HostId_None;
  }
}

// P2P group HostId를 개별 HostId로 바꾼 후 중복되는 것들을 병합한다.
void NetClientImpl::ConvertGroupToIndividualsAndUnion(int32 sendto_count, const HostId* sendto, ISendDestList_C& send_dest_list)
{
  for (int32 i = 0; i < sendto_count; ++i) {
    if (sendto[i] != HostId_None) {
      ConvertAndAppendP2PGroupToPeerList(sendto[i], send_dest_list);
    }
  }

  //TODO ISendDest 비교연산이 안되고 있음. 원인 분석해야함.
  //주의 : SendDestList안에 null이 있으면 안됨.
  //포인터를 정렬하는것도 지원하는게 좋을듯...
  algo::UnionDuplicateds(send_dest_list);
}

void NetClientImpl::Send_ToServer_Directly_Copy(
      HostId dest_id,
      MessageReliability reliability,
      const SendFragRefs& data_to_send,
      const UdpSendOption& send_opt)
{
  // send_core to server via UDP or TCP
  if (reliability == MessageReliability::Reliable) {
    // Reliable이므로, TCP를 통해서 서버에게 보냄.
    Get_ToServerTcp()->SendWhenReady(data_to_send, TcpSendOption());
  } else {
    // Unreliable이므로, UDP로 서버에게 보냄. (UDP가 아직 안될 경우에는 TCP로 보냄.)

    // JIT UDP trigger하기
    RequestServerUdpSocketReady_FirstTimeOnly();

    // unrealiable이 되기전까지 TCP로 통신
    // unreliable일때만 uniqueid를 사용하므로...
    to_server_udp_fallbackable_->SendWhenReady(dest_id, data_to_send, send_opt);
  }
}

String NetClientImpl::DumpGroupStatus() {
  CScopedLock2 main_guard(GetMutex());

  for (auto& pair : remote_peers_) {
    auto peer = pair.value;

    //@todo 무엇을 보여주어야할지??
  }

  return String();
}

bool NetClientImpl::Send( const SendFragRefs& data_to_send,
                          const SendOption& send_opt,
                          const HostId* sendto,
                          int32 sendto_count) {
  // 연결이 안된 상태일 경우에는 바로 리턴합니다.
  if (!worker_) {
    return false;
  }

  // 메시지 압축 레이어를 통하여 메시지에 압축 여부 관련 헤더를 삽입한다.
  // 암호화 된 후에는 데이터의 규칙성이 없어져서 압축이 재대로 되지 않기 때문에 반드시 암호화 전에 한다.
  return Send_CompressLayer(data_to_send, send_opt, sendto, sendto_count);
}

bool NetClientImpl::NextEncryptCount(HostId remote_id, CryptoCountType& out_count) {
  CScopedLock2 main_guard(GetMutex());

  // 아직 서버와 연결이 되지 않았다면 encryptcount는 존재하지 않는다.
  if (!to_server_tcp_ || local_host_id_ == HostId_None) {
    return false;
  }

  if (remote_id == HostId_Server) { // Server
    out_count = to_server_encrypt_count_++;
    return true;
  } else if (remote_id == local_host_id_) { // Local(self)
    out_count = self_encrypt_count_++;
    return true;
  } else if (auto peer = GetPeerByHostId(remote_id)) { // Peers
    fun_check(!peer->garbaged_);
    out_count = peer->encrypt_count++;
    return true;
  } else {
    //@maxidea: todo: RemoteId가 유효하지 않은 상호인데, 로깅을 해주어야할까??
    fun_check(0);

    return false;
  }
}

void NetClientImpl::PrevEncryptCount(HostId remote_id) {
  CScopedLock2 main_guard(GetMutex());

  // 아직 서버와 연결이 되지 않았다면 encryptcount는 존재하지 않는다.
  if (!to_server_tcp_ || local_host_id_ == HostId_None) {
    return;
  }

  if (remote_id == HostId_Server) { // Server
    --to_server_encrypt_count_;
  } else if (remote_id == local_host_id_) { // Local
    --self_encrypt_count_;
  } else if (auto peer = GetPeerByHostId(remote_id)) { // RPs
    fun_check(!peer->garbaged_);
    --peer->encrypt_count;
  } else {
    //@maxidea: todo: RemoteId가 유효하지 않은 상호인데, 로깅을 해주어야할까??
    fun_check(0);
  }
}

bool NetClientImpl::GetExpectedDecryptCount(HostId remote_id, CryptoCountType& out_count) {
  CScopedLock2 main_guard(GetMutex());

  if (remote_id == HostId_Server) { // Server
    out_count = to_server_decrypt_count_;
    return true;
  } else if (remote_id == local_host_id_) { // Local
    out_count = self_decrypt_count_;
    return true;
  } else if (auto peer = GetPeerByHostId(remote_id)) { // RPs
    fun_check(!peer->garbaged_);
    out_count = peer->decrypt_count;
    return true;
  } else {
    //@maxidea: todo: RemoteId가 유효하지 않은 상호인데, 로깅을 해주어야할까??
    fun_check(0);
    return false;
  }
}

bool NetClientImpl::NextDecryptCount(HostId remote_id) {
  CScopedLock2 main_guard(GetMutex());

  if (remote_id == HostId_Server) { // Server
    ++to_server_decrypt_count_;
    return true;
  } else if (remote_id == local_host_id_) { // Local
    ++self_decrypt_count_;
    return true;
  } else if (auto peer = GetPeerByHostId(remote_id)) { // RPs
    fun_check(!peer->garbaged_);
    ++peer->decrypt_count;
    return true;
  } else {
    //@maxidea: todo: RemoteId가 유효하지 않은 상호인데, 로깅을 해주어야할까??
    fun_check(0);
    return false;
  }
}

void NetClientImpl::IntraLogToServer(LogCategory category, const char* text) {
  // Send log-message to server.
  if (intra_logging_on_ && local_host_id_ != HostId_None) {
    c2s_proxy_.NotifyLog(HostId_Server, GReliableSend_INTERNAL, category, text);
  }

  // Also append emergency-log message.
  if (settings_.emergency_log_line_count > 0) {
    AddEmergencyLogList(category, text);
  }
}

void NetClientImpl::LogToServer_HolepunchFreqFail(int32 rank, const char* text) {
  // Send log-message which related to hole-punching to server.
  if (intra_logging_on_ && local_host_id_ != HostId_None) {
    c2s_proxy_.NotifyLogHolepunchFreqFail(HostId_Server, GReliableSend_INTERNAL, rank, text);
  }

  // Also append emergency-log message.
  if (settings_.emergency_log_line_count > 0) {
    //TODO 카테고리를 뭘로 해야하나??
    AddEmergencyLogList(LogCategory::P2P, text);
  }
}

void NetClientImpl::FirstChanceFallbackServerUdpToTcp(ResultCode reason_to_show) {
  if (to_server_udp_fallbackable_->IsRealUdpEnabled() == true) {
    // 이것을 먼저 시행할 것!
    // fun::RemotePeer_C::FirstChanceChangeToRelay에서 이 메서드를 재귀호출할 터이니.
    to_server_udp_fallbackable_->SetRealUdpEnabled(false);

    // 로컬 이벤트
    LocalEvent event(LocalEventType::ServerUdpChanged);
    event.result_info.Reset(new ResultInfo());
    event.result_info->result_code = ResultCode::ServerUdpFailed;
    event.remote_id = HostId_Server;
    EnqueueLocalEvent(event);

    // 서버와의 UDP 포트 매핑이 증발했지만, 일정 시간 후에 다시 재시도해야 한다.
    // 단, 서버와의 UDP 포트 매핑 재시도는 제한을 두도록 한다.
    if (to_server_udp_fallbackable_->tcp_fallback_count < NetConfig::server_udp_repunch_max_attempt_count) {
      to_server_udp_fallbackable_->holepunch_cooltime_ = NetConfig::server_udp_repunch_interval_sec;
      to_server_udp_fallbackable_->tcp_fallback_count++;

      to_server_udp_fallbackable_->holepunch_attempt_count_ = 0;
    } else {
      // 최대 제한 횟수를 초과했으므로, 더이상 서버와의 UDP 홀펀칭은 시도하지 않는다.
      to_server_udp_fallbackable_->holepunch_cooltime_ = NetConfig::INFINITE_COOLTIME;
    }

#ifndef _DEBUG // 디버그 빌드에서는 의레 있는 일이므로
    if (IsIntraLoggingOn()) {
      String traffic_stat = GetTrafficStatText();
      const double udp_duration = GetAbsoluteTime() - to_server_udp_fallbackable_->real_udp_enabled_time;
      double last_udp_recv_issue_duration = 0;

      if (Get_ToServerUdpSocket()) {
        last_udp_recv_issue_duration = GetAbsoluteTime() - Get_ToServerUdpSocket()->last_udp_recv_issued_time_;
      }

      String S1 = Get_ToServerUdpSocketLocalAddr().ToString();
      const char* P1 = *S1;
      const char* P2 = *traffic_stat;

      int32 rank = 1;
      bool LBN = false;
      if (IsLocalHostBehindNAT(LBN) && !LBN) {
        rank++;
      }

      if (!GetNatDeviceName().IsEmpty()) {
        rank++;
      }

      const String text = String::Format("(first chance) to-server UDP punch lost##reason:%d##CliInstCount=%d##RecentElapTime=%3.3f##DisconnedCount=%d##recv count=%d##last ok recv interval=%3.3f##Recurred:%d##LocalIP:%s##UDP kept time:%3.3f##Time diff since RecvIssue:%3.3f##NAT name=%s##%s##Process=%s",
                      *ToString(reason_to_show),
                      manager_->Instances.Count(),
                      manager_->GetCachedRecentAverageElapsedTime(),
                      manager_->disconnection_invoke_count_.GetValue(),
                      to_server_udp_fallbackable_->last_server_udp_packet_recv_count_,
                      to_server_udp_fallbackable_->last_udp_packet_recv_interval_,
                      to_server_udp_fallbackable_->tcp_fallback_count,
                      P1,
                      udp_duration,
                      last_udp_recv_issue_duration,
                      *GetNatDeviceName(),
                      P2,
                      "process-name?"//CPlatformProcess::ExecutableName()
                    );

      LogToServer_HolepunchFreqFail(rank, *text);
    }
#endif // _DEBUG

    // 서버에 TCP fallback을 해야 함을 노티.
    c2s_proxy_.NotifyUdpToTcpFallbackByClient(HostId_Server, GReliableSend_INTERNAL);

    // 클라에서 to-server-UDP가 증발해도 per-peer UDP는 증발하지 않는다. 아예 internal port 자체가 다르니까.
    // 따라서 to-peer UDP는 그대로 둔다.
  }
}

void NetClientImpl::TEST_EnableVirtualSpeedHack(double multiplied_speed) {
  if (multiplied_speed <= 0) {
    throw Exception("invalid parameter.");
  }

  virtual_speed_hack_multiplication_ = multiplied_speed;
}

HostId NetClientImpl::GetLocalHostId() {
  CScopedLock2 main_guard(GetMutex());

  return local_host_id_;
}

//@todo 연결 상태는 하나(EConnectState)로 통합하는게 바람직할 것으로 보임.
ConnectionState NetClientImpl::GetServerConnectionState() {
  CScopedLock2 main_guard(GetMutex());

  if (worker_) {
    switch (worker_->GetState()) {
      case NetClientWorker::State::Disconnected: return ConnectionState::Disconnected;
      case NetClientWorker::State::IssueConnect: return ConnectionState::Connecting;
      case NetClientWorker::State::Connecting: return ConnectionState::Connecting;
      case NetClientWorker::State::JustConnected: return ConnectionState::Connected;
      case NetClientWorker::State::Connected: return ConnectionState::Connected;
      case NetClientWorker::State::Disconnecting: return ConnectionState::Disconnecting;
    }
  }

  return ConnectionState::Disconnected;
}

//@todo 연결 상태는 하나(EConnectState)로 통합하는게 바람직할 것으로 보임.
ConnectionState NetClientImpl::GetServerConnectionState(ServerConnectionState& out_state) {
  CScopedLock2 main_guard(GetMutex());

  out_state.real_udp_enabled = to_server_udp_fallbackable_ ? to_server_udp_fallbackable_->IsRealUdpEnabled() : false;

  if (worker_) {
    switch (worker_->GetState()) {
      case NetClientWorker::State::Disconnected: return ConnectionState::Disconnected;
      case NetClientWorker::State::IssueConnect: return ConnectionState::Connecting;
      case NetClientWorker::State::Connecting: return ConnectionState::Connecting;
      case NetClientWorker::State::JustConnected: return ConnectionState::Connected;
      case NetClientWorker::State::Connected: return ConnectionState::Connected;
      case NetClientWorker::State::Disconnecting: return ConnectionState::Disconnecting;
    }
  }

  return ConnectionState::Disconnected;
}

void NetClientImpl::Tick(TickResult* out_result) {
  if (last_tick_invoked_time_ != -1) {
    last_tick_invoked_time_ = GetAbsoluteTime(); // 어차피 이 값은 부정확해도 되므로.
  }

  // 미뤄 놨던 것들을 바로 처리할 수 있도록 final-queue로 보내줌.
  Tick_PullPostponeeToFinalQueue();

  // final-queue에 enqueue된 내용들을 뽑아내서 처리.
  Tick_FinalUserWorkItem(out_result);
}

InetAddress NetClientImpl::GetLocalUdpSocketAddr(HostId remote_peer_id) {
  CScopedLock2 main_guard(GetMutex());

  auto peer = GetPeerByHostId(remote_peer_id);
  return (peer && peer->udp_socket_) ? peer->Get_ToPeerUdpSocket()->local_addr_ : InetAddress::None;
}

bool NetClientImpl::GetPeerRUdpStats(HostId peer_id, RUdpHostStats& out_stats) {
  CScopedLock2 main_guard(GetMutex());

  RemotePeerPtr_C peer;
  if (remote_peers_.TryGetValue(peer_id, peer) && peer->to_peer_rudp_.host) {
    peer->to_peer_rudp_.Host->GetStats(out_stats);
    return true;
  } else {
    return false;
  }
}

void NetClientImpl::EnqueueFallbackP2PToRelayEvent(HostId remote_peer_id, ResultCode reason) {
  LocalEvent event(LocalEventType::RelayP2PEnabled);
  event.result_info.Reset(new ResultInfo());
  event.result_info->result_code = reason;
  event.remote_id = remote_peer_id;
  EnqueueLocalEvent(event);
}

//@todo 루틴이 살짝 이상한데..
void NetClientImpl::GetStats(NetClientStats& out_stats) {
  CScopedLock2 main_guard(GetMutex());

  // Copy
  out_stats = stats_;

  out_stats.remote_peer_count = remote_peers_.Count();
  out_stats.server_udp_enabled = to_server_udp_fallbackable_ ? to_server_udp_fallbackable_->IsRealUdpEnabled() : false;

  out_stats.direct_p2p_enabled_peer_count = 0;
  for (auto& pair : remote_peers_) {
    auto peer = pair.value;

    if (peer->IsDirectP2P()) {
      out_stats.direct_p2p_enabled_peer_count++;
    }
  }
}

// 릴레이 메시지에 들어갈 '릴레이 최종 수신자들' 명단을 만든다.
void NetClientImpl::RelayDestList_C::ToSerializable(RelayDestList& out_result) {
  out_result.Clear();

  for (int32 i = 0; i < Count(); ++i) {
    const RelayDest_C& dst = (*this)[i];

    RelayDest dest_info;
    dest_info.sendto = dst.remote_peer->host_id_;
    dest_info.frame_number = dst.frame_number;
    out_result.Add(dest_info);
  }
}

//@maxidea: todo: 제거하자..
void NetClientImpl::TEST_GetTestStats(TestStats2& out_stats) {
  out_stats = TestStats2;
}


NetClient* NetClient::New() {
  return new NetClientImpl();
}

// 모든 peer들과의 UDP를 relay화한다.
void NetClientImpl::FirstChanceFallbackEveryPeerUdpToTcp(ResultCode reason) {
  for (auto& pair : remote_peers_) {
    auto peer = pair.value;

    peer->FallbackP2PToRelay(true, reason);
  }
}

bool NetClientImpl::RestoreUdpSocket(HostId peer_id) {
  CScopedLock2 main_guard(GetMutex());

  if (auto peer = GetPeerByHostId(peer_id)) {
    peer->restore_needed = true;
    return true;
  }

  return false;
}

IMPLEMENT_RPCSTUB_NetS2C_RenewP2PConnectionState(NetClientImpl::S2CStub) {
  CScopedLock2 main_guard(owner->GetMutex());

  if (auto peer = owner->GetPeerByHostId(peer_id)) {
    peer->SetRelayedP2P();

    peer->repunch_count = 0;
    peer->repunch_started_time = owner->GetAbsoluteTime();
    peer->restore_needed = false;

    peer->CreateP2PHolepunchAttemptContext();

    if (owner->IsIntraLoggingOn()) {
      const String text = String::Format("Perfectly reset P2P connection client %d.", (int32)peer->host_id_);
      owner->IntraLogToServer(LogCategory::PP2P, *text);
    }
  }

  return true;
}

IMPLEMENT_RPCSTUB_NetS2C_NewDirectP2PConnection(NetClientImpl::S2CStub) {
  CScopedLock2 main_guard(owner->GetMutex());

  if (auto peer = owner->GetPeerByHostId(peer_id)) {
    if (!peer->udp_socket_) {
      peer->new_p2p_connection_needed = true;

      if (owner->IsIntraLoggingOn()) {
        const String text = String::Format("Request P2P connection to client %d.", (int32)peer->host_id_);
        owner->IntraLogToServer(LogCategory::PP2P, *text);
      }
    }
  }

  return true;
}

bool NetClientImpl::GetDirectP2PInfo(HostId peer_id, DirectP2PInfo& out_info) {
  CScopedLock2 main_guard(GetMutex());

  if (peer_id == HostId_Server) { // Server
    return false;
  } else if (auto peer = GetPeerByHostId(peer_id)) { // RPs
    peer->jit_direct_p2p_needed_ = true; // 이 메서드를 호출한 이상 JIT P2P를 켠다.
    peer->GetDirectP2PInfo(out_info);
    return out_info.HasBeenHolepunched();
  } //else {
  //  fun_check(peer_id == local_host_id_);
  //}

  return false;
}

bool NetClientImpl::InvalidateUdpSocket(HostId peer_id, DirectP2PInfo& out_info) {
  CScopedLock2 main_guard(GetMutex());

  if (peer_id == HostId_Server) { // server
    return false;
  } else if (auto peer = GetPeerByHostId(peer_id)) { // RPs
    peer->GetDirectP2PInfo(out_info);

    if (peer->udp_socket_ && !peer->Get_ToPeerUdpSocket()->socket_->IsClosed()) {
      // Force the UDP socket to be closed.
      // The UDP socket must be closed right here to give other network engines the opportunity to create sockets on the same port.
      // (Of course, it may be banned at the OS level by SO_EXCLUSIVEUSEADDR)
      LockMain_AssertIsLockedByCurrentThread();

      peer->Get_ToPeerUdpSocket()->socket_->CloseSocketHandleOnly();

      // Fallback should happen right away
      peer->FallbackP2PToRelay(true, ResultCode::UserRequested);
    }

    const bool has_been_holepunched = out_info.HasBeenHolepunched();
    return has_been_holepunched;
  } else { // local
    fun_check(peer_id == local_host_id_);
  }

  return false;
}

//void NetClientImpl::ConditionalReissueUdpRecv()
//{
//  // 유저의 요청이 있으면, 여기서 restored UDP socket의 new issue recv를 해야 한다.
//  if (Get_ToServerUdpSocket()->just_restored_) {
//    Get_ToServerUdpSocket()->just_restored_ = false;
//    Get_ToServerUdpSocket()->IssueRecvFrom();
//  }
//
//  for (auto pair : remote_peers_) {
//    auto peer = pair.value;
//
//    if (peer->udp_socket_ && peer->udp_socket_->just_restored_) {
//      peer->udp_socket_->just_restored_ = false;
//      peer->udp_socket_->IssueRecvFrom();
//    }
//  }
//}

UdpSocket_C* NetClientImpl::Get_ToServerUdpSocket() {
  return to_server_udp_socket_ ? (UdpSocket_C*)(to_server_udp_socket_.Get()) : nullptr;
}

TcpTransport_C* NetClientImpl::Get_ToServerTcp() {
  return to_server_tcp_ ? (TcpTransport_C*)(to_server_tcp_.Get()) : nullptr;
}

void NetClientImpl::LogLastServerUdpPacketReceived() {
  if (to_server_udp_fallbackable_) {
    // pong 체크를 했다고 처리하도록 하자.
    // 이게 없으면 대량 통신시 pong 수신 지연으로 인한 튕김이 발생하니까.
    const double interval = GetAbsoluteTime() - to_server_udp_fallbackable_->last_server_udp_packet_recv_time_;

    if (interval > 0) {
      to_server_udp_fallbackable_->last_udp_packet_recv_interval_ = interval;
    }

    to_server_udp_fallbackable_->last_server_udp_packet_recv_time_ = GetAbsoluteTime();
    to_server_udp_fallbackable_->last_server_udp_packet_recv_count_++;

#ifdef UDP_PACKET_RECEIVE_LOG
    to_server_udp_fallbackable_->last_server_udp_packet_recv_queue_.Add(last_server_udp_packet_recv_time_);
#endif
  }
}

bool NetClientImpl::IsLocalHostBehindNAT(bool& out_is_behind_nat) {
  out_is_behind_nat = false;

  if (!HasServerConnection()) {
    return false;
  }

  out_is_behind_nat = (Get_ToServerUdpSocketLocalAddr() != Get_ToServerUdpSocketAddrAtServer());
  return true;
}

// 디버깅 용도
//@maxidea: 제거대상
String NetClientImpl::GetTrafficStatText() {
  NetClientStats stats;
  GetStats(stats);

  return String::Format("{total_send_bytes: %I64d, total_recv_bytes: %I64d, peer_count: %d/%d, nat_name: %s}",
            stats.total_udp_send_bytes,
            stats.total_udp_recv_bytes,
            stats.direct_p2p_enabled_peer_count,
            stats.remote_peer_count,
            *GetNatDeviceName());
}

//@deprecated
String NetClientImpl::TEST_GetDebugText() {
  return String();
//    return String::Format("%s %s", local_udp_socket_addr.ToString(), LocalUdpSocketAddrFromServer.ToString());
}

InetAddress NetClientImpl::Get_ToServerUdpSocketLocalAddr() {
  auto udp_socket = Get_ToServerUdpSocket();
  return udp_socket ? udp_socket->local_addr_ : InetAddress::None;
}

// UDP socket address of the client that the server knows
InetAddress NetClientImpl::Get_ToServerUdpSocketAddrAtServer() {
  auto udp_socket = Get_ToServerUdpSocket();
  return udp_socket ? udp_socket->here_addr_at_server_ : InetAddress::None;
}

// 일정 시간마다 휴지통 총정리
void NetClientImpl::DoGarbageCollection() {
  CScopedLock2 main_guard(GetMutex());

  const double absolute_time = GetAbsoluteTime();

  for (auto it = recycles_.CreateIterator(); it; ++it) {
    auto udp_socket = (UdpSocket_C*)it->value.Get();

    // NetClientImpl::DoGarbageCollection()에서는 RecyclePairReuseTime보다 10초는 더 기다려야 안전빵.
    if ((absolute_time - udp_socket->recycle_binned_time_) > NetConfig::recycle_pair_reuse_time_sec + 10) {
      GarbageSocket(it->value);
      it.RemoveCurrent();
    }
  }

  for (auto it = peer_garbages_.CreateIterator(); it; ++it) {
    auto& peer = it->value;

    if (peer->leave_event_count == 0) {
      remote_peers_.Remove(peer->GetHostId());
      it.RemoveCurrent();
    }
  }

  for (auto it = garbages_.CreateIterator(); it; ++it) {
    auto& socket = *it;
    fun_check(socket_->IsSocketClosed());

    // issue send|recv도 UDP socket이 is-closed이면 아예 시도조차 안하므로 OK.
    // 참고: send-issue-post를 이제는 안하므로 없는 UDP socket에 대한 completion이 뒷북으로는 안온다.
    const bool should_delete = socket_->IsSocketClosed() && !socket_->recv_issued_ && !socket_->send_issued_;

    if (should_delete) {
      auto udp_socket = (UdpSocket_C*)socket.Get();

      const int32 udp_port = udp_socket->local_addr_.GetPort();
      if (used_udp_ports_.Contains(udp_port)) {
        // Returns if the port is assigned to a user-defined UDP port.
        unused_udp_ports_.Add(udp_port);
        used_udp_ports_.Remove(udp_port);
      }

      // Guaranteed no more I/O. Let's erase it now.
      garbages_.Remove(it);
    }
  }
}

/**
Remove the peer and put the peer's socket in the trash. The trash can empty later.
*/
void NetClientImpl::RemovePeer(RemotePeerPtr_C peer) {
  LockMain_AssertIsLockedByCurrentThread();
  //CScopedLock2 main_guard(GetMutex());

  //if (peer->owner_ == this) {
  if (peer->udp_socket_) {
    // Now, this UDP socket will no longer issue any transmission issues and will be ignored even if the reception is completed.
    if (peer->IsDirectP2P()) {
      // 홀펀칭이 이루어졌었던 피어이므로, 차후 재사용 가능함.
      UdpSocketToRecycleBin(peer->udp_socket_);
    } else {
      GarbageSocket(peer->udp_socket_);
    }

    peer->udp_socket_.Reset();
  }

  peer->owner_ = nullptr;

  remote_peers_.Remove(peer->host_id_);
  //}
}

void NetClientImpl::GarbagePeer(RemotePeerPtr_C peer) {
  //CScopedLock2 main_guard(GetMutex());
  LockMain_AssertIsLockedByCurrentThread();

  if (peer->garbaged_) {
    return;
  }

  if (peer->owner_ == this) {
    if (peer->udp_socket_) {
      // Now, this UDP socket will no longer issue any transmission issues and will be ignored even if the reception is completed.
      if (peer->IsDirectP2P()) {
        // 홀펀칭이 이루어졌었던 피어이므로, 차후 재사용 가능함.
        UdpSocketToRecycleBin(peer->udp_socket_);
      } else {
        GarbageSocket(peer->udp_socket_);
      }

      peer->udp_socket_.Reset();
    }

    peer->owner_ = nullptr;
    peer->garbaged_ = true;
    peer->p2p_holepunch_attempt_context_.Reset();
    peer->SetRelayedP2P();
    //remote_peers_.Remove(peer->host_id_);
    peer_garbages_.Add(peer->host_id_, peer);
  }
}

void NetClientImpl::DecreaseLeaveEventCount(HostId host_id) {
  CScopedLock2 main_guard(GetMutex());

  if (auto peer = GetPeerByHostId(host_id)) {
    --peer->leave_event_count;
  }
}

String NetClientImpl::GetNatDeviceName() {
  return manager_->upnp_ ? manager_->upnp_->GetNatDeviceName() : String();
}

void NetClientImpl::OnSocketWarning(InternalSocket* socket, const String& msg) {
  //@maxidea: main_guard 하에서 해야하나??
  CScopedLock2 main_guard(GetMutex());

  if (IsIntraLoggingOn()) {
    IntraLogToServer(LogCategory::System, *String::Format("socket warning: %s", *msg));
  }
}

void NetClientImpl::EveryRemote_IssueSendOnNeed() {
  CScopedLock2 main_guard(GetMutex());

  // 서버 TCP
  //@todo 이름이 좀 거시기함..
  if (auto to_server_tcp_component = Get_ToServerTcp()) {
    to_server_tcp_component->ConditionalIssueSend();

    io_pending_count_ = to_server_tcp_component->socket_->io_pending_count_;
    total_tcp_issued_send_bytes_ = to_server_tcp_component->total_tcp_issued_send_bytes_;

    //TODO IoPendingCount는 어디서 감하는지??  현재 감하는 부분이 없음. 누락된건가??

    //LOG(LogNetEngine,Warning,"io_pending_count_:%d total_tcp_issued_send_bytes_:%d", io_pending_count_, total_tcp_issued_send_bytes_);
  }

  // 서버 UDP
  if (auto ToServerUdpSocket_Component = Get_ToServerUdpSocket()) {
    ToServerUdpSocket_Component->ConditionalIssueSend();
  }

  // 피어 UDP
  for (auto& pair : remote_peers_) {
    auto& peer = pair.value;

    if (peer && !peer->garbaged_ && peer->udp_socket_) {
      peer->Get_ToPeerUdpSocket()->ConditionalIssueSend();
    }
  }
}

CCriticalSection2& NetClientImpl::GetMutex() {
  return manager_->mutex;
}

//@maxidea: 제거해야하나??
double NetClientImpl::GetSendToServerSpeed() {
  //CScopedLock2 manager_guard(manager_->mutex, true);
  //
  //if (manager_->send_speed_measurer_) {
  //  return manager_->send_speed_measurer_->GetLastMeasuredSendSpeed();
  //}
  //else
  {
    return 0;
  }
}

uint32 NetClientImpl::GetInternalVersion() {
  return internal_version_;
}

void NetClientImpl::EnqueueLocalEvent(LocalEvent& event) {
  CScopedLock2 main_guard(GetMutex());
  final_user_work_queue_.Enqueue(FinalUserWorkItem(event));
}

void NetClientImpl::SetCallbacks(INetClientCallbacks* callbacks) {
  if (AsyncCallbackMayOccur()) {
    //TODO Exception 클래스를 특수화하는게 좋을듯함.
    //CAsyncCallbackOccurException() 같은...?  이름은 좀더 생각을 해봐야할듯..
    throw Exception("Already async callback may occur.  Server start or client connection should have not been done before here.");
  }

  LockMain_AssertIsNotLockedByCurrentThread();
  callbacks_ = callbacks;
}

void NetClientImpl::EnqueueError(SharedPtr<ResultInfo> result_info) {
  LocalEvent event(LocalEventType::Error);
  event.result_info = result_info;
  event.remote_id = result_info->remote;
  event.remote_addr = result_info->remote_addr;
  EnqueueLocalEvent(event);
}

void NetClientImpl::EnqueueWarning(SharedPtr<ResultInfo> result_info) {
  LocalEvent event(LocalEventType::Warning);
  event.result_info = result_info;
  event.remote_id = result_info->remote;
  event.remote_addr = result_info->remote_addr;
  EnqueueLocalEvent(event);
}

bool NetClientImpl::AsyncCallbackMayOccur() {
  return worker_.IsValid();
}

int32 NetClientImpl::GetMessageMaxLength() {
  return settings_.message_max_length;
}

void NetClientImpl::EnqueuePacketDefragWarning(const InetAddress& addr, const char* text) {
  CScopedLock2 main_guard(GetMutex());

  auto peer = GetPeerByUdpAddr(addr);
  EnqueueWarning(ResultInfo::From(ResultCode::InvalidPacketFormat, peer ? peer->host_id_ : HostId_None, text));
}

//void NetClientImpl::ConditionalPruneTooOldDefragBoard() {
//  CScopedLock2 main_guard(GetMutex());
//
//  if ((GetAbsoluteTime() - last_prune_too_old_defragger_time_) > NetConfig::assemble_fragged_packet_timeout_sec / 2) {
//    last_prune_too_old_defragger_time_ = GetAbsoluteTime();
//
//    packet_defragger_->PruneTooOldDefragBoard();
//  }
//}

// PublicAddress = Local address of client recognized by server
InetAddress NetClientImpl::GetPublicAddress() {
  CScopedLock2 main_guard(GetMutex());
  return Get_ToServerTcp() ? Get_ToServerTcp()->local_addr_at_server_ : InetAddress::None;
}

void NetClientImpl::ConditionalReportP2PPeerPing() {
  const double absolute_time = GetAbsoluteTime();

  if (settings_.ping_test_enabled && (absolute_time - enable_ping_test_end_time_) > NetConfig::report_p2p_peer_ping_test_interval_sec) {
    for (auto& pair : remote_peers_) {
      enable_ping_test_end_time_ = absolute_time;

      const HostId remote_id = pair.key;
      if (local_host_id_ < remote_id) {
        auto& peer = pair.value;

        if (peer->garbaged_) {
          continue;
        }

        const bool direct_p2p = peer->IsDirectP2P();

        // P2P가 릴레이 서버를 타는 경우보다 느린 경우에는 릴레이 서버를 타게끔 되어 있다.
        //TODO 아래 조건을 체크 하는 부분이 코드 형태로 다른곳에도 있으므로, 유지보수에 문제가 생길 수 있음.
        //체크 함수를 별도로 두는게 좋을듯함.
        if (direct_p2p &&
            peer->recent_ping_ > 0 &&
            peer->peer_to_server_ping_ > 0 &&
            (server_udp_recent_ping_ + peer->peer_to_server_ping_) < peer->recent_ping_) {
          // 릴레이 서버를 타는 경우 remote_id Peer와 서버의 핑과 자신과 서버의 핑을 더함
          c2s_proxy_.ReportP2PPeerPing(HostId_Server, GReliableSend_INTERNAL, peer->host_id_, (uint32)((server_udp_recent_ping_ + peer->peer_to_server_ping_) * 1000));
        }
        // 릴레이 일때는 보내지 않는다.
        else if (direct_p2p) {
          c2s_proxy_.ReportP2PPeerPing(HostId_Server, GReliableSend_INTERNAL, peer->host_id_, (uint32)(peer->recent_ping_ * 1000));
        }
      }
    }
  }
}

void NetClientImpl::ReportRealUdpCount() {
  // If the feature is off, just return it.
  // Unnecessary traffic is wasted unless you really want to see statistics.
  if (!NetConfig::report_real_udp_count_enabled) {
    return;
  }

  //@todo 실제로 동작하는 UDP가 하나도 없을 경우에도 무의미하게 전송이 이루어짐.
  //      꼭 필요한 경우에만 하도록 해보자.

  CScopedLock2 main_guard(GetMutex());

  const double absolute_time = GetAbsoluteTime();

  if (local_host_id_ != HostId_None &&
      last_report_udp_count_time_ > 0 &&
      (absolute_time - last_report_udp_count_time_) > NetConfig::report_real_udp_count_interval_sec) {
    last_report_udp_count_time_ = absolute_time;

    // Report the number of UDP sent to the server.
    c2s_proxy_.ReportC2SUdpMessageTrialCount(HostId_Server, GReliableSend_INTERNAL, to_server_udp_send_count_);

    // Tell the peers the number of UDPs I have sent to or received from the peers.
    // (Traffic may spike every 10 seconds, but even if you have 100 peers, it's about 2000 bytes.)
    for (auto& pair : remote_peers_) {
      auto& peer = pair.value;

      if (peer->garbaged_ == false) {
        c2c_proxy_.ReportUdpMessageCount(peer->host_id_, GReliableSend_INTERNAL, peer->receive_udp_message_success_count);
      }
    }
  }
}

void NetClientImpl::ConditionalStartupUPnP() {
  CScopedLock2 main_guard(GetMutex()); // 이게 없으면 정확한 체크가 안되더라.

  if (manager_->upnp_.IsValid() == false &&
      settings_.upnp_detect_nat_device &&
      Get_ToServerTcp()->local_addr_.IsUnicast() &&
      Get_ToServerTcp()->local_addr_at_server_.IsUnicast() &&
      IsBehindNAT()) {
    manager_->upnp_.Reset(new UPnP(manager_.Get()));
  }
}

void NetClientImpl::DoOneUserWorkItem(FinalUserWorkItem& uwi,
                                      bool& out_holster_more_callback,
                                      bool& out_postpone_this_callback,
                                      int32& rpc_processed_count,
                                      int32& event_processed_count) {
  switch (uwi.type) {
  //TODO
  //클라이언트가 one thread일 경우에는 처리가 안됨.. 생각좀 해보는걸로...
  case FinalUserWorkItemType::UserTask:
    fun_check(0);
    break;

  case FinalUserWorkItemType::RPC: {
      auto& received_msg = uwi.unsafe_message;

      auto& msg_content = received_msg.unsafe_message;
      const int32 saved_read_pos = msg_content.Tell();
      //fun_check(saved_read_pos == 0);

      bool processed = false;
      bool turn_processed = false;

      LockMain_AssertIsNotLockedByCurrentThread();

      void* host_tag = GetHostTag(received_msg.remote_id);

      RpcId rpc_id = RpcId_None;
      if (lf::Read(msg_content, rpc_id)) {
        for (int32 stub_index = 0; stub_index < stubs_nolock_.Count(); ++stub_index) {
          RpcStub* stub = stubs_nolock_[stub_index];
          try {
            //각 RPC STUB들이 내부에서 RPC ID를 읽는것 부터 시작하므로, 다시 원위치.
            msg_content.Seek(saved_read_pos);

            // one thread model인 클라라서 안전
            stub->bHolsterMoreCallback_FORONETHREADEDMODEL = false;
            stub->bPostponeThisCallback_FORONETHREADEDMODEL = false;

            turn_processed |= stub->ProcessReceivedMessage(received_msg, host_tag);
            processed |= turn_processed;

            out_holster_more_callback  |= stub->bHolsterMoreCallback_FORONETHREADEDMODEL;
            out_postpone_this_callback |= stub->bPostponeThisCallback_FORONETHREADEDMODEL;

            // If the process succeeds and is not an internal message.
            if (turn_processed && stub != ((RpcStub*)&s2c_stub_) && stub != ((RpcStub*)&c2c_stub_)) {
              ++rpc_processed_count;
            }
          } catch (Exception& e) {
            if (callbacks_) {
              callbacks_->OnException(received_msg.remote_id, e);
            }
          } catch (std::exception& e) {
            if (callbacks_) {
              callbacks_->OnException(received_msg.remote_id, Exception(e));
            }
          } //catch (_com_error& e) {
          //  if (callbacks_) {
          //    callbacks_->OnException(received_msg.remote_id, Exception(e));
          //  }
          //} catch (void* e) {
          //  if (callbacks_) {
          //    callbacks_->OnException(received_msg.remote_id, Exception(e));
          //  }
          //}
#ifdef ALLOW_CATCH_UNHANDLED_EVEN_FOR_USER_ROUTINE
          catch (...) {
            if (callbacks_) {
              Exception e;
              e.exception_type = ExceptionType::Unhandled;
              callbacks_->OnException(received_msg.remote_id, e);
            }
          }
#endif

          turn_processed = false;
        }
      }

      if (!processed) {
        msg_content.Seek(saved_read_pos);

        if (callbacks_) {
          ++event_processed_count;

          callbacks_->OnNoRpcProcessed(rpc_id);
        }
      }
    }
    break;

  case FinalUserWorkItemType::FreeformMessage: {
      auto& received_msg = uwi.unsafe_message;
      auto& msg_content = received_msg.unsafe_message;

      LockMain_AssertIsNotLockedByCurrentThread();

      void* host_tag = GetHostTag(received_msg.remote_id);
      {
        // Receive user-defined message.
        if (callbacks_) {
          try {
            OptimalCounter32 payload_length;
            if (!lf::Read(msg_content, payload_length) || payload_length != msg_content.GetReadableLength()) {
              SharedPtr<ResultInfo> result_info(new ResultInfo);
              result_info->result_code = ResultCode::InvalidPacketFormat;
              result_info->comment = String::Format("Invalid payload size in user message.  payload_length: %d, readable_length: %d", (int32)payload_length, msg_content.GetReadableLength());
              EnqueueError(result_info);
            }
            else {
              // one thread model인 클라라서 안전
              callbacks_->bHolsterMoreCallback_FORONETHREADEDMODEL = false;
              callbacks_->bPostponeThisCallback_FORONETHREADEDMODEL = false;

              RpcHint rpc_hint;
              rpc_hint.relayed = received_msg.relayed;
              rpc_hint.host_tag = host_tag;

              //TODO 읽기 가능한 데이터를 CByteString으로 변환해서 넘겨줘야함.
              callbacks_->OnReceiveFreeform(received_msg.remote_id, rpc_hint, ByteArray((const char*)msg_content.GetReadableData(), msg_content.GetReadableLength())); // copy

              //TODO 아래에 있는 것들을 제거하는게 좋을듯함...
              out_holster_more_callback |= callbacks_->bHolsterMoreCallback_FORONETHREADEDMODEL;
              out_postpone_this_callback |= callbacks_->bPostponeThisCallback_FORONETHREADEDMODEL;

              ++rpc_processed_count;
            }
          }
          catch (Exception& e) {
            if (callbacks_) {
              callbacks_->OnException(received_msg.remote_id, e);
            }
          }
          catch (std::exception& e) {
            if (callbacks_) {
              callbacks_->OnException(received_msg.remote_id, Exception(e));
            }
          }
          //catch (_com_error& e) {
          //  if (callbacks_) {
          //    callbacks_->OnException(received_msg.remote_id, Exception(e));
          //  }
          //}
          //catch (void* e) {
          //  if (callbacks_) {
          //    callbacks_->OnException(received_msg.remote_id, Exception(e));
          //  }
          //}
#ifdef ALLOW_CATCH_UNHANDLED_EVEN_FOR_USER_ROUTINE
          catch (...) {
            if (callbacks_) {
              Exception e;
              e.exception_type = ExceptionType::Unhandled;
              callbacks_->OnException(received_msg.remote_id, e);
            }
          }
#endif
        }
      }
    }
    break;

  case FinalUserWorkItemType::LocalEvent: {
      LockMain_AssertIsNotLockedByCurrentThread();

      if (callbacks_) {
        const LocalEvent& event = uwi.event;

        try {
          // one thread model인 클라라서 안전
          callbacks_->bHolsterMoreCallback_FORONETHREADEDMODEL = false;
          callbacks_->bPostponeThisCallback_FORONETHREADEDMODEL = false;

          bool processed = true;

          switch (event.type) {
          case LocalEventType::ConnectServerSuccess: {
              //TODO event.UserData가 sharable하지 않을런지??
              callbacks_->OnJoinedToServer(SharedPtr<ResultInfo>(new ResultInfo()).Get(), event.user_data);
            }
            break;

          case LocalEventType::ConnectServerFail: {
              auto result_info = ResultInfo::From(event.result_info->result_code, HostId_Server, "");
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
            DecreaseLeaveEventCount(event.member_id);
            break;

          case LocalEventType::DirectP2PEnabled:
            callbacks_->OnP2PStateChanged(event.remote_id, ResultCode::Ok);
            break;

          case LocalEventType::RelayP2PEnabled:
            callbacks_->OnP2PStateChanged(event.remote_id, event.result_info->result_code);
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

          case LocalEventType::ServerUdpChanged:
            callbacks_->OnServerUdpStateChanged(event.result_info->result_code);
            break;

          default:
            processed = false;
            return;
          }

          out_holster_more_callback = callbacks_->bHolsterMoreCallback_FORONETHREADEDMODEL;
          out_postpone_this_callback = callbacks_->bPostponeThisCallback_FORONETHREADEDMODEL;

          if (processed) {
            ++event_processed_count;
          }
        }
        catch (Exception& e) {
          callbacks_->OnException(event.remote_id, e);
        }
        catch (std::exception& e) {
          callbacks_->OnException(event.remote_id, Exception(e));
        }
        //catch (_com_error& e) {
        //  callbacks_->OnException(event.remote_id, Exception(e));
        //}
        //catch (void* e) {
        //  callbacks_->OnException(event.remote_id, Exception(e));
        //}
#ifdef ALLOW_CATCH_UNHANDLED_EVEN_FOR_USER_ROUTINE
        catch (...) {
          Exception e;
          e.exception_type = ExceptionType::Unhandled;
          callbacks_->OnException(event.remote_id, e);
        }
#endif
      }
    }
    break;
  }
}

bool NetClientImpl::PopFinalUserWorkItem(FinalUserWorkItem& out_item) {
  CScopedLock2 main_guard(GetMutex());

  if (!final_user_work_queue_.IsEmpty()) {
    out_item = final_user_work_queue_.Dequeue();
    return true;
  }

  return false;
}

HostId NetClientImpl::GetSrcHostIdByAddrAtDestSide_NOLOCK(const InetAddress& addr) {
  LockMain_AssertIsLockedByCurrentThread();

  if (!addr.IsUnicast()) {
    return HostId_None;
  }

  if (to_server_udp_fallbackable_ &&
      to_server_udp_fallbackable_->server_addr_.IsUnicast() &&
      to_server_udp_fallbackable_->server_addr_ == addr) {
    return HostId_Server;
  }

  // for loopback case
  if (Get_ToServerUdpSocketAddrAtServer() == addr) {
    return local_host_id_;
  }

  for (const auto& pair : remote_peers_) {
    const auto& peer = pair.value;

    if (peer->p2p_holepunched_remote_to_local_addr_ == addr) {
      return peer->host_id_;
    }
  }

  return HostId_None;
}

void NetClientImpl::RequestReceiveSpeedAtReceiverSide_NoRelay(const InetAddress& dst) {
  CScopedLock2 main_guard(GetMutex());

  if (Get_ToServerUdpSocket() && !Get_ToServerUdpSocket()->IsSocketClosed()) {
    // Request the UDP reception rate from the server side.
    MessageOut msg_to_send;
    lf::Write(msg_to_send, MessageType::RequestReceiveSpeedAtReceiverSide_NoRelay);

    const auto filter_tag = FilterTag::Make(GetLocalHostId(), HostId_Server); // Local -> Server
    const UdpSendOption send_opt(MessagePriority::Ring1, EngineOnlyFeature);
    Get_ToServerUdpSocket()->SendWhenReady(HostId_Server, filter_tag, dst, msg_to_send, GetAbsoluteTime(), send_opt);
  }
}

int32 NetClientImpl::GetOverSendSuspectingThresholdInByte() {
  return settings_.over_send_suspecting_threshold_in_byte;
}

// delay processing, will be processed in the next turn.
// (Just put it in the back of the queue to induce recurrence.)
void NetClientImpl::PostponeFinalUserWorlItem(FinalUserWorkItem& uwi) {
  CScopedLock2 main_guard(GetMutex());

  postponed_final_user_work_item_list_.Enqueue(uwi);
  uwi.unsafe_message.unsafe_message.Seek(0); //혹시 몰라서??
}

void NetClientImpl::Tick_PullPostponeeToFinalQueue() {
  CScopedLock2 main_guard(GetMutex());

  while (!postponed_final_user_work_item_list_.IsEmpty()) {
    final_user_work_queue_.Enqueue(postponed_final_user_work_item_list_.Front());
    postponed_final_user_work_item_list_.RemoveFront();
  }
}

void NetClientImpl::CleanupEvenUnstableSituation(bool clear_work_items) {
  LockMain_AssertIsLockedByCurrentThread();

  // Clean up everything you need in an abnormal situation.
  to_server_udp_fallbackable_.Reset();

  // If there is a socket with an issue, ask the manager to wait until the issue disappears and then delete it
  if (to_server_tcp_ && (to_server_tcp_->send_issued_ || to_server_tcp_->recv_issued_)) {
    manager_->HasOverlappedIoToGarbage(to_server_tcp_);

    if (IsIntraLoggingOn()) {
      IntraLogToServer(LogCategory::TCP, "CleanupEvenUnstableSituation, CloseSocketHandleOnly() called.");
    }
  }

  if (to_server_udp_socket_ && (to_server_udp_socket_->send_issued_ || to_server_udp_socket_->recv_issued_)) {
    manager_->HasOverlappedIoToGarbage(to_server_udp_socket_);
  }

  to_server_tcp_.Reset();
  to_server_udp_socket_.Reset();

  AllClearRecycleToGarbage();

  for (auto it = garbages_.CreateIterator(); it; ++it) {
    auto& socket = *it;

    if (socket_->send_issued_ || socket_->recv_issued_) {
      manager_->HasOverlappedIoToGarbage(socket);
    }

    garbages_.Remove(it);
  }

  garbages_.Clear();

  // No event should happen after Disconnect.
  // because disconnect means to stop the NetClient.
  if (clear_work_items) {
    final_user_work_queue_.Clear();
  }

  // 추가로 클린업해야 할것들을 한다.
  self_p2p_session_key_.Reset();
  self_encrypt_count_ = 0;
  self_decrypt_count_ = 0;

  suppress_subsequent_disconnection_events_ = false;
  request_server_time_count_ = 0;
  dx_server_time_diff_ = 0;
  server_udp_recent_ping_ = 0;
  server_udp_last_ping_ = 0;
  last_request_server_time_time_ = 0;
  p2p_groups_.Clear(); // 확인사살.
  p2p_connection_attempt_end_time_ = NetConfig::GetP2PHolepunchEndTime();
  p2p_holepunch_interval_ = NetConfig::p2p_holepunch_interval_;
  last_report_udp_count_time_ = NetConfig::report_real_udp_count_interval_sec;
  nat_device_name_detected_ = false;
  stats_.Reset();
  internal_version_ = NetConfig::INTERNAL_NET_VERSION;
  settings_.Reset();
  server_instance_tag_ = Uuid::None;
  pre_final_recv_queue.Clear();
  to_server_encrypt_count_ = 0;
  to_server_decrypt_count_ = 0;
  to_server_udp_socket_failed_ = false;
  lookback_final_received_message_queue_.Clear();
  //min_extra_ping_ = 0;
  //extra_ping_variance_ = 0;
  local_host_id_ = HostId_None;
  connection_args_ = NetConnectionArgs();

  postponed_final_user_work_item_list_.Clear();

  more_precision_clock_.Stop();
  more_precision_clock_.Reset(); //여기서 reset은 왜하지???

  last_tick_invoked_time_ = 0;
  remote_peers_.Clear();
  peer_garbages_.Clear();
  conditional_remove_too_old_udp_send_packet_queue_alarm_ = IntervalAlaram(NetConfig::udp_packet_board_long_interval_sec);
  process_send_ready_remotes_alarm_ = IntervalAlaram(NetConfig::every_remote_issue_send_on_need_interval_sec);
  virtual_speed_hack_multiplication_ = 1;
  speedhack_detect_ping_cooltime_ = NetConfig::speedhack_detector_enabled_by_default ? 0 : NetConfig::INFINITE_COOLTIME;
  last_tcp_stream_recv_time_ = 0;
  test_stats2_ = TestStats2(); //TODO 제거해야지??
  application_hint_.recent_frame_rate = 0;
  last_check_send_queue_time_ = 0;
  last_update_net_client_stat_clone_time_ = 0;
  send_queue_heavy_started_time_ = 0;

  tcp_and_udp_short_tick_last_time_ = 0;

  unused_udp_ports_.Clear();
  used_udp_ports_.Clear();
}

#ifdef TEST_DISCONNECT_CLEANUP
bool NetClientImpl::IsAllCleanup() {
  if (to_server_udp_fallbackable_) {
    return false;
  }

  if (to_server_tcp_) {
    return false;
  }

  if (self_p2p_session_key_) {
    return false;
  }

  if (self_decrypt_count_ != 0 || self_encrypt_count_ != 0) {
    return false;
  }

  if (suppress_subsequent_disconnection_events_) {
    return false;
  }

  if (request_server_time_count_ != 0) {
    return false;
  }
  if (dx_server_time_diff_ != 0) {
    return false;
  }
  if (server_udp_recent_ping_ != 0) {
    return false;
  }
  if (server_udp_last_ping_ != 0) {
    return false;
  }

  if (last_request_server_time_time_ != 0) {
    return false;
  }

  if (p2p_groups_.Count() != 0) {
    return false;
  }
  if (p2p_connection_attempt_end_time_ != NetConfig::GetP2PHolepunchEndTime()) {
    return false;
  }

  if (p2p_holepunch_interval_ !=  NetConfig::p2p_holepunch_interval_) {
    return false;
  }

  if (nat_device_name_detected_) {
    return false;
  }

  if (internal_version_ != NetConfig::internal_version_) {
    return false;
  }

  if (server_instance_tag_ != Uuid::None) {
    return false;
  }

  if (pre_final_recv_queue.Count() != 0) {
    return false;
  }

  if ((to_server_encrypt_count_ | to_server_decrypt_count_) != 0) {
    return false;
  }

  if (to_server_udp_socket_) {
    return false;
  }

  if (to_server_udp_socket_failed_) {
    return false;
  }

  if (!lookback_final_received_message_queue_.IsEmpty()) {
    return false;
  }

  //if (min_extra_ping_ != 0) {
  //  return false;
  //}

  if (extra_ping_variance_ != 0) {
    return false;
  }

  if (local_host_id_ != HostId_None) {
    return false;
  }

  if (!postponed_final_user_work_item_list_.IsEmpty()) {
    return false;
  }

  if (!more_precision_clock_.IsStopped()) {
    return false;
  }

  if (last_tick_invoked_time_ != 0) {
    return false;
  }

  if (!remote_peers_.IsEmpty()) {
    return false;
  }

  if (!garbages_.IsEmpty()) {
    return false;
  }

  if (virtual_speed_hack_multiplication_ != 1) {
    return false;
  }

  if (last_tcp_stream_recv_time_ != 0) {
    return false;
  }

  return true;
}
#endif


void NetClientImpl::EnableVizAgent( const char* viz_server_ip,
                                    int32 viz_server_port,
                                    const String& login_key) {
  //TODO
  //if (!viz_agent_) {
  //  viz_agent_.Reset(new VizAgent(this, viz_server_ip, viz_server_port, login_key));
  //}
}

void NetClientImpl::Viz_NotifySendByProxy(const HostId* sendto_list,
                                          int32 sendto_count,
                                          const MessageSummary& summary,
                                          const RpcCallOption& rpc_call_opt) {
  //TODO
  //if (viz_agent_) {
  //  Array<HostId> targets;
  //  targets.ResizeUninitialized(sendto_count);
  //  UnsafeMemory::Memcpy(targets.MutableData(), sendto_list, sizeof(HostId) * sendto_count);
  //
  //  //TODO 구지 TArray로 변환해서 전송해야만 하나?? raw는 IDL에서 처리못하려나... 흠...
  //  viz_agent_->c2s_proxy_.NotifyCommon_SendRpc(HostId_Server, GReliableSend_INTERNAL, targets, summary);
  //}
}

void NetClientImpl::Viz_NotifyRecvToStub( HostId sender,
                                          RpcId rpc_id,
                                          const char* rpc_name,
                                          const char* params_as_string) {
  //TODO
  //if (viz_agent_) {
  //  viz_agent_->c2s_proxy_.NotifyCommon_ReceiveRpc(HostId_Server, GReliableSend_INTERNAL, sender, rpc_name, rpc_id);
  //}
}

bool NetClientImpl::New_ToServerUdpSocket() {
  // If udp socket creation fails even once, no attempt is made to create it.
  if (to_server_udp_socket_failed_) {
    return false;
  }

  if (!to_server_udp_socket_) {
    try {
      //to_server_udp_socket_ = IHasOverlappedIoPtr(new UdpSocket_C(this, nullptr, 0/*connection_args_.LocalPort*/));
      to_server_udp_socket_.Reset(new UdpSocket_C(this, nullptr));

      InetAddress udp_local_addr = Get_ToServerTcp()->local_addr;

      if (!udp_local_addr.GetHost().IsUnicast()) {
        ErrorReporter::Report(String::Format("FATAL: New_ToServerUdpSocket - UDP 소켓을 생성하기 전에 TCP 연결이 이미 되어있는 상태이어야 하는데, %s가 나왔다.", *udp_local_addr.ToString()));

        to_server_udp_socket_.Reset();
        to_server_udp_socket_failed_ = true;
        EnqueueWarning(ResultInfo::From(ResultCode::LocalSocketCreationFailed, GetLocalHostId(), "UDP socket for server connection."));
        return false;
      }

      if (!CreateUdpSocket(Get_ToServerUdpSocket(), udp_local_addr)) {
        to_server_udp_socket_.Reset();
        to_server_udp_socket_failed_ = true;
        EnqueueWarning(ResultInfo::From(ResultCode::LocalSocketCreationFailed, GetLocalHostId(), "UDP socket for server connection."));
        return false;
      }
    } catch (Exception& e) {
      if (callbacks_) {
        callbacks_->OnException(HostId_None, e);
      }

      to_server_udp_socket_.Reset();
      to_server_udp_socket_failed_ = true;
      return false;
    }

    // issue UDP first recv
    ((UdpSocket_C*)to_server_udp_socket_.Get())->ConditionalIssueRecvFrom();
  }

  return true;
}

// 서버에게 UDP 소켓을 준비해줄 것을 요청함.
// 단, 바로 처리되는것은 아니므로 플래그를 표시하고 대기함.
// IMPLEMENT_RPCSTUB_NetS2C_S2C_CreateUdpSocketAck를 수신하게 되면,
// 서버에서 정상적으로 생성이 완료된것임.
void NetClientImpl::RequestServerUdpSocketReady_FirstTimeOnly() {
  // Request to create a udp socket.
  if (to_server_udp_socket_ &&
      to_server_udp_fallbackable_->server_udp_ready_waiting_ == false &&
      settings_.fallback_method <= FallbackMethod::PeersUdpToTcp &&
      to_server_udp_socket_failed_ == false) {
    c2s_proxy_.C2S_RequestCreateUdpSocket(HostId_Server, GReliableSend_INTERNAL);

    to_server_udp_fallbackable_->server_udp_ready_waiting_ = true;
  }
}

void NetClientImpl::GetNetWorkerThreadInfo(ThreadInfo& out_info) {
  out_info.thread_handle = manager_->GetWorkerThreadHandle();
  out_info.thread_id = manager_->GetWorkerThreadId();
}

// LocalEvent가 아니면, 지워버리는 함수인데, 이게 왜 필요한지???
// 현재는 사용되고 있지 않음.
//
// 접속 종료처리시, 접속해제를 방해하는 메시지를 지우고
// 로컬 이벤트만 처리하게 하기 위함일듯...
void NetClientImpl::FinalUserWorkItemList_RemoveReceivedMessagesOnly() {
  LockMain_AssertIsLockedByCurrentThread();

  for (auto it = final_user_work_queue_.CreateIterator(); it; ++it) {
    const auto& item = *it;

    if (item.type != FinalUserWorkItemType::LocalEvent) {
      final_user_work_queue_.Remove(it);
    }
  }
}

void NetClientImpl::UpdateP2PGroup_MemberJoin(
    HostId group_id,
    HostId member_id,
    const ByteArray& custom_field,
    uint32 event_id,
    FrameNumber p2p_first_frame_number,
    const Uuid& connection_tag,
    const ByteArray& p2p_aes_session_key,
    const ByteArray& p2p_rc4_session_key,
    bool direct_p2p_enabled,
    int32 bind_port,
    bool real_udp_enabled) {
  LockMain_AssertIsLockedByCurrentThread();

  // Should be ignored if the connection to the server is broken.
  if (!worker || worker_->GetState() != NetClientWorker::State::Connected) {
    return;
  }

  // Create or Update P2P group
  // 이 그룹은 local이 소속된 그룹이기도 하다. 어차피 이 RPC는 local이 소속된 그룹에 대해서 호출되니까.
  auto group = GetP2PGroupByHostId_INTERNAL(group_id);
  if (!group) {
    group = CreateP2PGroupObject_INTERNAL(group_id);
  }

  bool local_port_reuse_success = false;

  // Server인경우와 아닌경우를 나눈다.
  if (member_id != HostId_Server) {
    // create or update the peer
    auto member_rp = GetPeerByHostId(member_id);
    if (local_host_id_ != member_id) // RPs {
      if (!member_rp) {
        member_rp.Reset(new RemotePeer_C(this));
        member_rp->host_id_ = member_id;
        member_rp->holepunch_tag_ = connection_tag;

        // 이 값은 생성될 때만 적용해야지, 이미 RP가 있는 상태에서는 오버라이드하지 않는다.
        // 이미 Direct P2P를 맺고있으면 그것이 우선이니까.
        member_rp->direct_p2p_enabled_ = direct_p2p_enabled;

        remote_peers_.Add(member_rp->host_id_, member_rp);

        // 생성된 remote peer는 P2P relay이다.
        member_rp->SetRelayedP2P();

        member_rp->real_udp_enabled_ = real_udp_enabled; //ADDED: 아직 서버에 추가 안되었고, 프로토콜도 일부 변경해야함.

        // assign session key and first frame number of reliable UDP

        // String encryption key. (AES)
        if (!p2p_aes_session_key.IsEmpty()) {
          if (!CryptoAES::ExpandFrom(member_rp->p2p_session_key_.aes_key, (const uint8*)p2p_aes_session_key.ConstData(), settings_.strong_encrypted_message_key_length / 8)) {
            throw Exception("Failed to create session key");
          }
        } else {
          // Non strong encryption key.
          member_rp->p2p_session_key_.aes_key.Reset();
        }

        // Weak encryption key. (RC4)
        if (!p2p_rc4_session_key.IsEmpty()) {
          if (!CryptoRC4::ExpandFrom(member_rp->p2p_session_key_.rc4_key, (const uint8*)p2p_rc4_session_key.ConstData(), settings_.weak_encrypted_message_key_length / 8)) {
            throw Exception("Failed to create session_key_");
          }
        } else {
          // Non weak encryption key.
          member_rp->p2p_session_key_.rc4_key.key_exists = true;
        }

        member_rp->to_peer_rudp_.ResetEngine(p2p_first_frame_number);

        if (bind_port != 0 && direct_p2p_enabled) {
          local_port_reuse_success = member_rp->NewUdpSocketBindPort(bind_port);
        }
      } else if (member_rp->garbaged_) { //garbage에 들어가 있는경우.... 처음부터 세팅을 하고, garbage에서 삭제한다.
        member_rp->InitGarbage(this);
        member_rp->host_id_ = member_id;
        member_rp->holepunch_tag_ = connection_tag;

        // 이 값은 생성될 때만 적용해야지, 이미 RP가 있는 상태에서는 오버라이드하지 않는다. 이미 Direct P2P를 맺고있으면 그것이 우선이니까.
        member_rp->direct_p2p_enabled_ = direct_p2p_enabled;

        // 생성된 remote peer는 P2P relay이다.
        member_rp->SetRelayedP2P();

        member_rp->real_udp_enabled_ = real_udp_enabled;

        // assign session key and first frame number of reliable UDP

        // Strong encryption key. (AES)
        if (!p2p_aes_session_key.IsEmpty()) {
          if (!CryptoAES::ExpandFrom(member_rp->p2p_session_key_.aes_key, (const uint8*)p2p_aes_session_key.ConstData(), settings_.strong_encrypted_message_key_length / 8)) {
            throw Exception("Failed to create session key");
          }
        } else {
          // Non strong encryption key.
          member_rp->p2p_session_key_.Reset();
        }

        // Strong encryption key. (RC4)
        if (!p2p_rc4_session_key.IsEmpty()) {
          if (!CryptoRC4::ExpandFrom(member_rp->p2p_session_key_.rc4_key, (const uint8*)p2p_rc4_session_key.ConstData(), settings_.weak_encrypted_message_key_length / 8)) {
            throw Exception("Failed to create session key");
          }
        } else {
          // Non weak encryption key.
          member_rp->p2p_session_key_.rc4_key.key_exists = true;
        }

        member_rp->to_peer_rudp_.ResetEngine(p2p_first_frame_number);

        if (bind_port != 0 && direct_p2p_enabled) {
          local_port_reuse_success = member_rp->NewUdpSocketBindPort(bind_port);
        }

        member_rp->garbaged_ = false;

        peer_garbages_.Remove(member_id);
      }

      // update peer's joined groups
      member_rp->joined_p2p_groups_.Add(group->group_id_, group);

      group->members_.Add(member_id, member_rp.Get());
    } else { // local
      group->members_.Add(member_id, this);
    }
  } else { // server
    // Add the server to the group.
    group->members_.Add(member_id, &server_as_send_dest_);
  }

  // P2P-member-add-ack RPC with received event time
  c2s_proxy_.P2PGroup_MemberJoin_Ack(HostId_Server, GReliableSend_INTERNAL, group_id, member_id, event_id, local_port_reuse_success);

  // P2PAddMember
  LocalEvent event(LocalEventType::P2PAddMember);
  event.group_id = group_id;
  event.member_id = member_id;
  event.remote_id = member_id;
  event.member_count = group->members_.Count();
  event.custom_field = custom_field; //note: 구태여 사본을 가져갈 필요는 없음. just share it!
  EnqueueLocalEvent(event);
}

void NetClientImpl::SetApplicationHint(const ApplicationHint& hint) {
  application_hint_ = hint;
}

void NetClientImpl::GetApplicationHint(ApplicationHint& out_hint) {
  out_hint = application_hint_;
}

void NetClientImpl::AssociateSocket(InternalSocket* socket) {
  manager_->completion_port_->AssociateSocket(socket);
}

bool NetClientImpl::SetHostTag(HostId host_id, void* host_tag) {
  CScopedLock2 main_guard(GetMutex());

  if (host_id == HostId_Server) {
    server_as_send_dest_.host_tag_ = host_tag;
    return true;
  } else if (host_id == local_host_id_) {
    host_tag_ = host_tag;
    return true;
  }

  auto peer = GetPeerByHostId(host_id);
  if (peer && !peer->garbaged_) {
    peer->host_tag_ = host_tag;
    return true;
  }

  return false;
}

void* NetClientImpl::GetHostTag(HostId host_id) {
  CScopedLock2 main_guard(GetMutex());

  if (host_id == HostId_Server) {
    return server_as_send_dest_.host_tag_;
  } else if (host_id == local_host_id_) {
    return host_tag_;
  }

  auto peer = GetPeerByHostId(host_id);
  if (peer && !peer->garbaged_) {
    return peer->host_tag_;
  }

  return nullptr;
}

InetAddress NetClientImpl::GetTcpLocalAddr() {
  CScopedLock2 main_guard(GetMutex());

  return Get_ToServerTcp() ? Get_ToServerTcp()->local_addr_ : InetAddress::None;
}

InetAddress NetClientImpl::GetUdpLocalAddr() {
  CScopedLock2 main_guard(GetMutex());

  return Get_ToServerUdpSocket() ? Get_ToServerUdpSocket()->local_addr_ : InetAddress::None;
}

void NetClientImpl::CheckSendQueue() {
  const double absolute_time = GetAbsoluteTime();

  //TODO 시간 측정하는 Resetable stop watch를 구현해서 사용하는게 바람직하지 않을런지??

  if (Get_ToServerTcp() &&
      (absolute_time - last_check_send_queue_time_) > NetConfig::send_queue_heavy_warning_check_cooltime_sec) {
    //TODO
    //TCP / UDP 각각 나누어서 체크하는게 좀더 구체적이지 않을까 싶은데??
    //현재는 TCP / UDP 통계를 합산하여 처리하고 있음.

    int32 length = Get_ToServerTcp()->send_queue_.GetLength();

    if (Get_ToServerUdpSocket()) {
      length += Get_ToServerUdpSocket()->packet_fragger_->FromTotalPacketInByteByAddr(to_server_udp_fallbackable_->server_addr_);
    }

    if (send_queue_heavy_started_time_ != 0) {
      if (length > NetConfig::send_queue_heavy_warning_capacity) {
        if ((absolute_time - send_queue_heavy_started_time_) > NetConfig::send_queue_heavy_warning_time_sec) {
          send_queue_heavy_started_time_ = absolute_time;

          const String text = String::Format("%d bytes in send queue", Length);
          EnqueueWarning(ResultInfo::From(ResultCode::SendQueueIsHeavy, HostId_Server, text));
        }
      } else {
        send_queue_heavy_started_time_ = 0;
      }
    } else if (length > NetConfig::send_queue_heavy_warning_capacity) {
      send_queue_heavy_started_time_ = absolute_time;
    }

    last_check_send_queue_time_ = absolute_time;
  }
}

void NetClientImpl::UpdateNetClientStatClone() {
  CScopedLock2 main_guard(GetMutex());

  // emergency log가 활성화되어있을때만 처리
  if (settings_.emergency_log_line_count > 0 &&
      worker_->GetState() < NetClientWorker::State::Disconnecting) {
    const double absolute_time = GetAbsoluteTime();

    if ((absolute_time - last_update_net_client_stat_clone_time_) > NetConfig::update_net_client_stat_clone_cooltime_sec) {
      last_update_net_client_stat_clone_time_ = absolute_time;

      GetStats(recent_backedup_stats_);
    }
  }
}

// Now, this UDP socket will no longer issue any transmission issues and will be ignored even if the reception is completed.
// UdpSocketPtr_C should be placed anywhere that you assign it to avoid the risk of being sensitive to future updates.
void NetClientImpl::GarbageSocket(IHasOverlappedIoPtr socket) {
  socket_->OnCloseSocketAndMakeOrphant();

  garbages_.Append(socket);
}

void NetClientImpl::UdpSocketToRecycleBin(IHasOverlappedIoPtr udp_socket) {
  auto assigned_udp = (UdpSocket_C*)(udp_socket.Get());

  fun_check(recycles_.Contains(assigned_udp->local_addr_.GetPort()) == false);
  recycles_.Add(assigned_udp->local_addr_.GetPort(), udp_socket);

  assigned_udp->owner_peer_ = nullptr;
  assigned_udp->recycle_binned_time_ = GetAbsoluteTime();
  ///assigned_udp->ResetPacketFragState();
}

void NetClientImpl::AllClearRecycleToGarbage() {
  for (auto& pair : recycles_) {
    GarbageSocket(pair.value);
  }

  recycles_.Clear();
}

void NetClientImpl::GetWorkerState(ClientWorkerInfo& out_info) {
  CScopedLock2 main_guard(GetMutex());

  out_info.is_worker_thread_null = worker.IsValid();
  out_info.is_worker_thread_ended = manager_->thread_ended_; //TODO thread_ended_ 멤버변수는 없앨는게 깔끔해보임. 함수하나 노출해주는게 좋을듯..
  out_info.disconnect_call_count = disconnection_invoke_count_.GetValue();
  out_info.final_work_item_count = final_user_work_queue_.Count();
  out_info.peer_count = remote_peers_.Count();
  out_info.connect_call_count = connect_count_.GetValue();
  out_info.current_time = GetAbsoluteTime();
  out_info.last_tcp_stream_time = last_tcp_stream_recv_time_;
  out_info.worker_thread_id = 0;

  if (!out_info.is_worker_thread_ended) {
    out_info.worker_thread_id = manager_->GetWorkerThreadId();
  }

  out_info.ConnectionState = GetServerConnectionState();
}

void NetClientImpl::AllClearGarbagePeer() {
  peer_garbages_.Clear();
}

void NetClientImpl::CheckCriticalSectionDeadLock_INTERNAL(const char* where) {
}

bool NetClientImpl::GetSocketInfo(HostId remote_id, SocketInfo& out_info) {
  CScopedLock2 main_guard(GetMutex());

  out_info.tcp_socket = INVALID_SOCKET;
  out_info.udp_socket = INVALID_SOCKET;

  if (remote_id == HostId_None) {
    return true;
  }

  // TCP

  // The TCP connection to the server must exist by default.
  if (Get_ToServerTcp() && Get_ToServerTcp()->socket) {
    out_info.tcp_socket = Get_ToServerTcp()->socket_->socket_;
  } else {
    return false;
  }

  // UDP

  if (HostId_Server == remote_id) { // server
    if (Get_ToServerUdpSocket() && Get_ToServerUdpSocket()->socket) {
      out_info.udp_socket = Get_ToServerUdpSocket()->socket_->socket_;
      return true;
    }
  } else if (auto peer = GetPeerByHostId(remote_id)) { // RPs
    if (peer->IsDirectP2P() && peer->Get_ToPeerUdpSocket() && peer->Get_ToPeerUdpSocket()->socket_) {
      // In case of direct P2P
      out_info.udp_socket = peer->Get_ToPeerUdpSocket()->socket_->socket_;
      return true;
    } else if (peer->IsRelayedP2P() && Get_ToServerUdpSocket() && Get_ToServerUdpSocket()->socket_) {
      // In case of relayed P2P.
      out_info.udp_socket = Get_ToServerUdpSocket()->socket_->socket_;
      return true;
    }
  }

  return false;
}


//TODO Emergency logging은 빼던지, http 형태로 처리하는 방법을 생각해보자...
//     로그를 항동안 보관하는 기능이 있어서 이또한 감안을 해주어야할듯...

void NetClientImpl::AddEmergencyLogList(LogCategory category, const String& text) {
  // should be done under lock.
  LockMain_AssertIsLockedByCurrentThread();

  if (settings_.emergency_log_line_count == 0) {
    return;
  }

  EmergencyLogData::Log log;
  log.category = category;
  log.text = text;
  log.added_time = DateTime::Now();

  emergency_log_data.log_list.Append(log);

  // limitation.
  if (emergency_log_data.log_list.Count() > settings_.emergency_log_line_count) {
    emergency_log_data.log_list.RemoveFront();
  }
}

//TODO 제거하거나 http로 처리하도록 하자.
bool NetClientImpl::SendEmergencyLogData(const String& server_ip, int32 server_port) {
  fun_check(0);
  return false;
}

//bool NetClientImpl::SendEmergencyLogData(const String& server_ip, int32 server_port) {
//  CScopedLock2 main_guard(GetMutex());
//
//  auto log_data = new EmergencyLogData();
//
//  // NetClient 가 날라갈것을 대비해 지역으로 데이터를 옮기자.
//
//  // 기존걸 복사하자.
//  *log_data = EmergencyLogData;
//
//  // LogData의 나머지를 채운다.
//  log_data->host_id = backuped_host_id_;
//  log_data->total_tcp_issued_send_bytes = total_tcp_issued_send_bytes_;
//  log_data->io_pending_count = io_pending_count_;
//  log_data->logon_time = DateTime::UtcNow();
//  log_data->connect_count = connect_count_.Value();
//  log_data->conn_reset_error_count = g_conn_reset_error_count.Value();
//  log_data->msg_size_error_count = g_msg_size_error_count.Value();
//  log_data->net_reset_error_count = g_net_reset_error_count.Value();
//  log_data->direct_p2p_enable_peer_count = recent_backedup_stats_.direct_p2p_enabled_peer_count;
//  log_data->remote_peer_count = recent_backedup_stats_.remote_peer_count;
//  log_data->total_tcp_recv_bytes = recent_backedup_stats_.total_tcp_recv_bytes;
//  log_data->total_tcp_send_bytes = recent_backedup_stats_.total_tcp_send_bytes;
//  log_data->total_udp_recv_count = recent_backedup_stats_.total_udp_recv_count;
//  log_data->total_udp_send_bytes = recent_backedup_stats_.total_udp_send_bytes;
//  log_data->total_udp_send_count = recent_backedup_stats_.total_udp_send_count;
//  log_data->nat_device_name = GetNatDeviceName();
//
//  SYSTEM_INFO si;
//  ::GetSystemInfo(&si);
//  log_data->processor_architecture = si.wProcessorArchitecture;
//
//  //@fixme deprecation issue
//  //http://stackoverflow.com/questions/22303824/warning-c4996-getversionexw-was-declared-deprecated
//  OSVERSIONINFOEX os_vi;
//  ::ZeroMemory(&os_vi, sizeof(OSVERSIONINFOEX));
//  os_vi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
//  ::GetVersionEx((OSVERSIONINFO*)&os_vi);
//
//  log_data->os_major_version = os_vi.dwMajorVersion;
//  log_data->os_minor_version = os_vi.dwMinorVersion;
//  log_data->product_type = os_vi.wProductType;
//
//  log_data->server_ip = server_ip;
//  log_data->server_port = server_port;
//
//  // WT_EXECUTELONGFUNCTION 플래그의 경우에는 포스팅된 작업이 다소 오래걸릴 수 있는 작업이므로,
//  // 내부적으로 스레드를 더 할당하여 처리할 수 있도록 힌트를 제공하는 역활을 함.
//  // 궁극적으로는 Http로 처리하는 쪽이 나아보임..
//  ::QueueUserWorkItem(RunEmergencyLogClient, (PVOID)log_data, WT_EXECUTELONGFUNCTION);
//
//  return true;
//}
//
//DWORD WINAPI NetClientImpl::RunEmergencyLogClient(void* context) {
//  EmergencyLogData* log_data = (EmergencyLogData*)context;
//
//  EmergencyLogClient* log_client = new EmergencyLogClient();
//
//  try {
//    log_client->Start(log_data->server_ip, log_data->server_port, log_data);
//
//    while (log_client->GetState() != EmergencyLogClient::State::Stopped) {
//      // polling.
//      log_client->Tick();
//      CPlatformProcess::Sleep(0.01f);
//    }
//  } catch (Exception& e) {
//    // platform issue.
//    ::OutputDebugString(e.What());
//  }
//
//  // Client를 먼저삭제해야 LogData의 개런티 보장
//  delete log_client;
//
//  // log_data 삭제
//  delete log_data;
//
//  return 0;
//}

bool NetClientImpl::CreateUdpSocket(UdpSocket_C* udp_socket, InetAddress& ref_udp_local_addr) {
  // Create a socket with the UDP port specified by the user.
  bool creation_ok = false;
  for (auto it = unused_udp_ports_.CreateIterator(); it; ++it) {
    ref_udp_local_addr.SetPort(*it); // One of the ports assigned to NetConnectionArgs::local_udp_port_pool is assigned.

    if (udp_socket->CreateSocket(ref_udp_local_addr)) {
      // Put it in the list of used UDP ports.
      used_udp_ports_.Add(ref_udp_local_addr.GetPort());
      //TODO iterator로 제거해도 될듯?
      unused_udp_ports_.Remove(ref_udp_local_addr.GetPort());
      creation_ok = true;
      break;
    }
  }

  // If all user udp port fails, specify it arbitrarily.
  if (!creation_ok) {
    ref_udp_local_addr.SetPort(0); // ANY: 할당받음...
    creation_ok = udp_socket->CreateSocket(ref_udp_local_addr);

    // UDP 포트 풀이 지정되었지만 실패해서, any 포트로 바인딩할 경우, 경고 메시지로 알려줍니다.
    const int32 num_ports_in_port_pool = used_udp_ports_.Count() + unused_udp_ports_.Count();
    const bool is_port_pool_specified = (num_ports_in_port_pool > 0);
    if (is_port_pool_specified) {
      // If the user specified a UDP port, it will report failure.
      const String text = String::Format("UDP port-pool is specified but use arbitrary port number used: %d", udp_socket->local_addr_.GetPort());
      EnqueueWarning(ResultInfo::From(ResultCode::NoneAvailableInPortPool, GetLocalHostId(), text));
    }
  }
  fun_check(creation_ok);
  return creation_ok;
}

void NetClientImpl::SetDefaultTimeoutTimeSec(double timeout_sec) {
  CScopedLock2 main_guard(GetMutex());

  if (timeout_sec < 1) {
    if (callbacks_) {
      LOG(LogNetEngine, Warning, "Too short timeout value. it may cause unfair disconnection.");
      return;
    }
  }
#ifndef _DEBUG
  if (timeout_sec > 240) {
    if (callbacks_) {
      //LOG(LogNetEngine, Warning, "Too long timeout value. it may take a lot of time to detect lost connection.");
    }
  }
#endif

  CheckDefaultTimeoutTimeValidation(timeout_sec);
  settings_.default_timeout_sec = timeout_sec;
}

// UPnP Router is detected, TCP connection is established, all internal
// and external addresses are recognized
// If you have not done so yet, put the TCP port mapping on UPnP based on
// the hole-punched information.
void NetClientImpl::ConditionalAddUPnPTcpPortMapping() {
  if (!settings_.upnp_tcp_addr_port_mapping) {
    return;
  }

  if (upnp_tcp_port_mapping_state_) {
    return;
  }

  if (!HasServerConnection()) {
    return;
  }

  if (!manager_->upnp_) {
    return;
  }

  // You do not need to do this if the server and client is on the same LAN or is a real IP. (Not necessarily)
  if (!Get_ToServerTcp()->local_addr_.IsUnicast() ||
      !Get_ToServerTcp()->local_addr_at_server_.IsUnicast()) {
    return;
  }

  // If there is no behind the NAT, then the hole punching is meaningless, so skip it.
  // (If not behind NAT, internal / external IP is the same.)
  if (!IsBehindNAT()) {
    return;
  }

  if (manager_->upnp_->GetCommandAvailableNatDevice() == nullptr) {
    return;
  }

  upnp_tcp_port_mapping_state_.Reset(new UPnPTcpPortMappingState);
  upnp_tcp_port_mapping_state_->lan_addr_ = Get_ToServerTcp()->local_addr_;
  upnp_tcp_port_mapping_state_->wan_addr_ = Get_ToServerTcp()->local_addr_at_server_;

  manager_->upnp_->AddTcpPortMapping(upnp_tcp_port_mapping_state_->lan_addr_, upnp_tcp_port_mapping_state_->wan_addr_, true);
}

// Returns true if it finds groupHostId containing any one.
bool NetClientImpl::GetIntersectionOfHostIdListAndP2PGroupsOfRemotePeer(
        const HostIdArray& sorted_host_id_list,
        RemotePeer_C* peer,
        HostIdArray* out_subset_group_host_id_list) {
  bool result = false;

  // This object itself is reused, so clean
  out_subset_group_host_id_list->Clear();

  // For each P2P group to which the remote peer belongs
  //for (auto it = peer->joined_p2p_groups_.begin(); it != peer->joined_p2p_groups_.end(); ++it) {
  //  // Do a binary search and add it to the list if it is in the host ID list.
  //  if (std::binary_search(sorted_host_id_list.GetData(), sorted_host_id_list.GetData() + sorted_host_id_list.Count(), it->key)) {
  //    out_subset_group_host_id_list->Add(it->key);
  //    result = true;
  //  }
  //}

  //TODO binary-search 구현.
  for (const auto& pair : peer->joined_p2p_groups_) {
    if (sorted_host_id_list.Contains(pair.key)) {
      out_subset_group_host_id_list->Add(pair.key);
      result = true;
    }
  }

  return result;
}

void NetClientImpl::ConditionalDeleteUPnPTcpPortMapping() {
  if (upnp_tcp_port_mapping_state_ == nullptr) {
    return;
  }

  if (HasServerConnection()) {
    return;
  }

  if (!manager_->upnp_) {
    return;
  }

  manager_->upnp_->DeleteTcpPortMapping(upnp_tcp_port_mapping_state_->lan_addr_, upnp_tcp_port_mapping_state_->wan_addr_, true);

  upnp_tcp_port_mapping_state_.Reset();
}

// Make sure the local host is behind the router
bool NetClientImpl::IsBehindNAT() {
  if (Get_ToServerTcp() &&
      Get_ToServerTcp()->local_addr != Get_ToServerTcp()->local_addr_at_server_) {
    return true;
  }

  return false;
}

// Get the total number of group id and host id in it.
// compressed relay dst for size estimation before serializing list.
int32 NetClientImpl::CompressedRelayDestList_C::GetAllHostIdCount() {
  int32 count = p2p_group_list.Count(); // group host_id count must also be included in the number.
  for (const auto& pair : p2p_group_list) {
    count += pair.value.excludee_host_id_list.Count();
  }

  count += includee_host_id_list.Count();

  return count;
}

// Add the individual to be excluded from the P2P group and its group. Add it only if there is no group yet.
void NetClientImpl::CompressedRelayDestList_C::AddSubset(const HostIdArray& subset_group_host_id, HostId host_id) {
  for (int32 subset_index = 0; subset_index < subset_group_host_id.Count(); ++subset_index) {
    // 없으면 새 항목을 넣고 있으면 그걸 찾는다.
    auto& subset = p2p_group_list.FindOrAdd(subset_group_host_id[subset_index]);

    if (host_id != HostId_None) {
      subset.excludee_host_id_list.Add(host_id);
    }
  }
}

void NetClientImpl::CompressedRelayDestList_C::AddIndividual(HostId host_id) {
  includee_host_id_list.Add(host_id);
}

bool NetClientImpl::RunAsync(HostId task_owner_id, Function<void()> func) {
  //TODO
  fun_check(0);
  return false;
}


//
// NetClientStats
//

NetClientStats::NetClientStats() {
  Reset();
}

void NetClientStats::Reset() {
  server_udp_enabled = false;
  remote_peer_count = 0;
  direct_p2p_enabled_peer_count = 0;
  total_tcp_recv_bytes = 0;
  total_tcp_send_bytes = 0;
  total_udp_recv_bytes = 0;
  total_udp_recv_count = 0;
  total_udp_send_bytes = 0;
  total_udp_send_count = 0;
}

String NetClientStats::ToString() const {
  return String::Format("{server_udp_enabled: %d, remote_peer_count: %d, direct_p2p_enabled_peer_count: %d, total_udp_recv_bytes: %I64d}",
          server_udp_enabled, remote_peer_count, direct_p2p_enabled_peer_count, total_udp_recv_bytes);
}


//
// INetClientCallbacks
//

void INetClientCallbacks::HolsterMoreCallbackUntilNextTick() {
  bHolsterMoreCallback_FORONETHREADEDMODEL = true;
}

void INetClientCallbacks::PostponeThisCallback() {
  bPostponeThisCallback_FORONETHREADEDMODEL = true;
}

} // namespace net
} // namespace fun
