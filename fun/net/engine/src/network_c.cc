#include "fun/net/net.h"

#include "Networker_C.h"
#include "NetClient.h"

#include "ReportError.h"

//TODO
//#include "Apps/viz_agent_.h"

#include "PacketFrag.h"
#include "UdpSocket_C.h"

#include "RUdp.h"
#include "RUdpFrame.h"
#include "RUdpHelper.h"

#define TRACE_NETCLIENT_MESSAGES  0

namespace fun {
namespace net {

using lf = LiteFormat;

void NetClientWorker::ProcessEveryMessageOrMoveToFinalRecvQueue(UdpSocket_C* udp_socket) {
  owner_->LockMain_AssertIsLockedByCurrentThread();

  auto& queue = owner_->pre_final_recv_queue_;
  while (!queue.IsEmpty()) {
    auto& received_msg = queue.Front();

    // 시뮬 랙에 의해 당장 시행할 메시지가 아니면 그만 처리.
    //if (received_msg.action_time > owner_->GetAbsoluteTime()) {
    //  break;
    //}

    ProcessMessageOrMoveToFinalRecvQueue(udp_socket, received_msg);

    queue.RemoveFront();
  }
}

NetClientWorker::NetClientWorker(NetClientImpl* owner) {
  owner_ = owner;

  shutdown_issued_time_ = 0;
  disconnecting_mode_heartbeat_count_ = 0;
  disconnecting_mode_start_time_ = 0;
  disconnecting_mode_warned_ = false;

  state_ = State::IssueConnect;

  owner_->manager_->Register(this);
}

NetClientWorker::~NetClientWorker() {
  const bool is_disconnected = (GetState() == NetClientWorker::State::Disconnected);
  const bool is_unregistered = (!owner_->manager_->IsRegistered(this));

  if (!is_disconnected || !is_unregistered) {
    ErrorReporter::Report(String::Format("NetClientWorker.dtor - assert fail: %d %d", is_disconnected, is_unregistered));
  }
}

void NetClientWorker::Heartbeat() {
  //WarnTooLongElapsedTime();

  switch (GetState()) {
    case State::IssueConnect:
      Heartbeat_IssueConnect();
      break;
    case State::CONNECTING:
      Heartbeat_Connecting();
      break;
    case State::JUST_CONNECTED:
      Heartbeat_JustConnected();
      break;
    case State::CONNECTED:
      Heartbeat_Connected();
      break;
    case State::DISCONNECTING:
      Heartbeat_Disconnecting();
      break;
    case State::DISCONNECTED:
      // 아무것도 안함.
      break;
  }

  owner_->TcpAndUdp_LongTick();

  // Send issue를 할 것들을 찾아서 socket issue를 시행한다.
  // 10ms는 눈에도 띄지 않고 send completion event시 즉시 다음 send issue를 하므로 랙에 민감하지도 않다.
  // 게다가 TCP Nagle OFF나 UDP coalesce에서는 초송신이 오히려 약간의 시간은 대기 후 전송이니까.
  if (owner_->process_send_ready_remotes_alarm_.TakeElapsedTime(owner_->GetElapsedTime())) {
    owner_->EveryRemote_IssueSendOnNeed();
  }
}

//TODO 락을 걸고 들어오므로, 여기서 구태여 락을 또 걸필요는 없어보이는데...
void NetClientWorker::Heartbeat_ConnectedCase() {
  CScopedLock2 owner_guard(owner_->GetMutex());

  const double absolute_time = owner_->GetAbsoluteTime();

  //@maxidea: todo: 30이라는 값을 별도로 빼주는게 좋을듯..
  // 30초 동안이나 Client->Tick()함수가 호출되지 않았을 체크.
  if (owner_->last_tick_invoked_time_ > 0 && (absolute_time - owner_->last_tick_invoked_time_) > 30) { //TODO 하드코딩된 이 숫자를 외부로 빼주도록 하자.
    if (owner_->IsIntraLoggingOn()) {
      owner_->IntraLogToServer(LogCategory::System, "WARNING: NetClient.Tick is not called in 30 seconds.  Is this your intention?");
    }

    owner_->last_tick_invoked_time_ = -1;
  }

#if TRACE_ISSUE_DELAY_LOG
  if (owner_->IsIntraLoggingOn()) {
    auto to_server_tcp = owner_->Get_ToServerTcp();

    if (to_server_tcp) {
      if (!to_server_tcp->recv_issued_) {
        if (to_server_tcp->last_recv_invoke_warning_time_ != 0) {
          if ((owner_->GetAbsoluteTime() - to_server_tcp->last_recv_invoke_warning_time_) > 3) { // TODO: NetConfig에 값으로 빼주는게...
            to_server_tcp->last_recv_invoke_warning_time_ = absolute_time;
            owner_->IntraLogToServer(LogCategory::System, "WARNING: Recvissue is not called in 3 seconds.");
          }
        } else {
          to_server_tcp->last_recv_invoke_warning_time_ = absolute_time;
        }
      } else {
        to_server_tcp->last_recv_invoke_warning_time_ = 0;
      }

      if (!to_server_tcp->send_issued_) {
        if (to_server_tcp->last_send_invoked_warning_time_ != 0) {
          if ((absolute_time - to_server_tcp->last_send_invoked_warning_time_) > 30) { // TODO: NetConfig에 값으로 빼주는게...
            to_server_tcp->last_send_invoked_warning_time_ = absolute_time;
            owner_->IntraLogToServer(LogCategory::System, "WARNING: Sendissue is not called in 30 seconds.");
          }
        } else {
          to_server_tcp->last_send_invoked_warning_time_ = absolute_time;
        }
      } else {
        to_server_tcp->last_send_invoked_warning_time_ = 0;
      }
    }
  }
#endif // TRACE_ISSUE_DELAY_LOG

  // TCP Heartbeat가 한동안 오가지 않았다면, Disconnect.
  if ((absolute_time - owner_->last_tcp_stream_recv_time_) > owner_->settings_.default_timeout_sec) {
    if (owner_->worker_) {
      if (owner_->IsIntraLoggingOn()) {
        const String text = String::Format("Connect to server timed out.  default_timeout_time: %lf, absolute_time: %lf, last_tcp_stream_received_time: %lf",
                        owner_->settings_.default_timeout_sec, absolute_time, owner_->last_tcp_stream_recv_time_);
        owner_->IntraLogToServer(LogCategory::System, *text);
      }

      // Disconnect
      owner_->EnqueueDisconnectionEvent(ResultCode::DisconnectFromLocal, ResultCode::ConnectServerTimeout);
      owner_->worker_->SetState(NetClientWorker::State::Disconnecting);
    }
  }

  fun_check(owner_->manager_->IsThisWorkerThread());

  Heartbeat_EveryRemotePeer();

  owner_->ConditionalSendServerHolePunch();
  owner_->ConditionalRequestServerTime();
  owner_->ConditionalSpeedHackPing();
  owner_->ConditionalSyncIndirectServerTime();
  owner_->ConditionalFallbackServerUdpToTcp();
  owner_->ConditionalReportP2PPeerPing();
  owner_->ConditionalAddUPnPTcpPortMapping();
  owner_->ReportRealUdpCount();
  owner_->CheckSendQueue();

  // Emergency log를 위해 CNetClientStats의 사본을 일정시간마다 갱신한다.
  owner_->UpdateNetClientStatClone();

  // 중간에 컴플리션이 뜨지 않으면 남아있는 preFinal이 Final로 이동되지 않으므로..
  // 반드시 필요하다!!! 일단 사용중지.
  //ProcessEveryMessageOrMoveToFinalRecvQueue(nullptr);
}

void NetClientWorker::Heartbeat_EveryRemotePeer() {
  owner_->LockMain_AssertIsLockedByCurrentThread();

  const double absolute_time = owner_->GetAbsoluteTime();

  int32 p2p_conn_count = 0;
  for (auto& pair : owner_->remote_peers_) {
    auto& peer = pair.value;

    if (peer && !peer->garbaged_) {
      peer->Heartbeat(absolute_time);

      if (peer->p2p_holepunch_attempt_context_) {
        p2p_conn_count++;
      }
    }
  }

  // 병렬 진행중인 P2P 연결 시도 횟수에 따라 인터벌을 조절한다.
  const double p2p_attempt_under_progress_count = MathBase::Max(1.0, (double)p2p_conn_count);

  owner_->p2p_holepunch_interval_sec_ = NetConfig::p2p_holepunch_interval_sec * p2p_attempt_under_progress_count;
  owner_->p2p_connection_attempt_end_time_ = NetConfig::GetP2PHolepunchEndTime() * p2p_attempt_under_progress_count;

  fun_check(owner_->p2p_holepunch_interval_sec_ > 0);
  fun_check(owner_->p2p_connection_attempt_end_time_ > 0);
}

void NetClientWorker::UdpRecvCompletionCase(UdpSocket_C* udp_socket,
                                            CompletionStatus& completion) {
  CScopedLock2 owner_guard(owner_->GetMutex());

  udp_socket->recv_issued_ = false;

  // Length > 0이지만 SocketError가 있는 경우가 있을 수 있다.
  // 따라서 Length > 0인 경우는 에러 코드를 무시해야 한다.
  // 재활용인 UdpsScket는 RecvCompletion을 씹는다.
  if (udp_socket->recycle_binned_time_ == 0 && completion.completed_length > 0) {
    const double absolute_time = owner_->GetAbsoluteTime();

    // 받은 UDP packet을 처리한다.
    owner_->stats_.total_udp_recv_count++;
    owner_->stats_.total_udp_recv_bytes += completion.completed_length;

    // 받은 UDP packet을 assemble한다.
    assembled_packet assembled_packet;
    String out_error;

    const UdpPacketDefragger::AssembledPacketError assembled_result =
      udp_socket->packet_defragger_->PushFragmentAndPopAssembledPacket(
          udp_socket->socket_->GetRecvBufferPtr(),
          completion.completed_length,
          completion.recvfrom_addr,
          owner_->GetSrcHostIdByAddrAtDestSide_NOLOCK(completion.recvfrom_addr),
          absolute_time,
          assembled_packet,
          out_error);

    if (assembled_result == UdpPacketDefragger::AssembledPacketError::Ok) {
      // 완전한 msg가 도착한 것들을 모두 추려서 final recv queue로 옮기거나 여기서 처리한다.
      ReceivedMessageList extracted_msg_list;
      ResultCode extract_result;
      owner_->ExtractMessagesFromUdpRecvQueue(
          assembled_packet.ConstData(),
          assembled_packet.Len(),
          assembled_packet.sender_addr,
          extracted_msg_list,
          extract_result);
      if (extract_result == ResultCode::Ok) {
        for (auto& msg : extracted_msg_list) {
          //ExtractedMessage.action_time = absolute_time;
          owner_->pre_final_recv_queue.Enqueue(msg);
        }

        // 휴지통에 들어갔으면 defragboard access를 하지 말아야.
        // 왜냐하면 FunNet.IUdpPacketDefragBoardDelegate가 main인데
        // 그걸 참조할 수 없는 상황일 것이므로(아마도)
        // 물론, 더 이상의 issue recv도 걸지 말아야. 폐기되어야 하니.
        if (udp_socket->owner_peer_ || udp_socket == owner_->Get_ToServerUdpSocket()) {
          // 이제 수신된 메시지에 따라 명령 수행을.
          ProcessEveryMessageOrMoveToFinalRecvQueue(udp_socket);
        }
      } else {
        //TODO error handling..
      }
    } else if (assembled_result == UdpPacketDefragger::AssembledPacketError::Error) {
      owner_->EnqueuePacketDefragWarning(completion.recvfrom_addr, *out_error);
    }
  }

  // 만약 completed_length < 0 이라면 GetLastError를 남긴다.
  if (completion.completed_length < 0) {
    owner_->emergency_log_data_.last_error_completion_length = (int32)::GetLastError(); //@todo 길이에 에러코드를 담는다??
  }

  // UDP 소켓을 닫은 경우가 아닌 이상 수신을 다시 건다.
  // WSAECONNRESET이 에러인 경우 아직 소켓은 건재하므로 계속 강행
  udp_socket->ConditionalIssueRecvFrom();
}

// user에게 처리권이 넘어가기 직전의 수신된 message를 처리한다.
//
// 여기서 처리된 것 중 일부는 final message로 넘어가게 된다.
bool NetClientWorker::ProcessMessage_EngineLayer( UdpSocket_C* udp_socket,
                                                  ReceivedMessage& received_msg) {
  // NOTE: udp_socket may be nullptr if received from TCP socket

  owner_->LockMain_AssertIsLockedByCurrentThread();

  auto& msg = received_msg.unsafe_message;
  const int32 saved_read_pos = msg.Tell();

  //debugging
//#if TRACE_NETCLIENT_MESSAGES
//  ByteArray HexDump;
//  ByteArray::BytesToDebuggableString(HexDump, msg.ToReadableBytesCopy(), false, "  >> ");
//#endif

  MessageType msg_type;
  if (!lf::Read(msg, msg_type)) {
    msg.Seek(saved_read_pos);
    return false;
  }

  //debugging
#if TRACE_NETCLIENT_MESSAGES
  //UdpSocket이 유효한 경우와 아닌 경우를 나누어서 출력해야함.

  if (msg_type == MessageType::RPC) {
    //TODO
    //등록된 stub들에서 함수이름을 찾아낼 수 있어야함.
    //인자까지 얻어낼 수 있다면 더 좋을듯 싶은데...?
    //함수를 하나 만들어낼까??
    //처리는 하지 않고 인자까지만 읽어애는 코드를 추가하는게 좋을듯 싶다.
    //아님 flag만으로 처리하는것도 좋을듯 싶음.
    const int32 old_pos = msg.Tell();
    RpcId rpc_id = (RpcId)0;
    lf::Read(msg, rpc_id);
    msg.Seek(old_pos);

    LOG(LogNetEngine, Info, ">> NetClient.MSG [%s] [%05d] [%s] RPC#%d (LEN: %d)",
          udp_socket ? "UDP" : "TCP",
          (int32)received_msg.remote_id,
          received_msg.relayed ? "Relayed" : "NonRelayed",
          (int32)rpc_id,
          msg.ReadableLength());
          //*String(HexDump));
  } else {
    //LOG(LogNetEngine, Info, ">> NetClient.MSG [%s] [%05d] [%s] %s (LEN: %d)",
    //      udp_socket ? "UDP" : "TCP",
    //      (int32)received_msg.remote_id,
    //      received_msg.relayed ? "Relayed" : "NonRelayed",
    //      *ToString(msg_type),
    //      msg.ReadableLength());
    //      //*String(HexDump));
  }
#endif

  bool msg_processed = false;
  switch (msg_type) {
    case MessageType::ConnectServerTimedout:
      if (!IsFromRemoteClientPeer(received_msg)) {
        ProcessMessage_ConnectServerTimedout(msg);
      }
      msg_processed = true;
      break;

    case MessageType::NotifyServerConnectionHint:
      if (!IsFromRemoteClientPeer(received_msg)) {
        ProcessMessage_NotifyServerConnectionHint(msg);
      }
      msg_processed = true;
      break;

    case MessageType::NotifyCSSessionKeySuccess:
      if (!IsFromRemoteClientPeer(received_msg)) {
        ProcessMessage_NotifyCSSessionKeySuccess(msg);
      }
      msg_processed = true;
      break;

    case MessageType::NotifyProtocolVersionMismatch:
      if (!IsFromRemoteClientPeer(received_msg)) {
        ProcessMessage_NotifyProtocolVersionMismatch(msg);
      }
      msg_processed = true;
      break;

    case MessageType::NotifyServerDeniedConnection:
      if (!IsFromRemoteClientPeer(received_msg)) {
        ProcessMessage_NotifyServerDeniedConnection(msg);
      }
      msg_processed = true;
      break;

    case MessageType::NotifyServerConnectSuccess:
      if (!IsFromRemoteClientPeer(received_msg)) {
        ProcessMessage_NotifyServerConnectSuccess(msg);
      }
      msg_processed = true;
      break;

    case MessageType::RequestStartServerHolepunch:
      if (!IsFromRemoteClientPeer(received_msg)) {
        ProcessMessage_RequestStartServerHolepunch(msg);
      }
      msg_processed = true;
      break;

    case MessageType::ServerHolepunchAck:
      if (!IsFromRemoteClientPeer(received_msg)) {
        ProcessMessage_ServerHolepunchAck(received_msg);
      }
      msg_processed = true;
      break;

    case MessageType::PeerUdp_ServerHolepunchAck:
      if (!IsFromRemoteClientPeer(received_msg)) {
        ProcessMessage_PeerUdp_ServerHolepunchAck(received_msg);
      }
      msg_processed = true;
      break;

    case MessageType::NotifyClientServerUdpMatched:
      if (!IsFromRemoteClientPeer(received_msg)) {
        ProcessMessage_NotifyClientServerUdpMatched(msg);
      }
      msg_processed = true;
      break;

    case MessageType::ReplyServerTime:
      if (!IsFromRemoteClientPeer(received_msg)) {
        ProcessMessage_ReplyServerTime(msg);
      }
      msg_processed = true;
      break;

    case MessageType::ArbitaryTouch:
      msg_processed = true;
      break;

    case MessageType::ReliableRelay2:
      if (!IsFromRemoteClientPeer(received_msg)) {
        ProcessMessage_ReliableRelay2(msg);
      }
      msg_processed = true;
      break;

    case MessageType::UnreliableRelay2:
      if (!IsFromRemoteClientPeer(received_msg)) {
        ProcessMessage_UnreliableRelay2(udp_socket, received_msg);
      }
      msg_processed = true;
      break;

    case MessageType::LingerDataFrame2:
      if (!IsFromRemoteClientPeer(received_msg)) {
        ProcessMessage_LingerDataFrame2(udp_socket, received_msg);
      }
      msg_processed = true;
      break;

    case MessageType::RUdp_Frame:
      ProcessMessage_RUdp_Frame(udp_socket, received_msg);
      msg_processed = true;
      break;

    case MessageType::PeerUdp_PeerHolepunch:
      ProcessMessage_PeerUdp_PeerHolepunch(received_msg);
      msg_processed = true;
      break;

    case MessageType::PeerUdp_PeerHolepunchAck:
      ProcessMessage_PeerHolepunchAck(received_msg);
      msg_processed = true;
      break;

    case MessageType::P2PIndirectServerTimeAndPing:
      ProcessMessage_P2PIndirectServerTimeAndPing(received_msg);
      msg_processed = true;
      break;

    case MessageType::P2PIndirectServerTimeAndPong:
      ProcessMessage_P2PIndirectServerTimeAndPong(received_msg);
      msg_processed = true;
      break;

    case MessageType::S2CRoutedMulticast1:
      if (!IsFromRemoteClientPeer(received_msg)) {
        ProcessMessage_S2CRoutedMulticast1(udp_socket, received_msg);
      }
      msg_processed = true;
      break;

    case MessageType::S2CRoutedMulticast2:
      ProcessMessage_S2CRoutedMulticast2(udp_socket, received_msg);
      msg_processed = true;
      break;

    case MessageType::RPC:
      ProcessMessage_RPC(received_msg, msg_processed);
      break;

    case MessageType::FreeformMessage:
      ProcessMessage_FreeformMessage(received_msg, msg_processed);
      break;

    case MessageType::Encrypted_Reliable:
    case MessageType::Encrypted_Unreliable: {
        ReceivedMessage decrypted_received_msg;
        if (owner_->DecryptMessage(msg_type, received_msg, decrypted_received_msg.unsafe_message)) {
          //decrypted_received_msg.action_time = received_msg.action_time;
          decrypted_received_msg.relayed = received_msg.relayed;
          decrypted_received_msg.remote_addr_udp_only = received_msg.remote_addr_udp_only;
          decrypted_received_msg.remote_id = received_msg.remote_id;
          msg_processed |= ProcessMessage_EngineLayer(udp_socket, decrypted_received_msg); // Recursive call
        }
        break;
      }

    case MessageType::Compressed: {
        ReceivedMessage decompressed_received_msg;
        if (owner_->DecompressMessage(received_msg, decompressed_received_msg.unsafe_message)) {
          //decompressed_received_msg.action_time = received_msg.action_time;
          decompressed_received_msg.relayed = received_msg.relayed;
          decompressed_received_msg.remote_addr_udp_only = received_msg.remote_addr_udp_only;
          decompressed_received_msg.remote_id = received_msg.remote_id;
          msg_processed |= ProcessMessage_EngineLayer(udp_socket, decompressed_received_msg); // Recursive call
        }
        break;
      }

    case MessageType::RequestReceiveSpeedAtReceiverSide_NoRelay:
      ProcessMessage_RequestReceiveSpeedAtReceiverSide_NoRelay(udp_socket, received_msg);
      msg_processed = true;
      break;

    case MessageType::ReplyReceiveSpeedAtReceiverSide_NoRelay:
      ProcessMessage_ReplyReceiveSpeedAtReceiverSide_NoRelay(received_msg);
      msg_processed = true;
      break;

    default:
      //TODO logging...
      break;

  } // end of switch(msg_type)


  // 만약 잘못된 메시지가 도착한 것이면 이미 FunNet 계층에서 처리한 것으로 간주하고
  // 메시지를 폐기한다. 그리고 예외 발생 이벤트를 던진다.
  // 단, C++ 예외를 발생시키지 말자. 디버깅시 혼란도 생기며 fail over 처리에도 애매해진다.
  const int32 l1 = msg.GetLength();
  const int32 l2 = msg.Tell();

  // 암호화된 메시지는 별도 버퍼에서 복호화된 후 처리되므로
  if (msg_processed &&
      l1 != l2 &&
      msg_type != MessageType::Encrypted_Reliable &&
      msg_type != MessageType::Encrypted_Unreliable) {
    //@note 에러 상황이지만, fail-over를 위해서 에러만 기록하고 계속 진행하도록 한다.
    msg_processed = true;

    // 에러가 난시점의 msg를 기록한다.
    String comment;
    comment += String::Format("Location: %s\n", __FUNCTION__);
    comment += String::Format(", Message=(Name=%s(%d), Length=%d, ReadOffset=%d)", *ToString(msg_type), (int32)msg_type, l1, l2);

    // 딱히 copy를 하지 않아도 될듯한데...
    // ByteArray ProblemMessage(msg.GetData(), msg.GetLength());
    // ByteArray ProblemMessage = msg.GetSharableBuffer(); //TODO 오프셋 관련하여 문제가 없으려나???
    // 우선은 안전하게 전체 데이터의 사본을 넘겨주는 형태로 처리하도록 하자.

    // 일단은 안전하게 전체 내용 복사해서 처리.. 차후에 복사를 제거하는 쪽으로 개선해야함.
    // 대량으로 계속 패킷이 꺠져온다는 가정을 한다면, 이 복사비용도 부담이될터이니,
    // 추후에 참조 형태로 넘길 수 있도록 변경하자.
    const ByteArray spurious_msg = msg.ToAllBytesCopy();
    owner_->EnqueueError(ResultInfo::From(ResultCode::InvalidPacketFormat, received_msg.remote_id, comment, spurious_msg));
    //SetState(State::Disconnecting);
  }

  // 정상적으로 처리되지 않았다면, 메시지 읽기 위치를 호출 시점으로 복구.
  if (!msg_processed) {
    msg.Seek(saved_read_pos);
    return false;
  }

  return true;
}

void NetClientWorker::ProcessMessage_PeerUdp_PeerHolepunch(ReceivedMessage& received_msg) {
  owner_->LockMain_AssertIsLockedByCurrentThread();

  P2PHolepunchAttemptContext::ProcessPeerHolepunch(owner, received_msg);
}

void NetClientWorker::ProcessMessage_PeerHolepunchAck(ReceivedMessage& received_msg) {
  owner_->LockMain_AssertIsLockedByCurrentThread();

  P2PHolepunchAttemptContext::ProcessPeerHolepunchAck(owner, received_msg);
}

void NetClientWorker::ProcessMessage_RUdp_Frame(UdpSocket_C* udp_socket,
                                                ReceivedMessage& received_msg) {
  owner_->LockMain_AssertIsLockedByCurrentThread();

  // UDP addr를 근거로 remote peer를 찾는다.
  // GetPeerByUdpAddr는 홀펀칭이 아직 안된 remote peer는 찾지 못하므로 OK.
  auto peer = owner_->GetPeerByUdpAddr(received_msg.remote_addr_udp_only);
  if (peer && !peer->to_peer_rudp.panic_) {
    //TODO 매우 큰 데이터(1MB남짓)를 연속해서 보내면, 데이터의 변형이 생기는 경우가 있음.

    ReceivedMessageList udp_received_msgs;
    ResultCode extract_result;
    peer->to_peer_rudp.EnqueueReceivedFrameAndGetFlushedMessages(received_msg.unsafe_message, udp_received_msgs, extract_result);
    if (extract_result == ResultCode::Ok) {
      for (auto& udp_received_msg : udp_received_msgs) {
        //udp_received_msg.action_time = owner_->GetAbsoluteTime();
        udp_received_msg.relayed = received_msg.relayed;

        // 각 메시지에 대해서 재귀 호출을 한다. 이렇게 안하면 무한루프에 걸린다.
        ProcessMessageOrMoveToFinalRecvQueue(udp_socket, udp_received_msg);
      }
    } else {
      //TODO 에러가 안나는게 맞는건가?
      LOG(LogNetEngine,Error, "Extraction failed: %s", *ToString(extract_result));
      owner_->EnqueueError(ResultInfo::From(extract_result, peer->host_id_, "Stream extract error at reliable UDP"));
      //TODO 접속을 끊어야하지 않을까??
      //SetState(State::Disconnecting);
    }
  } else {
    // Symmetric NAT 홀펀칭을 하는 경우 등에서, 의도하지 않은 다른 peer로부터의 메시지 수신이 있을 수 있다.
    // 이런 경우, 즉 대응되지 않는 패킷인 경우라도 끝까지 다 읽은셈 쳐야 한다.
    // 안그러면 엄한데서 온 UDP frame message가 있는 경우 bad stream exception이 발생하기 때문이다.
    received_msg.unsafe_message.SeekToEnd();
  }
}

// 서버와의 홀펀칭이 완료 되었을때 수신받는 메시지
void NetClientWorker::ProcessMessage_NotifyClientServerUdpMatched(MessageIn& msg) {
  // Set that TCP fallback nomore
  if (!lf::Read(msg, owner_->to_server_udp_fallbackable_->holepunch_tag_)) {
    TRACE_SOURCE_LOCATION();
    return;
  }

  owner_->to_server_udp_fallbackable_->SetRealUdpEnabled(true);

  // Enqueue a local event
  LocalEvent event(LocalEventType::ServerUdpChanged);
  event.result_info.Reset(new ResultInfo());
  event.remote_id = HostId_Server;
  owner_->EnqueueLocalEvent(event);

  if (owner_->IsIntraLoggingOn()) {
    owner_->IntraLogToServer(LogCategory::SP2P, "Holepunch to server UDP successful.");
  }
}

//  홀펀치에 대한 응답을 서버에서 받음.
//
//   그런데 현재 로컬에서 클라이언트를 띄웠을 경우에는 서버 홀펀칭이 되지만,
//   외부에서 시도할 경우에는 홀펀칭이 되지 않고 있다.
//   UDP 통신측면에서 서버로 전송은 하고 있지만, 수신을 받지 못하고 있는 상태다.
//   왜일까??
//
// -> 해당 버그는 고쳐졌음.
//    UDP 소켓 생성시 TCP 접속으로 얻어낸 클라 주소로 Dummy(MessageType::Ignore) 패킷을
//    보내도록 되어 있었는데, 이 패킷을 보낸 후로 서버의 Send쪽이 막히는 문제가 있었음. (wireshark를 통해서 확인.)
//    해당 부분의 코드를 막아준 이후로 아주 잘됨.
void NetClientWorker::ProcessMessage_ServerHolepunchAck(ReceivedMessage& received_msg) {
  auto& msg = received_msg.unsafe_message;

  Uuid holepunch_tag;               // Key to confirm arrival from desired.
  InetAddress here_addr_at_server;  // The address of this client that is recognized by the server (this address is used as a unique identifier).
  if (!lf::Reads(msg,  holepunch_tag, here_addr_at_server)) {
    // Since it is an invalid message, progress is no longer meaningful.
    TRACE_SOURCE_LOCATION();
    return;
  }

  // it came from the wrong place, not where I wanted to be. Ignore it.
  if (holepunch_tag != owner_->to_server_udp_fallbackable_->holepunch_tag_) {
    TRACE_SOURCE_LOCATION();
    return;
  }

  // it came from the wrong place, not where I wanted to be. Ignore it.
  if (owner_->to_server_udp_fallbackable_->server_addr_ != received_msg.remote_addr_udp_only) {
    TRACE_SOURCE_LOCATION();
    return;
  }

  // Must be a valid port number.
  //fun_check(owner_->Get_ToServerUdpSocketLocalAddr().GetPort() != 0);
  //fun_check(owner_->Get_ToServerUdpSocketLocalAddr().GetPort() != 0xFFFF);
  fun_check(owner_->Get_ToServerUdpSocketLocalAddr().IsUnicast());

  // The address sent by the server. Saved for use.
  owner_->Get_ToServerUdpSocket()->here_addr_at_server_ = here_addr_at_server;

  // Send my tag & client local UDP address via TCP
  MessageOut msg_to_send;
  lf::Write(msg_to_send, MessageType::NotifyHolepunchSuccess);
  lf::Write(msg_to_send, owner_->to_server_udp_fallbackable_->holepunch_tag_);
  lf::Write(msg_to_send, owner_->Get_ToServerUdpSocketLocalAddr());
  lf::Write(msg_to_send, here_addr_at_server);
  owner_->Get_ToServerTcp()->SendWhenReady(SendFragRefs(msg_to_send), TcpSendOption());

  if (owner_->IsIntraLoggingOn()) {
    const String text = String::Format("Message_ServerHolepunchAck.  addr_of_here_at_server: %s", *here_addr_at_server_.ToString());
    owner_->IntraLogToServer(LogCategory::SP2P, *text);
  }
}

void NetClientWorker::ProcessMessage_PeerUdp_ServerHolepunchAck(ReceivedMessage& received_msg) {
  auto& msg = received_msg.unsafe_message;

  Uuid holepunch_tag;
  InetAddress here_addr_at_server;
  HostId peer_id;
  if (!lf::Reads(msg,  holepunch_tag, here_addr_at_server_, peer_id)) {
    TRACE_SOURCE_LOCATION();
    return;
  }

//debugging
#if 0
  LOG(LogNetEngine,Warning,">> PeerUdp_ServerHolepunchAck: tag: %s, addr_of_here_at_server: %s, peer_id: %d",
      *holepunch_tag.ToString(), *here_addr_at_server_.ToString(), (int32)peer_id);
#endif

  auto peer = owner_->GetPeerByHostId(peer_id);
  if (peer && !peer->garbaged_ && peer->p2p_holepunch_attempt_context_) {
    peer->p2p_holepunch_attempt_context_->ProcessMessage_PeerUdp_ServerHolepunchAck(received_msg, holepunch_tag, here_addr_at_server, peer_id);
  }
}

// 서버로 부터의 브로드캐스팅 메세지를 클라이언트 단에서 릴레이하기 위해서 설정
// 릴레이 해줄 피어가 처리한다.
//
// MessageType::S2CRoutedMulticast1 를 처리하기 위한 프로세스를 수행한다.
// HostIdArray를 얻어서 바로 Data를 Sending 해 준다.
//
// 서버로 부터 넘어온 메세지를 피어간 릴레이를 하기 위해서 설정
bool NetClientWorker::ProcessMessage_S2CRoutedMulticast1( UdpSocket_C* udp_socket,
                                                          ReceivedMessage& received_msg) {
  // 서버로부터 온 메시지가 아니면 무시한다.
  if (received_msg.remote_id != HostId_Server) {
    TRACE_SOURCE_LOCATION();
    return false;
  }

  auto& msg = received_msg.unsafe_message;

  MessagePriority priority;
  uint64 unique_id;
  HostIdArray host_id_list;
  ByteArray content;
  if (!lf::Reads(msg,  priority, unique_id, host_id_list, content)) {
    TRACE_SOURCE_LOCATION();
    return false;
  }

  MessageOut routed_msg;
  lf::Write(routed_msg, MessageType::S2CRoutedMulticast2);
  lf::Write(routed_msg, content);

  UdpSendOption send_opt(GUnreliableSend_INTERNAL);
  send_opt.priority = priority;
  send_opt.unique_id = unique_id;
  send_opt.conditional_fragging = false;

  // Peer가 relayed thru server 상태인 경우 다시 릴레이하면 효율성이 무시되어버린다.
  // 일단, 서버에서 direct P2P라는걸로 인식된 상태이므로 그걸 받아들이고 직빵 전송해야 한다.
  for (int32 i = 0; i < host_id_list.Count(); ++i) {
    if (host_id_list[i] != owner_->local_host_id_) {
      auto peer = owner_->GetPeerByHostId(host_id_list[i]);

      if (peer && !peer->garbaged_ && peer->IsDirectP2P() && peer->udp_socket_) {
        // 상대가 relay일 때 이는 무시되겠지만 그래도 상관없다. 어차피 unreliable이고 조만간 서버에서는
        // 제대로 조정된 것으로 보내질 터이니.
        const auto filter_tag = FilterTag::Make(owner_->GetLocalHostId(), host_id_list[i]);
        peer->Get_ToPeerUdpSocket()->SendWhenReady(
              host_id_list[i],
              filter_tag,
              peer->p2p_holepunched_local_to_remote_addr_,
              routed_msg,
              owner_->GetAbsoluteTime(),
              send_opt);
      }
    }
  }


  //FAKE: 릴레이를 하는 peer에게 서버로 부터 온 메세지라고 인지시킨다.
  ReceivedMessage relayed_msg;
  relayed_msg.remote_id = HostId_Server;
  //relayed_msg.action_time = owner_->GetAbsoluteTime();
  relayed_msg.unsafe_message = MessageIn(content); // share

  ProcessMessageOrMoveToFinalRecvQueue(udp_socket, relayed_msg);

  return true;
}

// 피어로 부터 온 서버 메세지를 처리하기 위한 메써드
//
// 피어로 부터 받은 서버로 부터의 릴레이드 메세지를 받는 피어기 처리한다.
bool NetClientWorker::ProcessMessage_S2CRoutedMulticast2( UdpSocket_C* udp_socket,
                                                          ReceivedMessage& received_msg) {
  auto& msg = received_msg.unsafe_message;

  ByteArray content;
  if (!lf::Read(msg, content)) {
    TRACE_SOURCE_LOCATION();
    return false;
  }

  //@fake 릴레이를 하는 피어에게 서버로 부터 온 메세지라고 인지시킨다.
  // FAKE : Recognize that the relaying peer is a message from the server.
  ReceivedMessage routed_msg;
  routed_msg.remote_id = HostId_Server;
  //routed_msg.action_time = owner_->GetAbsoluteTime();
  routed_msg.unsafe_message = MessageIn(content); // share
  routed_msg.relayed = true; // fake!

  ProcessMessageOrMoveToFinalRecvQueue(udp_socket, routed_msg);

  return true;
}

void NetClientWorker::ProcessMessage_RequestStartServerHolepunch(MessageIn& msg) {
  //읽어들이기만 하고 실제로는 사용하지 않음.  검증 과정에 사용하려고 보낸듯..
  if (!lf::Read(msg, owner_->to_server_udp_fallbackable_->holepunch_tag_)) {
    TRACE_SOURCE_LOCATION();
    return;
  }

  fun_check(owner_->to_server_udp_fallbackable_->server_addr_.IsUnicast());

  // Begin sending signature to server via UDP, repeat
  owner_->to_server_udp_fallbackable_->holepunch_cooltime_ = 0;
  owner_->to_server_udp_fallbackable_->holepunch_attempt_count_ = 0;
}

void NetClientWorker::ProcessMessage_NotifyServerConnectSuccess(MessageIn& msg) {
  owner_->LockMain_AssertIsLockedByCurrentThread();

  // Set local host_id: connect established! but TCP fallback mode yet
  Uuid server_instance_tag_;
  HostId local_host_id;
  ByteArray user_data;
  NamedInetAddress local_addr_at_server;
  if (!lf::Reads(msg,  local_host_id, server_instance_tag_, user_data, local_addr_at_server)) {
    auto error = ResultInfo::From(ResultCode::ProtocolVersionMismatch, HostId_Server, String::Format("Bad format in %s", __FUNCTION__));
    owner_->EnqueueConnectFailEvent(ResultCode::InvalidPacketFormat, error);

    SetState(State::Disconnecting);
    return;
  }

  owner_->local_host_id_ = local_host_id;
  owner_->backup_host_id_ = local_host_id;

  fun_check(owner_->to_server_tcp_);
  // 경우에 따라서는 resolve하느라 느려질 수 있지 않을까?? 뭐 어짜피 ip literal로 오므로 상관 없으려나???
  owner_->Get_ToServerTcp()->local_addr_at_server_ = local_addr_at_server.ToInetAddress();

  // Connection hint까지 왔으면, TCP 연결을 add port mapping upnp를 걸기 위해, 여기서부터 미리 켜도록 한다.
  owner_->ConditionalStartupUPnP();

  const InetAddress server_addr = InetAddress(owner_->connect_args_.server_ip, owner_->connect_args_.server_port);
  {
    CScopedLock2 owner_guard(owner_->GetMutex());

    owner_->server_instance_tag_ = server_instance_tag;

    // Enqueue connect ok event
    LocalEvent event(LocalEventType::ConnectServerSuccess);
    event.user_data = user_data;
    event.remote_id = HostId_Server;
    event.remote_addr = server_addr;
    owner_->EnqueueLocalEvent(event);

    //TODO
    //연결상태가 변경되었음을 Visualizer에게 알림.
    //if (owner_->viz_agent) {
    //  ServerConnectionState ignorant;
    //
    //  CScopedLock2 viz_agent_guard(owner_->viz_agent_->mutex_);
    //  owner_->viz_agent_->c2s_proxy_.NotifyClient_ConnectionState(HostId_Server, GReliableSend_INTERNAL, owner_->GetServerConnectionState(ignorant));
    //}
  }

  if (owner_->IsIntraLoggingOn()) {
    const String text = String::Format("Connecting to server successful.  host_id: %d, local_addr: %s, addr_at_server: %s, server_addr: %s, server_instance_tag: %s",
                    (int32)owner_->local_host_id_,
                    *owner_->Get_ToServerTcp()->local_addr.ToString(),
                    *owner_->Get_ToServerTcp()->local_addr_at_server_.ToString(),
                    *server_addr.ToString(),
                    *server_instance_tag.ToString());
    owner_->IntraLogToServer(LogCategory::System, *text);
  }
}

void NetClientWorker::ProcessMessage_ConnectServerTimedout(MessageIn& msg) {
  owner_->EnqueueConnectFailEvent(ResultCode::ConnectServerTimeout);

  SetState(State::Disconnecting);
}

void NetClientWorker::ProcessMessage_NotifyServerConnectionHint(MessageIn& msg) {
  owner_->LockMain_AssertIsLockedByCurrentThread();

  bool intra_logging_on;
  NetSettings settings;
  ByteArray public_key_blob_;
  if (!lf::Reads(msg,  intra_logging_on, settings, public_key_blob_)) {
    owner_->EnqueueDisconnectionEvent(ResultCode::ProtocolVersionMismatch, ResultCode::TCPConnectFailure);
    SetState(State::Disconnecting);
    return;
  }

  //debugging
  //LOG(LogNetEngine,Warning, "settings_: %s", *ToString(settings_));

  owner_->settings_ = settings;

  if (owner_->settings_.ping_test_enabled) {
     owner_->enable_ping_test_end_time_ = owner_->GetAbsoluteTime();
  }

  ByteArray strong_random_block;
  ByteArray weak_random_block;

  if (!CryptoRSA::CreateRandomBlock(strong_random_block, owner_->settings_.strong_encrypted_message_key_length) ||
      !CryptoAES::ExpandFrom(owner_->SelfP2PSessionKey.aes_key, (const uint8*)strong_random_block.ConstData(), owner_->settings_.strong_encrypted_message_key_length / 8) ||

      !CryptoRSA::CreateRandomBlock(weak_random_block, owner_->settings_.weak_encrypted_message_key_length) ||
      !CryptoRC4::ExpandFrom(owner_->SelfP2PSessionKey.rc4_key, (const uint8*)weak_random_block.ConstData(), owner_->settings_.weak_encrypted_message_key_length / 8)) {
    owner_->EnqueueDisconnectionEvent(ResultCode::EncryptFail, ResultCode::TCPConnectFailure);

    if (owner_->callbacks_) {
      owner_->callbacks_->OnException(HostId_None, Exception("Failed to create session-key"));
    }

    SetState(State::Disconnecting);
    return;
  }

  // 아예 전역 값도 수정하도록 한다.
  {
    CScopedLock2 net_config_write_guard(NetConfig::GetWriteMutex());
    NetConfig::message_max_length = MathBase::Max<int32>(NetConfig::message_max_length, settings_.message_max_length);
  }

  // 끝까지 못 읽어들였다면, 문제가 있을 수 있음. (버젼이 맞지 않는다던지...)
  if (!msg.AtEnd()) {
    owner_->EnqueueDisconnectionEvent(ResultCode::ProtocolVersionMismatch, ResultCode::TCPConnectFailure);
    SetState(State::Disconnecting);
    return;
  }


  // 서버 연결 시점 전에 이미 로그를 남겨야 하냐 여부도 갱신받아야 하므로.
  owner_->intra_logging_on_ = intra_logging_on;

  // TCP nagle 알고리즘을 선택적으로 on/off 합니다.
  owner_->Get_ToServerTcp()->SetEnableNagleAlgorithm(owner_->settings_.bEnableNagleAlgorithm);

  // 1. AES key는 공개키로 암호화한다.
  // 2. RC4 key는 AES key로 암호화한다.

  //fun_check(owner_->to_server_udp_fallbackable_->server_ip.IsUnicast());
  ByteArray encrypted_aes_key_blob;
  ByteArray encrypted_rc4_key;

  // AES Key 는 공개키로 암호화하고, RC4 Key는 AES Key로 암호화한다.
  if (!CryptoRSA::CreateRandomBlock(strong_random_block, owner_->settings_.strong_encrypted_message_key_length) ||
      !CryptoAES::ExpandFrom(owner_->ToServerSessionKey.aes_key, (const uint8*)strong_random_block.ConstData(), owner_->settings_.strong_encrypted_message_key_length / 8) ||
      !CryptoRSA::EncryptSessionKeyByPublicKey(encrypted_aes_key_blob, strong_random_block, public_key_blob_) ||

      !CryptoRSA::CreateRandomBlock(weak_random_block, owner_->settings_.weak_encrypted_message_key_length) ||
      !CryptoRC4::ExpandFrom(owner_->ToServerSessionKey.rc4_key, (const uint8*)weak_random_block.ConstData(), owner_->settings_.weak_encrypted_message_key_length / 8) ||
      !CryptoAES::Encrypt(owner_->ToServerSessionKey.aes_key, weak_random_block, encrypted_rc4_key)) {
    owner_->EnqueueDisconnectionEvent(ResultCode::EncryptFail, ResultCode::TCPConnectFailure);
    SetState(State::Disconnecting);

    if (owner_->callbacks_) {
      owner_->callbacks_->OnException(HostId_None, Exception("Failed to create session-key"));
    }
    return;
  }

  MessageOut response;
  lf::Write(response, MessageType::NotifyCSEncryptedSessionKey);
  lf::Write(response, encrypted_aes_key_blob);
  lf::Write(response, encrypted_rc4_key);
  owner_->Get_ToServerTcp()->SendWhenReady(SendFragRefs(response), TcpSendOption());
}

void NetClientWorker::ProcessMessage_NotifyProtocolVersionMismatch(MessageIn& msg) {
  owner_->EnqueueConnectFailEvent(ResultCode::ProtocolVersionMismatch);
  SetState(State::Disconnecting);
}

void NetClientWorker::ProcessMessage_NotifyServerDeniedConnection(MessageIn& msg) {
  owner_->EnqueueConnectFailEvent(ResultCode::NotifyServerDeniedConnection);
  SetState(State::Disconnecting);
}

void NetClientWorker::UdpSendCompletionCase(UdpSocket_C* udp_socket,
                                            CompletionStatus& completion) {
  CScopedLock2 owner_guard(owner_->GetMutex());

  udp_socket->send_issued_ = false;

  // Restore ttl
  if (udp_socket->socket_) {
    udp_socket->socket_->RestoreTtlOnCompletion();
  }

  // Stats
  if (completion.completed_length > 0) {
    owner_->stats_.total_udp_send_count++;
    owner_->stats_.total_udp_send_bytes += completion.completed_length;
  }

  // 보낼게 있으면 즉시 send issue를 건다.
  udp_socket->ConditionalIssueSend();
}

void NetClientWorker::TcpSendCompletionCase(CompletionStatus& completion) {
  CScopedLock2 owner_guard(owner_->GetMutex());

  owner_->Get_ToServerTcp()->send_issued_ = false;

  if (completion.completed_length < 0) { //0은 허용?
    // TCP 연결이 끊어졌음을 의미한다. 따라서 에러 처리한다.
    owner_->EnqueueDisconnectionEvent(ResultCode::DisconnectFromRemote, ResultCode::TCPConnectFailure, "completed_length < 0 SendComplete");
    //Disconnecting으로 상태를 전환하지 않아도 되나??
  } else {
    if (completion.completed_length > 0) {
      // 송신 큐에서 완료된 만큼의 데이터를 제거한다. 그리고 다음 송신을 건다.
      // (최종 completion 상황이라 하더라도 이 과정은 필수!)
      owner_->Get_ToServerTcp()->send_queue_.DequeueNoCopy(completion.completed_length);

      // Stats
      owner_->stats_.total_tcp_send_bytes += completion.completed_length;
      //owner_->RecentBackedUpStats.total_tcp_send_bytes = owner_->stats_.total_tcp_send_bytes;
    }

    // 이 루틴이 실행중일떄는 아직 owner가 건재함을 전제한다.
    // 이 비교가 불필요할 듯. socket is NOT closed and 'Disconnecting' state인 경우 난감한데다
    // ConditionalIssueSend 자체가 dead socket이면 skip하므로.
    //if (owner_->worker_->GetState() != NetClientWorker::Disconnecting) {
    owner_->Get_ToServerTcp()->ConditionalIssueSend();
    //}
  }
}

bool NetClientWorker::LoopbackRecvCompletionCase() {
  // 어차피 클라는 1개 스레드이므로 이렇게 해도 문제없음. 서버는 이렇게 함.
  CScopedLock2 owner_guard(owner_->GetMutex());

  if (!owner_->loopback_final_recv_message_queue_.IsEmpty()) {
    // 수신 큐에서 받은 데이터를 꺼낸 후 ...
    ReceivedMessage received_msg;
    received_msg.remote_id = owner_->local_host_id_;
    received_msg.unsafe_message = owner_->loopback_final_recv_message_queue_.CutFront();
    received_msg.unsafe_message.Seek(0);
    //received_msg.action_time = owner_->GetAbsoluteTime();
    owner_->pre_final_recv_queue.Enqueue(received_msg);

    // Final recv queue로 옮기거나 여기서 처리한다.
    ProcessEveryMessageOrMoveToFinalRecvQueue(nullptr);
    return true;
  }

  return false;
}

void NetClientWorker::TcpRecvCompletionCase(CompletionStatus& completion) {
  CScopedLock2 owner_guard(owner_->GetMutex());

  owner_->Get_ToServerTcp()->recv_issued_ = false;

  // 수신의 경우 0바이트 수신했음 혹은 음수바이트 수신했음이면 연결에 문제가 발생한 것이므로 디스해야 한다.
  if (completion.completed_length <= 0) {
    // GetLastError도 남긴다.
    //TODO Completion에 에러코드가 같이 올터인데??
    const DWORD last_error = ::GetLastError();

    if (owner_->worker_->GetState() < NetClientWorker::State::Disconnecting) {
      // TCP 연결이 끊어졌음을 의미한다. 따라서 에러 처리한다.
      const String text = String::Format("completed_length(%d) <= 0 RecvComplete, comp_error_code: %d, last_error: %u, socket_error: %d",
                      completion.completed_length, (int32)completion.socket_error, last_error, (int32)completion.socket_error);
      owner_->EnqueueDisconnectionEvent(ResultCode::DisconnectFromRemote, ResultCode::TCPConnectFailure, text);
      SetState(State::Disconnecting);
    }

    if (owner_->IsIntraLoggingOn()) {
      const String text = String::Format("[Error] completed_length(%d) <= 0 RecvComplete, last_error: %u, socket_error: %d",
                    completion.completed_length, last_error, (int32)completion.socket_error);
      owner_->IntraLogToServer(LogCategory::System, *text);
    }
  } else if (owner_->Get_ToServerTcp()->socket_->IsClosed()) {
    if (GetState() < State::Disconnecting) {
      owner_->EnqueueDisconnectionEvent(ResultCode::DisconnectFromRemote, ResultCode::TCPConnectFailure, "Close socket completion");
      SetState(State::Disconnecting);
    }
  } else {
    fun_check(owner_->to_server_tcp_);

    owner_->last_tcp_stream_recv_time_ = owner_->GetAbsoluteTime(); // 타임스탬핑
    owner_->Get_ToServerTcp()->recv_stream_.EnqueueCopy(owner_->Get_ToServerTcp()->socket_->GetRecvBufferPtr(), completion.completed_length); //TODO 복사가 불필요해보이는데, 차후에 개선하도록 하자.
    owner_->stats_.total_tcp_recv_bytes += completion.completed_length; // Stats

    // 완전한 msg가 도착한 것들을 모두 추려서 final recv queue로 옮기거나 여기서 처리한다.
    ReceivedMessageList extracted_msg_list;
    owner_->ExtractMessagesFromTcpStream(extracted_msg_list);

    for (auto& msg : extracted_msg_list) {
      //msg.action_time = owner_->GetAbsoluteTime();
      owner_->pre_final_recv_queue.Enqueue(msg);
    }

    ProcessEveryMessageOrMoveToFinalRecvQueue(nullptr);

    // 메시지 처리중에 TCP 연결이 끊어지지 않음을 보장.
    fun_check(owner_->to_server_tcp_);
  }

  // 다음 recv를 건다.  여기까지 왔을떄는 시스템이 파괴되지 않고 건재함을 전제해야 한다.
  if (owner_->worker_->GetState() < NetClientWorker::State::Disconnecting) {
    const SocketErrorCode socket_error = owner_->Get_ToServerTcp()->IssueRecvAndCheck();
    if (socket_error != SocketErrorCode::Ok) {
      const String text = String::Format("%d TCP IssueRecv failed socket_error", (int32)socket_error);
      owner_->EnqueueDisconnectionEvent(ResultCode::TCPConnectFailure, ResultCode::Unexpected, text);
      SetState(State::Disconnecting);
    }
  }
}

void NetClientWorker::ProcessMessageOrMoveToFinalRecvQueue( UdpSocket_C* udp_socket,
                                                            ReceivedMessage& received_msg) {
  fun_check(received_msg.unsafe_message.AtBegin());
  ProcessMessage_EngineLayer(udp_socket, received_msg);
}

// UnreliableRelay1(sender RemoteClient) -> Server -> UnreliableRelay2(Server) -> local
void NetClientWorker::ProcessMessage_UnreliableRelay2(UdpSocket_C* udp_socket, ReceivedMessage& received_msg) {
  // 서버로부터 온 메시지가 아니면 무시한다.
  if (received_msg.remote_id != HostId_Server) {
    TRACE_SOURCE_LOCATION();
    return;
  }

  owner_->LockMain_AssertIsLockedByCurrentThread();

  auto& msg = received_msg.unsafe_message;

  HostId sender_id;
  OptimalCounter32 content_length;
  if (!lf::Reads(msg,  sender_id, content_length)) {
    TRACE_SOURCE_LOCATION();
    return;
  }

  if (content_length < 0 || content_length >= NetConfig::message_max_length) {
    TRACE_SOURCE_LOCATION();
    return;
  }

  fun_check(msg.GetReadableLength() == content_length);
  MessageIn content;
  if (!msg.ReadAsShared(content, content_length)) {
    TRACE_SOURCE_LOCATION();
    return;
  }

  auto peer = owner_->GetPeerByHostId(sender_id);
  if (peer && !peer->garbaged_) {
    ReceivedMessage received_msg2;
    received_msg2.unsafe_message = content; // share
    received_msg2.relayed = true;
    received_msg2.remote_id = sender_id;
    //received_msg2.action_time = owner_->GetAbsoluteTime();

    ProcessMessageOrMoveToFinalRecvQueue(udp_socket, received_msg2);
  }
}

//FIXME ReadAsShared 이 함수가 정상동작하는건지???
//이 함수는 문제가 없어보이고, 호출 이후 버퍼의 소유권의 문제인건지??
//좀더 쫓아가서 확인을 해봐야할것으로 보임.
void NetClientWorker::ProcessMessage_ReliableRelay2(MessageIn& msg) {
  owner_->LockMain_AssertIsLockedByCurrentThread();

  HostId sender_id;
  FrameNumber frame_number;
  OptimalCounter32 content_length;
  if (!lf::Reads(msg,  sender_id, frame_number, content_length)) {
    TRACE_SOURCE_LOCATION();
    return;
  }

  if (content_length < 0 || content_length >= owner_->settings_.message_max_length) {
    TRACE_SOURCE_LOCATION();
    return;
  }

  fun_check(msg.GetReadableLength() == content_length);
  MessageIn content;
  if (!msg.ReadAsShared(content, content_length)) {
    TRACE_SOURCE_LOCATION();
    return;
  }

  auto peer = owner_->GetPeerByHostId(sender_id);
  if (peer && !peer->garbaged_ && !peer->to_peer_rudp.panic_) {
    RUdpFrame long_frame;
    RUdpHelper::BuildRelayed2LongDataFrame(frame_number, SendFragRefs(content), long_frame);

    ResultCode extract_result;
    ReceivedMessageList received_msg_list;
    peer->to_peer_rudp.EnqueueReceivedFrameAndGetFlushedMessages(long_frame, received_msg_list, extract_result);
    if (extract_result == ResultCode::Ok) {
      for (auto& received_msg : received_msg_list) {
        received_msg.relayed = true;
        //received_msg.action_time = owner_->GetAbsoluteTime();
        //TODO 이게 구지 필요한가??
        //received_msg.unsafe_message.Seek(0);

        // TCP로 수신 받은 메시지들이므로, 첫번째 인자인 UdpSocket은 null로 지정.
        ProcessMessageOrMoveToFinalRecvQueue(nullptr, received_msg);
      }
    } else {
      owner_->EnqueueError(ResultInfo::From(extract_result, peer->host_id_, "Stream extract error at reliable UDP"));
      //SetState(State::Disconnecting);
    }
  }
}

// Client -> LingerDataFrame1 -> Server -> LingerDataFrame2 -> Peer
void NetClientWorker::ProcessMessage_LingerDataFrame2(UdpSocket_C* udp_socket,
                                                      ReceivedMessage& received_msg) {
  // 서버에서 보낸 경우에만 제대로 처리한다.
  if (received_msg.remote_id != HostId_Server) {
    TRACE_SOURCE_LOCATION();
    return;
  }

  owner_->LockMain_AssertIsLockedByCurrentThread();

  //const HostId remote = received_msg.remote_id;
  auto& msg = received_msg.unsafe_message;

  // Read fields
  HostId sender_remote_host_id;
  FrameNumber frame_number;
  OptimalCounter32 frame_length;
  if (!lf::Reads(msg,  sender_remote_host_id, frame_number, frame_length)) {
    TRACE_SOURCE_LOCATION();
    return;
  }

  // 먼저, 유효한 갈이인지부터 체크후 유효하지 않을 경우 무시한다.
  // 그냥 MessageIn::ReadAsShared() 함수에서 같이 처리해버리는건 어떨런지?
  // 아니면, 아래처럼 별도의 길이 체크용 함수를 하나 만들어서 사용하는것도 좋을듯 싶다.
  //if (!msg.IsValidMessageLength(frame_length))
  if (frame_length < 0 || frame_length >= owner_->settings_.message_max_length) {
    TRACE_SOURCE_LOCATION();
    return;
  }

  //Hack or engine bug.
  fun_check(msg.GetReadableLength() == frame_length); // 정확히 맞아떨어져야함. (부족해도 남아도 안됨)

  //ReadRawBytes 함수를 통해서 복사를 할 경우에는 문제가 없으나,
  //ReadAsShared 함수를 통해서 복사 없이 처리할 경우에는 문제가 있음.
  //-> ByteArray(const char*, int32)에서 문자열에 '\0'이 있을 경우, 빈문자열로 할당되던 코드가 있어서 발생했었음.

  MessageIn frame_data;
  if (!msg.ReadAsShared(frame_data, frame_length)) {
  //ByteArray frame_data(frame_length,NoInit);
  //if (!msg.ReadRawBytes(frame_data.MutableData(),frame_length)) {
    TRACE_SOURCE_LOCATION();
    return;
  }

  // Reliable UDP 프레임을 꺼내서 처리한다.
  auto peer = owner_->GetPeerByHostId(sender_remote_host_id);
  if (peer && !peer->garbaged_ && !peer->to_peer_rudp.panic_) {
    RUdpFrame frame;
    frame.type = RUdpFrameType::Data;
    frame.frame_number = frame_number;
    //TODO 바로쓰고 버릴것이므로, raw로 넘기도록하자.
    //frame.data = frame_data.ToReadableBytesCopy(); //TODO frame.Data를 CMessageIn형태로 변경해주다면, 복사를 제거할 수 있을듯.  현재는 frame.Data가 ByteArray 타입임.
    //frame.data = frame_data.ToReadableBytesRaw();
    //recevier windows에서 참조를 가지고 유지했다가 해당 프레임이 왔을 경우 스트림에 반영되므로,
    //복사를 해서 넘겨주어야함.
    //frame.data = ByteArray::FromRawData((const char*)frame_data.GetReadableData(), frame_data.GetReadableLength());
    //frame.data = frame_data;

    //주의:
    //recevier windows에서 참조를 가지고 유지했다가 해당 프레임이 왔을 경우 스트림에 반영되므로,
    //복사를 해서 넘겨주어야함.
    //frame.data = frame_data;
    frame.data = frame_data.ToReadableBytesCopy();

    ReceivedMessageList received_msg_list;
    ResultCode extract_result;
    peer->to_peer_rudp.EnqueueReceivedFrameAndGetFlushedMessages(Frame, received_msg_list, extract_result);
    if (extract_result == ResultCode::Ok) {
      for (auto& received_msg : received_msg_list) {
        received_msg.relayed = true;
        //received_msg.action_time = owner_->GetAbsoluteTime();
        ProcessMessageOrMoveToFinalRecvQueue(udp_socket, received_msg);
      }
    } else {
      TRACE_SOURCE_LOCATION();
      owner_->EnqueueError(ResultInfo::From(extract_result, peer->host_id_, "Stream extract error at reliable UDP"));
      //SetState(State::Disconnecting);
    }
  }
}

// 서버에서 와야 하는 메시지인데 P2P로 도착한 경우
//
// (혹은 아직 서버와의 연결 처리가 다 끝나지 않은 상태이면 HostId_None에서 오는 경우일테고 이때는 무시)
bool NetClientWorker::IsFromRemoteClientPeer(ReceivedMessage& received_msg) const {
  return received_msg.remote_id != HostId_Server && received_msg.remote_id != HostId_None;
}

bool NetClientWorker::Main_IssueConnect(SocketErrorCode& out_socket_error) {
  const SocketErrorCode socket_error = owner_->Get_ToServerTcp()->socket_->Connect(owner_->connect_args_.server_ip, owner_->connect_args_.server_port);
  if (socket_error == SocketErrorCode::Ok ||
      socket_error == SocketErrorCode::WouldBlock) {
    out_socket_error = SocketErrorCode::Ok;
    return true;
  } else {
    out_socket_error = socket_error;
    return false;
  }
}

// 에러가 났거나 타임아웃 상황
void NetClientWorker::Heartbeat_ConnectFailCase(SocketErrorCode socket_error) {
  // CloseSocket 후 Disconnecting mode로 전환
  owner_->Get_ToServerTcp()->socket_->SetBlockingMode(true);
  owner_->Get_ToServerTcp()->socket_->SetVerboseFlag(true);

  // close, equeue notify event and stop thread
  owner_->EnqueueConnectFailEvent(ResultCode::TCPConnectFailure, socket_error);
  SetState(State::Disconnecting);
}

// 최초 TCP issue를 건다.
void NetClientWorker::IssueTcpFirstRecv() {
  CScopedLock2 owner_guard(owner_->GetMutex());

  const SocketErrorCode socket_error = owner_->Get_ToServerTcp()->IssueRecvAndCheck();
  if (socket_error != SocketErrorCode::Ok) { //@note 첫번째이므로, wouldblock가 온다면 그것 또한 의미 없으리..
    owner_->EnqueueConnectFailEvent(ResultCode::TCPConnectFailure, socket_error);
    SetState(State::Disconnecting);
  }
}

void NetClientWorker::Heartbeat_IssueConnect() {
  if (owner_->IsIntraLoggingOn()) {
    owner_->IntraLogToServer(LogCategory::System, "Networker thread is started.");
  }

  // Issue UDP first recv
  //owner_->Get_ToServerUdpSocket()->ConditionalIssueRecvFrom();

  // Create TCP socket and TCP blocked connect to server socket
  //
  // winsock connect()는 TCP 연결 제한시간이 1초이다. 이를 API 딴에서 고칠 방법은 없다.
  // 따라서 TCP connect()를 수차례 반복후 안되면 즐쳐줘야 한다.
  // 통상적인 소켓은 1초지만 어떤 유저들은 이 값을 윈도 레지스트리를 통해 바꿔놓으므로 타임 기준으로 체크한다.
  fun_check(owner_->Get_ToServerTcp());
  fun_check(owner_->Get_ToServerTcp()->socket);

  if (!owner_->Get_ToServerTcp()->socket_->Bind()) {
    owner_->EnqueueError(ResultInfo::From(ResultCode::TCPConnectFailure, HostId_None, "Cannot bind TCP socket to a local address."));
    Heartbeat_ConnectFailCase((SocketErrorCode)WSAGetLastError());
    return;
  } else {
    // Bind가 성공했다. 일단 IP는 ANY이겠지만 port 번호값이라도 얻어두자.
    owner_->Get_ToServerTcp()->RefreshLocalAddress();
  }

  // 연결 시작 시각 기록
  connect_issued_time_ = owner_->GetAbsoluteTime(); // clock_.AbsoluteSeconds();

  // 소켓 기본 설정
  const bool old_verbose = owner_->Get_ToServerTcp()->socket_->GetVerboseFlag();
  owner_->Get_ToServerTcp()->socket_->SetVerboseFlag(false); //소켓 경고를 잠시 꺼준다.
  owner_->Get_ToServerTcp()->socket_->SetBlockingMode(false); // 소켓을 넌블럭킹 모드로 바꾼 후 연결 시도를 한다. 연결 시도가 끝나면 블럭킹 모드로 다시 바꾼다.
  owner_->Get_ToServerTcp()->socket_->SetVerboseFlag(old_verbose);

  // 연결 시도를 건다.
  SocketErrorCode socket_error;
  if (Main_IssueConnect(socket_error)) {
    // 연결 완료 대기 상태로 전환.
    SetState(State::Connecting);
  } else {
    Heartbeat_ConnectFailCase(socket_error);
  }
}

void NetClientWorker::Heartbeat_Connecting() {
  const double T1 = owner_->GetAbsoluteTime();

  if ((T1 - connect_issued_time_) > NetConfig::tcp_socket_connect_timeout_sec) {
    // 연결 대기시간이 초과 되었음.
    Heartbeat_ConnectFailCase(SocketErrorCode::Timeout);
  } else {
    //TODO polling(select)으로 대체하도록 하자.

    //Poll에 Write/Error를 감지하면 될터??

    InternalSocketSelectContext select_context;
    select_context.AddWriteWaiter(*owner_->Get_ToServerTcp()->socket);
    select_context.AddExceptionWaiter(*owner_->Get_ToServerTcp()->socket);

    // 기다리지 않고 폴링한다. 어차피 이 함수는 타이머에 의해 일정 시간마다 호출되니까.
    select_context.Wait(0);

    SocketErrorCode socket_error;
    const bool did = select_context.GetConnectResult(*owner_->Get_ToServerTcp()->socket, socket_error);
    if (did) {
      if (socket_error == SocketErrorCode::Ok) {
        // 연결 완료!

        owner_->Get_ToServerTcp()->socket_->SetBlockingMode(true); //@todo blocking?? (연결중일때만 Non-blocking으로 함)

        // 연결이 정상적으로 완료 되었으니,
        // 이제 로컬 IP주소도 얻자. 127.0.0.1 아니면 연결된 NIC의 주소가 나올 것이다.
        owner_->Get_ToServerTcp()->RefreshLocalAddress();

        if (!owner_->Get_ToServerTcp()->local_addr.IsUnicast()) {
          LOG(LogNetEngine, Warning, "서버와 TCP 연결이 되었는데도 로컬 소켓 주소를 얻을 수 없다니!");
        }

        // 연결되었음으로 상태 전환.
        SetState(State::JustConnected);
      } else {
        //TODO WSAENETUNREACH일 경우에는 재시도가 필요없을터인데??

        //char msg[1024];
        //LOG(LogNetEngine, Warning, "connection failure: %s", CPlatformMisc::GetSystemErrorMessage(msg, countof(msg), (int32)socket_error));

        //@workaround
        // nonblock socket의 connect에서는 blocked socket에서는 없던
        // '아직 연결 결과가 안나왔는데도 연결 실패라고 오는' 경우가
        // 종종 있다. 이런 것도 이렇게 막아야 한다.

        //if (!Main_IssueConnect(socket_error)) {
        //  Heartbeat_ConnectFailCase(socket_error);
        //}
      }
    } else {
      // Waiting.. pending..
    }
  }
}

// 갇 연결되었을때 호출됨. 연결된 후 수행할 초기화 수행.
void NetClientWorker::Heartbeat_JustConnected() {
  owner_->SetInitialTcpSocketParameters();

  //@note
  // ConnectionHint를 요청한다.
  // 클라이언트가 서버에게 보내는 최초 메시지
  // 이 메시지를 받은 서버는 접속 정보등에 관련해서 알려주게 된다. (서버내 설정등을 동기화)
  MessageOut msg;
  lf::Write(msg, MessageType::RequestServerConnectionHint);
  //TODO 실제 플랫폼에 맞추어서 넣어주어야함. 일단은 Windows로 고정함.
  lf::Write(msg, RuntimePlatform::Windows);

  owner_->Get_ToServerTcp()->SendWhenReady(SendFragRefs(msg), TcpSendOption());

  IssueTcpFirstRecv();            // 완료 결과를 통보 받기 위해서, 최초 Recv를 걸어줌.
  owner_->PerformGarbageCollection();  // 쓸모없는 객체들 정리.
  SetState(State::Connected);     // 상태를 연결됨으로 변경. (정말로 연결되었음!)
}

void NetClientWorker::Heartbeat_Connected() {
  CScopedLock2 owner_guard(owner_->GetMutex());

  owner_->PerformGarbageCollection();

  // 예전에는 Heartbeat_ConnectedCase가 아래 while 문 안에 있었으나, 그게 아닌 듯 하다.
  // 아래는 더 이상 수신된 시그널이 없을 때까지 도는 루프니까.
  Heartbeat_ConnectedCase();

  // Nat 장치 이름을 얻어옴. (바로 얻어오지 못하고, 주기적으로 조회해서 가져와야함.)
  Heartbeat_DetectNatDeviceName();

  while (true) {
    //TODO 루프를 돌면서 처리하는데, 여기서 시간이 흘러갈란가? owner_->GetAbsoluteTime()이 확실히 리얼타이인지?
    //아니면, Heartbeat시에만 갱신되는 steppedtime인지 명확히 할 필요가 있어보인다.
    //const double absolute_time = owner_->GetMorePrecisionAbsoluteTime();//owner_->GetAbsoluteTime();
    const double absolute_time = owner_->GetAbsoluteTime();

    // 서버와의 연결 해제 시작 상황이 된 후 서버와 오랫동안 연결이 안끊어지면 강제 스레드 종료를 한다.
    // 접속해제를 요청한(NetClient.Disconnect() 함수 호출) 시각으로부터 오랜 시간동안 응답이 없으면
    // 바로 종료 절차로 넘어가도록 합니다.
    // 대기시간이 있는 이유는, 정상적으로 TCP 접속을 서버가 인지한 후에 안전하게 끊을 수 있도록 하기
    // 위함인데, 스트레스 테스팅을 하는 과정에서는 지연만 생길 수 있으므로, graceful_disconnect_timeout_ 값을
    // 0으로 하면 지연 없이 접속이 해제됩니다.
    if (shutdown_issued_time_ > 0) {
      const double elapsed_time = (absolute_time - shutdown_issued_time_);
      if (elapsed_time > graceful_disconnect_timeout_) {
        if (graceful_disconnect_timeout_ > 0.0) {
          LOG(LogNetEngine,Warning,"Graceful disconnection timedout.  elapsed: %fsec", elapsed_time);
        }

        SetState(State::Disconnecting);
        return;
      }
    }

    const bool anything_did = LoopbackRecvCompletionCase();

    // 상태가 Disconnecting으로 변경 되었다면, 루프를 바로 탈출합니다.
    if (GetState() == State::Disconnecting) {
      return;
    }

    // 이벤트가 발생한게 있으면 리턴하지 말고 재시도한다. 더이상 없을때까지!
    // (이렇게 안하면 클라에서 메시지 처리 성능이 급격히 떨어진다)
    //
    //@note 즉, 한번에 처리할 수 있는 양까지 모두 처리하고 루프를 빠져나가야
    //      밀리지 않는다.
    if (!anything_did) {
      return;
    }
  }
}

void NetClientWorker::Heartbeat_Disconnecting() {
  CScopedLock2 owner_guard(owner_->GetMutex());

  disconnecting_mode_heartbeat_count_++;

  owner_->ConditionalDeleteUPnPTcpPortMapping();

  // 첫 1회인 경우 모든 소켓을 닫는다.
  if (disconnecting_mode_heartbeat_count_ == 1) {
    LOG(LogNetEngine,Warning,"%s",__FUNCTION__);

    owner_->LockMain_AssertIsLockedByCurrentThread();

    if (owner_->Get_ToServerUdpSocket()) { // UDP 소켓은 일단 바로 끊어버린다. (소켓 객체만 닫아줌!)
      owner_->Get_ToServerUdpSocket()->socket_->CloseSocketHandleOnly();
    }

    // Per-peer UDP socket들도 모두 끊는다.
    {
      while (!owner_->remote_peers_.IsEmpty()) {
        auto it = owner_->remote_peers_.CreateIterator();
        owner_->RemovePeer(it->value);
      }

      owner_->AllClearRecycleToGarbage();

      owner_->AllClearGarbagePeer();

      fun_check(owner_->remote_peers_.IsEmpty());
    }

    // TCP 소켓도 바로 끊어야 한다.
    // 소켓을 끊으면서 async IO가 모두 종료되는데, 이것부터 먼저 한 후에 worker thread unregister를
    // 하는 것이 안전하기 때문이다.

    if (owner_->Get_ToServerTcp()) {
      // owner_->Get_ToServerTcp()->socket_->Shutdown(ShutdownFlag_Both);
      // 이거 안해도 된다. 이미 일전에 RPC를 통해 shutdown을 서버에 노티했으니까.
      owner_->Get_ToServerTcp()->socket_->CloseSocketHandleOnly();

      if (owner_->IsIntraLoggingOn()) {
        owner_->IntraLogToServer(LogCategory::UDP, "Heartbeat_Disconnecting, CloseSocketHandleOnly called.");
      }
    }
  }

  owner_->PerformGarbageCollection();

  owner_->GetMutex().AssertIsLockedByCurrentThread();

  //@todo enum으로 대체해서 좀더 명확하게 하는게 좋을듯..
  int32 unsafe_disconnect_reason = 0;

  // Async io가 모두 끝난게 확인될 떄까지 disconnecting mode 지속.
  // 어차피 async io는 이 스레드 하나에서만 이슈하기 떄문에 이 스레드 issue 없는 상태인 것을 확인하면 더 이상 async io가 진행중일 수 없다.
  bool safe_disconnect = true;
  if (!owner_->Garbages.IsEmpty()) {
    unsafe_disconnect_reason = 1;
    safe_disconnect = false;
  }
  else if (owner_->Get_ToServerUdpSocket() &&
          owner_->Get_ToServerUdpSocket()->socket_ &&
          (!owner_->Get_ToServerUdpSocket()->socket_->IsClosed() || owner_->Get_ToServerUdpSocket()->send_issued_ || owner_->Get_ToServerUdpSocket()->recv_issued_)) {
    unsafe_disconnect_reason = 2;
    safe_disconnect = false;
  }
  else if (owner_->Get_ToServerTcp() &&
          owner_->Get_ToServerTcp()->socket_ &&
          (!owner_->Get_ToServerTcp()->socket_->IsClosed() || owner_->Get_ToServerTcp()->send_issued_ || owner_->Get_ToServerTcp()->recv_issued_)) {
    unsafe_disconnect_reason = 3;
    safe_disconnect = false;
  }

  if (safe_disconnect) {
    // 모든 컨텍스트를 초기화한다.
    owner_->more_precision_clock_.Stop();

    owner_->CleanupEvenUnstableSituation(false); // 아직 처리되지 않은 패킷들은 지우지 않음.

    fun_check(owner_->remote_peers_.IsEmpty());

    // FinalUserWorkItemList_RemoveReceivedMessagesOnly(); 이걸 하지 말자.
    // 서버측에서 RPC직후추방시 클라에서는 수신RPC를 폴링해야하는데 그게 증발해버리니까. 대안을 찾자.
    // 이러면 disconnected 상황이 되어도 미수신 콜백을 유저가 폴링할 수 있다.
    // 이래야 서버측에서 디스 직전 보낸 RPC를 클라가 처리할 수 있다.

    //if (owner_->IsIntraLoggingOn()) {
    //  owner_->IntraLogToServer(LogCategory::System, "Networker thread is stopped.");
    //}

    SetState(State::Disconnected);
  }

  //@todo 제대로 처리하지 못했음. FunNet report센터로 보냄.
  else if ((owner_->GetAbsoluteTime() - disconnecting_mode_start_time_) > 5 && !disconnecting_mode_warned_) {
    ErrorReporter::Report(String::Format("Too long time elapsed since disconnecting mode.  unsafe_disconnect_reason: %d", unsafe_disconnect_reason));

    disconnecting_mode_warned_ = true;
  }
}

// 락을 한 상태에서 처리
void NetClientWorker::Heartbeat_Connected_AfterLock() {
}

// 서버 연결 과정의 마지막 메시지를 쏜다.
// 이 메시지를 받은 서버는, ProtocolVersion / InternalVersion을
// 체크 후 최종 인증을 할지 여부를 판단하게됨.
void NetClientWorker::ProcessMessage_NotifyCSSessionKeySuccess(MessageIn& msg) {
  owner_->LockMain_AssertIsLockedByCurrentThread();

  MessageOut request_msg;
  lf::Write(request_msg, MessageType::NotifyServerConnectionRequestData);
  lf::Write(request_msg, owner_->connect_args_.user_data);
  lf::Write(request_msg, owner_->connect_args_.protocol_version);
  lf::Write(request_msg, owner_->internal_version_);
  owner_->Get_ToServerTcp()->SendWhenReady(SendFragRefs(request_msg), TcpSendOption());
}

void NetClientWorker::ProcessMessage_P2PIndirectServerTimeAndPing(ReceivedMessage& received_msg) {
  auto& msg = received_msg.unsafe_message;

  double client_local_time;
  if (!lf::Read(msg, client_local_time)) {
    TRACE_SOURCE_LOCATION();
    return;
  }

  CScopedLock2 owner_guard(owner_->GetMutex());

  auto peer = owner_->GetPeerByUdpAddr(received_msg.remote_addr_udp_only);
  if (!peer || peer->host_id_ == HostId_Server) { //??? RP에 서버가 들어갈 수 있나?? (서버가 P2P멤버로 참여 가능한 설정이 활성화 되었을 경우에는 가능?)
    TRACE_SOURCE_LOCATION();
    return;
  }

  MessageOut pong;
  lf::Write(pong, MessageType::P2PIndirectServerTimeAndPong);
  lf::Write(pong, client_local_time);
  peer->to_peer_udp.SendWhenReady(SendFragRefs(pong), UdpSendOption(MessagePriority::Ring0, EngineOnlyFeature));
}

void NetClientWorker::ProcessMessage_P2PIndirectServerTimeAndPong(ReceivedMessage& received_msg) {
  auto& msg = received_msg.unsafe_message;

  double client_old_local_time;
  if (!lf::Read(msg, client_old_local_time)) {
    TRACE_SOURCE_LOCATION();
    return;
  }

  CScopedLock2 owner_guard(owner_->GetMutex());

  auto peer = owner_->GetPeerByUdpAddr(received_msg.remote_addr_udp_only);
  if (!peer || peer->host_id_ == HostId_Server) {
    TRACE_SOURCE_LOCATION();
    return;
  }

  // 피어와의 핑만 계산한다.
  const double absolute_time = owner_->more_precision_clock_.AbsoluteSeconds();
  const double rtt = (absolute_time - client_old_local_time);
  const double lag = rtt * 0.5;

  // 마지막 ping값 갱신
  peer->last_ping_ = lag;

  if (peer->recent_ping_ > 0) { // second or
    // 최근 핑값 계산(완만하게 적용됨)
    peer->recent_ping_ = MathBase::Lerp(peer->recent_ping_, lag, NetConfig::log_linear_programming_factor);
  } else { // First time
    peer->recent_ping_ = lag;
  }

  peer->last_direct_udp_packet_recv_time_ = owner_->GetAbsoluteTime();
  peer->direct_udp_packet_recv_count_++;
}

// 주기적으로 주고 받고 있음.  시간계산(핑타임등)
void NetClientWorker::ProcessMessage_ReplyServerTime(MessageIn& msg) {
  double client_old_local_time;   // 서버가 마지막에 통보 받았던 클라이언트 시간
  double server_local_time;       // 서버가 보낸 시점에서의 서버시간 (과거)
  if (!lf::Reads(msg,  client_old_local_time, server_local_time)) {
    TRACE_SOURCE_LOCATION();
    return;
  }

  CScopedLock2 owner_guard(owner_->GetMutex());

  // 새로 server lag을 계산한 후 클라이언트와의 시간차도 구한다.

  //TODO 리얼타임을 써야하는지, owner_->GetAbsoluteTime()를 써야하는지 좀 헷갈림...
  //꼭 리얼타임을 써야함!!
  //const double client_time = owner_->GetAbsoluteTime();//clock_.AbsoluteSeconds();
  const double client_time = owner_->more_precision_clock_.AbsoluteSeconds();
  const double rtt = (client_time - client_old_local_time);
  const double lag = rtt * 0.5;
  owner_->server_udp_last_ping_ = lag;

  if (owner_->server_udp_recent_ping_ != 0) {
    // 튐(jerky) 현상을 보완하고자, 바로 대입하지 않고 선형보간을
    // 적용해주어 민감하게 반응하지 않도록한다.
    owner_->server_udp_recent_ping_ = MathBase::Lerp(owner_->server_udp_recent_ping_, lag, NetConfig::log_linear_programming_factor);
  } else {
    // 처음 값을 설정하는 것이므로, 그냥 어싸인함.
    owner_->server_udp_recent_ping_ = lag;
  }

  const double server_time = server_local_time + owner_->server_udp_recent_ping_;
  owner_->dx_server_time_diff_ = client_time - server_time;

  // Enqueue local event
  LocalEvent event(LocalEventType::SynchronizeServerTime);
  event.remote_id = HostId_Server;
  owner_->EnqueueLocalEvent(event);
}

void NetClientWorker::SetState(State new_state) {
  if (new_state != state_ && new_state > state_) { // 뒤 상태로는 이동할 수 없어야 한다.
    disconnecting_mode_heartbeat_count_ = 0;
    state_ = new_state;
    disconnecting_mode_start_time_ = owner_->GetAbsoluteTime();
    disconnecting_mode_warned_ = false;
  }
}

//FIXME
//  현재 유저 RPC가 하나도 없음애도 불구하고, 유저 RPC 호출이 발생하고 있음.
//  어디선가 메시지 오프셋이 꼬인듯 싶은데...

// 얘는 네트워크 스레드임
//
// 내부 RPC에서 처리가 되지 않았을 경우에는 유저 스레드에서 처리할 수 있도록
// 유저스레드 큐에 큐잉함(forwarding).
void NetClientWorker::ProcessMessage_RPC( ReceivedMessage& received_msg,
                                          bool& ref_msg_processed) {
  auto& content = received_msg.unsafe_message;
  const int32 saved_read_pos = content.Tell();

  //여기까지 왔다는건 일단 MessageType::RPC(1바이트)는 읽은 후 오니까
  //읽기 위치가 1이어야함.
  //fun_check(saved_read_pos == 1);

  void* host_tag = owner_->GetHostTag(received_msg.remote_id);

  // 엔진 내부 Stub에서 처리가 될 경우에는 유저 스레드로 넘어가지 않는다.
  // 엔진 내부 Stub에서 처리되지 않은 경우에는, 유저 스레드로 넘겨주어서 처리하도록 한다.

  //여기서 잘못 읽혔다는건 시리얼라이징이 잘못되었다는건데...
  //const ByteArray S = ByteArray((const char*)received_msg.unsafe_message.GetReadableData() - 1, received_msg.unsafe_message.GetReadableLength() + 1);
  //LOG(LogNetEngine,Warning,"DUMP: %s", *String(S.ToHex()));

  // 엔진 내부 S2CStub에서 처리 시도.
  ref_msg_processed |= owner_->s2c_stub_.ProcessReceivedMessage(received_msg, host_tag);

  // 엔진 내부 C2CStub에서 처리 시도.
  if (!ref_msg_processed) {
    content.Seek(saved_read_pos); // Rewind
    ref_msg_processed |= owner_->c2c_stub_.ProcessReceivedMessage(received_msg, host_tag);
  }

  // 유저 스레드로 넘겨주어서 처리 시도.
  if (!ref_msg_processed) {
    // 엔진 내부 RPC에서 처리가 안되었으므로, 이제 유저 RPC임.
    // 그런데, 유저 RPC가 호출되는 시점에서 무조건 JIT P2P가 트리거 되는게 맞는걸까??

    content.Seek(saved_read_pos); // Rewind

    ReceivedMessage received_msg_to_user_thread;
    //received_msg_to_user_thread.action_time = received_msg.action_time;
    received_msg_to_user_thread.unsafe_message = content; // share
    received_msg_to_user_thread.relayed = received_msg.relayed;
    received_msg_to_user_thread.remote_addr_udp_only = received_msg.remote_addr_udp_only;
    received_msg_to_user_thread.remote_id = received_msg.remote_id;

    owner_->LockMain_AssertIsLockedByCurrentThread();
    owner_->FinalUserWorkItemQueue.Enqueue(FinalUserWorkItem(received_msg_to_user_thread, FinalUserWorkItemType::RPC));

    //LOG(LogNetEngine,Info,"UserRPC: %d", (int32)received_msg_to_user_thread.remote_id);

    // 보낸 호스트가 Server/None이 아닌 경우, 즉 피어에게서 받은 경우에는 P2P를 개시하게 됨.
    // 단, 이때 해당 피어가 같은 그룹내에 있어야함.
    // 그룹에 들어있지 않은 경우에는 릴레이를 통해서 서버를 경유해서 보내지게됨.
    if (received_msg_to_user_thread.remote_id != HostId_Server &&
        received_msg_to_user_thread.remote_id != HostId_None) {
      CScopedLock2 owner_guard(owner_->GetMutex());

      auto peer = owner_->GetPeerByHostId(received_msg_to_user_thread.remote_id);
      if (peer && !peer->garbaged_) {
        // Non-RZ RPC가 도착하면 P2P 홀펀칭JIT 조건이 된다.
        peer->jit_direct_p2p_needed_ = true;

        //LOG(LogNetEngine,Info,"jit_direct_p2p_needed!");

        // Update stats
        // 외부 RPC real P2P 인경우 해당 peer에게 받은 UDP packet count를 센다.
        if (!received_msg_to_user_thread.relayed) {
          peer->receive_udp_message_success_count++;
        }
      }
    }
  }
}

void NetClientWorker::Heartbeat_DetectNatDeviceName() {
  // NAT 장치이름을 구할때까지만, 수행함.
  if (owner_->nat_device_name_detected_ == false &&
      owner_->HasServerConnection() &&
      owner_->local_host_id_ != HostId_None) {
    const String name = owner_->GetNatDeviceName();
    if (!name.IsEmpty()) {
      owner_->nat_device_name_detected_ = true;
      owner_->c2s_proxy_.NotifyNatDeviceNameDetected(HostId_Server, GReliableSend_INTERNAL, name);
    }
  }
}

void NetClientWorker::ProcessMessage_RequestReceiveSpeedAtReceiverSide_NoRelay(
      UdpSocket_C* udp_socket, ReceivedMessage& received_msg) {
#ifdef UPDATE_TEST_STATS
  owner_->tes_stats2_.c2c_request_recent_recv_speed_done = true;
#endif

  // 이 메시지는 real UDP로 도착하지 않으면 불필요한 기능이다.
  if (udp_socket == nullptr) {
    TRACE_SOURCE_LOCATION();
    return;
  }

  // 수신속도를 리플한다.
  // 그러나 수신 UDP 주소로 바로 에코하지 않는다. 드물지만 src->dest시 src과 dst->src 리플시 src가 서로 다를 수 있으니까.
  const double receive_speed = udp_socket->packet_defragger_->GetRecentReceiveSpeed(received_msg.remote_addr_udp_only);
  if (receive_speed > 0) {
    MessageOut msg_to_send;
    lf::Write(msg_to_send, MessageType::ReplyReceiveSpeedAtReceiverSide_NoRelay);
    lf::Write(msg_to_send, receive_speed);

    if (received_msg.remote_id == HostId_Server) {
      const auto filter_tag = FilterTag::Make(owner_->GetLocalHostId(), received_msg.remote_id);
      const UdpSendOption send_opt(MessagePriority::Ring1, EngineOnlyFeature);
      owner_->Get_ToServerUdpSocket()->SendWhenReady(
            received_msg.remote_id,
            filter_tag,
            owner_->to_server_udp_fallbackable_->server_addr_,
            msg_to_send,
            owner_->GetAbsoluteTime(),
            send_opt);
    } else if (received_msg.remote_id != HostId_None) {
      auto peer = owner_->GetPeerByHostId(received_msg.remote_id);
      if (peer && !peer->garbaged_ && peer->udp_socket_) {
        const auto filter_tag = FilterTag::Make(owner_->GetLocalHostId(), received_msg.remote_id);
        const UdpSendOption send_opt(MessagePriority::Ring1, EngineOnlyFeature);
        peer->Get_ToPeerUdpSocket()->SendWhenReady(
              received_msg.remote_id,
              filter_tag,
              peer->p2p_holepunched_local_to_remote_addr_,
              msg_to_send,
              owner_->GetAbsoluteTime(),
              send_opt);
      }
    }
  }
}

void NetClientWorker::ProcessMessage_ReplyReceiveSpeedAtReceiverSide_NoRelay(ReceivedMessage& received_msg) {
  owner_->LockMain_AssertIsLockedByCurrentThread();

  auto& msg = received_msg.unsafe_message;

  double receive_speed;
  if (!lf::Read(msg, receive_speed)) {
    TRACE_SOURCE_LOCATION();
    return;
  }

  // 받은 '수신속도'를 저장
  InetAddress sendto;
  UdpPacketFragger* fragger = nullptr;
  if (received_msg.remote_id == HostId_Server && owner_->Get_ToServerUdpSocket()) {
    fragger = owner_->Get_ToServerUdpSocket()->packet_fragger_.Get();
    sendto = owner_->Get_ToServerUdpSocket()->here_addr_at_server_;
  } else {
    auto peer = owner_->GetPeerByHostId(received_msg.remote_id);
    if (peer && peer->udp_socket_) {
      fragger = peer->Get_ToPeerUdpSocket()->packet_fragger_.Get();
      sendto = peer->p2p_holepunched_local_to_remote_addr_;
    }
  }

  if (fragger) {
    fragger->SetReceiveSpeedAtReceiverSide(sendto, receive_speed);
  }
}

void NetClientWorker::ProcessMessage_FreeformMessage(ReceivedMessage& received_msg, bool& ref_msg_processed) {
  auto& content = received_msg.unsafe_message;

  fun_check(!content.IsViewAdjusted());

  // 유저 스레드에서 user message를 처리하도록 enqueue한다.

  ReceivedMessage received_msg_to_user_thread;
  //received_msg_to_user_thread.action_time = received_msg.action_time;
  received_msg_to_user_thread.unsafe_message = content.ToReadableMessage(); // shared
  received_msg_to_user_thread.relayed = received_msg.relayed;
  received_msg_to_user_thread.remote_addr_udp_only = received_msg.remote_addr_udp_only;
  received_msg_to_user_thread.remote_id = received_msg.remote_id;

  owner_->LockMain_AssertIsLockedByCurrentThread();
  owner_->final_user_work_queue_.Enqueue(FinalUserWorkItem(received_msg_to_user_thread, FinalUserWorkItemType::FreeformMessage));

  //주의:
  // 원격지의 피어에게서 메시지를 받았다고 해서, P2P가 바로 개시가 안됨.
  // 일단은 그룹핑으로 결성되어야만 P2P가 개시될 수 있음.

  // 피어에게서 메시지를 받은 경우에는 DirectP2P가 개시하게됨.
  if (received_msg_to_user_thread.remote_id != HostId_Server &&
      received_msg_to_user_thread.remote_id != HostId_None) {
    // User RPC가 도착하면 P2P 홀펀칭JIT 조건이 된다.
    CScopedLock2 owner_guard(owner_->GetMutex());

    //RP가 목록에 들어 있어야만 처리가 되는데, 이말은 그룹핑이 된 이후에만 처리가 된다는 얘기?
    //그룹핑이 안되어도 되게끔할 수는 없으려나??

    auto peer = owner_->GetPeerByHostId(received_msg_to_user_thread.remote_id);
    if (peer && !peer->garbaged_) {
      peer->jit_direct_p2p_needed_ = true;

      //LOG(LogNetEngine,Info,"jit_direct_p2p_needed!");

      // Real UDP인 경우 count를 한다.
      if (!received_msg_to_user_thread.relayed) {
        peer->receive_udp_message_success_count++;
      }
    }
  }
}

void NetClientWorker::WarnTooLongElapsedTime() {
  if (NetConfig::starvation_warning_enabled) {
    static const double COMPARE_WARNING_TIME = double(NetConfig::client_heartbeat_interval_msec) / 1000 * 10;

    const double average = owner_->manager_->average_elapsed_time_;
    const double first_average_calc_time_ = owner_->manager_->first_average_calc_time_;

    if (average > COMPARE_WARNING_TIME &&
        (owner_->GetAbsoluteTime() - first_average_calc_time_) > 10) { // 최소 10초는 지난 후에 체크하지 않으면 넷클라 처음 뜨는 동안 버벅대는 시간 등이 모두 모아지므로 false positive가 생김.
      const String text = String::Format("%lf average heartbeat elapsed-time", average);

      //note: EnqueueWarning() 내부에서 락이 걸려있는 상태이므로, 구태여 락을 걸필요 없음.
      //CScopedLock2 owner_guard(owner_->GetMutex());
      owner_->EnqueueWarning(ResultInfo::From(ResultCode::TooSlowHeartbeatWarning, owner_->GetLocalHostId(), text));

      if (owner_->IsIntraLoggingOn()) {
        owner_->IntraLogToServer(LogCategory::System, *text);
      }
    }
  }
}

void NetClientManager::ProcessIoCompletion(uint32 timeout_msec) {
  mutex_.AssertIsNotLockedByCurrentThread();

  CompletionStatus completion;
  if (completion_port_->GetQueuedCompletionStatus(&completion, timeout_msec)) {
    const LeanType lean_type = completion.completion_context->GetLeanType();
    switch (lean_type) {
      case LeanType::TcpTransport_C:
        ProcessIoCompletion_TCP(completion);
        break;
      case LeanType::UdpSocket_C:
        ProcessIoCompletion_UDP(completion);
        break;
      default:
        ErrorReporter::Report("Unexpected at client IOCP completion.");
        break;
    }
  }
}

NetClientManager::NetClientManager() {
  timer_touched_ = false;

  clock_.Start();
  {
    absolute_time = clock_.AbsoluteSeconds();
    elapsed_time = 0;
    recent_elapsed_time_ = 0;
    timer_touched_ = true;

    average_elapsed_time_ = 0;
    first_average_calc_time_ = 0;
    last_heartbeat_time_ = 0;
    last_garbage_free_time_ = 0;
  }

  completion_port_.Reset(new CompletionPort(this, true, CPlatformMisc::NumberOfCoresIncludingHyperthreads()));

  should_stop_thread_ = false;
  heartbeat_pulse_.Set(0);

  disconnection_invoke_count_.Set(0);

  last_send_enqueue_completion_time_ = 0;

  thread_ended_ = false;
  worker_thread_.Reset(RunnableThread::Create(this, "NetClientWorker"));

  timer_.Start();

  const uint32 Interval = MathBase::Max(NetConfig::client_heartbeat_interval_msec, 1u);
  heartbeat_signal_timer_id_ = timer_.Schedule(
      Timespan::FromMilliseconds(Interval),
      Timespan::FromMilliseconds(Interval),
      [&](const TimerTaskContext&) { heartbeat_pulse_.Set(1); },
      "NetClientManager.HeartbeatSignal");
}

NetClientManager::~NetClientManager() {
  // 우선 타이머부터 꺼주도록 하자.
  timer_.Stop(true);
  heartbeat_signal_timer_id_ = 0;

  if (upnp_) {
    upnp_->IssueTermination();
    upnp_->WaitUntilNoIssueGuaranteed();
  }

  //if (send_speed_measurer_) {
  //  send_speed_measurer_->IssueTermination();
  //  send_speed_measurer_->WaitUntilNoIssueGuaranteed();
  //}

  should_stop_thread_ = true;
  worker_thread_->Join();
  worker_thread_.Reset();

  completion_port_.Reset();

  upnp_.Reset();

  overlapped_io_garbages_.Clear();
}

void NetClientManager::Run()
{
  // 게임 렌더링 코어 등에 의해 많은 시간을 뺏기면 대량 통신시 UDP 타임아웃이 자주 발생하는 것 같아 이 설정이 필요
  if (NetConfig::networker_thread_priority_is_high) {
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
  }

  thread_ended_ = false;

#if EXPOSE_CRASH_THAN_CONCEAL_EXCEPTION
  try {
#endif
    while (!should_stop_thread_) {
      //TODO
      // 프로세스 종료중이면 아무것도 하지 않는다.
      //if (CThread::bDllProcessDetached_INTERNAL)
      //{
      //  return;
      //}

      // 1개의 스레드만이 존재하므로 바로 여기서 heartbeat or GQCS 처리를 한다.
      ProcessIoCompletion(NetConfig::client_heartbeat_interval_msec);

      //TODO 위에서 어짜피 대기를 하니까 그냥 하면 되는거 아닐런지???
      //아니다 위에서 대기는 이벤트가 없을 경우에만 주어진 시간까지 대기하는거니까
      //이벤트가 있을 경우에는 바로 블럭에서 해제될 수 있으므로, 타이밍 체크는 별도로 해야함.

      // 병렬로 호출되지 않음. 클라이언트는 단일 스레드로 구동되기 때문이다.
      // (메인 워커 스레드는 단일 스레드이거나, 폴링 형태로 구동됨)

      //TODO 현재 1ms마다 호출되고 있는데, 이게 정상인가???
      if (heartbeat_pulse_.GetValue()) {
        heartbeat_pulse_.Set(0);
        {
          CScopedLock2 time_status_guard(time_status_mutex_);

          absolute_time = clock_.AbsoluteSeconds(); // not elapsed_time here!
          elapsed_time = clock_.ElapsedSeconds();

          //@note 민감하게 변하지 않도록, linear interpolation시킴
          //변수 선언시 volatile이 지정되어 잇어서, MathBase::Lerp에서 문법오류(?) 나고 있음.
          //강제로 double로 캐스팅 했더니 문제는 없는데... 흠..
          recent_elapsed_time_ = MathBase::Lerp((double)recent_elapsed_time_, (double)elapsed_time, 0.3);

          timer_touched_ = true;
        }

        Heartbeat_EveryNetClient();

        if (upnp_) {
          upnp_->Heartbeat();
        }
      }
    }
#if EXPOSE_CRASH_THAN_CONCEAL_EXCEPTION
  } catch (Exception& e) {
    const String text = String::Format("Exception(%s)", *e.Message());
    CatchThreadUnexpectedExit(__FUNCTION__, *text);
  } catch (std::exception& e) {
    const String text = String::Format("std.exception(%s)", (const char*)UTF8_TO_TCHAR(e.what()));
    CatchThreadUnexpectedExit(__FUNCTION__, *text);
  } //catch (_com_error& e) {
  //  const String text = String::Format("_com_error(%s)", (const char*)e.Description());
  //  CatchThreadUnexpectedExit(__FUNCTION__, text);
  //} catch (void*) {
  //  CatchThreadUnexpectedExit(__FUNCTION__, "void*");
  //}
#endif
  //catch (...) { // 사용자 정의 루틴을 콜 하는 곳이 없으므로 주석화
  //  if (NetConfig::catch_unhandled_exception) {
  //    CatchThreadUnexpectedExit(__FUNCTION__, "Unknown");
  //  } else {
  //    throw;
  //  }
  //}

  thread_ended_ = true;
}

void NetClientManager::CatchThreadUnexpectedExit(const char* where, const char* reason) {
  LOG(LogNetEngine, Warning, "(%s): Unexpected thread exit with (%s)", where, reason);
}

void NetClientManager::Register(NetClientWorker* instance) {
  CScopedLock2 owner_guard(mutex_);
  instance->UnlinkSelf();
  instances_.Append(instance);
}

void NetClientManager::Unregister(NetClientWorker* instance) {
  CScopedLock2 owner_guard(mutex_);

  if (instance->GetState() != NetClientWorker::State::Disconnected) {
    ErrorReporter::Report("Unexpected at NetClientManager::Unregister");
  }

  instance->UnlinkSelf();
}

// 모든 넷클라에 대한 타이머 루틴을 각각 실행.
void NetClientManager::Heartbeat_EveryNetClient() {
  CScopedLock2 owner_guard(mutex_);

  auto instance = instances_.Front();
  while (instance) {
    if (completion_port_) {
      // 이 안에서 각 인스턴스에 대한 lock이 있을 수 있다.
      instance->Heartbeat();
    } else {
      // Networker thread에서도 completion port=null 시 무조건 managee들을
      // disconnected state로 바꾸도록 만들어야 하겠다.
      instance->SetState(NetClientWorker::State::Disconnected);
    }

    if (instance->GetState() != NetClientWorker::State::Disconnected) {
      instance = instance->GetNextNode();
    } else {
      // 완전 디스 상태이면 heartbeat도 막아버린다.
      auto instance_to_delete = instance;
      instance = instance->GetNextNode();
      Unregister(instance_to_delete);

      // 별 문제는 없어 보이지만, 안전하게 하자.
      auto owner = instance_to_delete->owner_;
      owner_->worker_.Reset();
    }
  }

  //CalculateAverageElapsedTime();
  DoGarbageFree();
}

bool NetClientManager::IsThisWorkerThread() {
  return worker_thread_ && worker_thread_->GetTid() == Thread::CurrentTid();
}

bool NetClientManager::IsRegistered(NetClientWorker* instance) {
  CScopedLock2 owner_guard(mutex_);

  //TODO instance->IsLinkedIn() 와 같은 함수를 만드는게 가독성이 좋아지려나??
  return instance->GetListOwner() != nullptr;
}

bool NetClientManager::ProcessIoCompletion_TCP(CompletionStatus& completion) {
  auto tcp_transport = (TcpTransport_C*)completion.completion_context;

  if (tcp_transport) {
    CScopedLock2 owner_guard(mutex_); //TODO main lock을 잡은 상태에서 해야하나??

    if (completion.type == CompletionType::Send) {
      if (tcp_transport->owner_->GetWorker()) {
        tcp_transport->owner_->GetWorker()->TcpSendCompletionCase(completion);
      }
    } else if (completion.type == CompletionType::Receive) {
      if (tcp_transport->owner_->GetWorker()) {
        tcp_transport->owner_->GetWorker()->TcpRecvCompletionCase(completion);
      }
    } else {
      owner_guard.Unlock();
      ErrorReporter::Report(*String::Format("Unexpected at client IOCP TCP completion. type: %d", (int32)completion.type));
    }
    return true;
  }

  return false;
}

bool NetClientManager::ProcessIoCompletion_UDP(CompletionStatus& completion) {
  auto udp_socket = (UdpSocket_C*)completion.completion_context;

  if (udp_socket/* && !udp_socket->garbaged_*/) { // Garbage인 경우에도 I/O 처리는 해야함.
    CScopedLock2 owner_guard(mutex_); //TODO main lock을 잡은 상태에서 해야하나??

    if (completion.type == CompletionType::Send) {
      if (udp_socket->main_->worker_) {
        udp_socket->main_->worker_->UdpSendCompletionCase(udp_socket, completion);
      }
    } else if (completion.type == CompletionType::Receive) {
      if (udp_socket->main_->worker_) {
        udp_socket->main_->worker_->UdpRecvCompletionCase(udp_socket, completion);
      }
    } else {
      owner_guard.Unlock();
      ErrorReporter::Report(*String::Format("Unexpected at client IOCP UDP completion. type: %d", (int32)completion.type));
    }

    return true;
  }

  return false;
}

void NetClientManager::RequestMeasureSendSpeed(bool enable) {
  //CScopedLock2 owner_guard(mutex_);
  //
  //if (send_speed_measurer_ == nullptr && enable) {
  //  send_speed_measurer_ = SendSpeedMeasurerPtr(new SendSpeedMeasurer(this));
  //
  //  send_speed_measurer_->RequestProcess();
  //} else if (send_speed_measurer_ && !enable) {
  //  // 객체를 제거하지는 말고 그냥 중단 요청만 한다.
  //  send_speed_measurer_->RequestStopProcess();
  //}
}

//TODO 이렇게 복잡하게 구현할 필요가 있었을까??
double NetClientManager::GetCachedAbsoluteTime() {
  if (!timer_touched_) {
    CScopedLock2 timer_init_guard(timer_init_mutex_);
    CScopedLock2 time_status_guard(time_status_mutex_);

    if (!timer_touched_ && clock_.IsStopped()) {
      clock_.Start();
      absolute_time = clock_.AbsoluteSeconds();
    }

    timer_touched_ = true;
  }

  return absolute_time;
}

void NetClientManager::OnMeasureComplete(double speed) {
  for (auto instance = instances_.Front(); instance; instance = instance->GetNextNode()) {
    instance->owner_->c2s_proxy_.NotifySendSpeed(HostId_Server, GReliableSend_INTERNAL, speed);
  }
}

void NetClientManager::CalculateAverageElapsedTime() {
  if (NetConfig::starvation_warning_enabled) {
    const double abs_time = absolute_time;

    if (last_heartbeat_time_ != 0) {
      const double elapsed_time = abs_time - last_heartbeat_time_;

      // 최소 한개 이상은 되어야한다. 그렇지 않을 경우 나누기 오류가 발생한다.
      fun_check_dbg(NetConfig::manager_average_elapsed_time_sample_count >= 1);

      // 샘플 수집
      elapsed_time_queue_.Append(elapsed_time);

      // 최대 샘플까지만 제한함.
      if (elapsed_time_queue_.Count() > NetConfig::manager_average_elapsed_time_sample_count) {
        elapsed_time_queue_.RemoveFront();
      }

      // 최대 샘플까지 수집이 된경우에만 정확한 값을 측정할 수 있음.
      if (elapsed_time_queue_.Count() == NetConfig::manager_average_elapsed_time_sample_count) { //TODO 이름을 좀 바꾸는데...
        //dword/1000 *10 dword를 double로 바꿔 10배로 바꿈 이것이 warning의 기준이 된다.
        double accum_elapsed_time = 0;
        for (auto elapsed_time : elapsed_time_queue_) {
          accum_elapsed_time += elapsed_time;
        }

        const double average = accum_elapsed_time / (double)elapsed_time_queue_.Count();
        average_elapsed_time_ = average;

        if (first_average_calc_time_ == 0) {
          first_average_calc_time_ = abs_time;
        }
      }

      last_heartbeat_time_ = abs_time;
    } else { // 처음호출시에는 last_heartbeat_time_ 값만 갱신.
      last_heartbeat_time_ = abs_time;
    }
  }
}

void NetClientManager::HasOverlappedIoToGarbage(IHasOverlappedIoPtr overlapped_io) {
  // Induce dispose.
  if (!overlapped_io->IsSocketClosed()) {
    overlapped_io->OnCloseSocketAndMakeOrphant();
  }

  // Insert into garbages.
  overlapped_io_garbages_.Append(overlapped_io);
}

void NetClientManager::DoGarbageFree() {
  //TODO IntervalAlarm으로 처리하는게 좋을듯 싶은데...??
  if ((absolute_time - last_garbage_free_time_) > NetConfig::manager_garbage_free_interval_sec) {
    last_garbage_free_time_ = absolute_time;

    for (auto it = overlapped_io_garbages_.CreateIterator(); it; ++it) {
      const bool any_io_issued = (it->recv_issued_ || it->send_issued_);
      if (!any_io_issued) {
        overlapped_io_garbages_.Remove(it);
      }
    }
  }
}

} // namespace net
} // namespace fun
