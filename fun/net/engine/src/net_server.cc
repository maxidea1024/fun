//TODO 코드정리
#include "fun/net/net.h"

#include "NetServer.h"
#include "NetListener_S.h"
#include "LeanDynamicCast.h"

#include "PacketFrag.h"
#include "Relayer.h"

#include "TcpTransport_S.h"
#include "RUdpHelper.h"

#include "ReportError.h"

//TODO
//#include "Apps/viz_agent_.h"

#include "ServerSocketPool.h"

#include "GeneratedRPCs/net_NetC2S_stub.cc"
#include "GeneratedRPCs/net_NetS2C_proxy.cc"

//TODO 딱히 구현 내용이 없는데 말이지??
#include "GeneratedRPCs/net.cc" //클라이언트/서버 구분이 안되는 문제가 있음.. 헤더화할수는 없는건가??

#include "stats/Counters.h"


namespace fun {
namespace net {

#define TRACE_NETSERVER_MESSAGES  0

using lf = LiteFormat;

/*

<?xml version="1.0"?>
<!-- http://www.adobe.com/crossdomain.xml -->
<cross-domain-policy>
  <allow-access-from domain="*" />
</cross-domain-policy>

*/
ByteArray POLICY_FILE_TEXT = "<?xml version=\"1.0\"?><cross-domain-policy><allow-access-from domain=\"*\" to-ports=\"*\" /></cross-domain-policy>";

#define ASSERT_OR_HACKED(X)  { if (!(X)) { EnqueueHackSuspectEvent(rc, #X, HackType::PacketRig); } }

IMPLEMENT_RPCSTUB_NetC2S_P2PGroup_MemberJoin_Ack(NetServerImpl::C2SStub) {
  CScopedLock2 main_guard(owner_->GetMutex());
  owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

  // Remove from group's relevant ack-waiter
  if (auto group = owner_->GetP2PGroupByHostId_NOLOCK(group_id)) {
    // 제대로 지운 다음...
    // ack-waiter for new member became empty?
    if (group->add_member_ack_waiters.RemoveEqualItem(rpc_recvfrom, added_member_id, event_id) == true &&
        group->add_member_ack_waiters.AckWaitingItemExists(added_member_id, event_id) == false) {
        owner_->EnqueueP2PAddMemberAckCompleteEvent(group_id, added_member_id, ResultCode::Ok);
    }
  }

  // 두 피어를 연결 정보를 얻는다.
  auto state = owner_->p2p_connection_pair_list_.GetPair(rpc_recvfrom, added_member_id);
  if (!state) {
    // 뒷북으로 오는 경우가 있으므로 warning를 띄우지 말자...
    //if (added_member_id != HostId_Server && added_member_id != remote) {
    //  // 엄청난 랙이 걸린 경우라면 용서되지만 그게 아닌 이상 이런 일은 없어야 한다.
    //  owner_->EnqueueWarning(ResultInfo::From(ResultCode::RUdpFailed, remote, "경고:대응하는 P2PConnectionState가 없습니다."));
    //}

    return true;
  }

  if (state->member_join_acked_start_) {
    state->MemberJoinAcked(rpc_recvfrom);

    if (state->release_time_ != 0 && bLocalPortReuseOk) {
      state->SetRecycleSuccess(rpc_recvfrom);
    }

    if (state->MemberJoinAckedAllComplete()) {
      if (state->release_time_ != 0 && state->GetServerHolepunchOkCount() == 2) {
        // absolute_time that you have completed all the steps, let me start reusing it.

        owner_->s2c_proxy_.P2PRecycleComplete(
            rpc_recvfrom,
            GReliableSend_INTERNAL,
            added_member_id,
            true,
            state->GetInternalAddr(added_member_id),
            state->GetExternalAddr(added_member_id),
            state->GetHolepunchedSendToAddr(rpc_recvfrom),
            state->GetHolepunchedRecvFromAddr(rpc_recvfrom));

        owner_->s2c_proxy_.P2PRecycleComplete(
            added_member_id,
            GReliableSend_INTERNAL,
            rpc_recvfrom,
            true,
            state->GetInternalAddr(rpc_recvfrom),
            state->GetExternalAddr(rpc_recvfrom),
            state->GetHolepunchedSendToAddr(added_member_id),
            state->GetHolepunchedRecvFromAddr(added_member_id));

        // Mark as DirectP2P
        state->SetRelayed(false);
      }
      else {
        // Since the process did not succeed, initialize the state and tell it to start from scratch.

        owner_->s2c_proxy_.P2PRecycleComplete(
            rpc_recvfrom,
            GReliableSend_INTERNAL,
            added_member_id,
            false,
            InetAddress::None,
            InetAddress::None,
            InetAddress::None,
            InetAddress::None);

        owner_->s2c_proxy_.P2PRecycleComplete(
            added_member_id,
            GReliableSend_INTERNAL,
            rpc_recvfrom,
            false,
            InetAddress::None,
            InetAddress::None,
            InetAddress::None,
            InetAddress::None);

        state->SetRelayed(true);
        state->ResetPeerInfo();
        state->last_holepunch_success_time_ = 0;
      }

      state->release_time_ = 0;
      state->member_join_acked_start_ = false;
    }
  }

  return true;
}

// Send a hole punching success to each client.  Send to both connected.
IMPLEMENT_RPCSTUB_NetC2S_NotifyP2PHolepunchSuccess(NetServerImpl::C2SStub) {
  CScopedLock2 main_guard(owner_->GetMutex());
  owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

  // Ignore if a-b is already passed
  // 다중 경로로 holepunch가 짧은 시간차로 다수 성공할 경우 가장 최초의 것만
  // 선택해주는 중재역할을 서버가 해야 하므로..

  if (auto state = owner_->p2p_connection_pair_list_.GetPair(a, b)) {
    // Send a signal that a direct connection has been made.
    if (state->GetRelayed()) { // switch from Relayed To Direct P2P mode..
      state->SetRelayed(false);

      // Save the holepunched information.
      state->last_holepunch_success_time_ = owner_->GetAbsoluteTime();

      state->SetPeerHolepunchOk(a, a2b_send_addr, b2a_recv_addr);
      state->SetPeerHolepunchOk(b, b2a_send_addr, a2b_recv_addr);

      // Pass it to a and b
      const auto& send_opt = GReliableSend_INTERNAL;
      owner_->s2c_proxy_.NotifyDirectP2PEstablish(a, send_opt, a, b, a2b_send_addr, a2b_recv_addr, b2a_send_addr, b2a_recv_addr);
      owner_->s2c_proxy_.NotifyDirectP2PEstablish(b, send_opt, a, b, a2b_send_addr, a2b_recv_addr, b2a_send_addr, b2a_recv_addr);

      if (owner_->intra_logger_) {
        const String text = String::Format("Holepunching between client %d and %d is completed successfully.  a2b_send_addr: %s, a2b_recv_addr: %s, b2a_send_addr: %s, b2a_recv_addr: %s",
                      (int32)a, (int32)b,
                      *a2b_send_addr.ToString(), *a2b_recv_addr.ToString(),
                      *b2a_send_addr.ToString(), *b2a_recv_addr.ToString());
        owner_->intra_logger_->WriteLine(LogCategory::PP2P, *text);
      }
    }
  } else {
    //FUN_TRACE("경고: P2P 연결 정보가 존재하지 않으면서 홀펀칭이 성공했다고라고라?\n");
  }

  return true;
}

IMPLEMENT_RPCSTUB_NetC2S_P2P_NotifyDirectP2PDisconnected(NetServerImpl::C2SStub) {
  CScopedLock2 main_guard(owner_->GetMutex());
  owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

  // Check validation
  if (auto rc = owner_->GetAuthedClientByHostId_NOLOCK(peer_id)) {
    // Update the connection status information between two clients stored in the server.
    auto state = owner_->p2p_connection_pair_list_.GetPair(rpc_recvfrom, peer_id);
    if (state && !state->GetRelayed()) {
      if (reason != ResultCode::NoP2PGroupRelation) {
        state->last_holepunch_success_time_ = 0;
      }

      // Direct because P2P is disconnected, switch to relay mode.
      state->SetRelayed(true);

      // Send ack.
      owner_->s2c_proxy_.P2P_NotifyDirectP2PDisconnected2(peer_id, GReliableSend_INTERNAL, rpc_recvfrom, reason);

      if (owner_->intra_logger_) {
        const String text = String::Format("Disconnected P2P UDP holepunching between client %d and %d.", (int32)rpc_recvfrom, (int32)peer_id);
        owner_->intra_logger_->WriteLine(LogCategory::PP2P, *text);
      }
    }
  }

  return true;
}

//TODO PeerBID -> PeerBID 이름에 신경을 써야할듯... IDL쪽 문제인데 고민이 필요하다.
IMPLEMENT_RPCSTUB_NetC2S_NotifyPeerUdpSocketRestored(NetServerImpl::C2SStub) {
  CScopedLock2 main_guard(owner_->GetMutex());
  owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

  // Check validation
  if (auto rc = owner_->GetAuthedClientByHostId_NOLOCK(PeerBID)) {
    // Fully initialize the connection state information between two classes stored in the server.
    // even if it is already specified as Direct P2P.
    if (auto state = owner_->p2p_connection_pair_list_.GetPair(rpc_recvfrom, PeerBID)) {
      // Start from the beginning, start with Relayed.
      state->SetRelayed(true);

      // For both clients, reset the P2P connection state completely and let it start as it did when it first started.
      // This is necessary because the socket is different.
      const auto& send_opt = GReliableSend_INTERNAL;
      owner_->s2c_proxy_.RenewP2PConnectionState(PeerBID, send_opt, rpc_recvfrom);
      owner_->s2c_proxy_.RenewP2PConnectionState(rpc_recvfrom, send_opt, PeerBID);

      if (owner_->intra_logger_) {
        const String text = String::Format("P2P UDP hole punching between client %d and %d is completely restarted from the beginning.  One UDP socket is being restored", (int32)rpc_recvfrom, (int32)PeerBID);
        owner_->intra_logger_->WriteLine(LogCategory::PP2P, *text);
      }
    }
  }

  return true;
}

IMPLEMENT_RPCSTUB_NetC2S_NotifyNatDeviceNameDetected(NetServerImpl::C2SStub) {
  CScopedLock2 main_guard(owner_->GetMutex());
  owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

  if (auto rc = owner_->GetAuthedClientByHostId_NOLOCK(rpc_recvfrom)) {
    rc->nat_device_name_ = device_name;
  }

  return true;
}

IMPLEMENT_RPCSTUB_NetC2S_NotifyJitDirectP2PTriggered(NetServerImpl::C2SStub) {
  CScopedLock2 main_guard(owner_->GetMutex());
  owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

  // Check validation
  if (auto rc = owner_->GetAuthedClientByHostId_NOLOCK(PeerBID)) {
    // 서버에 저장된 두 클라간 연결 상태 정보를 완전히 초기화한다.
    // 이미 Direct P2P로 지정되어 있어도 말이다.
    if (auto state = owner_->p2p_connection_pair_list_.GetPair(rpc_recvfrom, PeerBID)) {
      if (state->first_client->p2p_connection_pairs_.Count() < state->first_client->max_direct_p2p_connection_count_ &&
          state->second_client->p2p_connection_pairs_.Count() < state->second_client->max_direct_p2p_connection_count_) {
        // UDP Socket이 생성되어있지 않다면, UDP socket 생성을 요청한다.
        owner_->RemoteClient_NewLocalUdpSocketAndRequestNewRemoteUdpSocket(state->first_client);
        owner_->RemoteClient_NewLocalUdpSocketAndRequestNewRemoteUdpSocket(state->second_client);

        // (요청했으며 reliable messaging이므로 연이어 아래 요청을 해도 안전하다.
        // 아래 요청을 받는 클라는 이미 UDP socket이 있을테니까.)
        // 양측 클라이언트 모두에게, Direct P2P 시작을 해야하는 조건이 되었으니
        // 서로 홀펀칭을 시작하라는 지령을 한다.
        const auto& send_opt = GReliableSend_INTERNAL;
        owner_->s2c_proxy_.NewDirectP2PConnection(PeerBID, send_opt, rpc_recvfrom);
        owner_->s2c_proxy_.NewDirectP2PConnection(rpc_recvfrom, send_opt, PeerBID);

        state->jit_direct_p2p_requested = true;

        if (owner_->intra_logger_) {
          //const String text = String::Format("P2P UDP hole punching start condition between client %d and %d has been established.  absolute_time it is time to start the hole punching between the two peers.", (int32)rpc_recvfrom, (int32)PeerBID);
          const String text = String::Format("P2P UDP hole punching start condition between client %d and %d has been established.", (int32)rpc_recvfrom, (int32)PeerBID);
          owner_->intra_logger_->WriteLine(LogCategory::PP2P, *text);
        }
      }
    }
  }

  return true;
}

IMPLEMENT_RPCSTUB_NetC2S_NotifySendSpeed(NetServerImpl::C2SStub) {
  CScopedLock2 main_guard(owner_->GetMutex());
  owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

  if (auto rc = owner_->GetAuthedClientByHostId_NOLOCK(rpc_recvfrom)) {
    rc->send_speed = speed;
  }

  return true;
}

/*
Client로 부터 도착한 로그메시지를 출력.

@maxidea: Category외에 Severity를 추가 적용해 주어야할듯..
*/
IMPLEMENT_RPCSTUB_NetC2S_NotifyLog(NetServerImpl::C2SStub) {
  CScopedLock2 main_guard(owner_->GetMutex());
  owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

  if (owner_->intra_logger_) {
    const String FormattedText = String::Format(">> [Client %d] %s", (int32)rpc_recvfrom, *text);
    owner_->intra_logger_->WriteLine(Category, *FormattedText);
  }

  return true;
}

IMPLEMENT_RPCSTUB_NetC2S_NotifyLogHolepunchFreqFail(NetServerImpl::C2SStub) {
  CScopedLock2 main_guard(owner_->GetMutex());
  owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

  NetServerStats stats;
  owner_->GetStats(stats);

  if (owner_->intra_logger_) {
    const String prefix = String::Format(">> [Client %d] pair=%d/%d##CCU=%d##",
                  (int32)rpc_recvfrom,
                  stats.p2p_direct_connection_pair_count,
                  stats.p2p_connection_pair_count,
                  stats.client_count);
    owner_->intra_logger_->WriteLine(LogCategory::P2P, *(prefix + text));
  }


  // 랭크가 가장 높은것만 지속적으로 유지해줌. (차후에 가장 랭크가 높은것으로 기록된 것을 실제로 저장할 것임)
  RemoteClient_S* rc;
  if (owner_->freq_fail_log_most_rank_ < rank && (rc = owner_->GetAuthedClientByHostId_NOLOCK(rpc_recvfrom))) {
    const String prefix = String::Format("[Client %d %s] pair=%d/%d##CCU=%d",
                  (int32)rpc_recvfrom,
                  *rc->to_client_tcp_->remote_addr_.ToString(),
                  stats.p2p_direct_connection_pair_count,
                  stats.p2p_connection_pair_count,
                  stats.client_count);

    owner_->freq_fail_log_most_rank_ = rank;
    owner_->freq_fail_log_most_rank_text_ = prefix + text;

    owner_->free_fail_need_ = true;
  }

  return true;
}

// 클라에서 먼저 UDP 핑 실패로 인한 fallback을 요청한 경우
IMPLEMENT_RPCSTUB_NetC2S_NotifyUdpToTcpFallbackByClient(NetServerImpl::C2SStub) {
  CScopedLock2 main_guard(owner_->GetMutex());
  owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

  if (auto rc = owner_->GetAuthedClientByHostId_NOLOCK(rpc_recvfrom)) {
    owner_->LocalProcessForFallbackUdpToTcp(rc);
  }

  return true;
}

// 클라이언트에서 핑을 받았으니, 퐁을 보낸다.
IMPLEMENT_RPCSTUB_NetC2S_ReliablePing(NetServerImpl::C2SStub) {
  CScopedLock2 main_guard(owner_->GetMutex());
  owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

  if (auto rc = owner_->GetAuthedClientByHostId_NOLOCK(rpc_recvfrom)) {
    rc->last_tcp_stream_recv_time_ = owner_->GetAbsoluteTime();
    rc->last_application_hint_.recent_frame_rate = recent_frame_rate; // update frame-rate

    owner_->s2c_proxy_.ReliablePong(rpc_recvfrom, GReliableSend_INTERNAL);
  }

  return true;
}

// 클라이언트에서 안전하게 접속을 종료하기를 요청받은 경우, 이에 대한 처리를 하고
// 응답을 보내준다.
IMPLEMENT_RPCSTUB_NetC2S_ShutdownTcp(NetServerImpl::C2SStub) {
  CScopedLock2 main_guard(owner_->GetMutex());
  owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

  if (auto conn = owner_->GetAuthedClientByHostId_NOLOCK(rpc_recvfrom)) {
    conn->shutdown_comment_ = comment;
    owner_->s2c_proxy_.ShutdownTcpAck(rpc_recvfrom, GReliableSend_INTERNAL);
  }

  //return EStubResult::Handled; // 이게 좀더 의미가 있지 않을까???
  return true;
}

IMPLEMENT_RPCSTUB_NetC2S_ShutdownTcpHandshake(NetServerImpl::C2SStub) {
  CScopedLock2 main_guard(owner_->GetMutex());
  owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

  if (auto conn = owner_->GetAuthedClientByHostId_NOLOCK(rpc_recvfrom)) {
    owner_->IssueDisposeRemoteClient(rc, ResultCode::DisconnectFromRemote, ResultCode::TCPConnectFailure, rc->shutdown_comment_, __FUNCTION__, SocketErrorCode::Ok);
  }

  return true;
}

/*
Notified that a NAT device has been detected.
*/
IMPLEMENT_RPCSTUB_NetC2S_NotifyNatDeviceName(NetServerImpl::C2SStub) {
  CScopedLock2 main_guard(owner_->GetMutex());
  owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

  if (auto conn = owner_->GetAuthedClientByHostId_NOLOCK(rpc_recvfrom)) {
    rc->nat_device_name_ = device_name;
  }

  return true;
}

/*
Super-peer 선정등에 사용하기 위해서, 클라이언트에서 해당 피어와의 핑을 수신한다.

settings_.bEnablePingTest이 true인 경우에만 클라이언트가 보고를 하게됨에 주의해야함.
*/
IMPLEMENT_RPCSTUB_NetC2S_ReportP2PPeerPing(NetServerImpl::C2SStub) {
  CScopedLock2 main_guard(owner_->GetMutex());
  owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

  //TODO uint32이므로 아래 조건에 항상 참이므로 의미 없음.
  //if (recent_ping >= 0) {
    if (auto conn = owner_->GetAuthedClientByHostId_NOLOCK(rpc_recvfrom)) {
      for (auto& pair : conn->p2p_connection_pairs_) {
        if (pair->ContainsHostId(peer_id)) {
          pair->recent_ping_ = double(recent_ping) / 1000;
        }
      }
    }
  //}

  return true;
}

IMPLEMENT_RPCSTUB_NetC2S_C2S_RequestCreateUdpSocket(NetServerImpl::C2SStub) {
  CScopedLock2 main_guard(owner_->GetMutex());
  owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

  if (auto conn = owner_->GetAuthedClientByHostId_NOLOCK(rpc_recvfrom)) {
    NamedInetAddress sock_addr;

    const bool ok = owner_->RemoteClient_New_ToClientUdpSocket(rc);
    if (ok) {
      sock_addr = conn->to_client_udp_fallbackable_.udp_socket_->GetRemoteIdentifiableLocalAddr(conn);
    }

    // End of UDP socket creation by partner request. Your opponent is already there. Therefore, ACK is sent.
    owner_->s2c_proxy_.S2C_CreateUdpSocketAck(rpc_recvfrom, GReliableSend_INTERNAL, ok, sock_addr);

    if (ok) {
      owner_->RemoteClient_RequestStartServerHolepunch_OnFirst(conn);
    }
  }

  return true;
}

IMPLEMENT_RPCSTUB_NetC2S_C2S_CreateUdpSocketAck(NetServerImpl::C2SStub) {
  CScopedLock2 main_guard(owner_->GetMutex());
  owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

  if (auto rc = owner_->GetAuthedClientByHostId_NOLOCK(rpc_recvfrom)) {
    //owner_->RemoteClient_New_ToClientUdpSocket(rc);
    rc->to_client_udp_fallbackable_.client_udp_ready_waiting_ = false;

    if (ok) {
      owner_->RemoteClient_RequestStartServerHolepunch_OnFirst(rc);
    } else {
      // If udp socket creation of client fails, udp communication is not done.

      // 여기서 RefCount가 2여야 정상임.
      // PerClient일때는 OwnedUdpSocket이 있고 static일때는 UdpSockets가 있기때문..

      if (owner_->intra_logger_) {
        const String text = String::Format("Client UDP socket creation failed.  host_id: %d, udp_socket_ref_count: %d",
                        (int32)rc->host_id_, rc->to_client_udp_fallbackable_.udp_socket_.GetSharedReferenceCount());
        owner_->intra_logger_->WriteLine(LogCategory::System, *text);
      }

      // real_udp_enabled is false because there is no udp_socket.
      rc->to_client_udp_fallbackable_.real_udp_enabled_ = false;
      rc->to_client_udp_fallbackable_.client_udp_socket_create_failed_ = true;
      rc->to_client_udp_fallbackable_.udp_socket_.Reset();
    }
  }

  return true;
}

/*
C2C간의 UDP Message 전송 통계를 서버로 전송함.
단 이 기능은 필요에 따라서 켜고 끌 수 있음.

  bool NetConfig::report_real_udp_count_enabled;
  double NetConfig::report_real_udp_count_interval_sec;

위 두 값을 참조해서 동작한다.


//@todo remote -> remote_id, peer -> PeerId로 이름을 변경하는게 좋을듯..
*/
IMPLEMENT_RPCSTUB_NetC2S_ReportC2CUdpMessageCount(NetServerImpl::C2SStub) {
  CScopedLock2 main_guard(owner_->GetMutex());
  owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

  auto rc = owner_->GetAuthedClientByHostId_NOLOCK(rpc_recvfrom);
  if (rc && peer_id != HostId_None) {
    // 받은 packet 갯수를 업데이트한다.
    rc->to_remote_peer_send_udp_message_attempt_count += udp_message_attempt_count;
    rc->to_remote_peer_send_udp_message_success_count += udp_message_success_count;
    //TODO Loss비율 계산

    for (auto& pair : rc->p2p_connection_pairs_) {
      if (pair->ContainsHostId(peer_id)) {
        pair->to_remote_peer_send_udp_message_attempt_count = udp_message_attempt_count;
        pair->to_remote_peer_send_udp_message_success_count = udp_message_success_count;
        //TODO Loss비율 계산
      }
    }
  }

  return true;
}

IMPLEMENT_RPCSTUB_NetC2S_ReportC2SUdpMessageTrialCount(NetServerImpl::C2SStub) {
  CScopedLock2 main_guard(owner_->GetMutex());
  owner_->CheckCriticalSectionDeadLock(__FUNCTION__);

  if (auto rc = owner_->GetAuthedClientByHostId_NOLOCK(rpc_recvfrom)) {
    rc->to_server_send_udp_message_attempt_count = to_server_udp_attempt_count;
  }

  return true;
}

bool NetServerImpl::GetJoinedP2PGroups(HostId client_id, HostIdArray& output) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (auto conn = authed_remote_clients_.FindRef(client_id)) {
    output.Clear(conn->joined_p2p_groups_.Count()); // just in case
    for (const auto& group : conn->joined_p2p_groups_) {
      output.Add(group.key);
    }
    return true;
  } else {
    output.Clear(); // just in case
    return false;
  }
}

void NetServerImpl::GetP2PGroups(P2PGroupInfos& out_result) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  out_result.Clear(p2p_groups_.Count()); // just in case
  for (const auto& pair : p2p_groups_) {
    auto group = pair.value;
    auto group_info = group->GetInfo();
    out_result.Add(group_info->group_id_, group_info);
  }
}

int32 NetServerImpl::GetP2PGroupCount() {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  return p2p_groups_.Count();
}


//TODO 초기화중 예외는 던지지 않는게 좋을듯함...

//@todo InParams은 내부에서 수정될 개연성이 있으므로, const로 넘기면 안된다.
//      넘겨진 인자 필드들만 수정되는게 아니라, 내용이 담기기도 하기 때문에
//      in이 아닌, inout인 성격을 가짐.
bool NetServerImpl::Start(const StartServerArgs& InParams, SharedPtr<ResultInfo>& out_error) {
  CScopedLock2 start_stop_phase_guard(start_stop_phase_mutex_);

  // If we have already started, we will handle the error.
  if (tcp_listening_socket_) {
    out_error = ResultInfo::From(ResultCode::AlreadyConnected, HostId_None, "Already listening socket exists.");
    return false;
  }

  if (net_thread_pool_ || user_thread_pool_) {
    out_error = ResultInfo::From(ResultCode::ServerPortListenFailure, HostId_None, "Already thread-pool exists.");
    return false;
  }

  //@todo 제거해야함.. InParams자체가 inout이다. -.-;;
  StartServerArgs args = InParams; // Copy

  if (args.thread_count == 0) {
    args.thread_count = CPlatformMisc::NumberOfCoresIncludingHyperthreads();
  }

  if (args.thread_count < 0) {
    throw Exception("Invalid thread count");
  }

  if (args.udp_assign_mode == ServerUdpAssignMode::PerClient &&
      args.udp_ports.Count() > 0 &&
      args.udp_ports.Count() < 1000) {
    LOG(LogNetEngine, Warning, "WARNING: You specified too few UDP ports with ServerUdpAssignMode::PerClient.  This may cause inefficient P2P communication and unreliable messaging.");
  }

  // 파라메터 validation을 체크한다.
  bool has_non_zero_port_num = false;
  bool has_zero_port_num = false;
  for (int32 i = 0; i < args.udp_ports.Count(); ++i) {
    if (args.udp_ports[i] == 0) {
      has_zero_port_num = true;
    } else {
      has_non_zero_port_num = true;
    }
  }

  if (has_zero_port_num && has_non_zero_port_num) {
    throw Exception("Cannot assign non-zero UDP port number with zero UDP port number.  All non-zero or all zero must be guaranteed.");
  }

  //@todo 아예 TArray의 함수내에서 중복 여부를 체크하는건 어떨런지? 안어울릴까??
  //AllItemsAreUniqueInArray();
  auto dup_ports_check_list = args.udp_ports;
  dup_ports_check_list.Sort();
  for (int32 i = 0; i < dup_ports_check_list.Count() - 1; ++i) {
    if (dup_ports_check_list[i] == dup_ports_check_list[i+1] && dup_ports_check_list[i] != 0) {
      // 겹친 포트 번호가 있을 경우는 예외
      throw Exception("Cannot add duplicated UDP port number.");
    }
  }

  // NIC(네트워크카드)가 여러개 있을 경우, 로컬 주소가 여러개가 되는데,
  // 이때 바인딩될 주소를 지정하지 않는다면 문제가 됨.
  const bool has_multiple_nics = Dns::ThisHost().Addresses().Count() > 1;
  if (has_multiple_nics && args.local_nic_addr_.IsEmpty()) {
    ShowWarning_NOLOCK(ResultInfo::From(ResultCode::Unexpected, HostId_None,
                  "Server has multiple network devices, however, no device is specified for listening.  Is it your intention?"));

    //TODO 이 케이스가 오류 보고의 상황인건가??
    const String text = String::Format("no NIC binding though multiple NIC detected##Process=%s", CPlatformProcess::ExecutableName());
    ErrorReporter::Report(text);
  }

  // Send coalsecse 시간을 임의로 지정하는 경우에 대한 처리.(테스트 용도 혹은 튜닝용)
  if (args.TunedNetworkerSendInterval_TEST > 0) {
    every_remote_issue_send_on_need_internal_ = args.TunedNetworkerSendInterval_TEST;
  } else {
    every_remote_issue_send_on_need_internal_ = NetConfig::every_remote_issue_send_on_need_internal_;
  }

  settings_.server_as_p2p_group_member_allowed = args.server_as_p2p_group_member_allowed;
  settings_.p2p_encrypted_messaging_enabled = args.p2p_encrypted_messaging_enabled;
  settings_.upnp_detect_nat_device = args.upnp_detect_nat_device;
  settings_.upnp_tcp_addr_port_mapping = args.upnp_tcp_addr_port_mapping;
  settings_.emergency_log_line_count = args.client_emergency_log_max_line_count;

  settings_.ping_test_enabled = args.ping_test_enabled;
  settings_.ignore_failed_bind_port = args.ignore_failed_bind_port;

  settings_.bEnableNagleAlgorithm = args.bEnableNagleAlgorithm;
  if (settings_.bEnableNagleAlgorithm) {
    LOG(LogNetEngine, Warning, "Setting bEnableNagleAlgorithm to false is RECOMMENDED.  FunNet affords better anti-silly window syndrome technology than TCP nagle algorithm.");
  }

  using_over_block_icmp_environment_ = args.using_over_block_icmp_environment_;

  // RC4 키 길이 Bit 안쓸 떄에는 0
  settings_.weak_encrypted_message_key_length = args.weak_encrypted_message_key_length;
  if (settings_.weak_encrypted_message_key_length != (int32)WeakEncryptionLevel::None &&    // 0
      settings_.weak_encrypted_message_key_length != (int32)WeakEncryptionLevel::Low &&   // 512
      settings_.weak_encrypted_message_key_length != (int32)WeakEncryptionLevel::Middle &&  // 1024
      settings_.weak_encrypted_message_key_length != (int32)WeakEncryptionLevel::High) {   // 2048
    throw Exception("StartServerArgs::weak_encrypted_message_key_length incorrect key length, MaxSize is 2048, MinSize is 512");
  }

  settings_.strong_encrypted_message_key_length = args.strong_encrypted_message_key_length;
  if (settings_.strong_encrypted_message_key_length != (int32)StrongEncryptionLevel::None &&    // 0
      settings_.strong_encrypted_message_key_length != (int32)StrongEncryptionLevel::Low &&   // 128
      settings_.strong_encrypted_message_key_length != (int32)StrongEncryptionLevel::Middle &&  // 192
      settings_.strong_encrypted_message_key_length != (int32)StrongEncryptionLevel::High) {    // 256
    throw Exception("StartServerArgs::EncryptedMessageKeyLength incorrect key length, only 128, 192, 256");
  }

  // Generate public and private keys in advance and generate AES key for loopback ..
  ByteArray rsa_random_block;
  ByteArray rc4_random_block;
  if (!CryptoRSA::CreatePublicAndPrivateKey(self_xchg_key_, public_key_blob_) ||
      !CryptoRSA::CreateRandomBlock(rsa_random_block, settings_.strong_encrypted_message_key_length) ||
      !CryptoAES::ExpandFrom(self_session_key_.aes_key, (const uint8*)rsa_random_block.ConstData(), settings_.strong_encrypted_message_key_length / 8) ||

      !CryptoRSA::CreateRandomBlock(rc4_random_block, settings_.weak_encrypted_message_key_length) ||
      !CryptoRC4::ExpandFrom(self_session_key_.rc4_key, (const uint8*)rc4_random_block.ConstData(), settings_.weak_encrypted_message_key_length / 8)) {
    throw Exception("Failed to create session-key.");
  }

  self_encrypt_count_ = 0;
  self_decrypt_count_ = 0;

  // Create IOCP & user worker IOCP
  tcp_accept_cp_.Reset(new CompletionPort(this, true, 1));

  clock_.Reset();
  clock_.Start();

  // Create server instance GUID
  instance_tag_ = Uuid::NewUuid();

  // Copy parameters.
  udp_assign_mode = args.udp_assign_mode;
  protocol_version = args.protocol_version;
  server_ip_alias_ = args.server_addr_at_client;
  local_nic_addr_ = args.local_nic_addr_;

  start_create_p2p_group_ = false;

  args.network_thread_count = MathBase::Clamp(args.network_thread_count, 0, CPlatformMisc::NumberOfCoresIncludingHyperthreads());
  if (args.network_thread_count == 0) {
    args.network_thread_count = CPlatformMisc::NumberOfCoresIncludingHyperthreads();
  }

  // Initialize thread pool.
  if (args.external_net_worker_thread_pool) {
    auto thread_pool = dynamic_cast<ThreadPoolImpl*>(args.external_net_worker_thread_pool);
    if (thread_pool == nullptr || !thread_pool->IsActive()) {
      throw Exception("NetWorerThreadPool is not start.");
    }

    net_thread_pool_.Reset(thread_pool);
    net_thread_external_use_ = true;
  } else {
    net_thread_pool_.Reset((ThreadPoolImpl*)ThreadPool2::New());
    net_thread_pool_->Start(args.network_thread_count);
    net_thread_external_use_ = false;
  }

  if (args.external_user_worker_thread_pool) {
    auto thread_pool = dynamic_cast<ThreadPoolImpl*>(args.external_user_worker_thread_pool);
    if (thread_pool == nullptr || !thread_pool->IsActive()) {
      throw Exception("UserWorkerThreadPool is not start.");
    }

    user_thread_pool_.Reset(thread_pool);
    user_thread_external_use_ = true;
  } else {
    user_thread_pool_.Reset((ThreadPoolImpl*)ThreadPool2::New());
    user_thread_pool_->SetCallbacks(this); // 내장일 경우 event-callbacks는 this가 된다. (스레드풀 셋팅전에 해야함)
    user_thread_pool_->Start(args.thread_count);
    user_thread_external_use_ = false;
  }

  //TODO 중간에 0이 들어가 있어도 불량임...
  if (args.tcp_ports.IsEmpty()) {
    args.tcp_ports.Add(0); // Add 0 by default.
  }

  if (!CreateAndInitUdpSockets(args.udp_assign_mode, args.udp_ports, args.failed_bind_ports, out_error) ||
      !CreateTcpListenSocketAndInit(args.tcp_ports, out_error)) {
    // Because the sockets could not be created, it is no longer possible to operate normally,
    // Cancel all initializations that were done before.
    clock_.Stop();
    //completion_port_.Reset();
    tcp_accept_cp_.Reset();
    instance_tag_ = Uuid::None;

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

    CScopedLock2 main_guard(main_mutex_);

    udp_sockets_.Clear();
    local_addr_to_udp_socket_map_.Clear();

    out_error = ResultInfo::From(ResultCode::ServerPortListenFailure, HostId_None, "Listening tcp or udp socket creation is failed.");
    return false;
  }

  // referer 등록.
  net_thread_pool_->RegistReferer(this);
  net_thread_pool_unregisted_ = false;
  user_thread_pool_->RegistReferer(this);
  user_thread_pool_unregisted_ = false;

  //TODO host_id 생성 정책을 다변화할 필요가 있을까??

  // Initialize host_id factory.
  switch (args.host_id_generation_policy) {
    case HostIdGenerationPolicy::NoRecycle:
      host_id_factory.Reset(new HostIdFactory());
      break;

    case HostIdGenerationPolicy::Recycle:
      host_id_factory.Reset(new RecycleHostIdFactory(NetConfig::host_id_recycle_allow_time_sec));
      break;

    case HostIdGenerationPolicy::Assign:
      host_id_factory.Reset(new AssignHostIdFactory());

      if (args.pre_created_p2p_group_start_host_id > HostId_Last) {
        empty_p2p_group_allowed_ = true;
        start_create_p2p_group_ = true;

        for (int32 i = 0; i < args.pre_create_p2p_group_count; ++i) {
          CreateP2PGroup(nullptr, 0, ByteArray(), args.pre_create_p2p_group_option, (HostId)(args.pre_created_p2p_group_start_host_id + i));
        }
      }
      break;

    default:
      throw Exception("Wrong HostIdGenerationPolicy.");
  }

  freq_fail_log_most_rank_ = 0;
  freq_fail_log_most_rank_text_ = "";
  last_holepunch_freq_fail_logged_time_ = 0;
  free_fail_need_ = 0;

  // TCP, UDP를 받아 처리하는 스레드 풀을 만든다.
  listener_.Reset(new NetListener_S(this));
  listener_->StartListening();

  // Heartbeat 시작부
  //개별객체가 아닌, 묶음으로 처리하는게 좋을듯 한데...
  //TickableObject같이 말이지...
  //PurgeTooOldUnmatureClient_Alarm = IntervalAlaram(NetConfig::purge_too_old_add_member_ack_timeout_sec);
  //PurgeTooOldAddMemberAckItem_Alarm = IntervalAlaram(NetConfig::purge_too_old_add_member_ack_timeout_sec);
  //DisposeIssuedRemoteClients_Alarm = IntervalAlaram(NetConfig::dispose_issued_remote_clients_timeout_sec);
  //ElectSuperPeer_Alarm = IntervalAlaram(NetConfig::elect_super_peer_interval_sec);
  //RemoveTooOldUdpSendPacketQueue_Alarm = IntervalAlaram(NetConfig::udp_packet_board_long_interval_sec);
  //DisconnectRemoteClientOnTimeout_Alarm = IntervalAlaram(NetConfig::cs_ping_interval_sec);
  //RemoveTooOldRecyclePair_Alarm = IntervalAlaram(NetConfig::remove_too_old_recycle_pair_interval_sec);

  heartbeat_working_ = 0;
  on_tick_working_ = 0;

  heartbeat_tickable_timer_.ExpireRepeatedly(
      Timespan::FromSeconds(NetConfig::purge_too_old_add_member_ack_timeout_sec),
      [&](TickableTimer::CContext& context) { PurgeTooOldUnmatureClient(); },
      -1,
      "PurgeTooOldUnmatureClient");

  heartbeat_tickable_timer_.ExpireRepeatedly(
      Timespan::FromSeconds(NetConfig::purge_too_old_add_member_ack_timeout_sec),
      [&](TickableTimer::CContext& context) { PurgeTooOldAddMemberAckItem(); },
      -1,
      "PurgeTooOldAddMemberAckItem");

  heartbeat_tickable_timer_.ExpireRepeatedly(
      Timespan::FromSeconds(NetConfig::dispose_issued_remote_clients_timeout_sec),
      [&](TickableTimer::CContext& context) { DisposeIssuedRemoteClients(); },
      -1,
      "DisposeIssuedRemoteClients");

  // 통신량에 의해 적합 수퍼피어가 달라질 수 있기 때문에 주기적으로 super-peer를 선출해야함.
  heartbeat_tickable_timer_.ExpireRepeatedly(
      Timespan::FromSeconds(NetConfig::elect_super_peer_interval_sec),
      [&](TickableTimer::CContext& context) { ElectSuperPeer(); },
      -1,
      "ElectSuperPeer");

  //TODO 이름이 좀 이상하네 정리가 필요한듯...
  heartbeat_tickable_timer_.ExpireRepeatedly(
      Timespan::FromSeconds(NetConfig::udp_packet_board_long_interval_sec),
      [&](TickableTimer::CContext& context) { TcpAndUdp_LongTick(); },
      -1,
      "TcpAndUdp_LongTick");

  heartbeat_tickable_timer_.ExpireRepeatedly(
      Timespan::FromSeconds(NetConfig::cs_ping_interval_sec),
      [&](TickableTimer::CContext& context) { Heartbeat_PerClient(); },
      -1,
      "Heartbeat_PerClient");

  heartbeat_tickable_timer_.ExpireRepeatedly(
      Timespan::FromSeconds(NetConfig::remove_too_old_recycle_pair_interval_sec),
      [&](TickableTimer::CContext& context) {
        CScopedLock2 main_guard(main_mutex_);
        CheckCriticalSectionDeadLock(__FUNCTION__);

        p2p_connection_pair_list_.RemoveTooOldRecyclePair(GetAbsoluteTime());
      },
      -1,
      "Heartbeat_PerClient");


  timer_callback_interval_ = args.timer_callback_interval_;
  timer_callback_context_ = args.timer_callback_context_;

  if (timer_callback_interval_ > 0) {
    tick_timer_id_ = timer_.Schedule(
          Timespan::FromMilliseconds(100),
          Timespan::FromMilliseconds(timer_callback_interval_),
          [&](const TimerTaskContext&) { PostOnTick(); },
          "NetServer.Tick");
  } else {
    tick_timer_id_ = 0;
  }

  heartbeat_timer_id_ = timer_.Schedule(
        Timespan::FromMilliseconds(100),
        Timespan::FromMilliseconds(NetConfig::server_heartbeat_interval_msec),
        [&](const TimerTaskContext&) { PostHeartbeatIssue(); },
        "NetServer.Heartbeat");

  issue_send_on_need_timer_id_ = timer_.Schedule(
        Timespan::FromMilliseconds(100),
        Timespan::FromMilliseconds(every_remote_issue_send_on_need_internal_ * 1000),
        [&](const TimerTaskContext&) { PostEveryRemote_IssueSend(); },
        "NetServer.ConditionalIssueSend");

  //TODO Do not use sleep, but wait for all threads to work.
  CPlatformProcess::Sleep(0.1f);

  timer_.Start();

  return true;
}

void NetServerImpl::Stop() {
  //FIXME 왜 다운이 되는거지???
  //콘솔 닫기 버튼을 누른 후 호출될 경우에는 문제가 발생하고 있음.
  //이를 자연 스럽게 처리할 방법이 없을까??
  //콘솔 닫기 버튼이 클릭 되었을때, 콘솔을 출력에서 제외하면 안전하지 않을런지??
  CScopedLock2 start_stop_phase_guard(start_stop_phase_mutex_);

  AssertIsNotLockedByCurrentThread();

  // Ignore if already stopped.
  if (!tcp_listening_socket_ /*|| tear_down_*/) {
    return;
  }

  // Do not lock here because there is a lock inside threadpool.
  if (user_thread_pool_ && user_thread_pool_->IsCurrentThread()) {
    // 이상하게도 userworkerthread내에서 try catch로 잡으면 파괴자가 호출되지 않는 현상이 생긴다.
    // 그런데, userworkerthread내에서 Stop()을 호출하는게 말이되나?
    start_stop_phase_guard.Unlock();
    throw Exception("Call Stop() in UserWorker.");
  }

  {
    // Because the server is in the process of shutting down, the connection of the new client is no longer meaningful,
    // This closes the listener socket and blocks new client connections.
    // If you allow it, only errors will be generated and the process will not be clean.

    // Stop listener thread.
    listener_.Reset();

    // Close listener socket.
    if (tcp_listening_socket_) {
      tcp_listening_socket_->CloseSocketHandleOnly();
    }

    // Disconnect all clients that are already connected. However, if you force off immediately,
    // Because it is lost, tell it to disconnect, and then wait until the process is completed normally.

    CScopedLock2 main_guard(main_mutex_);
    CheckCriticalSectionDeadLock(__FUNCTION__);

    for (auto& pair : candidate_remote_clients_) {
      auto& rc = pair.value;
      IssueDisposeRemoteClient(rc, ResultCode::DisconnectFromLocal, ResultCode::TCPConnectFailure, ByteArray(), __FUNCTION__, SocketErrorCode::Ok);
    }

    for (auto& pair : authed_remote_clients_) {
      auto& rc = pair.value;
      IssueDisposeRemoteClient(rc, ResultCode::DisconnectFromLocal, ResultCode::TCPConnectFailure, ByteArray(), __FUNCTION__, SocketErrorCode::Ok);
    }

    // Close the socket before leaving the overlapped I/O thread (leaving the buffer)
    // no more worker's issue should be made.
    // Then there is no memory violation to exit.
    for (int32 socket_index = 0; socket_index < udp_sockets_.Count(); ++socket_index) {
      auto udp_socket = udp_sockets_[socket_index];
      udp_socket->socket_->CloseSocketHandleOnly();
    }
  }

  // Wait for asynchronous I/O of all clients to finish.
  for (int32 attempt_index = 0; attempt_index < 10000; ++attempt_index) {
    CScopedLock2 main_guard(main_mutex_);
    CheckCriticalSectionDeadLock(__FUNCTION__);

    if (authed_remote_clients_.IsEmpty() &&
        candidate_remote_clients_.IsEmpty() &&
        dispose_issued_remote_clients_map_.IsEmpty()) {
      break;
    }

    main_guard.Unlock();

    CPlatformProcess::Sleep(0.1f);
  }

  timer_.Stop(true); // 내부 처리가 완료될때까지 대기함.

  //TODO 구태여 타이머 ID를 유지할 필요가 없어보이는데...??
  tick_timer_id_ = 0;
  heartbeat_timer_id_ = 0;
  issue_send_on_need_timer_id_ = 0;

  heartbeat_tickable_timer_.CancelAll();

  // Instructs the thread pool to terminate, wait until it is terminated gracefully, and terminate safely.
  net_thread_pool_->PostCompletionStatus(this, (UINT_PTR)IocpCustomValue::End);

  if (net_thread_pool_ != user_thread_pool_) {
    user_thread_pool_->PostCompletionStatus(this, (UINT_PTR)IocpCustomValue::End);
  }

  for (int32 attempt_index = 0; attempt_index < 10000; ++attempt_index) {
    CPlatformMisc::MemoryBarrier();

    if (net_thread_pool_unregisted_ && user_thread_pool_unregisted_) {
      break;
    }

    CPlatformProcess::Sleep(0.1f);
  }

  CPlatformMisc::MemoryBarrier();
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

  // Since all the threads have been properly terminated, remove the remaining objects here.
  {
    CScopedLock2 main_guard(main_mutex_);
    CheckCriticalSectionDeadLock(__FUNCTION__);

    tcp_accept_cp_.Reset();
    clock_.Stop();

    tcp_listening_socket_.Reset();

    while (!authed_remote_clients_.IsEmpty()) {
      auto it = authed_remote_clients_.CreateIterator();
      ProcessOnClientDisposeCanSafe(it->value);
    }

    while (!candidate_remote_clients_.IsEmpty()) {
      auto it = candidate_remote_clients_.CreateIterator();
      ProcessOnClientDisposeCanSafe(it->value);
    }

    // Actually destroy all remote-client objects.
    for (auto rc_instance : remote_client_instances_) {
      delete rc_instance;
    }
    remote_client_instances_.Clear();

    p2p_groups_.Clear();

    // These final dependencies are destroyed here.
    udp_sockets_.Clear();
    local_addr_to_udp_socket_map_.Clear();
  }


  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  final_user_work_queue_.Clear();
  authed_remote_clients_.Clear();

  p2p_connection_pair_list_.Clear();
  candidate_remote_clients_.Clear();

  host_id_factory.Reset();

  //TODO
  //viz_agent_.Reset();

  //tear_down_ = false;
}

bool NetServerImpl::CreateTcpListenSocketAndInit( const Array<int32>& tcp_ports,
                                                  SharedPtr<ResultInfo>& out_error) {
  tcp_listening_socket_.Reset(new InternalSocket(SocketType::Tcp, this));
  tcp_listening_socket_->ignore_not_socket_error_ = true;

  bool bound = false;
  for (int32 port_index = 0; port_index < tcp_ports.Count(); ++port_index) {
    const int32 port = tcp_ports[port_index];

    if (tcp_listening_socket_->Bind(*local_nic_addr_, port)) {
      bound = true;
      break;
    }
  }

  if (!bound) {
    // TCP listening can not start itself. In this case, the server can not function properly.
    out_error = ResultInfo::From(ResultCode::ServerPortListenFailure, HostId_Server, "");
    tcp_listening_socket_.Reset();
    return false;
  } else {
    tcp_listening_socket_->SetCompletionContext((ICompletionContext*)this);
    tcp_accept_cp_->AssociateSocket(tcp_listening_socket_.Get());
    tcp_listening_socket_->Listen();
    return true;
  }
}

bool NetServerImpl::CreateAndInitUdpSockets(ServerUdpAssignMode udp_assign_mode,
                                            const Array<int32>& udp_ports,
                                            Array<int32>& failed_bind_ports,
                                            SharedPtr<ResultInfo>& out_error) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  // Per-client UDP socket does not have shared UDP sockets.
  if (udp_assign_mode == ServerUdpAssignMode::PerClient) {
    free_udp_ports_ = udp_ports;
    return true;
  }

  // socket 객체들을 만든다.
  udp_sockets_.Clear();
  local_addr_to_udp_socket_map_.Clear();

  for (int32 port_index = 0; port_index < udp_ports.Count(); ++port_index) {
    const int32 port = udp_ports[port_index];

    UdpSocketPtr_S udp_socket(new UdpSocket_S(this));

    bool bound;
    if (local_nic_addr_.IsEmpty()) {
      bound = udp_socket->socket_->Bind(port);
    } else {
      bound = udp_socket->socket_->Bind(*local_nic_addr_, port);
    }

    if (!bound) {
      if (!settings_.ignore_failed_bind_port) {
        udp_sockets_.Clear();
        local_addr_to_udp_socket_map_.Clear();

        out_error = ResultInfo::From(ResultCode::ServerPortListenFailure, HostId_Server, "");
        return false;
      } else {
        failed_bind_ports.Add(port);
        continue;
      }
    }

    // socket local 주소를 얻고 그것에 대한 매핑 정보를 설정한다.
    udp_socket->cached_local_addr_ = udp_socket->socket_->GetSockName();
    if (udp_socket->cached_local_addr_.GetPort() == 0 || udp_socket->cached_local_addr_.GetPort() == 0xFFFF) {
      throw Exception("cached_local_addr_ has an unexpected value.");
    }
    udp_sockets_.Add(udp_socket);
    local_addr_to_udp_socket_map_.Add(udp_socket->cached_local_addr_, udp_socket);

    // IOCP에 연계한다.
    udp_socket->socket_->SetCompletionContext((ICompletionContext*)udp_socket.Get());
    AssertIsLockedByCurrentThread();
    net_thread_pool_->AssociateSocket(udp_socket->socket_.Get());

    // overlapped send를 하므로 송신 버퍼는 불필요하다.
    // socket의 send buffer를 없앤다. CSocketBuffer가 non swappable이므로 안전하다.
    // send buffer 없애기는 coalsecse, throttling을 위해 필수다.
    // recv buffer 제거는 백해무익이므로 즐

// zero copy send는 빠르지만 너무 많은 nonpaged를 유발 위험. 따라서 이걸로 막자. 막으니 성능도 더 나은데?
#ifdef ALLOW_ZERO_COPY_SEND
    udp_socket->socket_->SetSendBufferSize(0);
#endif

    NetUtil::SetUdpDefaultBehavior(udp_socket->socket_.Get());

    // 여러 클라를 상대로 UDP 수신을 하는 만큼 버퍼의 양이 충분히 커야 한다.
    //udp_socket->socket_->SetRecvBufferSize(NetConfig::ServerUdpRecvBufferLength);

    ScopedUseCounter counter(*udp_socket);
    CScopedLock2 udp_socket_guard(udp_socket->mutex_);

    // Give the first receive overlapped issue.
    udp_socket->ConditionalIssueRecvFrom();
  }

  return true;
}

RemoteClient_S* NetServerImpl::GetRemoteClientByUdpEndPoint_NOLOCK(const InetAddress& client_addr) {
  AssertIsLockedByCurrentThread();

  return udp_addr_to_remote_client_index_.FindRef(client_addr);
}

void NetServerImpl::PurgeTooOldAddMemberAckItem() {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  // clear ack-info if it is too old.
  const double absolute_time = GetAbsoluteTime();

  for (auto& pair : p2p_groups_) {
    auto group = pair.value;

    for (int32 member_index = 0; member_index < group->add_member_ack_waiters.Count(); ++member_index) {
      const auto& ack = group->add_member_ack_waiters[member_index];

      if ((absolute_time - ack.event_time) > NetConfig::purge_too_old_add_member_ack_timeout_sec) {
        const HostId joining_peer_id = ack.joining_member_host_id;

        // 이거 하나만 지우고 리턴. 나머지는 다음 기회에서 지워도 충분하니까.
        group->add_member_ack_waiters.RemoveAt(member_index);

        // 너무 오랫동안 ack가 안와서 제거되는 항목이 영구적 콜백 없음으로 이어져서는 안되므로 아래가 필요.
        if (!group->add_member_ack_waiters.AckWaitingItemExists(joining_peer_id)) {
          EnqueueP2PAddMemberAckCompleteEvent(group->group_id_, joining_peer_id, ResultCode::ConnectServerTimeout);
        }

        return;
      }
    }
  }
}

void NetServerImpl::PurgeTooOldUnmatureClient() {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  const double absolute_time = GetAbsoluteTime();

  const int32 list_count = candidate_remote_clients_.Count();
  if (list_count <= 0) {
    return;
  }

  Array<RemoteClient_S*,InlineAllocator<256>> collected_timeout_rc_list(list_count, NoInit);

  // 중국의 경우 tcp_socket_connect_timeout_sec * 2 + 3 일때 63이 나오므로 상향조정함.
  fun_check(NetConfig::client_connect_server_timeout_sec < 70);

  int32 timeout_count = 0;
  for (auto& rc_pair : candidate_remote_clients_) {
    auto rc = rc_pair.value;

    if ((absolute_time - rc->created_time_) > NetConfig::client_connect_server_timeout_sec && !rc->purge_requested_) {
      // 한번만 처리되게 하기 위해서 요청됨을 표시.
      rc->purge_requested_ = true;

      rc->IncreaseUseCount();
      collected_timeout_rc_list[timeout_count++] = rc;
    }
  }


  MessageOut msg_to_send;
  lf::Write(msg_to_send, MessageType::ConnectServerTimedout);

  // main unlock
  main_guard.Unlock();

  while (timeout_count > 0) {
#if USE_PARALLEL_FOR
#if (_MSC_VER >= 1400)
#pragma omp parallel for
#endif
#endif
    for (int32 timeout_index = 0; timeout_index < timeout_count; ++timeout_index) {
      auto rc = collected_timeout_rc_list[timeout_index];

      CScopedLock2 rc_tcp_send_queue_guard(rc->to_client_tcp_->GetSendQueueMutex(), false);

      if (timeout_index != 0) {
        if (rc_tcp_send_queue_guard.TryLock()) {
          rc->to_client_tcp_->SendWhenReady(SendFragRefs(msg_to_send), TcpSendOption());
          rc_tcp_send_queue_guard.Unlock();

          rc->DecreaseUseCount();
          collected_timeout_rc_list[timeout_index] = collected_timeout_rc_list[--timeout_count];
        }
      } else {
        rc_tcp_send_queue_guard.Lock();
        rc->to_client_tcp_->SendWhenReady(SendFragRefs(msg_to_send), TcpSendOption());
        rc_tcp_send_queue_guard.Unlock();

        rc->DecreaseUseCount();
        collected_timeout_rc_list[timeout_index] = collected_timeout_rc_list[--timeout_count];
      }
    }
  }
}

void NetServerImpl::ProcessOnClientDisposeCanSafe(RemoteClient_S* rc) {
  AssertIsLockedByCurrentThread();

  // 인덱스 정리
  udp_addr_to_remote_client_index_.Remove(rc->to_client_udp_fallbackable_.GetUdpAddrFromHere());

  // 송신 이슈 큐에서도 제거
  {
    //CScopedLock2 tcp_issue_queue_guard(tcp_issue_queue_mutex_);
    rc->UnlinkSelf();
  }

  // Dispose Client object (including sockets)
  if (rc->to_client_tcp_->socket_) {
    rc->to_client_tcp_->socket_->CloseSocketHandleOnly();

    if (intra_logger_) {
      const String text = String::Format("%s CloseSocketHandleOnly() called.  remote_client: %d", __FUNCTION__, (int32)rc->host_id_);
      intra_logger_->WriteLine(LogCategory::System, *text);
    }
  }

  if (rc->to_client_udp_fallbackable_.udp_socket) {
    //CScopedLock2 udp_socket_guard(rc->to_client_udp_fallbackable_.udp_socket_->GetMutex());
    rc->to_client_udp_fallbackable_.ResetPacketFragState();
  }

  // Remove from collections.
  candidate_remote_clients_.Remove(rc);
  authed_remote_clients_.Remove(rc->host_id_);
  host_id_factory_->Drop(GetAbsoluteTime(), rc->host_id_);
  udp_addr_to_remote_client_index_.Remove(rc->to_client_udp_fallbackable_.udp_addr_from_here_);
}

SessionKey* NetServerImpl::GetCryptSessionKey(HostId remote_id, String& out_error) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  SessionKey* key = nullptr;

  auto rc = GetAuthedClientByHostId_NOLOCK(remote_id);
  if (rc) {
    key = &rc->session_key_;
  } else if (remote_id == HostId_Server) {
    key = &self_session_key_;
  }

  if (key && !key->KeyExists()) {
    out_error = String::Format("key is not exists.");
    return nullptr;
  }

  if (key == nullptr) {
    out_error = String::Format("%d rc is %s in NetServer.", (int32)remote_id, rc == nullptr ? "null" : "not null");
  }

  return key;
}

//SendBroadcast
//SendCompressed
//SendSecured

bool NetServerImpl::Send( const SendFragRefs& data_to_send,
                          const SendOption& send_opt,
                          const HostId* sendto_list,
                          int32 sendto_count) {
  // The send is meaningless when the server is shutting down and does not do anything because it may cause errors during the sending process.
  if (!listener_) {
    return false;
  }

  // 메시지 압축 레이어를 통하여 메시지에 압축 여부 관련 헤더를 삽입한다.
  // 암호화 된 후에는 데이터의 규칙성이 없어져서 압축이 재대로 되지 않기 때문에 반드시 암호화 전에 한다.
  return Send_CompressLayer(data_to_send, send_opt, sendto_list, sendto_count);
}

bool NetServerImpl::Send_BroadcastLayer(const SendFragRefs& payload,
                                        const SendOption& send_opt,
                                        const HostId* sendto_list,
                                        int32 sendto_count) {
  // lock을 이제 여기서 따로 건다.
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  // NOTE: MaxDirectBroadcastCount는 서버에서 안쓰임
  const int32 unreliable_s2c_routed_multicast_max_count = MathBase::Max(send_opt.unreliable_s2c_routed_multicast_max_count, 0);
  const double unreliable_s2c_routed_multicast_max_ping = MathBase::Max(send_opt.unreliable_s2c_routed_multicast_max_ping, 0.0);

  // P2P 그룹을 host_id 리스트로 변환, 즉 ungroup한다.
  HostIdArray send_dest_list0;
  ConvertGroupToIndividualsAndUnion(sendto_count, sendto_list, send_dest_list0);

  // 각 수신 대상의 remote host object를 얻는다.
  SendDestInfoList_S send_dest_list;
  Convert_NOLOCK(send_dest_list, send_dest_list0);

  //@todo SendDestList가 비대하지는 않을런지? 스택 공간을 할당받아서 사용해야 하므로 대량은 곤란하다. 흠..
  int32 send_dest_list_count = send_dest_list.Count();

  // main lock이 있는 상태에서 수행해야 할것들을 처리하자
  if (send_opt.reliability == MessageReliability::Reliable) {
    int32 reliable_count = 0;
    Array<ISendDest_S*,InlineAllocator<256>> reliable_send_list(send_dest_list_count, NoInit);
    for (int32 dst_index = 0; dst_index < send_dest_list_count; ++dst_index) {
      auto& send_dest_info = send_dest_list[dst_index];
      auto send_dest = send_dest_info.object;

      // if loopback
      if (send_dest == this && send_opt.bounce) {
        // 즉시 final user work로 넣어버리자. 루프백 메시지는 사용자가 발생하는 것 말고는 없는 것을 전제한다.
        // 메시지 타입을 제거한 후 페이로드만 온전히 남겨준다음 넘겨줌.

        const ByteArray payload_data = payload.ToBytes(); // copy

        //EMssageType은 제거한 상태로 넘어가야함.
        MessageType msg_type;
        MessageIn payload_without_msg_type(payload_data);
        lf::Read(payload_without_msg_type, msg_type);

        FinalUserWorkItemType user_work_type;
        switch (msg_type) {
        case MessageType::RPC: user_work_type = FinalUserWorkItemType::RPC; break;
        case MessageType::FreeformMessage: user_work_type = FinalUserWorkItemType::FreeformMessage; break;
        default: fun_check(0); break;
        }

        // Post(이건 별도의 함수로 만들어도 좋을듯 싶은데...)
        final_user_work_queue_.Enqueue(FinalUserWorkItem_S(payload_without_msg_type, user_work_type));
        user_task_queue_.AddTaskSubject(this);
      } else if (send_dest && LeanDynamicCast(send_dest)) {
        auto rc = (RemoteClient_S*)send_dest;

        rc->IncreaseUseCount();
        reliable_send_list[reliable_count++] = send_dest;
      }
    }

    main_guard.Unlock();

    // reliable message 수신자들에 대한 처리.
    for (int32 dst_index = 0; dst_index < reliable_count; ++dst_index) {
      auto send_dest = reliable_send_list[dst_index];
      auto rc = (RemoteClient_S*)send_dest;

      // rc tcp send lock
      CScopedLock2 rc_send_guard(rc->GetSendMutex());
      rc->to_client_tcp_->SendWhenReady(payload, TcpSendOption());
      rc_send_guard.Unlock();

      rc->DecreaseUseCount();
    }
  } else { // MessageReliability::Unreliable
    int32 unreliable_count = 0;

    // host_id 리스트간에, P2P route를 할 수 있는 것들끼리 묶는다. 단, unreliable 메시징인 경우에 한해서만.
    // 여기에서 SendDestList가 재조정 될수 있으므로, SendDestListCount를 갠신해야함.
    Array<SendDestInfo_S*,InlineAllocator<256>> unreliable_send_info_list(send_dest_list_count, NoInit);
    MakeP2PRouteLinks(send_dest_list, unreliable_s2c_routed_multicast_max_count, unreliable_s2c_routed_multicast_max_ping);

    // 각 수신자에 대해...
    send_dest_list_count = send_dest_list.Count();
    for (int32 dst_index = 0; dst_index < send_dest_list_count; ++dst_index) {
      auto& send_dest_info = send_dest_list[dst_index];
      auto send_dest = send_dest_info.object;

      // if loopback
      // 루프백으로 요청되는 경우가 종종 있으려나??
      if (send_dest == this && send_opt.bounce) {
        // 메시지 타입을 제거한 후 페이로드만 온전히 남겨준다음 넘겨줌.
        // 코드가 중복된다. 별도의 함수로 만드는것도 좋을듯 싶다.

        ByteArray payload_data = payload.ToBytes(); // copy

        //EMssageType은 제거한 상태로 넘어가야함.
        MessageType msg_type;
        MessageIn payload_without_msg_type(payload_data);
        lf::Read(payload_without_msg_type, msg_type);

        FinalUserWorkItemType final_work_type;
        switch (msg_type) {
          case MessageType::RPC: final_work_type = FinalUserWorkItemType::RPC; break;
          case MessageType::FreeformMessage: final_work_type = FinalUserWorkItemType::FreeformMessage; break;
          default: fun_check(0); break;
        }

        // Post(이건 별도의 함수로 만들어도 좋을듯 싶은데...)
        final_user_work_queue_.Enqueue(FinalUserWorkItem_S(payload_without_msg_type, final_work_type));
        user_task_queue_.AddTaskSubject(this);
      } else if (send_dest && LeanDynamicCast(send_dest)) {
        auto rc = (RemoteClient_S*)send_dest;

        // UDP socket이 생성되어있지 않다면, UDP socket 생성을 요청한다.
        RemoteClient_NewLocalUdpSocketAndRequestNewRemoteUdpSocket(rc);

        if (rc->to_client_udp_fallbackable_.real_udp_enabled_) {
          rc->to_client_udp_fallbackable_.udp_socket_->IncreaseUseCount();
          send_dest_list[dst_index].host_object = rc->to_client_udp_fallbackable_.udp_socket_.Get();
          send_dest_list[dst_index].sendto_addr = rc->to_client_udp_fallbackable_.GetUdpAddrFromHere();
        } else {
          rc->IncreaseUseCount();
          send_dest_list[dst_index].host_object = rc;
        }

        unreliable_send_info_list[unreliable_count++] = &send_dest_info;
      }
    }

    main_guard.Unlock();


    // 각 unreliable message에 대해...
    for (int32 dst_index = 0; dst_index < unreliable_count; ++dst_index) {
      // 이미 위에서 객체가 있음을 보장하기때문에 따로 검사를 수행하지 않는다.
      auto org_send_dest_info = unreliable_send_info_list[dst_index];
      auto send_dest_info = org_send_dest_info;
      auto send_dest = send_dest_info->object;
      auto rc = (RemoteClient_S*)send_dest;

      // 항목의 P2P route prev link가 있으면, 즉 이미 P2P route로 broadcast가 된 상태이다.
      // 그러므로, 넘어간다.
      if (send_dest_info->p2p_route_prev_link) {
        // 아무것도 안함
      }
      // 항목의 P2P next link가 있으면, link의 끝까지 찾아서 P2P route 메시지 내용물에 추가한다.
      else if (send_dest_info->p2p_route_next_link) {
        MessageOut header;
        lf::Write(header, MessageType::S2CRoutedMulticast1);
        lf::Write(header, send_opt.priority);
        lf::Write(header, send_opt.unique_id);

        Array<HostId, InlineAllocator<NetConfig::OrdinaryHeavyS2CMulticastCount>> p2p_route_list;
        for (; send_dest_info; send_dest_info = send_dest_info->p2p_route_next_link) {
          p2p_route_list.Add(send_dest_info->host_id_);
        }
        lf::Write(header, p2p_route_list);

        lf::Write(header, OptimalCounter32(payload.GetTotalLength()));

        SendFragRefs s2c_routed_multicast_msg;
        s2c_routed_multicast_msg.Add(header);
        s2c_routed_multicast_msg.Add(payload);

        // HostId_none는 uniqueid로 씹히지 않는다.
        CScopedLock2 host_send_guard(org_send_dest_info->host_object->GetSendMutex());
        org_send_dest_info->host_object->SendWhenReady(HostId_None, org_send_dest_info->sendto_addr, rc->GetHostId(), s2c_routed_multicast_msg, send_opt);
      }
      // 항목의 P2P link가 전혀 없다. 그냥 보내도록 한다.
      else {
        CScopedLock2 host_send_guard(org_send_dest_info->host_object->GetSendMutex());
        org_send_dest_info->host_object->SendWhenReady(rc->GetHostId(), org_send_dest_info->sendto_addr, rc->GetHostId(), payload, send_opt);
      }

      org_send_dest_info->host_object->Decrease();
    }
  }

  return true;
}

bool NetServerImpl::CloseConnection(HostId client_id) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (auto conn = GetAuthedClientByHostId_NOLOCK(client_id)) {
    if (intra_logger_) {
      const String text = String::Format("NetServer::CloseConnection(%d) is called.", (int32)client_id);
      intra_logger_->WriteLine(LogCategory::System, *text);
    }

    // Handle it in a request form so that it can be disconnected in a safe state without disconnecting the connection.
    RequestAutoPrune(rc);
    return true;
  }

  return false;
}

void NetServerImpl::CloseAllConnections() {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  for (auto& pair : authed_remote_clients_) {
    auto& conn = pair.value;

    // If it has already been requested to close, ignore it.
    if (!rc->dispose_waiter_) {
      RequestAutoPrune(conn);
    }
  }
}

RemoteClient_S* NetServerImpl::GetRemoteClientByHostId_NOLOCK(HostId client_id) {
  AssertIsLockedByCurrentThread();
  return authed_remote_clients_.FindRef(client_id);
}

RemoteClient_S* NetServerImpl::GetAuthedClientByHostId_NOLOCK(HostId client_id) {
  AssertIsLockedByCurrentThread();
  auto rc = authed_remote_clients_.FindRef(client_id);
  return (rc && !rc->dispose_waiter_) ? rc : nullptr;
}

double NetServerImpl::GetLastPingSec(HostId peer_id) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  auto rc = authed_remote_clients_.FindRef(peer_id);
  return rc ? rc->last_ping_ : -1;
}

double NetServerImpl::GetRecentPingSec(HostId peer_id) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  auto rc = authed_remote_clients_.FindRef(peer_id);
  return rc ? rc->recent_ping_ : -1;
}

double NetServerImpl::GetP2PRecentPing(HostId host_a, HostId host_b) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (auto rc = GetAuthedClientByHostId_NOLOCK(host_a)) {
    for (auto pair : rc->p2p_connection_pairs_) {
      if (pair->ContainsHostId(host_b)) {
        if (pair->GetRelayed()) {
          auto rc2 = pair->first_client;

          if (rc2 == rc) {
            rc2 = pair->second_client;
          }

          return rc->recent_ping_ + rc2->recent_ping_;
        } else {
          return pair->recent_ping_;
        }
      }
    }
  }

  return 0;
}

bool NetServerImpl::DestroyP2PGroup(HostId group_id) {
  // 모든 멤버를 다 쫓아낸다.
  // 그러면 그룹은 자동 소멸한다.
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (auto group = GetP2PGroupByHostId_NOLOCK(group_id)) {
    while (!group->members_.IsEmpty()) {
      auto it = group->members_.CreateIterator();
      LeaveP2PGroup(it->key, group->group_id_);
    }

    // 다 끝났다. 이제 P2P 그룹 자체를 파괴해버린다.
    if (p2p_groups_.Remove(group_id)) {
      // HostId를 drop한다. 재사용을 위해.
      host_id_factory_->Drop(GetAbsoluteTime(), group_id);
      EnqueueP2PGroupRemoveEvent(group_id);
    }

    return true;
  }
  return false;
}

bool NetServerImpl::LeaveP2PGroup(HostId member_id, HostId group_id) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  auto group = GetP2PGroupByHostId_NOLOCK(group_id);
  if (!group) {
    return false;
  }

  if (!group->members_.Contains(member_id)) {
    // 원래 그룹에 없었거나, 이미 그룹에서 나간 멤버가 되겠다.
    // 이경우에는 시도 자체가 무의미한 시도가 되겠다.
    return false;
  }

  // 유효한 객체인지 체크
  P2PGroupMemberBase_S* member_rc;
  if (member_id == HostId_Server) {
    member_rc = this;
  } else {
    member_rc = GetAuthedClientByHostId_NOLOCK(member_id);
    if (member_rc == nullptr) {
      return false;
    }
  }

  // notify P2PGroup_MemberLeave to related members
  // notify it to the banished member
  for (const auto& member_pair : group->members_) {
    if (member_pair.key != HostId_Server) {
      s2c_proxy_.P2PGroup_MemberLeave(member_pair.key, GReliableSend_INTERNAL, member_id, group_id);
    }

    if (member_id != HostId_Server && member_id != member_pair.key) {
      s2c_proxy_.P2PGroup_MemberLeave(member_id, GReliableSend_INTERNAL, member_pair.key, group_id);
    }
  }

  // P2P 연결 쌍 리스트에서도 제거하되 중복 카운트를 감안한다.
  if (member_id != HostId_Server) {
    for (const auto& member_pair : group->members_) {
      if (member_pair.value.ptr->GetHostId() != HostId_Server && member_rc->GetHostId() != HostId_Server) {
        p2p_connection_pair_list_.ReleasePair(this, (RemoteClient_S*)member_rc, (RemoteClient_S*)member_pair.value.ptr);
      }
    }
  }

  // 멤버 목록에서 삭제
  group->members_.Remove(member_id);
  member_rc->joined_p2p_groups_.Remove(group_id);

  // Remove from every P2PGroup's add-member-ack list
  AddMemberAckWaiters_RemoveRelated_MayTriggerJoinP2PMemberCompleteEvent(group.Get(), member_id, ResultCode::UserRequested);

  // 그룹을 파괴하던지 재정비하던지, 옵션에 따라.
  if (group->members_.IsEmpty() && !empty_p2p_group_allowed_) {
    const HostId IdToRemove = group->group_id_;

    if (p2p_groups_.Remove(IdToRemove)) {
      // HostId를 drop한다 재사용을 위해.
      host_id_factory_->Drop(GetAbsoluteTime(), IdToRemove);
      EnqueueP2PGroupRemoveEvent(IdToRemove);
    }
  } else {
    // 멤버가 모두 나갔어도 P2P group을 파괴하지는 않는다. 이는 명시적으로 파괴되는 것이 정책이다.
    P2PGroup_RefreshMostSuperPeerSuitableClientId(group.Get());
    P2PGroup_CheckConsistency();
  }

  return true;
}

HostId NetServerImpl::CreateP2PGroup(const HostId* ClientHostIdList, int32 Count,
    const ByteArray& custom_field, const P2PGroupOption& option, HostId assigned_host_id) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  // 빈 그룹도 만드는 것을 허용한다. (옵션에 따라)
  if (Count < 0 || HostIdFactory == nullptr) {
    return HostId_None;
  }

  if (!empty_p2p_group_allowed_ && Count == 0) {
    return HostId_None;
  }

  Array<HostId, InlineAllocator<256>> UniqueClientHostIdList;
  if (Count > 0) {
    // 클라이언트 목록에서 중복 항목을 제거한다.
    UniqueClientHostIdList.Append(ClientHostIdList, Count);
    Algo::UnionDuplicateds(UniqueClientHostIdList);
    Count = UniqueClientHostIdList.Count();

    // 클라이언트 유효성 체크
    for (int32 i = 0; i < Count; ++i) {
      // Server인경우 클라이언트가 아니기 때문에 유효성 체크 넘어감
      //@todo (의미 없는 동작 건너뜀, 그런데 만약 서버가 멤버가 될 수 있다면? ...)
      if (UniqueClientHostIdList[i] == HostId_Server) {
        continue;
      }

      auto rc = GetAuthedClientByHostId_NOLOCK(UniqueClientHostIdList[i]);
      if (rc == nullptr) {
        return HostId_None;
      }

      if (!rc->session_key_.KeyExists()) {
        EnqueueError(ResultInfo::From(ResultCode::Unexpected, rc->host_id_, "CreateP2PGroup failed: session-key is missing."));
      }
    }
  }

  // 일단 빈 P2P group을 만든다.
  // 그리고 즉시 member들을 하나씩 추가한다.

  const HostId group_id = host_id_factory_->Create(GetAbsoluteTime(), assigned_host_id);
  if (group_id == HostId_None) {
    //@todo Why failed???
    return HostId_None;
  }


  P2PGroupPtr_S NewGroup(new P2PGroup_S());
  NewGroup->group_id_ = group_id;
  p2p_groups_.Add(NewGroup->group_id_, NewGroup);
  NewGroup->Option = Option;

  joined_p2p_group_key_gen_++;

  //@note add members into created group.
  for (int32 i = 0; i < Count; ++i) {
    JoinP2PGroup_INTERNAL(UniqueClientHostIdList[i], NewGroup->group_id_, custom_field, joined_p2p_group_key_gen_);
  }

  return NewGroup->group_id_;
}

bool NetServerImpl::JoinP2PGroup_INTERNAL(HostId member_id, HostId group_id, const ByteArray& custom_field, uint32 joined_p2p_group_key_gen) {
  AssertIsLockedByCurrentThread();

  // 그룹 유효성 체크
  auto group = GetP2PGroupByHostId_NOLOCK(group_id);
  if (!group) {
    return false;
  }

  const uint32 ack_key_serial = joined_p2p_group_key_gen;

  // 클라이언트 유효성 체크
  P2PGroupMember_S member_rc0;
  P2PGroupMemberBase_S* member_rc;

  if (group->members_.Contains(member_id)) {
    // 이미 그룹내에 들어가 있다고 이벤트를 한다.
    EnqueueP2PAddMemberAckCompleteEvent(group_id, member_id, ResultCode::AlreadyExists);
    return true;
  }

  // 새로 넣으려는 멤버가 유효한 값인지?
  if (settings_.server_as_p2p_group_member_allowed && member_id == HostId_Server) {
    member_rc = this;
  } else {
    member_rc = GetAuthedClientByHostId_NOLOCK(member_id);
    if (member_rc == nullptr || IsDisposeRemoteClient_NOLOCK((RemoteClient_S*)member_rc)) {
      return false;
    }
  }

  const double absolute_time = GetAbsoluteTime();

  bool add_member_ack_waiter_done = false;

  // 그룹데이터 업데이트
  member_rc0.ptr = member_rc;
  member_rc0.joined_time = absolute_time;
  group->members_.Add(member_id, member_rc0);

  JoinedP2PGroupInfo group_info;
  group_info.group_ptr = group;

  member_rc->joined_p2p_groups_.Add(group_id, group_info);

  // add ack-waiters(with event time) for current members to new member.
  for (auto old_member_it = group->members_.CreateIterator(); old_member_it; ++old_member_it) {
    auto old_member = old_member_it->value.ptr; // 기존 그룹의 각 멤버

    if (old_member != this) { // 서버로부터 join ack 메시지 수신은 없으므로
      P2PGroup_S::AddMemberAckWaiter ack_waiter;
      ack_waiter.joining_member_host_id = member_id;
      ack_waiter.old_member_host_id = old_member->GetHostId();
      ack_waiter.event_id = ack_key_serial;
      ack_waiter.event_time = absolute_time;
      group->add_member_ack_waiters.Add(ack_waiter);

      add_member_ack_waiter_done = true;
    }

    // 새 멤버로부터 기 멤버에 대한 join ack들이 일괄 와야 하므로 이것도 추가해야.
    // 단, 서버로부터는 join ack가 안오므로 제낀다.
    if (member_rc != this && member_rc != old_member) {
      P2PGroup_S::AddMemberAckWaiter ack_waiter;
      ack_waiter.joining_member_host_id = old_member->GetHostId();
      ack_waiter.old_member_host_id = member_id;
      ack_waiter.event_id = ack_key_serial;
      ack_waiter.event_time = absolute_time;
      group->add_member_ack_waiters.Add(ack_waiter);

      add_member_ack_waiter_done = true;
    }


    bool recycle = false;
    int32 bind_port = 0;

    // P2P connection pair를 갱신.
    P2PConnectionStatePtr p2p_state;
    if (old_member->GetHostId() != HostId_Server && member_rc->GetHostId() != HostId_Server) {
      auto old_member_as_rc = (RemoteClient_S*)old_member;
      auto new_member_as_rc = (RemoteClient_S*)member_rc;

      // 이미 P2P 직간접 통신중이 아니었다면 새로 쌍을 만들어 등록한다.
      // 이미 통신중이었다면 그냥 카운트만 증가시킨다.
      p2p_state = p2p_connection_pair_list_.GetPair(old_member_as_rc->host_id_, new_member_as_rc->host_id_);
      if (!p2p_state) {
        p2p_state = p2p_connection_pair_list_.GetRecyclePair(old_member_as_rc->host_id_, new_member_as_rc->host_id_, true);

        fun_check(!p2p_state || p2p_state->release_time_ > 0);

        if (!p2p_state || (p2p_state && (absolute_time - p2p_state->release_time_ > NetConfig::recycle_pair_reuse_time_sec))) {
          p2p_state.Reset(new P2PConnectionState(this, old_member_as_rc->host_id_ == new_member_as_rc->host_id_));

          p2p_state->SetRelayed(true);

          if (settings_.p2p_encrypted_messaging_enabled) {
            //@note 암호화된 P2P통신을 사용해야 할 경우, 인증키를 준비함.  랜덤블럭 준비.
            if (!CryptoRSA::CreateRandomBlock(p2p_state->p2p_aes_session_key_, settings_.strong_encrypted_message_key_length) ||
                !CryptoRSA::CreateRandomBlock(p2p_state->p2p_rc4_session_key_, settings_.weak_encrypted_message_key_length)) {
              if (intra_logger_) {
                const String text = String::Format("P2P session-key make failed between client %d and %d.",
                              (int32)old_member->GetHostId(), (int32)member_rc->GetHostId());
                intra_logger_->WriteLine(LogCategory::PP2P, *text);
              }
            }
          } else {
            //@note 암호화된 P2P통신이 아닐 경우에는 인증키 필요없음.
            p2p_state->p2p_aes_session_key_ = ByteArray();
            p2p_state->p2p_rc4_session_key_ = ByteArray();
          }

          p2p_state->p2p_fist_frame_number_ = RUdpHelper::GetRandomFrameNumber(random_); //첫번째 프레임은 임의대로함. 약간의 보안을 위해서..
          p2p_state->holepunch_tag_ = random_.NextUuid();
        } else {
          recycle = true;
        }

        p2p_state->dup_count = 1;

        p2p_connection_pair_list_.AddPair(old_member_as_rc, new_member_as_rc, p2p_state);

        p2p_state->MemberJoinStart(old_member_as_rc->host_id_, new_member_as_rc->host_id_);

        // RC에도 커넥션 페어를 링크해준다.
        old_member_as_rc->p2p_connection_pairs_.Add(p2p_state);
        new_member_as_rc->p2p_connection_pairs_.Add(p2p_state);

        if (intra_logger_) {
          const String text = String::Format("Prepare P2P pair for client %d and %d.",
                        (int32)old_member->GetHostId(), (int32)member_rc->GetHostId());
          intra_logger_->WriteLine(LogCategory::PP2P, *text);
        }
      } else {
        p2p_state->dup_count++;
      }
    }

    // 이게 있어야 받는 쪽에서는 member join noti를 받은 후에야
    // P2P RPC(relayed)를 받는 순서가 보장된다.
    AssertIsLockedByCurrentThread();

    if (old_member != this) {
      if (recycle && p2p_state) {
        bind_port = p2p_state->GetHolepunchedInternalAddr(old_member->GetHostId()).port();
      }

      if (settings_.p2p_encrypted_messaging_enabled) { // P2P with encryption
        // P2P_AddMember RPC with heterogeneous session-keys and event time.
        // session-key와 1st frame 번호를 보내는 것이 필요한 이유:
        // 릴레이 모드에서도 어쨌거나 frame number와 보안 통신이 필요하니껭.
        s2c_proxy_.P2PGroup_MemberJoin(
            old_member->GetHostId(),
            GSecureReliableSend_INTERNAL,
            group->group_id_,
            member_id,
            custom_field,
            ack_key_serial,
            p2p_state ? p2p_state->p2p_aes_session_key_ : ByteArray(),
            p2p_state ? p2p_state->p2p_rc4_session_key_ : ByteArray(),
            p2p_state ? p2p_state->p2p_fist_frame_number_ : (FrameNumber)0,
            p2p_state ? p2p_state->holepunch_tag_ : Uuid::None,
            group->Option.direct_p2p_enabled,
            bind_port);
      } else { // P2P without encryption
        // P2P_AddMember RPC with heterogeneous session-keys and event time.
        // session-key와 1st frame 번호를 보내는 것이 필요한 이유:
        // 릴레이 모드에서도 어쨌거나 frame number와 보안 통신이 필요하니껭.
        s2c_proxy_.P2PGroup_MemberJoin_Unencrypted(
            old_member->GetHostId(),
            GReliableSend_INTERNAL,
            group->group_id_,
            member_id,
            custom_field,
            ack_key_serial,
            p2p_state ? p2p_state->p2p_fist_frame_number_ : (FrameNumber)0,
            p2p_state ? p2p_state->holepunch_tag_ : Uuid::None,
            group->Option.direct_p2p_enabled,
            bind_port);
      }
    }

    // 자기 자신에 대해서의 경우 이미 한번은 위 라인에서 보냈으므로
    // 이번에는 또 보내지는 않는다.
    if (old_member != member_rc && member_id != HostId_Server) {
      if (recycle && p2p_state) {
        bind_port = p2p_state->GetHolepunchedInternalAddr(member_id).port();
      }

      if (settings_.p2p_encrypted_messaging_enabled) {
        //@todo p2p_state 중복 체크 제거

        s2c_proxy_.P2PGroup_MemberJoin(
            member_id,
            GSecureReliableSend_INTERNAL,
            group->group_id_,
            old_member->GetHostId(),
            custom_field,
            ack_key_serial,
            p2p_state ? p2p_state->p2p_aes_session_key_ : ByteArray(),
            p2p_state ? p2p_state->p2p_rc4_session_key_ : ByteArray(),
            p2p_state ? p2p_state->p2p_fist_frame_number_ : (FrameNumber)0,
            p2p_state ? p2p_state->holepunch_tag_ : Uuid::None,
            group->Option.direct_p2p_enabled,
            bind_port);
      } else {
        //@todo p2p_state 중복 체크 제거

        s2c_proxy_.P2PGroup_MemberJoin_Unencrypted(
            member_id,
            GReliableSend_INTERNAL,
            group->group_id_,
            old_member->GetHostId(),
            custom_field,
            ack_key_serial,
            p2p_state ? p2p_state->p2p_fist_frame_number_ : (FrameNumber)0,
            p2p_state ? p2p_state->holepunch_tag_ : Uuid::None,
            group->Option.direct_p2p_enabled,
            bind_port);
      }
    }

    if (intra_logger_ && p2p_state) {
      const String text = String::Format("P2P tag created by server. tag: %s", *p2p_state->holepunch_tag_.ToString());
      intra_logger_->WriteLine(LogCategory::PP2P, *text);
    }
  }

  P2PGroup_RefreshMostSuperPeerSuitableClientId(group.Get());
  P2PGroup_CheckConsistency();

  // 성공적으로 멤버를 넣었으나 ack waiter가 추가되지 않은 경우
  // (예: 빈 그룹에 서버 1개만 추가) 바로 완료 콜백을 때린다.
  if (!add_member_ack_waiter_done) {
    EnqueueP2PAddMemberAckCompleteEvent(group_id, HostId_Server, ResultCode::Ok);
  }

  return true;
}

bool NetServerImpl::JoinP2PGroup(HostId member_id, HostId group_id, const ByteArray& custom_field) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  return JoinP2PGroup_INTERNAL(member_id, group_id, custom_field, ++joined_p2p_group_key_gen_);
}

UdpSocketPtr_S NetServerImpl::GetAnyUdpSocket() {
  const int32 random_index = random_.RangeI(0, udp_sockets_.Count() - 1);
  return udp_sockets_[random_index];
}

void NetServerImpl::EnqueueClientLeaveEvent(
      RemoteClient_S* rc,
      ResultCode result_code,
      ResultCode detail_code,
      const ByteArray& comment,
      SocketErrorCode socket_error) {
  AssertIsLockedByCurrentThread();

  if (callbacks_ && rc->host_id_ != HostId_None) {
    LocalEvent event(LocalEventType::ClientLeaveAfterDispose);
    event.result_info.Reset(new ResultInfo());
    event.result_info->result_code = result_code;
    event.result_info->detail_code = detail_code;
    event.client_info = rc->GetClientInfo();
    event.comment = comment;
    event.remote_id = rc->host_id_;

    // Per-remote event로 변경.
    //EnqueueLocalEvent(event);
    rc->EnqueueLocalEvent(event);

    //// 121 에러 감지를 위함
    //if (socket_error == 121) {
    //  String text;
    //  text = String::Format("socket_error=121! source={%s, %d}, client_info={%s}, CCU = %d, SrvConfig={%s}",
    //      rc->GetDisposeCaller() ? rc->GetDisposeCaller() : "",
    //      rc->dispose_waiter_ ? rc->dispose_waiter_->reason : 0,
    //      * e.peer_info->ToString(true),
    //      authed_remote_clients_.Count(),
    //      *GetConfigString()
    //    );
    //
    //  ErrorReporter::Report(text);
    //}
  }

  //TODO
  //if (viz_agent_) {
  //  CScopedLock2 viz_agent_guard(viz_agent_->main_mutex_);
  //  viz_agent_->C2SProxy.NotifySrv_Clients_Remove(HostId_Server, GReliableSend_INTERNAL, rc->host_id_);
  //}
}

INetCoreCallbacks* NetServerImpl::GetCallbacks_NOLOCK() {
  AssertIsNotLockedByCurrentThread();

  return callbacks_;
}

NetServerImpl::NetServerImpl()
  : main_mutex_(),
    user_task_queue_(this),
    heartbeat_tickable_timer_("NetServer.Heartbeat") {
  ServerSocketPool = ServerSocketPool::GetSharedPtr();

  internal_version_ = NetConfig::InternalNetVersion;

  //tear_down_ = false;
  callbacks_ = nullptr;

  AttachProxy(&src_proxy_); s2c_proxy_.engine_specific_only_ = true;
  AttachStub(&c2s_stub_); c2s_stub_.engine_specific_only_ = true;

  c2s_stub_.owner = this;

  user_task_is_running_ = false;

  speed_hack_detector_reck_ratio_ = 1;

  // 빈 P2P그룹을 제거하지 않고 유지할지 여부를 설정함.
  empty_p2p_group_allowed_ = false; //기존 값은 true(허용)이었음.

  // 용량을 미리 잡아놓는다.
  router_index_list_.Reserve(NetConfig::OrdinaryHeavyS2CMulticastCount);

  joined_p2p_group_key_gen_ = 0;

  timer_callback_interval_ = 0;
  timer_callback_context_ = nullptr;
  start_create_p2p_group_ = false;

  total_tcp_recv_count = 0;
  total_tcp_recv_bytes = 0;
  total_tcp_send_count = 0;
  total_tcp_send_bytes = 0;
  total_udp_recv_count = 0;
  total_udp_recv_bytes = 0;
  total_udp_send_bytes = 0;
  total_udp_send_count = 0;

  net_thread_pool_ = nullptr;
  user_thread_pool_ = nullptr;
  net_thread_external_use_ = false;
  user_thread_external_use_ = false;

  tick_timer_id_ = 0;
  heartbeat_timer_id_ = 0;
  issue_send_on_need_timer_id_ = 0;
  heartbeat_working_ = 0;
  on_tick_working_ = 0;
}

//TODO 담을 수 있는 갯수를 제한하는게 좋은건가??
int32 NetServerImpl::GetClientHostIds(HostId* output, int32 max_output_len) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  int32 out_count = 0;
  for (auto& pair : authed_remote_clients_) {
    if (out_count < max_output_len) {
      auto& rc = pair.value;
      output[out_count++] = rc->host_id_;
    } else {
      break;
    }
  }
  return out_count;
}

bool NetServerImpl::GetClientInfo(HostId client_id, NetClientInfo& out_info) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (auto rc = GetRemoteClientByHostId_NOLOCK(client_id)) {
    rc->GetClientInfo(out_info);
    return true;
  }

  return false;
}

P2PGroupPtr_S NetServerImpl::GetP2PGroupByHostId_NOLOCK(HostId group_id) {
  AssertIsLockedByCurrentThread();

  return p2p_groups_.FindRef(group_id);
}

bool NetServerImpl::GetP2PGroupInfo(HostId group_id, P2PGroupInfo& out_group_info) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (auto group = GetP2PGroupByHostId_NOLOCK(group_id)) {
    group->GetInfo(out_group_info);
    return true;
  }

  return false;
}

// P2P 그룹을 host id list로 변환 후 배열에 이어붙인다.
void NetServerImpl::ConvertAndAppendP2PGroupToPeerList(HostId send_to, HostIdArray& SendTo2) {
  AssertIsLockedByCurrentThread();

  if (auto group = GetP2PGroupByHostId_NOLOCK(send_to)) {
    for (auto& pair : group->members_) {
      const HostId member_id = pair.key;
      SendTo2.Add(member_id);
    }
  } else {
    // 이미 dispose로 들어간 remote는 추가 하지 말자.
    // Disposing중일 경우에는 GetAuthedClientByHostId_NOLOCK가 nullptr반환
    if (send_to == HostId_Server || GetAuthedClientByHostId_NOLOCK(send_to)) {
      SendTo2.Add(send_to);
    }
  }
}

bool NetServerImpl::GetP2PConnectionStats(HostId remote_id, P2PConnectionStats& out_stats) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (auto rc = GetAuthedClientByHostId_NOLOCK(remote_id)) {
    out_stats.total_p2p_count = 0;
    out_stats.direct_p2p_count = 0;
    out_stats.to_remote_peer_send_udp_message_success_count = rc->to_remote_peer_send_udp_message_success_count_;
    out_stats.to_remote_peer_send_udp_message_attempt_count = rc->to_remote_peer_send_udp_message_attempt_count_;

    for (auto& pair : rc->p2p_connection_pairs_) {
      // 이것은 전에 한번이라도 p2p로 묶을것을 요청받은적이 있는지에 대한값이다.
      // 그렇기 때문에 이것이 false라면 카운트에 들어가지 않는다.
      if (pair->jit_direct_p2p_requested_) {
        ++out_stats.total_p2p_count;

        if (!pair->GetRelayed()) {
          ++out_stats.direct_p2p_count;
        }
      }
    }

    return true;
  }
  else if (auto group = GetP2PGroupByHostId_NOLOCK(remote_id)) {
    // 그룹아이디라면 그룹에서 얻는다.
    out_stats.total_p2p_count = 0;
    out_stats.direct_p2p_count = 0;
    out_stats.to_remote_peer_send_udp_message_success_count = 0;
    out_stats.to_remote_peer_send_udp_message_attempt_count = 0;

    Array<P2PConnectionState*> visited;
    //TODO Support SetMinCapacity
    //멤버갯수의 제곱의 반, 단 최소 1이상.
    //visited.SetMinCapacity((int32)MathBase::Max(pow((double)group->members_.size(), 2) * 0.5, 1.0));
    visited.Reserve((int32)MathBase::Max(pow((double)group->members_.Count(), 2) * 0.5, 1.0));

    for (auto& member_pair : group->members_) {
      if (member_pair.value.ptr->GetHostId() != HostId_Server) {
        auto member_as_rc = (RemoteClient_S*)member_pair.value.ptr;

        // 모든 group원의 총합을 구한다.
        if (member_as_rc == nullptr) {
          continue;
        }

        out_stats.to_remote_peer_send_udp_message_success_count += member_as_rc->to_remote_peer_send_udp_message_success_count_;
        out_stats.to_remote_peer_send_udp_message_attempt_count += member_as_rc->to_remote_peer_send_udp_message_attempt_count_;

        for (const auto& conn_pair_of_member : member_as_rc->p2p_connection_pairs_) {
          // 다른 그룹일경우 제외하자.
          if (!group->members_.Contains(conn_pair_of_member->first_client->host_id_) ||
            !group->members_.Contains(conn_pair_of_member->second_client->host_id_)) {
            continue;
          }

          // 이미 검사한 커넥션 페어는 검사하지 않는다.
          if (visited.Contains(conn_pair_of_member.Get())) {
            continue;
          }

          if (conn_pair_of_member->jit_direct_p2p_requested) {
            out_stats.total_p2p_count++;

            if (!conn_pair_of_member->GetRelayed()) {
              out_stats.direct_p2p_count++;
            }
          }

          visited.Add(conn_pair_of_member.Get());
        }
      }
    }

    return true;
  }

  return false;
}

bool NetServerImpl::GetP2PConnectionStats(HostId remote_a, HostId remote_b, P2PPairConnectionStats& out_stats) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  out_stats.to_remote_a_send_udp_message_success_count = 0;
  out_stats.to_remote_a_send_udp_message_attempt_count = 0;
  out_stats.to_remote_b_send_udp_message_success_count = 0;
  out_stats.to_remote_b_send_udp_message_attempt_count = 0;

  auto rc_a = GetAuthedClientByHostId_NOLOCK(remote_a);
  auto rc_b = GetAuthedClientByHostId_NOLOCK(remote_b);

  if (rc_a && rc_b) {
    for (const auto& pair : rc_a->p2p_connection_pairs_) {
      if (pair->ContainsHostId(rc_b->host_id_)) {
        out_stats.to_remote_b_send_udp_message_success_count = pair->to_remote_peer_send_udp_message_success_count_;
        out_stats.to_remote_b_send_udp_message_attempt_count = pair->to_remote_peer_send_udp_message_attempt_count_;
      }
    }

    for (const auto& pair : rc_b->p2p_connection_pairs_) {
      if (pair->ContainsHostId(rc_a->host_id_)) {
        out_stats.to_remote_a_send_udp_message_success_count = pair->to_remote_peer_send_udp_message_success_count_;
        out_stats.to_remote_a_send_udp_message_attempt_count = pair->to_remote_peer_send_udp_message_attempt_count_;
      }
    }

    return true;
  }

  return false;
}

ISendDest_S* NetServerImpl::GetSendDestByHostId_NOLOCK(HostId host_id) {
  AssertIsLockedByCurrentThread();

  if (host_id == HostId_Server) { // server
    return this;
  } else if (host_id == HostId_None) { // none
    return &ISendDest_S::None;
  } else { // RCs
    return GetAuthedClientByHostId_NOLOCK(host_id);
  }
}

bool NetServerImpl::IsFinalReceiveQueueEmpty() {
  AssertIsLockedByCurrentThread();

  return final_user_work_queue_.IsEmpty();
}

bool NetServerImpl::IsTaskRunning() {
  AssertIsLockedByCurrentThread();

  return user_task_is_running_;
}

void NetServerImpl::OnSetTaskRunningFlag(bool running) {
  AssertIsLockedByCurrentThread();

  user_task_is_running_ = running;
}

bool NetServerImpl::PopFirstUserWorkItem(FinalUserWorkItem& out_item) {
  AssertIsLockedByCurrentThread();

  if (!final_user_work_queue_.IsEmpty()) {
    out_item.from(final_user_work_queue_.Front(), HostId_Server);
    final_user_work_queue_.RemoveFront();
    return true;
  }

  return false;
}

// 클라이언트를 종료 모드로 바꾸도록 지시한다.
// 바로 객체를 파괴할 수 없다. NetWorkerThread에서 먼저 끝내줄 때까지 기다려야 하니까.
// 이 함수는 마구 호출해도 괜찮다.
void NetServerImpl::IssueDisposeRemoteClient(
      RemoteClient_S* rc,
      ResultCode result_code,
      ResultCode detail_code,
      const ByteArray& comment,
      const char* where,
      SocketErrorCode socket_error) {
  AssertIsLockedByCurrentThread();

  // 내부 절차는 아래와 같다.
  // 먼저 TCP socket을 close한다. (UDP는 공용되므로 불필요)
  // then issue중이던 것들은 에러가 발생한다. 혹은 issuerecv/send에서 이벤트 없이 에러가 발생한다.
  // 이때 recv on progress, send on progress가 중지될 것이다. 물론 TCP에 한해서 말이다.
  // 양쪽 모두 중지 확인되면 즉시 dispose를 한다.
  // group info 등을 파괴하는 것은 즉시 하지 않고 이 객체가 파괴되는 즉시 하는게 맞다.

  if (!rc->dispose_waiter_) { // just once
    rc->dispose_waiter_.Reset(new RemoteClient_S::DisposeWaiter());
    rc->dispose_waiter_->reason = result_code;
    rc->dispose_waiter_->detail = detail_code;
    rc->dispose_waiter_->comment = comment;
    rc->dispose_waiter_->socket_error = socket_error;

    if (intra_logger_) {
      const String text = String::Format("Call IssueDisposeRemoteClient() client=%d in %s.", (int32)rc->host_id_, where);
      intra_logger_->WriteLine(LogCategory::System, *text);
    }
  }

  if (!dispose_issued_remote_clients_map_.Contains(rc)) {
    // authed에서 빠지지않음.
    // 아직 남아있는 이벤트를 noti해야 하기때문이다.
    //if (remove_from_collection) {
    //  RemoteClient_RemoveFromCollections(rc);
    //}

    // 이제 여기서 per-remote로 LeaveEvent를 띄운다.
    if (rc->dispose_waiter_) {
      EnqueueClientLeaveEvent(rc, rc->dispose_waiter_->reason, rc->dispose_waiter_->detail, rc->dispose_waiter_->comment, rc->dispose_waiter_->socket_error);
    } else {
      EnqueueClientLeaveEvent(rc, ResultCode::DisconnectFromLocal, ResultCode::ConnectServerTimeout, ByteArray(), SocketErrorCode::Ok);
    }

    dispose_issued_remote_clients_map_.Add(rc, GetAbsoluteTime());

    // stats
    //CCounters::DecreaseBy("NetServer", "CCU", 1);
  } else {
    return; // 이렇게 하면 아래 CloseSocketHandleOnly가 자주호출되는 문제를 피할 듯.
  }

  // When you close the socket like this, all the issues that were in the middle of the issue are terminated.
  // And then you try to retry.
  // it will not be because there is DisposeWaiter. Then you can safely destroy the object.
  // Additionaly, record the total amount sent to this remote before closing the socket.

  if (intra_logger_) {
    //TODO
    // 접속 유지한 시간
    // 송신 횟수
    // 송신 바이트 수
    // 수신 횟수
    // 수신 바이트 수
    // 측정된 핑
    // 또 뭐가 있을까....

    //const String text = String::Format("RemoteClient stats in connected.  remote=%d, TotalSentBytes=%u bytes, TotalRecvBytes=%u bytes.",
    //        rc->host_id_, rc->to_client_tcp_->total_tcp_issued_send_bytes_, rc->to_client_tcp_->TotalTcpIssueRecvBytes);
    const String text = String::Format("RemoteClient stats in connected.  remote_id: %d, total_sent_bytes: %u bytes", (int32)rc->host_id_, rc->to_client_tcp_->total_tcp_issued_send_bytes_);
    intra_logger_->WriteLine(LogCategory::System, *text);
  }

  rc->to_client_tcp_->socket_->CloseSocketHandleOnly();

  if (rc->owned_udp_socket_) { // per rc udp 인경우 즉, RC당 UDPSocket이 할당된 경우, 할당된 UDP 소켓을 닫아줌.
    rc->owned_udp_socket_->socket_->CloseSocketHandleOnly();
  }

  rc->WarnTooShortDisposal(where);

  for (auto& group_pair : rc->joined_p2p_groups_) {
    // Remove from P2PGroup_Add ack info
    auto& group = group_pair.value.group_ptr;

    AddMemberAckWaiters_RemoveRelated_MayTriggerJoinP2PMemberCompleteEvent(group.Get(), rc->host_id_, ResultCode::DisconnectFromRemote);

    // Notify member leave to related group members
    for (const auto& member_pair : group->members_) {
      if (member_pair.key != HostId_Server) { // server does not need to receive P2PGroup_MemberLeave.
        s2c_proxy_.P2PGroup_MemberLeave(member_pair.key, GReliableSend_INTERNAL, rc->host_id_, group->group_id_);
      }
    }

    // Remove this remote-client from P2P group.
    group->members_.Remove(rc->host_id_);

    // If the P2P group has to remain after the expulsion...
    if (!group->members_.IsEmpty() || empty_p2p_group_allowed_) {
      P2PGroup_RefreshMostSuperPeerSuitableClientId(group.Get());
    } else {
      // If a P2P group needs to be destroyed...
      const HostId GroupHostIdToDelete = group->group_id_;

      if (p2p_groups_.Remove(GroupHostIdToDelete)) {
        host_id_factory_->Drop(GetAbsoluteTime(), GroupHostIdToDelete);

        EnqueueP2PGroupRemoveEvent(GroupHostIdToDelete);
      }
    }

    // In the client exit event, 'backup P2P group that remained
    // until just before the process of leaving' should be given.
    rc->had_joined_p2p_groups_.Add(group->group_id_);
  }

  // Clear all joined groups.
  rc->joined_p2p_groups_.Clear();

  // Clients find a list of peers with P2P connections and remove all
  p2p_connection_pair_list_.RemovePairOfAnySide(rc);
}

void NetServerImpl::DisposeIssuedRemoteClients() {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  const int32 Num = dispose_issued_remote_clients_map_.Count();

  for (auto it = dispose_issued_remote_clients_map_.CreateIterator(); it; ++it) {
    auto rc = it->key;

    HardDisconnect_AutoPruneGoesTooLongClient(rc);

    // rc lock
    CScopedLock2 rc_tcp_guard(rc->to_client_tcp_->GetMutex());
    CScopedLock2 udp_socket_guard;
    CScopedLock2 udp_socket_fragger_guard;

    if (rc->to_client_udp_fallbackable_.udp_socket_) {
      //warning: care locking order.
      udp_socket_guard.SetMutex(rc->to_client_udp_fallbackable_.udp_socket_->GetMutex(), true);
      udp_socket_fragger_guard.SetMutex(rc->to_client_udp_fallbackable_.udp_socket_->GetFraggerMutex(), true);
    }

    CScopedLock2 tcp_issue_queue_guard(tcp_issue_queue_mutex_);
    CScopedLock2 udp_issue_queue_guard(udp_issue_queue_mutex_);

    // UseCount 검사
    const bool use_count_safe = rc->GetUseCount() == 0;

    // 양쪽 모두 중지 확인되면 즉시 dispose를 한다.
    // 300초가 넘으면 무조건 dispose를 한다. 300에 훨씬 못미치는 초 안에
    // 충분히 recv, send는 completion이 발생하기 때문이다.
    // 서버가 과부하가 걸리면 closesocket후 몇십초가 지나도 recv completion 이 안올
    // 가능성은 존재하기 마련. 하지만 300초씩이나 걸릴 정도면 막장 상황이므로 차라리 서버를 끄는게 낫다.
    const bool tcp_close_safe = !rc->to_client_tcp_->recv_issued_ && !rc->to_client_tcp_->send_issued_;

    bool udp_close_safe;
    if (!rc->owned_udp_socket_) {
      udp_close_safe = true;
    } else {
      udp_close_safe = !rc->owned_udp_socket_->recv_issued_ && !rc->owned_udp_socket_->send_issued_ && rc->owned_udp_socket_->GetUseCount() == 0;
    }

    const bool works_remain = (rc->task_subject_node_.GetListOwner() || rc->IsTaskRunning() || rc->GetListOwner());

    rc_tcp_guard.Unlock(); // 이미 close된 소켓이므로 여기서 unlock해도 무방.

    // 소켓이 잘닫히고 일거리가 남아있지 않다면...
    if (use_count_safe && tcp_close_safe && udp_close_safe && !works_remain) {
      ProcessOnClientDisposeCanSafe(rc);

      remote_client_instances_.Remove(rc);

      if (udp_socket_guard.IsLocked()) {
        // lock의 파괴자가 나중에 호출 되므로 이경우는 unlock를 먼저 해줘야 하겠다.
        udp_socket_guard.Unlock();
      }

      if (udp_socket_fragger_guard.IsLocked()) {
        udp_socket_fragger_guard.Unlock();
      }

      it.RemoveCurrent();

      delete rc; // 실제로 여기서 제거함
    }
  }
}

double NetServerImpl::GetTime() {
  return GetAbsoluteTime();
}

int32 NetServerImpl::GetClientCount() {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  return authed_remote_clients_.Count();
}

NetServerImpl::~NetServerImpl() {
  Stop();

  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  // RZ 내부에서도 쓰는 RPC까지 더 이상 참조되지 않음을 확인해야 하므로 여기서 시행해야 한다.
  CleanupEveryProxyAndStub(); // 꼭 이걸 호출해서 미리 청소해놔야 한다.
}


//외부에서 여기를 인식시키기 위한주소 조회하기.

//FIXME 여기서 주소가 IPv4/IPv6인지 헷갈린다...
//IPv4 / IPv6 둘중 어느것인지 정확하게 할필요가 있겠다..
//Listening 바인딩된 주소 기준으로 IPv4 인지 IPv6인지 구분해서 처리하는게 바람직해보임.
//Listening 바인딩된 주소 기준으로 IPv4 인지 IPv6인지 구분해서 처리하는게 바람직해보임.
//Listening 바인딩된 주소 기준으로 IPv4 인지 IPv6인지 구분해서 처리하는게 바람직해보임.
//Listening 바인딩된 주소 기준으로 IPv4 인지 IPv6인지 구분해서 처리하는게 바람직해보임.
//Listening 바인딩된 주소 기준으로 IPv4 인지 IPv6인지 구분해서 처리하는게 바람직해보임.
//Listening 바인딩된 주소 기준으로 IPv4 인지 IPv6인지 구분해서 처리하는게 바람직해보임.
//Listening 바인딩된 주소 기준으로 IPv4 인지 IPv6인지 구분해서 처리하는게 바람직해보임.
NamedInetAddress NetServerImpl::GetRemoteIdentifiableLocalAddr() {
  NamedInetAddress local_addr = NamedInetAddress::None;

  if (tcp_listening_socket_) {
    // 4순위 : Listening 소켓의 바인딩된 주소
    local_addr = NamedInetAddress(tcp_listening_socket_->GetSockName());

    // 3순위 : 로컬 NIC 주소들중 첫번째
    local_addr.OverwriteHostNameIfExists(NetUtil::LocalAddressAt(0).ToString());

    // 2순위 : 지정된 로컬 NIC 주소
    local_addr.OverwriteHostNameIfExists(local_nic_addr_);

    // 1순위 : 별칭 (결국 이게 최종이라는거임)
    local_addr.OverwriteHostNameIfExists(server_ip_alias_);

    // 최종적으로 설정된 주소가 unicast 주소가 아니라면, 그냥 첫번째 로컬 ip를 넣도록 한다.
    // 하지만 이는 fallback 처리이므로 로그를 남겨준다던지 하는것도 좋을듯 싶다.
    if (!local_addr.IsUnicast()) {
      local_addr.Address = NetUtil::LocalAddressAt(0).ToString();
    }
  }

  return local_addr;
}

void NetServerImpl::TcpAndUdp_LongTick() {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  int32 udp_socket_list_count = udp_sockets_.Count();
  int32 authed_remote_client_list_count = authed_remote_clients_.Count();
  int32 candidate_remote_client_list_count = candidate_remote_clients_.Count();

  int32 owned_socket_count = 0;

  Array<UdpSocket_S*,InlineAllocator<256>> udp_socket_list;
  if (udp_socket_list_count > 0) {
    owned_socket_count = udp_socket_list_count;
    udp_socket_list.ResizeUninitialized(udp_socket_list_count);
    for (int32 i = 0; i < udp_socket_list_count; ++i) {
      auto udp_socket = udp_sockets_[i].Get();

      udp_socket->IncreaseUseCount();
      udp_socket_list[i] = udp_socket;
    }
  }
  else {
    udp_socket_list_count = authed_remote_client_list_count + candidate_remote_client_list_count;
    udp_socket_list.ResizeUninitialized(udp_socket_list_count);
  }

  // Authed RC의 udp 소켓들도 수집.
  Array<RemoteClient_S*,InlineAllocator<256>> collected_authed_rc_list;
  if (authed_remote_client_list_count > 0) {
    collected_authed_rc_list.ResizeUninitialized(authed_remote_client_list_count);

    int32 count = 0;
    for (auto& rc_pair : authed_remote_clients_) {
      auto rc = rc_pair.value;

      if (rc->owned_udp_socket_) {
        rc->owned_udp_socket_->IncreaseUseCount();
        udp_socket_list[owned_socket_count++] = rc->owned_udp_socket_.Get();
      }

      rc->IncreaseUseCount();
      collected_authed_rc_list[count++] = rc;
    }
  }

  // Candidate RC의 udp 소켓들도 수집.
  Array<RemoteClient_S*,InlineAllocator<256>> collected_candidate_rc_list;
  if (candidate_remote_client_list_count > 0) {
    int32 count = 0;
    collected_candidate_rc_list.ResizeUninitialized(candidate_remote_client_list_count);
    for (auto& rc_pair : candidate_remote_clients_) {
      auto rc = rc_pair.value;

      if (rc->owned_udp_socket_) {
        rc->owned_udp_socket_->IncreaseUseCount();
        udp_socket_list[owned_socket_count++] = rc->owned_udp_socket_.Get();
      }

      rc->IncreaseUseCount();
      collected_candidate_rc_list[count++] = rc;
    }
  }

  const double absolute_time = GetAbsoluteTime();

  // main unlock
  main_guard.Unlock();

  while (owned_socket_count > 0) {
#if USE_PARALLEL_FOR
#if (_MSC_VER >= 1400)
#pragma omp parallel for
#endif
#endif
    for (int32 socket_index = 0; socket_index < owned_socket_count; ++socket_index) {
      auto udp_socket = udp_socket_list[socket_index];

      CScopedLock2 udp_socket_guard(udp_socket->GetMutex(), false);

      if (socket_index != 0) {
        if (udp_socket_guard.TryLock()) {
          udp_socket->LongTick(absolute_time);
          udp_socket_guard.Unlock();

          udp_socket->DecreaseUseCount();
          udp_socket_list[socket_index] = udp_socket_list[--owned_socket_count];
        }
      } else {
        udp_socket_guard.Lock();
        udp_socket->LongTick(absolute_time);
        udp_socket_guard.Unlock();

        udp_socket->DecreaseUseCount();
        udp_socket_list[socket_index] = udp_socket_list[--owned_socket_count];
      }
    }
  }

  while (authed_remote_client_list_count > 0) {
#if USE_PARALLEL_FOR
#if (_MSC_VER >= 1400)
#pragma omp parallel for
#endif
#endif
    for (int32 rc_index = 0; rc_index < authed_remote_client_list_count; ++rc_index) {
      auto rc = collected_authed_rc_list[rc_index];

      CScopedLock2 rc_tcp_send_queue_guard(rc->to_client_tcp_->GetSendQueueMutex(), false);

      if (rc_index != 0) {
        if (rc_tcp_send_queue_guard.TryLock()) {
          rc->to_client_tcp_->LongTick(absolute_time);
          rc_tcp_send_queue_guard.Unlock();

          rc->DecreaseUseCount();
          collected_authed_rc_list[rc_index] = collected_authed_rc_list[--authed_remote_client_list_count];
        }
      } else {
        rc_tcp_send_queue_guard.Lock();
        rc->to_client_tcp_->LongTick(absolute_time);
        rc_tcp_send_queue_guard.Unlock();

        rc->DecreaseUseCount();
        collected_authed_rc_list[rc_index] = collected_authed_rc_list[--authed_remote_client_list_count];
      }
    }
  }

  while (candidate_remote_client_list_count > 0) {
#if USE_PARALLEL_FOR
#if (_MSC_VER >= 1400)
#pragma omp parallel for
#endif
#endif
    for (int32 rc_index = 0; rc_index < candidate_remote_client_list_count; ++rc_index) {
      auto rc = collected_candidate_rc_list[rc_index];

      CScopedLock2 rc_tcp_send_queue_guard(rc->to_client_tcp_->GetSendQueueMutex(), false);

      if (rc_index != 0) {
        if (rc_tcp_send_queue_guard.TryLock()) {
          rc->to_client_tcp_->LongTick(absolute_time);
          rc_tcp_send_queue_guard.Unlock();

          rc->DecreaseUseCount();
          collected_candidate_rc_list[rc_index] = collected_candidate_rc_list[--candidate_remote_client_list_count];
        }
      } else {
        rc_tcp_send_queue_guard.Lock();
        rc->to_client_tcp_->LongTick(absolute_time);
        rc_tcp_send_queue_guard.Unlock();

        rc->DecreaseUseCount();
        collected_candidate_rc_list[rc_index] = collected_candidate_rc_list[--candidate_remote_client_list_count];
      }
    }
  }
}

void NetServerImpl::SetDefaultFallbackMethod(FallbackMethod fallback_method) {
  if (fallback_method == FallbackMethod::CloseUdpSocket) {
    throw Exception("not supported value yet.");
  }

  settings_.fallback_method = fallback_method;
}

//@maxidea:
//클라이언트에서 보내는 로그를 받을지 여부를 별도로 지정하는게 좋을듯 싶은데...

void NetServerImpl::EnableIntraLogging(const char* log_filename) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (!intra_logger_) {
    intra_logger_.Reset(LogWriter::New(log_filename));

    // 모든 클라이언트들에게 로그를 보내라는 명령을 한다.
    for (auto& rc_pair : authed_remote_clients_) {
      if (!rc_pair.value->dispose_waiter_) {
        s2c_proxy_.EnableIntraLogging(rc_pair.key, GReliableSend_INTERNAL);
      }
    }
  }
}

void NetServerImpl::DisableIntraLogging() {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (intra_logger_) {
    intra_logger_.Reset();

    // 모든 클라이언트들에게 로그를 보내지 말라는 명령을 한다.
    for (auto& rc_pair : authed_remote_clients_) {
      if (!rc_pair.value->dispose_waiter_) { // 종료 대기중인 것들은 제외.. 시도 해봐야 전송도 안될터...
        s2c_proxy_.DisableIntraLogging(rc_pair.key, GReliableSend_INTERNAL);
      }
    }
  }
}

void NetServerImpl::Heartbeat_PerClient() {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  const double absolute_time = GetAbsoluteTime();

  for (auto& rc_pair : authed_remote_clients_) {
    auto rc = rc_pair.value;

#if TRACE_ISSUE_DELAY_LOG
    if (intra_logger_) {
      if (rc->to_client_tcp) {
        if (!rc->to_client_tcp_->recv_issued_) {
          if (rc->to_client_tcp_->last_recv_issue_warning_time_ != 0.0) {
            if ((absolute_time - rc->to_client_tcp_->last_recv_issue_warning_time_) > 3.0) { //TODO: NetConfig로 빼주는게 좋을듯...
              rc->to_client_tcp_->last_recv_issue_warning_time_ = absolute_time;

              const String text = String::Format("host_id:%d - recvissued delay 3 second", (int32)rc->host_id_);
              intra_logger_->WriteLine(LogCategory::System, *text);
            }
          } else {
            rc->to_client_tcp_->last_recv_issue_warning_time_ = absolute_time;
          }
        } else {
          rc->to_client_tcp_->last_recv_issue_warning_time_ = 0.0;
        }

        if (!rc->to_client_tcp_->send_issued_) {
          if (rc->to_client_tcp_->last_send_issue_warning_time_ != 0.0) {
            if ((absolute_time - rc->to_client_tcp_->last_send_issue_warning_time_) > 5.0) { // TODO: NetConfig로 빼주는게 좋을듯...
              rc->to_client_tcp_->last_send_issue_warning_time_ = absolute_time;

              const String text = String::Format("host_id: %d - sendissued delay 5 second, InReadyList: %s", (int32)rc->host_id_, rc->GetListOwner() ? "true" : "false");
              intra_logger_->WriteLine(LogCategory::System, *text);
            }
          } else {
            rc->to_client_tcp_->last_send_issue_warning_time_ = absolute_time;
          }
        } else {
          rc->to_client_tcp_->last_send_issue_warning_time_ = 0;
        }
      }
    }
#endif //TRACE_ISSUE_DELAY_LOG

    if ((absolute_time - rc->last_tcp_stream_recv_time_) > settings_.default_timeout_sec) {
      if (!rc->dispose_waiter_) {
        if (intra_logger_) {
          const String text = String::Format("The TCP receive from the client %d no longer.  close the socket.", (int32)rc->GetHostId());
          intra_logger_->WriteLine(LogCategory::System, *text);
        }

        IssueDisposeRemoteClient(rc, ResultCode::DisconnectFromRemote, ResultCode::ConnectServerTimeout, ByteArray(), __FUNCTION__, SocketErrorCode::Ok);
      }

      continue;
    }

    HardDisconnect_AutoPruneGoesTooLongClient(rc);

    ConditionalFallbackServerUdpToTcp(rc, absolute_time);

    ConditionalArbitaryUdpTouch(rc, absolute_time);

    RefreshSendQueuedAmountStat(rc);
  }
}

// (클라이언트 요청에 의한) TCP fallback과 관련해서 서버 로컬에서의 처리 부분
//
// 임의로 fallback(TCP) 모드로 전환했을 경우..
void NetServerImpl::LocalProcessForFallbackUdpToTcp(RemoteClient_S* conn) {
  AssertIsLockedByCurrentThread();

  if (conn->to_client_udp_fallbackable_.real_udp_enabled_) {
    conn->to_client_udp_fallbackable_.real_udp_enabled_ = false;

    P2PGroup_RefreshMostSuperPeerSuitableClientId(conn);

    if (intra_logger_) {
      const String text = String::Format("Client %d cancel UDP hole punching with server.  client_local_addr: %s", (int32)conn->host_id_, *conn->to_client_tcp_->remote_addr_.ToString());
      //TODO 카테고리를 뭘로 해야할까??
      intra_logger_->WriteLine(LogCategory::P2P, *text);
    }
  }
}

void NetServerImpl::EnqueueHackSuspectEvent(RemoteClient_S* conn, const char* statement, HackType hack_type) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (callbacks_) {
    LocalEvent event(LocalEventType::HackSuspected);
    event.result_info.Reset(new ResultInfo());
    event.hack_type = hack_type;
    event.remote_id = conn ? conn->host_id_ : HostId_None;
    event.result_info->comment = statement;

    EnqueueLocalEvent(event);
  }
}

void NetServerImpl::SetSpeedHackDetectorReckRatio(double ratio) {
  if (ratio <= 0 || ratio >= 1) {
    throw InvalidArgumentException();
  }

  speed_hack_detector_reck_ratio_ = ratio;
}

void NetServerImpl::SetMessageMaxLength(int32 max_length) {
  if (max_length <= NetConfig::MessageMinLength) {
    throw InvalidArgumentException();
  }

  {
    CScopedLock2 ConfigWriteLock(NetConfig::GetWriteMutex());
    NetConfig::message_max_length = MathBase::Max(NetConfig::message_max_length, max_length); // 아예 전역 값도 수정하도록 한다.
  }

  settings_.message_max_length = max_length;
}

void NetServerImpl::SetDefaultTimeoutTimeMilisec(uint32 timeout_msec) {
  SetDefaultTimeoutTimeSec(((double)timeout_msec) / 1000);
}

void NetServerImpl::SetDefaultTimeoutTimeSec(double timeout_sec) {
  AssertIsNotLockedByCurrentThread();

  if (timeout_sec < 1) {
    if (callbacks_) {
      LOG(LogNetEngine, Warning, "Too short timeout value.  it may cause unfair disconnection.");
      return;
    }
  }

#ifndef _DEBUG
  if (timeout_sec > 240) {
    if (callbacks_) {
      //LOG(LogNetEngine, Warning, "too long timeout value. it may take a lot of time to detect lost connection.");
    }
  }
#endif

  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  CheckDefaultTimeoutTimeValidation(timeout_sec);
  settings_.default_timeout_sec = timeout_sec;
}

bool NetServerImpl::SetDirectP2PStartCondition(DirectP2PStartCondition condition) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (condition >= DirectP2PStartCondition::Last) {
    throw InvalidArgumentException();
  }

  settings_.direct_p2p_start_condition = condition;

  return true;
}

void NetServerImpl::GetStats(NetServerStats& out_stats) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  out_stats.Reset();
  out_stats.total_tcp_recv_count = total_tcp_recv_count_;
  out_stats.total_tcp_recv_bytes = total_tcp_recv_bytes_;
  out_stats.total_tcp_send_count = total_tcp_send_count_;
  out_stats.total_tcp_send_bytes = total_tcp_send_bytes_;

  out_stats.total_udp_recv_count = total_udp_recv_count_;
  out_stats.total_udp_recv_bytes = total_udp_recv_bytes_;
  out_stats.total_udp_send_count = total_udp_send_count_;
  out_stats.total_udp_send_bytes = total_udp_send_bytes_;

  out_stats.client_count = authed_remote_clients_.Count();

  out_stats.occupied_udp_port_count = udp_sockets_.Count();

  out_stats.real_udp_enabled_client_count = 0;
  for (auto& rc_pair : authed_remote_clients_) {
    auto rc = rc_pair.value;

    if (rc->to_client_udp_fallbackable_.real_udp_enabled_) {
      out_stats.real_udp_enabled_client_count++;
    }
  }

  out_stats.p2p_group_count = p2p_groups_.Count();
  out_stats.p2p_direct_connection_pair_count = 0;
  for (auto& active_pair : p2p_connection_pair_list_.active_pairs) {
    auto pair = active_pair.value;

    if (pair->jit_direct_p2p_requested_) {
      out_stats.p2p_connection_pair_count++;

      if (!pair->GetRelayed()) {
        out_stats.p2p_direct_connection_pair_count++;
      }
    }
  }
}

InetAddress NetServerImpl::GetTcpListenerLocalAddr() {
  return tcp_listening_socket_ ? tcp_listening_socket_->GetSockName() : InetAddress::None;
}

void NetServerImpl::GetUdpListenerLocalAddrs(Array<InetAddress>& output) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (udp_assign_mode != ServerUdpAssignMode::Static) {
    throw Exception("Cannot call GetUdpListenerLocalAddrs unless ServerUdpAssignMode::Static is used.");
  }

  output.Clear(); // Just in case

  for (int32 socket_index = 0; socket_index < udp_sockets_.Count(); ++socket_index) {
    if (udp_sockets_[socket_index]->socket) {
      output.Add(udp_sockets_[socket_index]->socket_->GetSockName());
    }
  }
}

void NetServerImpl::ConvertGroupToIndividualsAndUnion(int32 sendto_count, const HostId* sendto_list, HostIdArray& send_dest_list) {
  for (int32 i = 0; i < sendto_count; ++i) {
    if (sendto_list[i] != HostId_None) {
      ConvertAndAppendP2PGroupToPeerList(sendto_list[i], send_dest_list);
    }
  }

  algo::UnionDuplicateds(send_dest_list);
}

bool NetServerImpl::IsValidHostId(HostId host_id) {
  CScopedLock2 main_guard(main_mutex_);
  return IsValidHostId_NOLOCK(host_id);
}

// P2P route가 가능한 항목끼리 linked list를 구축한다.
void NetServerImpl::MakeP2PRouteLinks(SendDestInfoList_S& tgt, int32 unreliable_s2c_routed_multicast_max_count, double routed_send_max_ping) {
  AssertIsLockedByCurrentThread();

  // 이럴경우 link할 필요가 없다. 아래 로직은 많은 연산량을 요구하므로.
  if (unreliable_s2c_routed_multicast_max_count == 0) {
    return;
  }

  connection_info_list_.Reset(); //remarks: don't reset capacity

  ConnectionInfo info;
  int32 i, j, k;
  int32 tgt_count = tgt.Count();
  for (i = 0; i < tgt_count; ++i) {
    auto last_linked_item = &tgt[i];

    last_linked_item->p2p_route_prev_link = nullptr;
    last_linked_item->p2p_route_next_link = nullptr;

    for (j = i; j < tgt_count; ++j) {
      P2PConnectionStatePtr state;

      if (i != j && (state = p2p_connection_pair_list_.GetPair(tgt[i].host_id, tgt[j].host_id))) {
        if (!state->GetRelayed() && state->recent_ping_ < routed_send_max_ping) {
          info.state = state.Get();
          info.host_index0 = i;
          info.host_index1 = j;
          connection_info_list_.Add(info);

          // 현재 host의 directP2P 연결 갯수를 센다.
          int32 connection_info_list_count = connection_info_list_.Count();
          for (k = 0; k < connection_info_list_count; ++k) {
            if (connection_info_list_[k].host_index0 == i) {
              ++connection_info_list_[k].connect_count0;
            } else if (connection_info_list_[k].host_index1 == i) {
              ++connection_info_list_[k].connect_count1;
            }

            if (connection_info_list_[k].host_index0 == j) {
              ++connection_info_list_[k].connect_count0;
            } else if (connection_info_list_[k].host_index1 == j) {
              ++connection_info_list_[k].connect_count1;
            }
          }

          info.Reset();
        }
      }
    }
  }

  // ping 순으로 정렬한다.
  connection_info_list_.Sort();

  // router 후보 선별하여 indexlist에 담는다.
  router_index_list_.Reset();

  for (i = 0; i < connection_info_list_.Count(); ++i) {
    const auto& info2 = connection_info_list_[i];

    if (router_index_list_.Contains(info2.host_index0) || router_index_list_.Contains(info2.host_index1)) {
      continue;
    }

    if (info2.connect_count0 >= info2.connect_count1) {
      router_index_list_.Add(info2.host_index0);
    } else {
      router_index_list_.Add(info2.host_index1);
    }
  }

  // 링크를 연결해준다.
  int32 host_index = -1;
  int32 next_index = -1;
  SendDestInfo_S* linked_item = nullptr;
  SendDestInfo_S* next_linked_item = nullptr;

  int32 router_index_list_count = router_index_list_.Count();
  for (int32 router_index = 0; router_index < router_index_list_count; ++router_index) {
    host_index = router_index_list_[router_index];
    linked_item = &tgt[host_index];

    if (linked_item->p2p_route_prev_link || linked_item->p2p_route_next_link) {
      continue;
    }

    const int32 connection_info_list_count = connection_info_list_.Count();
    int32 count = 0;
    for (i = 0; i < connection_info_list_count && count < unreliable_s2c_routed_multicast_max_count; ++i) {
      if (host_index == connection_info_list_[i].host_index0) {
        // HostIndex1을 뒤로 붙인다.
        next_index = connection_info_list_[i].host_index1;
        next_linked_item = &tgt[next_index];

        if (next_linked_item->p2p_route_prev_link ||next_linked_item->p2p_route_next_link) {
          continue;
        }

        linked_item->p2p_route_next_link = next_linked_item;
        next_linked_item->p2p_route_prev_link = linked_item;
        linked_item = linked_item->p2p_route_next_link;

        count++;
      } else if (host_index == connection_info_list_[i].host_index1) {
        // HostIndex0을 뒤로 붙인다.
        next_index = connection_info_list_[i].host_index0;
        next_linked_item = &tgt[next_index];

        if (next_linked_item->p2p_route_prev_link || next_linked_item->p2p_route_next_link) {
          continue;
        }

        linked_item->p2p_route_next_link = next_linked_item;
        next_linked_item->p2p_route_prev_link = linked_item;
        linked_item = linked_item->p2p_route_next_link;

        count++;
      }
    }
  }

  // 쓰고 남은거 정리
  connection_info_list_.Reset();
  router_index_list_.Reset();
}

void NetServerImpl::Convert_NOLOCK(SendDestInfoList_S& to, HostIdArray& from) {
  AssertIsLockedByCurrentThread();

  const int32 from_count = from.Count();
  to.Resize(from_count);
  for (int32 i = 0; i < from_count; ++i) {
    to[i].host_id = from[i];
    to[i].object = GetSendDestByHostId_NOLOCK(from[i]);
  }
}

String NetServerImpl::DumpGroupStatus() {
  //@todo 원래 내용이 없는데...??
  return String();
}

bool NetServerImpl::NextEncryptCount(HostId remote_id, CryptoCountType& out_count) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (remote_id == HostId_Server) {
    out_count = self_encrypt_count_;
    ++self_encrypt_count_;
    return true;
  } else if (auto rc = GetAuthedClientByHostId_NOLOCK(remote_id)) {
    out_count = rc->encrypt_count;
    ++rc->encrypt_count;
    return true;
  } else {
    fun_check(0);
    return false;
  }
}

void NetServerImpl::PrevEncryptCount(HostId remote_id) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (remote_id == HostId_Server) {
    --self_encrypt_count_;
  } else if (auto rc = GetAuthedClientByHostId_NOLOCK(remote_id)) {
    --rc->encrypt_count;
  } else {
    fun_check(0);
  }
}

bool NetServerImpl::GetExpectedDecryptCount(HostId remote_id, CryptoCountType& out_count) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (remote_id == HostId_Server) {
    out_count = self_decrypt_count_;
    return true;
  } else if (auto rc = GetAuthedClientByHostId_NOLOCK(remote_id)) {
    out_count = rc->decrypt_count;
    return true;
  } else {
    fun_check(0);
    return false;
  }
}

bool NetServerImpl::NextDecryptCount(HostId remote_id) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (remote_id == HostId_Server) { //@maxidea: todo: Hit 비중으로 보자면, RC조회가 더 많을듯 싶은데... 하지만, 이게 연산이 없으니 이게 먼저 오는게 나으려나..
    ++self_decrypt_count_;
    return true;
  } else if (auto rc = GetAuthedClientByHostId_NOLOCK(remote_id)) {
    ++rc->decrypt_count;
    return true;
  } else {
    fun_check(0);
    return false;
  }
}

bool NetServerImpl::IsConnectedClient(HostId client_id) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  return GetAuthedClientByHostId_NOLOCK(client_id); // 종료 되었거나, 종료 중일 경우에는 nullptr을 반환함.
}

bool NetServerImpl::EnableSpeedHackDetector(HostId client_id, bool enable) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (auto rc = GetAuthedClientByHostId_NOLOCK(client_id)) {
    if (enable) {
      if (!rc->SpeedHackDetector) {
        rc->speed_hack_detector_.Reset(new SpeedHackDetector);
        s2c_proxy_.NotifySpeedHackDetectorEnabled(client_id, GReliableSend_INTERNAL, true);
      }
    } else {
      if (rc->speed_hack_detector_) {
        rc->speed_hack_detector_.Reset();
        s2c_proxy_.NotifySpeedHackDetectorEnabled(client_id, GReliableSend_INTERNAL, false);
      }
    }

    return true;
  }

  return false;
}

void NetServerImpl::P2PGroup_CheckConsistency() {
}

void NetServerImpl::DestroyEmptyP2PGroups() {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  for (auto it = p2p_groups_.CreateIterator(); it; ++it) {
    auto group = it->value;

    if (group->members_.IsEmpty()) {
      const HostId group_id_to_destroy = group->group_id_;

      host_id_factory_->Drop(GetAbsoluteTime(), group_id_to_destroy);
      EnqueueP2PGroupRemoveEvent(group_id_to_destroy);

      it.RemoveCurrent();
    }
  }
}

void NetServerImpl::EnqueueP2PGroupRemoveEvent(HostId group_id) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (callbacks_) {
    LocalEvent event(LocalEventType::P2PGroupRemoved);
    event.remote_id = group_id;
    EnqueueLocalEvent(event);
  }
}

void NetServerImpl::ConditionalFallbackServerUdpToTcp(RemoteClient_S* rc, double absolute_time) {
  // [CaseCMN] 간혹 섭->클 UDP 핑은 되면서 반대로의 핑이 안되는 경우로 인해 UDP fallback이 계속 안되는 경우가 있는 듯.
  // 그러므로 서버에서도 클->섭 UDP 핑이 오래 안오면 fallback한다.

  //if ((absolute_time - rc->last_udp_packet_recv_time_) > default_timeout_sec) 이게 아니라
  if (rc->to_client_udp_fallbackable_.real_udp_enabled_ &&
    (absolute_time - rc->last_udp_packet_recv_time_) > NetConfig::GetFallbackServerUdpToTcpTimeout()) {
    // LocalProcessForFallbackUdpToTcp(rc); 굳이 이걸 호출할 필요는 없다.
    // 클라에서 다시 서버로 fallback 신호를 할 테니까.

    // 클라에게 TCP fallback 노티를 한다.
    s2c_proxy_.NotifyUdpToTcpFallbackByServer(rc->host_id_, GReliableSend_INTERNAL);
  }
}

// 홀 펀칭된 포트를 유지하기 위해 더미 패킷을 쏴줌..
void NetServerImpl::ConditionalArbitaryUdpTouch(RemoteClient_S* rc, double absolute_time) {
  AssertIsLockedByCurrentThread();

  fun_check(NetConfig::GetFallbackServerUdpToTcpTimeout() > NetConfig::cs_ping_interval_sec * 2.5);

  // 클라로부터 최근까지도 UDP 수신은 됐지만 정작 ping만 안오면 다른 형태의 pong 보내기.
  // 클라에서 서버로 대량으로 UDP를 쏘느라 정작 핑이 늑장하는 경우 필요하다.
  if ((absolute_time - rc->last_udp_ping_recv_time_) > NetConfig::cs_ping_interval_sec * 1.5 &&
      (absolute_time - rc->last_udp_packet_recv_time_) < NetConfig::cs_ping_interval_sec) {
    if ((absolute_time - rc->arbitrary_udp_touched_time_) > NetConfig::cs_ping_interval_sec) {
      rc->arbitrary_udp_touched_time_ = absolute_time;

      MessageOut msg_to_send;
      lf::Write(msg_to_send, MessageType::ArbitaryTouch);
      rc->to_client_udp_fallbackable_.SendWhenReady(rc->GetHostId(), SendFragRefs(msg_to_send), UdpSendOption(MessagePriority::Ring0, EngineOnlyFeature));
    }
  }
}

void NetServerImpl::SetMaxDirectP2PConnectionCount(HostId client_id, int32 max_count) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (auto rc = GetAuthedClientByHostId_NOLOCK(client_id)) {
    rc->max_direct_p2p_connection_count_ = MathBase::Max(max_count, 0);
  }
}


struct SuperPeerCandidate {
  RemoteClient_S* peer;
  P2PGroup_S* group;
  double joined_time;

  bool operator == (const SuperPeerCandidate& rhs) const {
    return GetSuperPeerRatingC(peer, group) == GetSuperPeerRatingC(rhs.peer, group);
  }

  bool operator < (const SuperPeerCandidate& rhs) const {
    return GetSuperPeerRatingC(peer, group) > GetSuperPeerRatingC(rhs.peer, group);
  }

 private:
  static double GetSuperPeerRatingC(const RemoteClient_S* rc, const P2PGroup_S* group) {
    if (group->ordered_super_peer_suitables.Count() > 0 && group->ordered_super_peer_suitables[0].host_id == rc->host_id_) {
      return rc->super_peer_rating_ + NetConfig::super_peer_selection_premium;
    }
    else {
      return rc->super_peer_rating_;
    }
  }
};

// 수퍼피어로서 역할을 가장 잘 수행할 놈을 고른다.
//
// 이미 지정된 경우 웬만하면 바꾸지 않는다.
void NetServerImpl::P2PGroup_RefreshMostSuperPeerSuitableClientId(P2PGroup_S* group) {
  AssertIsLockedByCurrentThread();

  const double absolute_time = GetAbsoluteTime();

  if (group->members_.IsEmpty() || !group->super_peer_selection_policy_is_valid) {
    group->ordered_super_peer_suitables.Clear();
  } else {
    // 판단 기준: real UDP가 쓰이고 있는지? 공유기 없이 사용중인 클라인지?
    for (auto& member_pair : group->members_) {
      auto rc0 = member_pair.value;

      //TODO 아래 코드에서 서버일 경우에는 오류가 발생할터인데...
      //코드를 살짝 수정해야할듯.. 멤버가 서버이면 건너뛰는것을 먼저 간단히 해주어야할듯...
      RemoteClient_S* rc = nullptr;
      try {
        rc = dynamic_cast<RemoteClient_S*>(rc0.ptr);
      } catch (std::bad_cast& e) {
        OutputDebugString((const char*)e.what());
        int32* X = nullptr; *X = 1;
      }

      // 그룹 멤버로서의 서버는 평가 대상에서 제외한다.
      if (rc == nullptr) {
        continue;
      }

      // 수퍼피어가 아닐 당시의 계산이 된 적이 있고 현 수퍼피어이면 계산을 하지 않는다.
      // 이유: 수퍼피어가 되면 트래픽이 확 늘어남=>평가 점수가 급락하게 됨=>다른 피어에 비해서 평가 점수가 크게 하락할 수 있음
      if (rc->super_peer_rating_ != 0 &&
          group->ordered_super_peer_suitables.Count() > 0 &&
          group->ordered_super_peer_suitables[0].host_id == rc->host_id_ &&
          rc->to_client_udp_fallbackable_.real_udp_enabled_) {
        continue;
      }

      rc->super_peer_rating_ = 0;

      // TCP밖에 못한다면 수퍼피어로서는 꽝이다.
      if (rc->to_client_udp_fallbackable_.real_udp_enabled_) {
        rc->super_peer_rating_ += group->super_peer_selection_policy.real_udp_weight; // 10000
      }

      // 리얼 IP이면 장땡이다!
      if (!rc->IsBehindNAT()) {
        rc->super_peer_rating_ += group->super_peer_selection_policy.no_nat_device_weight; // 500;
      }

      // 핑 만큼 감점 처리한다.
      // (1초 이상이면 심각한 수준이므로 200점 감점)
      rc->super_peer_rating_ -= rc->recent_ping_ * group->super_peer_selection_policy.server_lag_weight; // 200;

      // 사용자가 입력한 프레임 속도만큼 가산점 처리한다. 40~60 프레임 * 4 = 240 점 정도
      rc->super_peer_rating_ += rc->last_application_hint_.recent_frame_rate * group->super_peer_selection_policy.frame_rate_weight;

      // 피어 평균 핑만큼 감점 처리한다.
      // (제아무리 서버와의 핑이 좋아도 타 피어와의 핑이 나쁘면 심각한 수준인 경우도 있고 아닌 경우도 있다.
      // 디폴트는 피어 갯수당 0점. >0이면 JIT 홀펀칭이 무의미하므로.)
      rc->super_peer_rating_ -= MathBase::Max(0.0, rc->GetP2PGroupTotalRecentPing(group->group_id_)) * group->super_peer_selection_policy.peer_lag_weight;

      // 트래픽 속도만큼 가산점 처리 (VDSL 수준(100Mbps) 이면 매우 높은 수준이므로 400점 가산점)
      rc->super_peer_rating_ += group->super_peer_selection_policy.send_speed_weight* rc->send_speed * (1024*1024*10); // 400

      // P2P 그룹에 들어온지 얼마 안됐으면 왕창 감점한다.
      if (group->super_peer_selection_policy.exclude_newjoinee_duration_time > 0 &&
        (absolute_time - rc0.joined_time) < group->super_peer_selection_policy.exclude_newjoinee_duration_time) {
        rc->super_peer_rating_ = -9e+30;
      }
    }

    // sort
    int32 count = 0;

    Array<SuperPeerCandidate,InlineAllocator<256>> member_list(group->members_.Count());
    for (auto& member_pair : group->members_) {
      //@todo 여기서 캐스팅을 꼬옥 해야만 하는건지??
      RemoteClient_S* as_rc = nullptr;
      try {
        as_rc = dynamic_cast<RemoteClient_S*>(member_pair.value.ptr);
      } catch (std::bad_cast& e) {
        OutputDebugString((const char*)e.what());
        int32* X = nullptr; *X = 1;
      }

      if (as_rc) {
        auto p = &member_list[count++];
        p->peer = as_rc;
        p->group = group;
        p->joined_time = (absolute_time - member_pair.value.joined_time);
      }
    }

    member_list.Resize(count); // 일치하지 않을 수도 있으려나??
    member_list.Sort();

    group->ordered_super_peer_suitables.Resize(count); //TODO ResizeUninitialized로 해도 되지 않을까? 물론, 이경우에는 멤버가 모두 초기화 되었을 경우에 한해서...
    for (int32 i = 0; i < count; ++i) {
      auto peer =  member_list[i].peer;

      group->ordered_super_peer_suitables[i].host_id = peer->host_id_;
      group->ordered_super_peer_suitables[i].rating = peer->super_peer_rating_;
      group->ordered_super_peer_suitables[i].real_udp_enabled = peer->to_client_udp_fallbackable_.real_udp_enabled_;
      group->ordered_super_peer_suitables[i].behind_nat = peer->IsBehindNAT();
      group->ordered_super_peer_suitables[i].recent_ping = peer->recent_ping_;
      group->ordered_super_peer_suitables[i].p2p_group_total_recent_ping = peer->GetP2PGroupTotalRecentPing(group->group_id_);
      group->ordered_super_peer_suitables[i].send_speed = peer->send_speed_;
      group->ordered_super_peer_suitables[i].joined_time = member_list[i].joined_time;
      group->ordered_super_peer_suitables[i].frame_rate = peer->last_application_hint_.recent_frame_rate;
    }
  }
}

void NetServerImpl::P2PGroup_RefreshMostSuperPeerSuitableClientId(RemoteClient_S* rc) {
  AssertIsLockedByCurrentThread();

  // 연계된 모든 p2p group에 대해 refresh를 시행
  for (auto& pair : rc->joined_p2p_groups_) {
    auto group = pair.value.group_ptr;

    P2PGroup_RefreshMostSuperPeerSuitableClientId(group.Get());
  }
}

HostId NetServerImpl::GetMostSuitableSuperPeerInGroup(HostId group_id, const SuperPeerSelectionPolicy& policy, const Array<HostId>& excludees) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (auto group = GetP2PGroupByHostId_NOLOCK(group_id)) {
    TouchSuitableSuperPeerCalcRequest(group.Get(), policy);

    // 종전 얻은 결과를 리턴
    for (int32 i = 0; i < group->ordered_super_peer_suitables.Count(); ++i) {
      if (!excludees.Contains(group->ordered_super_peer_suitables[i].host_id)) {
        return group->ordered_super_peer_suitables[i].host_id;
      }
    }
  }

  return HostId_None;
}

HostId NetServerImpl::GetMostSuitableSuperPeerInGroup(HostId group_id, const SuperPeerSelectionPolicy& policy, HostId excludee) {
  Array<HostId> excludees;
  if (excludee != HostId_None) {
    excludees.Add(excludee);
  }

  return GetMostSuitableSuperPeerInGroup(group_id, policy, excludees);
}

void NetServerImpl::ElectSuperPeer() {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  for (auto& pair : p2p_groups_) {
    auto group = pair.value;

    P2PGroup_RefreshMostSuperPeerSuitableClientId(group.Get());
  }
}

void NetServerImpl::GetUdpSocketAddrList(Array<InetAddress>& output) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (udp_assign_mode != ServerUdpAssignMode::Static) {
    throw Exception("Cannot call GetUdpSocketAddrList unless ServerUdpAssignMode::Static is used.");
  }

  output.Clear(); // Just in case

  for (int32 i = 0; i < udp_sockets_.Count(); ++i) {
    auto udp_socket = udp_sockets_[i];

    output.Add(udp_socket->socket_->GetSockName());
  }
}

void NetServerImpl::OnSocketWarning(InternalSocket* socket, const String& text) {
  // 여기는 socket가 접근하므로, 메인락을 걸면 데드락 위험이 있다.
  //LogWriter의 포인터를 보호할 크리티컬 섹션을 따로 걸어야 하나??ㅠㅠ
  //CScopedLock2 main_guard(main_mutex_);

  //TODO: 별도로 락을 거는게 좋을듯...

  if (intra_logger_) {
    intra_logger_->WriteLine(LogCategory::System, *text);
  }
}

uint32 NetServerImpl::GetInternalVersion() {
  return internal_version_;
}

void NetServerImpl::ShowError_NOLOCK(SharedPtr<ResultInfo> result_info) {
  if (intra_logger_) {
    intra_logger_->WriteLine(LogCategory::System, *result_info->ToString());
  }

  NetCoreImpl::ShowError_NOLOCK(result_info);
}

void NetServerImpl::ShowWarning_NOLOCK(SharedPtr<ResultInfo> result_info) {
  if (intra_logger_) {
    const String text = String::Format("warning: %s", *result_info->ToString());
    intra_logger_->WriteLine(LogCategory::System, *text);
  }

  if (callbacks_) {
    callbacks_->OnWarning(result_info.Get());
  }
}

void NetServerImpl::SetCallbacks(INetServerCallbacks* callbacks) {
  //@warning 서버가 기동하기 전에만 설정가능함!!
  if (AsyncCallbackMayOccur()) {
    //TODO Exception 클래스를 특수화하는게 좋을듯함.
    //CAsyncCallbackOccurException() 같은...?  이름은 좀더 생각을 해봐야할듯..
    throw Exception("Already async callback may occur! server start or client connection should have not been done before here.");
  }

  AssertIsNotLockedByCurrentThread();

  callbacks_ = callbacks; //@note accept nullptr
}

void NetServerImpl::EnqueueError(SharedPtr<ResultInfo> result_info) {
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

void NetServerImpl::EnqueueWarning(SharedPtr<ResultInfo> result_info) {
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

void NetServerImpl::EnqueuePacketDefragWarning(const InetAddress& sender, const char* text) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  auto rc = GetRemoteClientByUdpEndPoint_NOLOCK(sender);
  EnqueueWarning(ResultInfo::From(ResultCode::InvalidPacketFormat, rc ? rc->host_id_ : HostId_None, text));
}

void NetServerImpl::EnqueueClientJoinApproveDetermine(const InetAddress& client_tcp_addr, const ByteArray& request) {
  AssertIsLockedByCurrentThread();
  //CScopedLock2 main_guard(main_mutex_);

  fun_check(client_tcp_addr.IsUnicast());

  if (callbacks_) {
    LocalEvent event(LocalEventType::ClientJoinCandidate);
    event.remote_addr = client_tcp_addr;
    event.connection_request = Request;
    EnqueueLocalEvent(event);
  }
}

void NetServerImpl::ProcessOnClientJoinRejected(RemoteClient_S* rc, const ByteArray& response) {
  AssertIsLockedByCurrentThread();
  //CScopedLock2 main_guard(main_mutex_);

  MessageOut msg_to_send;
  lf::Write(msg_to_send, MessageType::NotifyServerDeniedConnection);
  lf::Write(msg_to_send, response);

  CScopedLock2 rc_tcp_send_queue_guard(rc->to_client_tcp_->GetSendQueueMutex());
  rc->to_client_tcp_->SendWhenReady(SendFragRefs(msg_to_send), TcpSendOption());
}

void NetServerImpl::ProcessOnClientJoinApproved(RemoteClient_S* rc, const ByteArray& response) {
  AssertIsLockedByCurrentThread();

  if (rc == nullptr) {
    throw Exception("unexpected at candidate remote client removal.");
  }

  // promote unmature client to mature, give new host_id.
  const HostId NewHostId = host_id_factory_->Create(GetAbsoluteTime());
  rc->host_id_ = NewHostId;

  // Promote candidate -> authed
  candidate_remote_clients_.Remove(rc);
  authed_remote_clients_.Add(NewHostId, rc);

  // 접속 성공!
  // 클라이언트가 서버에 성공적으로 접속되자 마자, 아래의 메시지를 보내줌.
  {
    fun_check(rc->to_client_tcp_->remote_addr_.IsUnicast());

    MessageOut msg_to_send;
    lf::Write(msg_to_send, MessageType::NotifyServerConnectSuccess);
    lf::Write(msg_to_send, NewHostId);
    lf::Write(msg_to_send, instance_tag_);
    lf::Write(msg_to_send, response);
    lf::Write(msg_to_send, NamedInetAddress(rc->to_client_tcp_->remote_addr_));
    {
      CScopedLock2 rc_tcp_send_queue_guard(rc->to_client_tcp_->GetSendQueueMutex());
      rc->to_client_tcp_->SendWhenReady(SendFragRefs(msg_to_send), TcpSendOption());
    }
  }

  //// create magic number
  //rc->to_client_udp_fallbackable_.holepunch_tag_ = Uuid::NewUuid();

  //// send magic number and any UDP address via TCP
  //{
  //  MessageOut msg_to_send;
  //  lf::Write(msg_to_send, MessageType::RequestStartServerHolepunch);
  //  lf::Write(msg_to_send, rc->to_client_udp_fallbackable_.holepunch_tag_);
  //  rc->to_client_tcp_->SendWhenReady(SendFragRefs(msg_to_send), TcpSendOption());
  //}

  // enqueue new client event.
  LocalEvent event(LocalEventType::ClientJoinApproved);
  event.client_info = rc->GetClientInfo();
  event.remote_id = rc->host_id_;
  rc->EnqueueLocalEvent(event);

  //TODO
  //if (viz_agent_) {
  //  CScopedLock2 viz_agent_guard(viz_agent_->main_mutex_);
  //  viz_agent_->C2SProxy.NotifySrv_Clients_AddOrEdit(HostId_Server, GReliableSend_INTERNAL, rc->host_id_);
  //}

  // stats
  //CCounters::IncreaseBy("NetServer", "CCU", 1);
}

// 클라이언트 외부주소로 후보(인증대기) 클라이언트를 찾습니다.
RemoteClient_S* NetServerImpl::GetCandidateRemoteClientByTcpAddr(const InetAddress& client_addr) {
  AssertIsLockedByCurrentThread();

  return candidate_remote_clients_.FindRef(client_addr);
}

bool NetServerImpl::AsyncCallbackMayOccur() {
  return listener_.IsValid();
}

// 로컬 이벤트 하나를 큐잉합니다.  큐잉된 이벤트는 유저 스레드풀에서 실행됩니다.
void NetServerImpl::EnqueueLocalEvent(LocalEvent& event) {
  AssertIsLockedByCurrentThread();
  //CScopedLock2 main_guard(main_mutex_);

  if (listener_) { // 종료중이 아닌 경우에만, 추가합니다.
    final_user_work_queue_.Enqueue(event);
    user_task_queue_.AddTaskSubject(this);
  }
}

// 로컬 이벤트 하나를 처리합니다.
void NetServerImpl::ProcessOneLocalEvent(LocalEvent& event) {
  AssertIsNotLockedByCurrentThread();

  if (callbacks_/* && !tear_down_*/) {
    try {
      switch (event.type) {
      case LocalEventType::ClientLeaveAfterDispose:
        callbacks_->OnClientLeft(event.client_info.Get(), event.result_info.Get(), event.comment);
        break;

      case LocalEventType::ClientJoinApproved:
        callbacks_->OnClientJoined(event.client_info.Get());
        break;

      case LocalEventType::P2PAddMemberAckComplete:
        callbacks_->OnP2PGroupJoinMemberAckComplete(event.group_id, event.member_id, event.result_info->result_code);
        break;

      case LocalEventType::HackSuspected:
        callbacks_->OnClientHackSuspected(event.remote_id, event.hack_type);
        break;

      case LocalEventType::P2PGroupRemoved:
        callbacks_->OnP2PGroupRemoved(event.remote_id);
        break;

      case LocalEventType::TcpListenFail:
        ShowError_NOLOCK(ResultInfo::From(ResultCode::ServerPortListenFailure, HostId_Server));
        break;

      //case LocalEventType::UnitTestFail:
      //  ShowError_NOLOCK(ResultInfo::From(ResultCode::UnitTestFailed, HostId_Server, event.result_info->comment));
      //  break;

      case LocalEventType::error:
        ShowError_NOLOCK(ResultInfo::From(event.result_info->result_code, event.remote_id, event.result_info->comment));
        break;

      case LocalEventType::Warning:
        ShowWarning_NOLOCK(ResultInfo::From(event.result_info->result_code, event.remote_id, event.result_info->comment));
        break;

      case LocalEventType::ClientJoinCandidate: {
          ByteArray response;
          const bool approved = callbacks_->OnConnectionRequest(event.remote_addr, event.connection_request, response);

          CScopedLock2 main_guard(main_mutex_);
          CheckCriticalSectionDeadLock(__FUNCTION__);

          if (auto rc = GetCandidateRemoteClientByTcpAddr(event.remote_addr)) {
            if (approved) {
              ProcessOnClientJoinApproved(rc, response);
            } else {
              ProcessOnClientJoinRejected(rc, response);
            }
          }
        }
      }
    } catch (Exception& e) {
      if (callbacks_/* && !tear_down_*/) {
        callbacks_->OnException(event.remote_id, e);
      }
    } catch (std::exception& e) {
      if (callbacks_/* && !tear_down_*/) {
        callbacks_->OnException(event.remote_id, Exception(e));
      }
    } //catch (_com_error& e) {
    //  if (callbacks_ /*&& !tear_down_*/) {
    //    callbacks_->OnException(event.remote_id, Exception(e));
    //  }
    //} catch (void* e) {
    //  if (callbacks_ /*&& !tear_down_*/) {
    //    callbacks_->OnException(event.remote_id, Exception(e));
    //  }
    //}
#ifdef ALLOW_CATCH_UNHANDLED_EVEN_FOR_USER_ROUTINE // 사용자가 잡아내는게 더 바람직하지 않을런지??
    catch (...) {
      if (callbacks_/* && !tear_down_*/) {
        Exception e;
        e.exception_type = ExceptionType::Unhandled;
        callbacks_->OnException(event.remote_id, e);
      }
    }
#endif
  }
}

//String NetServerImpl::GetConfigString() {
//  return String::Format("listen=%s", *local_nic_addr_);
//}

// 다룰수 있는 최대 메시지 길이를 반환합니다.
int32 NetServerImpl::GetMessageMaxLength() {
  return settings_.message_max_length;
}

//void NetServerImpl::ConditionalPruneTooOldDefragBoard() {
//  CScopedLock2 main_guard(main_mutex_);
//
//  if ((GetAbsoluteTime() - last_prune_too_old_defragger_time_) > (NetConfig::assemble_fragged_packet_timeout_sec / 2)) {
//    last_prune_too_old_defragger_time_ = GetAbsoluteTime();
//
//    packet_defragger_->PruneTooOldDefragBoard();
//  }
//}

void NetServerImpl::ConditionalLogFreqFail() {
  //CScopedLock2 main_guard(main_mutex_);
  //이거 쓰기싫어 FreqFailNeed를 만듬.

  if (free_fail_need_ && (GetAbsoluteTime() - last_holepunch_freq_fail_logged_time_) > 30) {
    LogFreqFailNow();
  }
}

void NetServerImpl::LogFreqFailNow() {
  if (!freq_fail_log_most_rank_text_.IsEmpty()) {
    ErrorReporter::Report(freq_fail_log_most_rank_text_);
  }

  last_holepunch_freq_fail_logged_time_ = GetAbsoluteTime();
  freq_fail_log_most_rank_ = 0;
  freq_fail_log_most_rank_text_ = "";

  free_fail_need_ = false;
}

HostId NetServerImpl::GetLocalHostId() {
  return HostId_Server;
}

HostId NetServerImpl::GetSrcHostIdByAddrAtDestSide_NOLOCK(const InetAddress& address) {
  AssertIsLockedByCurrentThread();

  //Any가 아니고, unicast가 아닌 경우에는 HostId_None을 반환.
  if (!address.IsAny() && !address.IsUnicast()) { //TODO wildcard도 인정???
    return HostId_None;
  }

  if (auto rc = GetRemoteClientByUdpEndPoint_NOLOCK(address)) {
    return rc->host_id_;
  }

  // for loopback
  if (local_addr_to_udp_socket_map_.Contains(address)) {
    return HostId_Server;
  }

  return HostId_None;
}

void NetServerImpl::RequestAutoPrune(RemoteClient_S* rc) {
  // 클라이언트 추방시 소켓을 바로 닫으면 직전에 보낸 RPC가 안갈 수 있다.
  // 따라서 클라에게 자진 탈퇴를 종용하되 시한을 둔다.
  // 주의 : 중복해서 요청할 수 없도록 함.
  if (rc->request_auto_prune_start_time_ == 0) {
    rc->request_auto_prune_start_time_ = GetAbsoluteTime();

    s2c_proxy_.RequestAutoPrune(rc->host_id_, GReliableSend_INTERNAL);
  }
}

// 오래 방치된 클라이언트 연결을 제거합니다.
void NetServerImpl::HardDisconnect_AutoPruneGoesTooLongClient(RemoteClient_S* rc) {
  // 5초로 잡아야 서버가 클라에게 많이 보내고 있던 중에도 직전 RPC의 확실한 송신이 되므로
  // 시간으로 하는건 좀 아니지 싶은데.. 어쩔 수 없으려나??
  if (rc->request_auto_prune_start_time_ > 0 &&
      (GetAbsoluteTime() - rc->request_auto_prune_start_time_) > 5.0) { //TODO 5.0 이값도 NetConfig로 빼주는게 좋을듯...
    rc->request_auto_prune_start_time_ = 0;
    IssueDisposeRemoteClient(rc, ResultCode::DisconnectFromLocal, ResultCode::ConnectServerTimeout, ByteArray(), __FUNCTION__, SocketErrorCode::Ok);
  }
}

void NetServerImpl::TEST_SetOverSendSuspectingThresholdInBytes(int32 threshold) {
  settings_.over_send_suspecting_threshold_in_byte = MathBase::Clamp(threshold, 0, NetConfig::default_over_send_suspecting_threshold_in_byte);
}

void NetServerImpl::TEST_SetTestping(HostId HostId_A, HostId HostId_B, double ping) {
  if (auto state = p2p_connection_pair_list_.GetPair(HostId_A, HostId_B)) {
    state->recent_ping_ = ping;
  }
}

bool NetServerImpl::IsNagleAlgorithmEnabled() {
  return settings_.bEnableNagleAlgorithm;
}

bool NetServerImpl::IsValidHostId_NOLOCK(HostId host_id) {
  AssertIsLockedByCurrentThread();
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (host_id == HostId_Server) {
    return true;
  } else if (GetRemoteClientByHostId_NOLOCK(host_id)) {
    return true;
  } else if (GetP2PGroupByHostId_NOLOCK(host_id)) {
    return true;
  } else {
    // Unknown(not server/client/group)
    return false;
  }
}

void NetServerImpl::RefreshSendQueuedAmountStat(RemoteClient_S* rc) {
  AssertIsLockedByCurrentThread();

  // 송신큐 잔여 총량을 산출한다.
  int32 tcp_queued_amount = 0;
  int32 udp_queued_amount = 0;
  {
    CScopedLock2 rc_tcp_send_queue_guard(rc->to_client_tcp_->GetSendQueueMutex());
    tcp_queued_amount = rc->to_client_tcp_->send_queue_.GetLength();
  }

  if (rc->to_client_udp_fallbackable_.udp_socket && rc->to_client_udp_fallbackable_.real_udp_enabled_) {
    CScopedLock2 rc_udp_fragger_guard(rc->to_client_udp_fallbackable_.udp_socket_->GetFraggerMutex());
    udp_queued_amount = rc->to_client_udp_fallbackable_.udp_socket_->packet_fragger_->FromTotalPacketInByteByAddr(rc->to_client_udp_fallbackable_.GetUdpAddrFromHere());
  }

  rc->send_queued_amount_in_byte_ = tcp_queued_amount + udp_queued_amount;

  // SendQueue의 용량을 검사한다.
  const double absolute_time = GetAbsoluteTime();

  if (rc->send_queue_warning_start_time_ != 0) {
    if (rc->send_queued_amount_in_byte_ > NetConfig::send_queue_heavy_warning_capacity) {
      if ((absolute_time - rc->send_queue_warning_start_time_) > NetConfig::send_queue_heavy_warning_time_sec) {
        EnqueueWarning(ResultInfo::From(ResultCode::SendQueueIsHeavy, rc->GetHostId(), String::Format("send_queue_ %dBytes", rc->send_queued_amount_in_byte_)));
        rc->send_queue_warning_start_time_ = absolute_time;
      }
    } else {
      rc->send_queue_warning_start_time_ = 0;
    }
  } else if (rc->send_queued_amount_in_byte_ > NetConfig::send_queue_heavy_warning_capacity) {
    rc->send_queue_warning_start_time_ = absolute_time;
  }
}

void NetServerImpl::AllowEmptyP2PGroup(bool allow) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  //@warning 서버가 이미 동작중일 경우에는 설정을 변경할 수 없다. 설정할 수 있게 한다고 하여도, 혼란만 가중시킬 것이다.
  if (AsyncCallbackMayOccur()) //@todo 이름이 좀?? {
    throw Exception("cannot set AllowEmptyP2PGroup after the server has started.");
  }

  if (!allow && start_create_p2p_group_ && dynamic_cast<AssignHostIdFactory*>(host_id_factory.Get()) == nullptr) {
    throw Exception("cannot set false after create P2PGroup when started.");
  }

  empty_p2p_group_allowed_ = allow;
}

//@todo Rating을 Score로 바꾸도록 하자???
int32 NetServerImpl::GetSuitableSuperPeerRankListInGroup(
      HostId group_id,
      super_peer_rating_* out_ratings,
      int32 out_ratings_buffer_count,
      const SuperPeerSelectionPolicy& policy,
      const Array<HostId>& excludees) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (auto group = GetP2PGroupByHostId_NOLOCK(group_id)) {
    TouchSuitableSuperPeerCalcRequest(group.Get(), policy);

    int32 out_rating_count = 0;

    // 종전 얻은 결과를 리턴. 단, 예외는 제끼고.
    for (int32 i = 0; i < group->ordered_super_peer_suitables.Count(); ++i) {
      if (out_rating_count < out_ratings_buffer_count &&
          excludees.Contains(group->ordered_super_peer_suitables[i].host_id) == false) {
        out_ratings[out_rating_count++] = group->ordered_super_peer_suitables[i];
      }
    }

    return out_rating_count;
  }

  return 0;
}

void NetServerImpl::TouchSuitableSuperPeerCalcRequest(P2PGroup_S* group, const SuperPeerSelectionPolicy& policy) {
  const double absolute_time = GetAbsoluteTime();

  // 클라들에게 통신량 측정을 지시. 필요시에만.
  //@maxidea: dynamic_casting을 꼭 해야하남??
  //TODO 멤버에 서버가 있으면 오류가 발생할듯 싶은데...???
  for (auto& member_pair : group->members_) {
    RemoteClient_S* rc = nullptr;
    try {
      rc = dynamic_cast<RemoteClient_S*>(member_pair.value.ptr);
    } catch (std::bad_cast& e) {
      OutputDebugString((const char*)e.what());
      int32* X = nullptr; *X = 1;
    }

    if (rc) {
      if (rc->last_request_measure_send_speed_time_ == 0 ||
          (absolute_time - rc->last_request_measure_send_speed_time_ > NetConfig::measure_client_send_speed_interval_sec)) {
        s2c_proxy_.RequestMeasureSendSpeed(rc->host_id_, GReliableSend_INTERNAL, policy.send_speed_weight > 0);
        rc->last_request_measure_send_speed_time_ = absolute_time;
      }
    }
  }

  // 새 Policy를 갱신
  group->super_peer_selection_policy = policy;
  group->super_peer_selection_policy_is_valid = true;

  // 일단 부정확하더라도 수퍼피어 목록은 넣어야 최초 콜 결과가 HostId_None이 리턴되는 불상사는 없다.
  if (group->ordered_super_peer_suitables.Count() == 0 && group->members_.Count() > 0) {
    P2PGroup_RefreshMostSuperPeerSuitableClientId(group);
  }
}

void NetServerImpl::EnableVizAgent(const char* viz_server_ip, int32 viz_server_port, const String& login_key) {
  //TODO
  //if (!viz_agent) {
  //  viz_agent_.Reset(new VizAgent(this, viz_server_ip, viz_server_port, login_key));
  //}
}

void NetServerImpl::Viz_NotifySendByProxy(const HostId* rpc_sendto_list,
                                          int32 rpc_sendto_count,
                                          const MessageSummary& summary,
                                          const RpcCallOption& rpc_call_opt) {
  //TODO
  //if (viz_agent_) { // only one
  //  Array<HostId> target;
  //  target.Resize(rpc_sendto_count);
  //  UnsafeMemory::Memcpy(target.GetData(), rpc_sendto_list, sizeof(HostId) * rpc_sendto_count);
  //
  //  viz_agent_->C2SProxy.NotifyCommon_SendRpc(HostId_Server, GReliableSend_INTERNAL, target, summary);
  //}
}

void NetServerImpl::Viz_NotifyRecvToStub( HostId rpc_recvfrom,
                                          RpcId rpc_id,
                                          const char* rpc_name,
                                          const char* params_as_string) {
  //TODO
  //if (viz_agent_) {
  //  viz_agent_->C2SProxy.NotifyCommon_ReceiveRpc(HostId_Server, GReliableSend_INTERNAL, rpc_recvfrom, rpc_name, rpc_id);
  //}
}

bool NetServerImpl::RemoteClient_New_ToClientUdpSocket(RemoteClient_S* rc) {
  AssertIsLockedByCurrentThread();

  if (rc->to_client_udp_fallbackable_.udp_socket) {
    return true;
  }

  // 한번 소켓생성 실패한  udp는 생성하지 않는다.
  //@note 다시 해봐야 될리가 없으려나??
  if (rc->to_client_udp_fallbackable_.client_udp_socket_create_failed_) {
    return false;
  }

  UdpSocketPtr_S assigned_udp_socket;
  int32 borrowed_port_number = 0;

  if (udp_assign_mode == ServerUdpAssignMode::PerClient) {
    assigned_udp_socket.Reset(new UdpSocket_S(this));

    int32 port_num;
    if (!free_udp_ports_.IsEmpty()) {
      port_num = free_udp_ports_.CutLast();
    } else {
      port_num = 0; // any를 의미
    }

    bool bound;
    if (!local_nic_addr_.IsEmpty()) {
      bound = assigned_udp_socket->socket_->Bind(*local_nic_addr_, port_num);
    } else {
      bound = assigned_udp_socket->socket_->Bind(port_num);
    }

    //@todo 인코딩 이슈인가? 주석이 깨졌다.. 차후에 찾아 수정하자!
    // ?ъ슜?먭? ?곸뿬?볦? ?ы듃 踰덊샇瑜??깃났?곸쑝濡?媛?몄솕??寃쎌슦?먮쭔
    if (bound && port_num > 0) {
      borrowed_port_number = port_num;
    }

    if (!bound && port_num > 0) {
      // 재시도: 아무 UDP 포트를 만들어 배정한다. 단 경고를 낸다.
      EnqueueWarning(ResultInfo::From(ResultCode::AlreadyExists, rc->host_id_, "All UDP port numbers specified at server start are deleted.  An arbitrary port number is assigned instead."));

      free_udp_ports_.Add(port_num);

      port_num = 0;

      if (!local_nic_addr_.IsEmpty()) {
        bound = assigned_udp_socket->socket_->Bind(*local_nic_addr_, 0);
      } else {
        bound = assigned_udp_socket->socket_->Bind(0);
      }
    }

    if (!bound) {
      // UDP 소켓 생성 실패다. 에러를 내자. 그리고 이 클라의 접속을 거부한다.
      rc->to_client_udp_fallbackable_.client_udp_socket_create_failed_ = true;
      return false;
    }

    // socket local 주소를 얻고 그것에 대한 매핑 정보를 설정한다.
    assigned_udp_socket->cached_local_addr_ = assigned_udp_socket->socket_->GetSockName();
    fun_check(assigned_udp_socket->cached_local_addr_.GetPort() > 0);
    local_addr_to_udp_socket_map_.Add(assigned_udp_socket->cached_local_addr_, assigned_udp_socket);

    assigned_udp_socket->socket_->SetCompletionContext((ICompletionContext*)assigned_udp_socket.Get());
    net_thread_pool_->AssociateSocket(assigned_udp_socket->socket_.Get());

    // overlapped send를 하므로 송신 버퍼는 불필요하다.
    // socket의 send buffer를 없앤다. CSocketBuffer가 non swappable이므로 안전하다.
    // send buffer 없애기는 coalsecse, throttling을 위해 필수다.
    // recv buffer 제거는 백해무익이므로 즐

#ifdef ALLOW_ZERO_COPY_SEND // zero copy send는 빠르지만 너무 많은 nonpaged를 유발 위험. 따라서 이걸로 막자. 막으니 성능도 더 나은데?
    assigned_udp_socket->socket_->SetSendBufferSize(0);
#endif

    NetUtil::SetUdpDefaultBehavior(assigned_udp_socket->socket_.Get());

    // 첫번째 issue recv는 netio thread에서 할 것이다.
  } else {
    // 고정된 UDP 소켓을 공용한다.
    assigned_udp_socket = GetAnyUdpSocket();
  }

  rc->to_client_udp_fallbackable_.udp_socket_ = assigned_udp_socket;

  if (udp_assign_mode == ServerUdpAssignMode::PerClient) {
    rc->owned_udp_socket_ = assigned_udp_socket;
    rc->borrowed_port_number_ = borrowed_port_number;

    ScopedUseCounter counter(*(rc->owned_udp_socket_));
    CScopedLock2 udp_socket_guard(rc->owned_udp_socket_->GetMutex());
    // Per rc UDP이면 첫 수신 이슈를 건다.
    rc->owned_udp_socket_->ConditionalIssueRecvFrom(); // first issue
  }

  /*
  Windows 2003의 경우는 아래와 같은지 모르겠으나, Windows 2016 Server에서는 Tcp 접속시에 알아낸 클라이언트 주소로
  UDP Dummy 패킷을 보내면 UDP 소켓이 차단되어, Receive는 되나 Send가 안된다.
  즉, Send쪽이 차단 되고 있다. (wireshark를 통해서 알아냄)

  포트를 바꿔서 보내보는건 어떨가 싶다.

  UdpSendOption opt;
  opt.bounce = false;
  opt.priority = MessagePriority::Ring0;
  opt.unique_id = 0;
  if (!using_over_block_icmp_environment_) {
    opt.ttl = 0; // NRM1 공유기 등은 알수없는 인바운드 패킷이 오면 멀웨어로 감지해 버린다. 따라서 거기까지 도착할 필요는 없다.
  }

  // Windows 2003의 경우, (포트와 상관없이) 서버쪽에서 클라에게 UDP패킷을 한번 쏴줘야 해당 클라로부터 오는 패킷을 블러킹하지 않는다.
  MessageOut ignorant;
  lf::Write(ignorant, MessageType::Ignore); // Dummy packet!!

  ScopedUseCounter counter(*(rc->to_client_udp_fallbackable_.udp_socket_));
  rc->to_client_udp_fallbackable_.udp_socket_->SendWhenReady(
      rc->GetHostId(),
      FilterTag::Make(HostId_Server, HostId_None),
      rc->to_client_tcp_->remote_addr_,
      ignorant,
      opt);
  */

  /*
  확인해 본바, TCP Accept시에 받은 주소로 UDP 메시지를 보내면 Send쪽이 먹통이 되는 현상이 있었다.
  이것 때문에 서버 홀펀칭이 안되는 문제가 발생한다.
  포트를 변경해서 해보자.
  그런데, 예측한 포트가 UDP가 아닌 TCP 프로토콜 형태로 점유하고 있다면 이 또한 Send쪽이 블럭이
  되지 않을까 싶다.

  확인해 본바, 아래처럼 해도 UDP Outbound쪽이 먹통이 된다.
  구지 이렇게 보내줄 이유가 없어 보인다.

  Windows 2003서버에 한해서만 처리해주는것도 한방법일 수도...

  int32 port = rc->to_client_tcp_->remote_addr_.GetPort() + 1;
  if (port < 1023 || port > 65534) {
    port = 1023;
  }

  InetAddress send_to(rc->to_client_tcp_->remote_addr_.BinaryAddress, port);

  MessageOut ignorant;
  lf::Write(ignorant, MessageType::Ignore); // Dummy packet!!

  ScopedUseCounter counter(*(rc->to_client_udp_fallbackable_.udp_socket_));
  rc->to_client_udp_fallbackable_.udp_socket_->SendWhenReady(
      rc->GetHostId(),
      FilterTag::Make(HostId_Server, HostId_None),
      send_to,
      ignorant,
      Opt);


  if (intra_logger_) {
    const String text = String::Format("Send dummy UDP message to %s(avoiding server blocking)", *send_to.ToString());
    intra_logger_->WriteLine(LogCategory::SP2P, *text);
  }
  */

  return true;
}

void NetServerImpl::GetUserWorkerThreadInfo(Array<ThreadInfo>& output) {
  if (user_thread_pool_) {
    user_thread_pool_->GetThreadInfos(output);
  }
}

void NetServerImpl::GetNetWorkerThreadInfo(Array<ThreadInfo>& output) {
  if (net_thread_pool_) {
    net_thread_pool_->GetThreadInfos(output);
  }
}

void NetServerImpl::RemoteClient_RequestStartServerHolepunch_OnFirst(RemoteClient_S* rc) {
  AssertIsLockedByCurrentThread();

  if (rc->to_client_udp_fallbackable_.holepunch_tag_ == Uuid::None) {
    rc->to_client_udp_fallbackable_.holepunch_tag_ = Uuid::NewRandomUuid();

    MessageOut msg_to_send;
    lf::Write(msg_to_send, MessageType::RequestStartServerHolepunch);
    lf::Write(msg_to_send, rc->to_client_udp_fallbackable_.holepunch_tag_);

    {
      CScopedLock2 rc_tcp_send_queue_guard(rc->to_client_tcp_->GetSendQueueMutex());
      rc->to_client_tcp_->SendWhenReady(SendFragRefs(msg_to_send), TcpSendOption());
    }
  }
}

void NetServerImpl::RemoteClient_NewLocalUdpSocketAndRequestNewRemoteUdpSocket(RemoteClient_S* rc) {
  if (rc->to_client_udp_fallbackable_.udp_socket_.IsValid() == false &&
      rc->to_client_udp_fallbackable_.client_udp_ready_waiting_ == false &&
      settings_.FallbackMethod == FallbackMethod::None) {
    if (RemoteClient_New_ToClientUdpSocket(rc)) {
      // 보내기전 만들어서 보낸다.
      const auto addr = rc->to_client_udp_fallbackable_.udp_socket_->GetRemoteIdentifiableLocalAddr(rc);
      s2c_proxy_.S2C_RequestCreateUdpSocket(rc->host_id_, GReliableSend_INTERNAL, addr);

      rc->to_client_udp_fallbackable_.client_udp_ready_waiting_ = true;
    }
  }
}

//FIXME ClientLeave시에 GetClientCount()가 차감이 안되는 현상이 있음.
//authed_remote_clients_.Count() 값이... 흠..
//fun_check 현재 이 함수가 사용되지 않고 있음.
void NetServerImpl::RemoteClient_RemoveFromCollections(RemoteClient_S* rc) {
  candidate_remote_clients_.Remove(rc);
  authed_remote_clients_.Remove(rc->host_id_);
  host_id_factory_->Drop(GetAbsoluteTime(), rc->host_id_);
  udp_addr_to_remote_client_index_.Remove(rc->to_client_udp_fallbackable_.udp_addr_from_here_);
}

void NetServerImpl::EnqueueP2PAddMemberAckCompleteEvent(HostId group_id, HostId added_member_id, ResultCode result) {
  LocalEvent event(LocalEventType::P2PAddMemberAckComplete);
  event.group_id = group_id;
  event.member_id = added_member_id;
  event.remote_id = added_member_id;
  event.result_info = ResultInfo::From(result);
  EnqueueLocalEvent(event);
}

// MemberHostId이 들어있는 AddMemberAckWaiter 객체를 목록에서 찾아서 모두 제거한다.
void NetServerImpl::AddMemberAckWaiters_RemoveRelated_MayTriggerJoinP2PMemberCompleteEvent(P2PGroup_S* group, HostId member_id, ResultCode reason) {
  Array<int32> deletee_list;
  Set<HostId> joining_member_deletee_list;

  // index i 를 통해서 group->add_member_ack_waiters.RemoveSwap 제거를 하는 동작을 하므로,
  // 아래와 같은 형태로 루프를 돌게 되면 문제가 생김.
  //for (int32 i = 0; i < group->add_member_ack_waiters_.Count(); ++i)

  for (int32 i = group->add_member_ack_waiters_.Count() - 1; i >= 0; --i) {
    const auto& ack_waiter = group->add_member_ack_waiters_[i];

    if (ack_waiter.joining_member_host_id == member_id || ack_waiter.old_member_host_id == member_id) {
      deletee_list.Add(i);

      // 제거된 항목이 추가완료대기를 기다리는 피어에 대한 것일테니
      // 추가완료대기중인 신규진입피어 목록만 따로 모아둔다.
      joining_member_deletee_list.Add(ack_waiter.joining_member_host_id);
    }
  }

  // sweep
  for (int32 i = 0; i < deletee_list.Count(); ++i) {
    group->add_member_ack_waiters_.RemoveAtSwap(deletee_list[i]);
  }

  // MemberHostId에 대한 OnP2PGroupJoinMemberAckComplete 대기에 대한 콜백에 대한 정리.
  // 중도 실패되어도 OnP2PGroupJoinMemberAckComplete 콜백을 되게 해주어야 하니까.
  for (auto joining_member_id : joining_member_deletee_list) {
    if (!group->add_member_ack_waiters_.AckWaitingItemExists(joining_member_id)) {
      EnqueueP2PAddMemberAckCompleteEvent(group->group_id_, joining_member_id, reason);
    }
  }
}

bool NetServerImpl::IsDisposeRemoteClient_NOLOCK(RemoteClient_S* rc) {
  return dispose_issued_remote_clients_map_.Contains(rc);
}

bool NetServerImpl::SetHostTag(HostId host_id, void* host_tag) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (auto subject = GetTaskSubjectByHostId_NOLOCK(host_id)) {
    subject->host_tag_ = host_tag;
    return true;
  }

  return false;
}

P2PConnectionStatePtr P2PPairList::GetPair(HostId a, HostId b) {
  //TODO 이게 중요할까??
  if (a > b) {
    Swap(a, b);
  }

  RCPair pair(a, b);
  return active_pairs.FindRef(pair);
}

void P2PPairList::RemovePairOfAnySide(RemoteClient_S* rc) {
  for (auto it = active_pairs.CreateIterator(); it; ++it) {
#if 1 // HostId가 미지정되면(그럴리는 없겠지만) 어쩔라구
    auto state = it->value;

    if (state->first_client == rc || state->second_client == rc) {
      state->first_client->p2p_connection_pairs_.Remove(state);
      state->second_client->p2p_connection_pairs_.Remove(state);

      it.RemoveCurrent();
    }
#else
    const RCPair& p = it->key;

    if (P.First == rc->host_id_ || p.Second == rc->host_id_) {
      auto state = it->value;

      state->first_client->p2p_connection_pairs_.Remove(state);
      state->second_client->p2p_connection_pairs_.Remove(state);

      it.RemoveCurrent();
    }
#endif
  }

  // 완전히 서버를 떠나는 상황이므로,
  // 재사용 리스트에서도 삭제토록 하자.
  for (auto it = recyclable_pairs.CreateIterator(); it; ++it) {
    auto state = it->value;

    if (state->first_client == rc || state->second_client == rc) {
      // 아마도 이미 다 해제된 후로 들어가니까 안해도 될거다.
      //state->first_client->p2p_connection_pairs_.Remove(state);
      //state->second_client->p2p_connection_pairs_.Remove(state);

      it.RemoveCurrent();
    }
  }
}

void P2PPairList::AddPair(RemoteClient_S* rc_a, RemoteClient_S* rc_b, P2PConnectionStatePtr state) {
  HostId a = rc_a->host_id_;
  HostId b = rc_b->host_id_;
  // 서버, 잘못된 아이디, 서로 같은 아이디는 추가 불가능. P2P 직빵연결된 것들의 인덱스니까.
  //fun_check(a != b);  loopback도 Pair에 들어갈 수 있으므로 이건 체크하지 말자.
  fun_check(a != HostId_Server);
  fun_check(a != HostId_None);
  fun_check(b != HostId_Server);
  fun_check(b != HostId_None);

  //@todo 대소가 중요한가??
  if (a > b) {
    Swap(a, b);
    Swap(rc_a, rc_b);
  }

  RCPair pair(a, b);

  state->first_client = rc_a;
  state->second_client = rc_b;

  active_pairs.Add(pair, state);
}

void P2PPairList::ReleasePair(NetServerImpl* owner, RemoteClient_S* rc_a, RemoteClient_S* rc_b) {
  RCPair pair_key(rc_a->host_id_, rc_b->host_id_);

  if (pair_key.First > pair_key.Second) {
    Swap(pair_key.First, pair_key.Second);
  }

  if (auto pair = active_pairs.FindRef(pair_key)) {
    if (--pair->dup_count == 0) {
      pair->first_client->p2p_connection_pairs_.Remove(pair);
      pair->second_client->p2p_connection_pairs_.Remove(pair);

      if (pair->CanRecycle()) {
        pair->release_time_ = owner_->GetAbsoluteTime();
        pair->SetRelayed(true);

        AddRecyclePair(pair->first_client, pair->second_client, pair);
      }

      active_pairs.Remove(pair_key);

      if (owner_->intra_logger_) {
        const String text = String::Format("Remove pair of connections between client %d and %d.", (int32)rc_a->host_id_, (int32)rc_b->host_id_);
        owner_->intra_logger_->WriteLine(LogCategory::PP2P, *text);
      }
    }
  }
}

void P2PPairList::AddRecyclePair(RemoteClient_S* rc_a, RemoteClient_S* rc_b, P2PConnectionStatePtr state) {
  HostId a = rc_a->host_id_;
  HostId b = rc_b->host_id_;

  // 서버, 잘못된 아이디, 서로 같은 아이디는 추가 불가능. P2P 직빵연결된 것들의 인덱스니까.
  //fun_check(a != b);  loopback도 Pair에 들어갈 수 있으므로 이건 체크하지 말자.
  fun_check(a != HostId_Server);
  fun_check(a != HostId_None);
  fun_check(b != HostId_Server);
  fun_check(b != HostId_None);

  //@todo 대소가 중요한가??
  if (a > b) {
    Swap(a, b);
    Swap(rc_a, rc_b);
  }

  rc_pair pair(a, b);

  state->first_client = rc_a;
  state->second_client = rc_b;

  recyclable_pairs.Add(pair, state);
}

void P2PPairList::RemoveTooOldRecyclePair(double absolute_time) {
  for (auto it = recyclable_pairs.CreateIterator(); it; ++it) {
    auto state = it->value;

    fun_check(state->release_time_ > 0);

    if ((absolute_time - state->release_time_) > NetConfig::remove_too_old_recycle_pair_time_sec) {
      // 아마도 이미 다 해제된 후로 들어가니까 안해도 될거다.
      //state->first_client->p2p_connection_pairs_.Remove(state);
      //state->second_client->p2p_connection_pairs_.Remove(state);

      it.RemoveCurrent();
    }
  }
}

void P2PPairList::Clear() {
  active_pairs.Clear();
  recyclable_pairs.Clear();
}

P2PConnectionStatePtr P2PPairList::GetRecyclePair(HostId a, HostId b, bool remove) {
  //TODO 이건 왜해야할까??
  if (a > b) {
    Swap(a, b);
  }

  RCPair pair(a, b);
  auto found = recyclable_pairs.FindRef(pair);
  if (found && remove) {
    recyclable_pairs.Remove(pair);
  }
  return Found;
}

void NetServerImpl::candidate_remote_clients_::Remove(RemoteClient_S* rc) {
  Map<InetAddress, RemoteClient_S*>::Remove(rc->to_client_tcp_->remote_addr_);
}

NetServerImpl::UdpAddrToRemoteClientIndex::UdpAddrToRemoteClientIndex() {
  //TODO?
  //InitHashTable(NetConfig::ClientListHashTableValue);
}

NetServer* NetServer::New() {
  return new NetServerImpl();
}

bool P2PConnectionState::ContainsHostId(HostId peer_id) {
  for (int32 i = 0; i < 2; ++i) {
    if (peers[i].id == peer_id) {
      return true;
    }
  }
  return false;
}

bool P2PConnectionState::SetRecycleSuccess(HostId peer_id) {
  for (int32 i = 0; i < 2; ++i) {
    if (peer_holepunched_infos[i].id == peer_id) {
      SetServerHolepunchOk(peer_id, peer_holepunched_infos[i].internal_addr, peer_holepunched_infos[i].external_addr);
      return true;
    }
  }

  return false;
}

void P2PConnectionState::SetServerHolepunchOk(HostId peer_id, const InetAddress& internal_addr, const InetAddress& external_addr) {
  for (int32 i = 0; i < 2; ++i) {
    if (peers[i].id == peer_id) {
      peers[i].internal_addr = internal_addr;
      peers[i].external_addr = external_addr;
      return;
    }
  }

  for (int32 i = 0; i < 2; ++i) {
    if (peers[i].id == HostId_None) {
      peers[i].internal_addr = internal_addr;
      peers[i].external_addr = external_addr;
      peers[i].id = peer_id;
      return;

    }
  }
}

InetAddress P2PConnectionState::GetInternalAddr(HostId peer_id) {
  for (int32 i = 0; i < 2; ++i) {
    if (peers[i].id == peer_id) {
      return peers[i].internal_addr;
    }
  }

  return InetAddress::None;
}

InetAddress P2PConnectionState::GetHolepunchedInternalAddr(HostId peer_id) {
  for (int32 i = 0; i < 2; ++i) {
    if (peer_holepunched_infos[i].id == peer_id) {
      return peer_holepunched_infos[i].internal_addr;
    }
  }

  return InetAddress::None;
}

InetAddress P2PConnectionState::GetExternalAddr(HostId peer_id) {
  for (int32 i = 0; i < 2; ++i) {
    if (peers[i].id == peer_id) {
      return peers[i].external_addr;
    }
  }

  return InetAddress::None;
}

int32 P2PConnectionState::GetServerHolepunchOkCount() {
  int32 count = 0;
  for (int32 i = 0; i < 2; ++i) {
    if (peers[i].external_addr != InetAddress::None) {
      count++;
    }
  }

  return count;
}

P2PConnectionState::P2PConnectionState(NetServerImpl* owner, bool is_loopback_connection) {
  owner_ = owner;
  is_loopback_connection_ = is_loopback_connection;

  jit_direct_p2p_requested_ = false;
  is_relayed_ = true;
  dup_count_ = 0;
  recent_ping_ = 0;
  release_time_ = 0;
  last_holepunch_success_time_ = 0;

  member_join_acked_start_ = false;

  to_remote_peer_send_udp_message_attempt_count_ = 0;
  to_remote_peer_send_udp_message_success_count_ = 0;

  first_client_ = nullptr;
  second_client_ = nullptr;

  p2p_first_frame_number_ = (FrameNumber)0;

  // 통계 정보를 업뎃한다.
  if (!is_loopback_connection_) {
    // owner_->stats.p2p_connection_pair_count++;
  }
}

P2PConnectionState::~P2PConnectionState() {
  // 통계 정보를 업뎃한다.
  if (!is_loopback_connection) {
    //owner_->stats.p2p_connection_pair_count--;
  }

  if (!is_relayed) {
    //owner_->stats.p2p_direct_connection_pair_count--;
  }
}

void P2PConnectionState::SetRelayed(bool relayed) {
  if (is_relayed_ != relayed) {
    if (relayed) {
      //owner_->stats.p2p_direct_connection_pair_count--;
      for (int32 i = 0; i < 2; ++i) {
        peers[i] = PeerAddrInfo();
      }
    } else {
      //owner_->stats.p2p_direct_connection_pair_count++;
    }

    is_relayed_ = relayed;
  }
}

bool P2PConnectionState::GetRelayed() {
  return is_relayed_;
}

bool P2PConnectionState::CanRecycle() {
  // 홀펀칭이 된적이 있고, 같은쌍이 아니라면 재활용 가능하다.
  return (last_holepunch_success_time_ != 0 &&
          peer_holepunched_infos[0].id != HostId_None &&
          peer_holepunched_infos[1].id != HostId_None &&
          peer_holepunched_infos[0].id != peer_holepunched_infos[1].id);
}

void P2PConnectionState::MemberJoinStart(HostId PeerId_A, HostId PeerId_B) {
  peers[0] = PeerAddrInfo();
  peers[0].member_join_acked = false;
  peers[0].id = PeerId_A;

  peers[1] = PeerAddrInfo();
  peers[1].member_join_acked = false;
  peers[1].id = PeerId_B;

  member_join_acked_start_ = true;
}

bool P2PConnectionState::MemberJoinAckedAllComplete() {
  int32 count = 0;
  for (int32 i = 0; i < 2; ++i) {
    if (peers[i].member_join_acked) {
      ++count;
    }
  }
  return count == 2;
}

void P2PConnectionState::MemberJoinAcked(HostId peer_id) {
  for (int32 i = 0; i < 2; ++i) {
    if (peers[i].id == peer_id) {
      peers[i].member_join_acked = true;
      return;
    }
  }
}

void P2PConnectionState::SetPeerHolepunchOk(HostId peer_id, const InetAddress& sendto_addr, const InetAddress& recvfrom_addr) {
  for (int32 i = 0; i < 2; ++i) {
    if (peer_holepunched_infos[i].id == peer_id) {
      peer_holepunched_infos[i].internal_addr = GetInternalAddr(peer_id);
      peer_holepunched_infos[i].external_addr = GetExternalAddr(peer_id);
      peer_holepunched_infos[i].sendto_addr = sendto_addr;
      peer_holepunched_infos[i].recvfrom_addr = recvfrom_addr;
      return;
    }
  }

  for (int32 i = 0; i < 2; ++i) {
    if (peer_holepunched_infos[i].id == HostId_None) {
      peer_holepunched_infos[i].id = peer_id;
      peer_holepunched_infos[i].internal_addr = GetInternalAddr(peer_id);
      peer_holepunched_infos[i].external_addr = GetExternalAddr(peer_id);
      peer_holepunched_infos[i].sendto_addr = sendto_addr;
      peer_holepunched_infos[i].recvfrom_addr = recvfrom_addr;
      return;
    }
  }
}

InetAddress P2PConnectionState::GetHolepunchedSendToAddr(HostId peer_id) {
  for (int32 i = 0; i < 2; ++i) {
    if (peer_holepunched_infos[i].id == peer_id) {
      return peer_holepunched_infos[i].sendto_addr;
    }
  }

  return InetAddress::None;
}

InetAddress P2PConnectionState::GetHolepunchedRecvFromAddr(HostId peer_id) {
  for (int32 i = 0; i < 2; ++i) {
    if (peer_holepunched_infos[i].id == peer_id) {
      return peer_holepunched_infos[i].recvfrom_addr;
    }
  }

  return InetAddress::None;
}

void P2PConnectionState::ResetPeerInfo() {
  peers[0] = PeerAddrInfo();
  peers[1] = PeerAddrInfo();
}

//@todo SuperPeerSelectionPolicy::Ordinary, SuperPeerSelectionPolicy::Null로 대체 하도록하자. 함수 이름이 다소 길다!

SuperPeerSelectionPolicy::SuperPeerSelectionPolicy() {
  // 반드시 0만 채워야 한다! GetNull 때문

  real_udp_weight = 0;
  no_nat_device_weight = 0;
  server_lag_weight = 0;
  peer_lag_weight = 0;
  send_speed_weight = 0;
  exclude_newjoinee_duration_time = 0;
  frame_rate_weight = 0;
}

SuperPeerSelectionPolicy SuperPeerSelectionPolicy::GetOrdinary() {
  SuperPeerSelectionPolicy result;

  result.real_udp_weight = 10000;

  result.no_nat_device_weight = 500;

  result.server_lag_weight = 150;

  // 게임방 멤버가 4명 이상인 경우 서버와의 핑보다 피어와의 핑의 가중치가 상회함을 의미한다.
  result.peer_lag_weight = 50;

  // 송신 속도 측정 기능에 문제가 있는 라우터를 피하기 위해 0으로 설정되어 있다.
  result.send_speed_weight = 0;

  // 통상적으로 이정도는 잡아주어야 잦은 수퍼피어 변경을 막음
  result.exclude_newjoinee_duration_time = 5;

  // ServerLagWeight보다 약간더 우선순위가 크다. (최고 60프레임정도로 보았을때 240)
  result.frame_rate_weight = 4;

  return result;
}

SuperPeerSelectionPolicy SuperPeerSelectionPolicy::GetNull() {
  return SuperPeerSelectionPolicy();
}


P2PGroupOption P2PGroupOption::Default;

P2PGroupOption::P2PGroupOption() : direct_p2p_enabled(true) {}

ITaskSubject* NetServerImpl::GetTaskSubjectByHostId_NOLOCK(HostId subject_host_id) {
  GetMutex().AssertIsLockedByCurrentThread();
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (subject_host_id == HostId_Server) {
    return this;
  } else {
    return GetRemoteClientByHostId_NOLOCK(subject_host_id);
  }
}

void NetServerImpl::CheckCriticalSectionDeadLock_INTERNAL(const char* where) {
  AssertIsLockedByCurrentThread();

  for (auto rc : remote_client_instances_) {
    if (rc->IsLockedByCurrentThread()) {
      LOG(LogNetEngine, Warning, "RemoteClient deadlock - %s", where);
    }

    if (udp_assign_mode == ServerUdpAssignMode::PerClient) {
      if (rc->owned_udp_socket_) {
        if (rc->owned_udp_socket_->GetMutex().IsLockedByCurrentThread()) {
          LOG(LogNetEngine, Warning, "UDPSocket deadlock - %s", where);
        }
      }
    }
  }

  if (udp_assign_mode == ServerUdpAssignMode::Static) {
    const int32 count = udp_sockets_.Count();
    for (int32 i = 0; i < count; ++i) {
      if (udp_sockets_[i]->IsLockedByCurrentThread()) {
        LOG(LogNetEngine, Warning, "UDPSocket deadlock - %s", where);
      }
    }
  }

  if (tcp_issue_queue_mutex_.IsLockedByCurrentThread()) {
    LOG(LogNetEngine, Warning, "NetServer tcp_issue_queue_mutex_ deadlock - %s", where);
  }

  if (udp_issue_queue_mutex_.IsLockedByCurrentThread()) {
    LOG(LogNetEngine, Warning, "NetServer udp_issue_queue_mutex_ deadlock - %s", where);
  }
}

void NetServerImpl::OnSocketIoCompletion(Array<IHostObject*>& send_issued_pool, ReceivedMessageList& msg_list, CompletionStatus& completion) {
  IoCompletion_PerRemoteClient(completion, msg_list);
  IoCompletion_UdpConnectee(completion, msg_list);
}

void NetServerImpl::IoCompletion_PerRemoteClient(CompletionStatus& completion, ReceivedMessageList& msg_list) {
  AssertIsNotLockedByCurrentThread();

  RemoteClient_S* rc = nullptr;

  //NOTE: Debug모드에서는 디버거에 의존해서 작업하는게 훨씬더 편하므로, 일단 제껴둠..
#ifndef _DEBUG
  try
#endif {
    rc = LeanDynamicCast2(completion.completion_context);
    if (rc) {
      if (completion.type == CompletionType::Receive) {
        IoCompletion_TcpRecvCompletionCase(completion, rc, msg_list);
      } else if (completion.type == CompletionType::Send) {
        IoCompletion_TcpSendCompletionCase(completion, rc);
      } else if (completion.type == CompletionType::ReferCustomValue) {
        IoCompletion_TcpCustomValueCase(completion, rc);
      }
    }
  }
#ifndef _DEBUG
  catch (Exception& e) {
    CatchThreadExceptionAndPurgeClient(rc, __FUNCTION__, *e.Message());
  }
  catch (std::exception& e) {
    CatchThreadExceptionAndPurgeClient(rc, __FUNCTION__, (const char*)UTF8_TO_TCHAR(e.what()));
  }
  //catch (_com_error& e) {
  //  CatchThreadExceptionAndPurgeClient(rc, __FUNCTION__, (const char*)e.Description());
  //} catch (void* e) {
  //  e;
  //  CatchThreadExceptionAndPurgeClient(rc, __FUNCTION__, "void*");
  //} catch (...) { // RZ 내부 로직만 작동하므로
  //  if (NetConfig::catch_unhandled_exception) {
  //    CatchThreadExceptionAndPurgeClient(rc, __FUNCTION__, "Unknown");
  //  } else {
  //    throw;
  //  }
  //}
#endif
}

void NetServerImpl::IoCompletion_TcpCustomValueCase(CompletionStatus& completion, RemoteClient_S* rc) {
  AssertIsNotLockedByCurrentThread();

  switch ((IocpCustomValue)completion.custom_value) {
    case IocpCustomValue::NewClient:
      // 서버 종료 조건이 아니면 클라를 IOCP에 엮고 다음 송신 절차를 시작한다.
      if (net_thread_pool_) {
        IoCompletion_NewClientCase(rc);
      }
      break;
  }
}

//@todo Lock변수 이름이 살짝 애매모호함..
void NetServerImpl::IoCompletion_NewClientCase(RemoteClient_S* rc) {
  AssertIsNotLockedByCurrentThread();
  rc->AssertIsZeroUseCount();

  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  net_thread_pool_->AssociateSocket(rc->to_client_tcp_->socket_.Get());

  // asend public key, one of server UDP ports
  //MessageOut msg_to_send;
  //lf::Write(msg_to_send, MessageType::NotifyServerConnectionHint);
  //lf::Write(msg_to_send, intra_logger_);
  //lf::Write(msg_to_send, settings_);
  //lf::Write(msg_to_send, public_key_blob_);

  main_guard.Unlock();

  //{
  //  CScopedLock2 rc_send_queue_guard(rc->to_client_tcp_->GetSendQueueMutex());
  //  rc->to_client_tcp_->SendWhenReady(SendFragRefs(msg_to_send), TcpSendOption());
  //}

  CScopedLock2 rc_to_client_tcp_guard(rc->to_client_tcp_->GetMutex());

#ifdef _DEBUG
  // 만약 zero copy send로 설정되어 있으면 아래 구문이 블러킹이 일어날거다.
  // 굳이 zero copy send로 수정하고자 한다면 아래 보내기 기능도 non blocked send로 로직을 바꾸어야 하겠다.
  //@todo 의도가 무엇인가??? {
    int32 send_buf_size = 0;
    if (rc->to_client_tcp_->socket_->GetSendBufferSize(send_buf_size) == 0) {
      fun_check(send_buf_size > POLICY_FILE_TEXT.Len() + 1);
    }
  }
#endif

  //@todo 잠시 막아두자.. 나중에 풀어주던지...

  // 상대방이 flash 클라이건 C++ 클라이건 일단 이것을 보내버린다.
  // 수신측에서는 받은 첫 메시지는 버려야 할 것이다.

  //@todo Unity Web(or Flash) 보안 이슈 때문에 그런가?? (cross-domain??)
  // 이부분을 안보냈더니, 클라이언트에 이미 첫번째 policy문자열을 읽어주는 부분이 있으므로,
  // 메시지를 제대로 수신하지 못하고 있었음.
  // 진짜로 필요한지 여부를 재판단해보고, 만약 필요 없다면, 서버와 클라이언트 모두 수정해주어야함!!

  //@warning 최초에 이 policy 문자열을 클라이언트로 전송하는데, 이렇게 되면
  // Splitter test에서 예외가 걸리게 된다.
  // 이부분의 처리를 위해서 최초에 한번은 기능을 끄는게 좋을듯 함.

  //@warning 끝의 nul 문자까지 같이 보냄에 주의 해야함!
#if SEND_POLICY_FILE_AT_FIRST
  const int32 poicy_file_text_length = (POLICY_FILE_TEXT.Len() + 1); // with NUL
  int32 send_offset = 0;
  while (send_offset < poicy_file_text_length) {
    // BlockedSend는 return값이 sent입니다.
    // 그러므로 모두 보낼때 까지 while를 돌게 됩니다.
    const SocketErrorCode socket_error = rc->to_client_tcp_->socket_->BlockedSend(((uint8*)POLICY_FILE_TEXT.ConstData()) + send_offset, poicy_file_text_length - send_offset);
    if (socket_error == SocketErrorCode::error) {
      rc_to_client_tcp_guard.Unlock();

      rc->AssertIsNotLockedByCurrentThread();

      main_guard.Lock();
      CheckCriticalSectionDeadLock(__FUNCTION__);

      //@todo 에러 코드를 좀더 세분화할 필요가 있을듯함.
      rc->IssueDispose(ResultCode::DisconnectFromRemote, ResultCode::TCPConnectFailure, ByteArray(), __FUNCTION__, socket_error);
      rc->WarnTooShortDisposal(__FUNCTION__);

      rc->DecreaseUseCount();
      return;
    }

    send_offset += (int32)socket_error; //@todo 음?? 에러코드 이기전에, 전송한 길이이기도 하다. (에러가 아닐 경우)
  }
#endif

  // issue initial receive.
  const SocketErrorCode socket_error = rc->to_client_tcp_->IssueRecvAndCheck();
  if (socket_error != SocketErrorCode::Ok) {
    rc_to_client_tcp_guard.Unlock();

    rc->AssertIsNotLockedByCurrentThread();
    main_guard.Lock();

    rc->IssueDispose(ResultCode::DisconnectFromRemote, ResultCode::TCPConnectFailure, ByteArray(), __FUNCTION__, socket_error);
    rc->WarnTooShortDisposal(__FUNCTION__);
  } else { // 이거 없으면, unlock가 두번 될 가능성이 있다.
    // Decrease하기전에 RCLock이 unlock되어야한다.
    rc_to_client_tcp_guard.Unlock();
  }

  // accept에서 increase했으므로, 여기서 decrease.
  rc->DecreaseUseCount();
}

void NetServerImpl::IoCompletion_TcpSendCompletionCase(CompletionStatus& completion, RemoteClient_S* conn) {
  AssertIsNotLockedByCurrentThread();
  rc->AssertIsNotLockedByCurrentThread();

  ScopedUseCounter counter(*rc);
  CScopedLock2 rc_to_client_tcp_guard(rc->to_client_tcp_->GetMutex());

  rc->to_client_tcp_->send_issued_ = false;

  if (completion.completed_length < 0) { // 0도 인정?
    rc_to_client_tcp_guard.Unlock();

    rc->AssertIsNotLockedByCurrentThread();

    //TODO MainLock이 필요할까??
    CScopedLock2 main_guard(main_mutex_);
    CheckCriticalSectionDeadLock(__FUNCTION__);

    // TCP 연결이 끊어졌음을 의미한다. 따라서 에러 처리한다.
    const String comment = String::Format("%s : error=%d", __FUNCTION__, (int32)completion.socket_error);
    IssueDisposeRemoteClient(rc, ResultCode::DisconnectFromRemote, ResultCode::TCPConnectFailure, ByteArray(), *comment, completion.socket_error);
  } else {
    // 송신 큐에서 완료된 만큼의 데이터를 제거한다. 그리고 다음 송신을 건다.
    {
      CScopedLock2 rc_tcp_send_queue_guard(rc->to_client_tcp_->GetSendQueueMutex());
      rc->to_client_tcp_->send_queue_.DequeueNoCopy(completion.completed_length);
    }

    // 보낼게 남아있다면, 재 송신요청.
    const SocketErrorCode socket_error = rc->to_client_tcp_->ConditionalIssueSend(GetAbsoluteTime());
    if (socket_error != SocketErrorCode::Ok) {
      //@note 송신 Issue 실패, 종료 처리 수순으로 전환.
      rc_to_client_tcp_guard.Unlock();

      rc->AssertIsNotLockedByCurrentThread();

      //TODO 메인 락이 필요할까??
      CScopedLock2 main_guard(main_mutex_);
      CheckCriticalSectionDeadLock(__FUNCTION__);

      rc->IssueDispose(ResultCode::DisconnectFromRemote, ResultCode::TCPConnectFailure, ByteArray(), __FUNCTION__, socket_error);
      rc->WarnTooShortDisposal(__FUNCTION__);
    }

    total_tcp_send_count++;
    total_tcp_send_bytes += completion.completed_length;
  }
}

void NetServerImpl::IoCompletion_TcpRecvCompletionCase(CompletionStatus& completion, RemoteClient_S* rc, ReceivedMessageList& msg_list) {
  AssertIsNotLockedByCurrentThread();
  rc->AssertIsNotLockedByCurrentThread();

  // RC별로 lock
  ScopedUseCounter counter(*rc);
  CScopedLock2 rc_tcp_guard(rc->to_client_tcp_->GetMutex());

  rc->to_client_tcp_->recv_issued_ = false;

  if (completion.completed_length <= 0) { // disconnected
    rc_tcp_guard.Unlock();

    //TODO 메인락이 필요할까?
    CScopedLock2 main_guard(main_mutex_);
    CheckCriticalSectionDeadLock(__FUNCTION__);

    // TCP socket에서 연결이 실패했으므로 연결 해제 처리를 한다.
    IssueDisposeRemoteClient(rc, ResultCode::DisconnectFromRemote, ResultCode::TCPConnectFailure, ByteArray(), __FUNCTION__, completion.socket_error);
    rc->WarnTooShortDisposal(__FUNCTION__);

    if (intra_logger_) {
      const String text = String::Format("Disconnect connection.  host_id: %d, remote_addr: %s, socket_error: %s",
                    (int32)rc->host_id_, *rc->to_client_tcp_->remote_addr_.ToString(), *ToString(completion.socket_error));
      intra_logger_->WriteLine(LogCategory::System, *text);
    }
  } else {
    // 수신 큐에서 받은 데이터를 꺼낸 후 ...
    const uint8* RecviedData = rc->to_client_tcp_->socket_->GetRecvBufferPtr();
    rc->to_client_tcp_->recv_stream_.EnqueueCopy(RecviedData, completion.completed_length);

    // 완전한 msg가 도착한 것들을 모두 추려서 final recv queue로 옮기거나 여기서 처리한다.
    //ReceivedMessageList extracted_msg_list;
    //msg_list.EmptyAndKeepCapacity();
    msg_list.Reset();

    const ResultCode error = rc->ExtractMessagesFromTcpStream(msg_list);
    if (error != ResultCode::Ok) {
      rc_tcp_guard.Unlock();

      //TODO 메인락이 필요할까?
      CScopedLock2 main_guard(main_mutex_);
      CheckCriticalSectionDeadLock(__FUNCTION__);

      // 해당 클라와의 TCP 통신이 더 이상 불가능한 상황이다.
      IssueDisposeRemoteClient(rc, error, ResultCode::TCPConnectFailure, ByteArray(), __FUNCTION__, SocketErrorCode::Ok); //InduceDisconnect()
      return; //return 하는게 맞는듯 하다...어차피 dis니까..문제가 있으면 수정토록 하자.
    }

    rc_tcp_guard.Unlock();


    // TCP 스트림을 받은 시간을 키핑한다.
    // 이것은 데이터 레이싱이지만, volatile로 그냥 넘긴다.
    rc->last_tcp_stream_recv_time_ = GetAbsoluteTime();
    total_tcp_recv_count++;
    total_tcp_recv_bytes += completion.completed_length;

    // 이 함수 안에서 Lock(main).
    IoCompletion_ProcessMessageOrMoveToFinalRecvQueue(rc, msg_list, nullptr);

    //다시 락을 걸고 issurecvandCheck를 한다...
    //****중요**** 컨텐션이 큰지 꼭체크 하고 수정할것~~
    //맘에 안든다. 방식을 생각해보자~
    rc_tcp_guard.Lock();

    // 다음 recv를 건다.
    const SocketErrorCode socket_error = rc->to_client_tcp_->IssueRecvAndCheck();
    if (socket_error != SocketErrorCode::Ok) {
      // MainLock를 하기전에 무조건 RC의 unlock를 해야 한다.
      rc_tcp_guard.Unlock();

      //TODO 메인 락이 필요할까??
      CScopedLock2 main_guard(main_mutex_);
      CheckCriticalSectionDeadLock(__FUNCTION__);

      rc->IssueDispose(ResultCode::DisconnectFromRemote, ResultCode::TCPConnectFailure, ByteArray(), __FUNCTION__, socket_error);
      rc->WarnTooShortDisposal(__FUNCTION__);
    }
  }
}

//FIXME 현재 UDP를 통해서 수신한 경우, extracted_msg_list 객체가 깨지는 문제가 있음.  왜일까??
void NetServerImpl::IoCompletion_ProcessMessageOrMoveToFinalRecvQueue(RemoteClient_S* rc, ReceivedMessageList& extracted_msg_list, UdpSocket_S* udp_socket) {
  AssertIsNotLockedByCurrentThread();

  for (auto& received_msg : extracted_msg_list) {
    ASSERT_OR_HACKED(received_msg.unsafe_message.AtBegin());
    IoCompletion_ProcessMessage_EngineLayer(received_msg, rc, udp_socket);
  }
}

bool NetServerImpl::IoCompletion_ProcessMessage_EngineLayer(ReceivedMessage& received_msg, RemoteClient_S* rc, UdpSocket_S* udp_socket) {
  AssertIsNotLockedByCurrentThread();

  auto& msg = received_msg.unsafe_message;
  const int32 saved_read_pos = msg.Tell();

  MessageType msg_type;
  if (!lf::Read(msg, msg_type)) {
    msg.Seek(saved_read_pos);
    return false;
  }

  bool msg_processed = false;
  const bool is_real_udp = (udp_socket != nullptr);

  //debug
#if TRACE_NETSERVER_MESSAGES
  String remote_ip;
  if (is_real_udp) {
    remote_ip = received_msg.remote_addr_udp_only.ToString();
  }
  else {
    remote_ip = rc->to_client_tcp_->remote_addr_.ToString();
  }

  const String debug_str = String::Format("[%05d] [%s] [%s] [%s] %s (LEN=%d)",
                  (int32)received_msg.remote_id,
                  *remote_ip,
                  is_real_udp ? "UDP" : "TCP",
                  received_msg.relayed ? "[Relayed]" : "NonRelayed",
                  *ToString(msg_type), msg.GetLength());
  LOG(LogNetEngine, info, "NetServer.MSG : %s", *debug_str);
#endif

  switch (msg_type) {
    case MessageType::ServerHolepunch:
      // 이 메시지는 아직 RemoteClient가 배정되지 않은 클라이언트에 대해서도 작동한다.
      ASSERT_OR_HACKED(received_msg.remote_addr_udp_only.IsUnicast());
      IoCompletion_ProcessMessage_ServerHolepunch(received_msg.unsafe_message, received_msg.remote_addr_udp_only, udp_socket);
      msg_processed = true;
      break;

    case MessageType::PeerUdp_ServerHolepunch:
      // 이 메시지는 아직 RemoteClient가 배정되지 않은 클라이언트에 대해서도 작동한다.
      ASSERT_OR_HACKED(received_msg.remote_addr_udp_only.IsUnicast());
      IoCompletion_ProcessMessage_PeerUdp_ServerHolepunch(received_msg.unsafe_message, received_msg.remote_addr_udp_only, udp_socket);
      msg_processed = true;
      break;

    case MessageType::NotifyCSEncryptedSessionKey:
      IoCompletion_ProcessMessage_NotifyCSEncryptedSessionKey(msg, rc);
      msg_processed = true;
      break;

    case MessageType::NotifyServerConnectionRequestData:
      IoCompletion_ProcessMessage_NotifyServerConnectionRequestData(msg, rc);
      msg_processed = true;
      break;

    case MessageType::RequestServerConnectionHint:
      IoCompletion_ProcessMessage_RequestServerConnectionHint(msg, rc);
      msg_processed = true;
      break;

    case MessageType::RequestServerTimeAndKeepAlive:
      IoCompletion_ProcessMessage_RequestServerTimeAndKeepAlive(msg, rc);
      msg_processed = true;
      break;

    case MessageType::SpeedHackDetectorPing:
      IoCompletion_ProcessMessage_SpeedHackDetectorPing(msg, rc);
      msg_processed = true;
      break;

    case MessageType::UnreliableRelay1:
      IoCompletion_ProcessMessage_UnreliableRelay1(msg, rc);
      msg_processed = true;
      break;

    case MessageType::ReliableRelay1:
      IoCompletion_ProcessMessage_ReliableRelay1(msg, rc);
      msg_processed = true;
      break;

    case MessageType::UnreliableRelay1_RelayDestListCompressed:
      IoCompletion_ProcessMessage_UnreliableRelay1_RelayDestListCompressed(msg, rc);
      msg_processed = true;
      break;

    case MessageType::LingerDataFrame1:
      IoCompletion_ProcessMessage_LingerDataFrame1(msg, rc);
      msg_processed = true;
      break;

    case MessageType::NotifyHolepunchSuccess:
      IoCompletion_ProcessMessage_NotifyHolepunchSuccess(msg, rc);
      msg_processed = true;
      break;

    case MessageType::PeerUdp_NotifyHolepunchSuccess:
      IoCompletion_ProcessMessage_PeerUdp_NotifyHolepunchSuccess(msg, rc);
      msg_processed = true;
      break;

    //case MessageType::RUdp_P2PDisconnectLinger1:
    //    NetWorkerThread_RUdp_P2PDisconnectLinger1(msg, rc);
    //    msg_processed = true;
    //    break;

    case MessageType::RPC:
      IoCompletion_ProcessMessage_RPC(received_msg, msg_processed, rc, is_real_udp);
      break;

    case MessageType::FreeformMessage:
      IoCompletion_ProcessMessage_FreeformMessage(received_msg, msg_processed, rc, is_real_udp);
      break;

    case MessageType::Encrypted_Reliable:
    case MessageType::Encrypted_Unreliable: {
        ReceivedMessage decrypted_received_msg;
        if (DecryptMessage(msg_type, received_msg, decrypted_received_msg.unsafe_message)) {
          decrypted_received_msg.relayed = received_msg.relayed;
          decrypted_received_msg.remote_addr_udp_only = received_msg.remote_addr_udp_only;
          decrypted_received_msg.remote_id = received_msg.remote_id;
          msg_processed |= IoCompletion_ProcessMessage_EngineLayer(decrypted_received_msg, rc, udp_socket); // recursive call
        }
      }
      break;

    case MessageType::Compressed: {
        ReceivedMessage decompressed_received_msg;
        if (DecompressMessage(received_msg, decompressed_received_msg.unsafe_message)) {
          decompressed_received_msg.relayed = received_msg.relayed;
          decompressed_received_msg.remote_addr_udp_only = received_msg.remote_addr_udp_only;
          decompressed_received_msg.remote_id = received_msg.remote_id;
          msg_processed |= IoCompletion_ProcessMessage_EngineLayer(decompressed_received_msg, rc, udp_socket); // recursive call
        }
        break;
      }

    case MessageType::RequestReceiveSpeedAtReceiverSide_NoRelay:
      IoCompletion_ProcessMessage_RequestReceiveSpeedAtReceiverSide_NoRelay(msg, rc);
      msg_processed = true;
      break;

    case MessageType::ReplyReceiveSpeedAtReceiverSide_NoRelay:
      IoCompletion_ProcessMessage_ReplyReceiveSpeedAtReceiverSide_NoRelay(received_msg, rc);
      msg_processed = true;
      break;

    default:
      break;
  }

  // 만약 잘못된 메시지가 도착한 것이면 이미 FunNet 계층에서 처리한 것으로 간주하고
  // 메시지를 폐기한다. 그리고 예외 발생 이벤트를 던진다.
  // 단, C++ 예외를 발생시키지 말자. 디버깅시 혼란도 생기며 fail over 처리에도 애매해진다.

  //@todo
  // 프로토콜 버져닝을 지원할 경우에, 체크 상황이 달라질 수 있을듯 싶다.
  // RPC일때만 체크를 달리하는 형태로 해야할까??
  // endmarker정도만 체크하는건 어떨런지??

  const int32 l1 = msg.GetLength();
  const int32 l2 = msg.Tell();

  if (msg_processed &&
      l1 != l2 &&
      msg_type != MessageType::Encrypted_Reliable && msg_type != MessageType::Encrypted_Unreliable) { // 암호화된 메시지는 별도 버퍼에서 복호화된 후 처리되므로
    msg_processed = true; // 정상적으로 처리된걸로 간주함.

    // 에러시에 마지막 메세지를 저장한다.
    // 복사 오버헤드가 있겠지만, 에러시에만 하는 동작이므로 크게 문제가 있어 보이지는 않음.
    // (단 에러가 자주 발생해서는 안되겠지..)
    //ByteArray last_received_msg(msg.GetData(), msg.GetLength());
    //EnqueueError(ResultInfo::From(ResultCode::InvalidPacketFormat, received_msg.remote_id, __FUNCTION__, last_received_msg));

    //TODO 문제를 일으킨 패킷 데이터를 넘길때 전체를 넘기는게 맞는건지? 아니면 유효한 부분만 넘겨야하는건지?
    //최종적으로 검증용이니, 전체를 넘기는게 맞겠지??

    //메시지 전체 내용을 복사해서 전달하는게 좋을듯...

    EnqueueError(ResultInfo::From(ResultCode::InvalidPacketFormat, received_msg.remote_id, __FUNCTION__, msg.ToAllBytesCopy()));
  }

  //AssureMessageReadOkToEnd(msg_processed, Type, received_msg, rc);

  if (!msg_processed) {
    // 처리되지 않았다면, 호출되기 전의 메시지 읽기 위치로 복구.
    msg.Seek(saved_read_pos);
    return false;
  }

  return true;
}

/**

UDP 전용 메시지(UDP로 수신 받고 UDP로 송신해야함!)

클라이언트 -> 서버로 서버와의 홀펀칭을 하고 싶다고 요청한 상태에서 호출되는 녀석임.

이름을 좀 의미 있게 바꿀까?
C2SRequestServerHolepunch이쯤??

실제 내부에서 하는것을 보면 별다른건 없고, 보낸이의 정보를 담아서(echo?)
UDP 소켓을 통해서 클라이언트로 보내주는게 다인데...

이게 현재 AWS환경하에서 외부에서 접속하여 시도하면 전달이 안되고 있다.

왜일까??

TCP VIEW를 통해서 확인해 본바 UDP 수신이 전혀 안되고 있다.
UDP 소켓쪽에 무슨 문제가 있는건지, 아니면 포트가 막혀 있는건지..
현재 방화벽을 양쪽 모두 꺼둔 상태이므로 포트가 막힌것 같지는 않은데 흠...

*/
void NetServerImpl::IoCompletion_ProcessMessage_ServerHolepunch(MessageIn& msg, const InetAddress& from, UdpSocket_S* udp_socket) {
  AssertIsNotLockedByCurrentThread();

  if (udp_socket == nullptr) {
    // 버그 혹은 패킷이 깨져서 발생하는 경우임...
    TRACE_SOURCE_LOCATION();
    return;
  }

  udp_socket->AssertIsZeroUseCount();

  Uuid tag;
  if (!lf::Read(msg, tag)) {
    TRACE_SOURCE_LOCATION();
    return;
  }

  // ack magic number and remote UDP address via UDP once
  MessageOut msg_to_send;
  lf::Write(msg_to_send, MessageType::ServerHolepunchAck);
  lf::Write(msg_to_send, tag);
  lf::Write(msg_to_send, from);
  const auto filter_tag = FilterTag::Make(HostId_Server, HostId_None); // server -> unknown
  const UdpSendOption send_opt(MessagePriority::Holepunch, EngineOnlyFeature);
  udp_socket->SendWhenReady(HostId_None, filter_tag, from, SendFragRefs(msg_to_send), send_opt);

  // 이하는 main-lock하에 진행.
  //TODO main-lock으로 할 필요가 있을까?  별도의 락으로 하는게 좋을듯 싶은데...
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (intra_logger_) {
    const String text = String::Format("ServerHolepunchAck from %s", *from.ToString());
    intra_logger_->WriteLine(LogCategory::SP2P, *text);;
  }
}

/**

UDP 전용 메시지(UDP로 수신 받고 UDP로 송신해야함!)


Peer들과 서버와의 홀펀칭을 시도요청을 클라이언트에게서 받음.

*/
void NetServerImpl::IoCompletion_ProcessMessage_PeerUdp_ServerHolepunch(MessageIn& msg, const InetAddress& from, UdpSocket_S* udp_socket) {
  AssertIsNotLockedByCurrentThread();

  if (udp_socket == nullptr) {
    // 버그 혹은 패킷이 깨져서 발생하는 경우임.
    // 패킷이 깨진 경우, 메시지 타입 오식별로 인해서 UdpSocket이 null인 상태에서 도달가능함.
    TRACE_SOURCE_LOCATION();
    return;
  }

  udp_socket->AssertIsZeroUseCount();

  Uuid holepunch_tag;
  HostId peer_id;
  if (!lf::Reads(msg,  holepunch_tag, peer_id)) {
    TRACE_SOURCE_LOCATION();
    return;
  }

  // ack magic number and remote UDP address via UDP once
  MessageOut msg_to_send;
  lf::Write(msg_to_send, MessageType::PeerUdp_ServerHolepunchAck);
  lf::Write(msg_to_send, holepunch_tag);
  lf::Write(msg_to_send, from);
  lf::Write(msg_to_send, peer_id);
  const auto filter_tag = FilterTag::Make(HostId_Server, HostId_None);
  const UdpSendOption send_opt(MessagePriority::Holepunch, EngineOnlyFeature);
  udp_socket->SendWhenReady(HostId_None, filter_tag, from, SendFragRefs(msg_to_send), send_opt);

  // 이하는 main-lock하에 진행.
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (intra_logger_) {
    const String text = String::Format("PeerUdp_ServerHolepunch from %s", *from.ToString());
    intra_logger_->WriteLine(LogCategory::SP2P, *text);
  }
}

void NetServerImpl::IoCompletion_ProcessMessage_NotifyCSEncryptedSessionKey(MessageIn& msg, RemoteClient_S* rc) {
  AssertIsNotLockedByCurrentThread();
  rc->AssertIsZeroUseCount();

  // AES/RC4 key를 얻는다.
  ByteArray encrypted_aes_key_blob; // Strong
  ByteArray encrypted_rc4_key_blob; // Weak
  if (!lf::Reads(msg,  encrypted_aes_key_blob, encrypted_rc4_key_blob)) {
    TRACE_SOURCE_LOCATION();
    return;
  }

  // 암호화된 세션키를 복호화해서 클라이언트 세션키에 넣는다.
  // SelfXchgKey는 서버 시작시에 한번만 만드므로 락을 안걸어도 괜찮을것 같다.
  ByteArray out_random_block;
  SharedPtr<ResultInfo> error = CryptoRSA::DecryptSessionKeyByPrivateKey(out_random_block, encrypted_aes_key_blob, self_xchg_key_);
  if (error) {
    TRACE_SOURCE_LOCATION();
    return;
  }

  // MainLock전에 remotelock가 걸렸다면 대략 난감...
  rc->AssertIsNotLockedByCurrentThread();
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  // 미인증 클라인지 확인.
  // 해킹된 클라에서 이게 반복적으로 오는 것일 수 있다. 그런 경우 무시하도록 하자.
  // Candidate 목록이 비대 하지는 않을테니, 검색 속도에 문제는 없으려나??

  //if (!candidate_remote_clients_.ContainsValue(rc))
  if (candidate_remote_clients_.FindKey(rc) == nullptr) {
    TRACE_SOURCE_LOCATION();
    return;
  }

  //@todo 암호화는 별도의 스레드에서 처리하는게 어떨런지..?

  // AES 키 세팅 / RC4 키 세팅
  ByteArray out_rc4_random_block;
  if (!CryptoAES::ExpandFrom(rc->session_key_.aes_key, (const uint8*)out_random_block.ConstData(), settings_.strong_encrypted_message_key_length / 8) ||
      !CryptoAES::Decrypt(rc->session_key_.aes_key, encrypted_rc4_key_blob, out_rc4_random_block) ||
      !CryptoRC4::ExpandFrom(rc->session_key_.rc4_key, (const uint8*)out_rc4_random_block.ConstData(), settings_.weak_encrypted_message_key_length / 8)) {
    callbacks_->OnException(HostId_None, Exception("failed to create session-key"));

    MessageOut msg_to_send;
    lf::Write(msg_to_send, MessageType::NotifyServerDeniedConnection);
    lf::Write(msg_to_send, ByteArray());

    CScopedLock2 rc_tcp_send_queue_guard(rc->to_client_tcp_->GetSendQueueMutex());
    rc->to_client_tcp_->SendWhenReady(SendFragRefs(msg_to_send), TcpSendOption());
    return;
  }

  rc->session_key_received_ = true;

  main_guard.Unlock(); //unlock를 하든 말든 상관없지만 기왕이면 하자.

  MessageOut msg_to_send;
  lf::Write(msg_to_send, MessageType::NotifyCSSessionKeySuccess);

  // MainLock를 풀 필요는 없다.
  {
    CScopedLock2 rc_tcp_send_queue_guard(rc->to_client_tcp_->GetSendQueueMutex());
    rc->to_client_tcp_->SendWhenReady(SendFragRefs(msg_to_send), TcpSendOption());
  }
}

void NetServerImpl::IoCompletion_ProcessMessage_NotifyServerConnectionRequestData(MessageIn& msg, RemoteClient_S* rc) {
  AssertIsNotLockedByCurrentThread();
  rc->AssertIsZeroUseCount();
  rc->AssertIsNotLockedByCurrentThread();

  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  // 미인증 클라이언트 여부 체크
  //@note 이곳에서 미인증에서 인증 상태의 클라이언트로 바뀌게 되므로, 우선 미인증 클라이언트인지 부터 체크 하도록 하자.
  //if (!candidate_remote_clients_.ContainsValue(rc)) //@warning 결과적으로, 본 메시지는 한번만 와야함. 두번째 부터는 실패함.
  if (candidate_remote_clients_.FindKey(rc) == nullptr) { //@warning 결과적으로, 본 메시지는 한번만 와야함. 두번째 부터는 실패함.
    NotifyProtocolVersionMismatch(rc);
    return; // 오류로 인해서 리턴하면, 해당 RC는 방치 상태가 되는데, 방치된 RC는 일정시간 이후 시스템에서 제거 될것이므로 괜찮음.
  }

  // Unpack parameters
  ByteArray msg_user_data;
  Uuid msg_protocol_version;
  uint32 msg_internal_version;
  if (!lf::Reads(msg,  msg_user_data, msg_protocol_version, msg_internal_version)) {
    NotifyProtocolVersionMismatch(rc);
    return; // 오류로 인해서 리턴하면, 해당 RC는 방치 상태가 되는데, 방치된 RC는 일정시간 이후 시스템에서 제거 될것이므로 괜찮음.
  }

  //@todo 버젼 체크는 정책을 수립해서 처리하는게 바람직할 듯 함!
  // protocol version match and assign received session-key
  if (msg_protocol_version != protocol_version || msg_internal_version != internal_version_) {
    NotifyProtocolVersionMismatch(rc);
    return; // 오류로 인해서 리턴하면, 해당 RC는 방치 상태가 되는데, 방치된 RC는 일정시간 이후 시스템에서 제거 될것이므로 괜찮음.
  }

  // 암호화 키가 교환이 완료됐나 체크한다.
  //@note 암호화 키가 교환이 되어 있지 않은 상태라면, 접속을 거부한다.
  if (!rc->session_key_.KeyExists()) {
    MessageOut msg_to_send;
    lf::Write(msg_to_send, MessageType::NotifyServerDeniedConnection);
    lf::Write(msg_to_send, ByteArray());
    {
      CScopedLock2 rc_tcp_send_queue_guard(rc->to_client_tcp_->GetSendQueueMutex());
      rc->to_client_tcp_->SendWhenReady(SendFragRefs(msg_to_send), TcpSendOption());
    }
    return; // 오류로 인해서 리턴하면, 해당 RC는 방치 상태가 되는데, 방치된 RC는 일정시간 이후 시스템에서 제거 될것이므로 괜찮음.
  }

  if (callbacks_) {
    // 콜백이 지정된 경우, 콜백내에서 다소 무거운(DB query 같은) 작업을 통해서 인증을 처리해야할 수
    // 있으므로, 여기서 바로 처리하지 않고, 유저 스레드풀에서 처리가 이루어지도록 함.
    EnqueueClientJoinApproveDetermine(rc->to_client_tcp_->remote_addr_, msg_user_data);
  } else {
    // 콜백이 지정안되어 있으므로, 여기서 바로 성공처리해줌.
    ProcessOnClientJoinApproved(rc, ByteArray());

    // stats
    //CCounters::IncreaseBy("NetServer", "CCU", 1);
  }
}

// 클라이언트에게 서버의 설정 값등을 알려주어서, 올바른 방법으로 접속할 수
// 있도록 힌트를 제공함.
void NetServerImpl::IoCompletion_ProcessMessage_RequestServerConnectionHint(MessageIn& msg, RemoteClient_S* rc) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  RuntimePlatform runtime_platform = RuntimePlatform::Unknown;
  if (!lf::Read(msg, runtime_platform)) {
    NotifyProtocolVersionMismatch(rc);
    return;
  }

  // rc에 runtime_platform 값을 저장해야할듯...

  const bool intra_logging_on = intra_logger_.IsValid();

  MessageOut msg_to_send;
  lf::Write(msg_to_send, MessageType::NotifyServerConnectionHint);
  lf::Write(msg_to_send, intra_logging_on);
  lf::Write(msg_to_send, settings_);
  lf::Write(msg_to_send, public_key_blob_);
  {
    CScopedLock2 rc_tcp_send_queue_guard(rc->to_client_tcp_->GetSendQueueMutex());
    rc->to_client_tcp_->SendWhenReady(SendFragRefs(msg_to_send), TcpSendOption());
  }
}

void NetServerImpl::IoCompletion_ProcessMessage_RequestServerTimeAndKeepAlive(MessageIn& msg, RemoteClient_S* rc) {
  AssertIsNotLockedByCurrentThread();
  rc->AssertIsZeroUseCount();

  double client_local_time;
  double measured_last_lag;
  if (!lf::Reads(msg,  client_local_time, measured_last_lag)) {
    TRACE_SOURCE_LOCATION();
    return;
  }

  rc->AssertIsNotLockedByCurrentThread();
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  const double absolute_time = GetAbsoluteTime();

  // 가장 마지막에 unreliable ping을 받은 시간을 체크해둔다.
  rc->last_udp_packet_recv_time_ = absolute_time;
  rc->last_udp_ping_recv_time_ = absolute_time;

  // 측정된 랙도 저장해둔다.
  rc->last_ping_ = measured_last_lag;
  // 급작스럽게 변하는걸 방지하기 위해서 직접 대입이 아닌 보간으로 처리.
  rc->recent_ping_ = MathBase::Lerp(rc->recent_ping_, measured_last_lag, 0.5);

  // 에코를 unreliable하게 보낸다.
  MessageOut msg_to_send;
  lf::Write(msg_to_send, MessageType::ReplyServerTime);
  lf::Write(msg_to_send, client_local_time);
  lf::Write(msg_to_send, absolute_time);

  // UDP가 활성화된 경우에는 UDP로 unreliable하게
  // 그렇지 않은 경우에는 TCP로 경유해서 보낸다.
  const UdpSendOption send_opt(MessagePriority::Ring0, EngineOnlyFeature);
  rc->to_client_udp_fallbackable_.SendWhenReady(rc->GetHostId(), SendFragRefs(msg_to_send), send_opt);
}

void NetServerImpl::IoCompletion_ProcessMessage_SpeedHackDetectorPing(MessageIn& msg, RemoteClient_S* rc) {
  AssertIsNotLockedByCurrentThread();
  rc->AssertIsZeroUseCount();
  rc->AssertIsNotLockedByCurrentThread();

  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  // detect speed-hack.
  rc->DetectSpeedHack();
}

struct UnreliableDestInfo {
  IHostObject* sendto_rc;
  HostId send_to;
  InetAddress sendto_addr;
};

void NetServerImpl::IoCompletion_ProcessMessage_UnreliableRelay1(MessageIn& msg, RemoteClient_S* rc) {
  AssertIsNotLockedByCurrentThread();

  // Unpack parameters
  MessagePriority priority;
  uint64 unique_id;
  HostIdArray relay_dest_list;
  OptimalCounter32 payload_length;
  if (!lf::Reads(msg,  priority, unique_id, relay_dest_list, payload_length)) {
    TRACE_SOURCE_LOCATION();
    return;
  }

  // 잘못된 크기의 데이터가 왔을 경우에는 조작 내지는 엔진의 버그일 수 있음.
  if (payload_length < 0 || payload_length >= settings_.message_max_length) {
    // 엔진 버그 아니면 해커의 소행일 수 있음. 해커의 소행이라면 릴리즈 빌드로 띄우자.
    TRACE_SOURCE_LOCATION();
    EnqueueHackSuspectEvent(rc, __FUNCTION__, HackType::PacketRig);
    return;
  }

  // 받은 데이터를 직접 접근하는 객체. 복사를 줄이기 위해.
  ASSERT_OR_HACKED(msg.GetReadableLength() == payload_length);
  MessageIn payload;
  if (!msg.ReadAsShared(payload, payload_length)) {
    // 엔진 버그 아니면 해커의 소행일 수 있음. 해커의 소행이라면 릴리즈 빌드로 띄우자.
    TRACE_SOURCE_LOCATION();
    EnqueueHackSuspectEvent(rc, __FUNCTION__, HackType::PacketRig);
    return;
  }

  CScopedLock2 main_guard(main_mutex_);
  IoCompletion_MulticastUnreliableRelay2_AndUnlock(&main_guard, relay_dest_list, rc->host_id_, payload, priority, unique_id);
}

void NetServerImpl::IoCompletion_ProcessMessage_UnreliableRelay1_RelayDestListCompressed(MessageIn& msg, RemoteClient_S* rc) {
  AssertIsNotLockedByCurrentThread();

  // Unpack parameters
  MessagePriority priority;
  int64 unique_id;
  if (!lf::Reads(msg,  priority, unique_id)) {
    TRACE_SOURCE_LOCATION();
    return;
  }

  // P2P group 어디에도 비종속 클라들
  HostIdArray includee_host_id_list;
  OptimalCounter32 p2p_group_subset_list_count;
  if (!lf::Reads(msg,  includee_host_id_list, p2p_group_subset_list_count)) {
    TRACE_SOURCE_LOCATION();
    return;
  }

  Array<P2PGroupSubset_S, InlineAllocator<256>> p2p_group_subset_list(p2p_group_subset_list_count);
  for (int32 i = 0; i < p2p_group_subset_list_count; ++i) {
    auto& p2p_dest = p2p_group_subset_list[i];

    if (!lf::Reads(msg, p2p_dest.group_id, p2p_dest.excludee_host_id_list)) {
      TRACE_SOURCE_LOCATION();
      return;
    }
  }

  // 페이로드 길이를 가져옴
  OptimalCounter32 payload_length;
  if (!lf::Read(msg, payload_length)) {
    TRACE_SOURCE_LOCATION();
    return;
  }

  // 페이로드의 길이가 정상적이지 않다면, 오류로 간주(해킹이려나??)
  if (payload_length < 0 || payload_length >= settings_.message_max_length) {
    TRACE_SOURCE_LOCATION();
    EnqueueHackSuspectEvent(rc, __FUNCTION__, HackType::PacketRig);
    return;
  }

  ASSERT_OR_HACKED(msg.GetReadableLength() == payload_length);
  MessageIn payload;
  if (!msg.ReadAsShared(payload, payload_length)) {
    TRACE_SOURCE_LOCATION();
    EnqueueHackSuspectEvent(rc, __FUNCTION__, HackType::PacketRig);
    return;
  }

  // relay할 호스트들을 뽑아낸다.
  HostIdArray final_relay_dest_list;
  final_relay_dest_list.Reserve(10000); //TODO 10000개나 미리 잡아야하나???

  // 기본 포함하는 대상이니 일단은 그대로 assign함.
  final_relay_dest_list = includee_host_id_list;

  // RC를 얻어내야하기 때문에 main lock이 필요하다.
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  for (int32 i = 0; i < p2p_group_subset_list.Count(); ++i) {
    const auto& p2p_dest = p2p_group_subset_list[i];

    if (auto group = GetP2PGroupByHostId_NOLOCK(p2p_dest.group_id)) {
      //// NOTE: ExcludeeHostIdList와 g->Members는 정렬되어 있다.
      //auto ItA = p2p_dest.excludee_host_id_list.begin(); // DirectP2P로 이미 보낸 리스트
      //auto ItB = group->members_.begin(); // P2P 전체 인원 리스트
            //
      //// merge sort와 비슷한 방법으로, group member들로부터 excludee를 제외한 것들을 찾는다.
      //for (; ItA != p2p_dest.excludee_host_id_list.end() || ItB != group->members_.end();) {
      //  if (ItA == p2p_dest.excludee_host_id_list.end()) {
      //    final_relay_dest_list.Add(ItB->key);
      //    ++ItB;
      //    continue;
      //  }
            //
      //  if (ItB == group->members_.end()) {
      //    break;
      //  }
            //
      //  if (*ItA > ItB->key) { // 릴레이 인원에 추가시킴
      //    final_relay_dest_list.Add(ItB->key);
      //    ++ItB;
      //  }
      //  else if (*ItA < ItB->key) {
      //    // 그냥 넘어감
      //    ++ItA;
      //  }
      //  else {
      //    // 양쪽 다 같음, 즉 부전승. 둘다 그냥 넘어감.
      //    ++ItA;
      //    ++ItB;
      //  }
      //}

      //TODO 이미 정렬된 상태에서는 처리가 위에서처럼 빠르지만, 코드 정리가 아직 안되었고
      //group->Members가 아직 ordered-map이 아니므로 일단은 아래처럼 처리.

      for (auto member_pair : group->members_) {
        if (!p2p_dest.excludee_host_id_list.Contains(member_pair.key)) { // 제외 대상에 없는 경우에만 최종 목록에 추가함.
          final_relay_dest_list.Add(member_pair.key);
        }
      }
    }
  }

  IoCompletion_MulticastUnreliableRelay2_AndUnlock(&main_guard, final_relay_dest_list, rc->host_id_, payload, priority, unique_id);
}

void NetServerImpl::IoCompletion_ProcessMessage_ReliableRelay1(MessageIn& msg, RemoteClient_S* rc) {
  AssertIsNotLockedByCurrentThread();

  // Unpack parameters
  RelayDestList relay_dest_list;
  OptimalCounter32 payload_length;
  if (!lf::Reads(msg,  relay_dest_list, payload_length)) {
    TRACE_SOURCE_LOCATION();
    return;
  }

  // 페이로드 길이를 체크함. 정상적인 경우가 아니라면, 무시 혹은 경고.
  if (payload_length < 0 || payload_length >= settings_.message_max_length) {
    ASSERT_OR_HACKED(0);
    return;
  }

  ASSERT_OR_HACKED(msg.GetReadableLength() == payload_length);
  MessageIn payload;
  if (!msg.ReadAsShared(payload, payload_length)) {
    ASSERT_OR_HACKED(0);
    return;
  }

  struct ReliableDestInfo {
    FrameNumber frame_number;
    RemoteClient_S* sendto_rc;
    HostId send_to;
  };

  const int32 relay_count = relay_dest_list.Count();
  if (relay_count == 0) {
    TRACE_SOURCE_LOCATION();
    //TODO 실질적으로는 오류임.  왜냐 릴레이를 시켜주어야하는데, 릴레이 대상이 지정되지 않았다는 것이니...
    return;
  }

  //Array<ReliableDestInfo,InlineAllocator<256>> dest_info_list(relay_count, NoInit);
  Array<ReliableDestInfo,InlineAllocator<256>> dest_info_list(relay_count);

  // RC를 얻어내야하기 때문에 main lock이 필요하다.
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  const HostId remote = rc->GetHostId();

  int32 add_count = 0;

  // 각 수신 대상에게 브로드캐스트한다.
  for (int32 i = 0; i < relay_count; ++i) {
    const auto& dst = relay_dest_list[i];

    ReliableDestInfo dest_info;
    dest_info.sendto_rc = GetAuthedClientByHostId_NOLOCK(dst.send_to);
    if (dest_info.sendto_rc) {
      dest_info.sendto_rc->IncreaseUseCount();
      dest_info.frame_number = dst.frame_number;
      dest_info.send_to = dst.send_to;
      dest_info_list[add_count] = dest_info;
      ++add_count;
    } else {
      //LOG(LogNetEngine,Warning,"cannot find out relay target. remote: %d", (int32)dst.send_to);
    }
  }

  // main lock unlock
  main_guard.Unlock();

  // 이하 전송 동작은 메인락 없는 상태에서 진행함.

  while (add_count > 0) {
#if USE_PARALLEL_FOR
#if (_MSC_VER >= 1400)
#pragma omp parallel for
#endif
#endif
    for (int32 dst_index = 0; dst_index < add_count; ++dst_index) {
      auto dst = dest_info_list[dst_index]; // 복사해야 안전한가??

      CScopedLock2 rc_tcp_send_queue_guard(dst.sendto_rc->to_client_tcp_->GetSendQueueMutex(), false);

      //LOG(LogNetEngine,Warning,"MessageType::ReliableRelay2: sender: %d, receiver: %d, frame: %d, payload_length: %d", (int32)remote, (int32)dst.sendto_id, (int32)dst.frame_number, payload_length);

      //TODO 프레임넘버만 달라서, 메시지를 재활용하지 못하고 있음.
      //이 부분에 대해서 고민이 필요해보임.
      MessageOut header;
      lf::Write(header, MessageType::ReliableRelay2);
      lf::Write(header, remote);
      lf::Write(header, dst.frame_number);
      lf::Write(header, OptimalCounter32(payload_length));

      SendFragRefs data_to_send;
      data_to_send.Add(header);
      data_to_send.Add(payload);

      if (dst_index != 0) { //DestIndex가 0이 아닌 경우가 더 많을테니 0이 아닌 경우를 먼저 체크함.
        if (rc_tcp_send_queue_guard.TryLock()) {
          dst.sendto_rc->to_client_tcp_->SendWhenReady(data_to_send, TcpSendOption());
          rc_tcp_send_queue_guard.Unlock();

          dst.sendto_rc->DecreaseUseCount();
          dest_info_list[dst_index].sendto_rc = nullptr;
          dest_info_list[dst_index] = dest_info_list[--add_count];
        }
      } else {
        rc_tcp_send_queue_guard.Lock();
        dst.sendto_rc->to_client_tcp_->SendWhenReady(data_to_send, TcpSendOption());
        rc_tcp_send_queue_guard.Unlock();

        dst.sendto_rc->DecreaseUseCount();
        dest_info_list[dst_index].sendto_rc = nullptr;
        dest_info_list[dst_index] = dest_info_list[--add_count];
      }
    }
  }
}

// Client -> peer
//    Client -> LingerDataFrame1 -> Server -> LingerDataFrame2 -> peer
void NetServerImpl::IoCompletion_ProcessMessage_LingerDataFrame1(MessageIn& msg, RemoteClient_S* rc) {
  AssertIsNotLockedByCurrentThread();
  rc->AssertIsZeroUseCount();

  // Unpack parameters
  HostId dest_remote_host_id;
  FrameNumber frame_number;
  OptimalCounter32 frame_length;
  if (!lf::Reads(msg,  dest_remote_host_id, frame_number, frame_length)) {
    TRACE_SOURCE_LOCATION();
    return;
  }

  if (frame_length < 0 || frame_length >= settings_.message_max_length) {
    TRACE_SOURCE_LOCATION();
    EnqueueHackSuspectEvent(rc, __FUNCTION__, HackType::PacketRig);
    return;
  }

  ASSERT_OR_HACKED(msg.GetReadableLength() == frame_length);
  MessageIn frame_data;
  if (!msg.ReadAsShared(frame_data, frame_length)) {
    TRACE_SOURCE_LOCATION();
    EnqueueHackSuspectEvent(rc, __FUNCTION__, HackType::PacketRig);
    return;
  }

  fun_check(frame_data.GetLength() == frame_length);
  fun_check(frame_data.GetReadableLength() == frame_length);

  // RC를 얻어내야 하기때문에 MainLock이 필요하다.
  rc->AssertIsNotLockedByCurrentThread();
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  const HostId remote = rc->GetHostId();
  auto dest_conn = GetAuthedClientByHostId_NOLOCK(dest_remote_host_id);
  if (dest_conn == nullptr) {
    // 패킷이 밀려서 뒤늦게 이미 disconnected된 RP에게 오는 경우임.
    // 뭐 딱히 로그를 남길 이유는 없어보임.
    TRACE_SOURCE_LOCATION();
    return; //TODO logging
  }

  main_guard.Unlock();

  // ReliableUDP 프레임 전송중 Relay로 전환된 상태에서, 해당 피어에게 릴레이 시켜주는 것이므로,
  // 목적지 피어에게 TCP를 통해서 보내줌.
  MessageOut header;
  lf::Write(header, MessageType::LingerDataFrame2);
  lf::Write(header, remote);
  lf::Write(header, frame_number);
  lf::Write(header, OptimalCounter32(frame_length));

  SendFragRefs data_to_send;
  data_to_send.Add(header);
  data_to_send.Add(frame_data);

  {
    CScopedLock2 rc_tcp_send_queue_guard(dest_conn->to_client_tcp_->GetSendQueueMutex());
    dest_conn->to_client_tcp_->SendWhenReady(data_to_send, TcpSendOption());
  }
}

// Remind the client that the hole punching with the server was successful.
void NetServerImpl::IoCompletion_ProcessMessage_NotifyHolepunchSuccess(MessageIn& msg, RemoteClient_S* rc) {
  AssertIsNotLockedByCurrentThread();
  rc->AssertIsZeroUseCount();

  // Unpack parameters
  Uuid tag;
  InetAddress client_local_addr;
  InetAddress client_addr_from_here;
  if (!lf::Reads(msg,  tag, client_local_addr, client_addr_from_here)) {
    TRACE_SOURCE_LOCATION();
    return;
  }

  rc->AssertIsNotLockedByCurrentThread();
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  // Find relevant mature or unmature client by magic number.
  if (rc->to_client_udp_fallbackable_.holepunch_tag_ == tag &&
      rc->to_client_udp_fallbackable_.real_udp_enabled_ == false) {
    // Associate remote UDP address with matured or unmatured client.
    rc->to_client_udp_fallbackable_.real_udp_enabled_ = true;
    rc->last_udp_packet_recv_time_ = GetAbsoluteTime();
    rc->last_udp_ping_recv_time_ = GetAbsoluteTime();

    rc->to_client_udp_fallbackable_.SetUdpAddrFromHere(client_addr_from_here);
    rc->to_client_udp_fallbackable_.udp_addr_internal_ = client_local_addr;

    P2PGroup_RefreshMostSuperPeerSuitableClientId(rc);

    // Send UDP matched via TCP.
    MessageOut msg_to_send;
    lf::Write(msg_to_send, MessageType::NotifyClientServerUdpMatched);
    lf::Write(msg_to_send, rc->to_client_udp_fallbackable_.holepunch_tag_);

    {
      CScopedLock2 rc_tcp_send_queue_guard(rc->to_client_tcp_->GetSendQueueMutex());
      rc->to_client_tcp_->SendWhenReady(SendFragRefs(msg_to_send), TcpSendOption());
    }

    if (intra_logger_) {
      const String text = String::Format("Client %d succeeded in hole punching to the server.  client_local_addr: %s, client_addr_at_server: %s",
                      (int32)rc->host_id_, *client_local_addr.ToString(), *client_addr_from_here.ToString());
      intra_logger_->WriteLine(LogCategory::SP2P, *text);
    }
  }
}

void NetServerImpl::IoCompletion_ProcessMessage_PeerUdp_NotifyHolepunchSuccess(MessageIn& msg, RemoteClient_S* rc) {
  AssertIsNotLockedByCurrentThread();
  rc->AssertIsZeroUseCount();

  // Unpack parameters
  InetAddress client_local_addr;
  InetAddress client_addr_from_here;
  HostId peer_id;
  if (!lf::Reads(msg,  client_local_addr, client_addr_from_here, peer_id)) {
    TRACE_SOURCE_LOCATION();
    return;
  }

  rc->AssertIsNotLockedByCurrentThread();
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  auto rc2 = GetAuthedClientByHostId_NOLOCK(peer_id);
  if (rc2 == nullptr) {
    TRACE_SOURCE_LOCATION();
    return;
  }

  // 쌍방이 모두 홀펀칭 OK가 와야만 쌍방에게 P2P 시도를 지시.
  auto p2p_conn_state = p2p_connection_pair_list_.GetPair(peer_id, rc->host_id_);

  // 늦장 도착일 경우가 있다...가령 예를 들면, ...
  // s->c로 leave, join을 연달아 보내는 경우...
  // 클라에서는 미처 leave, join을 처리 하지 못한 상태에서
  // 보낸 메시지 일수 있다는 이야기.
  if (!p2p_conn_state || p2p_conn_state->member_join_acked_start_) {
    return;
  }

  if (!p2p_conn_state->GetRelayed()) {
    return;
  }

  const int32 holepunch_ok_count_old = p2p_conn_state->GetServerHolepunchOkCount();

  p2p_conn_state->SetServerHolepunchOk(rc->host_id_, client_local_addr, client_addr_from_here);
  if (p2p_conn_state->GetServerHolepunchOkCount() != 2 || holepunch_ok_count_old != 1) {
    //LOG(LogNetEngine,Trace, "#### %d %d ####", p2p_conn_state->GetServerHolepunchOkCount(), holepunch_ok_count_old);
    return;
  }

  fun_check(p2p_conn_state->holepunch_tag_ != Uuid::None);

  // send to rc
  s2c_proxy_.RequestP2PHolepunch(
        rc->host_id_,
        GReliableSend_INTERNAL,
        rc2->host_id_,
        p2p_conn_state->GetInternalAddr(rc2->host_id_),
        p2p_conn_state->GetExternalAddr(rc2->host_id_));

  // send to rc2
  s2c_proxy_.RequestP2PHolepunch(
        rc2->host_id_,
        GReliableSend_INTERNAL,
        rc->host_id_,
        p2p_conn_state->GetInternalAddr(rc->host_id_),
        p2p_conn_state->GetExternalAddr(rc->host_id_));

  if (intra_logger_) {
    const String text = String::Format("Completed first step for hole punching between client %d(external_addr: %s) and client %d(external_addr: %s).",
                  (int32)rc->host_id_,
                  *p2p_conn_state->GetExternalAddr(rc->host_id_).ToString(),
                  (int32)rc2->host_id_,
                  *p2p_conn_state->GetExternalAddr(rc2->host_id_).ToString());
    intra_logger_->WriteLine(LogCategory::PP2P, *text);
  }
}

void NetServerImpl::IoCompletion_ProcessMessage_RPC(
      ReceivedMessage& received_msg,
      bool msg_processed,
      RemoteClient_S* rc,
      bool is_real_udp) {
  auto& payload = received_msg.unsafe_message;
  const int32 saved_read_pos = payload.Tell();
  fun_check(saved_read_pos == 1); // MessageType::RPC 다음이므로..

  AssertIsNotLockedByCurrentThread();
  rc->AssertIsZeroUseCount();

  // RC에 락이 걸린 상태에서 메인락을 다시 걸게 되면, deadlock을 유발하게 된다.
  rc->AssertIsNotLockedByCurrentThread();
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  // 일단은, 내부 RPC 메시지 일 수 있으므로, C2SStub에게 먼저 처리할 기회를 줘보고,
  // 처리가 되지 않는 다면(관심없는) 유저 RPC Stub쪽에 기회를 주도록 한다.
  msg_processed |= c2s_stub_.ProcessReceivedMessage(received_msg, rc->host_tag_);

  // 내부 RPC C2SStub에서 처리되지 않았으므로, 유저 RPC Sutb쪽으로 넘겨주도록 한다.
  if (!msg_processed) {
    // c2s_stub_.ProcessReceivedMessage에서 처리가 안되었을 경우에도, 읽기 위치가 변경되었을 수 있으므로,
    // 저장된 위치로 복원해주어야함.
    // 그러나, 실제로는 c2s_stub_.ProcessReceivedMessage에서 처리가 안되면, 내부적으로 읽기 위치를 원래대로
    // 복원해주므로 이중으로 복원해주는 꼴이다.
    // 만약을 위해서 일단은 유지하도록 하자.
    payload.Seek(saved_read_pos);

    ReceivedMessage rpc_msg;
    rpc_msg.unsafe_message = payload; // share
    rpc_msg.relayed = received_msg.relayed;
    rpc_msg.remote_addr_udp_only = received_msg.remote_addr_udp_only;
    rpc_msg.remote_id = received_msg.remote_id;

    UserTaskQueue_Add(rc, rpc_msg, FinalUserWorkItemType::RPC, is_real_udp);
  }
}

void NetServerImpl::IoCompletion_ProcessMessage_FreeformMessage(
    ReceivedMessage& received_msg, bool msg_processed, RemoteClient_S* rc, bool is_real_udp) {
  AssertIsNotLockedByCurrentThread();
  rc->AssertIsZeroUseCount();
  rc->AssertIsNotLockedByCurrentThread();

  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  // 이거 치우거나 MainLock아래로 내리지 말것...no lock CReceivedMessage파괴되면서 refcount꼬임.
  //TODO 딱히 블럭을 잡아줄 필요가 없어보이는데??
  {
    auto& payload = received_msg.unsafe_message;

    ReceivedMessage user_msg;
    user_msg.unsafe_message = payload; // share
    user_msg.relayed = received_msg.relayed;
    user_msg.remote_addr_udp_only = received_msg.remote_addr_udp_only;
    user_msg.remote_id = received_msg.remote_id;

    UserTaskQueue_Add(rc, user_msg, FinalUserWorkItemType::FreeformMessage, is_real_udp);
  }
}

void NetServerImpl::IoCompletion_ProcessMessage_RequestReceiveSpeedAtReceiverSide_NoRelay(MessageIn& msg, RemoteClient_S* rc) {
  AssertIsNotLockedByCurrentThread();

  if (rc == nullptr) {
    return;
  }

  rc->AssertIsZeroUseCount();
  rc->AssertIsNotLockedByCurrentThread();

  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (rc->to_client_udp_fallbackable_.udp_socket) {
    double speed = 0;
    {
      CScopedLock2 udp_fragger_guard(rc->to_client_udp_fallbackable_.udp_socket_->GetFraggerMutex());
      speed = rc->to_client_udp_fallbackable_.udp_socket_->packet_defragger_->GetRecentReceiveSpeed(rc->to_client_udp_fallbackable_.GetUdpAddrFromHere());
    }

    if (speed > 0) {
      // 집계됐던 UDP 수신속도를 리플한다.
      MessageOut msg_to_send;
      lf::Write(msg_to_send, MessageType::ReplyReceiveSpeedAtReceiverSide_NoRelay);
      lf::Write(msg_to_send, speed);

      const UdpSendOption send_opt(MessagePriority::Ring1, EngineOnlyFeature);
      rc->to_client_udp_fallbackable_.SendWhenReady(rc->GetHostId(), SendFragRefs(msg_to_send), send_opt);
    }
  }
}

void NetServerImpl::IoCompletion_ProcessMessage_ReplyReceiveSpeedAtReceiverSide_NoRelay(
          ReceivedMessage& received_msg, RemoteClient_S* rc) {
  AssertIsNotLockedByCurrentThread();

  if (rc == nullptr) {
    return;
  }

  //TODO 패킷이 깨져서 온 경우, rc->to_client_udp_fallbackable_.udp_socket 객체가 null일 경우
  //서버가 크래쉬 될 수 있음.
  //밑에 부분에서 해당 객체를 접근하고 있으므로, access-violation이 발생함.
  //
  //TODO 이런 상황이 더 있을터이니, 전반적으로 방어적으로 처리를 해야할듯!!
  if (!rc->to_client_udp_fallbackable_.udp_socket_.IsValid()) {
    TRACE_SOURCE_LOCATION();
    return;
  }

  auto& msg = received_msg.unsafe_message;

  double recv_speed;
  if (!lf::Read(msg, recv_speed)) {
    return;
  }

  // 받은 '수신속도'를 저장
  rc->to_client_udp_fallbackable_.udp_socket_->AssertIsNotLockedByCurrentThread();
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  ScopedUseCounter counter(*(rc->to_client_udp_fallbackable_.udp_socket_));
  CScopedLock2 udp_fragger_guard(rc->to_client_udp_fallbackable_.udp_socket_->GetFraggerMutex());
  rc->to_client_udp_fallbackable_.udp_socket_->packet_fragger_->SetReceiveSpeedAtReceiverSide(rc->to_client_udp_fallbackable_.GetUdpAddrFromHere(), recv_speed);
}

void NetServerImpl::UserTaskQueue_Add(
      RemoteClient_S* rc,
      ReceivedMessage& received_msg,
      FinalUserWorkItemType type,
      bool is_real_udp) {
  AssertIsLockedByCurrentThread();

  if (rc == nullptr) {
    return;
  }

  if (is_real_udp) { // Real UDP 일경우 클라에게 받은 UDP packet 갯수를 센다.
    rc->to_server_send_udp_message_success_count++;
  }

  rc->final_user_work_queue_.Enqueue(FinalUserWorkItem_S(received_msg.unsafe_message, type));
  user_task_queue_.AddTaskSubject(rc);
}

void NetServerImpl::NotifyProtocolVersionMismatch(RemoteClient_S* rc) {
  AssertIsLockedByCurrentThread();

  if (rc == nullptr) {
    return;
  }

  MessageOut msg_to_send;
  lf::Write(msg_to_send, MessageType::NotifyProtocolVersionMismatch);
  {
    CScopedLock2 rc_tcp_send_queue_guard(rc->to_client_tcp_->GetSendQueueMutex());
    rc->to_client_tcp_->SendWhenReady(SendFragRefs(msg_to_send), TcpSendOption());
  }
}

void NetServerImpl::IoCompletion_UdpConnectee(CompletionStatus& completion, ReceivedMessageList& msg_list) {
  if (auto udp_socket = LeanDynamicCast(completion.completion_context)) {
    if (completion.type == CompletionType::Receive) {
      IoCompletion_UdpRecvCompletionCase(completion, udp_socket, msg_list);
    } else if (completion.type == CompletionType::Send) {
      IoCompletion_UdpSendCompletionCase(completion, udp_socket);
    }
  }
}

void NetServerImpl::IoCompletion_UdpSendCompletionCase(
      CompletionStatus& completion, UdpSocket_S* udp_socket) {
  // 송신 큐에서 완료된 만큼의 데이터를 제거한다. 그리고 다음 송신을 건다.
  AssertIsNotLockedByCurrentThread();

  if (udp_socket == nullptr) {
    return;
  }

  ScopedUseCounter counter(*udp_socket);
  CScopedLock2 udp_socket_guard(udp_socket->GetMutex());

  // 반드시 이걸 콜 하도록 하자.
  // 메인 락을 사용하지 않아도 문제 없어보임.wsasend에서 사용을 하는데...
  // wsasend자체가 udpsocket의 lock로 보호되며, 1번 wsasend에 1번 completion이 올것이므로,
  udp_socket->socket_->RestoreTtlOnCompletion();

  if (completion.completed_length > 0) {
    total_udp_send_count_++;
    total_udp_send_bytes_ += completion.completed_length;
  }

  udp_socket->send_issued_ = false;
  udp_socket->ConditionalIssueSend();
}

//TODO message_list를 인자로 넘겨진것을 사용해야할지? 로컬을 잡아서 처리해야할지?
void NetServerImpl::IoCompletion_UdpRecvCompletionCase(
      CompletionStatus& completion, UdpSocket_S* udp_socket, ReceivedMessageList& MessageList00) {
  AssertIsNotLockedByCurrentThread();

  if (udp_socket == nullptr) {
    return;
  }

  //TODO 임시로 잡아둠...
  ReceivedMessageList msg_list;

  //이걸 하고, recvissued를 false해야 안전하다.
  ScopedUseCounter counter(*udp_socket);

  // Length > 0이지만 errorCode가 있는 경우가 있을 수 있다.
  // 따라서 length > 0인 경우는 에러 코드를 무시해야 한다.
  if (completion.completed_length > 0) {
    assembled_packet assembled_packet;
    auto assembling_result = UdpPacketDefragger::AssembledPacketError::Ok;
    RemoteClient_S* rc = nullptr;
    InetAddress udp_addr_from_here;
    int32 message_max_length = 0;
    HostId src_host_id = HostId_None;
    double absolute_time = 0;
    String out_error;

    {
      CScopedLock2 main_guard(main_mutex_);
      CheckCriticalSectionDeadLock(__FUNCTION__);

      absolute_time = GetAbsoluteTime();
      rc = GetRemoteClientByUdpEndPoint_NOLOCK(completion.recvfrom_addr);
      src_host_id = GetSrcHostIdByAddrAtDestSide_NOLOCK(completion.recvfrom_addr);

      message_max_length = settings_.message_max_length;

      // 이게 들어가야 안전하게 remote처리.
      // 왜냐하면 udpsocket이기 때문이다.
      if (rc) {
        rc->IncreaseUseCount();
        udp_addr_from_here = rc->to_client_udp_fallbackable_.GetUdpAddrFromHere();
      }
    }

    {
      CScopedLock2 udp_socket_guard(udp_socket->mutex_);

      assembling_result = udp_socket->packet_defragger_->PushFragmentAndPopAssembledPacket(
              udp_socket->socket_->GetRecvBufferPtr(),
              completion.completed_length,
              completion.recvfrom_addr,
              src_host_id,
              absolute_time,
              assembled_packet,
              out_error);
    }

    if (assembling_result == UdpPacketDefragger::AssembledPacketError::Ok) {
      if (rc) {
        try {
          // Update stats
          total_udp_recv_count++;
          total_udp_recv_bytes += assembled_packet.Len();

          // pong 체크를 했다고 처리하도록 하자.
          // 이게 없으면 대량 통신시 pong 수신 지연으로 인한 튕김이 발생하니까.
          rc->last_udp_packet_recv_time_ = GetAbsoluteTime();
          rc->last_udp_ping_recv_time_ = GetAbsoluteTime();

          // 완전한 msg가 도착한 것들을 모두 추려서 final recv queue로 옮기거나 여기서 처리한다.
          msg_list.Reset(); // keep capacity

          const uint8* packet_data = (const uint8*)assembled_packet.ConstData();
          const int32 packet_len = assembled_packet.Len();
          rc->ExtractMessagesFromUdpRecvQueue(packet_data, packet_len, udp_addr_from_here_, message_max_length, msg_list);

          IoCompletion_ProcessMessageOrMoveToFinalRecvQueue(rc, msg_list, udp_socket);
        } catch (Exception& e) {
          CatchThreadExceptionAndPurgeClient(rc, __FUNCTION__, *String::Format("Exception(%s)", *e.Message()));
        } catch (std::exception& e) {
          CatchThreadExceptionAndPurgeClient(rc, __FUNCTION__, *String::Format("std::exception(%s)", UTF8_TO_TCHAR(e.what())));
        } //catch (_com_error&) {
        //  CatchThreadExceptionAndPurgeClient(rc, __FUNCTION__, "_com_error");
        //} catch (void*) {
        //  CatchThreadExceptionAndPurgeClient(rc, __FUNCTION__, "void*");
        //} catch (...) { // 사용자 정의 루틴을 콜 하는 곳이 없으므로 주석화
        //  if (NetConfig::catch_unhandled_exception) {
        //    CatchThreadExceptionAndPurgeClient(rc, __FUNCTION__, "Unknown");
        //  }
        //  else {
        //    throw;
        //  }
        //}
      } else {
        // 아직 등록 안된 remote로부터 도착한거다. 단순 에코 등의 메시지일 수 있으므로 별도 처리한다.
        // coalesce를 감안해서 처리한다.
        //ReceivedMessageList extracted_msg_list;
        //msg_list.EmptyAndKeepCapacity();

        //warning: capacity는 리셋하면 안됨.. (성능상 문제가 있을 수 있음.)
        //이 콜렉션은 각 워커스레드의 로컬로 선언되어 있으므로, 재할당 이슈를
        //제거하려면, capacity를 리셋하면 안됨.
        msg_list.Reset();

        MessageStreamExtractor extractor;
        extractor.input = (const uint8*)assembled_packet.ConstData();
        extractor.input_length = assembled_packet.Len();
        extractor.output = &msg_list;
        extractor.message_max_length = message_max_length;
        extractor.sender_id = HostId_None;

        ResultCode extract_result;
        const int32 added_count = extractor.Extract(extract_result);
        if (added_count >= 0) {
          for (auto& received_msg : msg_list) {
            fun_check(received_msg.unsafe_message.AtBegin());
            IoCompletion_ProcessMessage_FromUnknownClient(assembled_packet.SenderAddr, received_msg.unsafe_message, udp_socket);
          }
        } else {
          // 잘못된 스트림 데이터이다. UDP인 경우에는 모두 처리된 것처럼 간주하고 그냥 무시해버린다.
        }
      }
    } else if (assembling_result == UdpPacketDefragger::AssembledPacketError::error) {
      CScopedLock2 main_guard(main_mutex_);
      CheckCriticalSectionDeadLock(__FUNCTION__);

      EnqueuePacketDefragWarning(completion.recvfrom_addr, *out_error);
    }

    if (rc) {
      // 모든 처리를 하고 decrease를 한다.
      rc->DecreaseUseCount();
    }

    // usecount는 위에서 하고 있다.
    // 다음 recv를 건다.
    CScopedLock2 udp_socket_guard(udp_socket->mutex_);
    udp_socket->recv_issued_ = false;
    udp_socket->ConditionalIssueRecvFrom();
  } else {
    // UDP 소켓을 닫은 경우가 아닌 이상 수신을 다시 건다.
    // WSAECONNRESET이 에러인 경우 아직 소켓은 건재하므로 계속 강행
    CScopedLock2 udp_socket_guard(udp_socket->mutex_);
    udp_socket->recv_issued_ = false;
    udp_socket->ConditionalIssueRecvFrom();
  }
}

bool NetServerImpl::IoCompletion_ProcessMessage_FromUnknownClient(
    const InetAddress& from, MessageIn& incomming_msg, UdpSocket_S* udp_socket) {
  AssertIsNotLockedByCurrentThread();

  if (udp_socket == nullptr) {
    return false;
  }

  const int32 saved_read_pos = incomming_msg.Tell();

  MessageType msg_type;
  if (!lf::Read(incomming_msg, msg_type)) {
    incomming_msg.Seek(saved_read_pos);
    return false;
  }

  bool msg_processed = false;
  switch (msg_type) {
    case MessageType::ServerHolepunch:
      IoCompletion_ProcessMessage_ServerHolepunch(incomming_msg, from, udp_socket);
      msg_processed = true;
      break;

    case MessageType::PeerUdp_ServerHolepunch:
      IoCompletion_ProcessMessage_PeerUdp_ServerHolepunch(incomming_msg, from, udp_socket);
      msg_processed = true;
      break;
  }

  if (!msg_processed) {
    incomming_msg.Seek(saved_read_pos);
    return false;
  }

  return true;
}

// caller는 이미 main 잠금을 only 1 recursion 상태로 이 함수를 콜 한 상태이어야 한다.
void NetServerImpl::IoCompletion_MulticastUnreliableRelay2_AndUnlock(
    CScopedLock2* main_mutex,
    const HostIdArray& relay_dest,
    HostId relay_sender_host_id,
    MessageIn& payload,
    MessagePriority priority,
    uint64 unique_id) {
  const int32 relay_count = relay_dest.Count();
  if (relay_count == 0) {
    return;
  }

  const OptimalCounter32 payload_length = payload.GetLength();

  Array<UnreliableDestInfo,InlineAllocator<256>> relay_dest_list(relay_count); // warning: must be initialized!

  // RC를 얻어내야하기 때문에 main lock이 필요하다.
  // 단, 콜러에서 이미 잠금을 한 상태이면(당연히 recursion count = 1이어야) 굳이 또 잠그지 않는다.
  // 이 함수의 하단에서 멀티코어를 사용하기 위해 잠금을 풀어야 하니까.

  fun_check(main_mutex->IsLocked());
  CheckCriticalSectionDeadLock(__FUNCTION__);

  // 각 dest들을 리스트에 추가한다.
  int32 add_count = 0;
  for (int32 relay_index = 0; relay_index < relay_count; ++relay_index) {
    const HostId dest_id = relay_dest[relay_index];

    UnreliableDestInfo dest_info;
    dest_info.sendto_id = dest_id;

    if (auto dest_conn = GetAuthedClientByHostId_NOLOCK(dest_id)) {
      RemoteClient_NewLocalUdpSocketAndRequestNewRemoteUdpSocket(dest_conn);

      if (dest_conn->to_client_udp_fallbackable_.real_udp_enabled_) {
        dest_conn->to_client_udp_fallbackable_.udp_socket_->IncreaseUseCount();

        dest_info.sendto_rc = dest_conn->to_client_udp_fallbackable_.udp_socket_.Get();
        dest_info.sendto_addr = dest_conn->to_client_udp_fallbackable_.GetUdpAddrFromHere();
      } else {
        dest_conn->IncreaseUseCount();
        dest_info.sendto_rc = dest_conn;
      }

      relay_dest_list[add_count++] = dest_info;
    }
  }

  // 잠금을 푼다.
  main_mutex->Unlock();
  // 이 스레드는 잠그고 있지 않음이 보장되어야 아래 멀티코어 멀티캐스트가 효력을 낸다.
  AssertIsNotLockedByCurrentThread();

  // packet setting.
  MessageOut header;
  lf::Write(header, MessageType::UnreliableRelay2);
  lf::Write(header, relay_sender_host_id);

  // (unreliable은 frame number가 불필요)
  lf::Write(header, OptimalCounter32(payload_length));

  //TODO 참조로 넘길 수 있다면 복사 비용을 대폭 줄일 수 있을터... CSendFragRefs는 전반적으로 고민을 해보아야할듯!!
  SendFragRefs data_to_send;
  data_to_send.Add(header);
  data_to_send.Add(payload);

  SendOption send_opt(GUnreliableSend_INTERNAL);
  send_opt.priority = priority;
  send_opt.unique_id = unique_id;
  send_opt.conditional_fragging = false;

  while (add_count > 0) {
#if USE_PARALLEL_FOR
#if (_MSC_VER >= 1400)
#pragma omp parallel for
#endif
#endif
    for (int32 dst_index = 0; dst_index < add_count; ++dst_index) {
      const auto& dst = relay_dest_list[dst_index];

      CScopedLock2 SendLock(dst.sendto_rc->GetSendMutex(), false);

      if (dst_index != 0) {
        if (SendLock.TryLock()) {
          dst.sendto_rc->SendWhenReady(dst.sendto_id, dst.sendto_addr, dst.sendto_id, data_to_send, send_opt);
          SendLock.Unlock();

          dst.sendto_rc->Decrease();
          relay_dest_list[dst_index].sendto_rc = nullptr;
          relay_dest_list[dst_index] = relay_dest_list[--add_count];
        }
      } else {
        SendLock.Lock();
        dst.sendto_rc->SendWhenReady(dst.sendto_id, dst.sendto_addr, dst.sendto_id, data_to_send, send_opt);
        SendLock.Unlock();

        dst.sendto_rc->Decrease();
        relay_dest_list[dst_index].sendto_rc = nullptr;
        relay_dest_list[dst_index] = relay_dest_list[--add_count];
      }
    }
  }
}

void NetServerImpl::CatchThreadExceptionAndPurgeClient(RemoteClient_S* rc, const char* where, const char* reason) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  if (rc) {
    if (callbacks_) {
      //TODO 메시지를 영문으로 수정하도록 하자.  오류 코드를 구체화할수 있어도 좋을듯...
      const String text = String::Format("%s에서 %s 이유로 클라이언트 %d를 추방합니다.", where, reason, (int32)rc->host_id_);
      EnqueueError(ResultInfo::From(ResultCode::Unexpected, rc->host_id_, text));
    }

    IssueDisposeRemoteClient(rc, ResultCode::Unexpected, ResultCode::TCPConnectFailure, ByteArray(), __FUNCTION__, SocketErrorCode::Ok);
  } else {
    if (callbacks_) {
      //TODO 메시지를 영문으로 수정하도록 하자.  오류 코드를 구체화할수 있어도 좋을듯...
      const String text = String::Format("%s에서 %s 오류가 발생했으나 클라이언트를 식별 불가.", where, reason);
      EnqueueError(ResultInfo::From(ResultCode::Unexpected, HostId_None, text));
    }
  }
}

void NetServerImpl::PostHeartbeatIssue() {
  // net에서 해야 한다. user작업이 오래 걸리면 끊긴다.
  net_thread_pool_->PostCompletionStatus(this, (UINT_PTR)IocpCustomValue::Heartbeat);
}

void NetServerImpl::OnIoCompletion( Array<IHostObject*>& send_issued_pool,
                                    ReceivedMessageList& msg_list,
                                    CompletionStatus& completion) {
  fun_check(completion.type == CompletionType::ReferCustomValue);

  switch ((IocpCustomValue)completion.custom_value) {
    case IocpCustomValue::Heartbeat:
      Heartbeat();
      break;

    case IocpCustomValue::SendEnqueued:
      EveryRemote_IssueSendOnNeed(send_issued_pool);
      //ConditionalEveryRemoteIssueSend(SendIssuePool);
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

void NetServerImpl::Heartbeat() {
// 사용자가 핸들링하지 않은 에러는 차라리 unhandled exception으로 이어지게 해서
// 미니덤프를 남기는 것이 conceal보다 훨씬 문제해결을 빨리 해낸다.
#if EXPOSE_CRASH_THAN_CONCEAL_EXCEPTION
  try {
#endif
    //스레드 풀에서 실행되므로, 여러개의 스레드가 접근할 수 있으므로...
    if (Atomics::CompareExchange(&heartbeat_working_, 1, 0) == 0) {
      heartbeat_tickable_timer_.Tick();

      ConditionalLogFreqFail();

      Atomics::Exchange(&heartbeat_working_, 0);
    }

// 사용자가 핸들링하지 않은 에러는 차라리 unhandled exception으로 이어지게 해서
// 미니덤프를 남기는 것이 conceal보다 훨씬 문제해결을 빨리 해낸다.
#if EXPOSE_CRASH_THAN_CONCEAL_EXCEPTION //예외를 숨기는것 보다는 차라리 노출하는게 좀더 유리하다?
  } catch (Exception& e) {
    CatchThreadUnexpectedExit(__FUNCTION__, String::Format("Exception(%s)", *e.Message()));
  } catch (std::exception& e) {
    CatchThreadUnexpectedExit(__FUNCTION__, String::Format("std.exception(%s)", UTF8_TO_TCHAR(e.what())));
  } //catch (_com_error& e) {
  //  CatchThreadUnexpectedExit(__FUNCTION__, String::Format("_com_error(%s)", (const char*)e.Description()));
  //} catch (void*) {
  //  CatchThreadUnexpectedExit(__FUNCTION__, "void*", true);
  //}
#endif
}

void NetServerImpl::EveryRemote_IssueSendOnNeed(Array<IHostObject*>& pool) {
  // 한 스레드만 작업하는것을 개런티
  AssertIsNotLockedByCurrentThread();

  CScopedLock2 tcp_issue_queue_guard(tcp_issue_queue_mutex_);
  CScopedLock2 udp_issue_queue_guard(udp_issue_queue_mutex_);

  //TODO 만약 보내기 요청 이슈가 대량이라면, 상당한 부담이 될수도?
  //그런데, 콜렉션 객체가 외부에서 인스턴스로 넘어오므로 문제는 없을듯 싶음.
  int32 total_parallel_count = udp_issued_send_ready_list_.Count() + tcp_issue_send_ready_remote_clients_.Count();

  //TODO Resize말고, expand만 해야할터??
  pool.ResizeUninitialized(total_parallel_count);

  int32 collected_parallel_count = 0;

  // 일단은 임시큐 두개로, 병렬 for을 한다. 하지만 1개 큐가 더나을수도 있으므로 수정여지는 있음.
  auto parallel_queue = pool.MutableData();
  while (true) {
    if (auto udp_socket = udp_issued_send_ready_list_.Front()) {
      //UDP 보내기 요청 이슈 목록에서 링크를 해제
      udp_socket->UnlinkSelf();
      udp_socket->IncreaseUseCount();
      parallel_queue[collected_parallel_count++] = udp_socket;
    } else {
      break;
    }
  }

  while (true) {
    if (auto conn = tcp_issue_send_ready_remote_clients_.Front()) {
      //TCP 보내기 요청 이슈 목록에서 링크를 해제
      conn->UnlinkSelf();
      conn->IncreaseUseCount();
      parallel_queue[collected_parallel_count++] = conn;
    } else {
      break;
    }
  }

  const double absolute_time = GetAbsoluteTime();
  fun_check(collected_parallel_count == total_parallel_count);

  udp_issue_queue_guard.Unlock();
  tcp_issue_queue_guard.Unlock();

  while (total_parallel_count > 0) {
#if USE_PARALLEL_FOR
#if (_MSC_VER >= 1400)
#pragma omp parallel for
#endif
#endif
    for (int32 object_index = 0; object_index < total_parallel_count; ++object_index) {
      auto object = parallel_queue[object_index];

      CScopedLock2 host_object_guard(object->GetMutex(), false);

      if (object_index != 0) {
        if (host_object_guard.TryLock()) {
          const SocketErrorCode socket_error = object->IssueSend(absolute_time);
          host_object_guard.Unlock();

          if (socket_error != SocketErrorCode::Ok) {
            CScopedLock2 main_guard(main_mutex_);
            CheckCriticalSectionDeadLock(__FUNCTION__);
            object->OnIssueSendFail(__FUNCTION__, socket_error);
          }

          object->Decrease();
          parallel_queue[object_index] = parallel_queue[--total_parallel_count];
        }
      } else {
        host_object_guard.Lock();
        const SocketErrorCode socket_error = object->IssueSend(absolute_time);
        host_object_guard.Unlock();

        if (socket_error != SocketErrorCode::Ok) {
          CScopedLock2 main_guard(main_mutex_);
          CheckCriticalSectionDeadLock(__FUNCTION__);
          object->OnIssueSendFail(__FUNCTION__, socket_error);
        }

        object->Decrease();
        parallel_queue[object_index] = parallel_queue[--total_parallel_count];
      }
    }
  }
}

void NetServerImpl::PostEveryRemote_IssueSend() {
  net_thread_pool_->PostCompletionStatus(this, (UINT_PTR)IocpCustomValue::SendEnqueued);
}

//TODO 사용자 정의 루틴을 호출하므로 막아주어야할듯 싶은데??
void NetServerImpl::OnTick() {
  if (callbacks_ /*&& !owner_->tear_down_*/) {
    // NOTE: 중복 실행되는 경우가 없도록 설계 되었으므로, 아래와 같은 처리를 할 필요가 없음.
    // 중복 호출이 안되도록 막아주어야함.
    if (Atomics::CompareExchange(&on_tick_working_, 1, 0) == 0) {
#ifdef ALLOW_CATCH_UNHANDLED_EVEN_FOR_USER_ROUTINE
      try {
#endif
        callbacks_->OnTick(timer_callback_context_);
#ifdef ALLOW_CATCH_UNHANDLED_EVEN_FOR_USER_ROUTINE
      } catch (...) { // 사용자 정의 루틴을 콜 하는 곳이 있으므로
        throw;
      }
#endif
    }

    Atomics::Exchange(&on_tick_working_, 0);
  }
}

void NetServerImpl::PostOnTick() {
  user_thread_pool_->PostCompletionStatus(this, (UINT_PTR)IocpCustomValue::OnTick);
}

void NetServerImpl::PostUserTask() {
  user_thread_pool_->PostCompletionStatus(this, (UINT_PTR)IocpCustomValue::DoUserTask);
}

void NetServerImpl::DoUserTask() {
  UserWorkerThreadCallbackContext context;
  FinalUserWorkItem uwi;

  bool running = false;
  do {
    void* host_tag = nullptr;
    {
      CScopedLock2 main_guard(main_mutex_);
      CheckCriticalSectionDeadLock(__FUNCTION__);

      running = user_task_queue_.PopAnyTaskNotRunningAndMarkAsRunning(uwi, &host_tag);
    }

    if (running) {
      if (callbacks_) {
        callbacks_->OnUserWorkerThreadCallbackBegin(&context);
      }

      switch (uwi.type) {
        case FinalUserWorkItemType::RPC:
          UserWork_FinalReceiveRPC(uwi, host_tag);
          break;

        case FinalUserWorkItemType::FreeformMessage:
          UserWork_FinalReceiveFreeformMessage(uwi, host_tag);
          break;

        // Async로 수행한다고 해도 블럭이 되는건 아닌지??
        // 동시에 다른 작업도 병행이 가능한건가?? 아무리 스레드 콜백이라고 해도 말이다..
        // 조금더 확인해보도록 하자.
        // 콜백을 받아서 처리하는건 아무래도 싱크를 맞추기가 쉽지 않다는 문제는 여전히 있다.

        case FinalUserWorkItemType::UserTask:
          UserWork_FinalUserTask(uwi, host_tag);
          break;

        default:
          UserWork_LocalEvent(uwi);
          break;
      }

      if (callbacks_ /*&& !owner_->tear_down_*/) {
        callbacks_->OnUserWorkerThreadCallbackEnd(&context);
      }
    }
  } while (running);
}

void NetServerImpl::UserWork_FinalReceiveRPC(FinalUserWorkItem& uwi, void* host_tag) {
  AssertIsNotLockedByCurrentThread();

  auto& content = uwi.unsafe_message.unsafe_message;
  const int32 saved_read_pos = content.Tell();

  // 원래는 위치가 0이었지만, 현재는 1임.  뭐 상관없지 싶기는한데...
  // MessageType를 읽은 다음 넘어오기 때문에 현재 위치는 0이 아닌 1이되어야함.

  if (saved_read_pos != 1) {
    TRACE_SOURCE_LOCATION();
    EnqueueHackSuspectEvent(nullptr, __FUNCTION__, HackType::PacketRig);
  }

  RpcId rpc_id = RpcId_None;
  if (!lf::Read(content, rpc_id)) {
    TRACE_SOURCE_LOCATION();
    return;
  }

  bool processed = false;
  const int32 stub_count = stubs_nolock_.Count();
  for (int32 stub_index = 0; stub_index < stub_count; ++stub_index) {
    auto stub = stubs_nolock_[stub_index];

    content.Seek(saved_read_pos);
    try {
      //if (!owner_->tear_down_) {
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
    } //catch (_com_error& e) {
    //  if (callbacks_/* && !owner_->tear_down_*/) {
    //    callbacks_->OnException(uwi.unsafe_message.remote_id, Exception(e));
    //  }
    //}
    //catch (void* e) {
    //  if (callbacks_/*&& !owner_->tear_down_*/) {
    //    callbacks_->OnException(uwi.unsafe_message.remote_id, Exception(e));
    //  }
    //}
#ifdef ALLOW_CATCH_UNHANDLED_EVEN_FOR_USER_ROUTINE
    catch (...) {
      if (owner_->callbacks_ /*&& !owner_->tear_down_*/) {
        Exception e;
        e.exception_type = ExceptionType::Unhandled;
        owner_->callbacks_->OnException(uwi.unsafe_message.remote_id, e);
      }
    }
#endif
  }

  if (!processed) {
    content.Seek(saved_read_pos);

    if (callbacks_/*&& !owner_->tear_down_*/) {
      callbacks_->OnNoRpcProcessed(rpc_id);
    }
  }

  user_task_queue_.SetTaskRunningFlagByHostId(uwi.unsafe_message.remote_id, false);
}

void NetServerImpl::UserWork_FinalReceiveFreeformMessage(FinalUserWorkItem& uwi, void* host_tag) {
  AssertIsNotLockedByCurrentThread();

  auto& msg = uwi.unsafe_message.unsafe_message;
  const int32 saved_read_pos = msg.Tell();

  //if (saved_read_pos != 0) {
  //  EnqueueHackSuspectEvent(nullptr, __FUNCTION__, HackType::PacketRig);
  //}

  //CReaderLock_NORECURSE Lock(owner_->CallbackMon, true);

  if (callbacks_ /*&& !owner_->tear_down_*/) {
    RpcHint rpc_hint;
    rpc_hint.relayed = uwi.unsafe_message.relayed;
    rpc_hint.host_tag = host_tag;

    try {
      OptimalCounter32 payload_length;
      if (!lf::Read(msg, payload_length) || payload_length != msg.GetReadableLength()) { // 뒷쪽에 더 추가가 될수도 있남??
      //if (!lf::Read(msg, payload_length) || !msg.CanRead(payload_length)) { // 뒷쪽에 더 추가가 될수도 있남??
        SharedPtr<ResultInfo> result_info(new ResultInfo);
        result_info->result_code = ResultCode::InvalidPacketFormat;
        result_info->comment = "invalid payload-length in user-message.";
        EnqueueError(result_info);
      } else {
        // 바로 접근하고 버려질것이므로, 복사없이 raw형태로 넘겨준다.
        // 이부분은 다시 한번 신중한 검토가 필요할 수도 있을듯 싶다.
        ByteArray payload = ByteArray::FromRawData((const char*)msg.GetReadableData(), msg.GetReadableLength());
        callbacks_->OnReceiveFreeform(uwi.unsafe_message.remote_id, rpc_hint, payload);
      }
    } catch (Exception& e) {
      if (callbacks_ /*&& !owner_->tear_down_*/) {
        callbacks_->OnException(uwi.unsafe_message.remote_id, e);
      }
    } catch (std::exception& e) {
      if (callbacks_ /*&& !owner_->tear_down_*/) {
        callbacks_->OnException(uwi.unsafe_message.remote_id, Exception(e));
      }
    } //catch (_com_error& e) {
    //  if (callbacks_ /*&& !owner_->tear_down_*/) {
    //    callbacks_->OnException(uwi.unsafe_message.remote_id, Exception(e));
    //  }
    //} catch (void* e) {
    //  if (callbacks_/*&& !owner_->tear_down_*/) {
    //    callbacks_->OnException(uwi.unsafe_message.remote_id, Exception(e));
    //  }
    //}
#ifdef ALLOW_CATCH_UNHANDLED_EVEN_FOR_USER_ROUTINE
    catch (...) {
      if (owner_->callbacks_ /*&& !owner_->tear_down_*/) {
        Exception e;
        e.exception_type = ExceptionType::Unhandled;
        owner_->callbacks_->OnException(uwi.unsafe_message.remote_id, e);
      }
    }
#endif
  }

  user_task_queue_.SetTaskRunningFlagByHostId(uwi.unsafe_message.remote_id, false);
}

void NetServerImpl::UserWork_FinalUserTask(FinalUserWorkItem& uwi, void* host_tag) {
  AssertIsNotLockedByCurrentThread();

  //CReaderLock_NORECURSE Lock(owner_->CallbackMon, true);

  if (callbacks_ /*&& !owner_->tear_down_*/) {
    try {
      // Call user function.
      uwi.func();
    } catch (Exception& e) {
      if (callbacks_ /*&& !owner_->tear_down_*/) {
        callbacks_->OnException(uwi.unsafe_message.remote_id, e);
      }
    } catch (std::exception& e) {
      if (callbacks_ /*&& !owner_->tear_down_*/) {
        callbacks_->OnException(uwi.unsafe_message.remote_id, Exception(e));
      }
    } //catch (_com_error& e) {
    //  if (callbacks_ /*&& !owner_->tear_down_*/) {
    //    callbacks_->OnException(uwi.unsafe_message.remote_id, Exception(e));
    //  }
    //} catch (void* e) {
    //  if (callbacks_/*&& !owner_->tear_down_*/) {
    //    callbacks_->OnException(uwi.unsafe_message.remote_id, Exception(e));
    //  }
    //}
#ifdef ALLOW_CATCH_UNHANDLED_EVEN_FOR_USER_ROUTINE
    catch (...) {
      if (owner_->callbacks_ /*&& !owner_->tear_down_*/) {
        Exception e;
        e.exception_type = ExceptionType::Unhandled;
        owner_->callbacks_->OnException(uwi.unsafe_message.remote_id, e);
      }
    }
#endif
  }

  user_task_queue_.SetTaskRunningFlagByHostId(uwi.unsafe_message.remote_id, false);
}

void NetServerImpl::UserWork_LocalEvent(FinalUserWorkItem& uwi) {
  ProcessOneLocalEvent(uwi.event);

  user_task_queue_.SetTaskRunningFlagByHostId(uwi.unsafe_message.remote_id, false);
}

void NetServerImpl::EndCompletion() {
  if (net_thread_pool_->IsCurrentThread()) { //@maxidea: net_thread_pool_->IsCurrentThread()가 true여야, net_thread_pool_() 내에서 EndCompletion이 일어남.
    net_thread_pool_->UnregisterReferer(this);
    net_thread_pool_unregisted_ = true;
  }

  if (user_thread_pool_->IsCurrentThread()) { //@maxidea: user_thread_pool_->IsCurrentThread()가 true여야, net_thread_pool_() 내에서 EndCompletion이 일어남.
    user_thread_pool_->UnregisterReferer(this);
    user_thread_pool_unregisted_ = true;
  }
}

void NetServerImpl::OnThreadBegin() {
  if (callbacks_) {
    callbacks_->OnUserWorkerThreadBegin();
  }
}

void NetServerImpl::OnThreadEnd() {
  if (callbacks_) {
    callbacks_->OnUserWorkerThreadEnd();
  }
}

bool NetServerImpl::RunAsync(HostId task_owner_id, Function<void()> func) {
  CScopedLock2 main_guard(main_mutex_);
  CheckCriticalSectionDeadLock(__FUNCTION__);

  bool executed = false;

  //TODO 그룹일 경우에는 처리가 안되는군... 가능하게 할 수 있지 않을까??

  // 각 커넥션별로 큐가 있으므로, 해당 커넥션의 큐에 넣어준다면 순서는 지켜지게 된다.
  if (auto conn = GetRemoteClientByHostId_NOLOCK(task_owner_id)) {
    conn->EnqueueUserTask(func);
    user_task_queue_.AddTaskSubject(conn);
    executed = true;
  } else { // RC가 아닌 경우에는 서버자체의 태스크큐에 넣어줌.
    if (listener_) { // 정상적으로 동작하고 있는 경우에만 요청함.
      final_user_work_queue_.Enqueue(func);
      user_task_queue_.AddTaskSubject(this);
      executed = true;
    }
  }

  return executed;
}


//
// NetServerStats
//

NetServerStats::NetServerStats() {
  Reset();
}

void NetServerStats::Reset() {
  p2p_group_count = 0;
  p2p_connection_pair_count = 0;
  p2p_direct_connection_pair_count = 0;
  total_tcp_recv_bytes = 0;
  total_tcp_recv_count = 0;
  total_tcp_send_bytes = 0;
  total_tcp_send_count = 0;
  total_udp_recv_bytes = 0;
  total_udp_recv_count = 0;
  total_udp_send_bytes = 0;
  total_udp_send_count = 0;
  client_count = 0;
  real_udp_enabled_client_count = 0;
  occupied_udp_port_count = 0;
}

String NetServerStats::ToString() const {
  String ret;
  /*
  ret << "p2p_group_count=" << ToString(p2p_group_count);
  ret << ", p2p_connection_pair_count=" << ToString(p2p_connection_pair_count);
  ret << ", p2p_direct_connection_pair_count=" << ToString(p2p_direct_connection_pair_count);
  ret << ", total_tcp_recv_bytes=" << ToString(total_tcp_recv_bytes);
  ret << ", total_tcp_recv_count=" << ToString(total_tcp_recv_count);
  ret << ", total_tcp_send_bytes=" << ToString(total_tcp_send_bytes);
  ret << ", total_tcp_send_count=" << ToString(total_tcp_send_count);
  ret << ", total_udp_recv_bytes=" << ToString(total_udp_recv_bytes);
  ret << ", total_udp_recv_count=" << ToString(total_udp_recv_count);
  ret << ", total_udp_send_bytes=" << ToString(total_udp_send_bytes);
  ret << ", total_udp_send_count=" << ToString(total_udp_send_count);
  ret << ", client_count=" << ToString(client_count);
  ret << ", real_udp_enabled_client_count=" << ToString(real_udp_enabled_client_count);
  ret << ", occupied_udp_port_count=" << ToString(occupied_udp_port_count);
  */
  ret << "{";
  ret << "\"client_count\": " << ToString(client_count);
  ret << ", \"total_tcp_recv_bytes\": " << ToString(total_tcp_recv_bytes);
  ret << ", \"total_tcp_recv_count\": " << ToString(total_tcp_recv_count);
  ret << ", \"total_tcp_sent_bytes\": " << ToString(total_tcp_send_bytes);
  ret << ", \"total_tcp_sent_count\": " << ToString(total_tcp_send_count);
  ret << ", \"total_udp_recv_bytes\": " << ToString(total_udp_recv_bytes);
  ret << ", \"total_udp_recv_count\": " << ToString(total_udp_recv_count);
  ret << ", \"total_udp_sent_bytes\": " << ToString(total_udp_send_bytes);
  ret << ", \"total_udp_sent_count\": " << ToString(total_udp_send_count);
  ret << ", \"real_udp_enabled_client_count\": " << ToString(real_udp_enabled_client_count);
  ret << ", \"occupied_udp_port_count\": " << ToString(occupied_udp_port_count);
  ret << ", \"p2p_group_count\": " << ToString(p2p_group_count);
  ret << ", \"p2p_connection_pair_count\": " << ToString(p2p_connection_pair_count);
  ret << ", \"p2p_direct_connection_pair_count\": " << ToString(p2p_direct_connection_pair_count);
  ret << "}";
  return ret;
}

} // namespace net
} // namespace fun
