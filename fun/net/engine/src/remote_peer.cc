#include "fun/net/net.h"
#include "NetClient.h"
#include "RemotePeer.h"
#include "PacketFrag.h"
#include "RpcCallOptionImpl.h"
#include "UdpSocket_C.h"
#include "RUdpHost.h"

#include "ReportError.h"

namespace fun {
namespace net {

using lf = LiteFormat;

bool P2PHolepunchAttemptContext::Heartbeat() {
  total_elapsed_time_ += GetClient()->GetElapsedTime();

  // 너무 긴 시간동안 홀펀칭을 시도 했지만, 여전히 이루어지지 않고 있는 상황입니다.
  // 이럴 경우에는 더 시도해봐야 홀펀칭이 이루어질 가능성이 없으므로, 홀펀칭 시도를 중지하도록 합니다.
  if (total_elapsed_time_ > owner_->owner_->p2p_connection_attempt_end_time_) {
    return false;
  }

  switch (state_->type) {
    case StateType::ServerHolepunch: {
        auto s = (ServerHolepunchState*)state_.Get();

        // 필요시 서버에 홀펀칭을 시도한다.
        s->send_cooltime_ -= GetClient()->GetElapsedTime();

        if (s->send_cooltime_ < 0) {
          s->send_cooltime_ += NetConfig::server_holepunch_interval_sec;

          MessageOut msg_to_send;
          lf::Write(msg_to_send, MessageType::PeerUdp_ServerHolepunch);
          lf::Write(msg_to_send, s->holepunch_tag_);
          lf::Write(msg_to_send, owner_->host_id_);

          const auto filter_tag = FilterTag::Make(owner_->owner_->GetLocalHostId(), HostId_Server); // Local -> Server
          const UdpSendOption send_opt(MessagePriority::Holepunch, EngineOnlyFeature);
          owner_->Get_ToPeerUdpSocket()->SendWhenReady(
              HostId_Server,
              filter_tag,
              GetServerUdpAddr(), // recvfrom에서 저장해둔 주소? (서버와의 홀펀칭이므로 서버의 UDP 주소)
              msg_to_send,
              GetClient()->GetAbsoluteTime(),
              send_opt);
        }
      }
      break;

    case StateType::PeerHolepunch: {
        // P2PShotgunStartTurn가 지나치게 짧으면 너무 섣불리 샷건을 쏘게 되고 과량 홀펀칭 매핑으로 이어지므로
        fun_check((NetConfig::p2p_shotgun_start_turn * 3) >= NetConfig::p2p_holepunch_max_turn_count);

        auto s = (PeerHolepunchState*)state_.Get();

        // 필요시 P2P 홀펀칭 쏘기
        s->send_cooltime_ -= GetClient()->GetElapsedTime();
        if (s->send_cooltime_ < 0) {
          s->send_cooltime_ += GetClient()->p2p_holepunch_interval_sec;
          s->send_turn_++;

          // Send magic number and each of ABSendAddrs via address ABSendAddrs, repeat:
          // for every one seconds until 10 seconds
          //
          // ABSendAddrs are:
          // B's internal address
          // B's punched address by server

          fun_check(owner_->udp_addr_from_server_.IsUnicast());
          fun_check(owner_->udp_addr_internal_.IsUnicast());

          SendPeerHolepunch(owner_->udp_addr_from_server_, owner_->holepunch_tag_, "udp_addr_from_server");

          // 헤어핀이 없는 공유기를 위해 내부랜끼리 홀펀칭.
          // IsSameLan 만으로는 부족하다. 외부 IP가 서로 같은 경우에만 해야 한다.
          // 안그러면 어떤 공유기(혹은 피어의 시스템)는 알수없는 호스트에 패킷 송신을 감행하고 있다는
          // 이유만으로 공유기 선에서 호스트를 막아버릴 수 있을테니까.
          fun_check(owner_->udp_addr_from_server_.IsUnicast());
          fun_check(owner_->Get_ToPeerUdpSocket()->local_addr.IsUnicast());

          if (owner_->udp_socket_ &&
              owner_->Get_ToPeerUdpSocket()->here_addr_at_server_.host_() == owner_->udp_addr_from_server_.host_() &&
              NetUtil::IsSameSubnet24(owner_->udp_addr_internal_.host_(), owner_->Get_ToPeerUdpSocket()->local_addr_.host_())) {
            SendPeerHolepunch(owner_->udp_addr_internal_, owner_->holepunch_tag_, "SameLAN");
          }

          // 포트 예측
          // IPTIME 때문에 바로 쏘지 말자
          if (s->send_turn_ > NetConfig::p2p_shotgun_start_turn) {
            s->offset_shotgun_countdown_--;

            if (s->offset_shotgun_countdown_ < 0) {
              s->offset_shotgun_countdown_ = NetConfig::shotgun_attempt_count;
              s->shotgun_min_port_ += (uint16)NetConfig::shotgun_range;
              s->shotgun_min_port_ = AdjustUdpPortNumber(s->shotgun_min_port_);
            }

            InetAddress shotgun_to = owner_->udp_addr_from_server_;
            shotgun_to.SetPort(s->shotgun_min_port_);
            for (int32 shotgun_index = 0; shotgun_index < NetConfig::shotgun_range; ++shotgun_index) {
              SendPeerHolepunch(shotgun_to, owner_->holepunch_tag_, "Shotgun");
              shotgun_to.SetPort(AdjustUdpPortNumber(shotgun_to.GetPort() + 1));
            }
          }
        }
      }
      break;
  }

  return true;
}


P2PHolepunchAttemptContext::p2p_holepunch_attempt_context_(RemotePeer_C* owner)
  : owner_(owner)
  , total_elapsed_time_(0) {
  fun_check(owner_->host_id_ != owner_->owner_->local_host_id_);
  state_.Reset(new ServerHolepunchState);
}


#define SP2P_LOG_ERROR \
  if (main->IsIntraLoggingOn())  { \
    main->IntraLogToServer(LogCategory::SP2P, *String::Format("Failed to nat-punchthrough1: %s(%d)", __FILE__, __LINE__)); \
  }

#define PP2P_LOG_ERROR \
  if (main->IsIntraLoggingOn())  { \
    main->IntraLogToServer(LogCategory::PP2P, *String::Format("Failed to nat-punchthrough1: %s(%d)", __FILE__, __LINE__)); \
  }

void P2PHolepunchAttemptContext::ProcessPeerHolepunch(NetClientImpl* main,
                                                      ReceivedMessage& received_msg) {
  // 이쪽이 B측이다. 저쪽은 A고.
  auto& msg = received_msg.unsafe_message;

  HostId host_a_id;
  Uuid holepunch_tag_;
  Uuid server_instance_tag;
  InetAddress a2b_send_addr; // 저쪽에서 이쪽으로 송신할 때 쓴 주소
  if (!lf::Reads(msg,  host_a_id, holepunch_tag_, server_instance_tag, a2b_send_addr)) {
    PP2P_LOG_ERROR;
    return;
  }

  // 서버가 1개 호스트에서 여럿 작동중일때 같은 HostId지만 서로 다른 클라로부터 holepunch가 올 수 있다.
  // 이런 경우 서버 인스턴스 guid 및 group member join 여부도 검사해서 불필요한 에코를 하지 않아야
  // 저쪽에서 잘못된 홀펀칭 성사를 막을 수 있다.
  if (server_instance_tag != main->server_instance_tag_) {
    PP2P_LOG_ERROR;
    return;
  }

  const InetAddress a2b_recv_addr = received_msg.remote_addr_udp_only; // 저쪽으로부터 받을 때 이 주소로 받으면 된다.

  auto rp = main->GetPeerByHostId(host_a_id);
  if (!rp || rp->garbaged_ || !rp->p2p_holepunch_attempt_context_) {
    PP2P_LOG_ERROR;
    return;
  }

  // remote peer를 찾았지만 그 remote peer와 사전에 합의된 (서버를 통해 받은) connection magic number와 다르면
  // 에코를 무시한다. 이렇게 해야 잘못된 3rd remote의 hole punch 요청에 대해 엉뚱한 hole punch success를 막을 수 있다.
  fun_check(rp->holepunch_tag_ != Uuid::None);
  fun_check(holepunch_tag_ != Uuid::None);
  if (holepunch_tag_ != rp->holepunch_tag_) {
    PP2P_LOG_ERROR;
    return;
  }

  if (main->IsIntraLoggingOn()) {
    const String text = String::Format("Received P2P holepunch.  a2b_send_addr: %s, a2b_recv_addr: %s", *a2b_send_addr.ToString(), *a2b_recv_addr.ToString());
    main->IntraLogToServer(LogCategory::P2P, *text);
  }

  // maybe B receives A's magic number via address a2b_recv_addr
  // we can conclude that
  //   A can send to B via selected one of ABSendAddrs
  //   B can choose A's messages if it is received from a2b_recv_addr
  //
  // THUS; send magic number ack / selected a2b_send_addr / a2b_recv_addr / each one of BASendAddrs via address BASendAddrs, once
  //
  // BASendAddrs are:
  //   A's internal address
  //   A's punched address for server
  // a2b_recv_addr

  // #ifdef PER_PEER_SOCKET_DEBUG
  //  if (rp->udp_addr_internal_ == InetAddress::None) {
  //    ShowErrorBox("rp->udp_addr_internal_ != InetAddress::None");
  //  }
  //  if (rp->udp_addr_from_server_ == InetAddress::None) {
  //    ShowErrorBox("rp->udp_addr_from_server_ == InetAddress::None");
  //  }
  //  if (received_msg.remote_addr_udp_only == InetAddress::None) {
  //    ShowErrorBox("received_msg.remote_addr_udp_only");
  //  }
  // #endif

  //fun_check(rp->udp_socket_->socket_->GetSockName().IsUnicast());

  // 아직 서버에서 받지 못한 상태일수도 있으므로, 체크가 필요하나 에러는 아니고 그냥 확인 차원..
  //if (!rp->udp_addr_from_server_.IsUnicast()) {
  //  //@todo
  //  //FUN_TRACE("*** udp_addr_from_server.IsUnicast() == false! 그러나 가끔 나오면 문제없음 ***\n");
  //}

  //fun_check(rp->udp_addr_from_server_.IsUnicast());// 무조건 fail하면 문제지만, UDP의 특성상 뒷북이 있을 수 있으므로 가끔 fail하면 무시해도 된다.

  if (rp->udp_addr_internal_.IsUnicast() &&  // 아직 서버로부터 이걸 갱신하기 위한 값이 안왔을 수 있으므로 이 조건문 필요
      rp->Get_ToPeerUdpSocket()->local_addr.IsUnicast() &&
      rp->Get_ToPeerUdpSocket()->here_addr_at_server_.GetHost() == rp->udp_addr_from_server_.GetHost() &&
      NetUtil::IsSameSubnet24(rp->udp_addr_internal_.GetHost(), rp->Get_ToPeerUdpSocket()->local_addr.GetHost())) {
    rp->p2p_holepunch_attempt_context_->SendPeerHolepunchAck(rp->udp_addr_internal_, holepunch_tag_, a2b_send_addr, a2b_recv_addr, "SameLAN");
  }

  //TODO UdpAddrFromServer와 RemoteAddr_OnlyUdp이 어떤 경우에 다른걸까??  어떤 경우라기 보다는 왜 다를까??

  if (rp->udp_addr_from_server_.IsUnicast()) { // 아직 서버로부터 이걸 갱신하기 위한 값이 안왔을 수 있으므로 이 조건문 필요
    rp->p2p_holepunch_attempt_context_->SendPeerHolepunchAck(rp->udp_addr_from_server_, holepunch_tag_, a2b_send_addr, a2b_recv_addr, "udp_addr_from_server");
  }

  if (received_msg.remote_addr_udp_only != rp->udp_addr_from_server_) {
    rp->p2p_holepunch_attempt_context_->SendPeerHolepunchAck(received_msg.remote_addr_udp_only, holepunch_tag_, a2b_send_addr, a2b_recv_addr, "remote_addr_udp_only");
  }
}

void P2PHolepunchAttemptContext::ProcessPeerHolepunchAck( NetClientImpl* main,
                                                          ReceivedMessage& received_msg) {
  auto& msg = received_msg.unsafe_message;

  // ** 이쪽이 A측 입장이고 저쪽은 B측 입장이다.

  // Maybe A receives the ack via address b2a_recv_addr
  Uuid        holepunch_tag;
  HostId      remote_id;
  InetAddress a2b_send_addr;
  InetAddress a2b_recv_addr;
  InetAddress b2a_send_addr;
  if (!lf::Reads(msg,  holepunch_tag, remote_id, a2b_send_addr, a2b_recv_addr, b2a_send_addr)) {
    //TRACE_SOURCE_LOCATION();
    return;
  }

  const InetAddress& b2a_recv_addr = received_msg.remote_addr_udp_only;

  auto rp = main->GetPeerByHostId(remote_id);
  if (rp.IsValid() == false ||
      rp->garbaged_ ||
      rp->p2p_holepunch_attempt_context_.IsValid() == false) {
    //rp->P2PConnectionTrialContext가 아직 할당 안되어있는 경우가 자주 나오므로, 로깅은 무의미함.

    //TRACE_SOURCE_LOCATION();
    //
    //if (rp.IsValid() == false) {
    //  LOG(LogNetEngine,Info,"rp.IsValid()=0");
    //}
    //else {
    //  LOG(LogNetEngine,Info,"rp.IsValid()=1, rp->garbaged_=%d, rp->p2p_holepunch_attempt_context_.IsValid()=%d",
    //          (int32)rp->garbaged_, (int32)rp->p2p_holepunch_attempt_context_.IsValid());
    //}
    return;
  }

  // Symmetric NAT 포트 예측을 하는 경우 HostId까지는 매치되지만 정작 magic number가 안맞을 수 있다.
  // 한 컴퓨터에 여러 서버가 띄워져있으면 HostId가 중복될테니까.
  // 이것까지 체크해야 한다.
  fun_check(rp->holepunch_tag_ != Uuid::None);

  if (rp->holepunch_tag_ != holepunch_tag) {
    //TRACE_SOURCE_LOCATION();
    return;
  }

  if (rp->p2p_holepunch_attempt_context_->state_.IsValid() == false ||
      rp->p2p_holepunch_attempt_context_->state_->type_ != StateType::PeerHolepunch) {
    //TRACE_SOURCE_LOCATION();
    return;
  }

  // 사용되지 않는 지역변수 주석
  //PeerHolepunchState* state = (PeerHolepunchState*)rp->p2p_holepunch_attempt_context_->state_.Get();

  // we can conclude that
  // B can send to A via selected one of b2a_send_addr
  // A can choose B's message if it is received from b2a_recv_addr
  //
  // send 'P2P holepunching ok' to server
  //
  // now we are interested to:
  // selected one of ABSendAddrs
  // a2b_recv_addr
  // selected one of b2a_send_addr
  // b2a_recv_addr
  // ...with the interested values below

  // symmetric NAT 포트 예측을 하는 경우 중복해서 대량으로 holepunch 2 success가 올 수 있으므로
  // 더 이상 받지 않도록 무시 처리해야 한다.
  // 또한 다른 P2P 연결 시도를 모두 막아야 한다. 안그러면 NAT 장치에서 낮은 확률로
  // 같은 endpoint에 대한 서로다른 port mapping이 race condition이 되어버리기 때문이다.
  rp->p2p_holepunch_attempt_context_.Reset();

  // 양측 모두 이걸 해야 한다. 따라서 상대측에서 못하게 하는 명령을 보내도록 한다.
  // 강제로 릴레이를 해야 한다. 그것도 reliable send로.
  RpcCallOption force_relay_send_opt = GReliableSend_INTERNAL;
  force_relay_send_opt.max_direct_p2p_multicast_count = 0; // 강제 relay
  main->c2c_proxy_.SuppressP2PHolepunchTrial(rp->host_id_, force_relay_send_opt);

  // P2P verify 과정을 거치지 말고, 바로 P2P 홀펀칭 성공을 노티한다.
  main->c2s_proxy_.NotifyP2PHolepunchSuccess(
        HostId_Server,
        GReliableSend_INTERNAL,
        main->local_host_id_,
        rp->host_id_,
        a2b_send_addr,
        a2b_recv_addr,
        b2a_send_addr,
        b2a_recv_addr);

  if (main->IsIntraLoggingOn()) {
    const String text = String::Format("PeerHolepunchAck OK.  a2b_send_addr: %s, a2b_recv_addr: %s, b2a_send_addr: %s, b2a_recv_addr: %s",
            *a2b_send_addr.ToString(), *a2b_recv_addr.ToString(),
            *b2a_send_addr.ToString(), *b2a_recv_addr.ToString());
    main->IntraLogToServer(LogCategory::P2P, *text);
  }
}

NetClientImpl* P2PHolepunchAttemptContext::GetClient() {
  return owner_->owner_;
}

InetAddress P2PHolepunchAttemptContext::GetExternalAddr() {
  return owner_->udp_addr_from_server_;
}

InetAddress P2PHolepunchAttemptContext::GetInternalAddr() {
  return owner_->udp_addr_internal_;
}

int32 P2PHolepunchAttemptContext::AdjustUdpPortNumber(int32 port) {
  if (port < 1023 || port > 65534) {
    port = 1023;
  }

  return port;
}

InetAddress P2PHolepunchAttemptContext::GetServerUdpAddr() {
  if (owner_->owner_->to_server_udp_fallbackable_) {
    return owner_->owner_->to_server_udp_fallbackable_->server_addr_;
  } else {
    return InetAddress::None;
  }
}

// TODO ReceivedMessage& received_msg 메시지는 넘길필요 없음.
//
// UDP 주소만 체크용으로 넘겨주면 될듯...
void P2PHolepunchAttemptContext::ProcessMessage_PeerUdp_ServerHolepunchAck(
    ReceivedMessage& received_msg,
    const Uuid& holepunch_tag,
    const InetAddress& here_addr_at_server,
    HostId peer_id) {
  if (!state_ || state_->type_ != StateType::ServerHolepunch) {
    //TRACE_SOURCE_LOCATION();
    return;
  }

  auto s = (ServerHolepunchState*)state_.Get();

  // 다른곳에서 온경우 무시.
  if (holepunch_tag != s->holepunch_tag_) {
    //TRACE_SOURCE_LOCATION();
    return;
  }

  // 이미 받았다면, 무시.
  if (s->ack_recv_count_ > 0) {
    //TRACE_SOURCE_LOCATION();
    return;
  }

  if (GetServerUdpAddr() != received_msg.remote_addr_udp_only) {
    //TRACE_SOURCE_LOCATION();
    return;
  }

  if (!owner_->udp_socket_) {
    //TRACE_SOURCE_LOCATION();
    return;
  }

  // 받은 응답 횟수를 증가시킴. (중복 여부를 판단하기 위함)
  s->ack_recv_count_++;

  fun_check(owner_->Get_ToPeerUdpSocket()->local_addr.IsUnicast());
  fun_check(here_addr_at_server_.IsUnicast());
  owner_->Get_ToPeerUdpSocket()->here_addr_at_server_ = here_addr_at_server;

  // 홀펀칭이 성공했을 경우의 해당 피어의 클라이언트 주소를 서버로 알려준다. (서버와 연결된 tcp transport를 통해서)
  MessageOut msg_to_send;
  lf::Write(msg_to_send, MessageType::PeerUdp_NotifyHolepunchSuccess);
  lf::Write(msg_to_send, owner_->Get_ToPeerUdpSocket()->local_addr_);
  lf::Write(msg_to_send, here_addr_at_server);
  lf::Write(msg_to_send, owner_->host_id_);
  GetClient()->Get_ToServerTcp()->SendWhenReady(SendFragRefs(msg_to_send), TcpSendOption());

  if (GetClient()->IsIntraLoggingOn()) {
    const String text = String::Format("Message_PeerUdp_ServerHolepunchAck.  addr_of_here_at_server: %s", *here_addr_at_server.ToString());
    GetClient()->IntraLogToServer(LogCategory::P2P, *text);
  }
}

void P2PHolepunchAttemptContext::SendPeerHolepunch( const InetAddress& a2b_send_addr,
                                                    const Uuid& holepunch_tag,
                                                    const char* debug_hint) {
  MessageOut msg_to_send;
  lf::Write(msg_to_send, MessageType::PeerUdp_PeerHolepunch);
  lf::Write(msg_to_send, GetClient()->local_host_id_);
  lf::Write(msg_to_send, holepunch_tag);
  lf::Write(msg_to_send, GetClient()->server_instance_tag_);
  lf::Write(msg_to_send, a2b_send_addr);
  const auto filter_tag = FilterTag::Make(owner_->owner_->GetLocalHostId(), owner_->host_id_); // Local -> Peer
  const UdpSendOption send_opt(MessagePriority::Holepunch, EngineOnlyFeature);
  owner_->Get_ToPeerUdpSocket()->SendWhenReady(owner_->host_id_, filter_tag, a2b_send_addr, msg_to_send, GetClient()->GetAbsoluteTime(), send_opt);

  if (GetClient()->intra_logging_on_) {
    const String text = String::Format("Sending peer-holepunch to client %d(addr: %s), case: %s", (int32)owner_->host_id_, *a2b_send_addr.ToString(), debug_hint);
    GetClient()->IntraLogToServer(LogCategory::P2P, *text);
  }
}

void P2PHolepunchAttemptContext::SendPeerHolepunchAck(
    const InetAddress& b2a_send_addr,
    const Uuid& holepunch_tag,
    const InetAddress& a2b_send_addr,
    const InetAddress& a2b_recv_addr,
    const char* debug_hint) {
  MessageOut msg_to_send;
  lf::Write(msg_to_send, MessageType::PeerUdp_PeerHolepunchAck);
  lf::Write(msg_to_send, holepunch_tag); // 그대로 echo한다.
  lf::Write(msg_to_send, GetClient()->local_host_id_);
  lf::Write(msg_to_send, a2b_send_addr);
  lf::Write(msg_to_send, a2b_recv_addr);
  lf::Write(msg_to_send, b2a_send_addr);
  const auto filter_tag = FilterTag::Make(owner_->owner_->GetLocalHostId(), owner_->host_id_); // Local -> Peer
  const UdpSendOption send_opt(MessagePriority::Holepunch, EngineOnlyFeature);
  owner_->Get_ToPeerUdpSocket()->SendWhenReady(owner_->host_id_, filter_tag, b2a_send_addr, msg_to_send, GetClient()->GetAbsoluteTime(), send_opt);

  if (GetClient()->IsIntraLoggingOn()) {
    const String text = String::Format("Trying to PeerHolepunchAck for %s.  a2b_send_addr: %s, a2b_recv_addr: %s, b2a_send_addr: %s",
                debug_hint, *a2b_send_addr.ToString(), *a2b_recv_addr.ToString(), *b2a_send_addr.ToString());
    GetClient()->IntraLogToServer(LogCategory::P2P, *text);
  }
}

void RemotePeer_C::GetPeerInfo(PeerInfo& out_result) {
  out_result.host_id = host_id_;
  out_result.udp_addr_from_server = udp_addr_from_server_;
  out_result.udp_addr_internal = udp_addr_internal_;
  out_result.recent_ping = recent_ping_;
  out_result.send_queued_amount_in_byte = send_queued_amount_in_byte_;

  out_result.joined_p2p_groups_.Clear(joined_p2p_groups_.Count());
  for (const auto& pair : joined_p2p_groups_) {
    out_result.joined_p2p_groups_.Add(pair.key);
  }

  //@maxidea: todo
  // 현재 peer의 real-udp enabled 상태를 알수가 없다.  서버에서는 이미 알고 있지만..
  // 전파를 안해줘서 알수가 없다.
  // 헌데 이걸 클라이언트에서 꼭 알아야만 하나?

  //out_result.real_udp_enabled = IsRealUdpEnabled();
  //별도의 패킷을 만들어서 전파를 해야하나?
  //아니면 별도로 처리해야하나?
  //전파를 시키려면, 보낼 대상을 추려내기가 마땅치가 않아보이는데...
  //
  // 서버에서 그룹에 참여할때나
  // 이미 참여한 그룹이 있을 경우에, 서버와의 홀펀칭이 되었을 경우에 전파를 시켜주면 될듯 싶은데...

  out_result.real_udp_enabled = real_udp_enabled_;
  out_result.relayed_p2p = IsRelayedP2P();
  out_result.behind_nat = IsBehindNAT();
  out_result.host_tag = host_tag_;
  out_result.direct_p2p_peer_frame_rate = recent_frame_rate_;
  out_result.to_remote_peer_send_udp_message_attempt_count = to_remote_peer_send_udp_message_attempt_count_;
  out_result.to_remote_peer_send_udp_message_success_count = to_remote_peer_send_udp_message_success_count_;
}

RemotePeer_C::RemotePeer_C(NetClientImpl* owner)
  : to_peer_rudp_(this),
    to_peer_udp_(this),
    host_id_(HostId_None) {
  fun_check_ptr(owner);
  InitGarbage(owner);

  leave_event_count_ = 0;
  garbaged_ = false;
}

void RemotePeer_C::InitGarbage(NetClientImpl* owner) {
  fun_check_ptr(owner);

  owner_ = owner;

  direct_p2p_enabled_ = P2PGroupOption::Default.direct_p2p_enabled;

  jit_direct_p2p_needed_ = (owner_->settings_.direct_p2p_start_condition == DirectP2PStartCondition::Always);
  jit_direct_p2p_triggered_ = false;

  member_join_process_end_ = false;

  //send_stop_acked_ = false;
  //recv_stop_acked_ = false;

  p2p_holepunch_attempt_context_.Reset();

  restore_needed_ = false;
  new_p2p_connection_needed_ = false;

  last_ping_ = 0;
  recent_frame_rate_ = 0;
  peer_to_server_ping_ = 0;

  p2p_holepunched_local_to_remote_addr_ = InetAddress::None;
  p2p_holepunched_remote_to_local_addr_ = InetAddress::None;
  udp_addr_from_server_ = InetAddress::None;
  udp_addr_internal_ = InetAddress::None;
  is_relayed_p2p_ = true;
  relayed_p2p_disabled_time_ = 0;

  real_udp_enabled_ = false;

  //@todo 아니 왜 랜덤으로 처리하지??
  sync_indirect_server_time_diff_cooltime_ = MathBase::Lerp(NetConfig::p2p_ping_interval_sec*0.5, NetConfig::p2p_ping_interval_sec, owner_->random_.NextDouble());
  indirect_server_time_diff = 0; // 이 값은 정상이 아니다. GetIndirectServerTime에서 교정될 것이다.

  last_direct_udp_packet_recv_time_ = owner_->GetAbsoluteTime();
  direct_udp_packet_recv_count_ = 0;
  last_udp_packet_recv_interval_ = -1;

  recent_ping_ = 0;
  send_queued_amount_in_byte_ = 0;
  last_ping_send_time_ = 0;

  encrypt_count_ = 0;
  decrypt_count_ = 0;

  repunch_count_ = 0;
  repunch_started_time_ = 0;

  to_remote_peer_send_udp_message_attempt_count_ = 0;
  to_remote_peer_send_udp_message_success_count_ = 0;
  receive_udp_message_success_count_ = 0;

  last_check_send_queue_time_ = 0;
  send_queue_heavy_start_time_ = 0;

  host_tag_ = nullptr;

  udp_socket_creation_time_ = GetRenewalSocketCreationTime();

  to_peer_rudp_heartbeat_last_time_ = 0;
  to_peer_report_server_time_and_ping_last_time_ = 0;

  // 처음엔 relay이므로 true
  set_to_relayed_but_last_ping_is_not_calculated_yet_ = true;
}

void RemotePeer_C::CreateP2PHolepunchAttemptContext() {
  p2p_holepunch_attempt_context_.Reset(new P2PHolepunchAttemptContext(this));
}

void RemotePeer_C::Heartbeat(double absolute_time) {
  if ((absolute_time - to_peer_rudp_heartbeat_last_time_) > NetConfig::rudp_heartbeat_interval_sec) {
    to_peer_rudp_heartbeat_last_time_ = absolute_time;

    to_peer_rudp_.Heartbeat();
  }

  // 생성 후 일정 시간이 지나면 UDP 소켓 준비

  const FallbackMethod fallback_method = owner_->settings_.fallback_method;
  if (fallback_method != FallbackMethod::PeersUdpToTcp &&
      fallback_method != FallbackMethod::CloseUdpSocket &&
      fallback_method != FallbackMethod::ServerUdpToTcp &&
      direct_p2p_enabled_ &&
      member_join_process_end_) {
    if (jit_direct_p2p_needed_ == true &&
        jit_direct_p2p_triggered_ == false &&
        udp_socket_.IsValid() == false &&
        owner_->GetAbsoluteTime() > udp_socket_creation_time_) {
      jit_direct_p2p_triggered_ = true;
      owner_->c2s_proxy_.NotifyJitDirectP2PTriggered(HostId_Server, GReliableSend_INTERNAL, host_id);
    }

    // 유저가 Restore를 요청했다면 역시 소켓 재준비

    ConditionalRestoreUdpSocket();

    ConditionalNewUdpSocket();
  }

  if (p2p_holepunch_attempt_context_) {
    const bool alive = p2p_holepunch_attempt_context_->Heartbeat();
    if (!alive) {
      // cancel nat-punchthroughing
      p2p_holepunch_attempt_context_.Reset();
    }
  }

  // 일정 시간 마다 클라간에 서버 시간 및 frame_rate 및 Ping을 동기화 하자.
  //TODO IntervalAlarm을 사용해도 좋을듯 싶은데..
  if ((absolute_time - to_peer_report_server_time_and_ping_last_time_) > NetConfig::report_server_time_and_ping_interval_sec) {
    ApplicationHint hint;
    owner_->GetApplicationHint(hint);

    owner_->c2c_proxy_.ReportServerTimeAndFrameRateAndPing(
          host_id_,
          GReliableSend_INTERNAL,
          owner_->more_precision_clock_.AbsoluteSeconds(), // Ping 측정용이므로, Heartbeat마다 갱신되는 시간 AbsoluteTime보다는 real-time클럭을 사용해야함.
          hint.recent_frame_rate);

    to_peer_report_server_time_and_ping_last_time_ = absolute_time;
  }

  // 일정 시간내로 P2P 연결간 ping이 전혀 없으면 릴레이 모드로 전환한다.
  if (IsRelayConditionByUdpFailure(absolute_time)) {
    //TODO 제거해야함.
    LOG(LogNetEngine,Warning,"Switch P2PUDP -> Relayed");
    FallbackP2PToRelay(true, ResultCode::P2PUdpFailed);
  }
  else if (IsRelayConditionByRUdpFailure()) {
    //TODO 제거해야함.
    //TODO 전환 후 통신 데이터에 문제가 생기는것 같다.
    //ReliableUDP를 통해서 보내던걸 릴레이를 통해서 보내게 될터인데, 이때 프레임이 깨지는듯 싶다...
    //LOG(LogNetEngine,Warning,"Switch ReliableUDP -> Relayed");
    FallbackP2PToRelay(true, ResultCode::RUdpFailed);
  }

  // P2P RePunch를 필요시 한다.
  if (IsRelayedP2P() &&
      repunch_started_time_ > 0 &&
      absolute_time > repunch_started_time_ &&
      udp_socket_.IsValid() &&
      Get_ToPeerUdpSocket()->socket_->IsClosed() == false) {
    repunch_started_time_ = 0; // 더 이상 할 필요가 없다. 또 fallback을 한 상황이 발생하기 전까지는.
    CreateP2PHolepunchAttemptContext();
  }

  if (udp_socket_ && (absolute_time - last_check_send_queue_time_) > NetConfig::send_queue_heavy_warning_check_cooltime_sec) {
    const int32 length = Get_ToPeerUdpSocket()->packet_fragger_->FromTotalPacketInByteByAddr(p2p_holepunched_local_to_remote_addr_);

    //TODO sendwindow에 적재된것도 고려해야하지 않을런지??
    //현재 sendwindow에 수십메가 이상 쌓여있는 상태에서도 경고가 노출되지 않고 있음.

    if (send_queue_heavy_start_time_ != 0) {
      if (length > NetConfig::send_queue_heavy_warning_capacity) {
        if ((absolute_time - send_queue_heavy_start_time_) > NetConfig::send_queue_heavy_warning_time_sec) {
          send_queue_heavy_start_time_ = absolute_time;
          owner_->EnqueueWarning(ResultInfo::From(ResultCode::SendQueueIsHeavy, host_id, String::Format("send_queue_ %dbytes", length)));
        }
      } else {
        send_queue_heavy_start_time_ = 0;
      }
    } else if (length > NetConfig::send_queue_heavy_warning_capacity) {
      send_queue_heavy_start_time_ = absolute_time;
    }

    last_check_send_queue_time_ = absolute_time;
  }
}

double RemotePeer_C::GetIndirectServerTimeDiff() {
  // 이젠 UDP가 안되도 일정시간마다 이값을 갱신한다.
  return indirect_server_time_diff_;
}

//@note 이게 호출되면, RelayedP2P로 전환됨!
void RemotePeer_C::FallbackP2PToRelay(bool first_chance, ResultCode reason) {
  if (IsDirectP2P()) {
    if (first_chance) {
#ifndef _DEBUG // 디버그 빌드에서는 의레 있는 일이므로
      if (reason != ResultCode::NoP2PGroupRelation && owner_->IsIntraLoggingOn()) {
        bool lbn = false, rbn = false;
        const char* lbn_str = "N/A";
        if (owner_->IsLocalHostBehindNAT(lbn)) {
          lbn_str = lbn ? "Yes" : "No";
        }
        rbn = IsBehindNAT();
        const String traffic_stat = owner_->GetTrafficStatText();
        const double udp_duration = owner_->GetAbsoluteTime() - relayed_p2p_disabled_time_;
        const double last_udp_recv_issue_duration = owner_->GetAbsoluteTime() - Get_ToPeerUdpSocket()->last_udp_recv_issued_time_;

        int32 rank = 1;
        if (!rbn) {
          rank++;
        }

        if (!owner_->GetNatDeviceName().IsEmpty()) {
          rank++;
        }

        const String text = String::Format("(first chance) to-peer client %d UDP punch lost##reason:%d##CliInstCount=%d##RecentElapTime=%3.3f##DisconnedCount=%d##recv count=%d##last ok recv interval=%3.3f##Recurred:%d##LocalIP:%s##remote peer behind NAT:%d##UDP kept time:%3.3f##Time diff since last RecvIssue:%3.3f##%s##Process=%s",
                      (int32)host_id,
                      *ToString(reason),
                      owner_->manager_->instances_.Count(),
                      owner_->manager_->GetCachedRecentAverageElapsedTime(),
                      owner_->manager_->disconnection_invoke_count_.GetValue(),
                      direct_udp_packet_recv_count_,
                      last_udp_packet_recv_interval_,
                      repunch_count_,
                      *owner_->Get_ToServerUdpSocketLocalAddr().ToString(),
                      rbn,
                      udp_duration,
                      last_udp_recv_issue_duration,
                      *traffic_stat,
                      "process-name?"//CPlatformProcess::ExecutableName()
                    );

        owner_->LogToServer_HolepunchFreqFail(rank, *text);
      }
#endif // _DEBUG
    }

    SetRelayedP2P();

    if (first_chance) {
      // 상대측에도 P2P 직빵 연결이 끊어졌으니 relay mode로 전환하라고 알려야 한다.
      owner_->c2s_proxy_.P2P_NotifyDirectP2PDisconnected(HostId_Server, GReliableSend_INTERNAL, host_id, reason);
    }

    // P2P 홀펀칭 시도중이던 것이 있으면 중지시킨다.
    p2p_holepunch_attempt_context_.Reset();

    // 이벤트 넣기
    // (디스 이유가, 상대측이 더 이상 통신할 이유가 없기 때문일 수 있는데
    // 이런 경우 이벤트 노티를 하지 말자. 어차피 조만간 서버로부터 P2P 멤버 나감 노티를 받을테니까.)
    if (reason != ResultCode::NoP2PGroupRelation) {
      owner_->EnqueueFallbackP2PToRelayEvent(host_id, reason);
    }

    // Repunch를 예비해둔다.
    ReserveRepunch();
  }

  // 아래는 불필요. 이미 이것이 호출될 때는 C/s TCP fallback이 필요하다면 이미 된 상태이니.
  //if (first_chance && owner_->to_server_udp_fallbackable_->real_udp_enabled_) {
  //  if (reason == ResultCode::ServerUdpFailed) {
  //    owner_->FallbackServerUdpToTcp(false, ResultCode::Ok);
  //  }
  //  //else if (reason == ResultCode::P2PUdpFailed) { // 과잉진압이다. 불필요.
  //  //  owner_->FallbackServerUdpToTcp();
  //  //}
  //}
}

void RemotePeer_C::UdpTransport::SendWhenReady( const SendFragRefs& data_to_send,
                                                const UdpSendOption& send_opt) {
  // 예전 소스에서는 Timer.GetAbsoluteTime()를 썼지만... 그거말고 owner_->owner_->AbsoluteTime를 써도 되지 않을까?
  if (owner_->udp_socket_) {
    const auto filter_tag = FilterTag::Make(owner_->owner_->GetLocalHostId(), owner_->host_id_); // Local -> Peer
    owner_->Get_ToPeerUdpSocket()->SendWhenReady(
          owner_->host_id_,
          filter_tag,
          owner_->p2p_holepunched_local_to_remote_addr_,
          data_to_send,
          owner_->owner_->GetAbsoluteTime(),
          send_opt);
  }
}

int32 RemotePeer_C::UdpTransport::GetUdpSendBufferPacketFilledCount() {
  if (owner_->udp_socket_) {
    return owner_->Get_ToPeerUdpSocket()->GetUdpSendBufferPacketFilledCount(owner_->p2p_holepunched_local_to_remote_addr_);
  } else {
    return 0;
  }
}

bool RemotePeer_C::UdpTransport::IsUdpSendBufferPacketEmpty() {
  if (owner_->udp_socket_) {
    return owner_->Get_ToPeerUdpSocket()->IsUdpSendBufferPacketEmpty(owner_->p2p_holepunched_local_to_remote_addr_);
  } else {
    return true;
  }
}

// 일정시간동안 UDP통신을 못하면, RelayedP2P 모드로 전환 되는데,
// 이러한 조건을 체크하는 루틴임.
bool RemotePeer_C::IsRelayConditionByUdpFailure(double absolute_time) {
  if (IsDirectP2P() &&
      (absolute_time - last_direct_udp_packet_recv_time_) > NetConfig::GetFallbackP2PUdpToTcpTimeout()) {
    return true;
  } else {
    return false;
  }
}

bool RemotePeer_C::IsRelayConditionByRUdpFailure() {
  // Reliable UDP 계층에서 sender window 재송신이 지나치게 오래 걸리면 릴레이 모드로 전환한다.
  // FunNet의 Reliable UDP만의 성능으로 모든 것을 신뢰할 수는 없기
  // 때문에 최악의 상황에서는 TCP의 성능에 의존해야 할 터이니.
  if (IsDirectP2P() &&
      to_peer_rudp_.host_ &&
      //to_peer_rudp_.host_->sender_.max_resend_elapsed_time_ > owner_->settings_.default_timeout_sec)
      to_peer_rudp_.host_->sender_.max_resend_elapsed_time_ > owner_->settings_.default_timeout_sec) {
    return true;
  } else {
    return false;
  }
}

void RemotePeer_C::SetRelayedP2P(bool relayed) {
  if (relayed) {
    if (!is_relayed_p2p_) {
      set_to_relayed_but_last_ping_is_not_calculated_yet_ = true;
    }
    is_relayed_p2p_ = true;
  } else {
    is_relayed_p2p_ = false;
    relayed_p2p_disabled_time_ = owner_->GetAbsoluteTime();
    last_direct_udp_packet_recv_time_ = owner_->GetAbsoluteTime(); // 아주 따끈따끈한 상태를 만든다.
    direct_udp_packet_recv_count_ = 0;
    last_udp_packet_recv_interval_ = -1;
  }

  // MaxResendElapsedTime은 P2P 핑이 너무 오래 걸려서 릴레이로 fallback을 검사하기 위한 용도다.
  // direct p2p로 바뀌건 relay로 바뀌건 이 값은 다시 리셋해야 한다.
  if (to_peer_rudp_.host_) {
    to_peer_rudp_.host_->sender_.max_resend_elapsed_time_ = 0;
  }
}

// Reserve Repunch
void RemotePeer_C::ReserveRepunch() {
  // 정해진 횟수만큼만 재시도하도록 함.
  if (repunch_count_ < NetConfig::server_udp_repunch_max_attempt_count) {
    repunch_count_++;
    repunch_started_time_ = owner_->GetAbsoluteTime() + NetConfig::server_udp_repunch_interval_sec; // Repunch 개시 시작 기억.
  }
}

RemotePeer_C::~RemotePeer_C() {
  if (udp_socket_) {
    // 소켓은 이미 닫힌 상태이어야 한다.
    fun_check_msg((Get_ToPeerUdpSocket()->socket_.IsValid() && Get_ToPeerUdpSocket()->socket_->IsClosed() == true),
                  "socket must be closed before destructor is called.");
  }
}

bool RemotePeer_C::IsBehindNAT() {
  // 포트를 제외한 호스트 값이 다른 경우, NAT(공유기) 뒤에 위치함.
  return udp_addr_internal.GetHost() != udp_addr_from_server.GetHost();
}

double RemotePeer_C::GetRenewalSocketCreationTime() {
  // 최소한 1초보다는 길어야 한다.
  // 안그러면 P2P member join 노티를 받은 직후 너무 짧은 속도로 P2P RPC를 주고 받을 경우
  // 상대측에서 P2P member join 노티를 받기도 전에 대응하는
  // 피어의 RPC를 받는 사태가 생길 수 있으므로.
  CScopedLock2 owner_guard(owner_->GetMutex());

  // 한번에 몰려서 생성되는걸 방지하기 위해서, 랜덤 시간 값을 부여해서, 몰려서 생성되는걸 방지함.
  return owner_->GetAbsoluteTime() + owner_->random_.NextDouble()*2 + 1;
}

void RemotePeer_C::ConditionalRestoreUdpSocket() {
  if (udp_socket_ && restore_needed_ && Get_ToPeerUdpSocket()->socket_) {
    if (Get_ToPeerUdpSocket()->socket_->IsClosed()) {
      if (!Get_ToPeerUdpSocket()->RestoreSocket()) {
        return;
      }

      // 이미 IOCP assoc까지 끝난 상태. 이제 first issue를 건다.
      fun_check(owner_->manager_->IsThisWorkerThread());
      Get_ToPeerUdpSocket()->ConditionalIssueRecvFrom(); // first issue

      owner_->c2s_proxy_.NotifyPeerUdpSocketRestored(HostId_Server, GReliableSend_INTERNAL, host_id);

      //CreateP2PHolepunchAttemptContext();

      if (owner_->IsIntraLoggingOn()) {
        const String text = String::Format("After restored UDP socket for remote-peer %d, wait to server reset order.", (int32)host_id);
        owner_->IntraLogToServer(LogCategory::P2P, *text);
      }
    }

    restore_needed_ = false;
  }
}

void RemotePeer_C::ConditionalNewUdpSocket() {
  owner_->LockMain_AssertIsLockedByCurrentThread();

  if (udp_socket_.IsValid() == false &&
      new_p2p_connection_needed_ &&
      owner_->Get_ToServerUdpSocket() != nullptr) {
    new_p2p_connection_needed_ = false;

    //TODO #if 로 묶인 코드는 구지 필요 없어 보이는데...

#if 1
    // UDP socket 객체를 파괴했으나 이것에 대한 overlapped io를 걸어놓은 상황이면 막장 댕글링.
    // 차라리 여기서 오류를 수면위로 노출하자.
    AssureUdpSocketNotUnderIssued();

    // 어차피 위에서 !udp_socket_ 을 만족해야 실행지점이 여기에 오므로 안전
    if (udp_socket_) {
      owner_->GarbageSocket(udp_socket_);
      udp_socket_.Reset();
    }
#endif

    udp_socket_.Reset(new UdpSocket_C(owner_, this));

    // TCP와 같은 NIC를 쓰기 위해 수정.
    InetAddress udp_local_addr = owner_->Get_ToServerTcp()->local_addr_;
    if (udp_local_addr.IsUnicast() == false) {
      udp_socket_.Reset();
      owner_->EnqueueWarning(ResultInfo::From(ResultCode::LocalSocketCreationFailed, owner_->GetLocalHostId(), "UDP socket for peer connection"));

      const String text = String::Format("RemotePeer_C.ConditionalNewUdpSocket - A TCP connection must already exist before creating a UDP socket, but %s is now.", *udp_local_addr.ToString());
      ErrorReporter::Report(text);
      return;
    }

    if (owner_->CreateUdpSocket(Get_ToPeerUdpSocket(), udp_local_addr) == false) {
      // new_p2p_connection_needed_ = true;
      udp_socket_.Reset();
      owner_->EnqueueWarning(ResultInfo::From(ResultCode::LocalSocketCreationFailed, owner_->GetLocalHostId(), "UDP socket for peer connection"));
      return;
    }

    // 이미 IOCP assoc까지 끝난 상태. 이제 first issue를 건다.
    fun_check(owner_->manager_->IsThisWorkerThread());
    Get_ToPeerUdpSocket()->ConditionalIssueRecvFrom(); // first issue

    CreateP2PHolepunchAttemptContext();
  }
}

// 새 UDP socket을 만들고 지정한 port로 바인딩을 한다.
bool RemotePeer_C::NewUdpSocketBindPort(int32 port) {
  owner_->LockMain_AssertIsLockedByCurrentThread();

  // UDP socket 객체를 파괴했으나 이것에 대한 overlapped io를 걸어놓은 상황이면 막장 댕글링.
  // 차라리 여기서 오류를 수면위로 노출하자.
  AssureUdpSocketNotUnderIssued();

  // 이것의 caller인 UpdateP2PGroup_MemberJoin에서, CRemotePeer_C를 갓 만든 상태에서만 이를 콜 하므로 안전
  if (udp_socket_) {
    owner_->GarbageSocket(udp_socket_);
    udp_socket_.Reset();
  }

  last_check_send_queue_time_ = 0;
  send_queue_heavy_start_time_ = 0;

  // 최근에 버린 UDP socket들 중 살아있는 것과 로컬 포트가 일치하는 것을 꺼내서 재사용한다.
  if (port != 0 && owner_->Recycles.TryGetValue(port, udp_socket_)) {
    Get_ToPeerUdpSocket()->recycle_binned_time_ = 0;
    //Get_ToPeerUdpSocket()->owner_peer_ = this;
    Get_ToPeerUdpSocket()->ResetPacketFragState(this);

    owner_->Recycles.Remove(port);
    return true;
  } else {
    udp_socket_.Reset(new UdpSocket_C(owner_, this));

    InetAddress udp_local_addr = owner_->Get_ToServerTcp()->local_addr_;
    if (!udp_local_addr.IsUnicast()) {
      const String text = String::Format("RemotePeer_C.NewUdpSocketBindPort - UDP 소켓을 생성하기 전에 TCP 연결이 이미 되어있는 상태이어야 하는데, TCP의 LocalAddr이 %s가 나왔다!", *udp_local_addr.ToString());
      ErrorReporter::Report(text);
      return false;
    }
    udp_local_addr.SetPort(port);

    if (Get_ToPeerUdpSocket()->CreateSocket(udp_local_addr) == false) {
      // CreateSocket가 실패하면 InternalSocket객체가 삭제됨 그러므로 Garbage로 들어갈 필요가 없음.
      // garbage에 들어가면서 뻑남.
      //AssureUdpSocketNotUnderIssued(); // UDP socket 객체를 파괴했으나 이것에 대한 overlapped io를 걸어놓은 상황이면 막장 댕글링. 차라리 여기서 오류를 수면위로 노출하자.
      //owner_->GarbageUdpSocket(udp_socket_);

      udp_socket_.Reset();
      return false;
    }

    return true;
  }

  return false;
}

// DirectP2P이건 아니건 일단 채우기는 한다.
void RemotePeer_C::GetDirectP2PInfo(DirectP2PInfo& out_info) {
  out_info.local_to_remote_addr = p2p_holepunched_local_to_remote_addr_;
  out_info.remote_to_local_addr = p2p_holepunched_remote_to_local_addr_;
  out_info.local_udp_socket_addr = udp_socket_ ? Get_ToPeerUdpSocket()->local_addr_ : InetAddress::None;
}

// 서버측에서의 UDP 수신 속도를 요청한다.
void RemotePeer_C::RequestReceiveSpeedAtReceiverSide_NoRelay(const InetAddress& dst) {
  if (udp_socket_) {
    MessageOut msg_to_send;
    lf::Write(msg_to_send, MessageType::RequestReceiveSpeedAtReceiverSide_NoRelay);
    const auto filter_tag = FilterTag::Make(owner_->GetLocalHostId(), host_id); // local -> peer
    const UdpSendOption send_opt(MessagePriority::Ring1, EngineOnlyFeature);
    Get_ToPeerUdpSocket()->SendWhenReady(host_id, filter_tag, dst, msg_to_send, GetAbsoluteTime(), send_opt);
  }
}

int32 RemotePeer_C::GetOverSendSuspectingThresholdInByte() {
  return owner_->settings_.over_send_suspecting_threshold_in_byte;
}

double RemotePeer_C::GetAbsoluteTime() {
  return owner_->GetAbsoluteTime();
}

void RemotePeer_C::AssureUdpSocketNotUnderIssued() {
  if (udp_socket_) {
    if (udp_socket_->send_issued_ || udp_socket_->recv_issued_) {
      int32* X = nullptr; *X = 1; // Raise exception
    }
  }
}

UdpSocket_C* RemotePeer_C::Get_ToPeerUdpSocket() {
  return udp_socket_ ? (UdpSocket_C*)(udp_socket_.Get()) : nullptr;
}

// 이 remote가 local과 같은 LAN에 있는가?  홀펀칭 안되어있으면 false를 리턴함.
bool RemotePeer_C::IsSameLanToLocal() const {
  if (!udp_socket_) {
    return false;
  }

  //TODO IPv6에서는 서브넷이 구분이 안되나?? 이부분은 좀더 알아봐야할듯 한데...
  //별도의 Unittest에서 확인 하도록 하자!
  return  owner_->Get_ToServerUdpSocketAddrAtServer().GetHost() == udp_addr_from_server.host_() &&
          NetUtil::IsSameSubnet24(owner_->Get_ToServerUdpSocketLocalAddr().GetHost(), udp_addr_internal.host_());
}

} // namespace net
} // namespace fun
