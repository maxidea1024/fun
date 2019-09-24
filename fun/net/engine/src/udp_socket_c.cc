// TODO 코드정리
#include "InternalSocket.h"
#include "NetClient.h"
#include "PacketFrag.h"
#include "UdpSocket_C.h"
#include "fun/net/net.h"

#include "ReportError.h"

namespace fun {
namespace net {

UdpSocket_C::UdpSocket_C(NetClientImpl* main, RemotePeer_C* owner_peer) {
  fun_check_ptr(main);
  main_ = main;
  owner_peer_ = owner_peer;

  local_addr_ = InetAddress::None;
  here_addr_at_server_ = InetAddress::None;

  send_issued_ = false;
  recv_issued_ = false;
  recycle_binned_time_ = 0;

  last_udp_recv_issued_time_ = main_->GetAbsoluteTime();

  packet_fragger_.Reset(
      new UdpPacketFragger(owner_peer_ ? (IUdpPacketFraggerDelegate*)owner_peer_
                                       : (IUdpPacketFraggerDelegate*)main_));
  packet_fragger_->InitHashTableForClient();

  send_issued_frag_.Reset(new UdpPacketFraggerOutput());

  packet_defragger_.Reset(new UdpPacketDefragger(main_));
}

bool UdpSocket_C::CreateSocket(const InetAddress& udp_local_addr) {
  // If this function is called with a socket already in it, socket should be no
  // I/O issued.
  fun_check(socket_.IsValid() == false);
  fun_check_ptr(main_);
  if (main_ == nullptr) {
    return false;
  }

  main_->LockMain_AssertIsLockedByCurrentThread();

  // create UDP socket, set send buffer to 0, set recv buffer to 1MB
  socket_.Reset(new InternalSocket(SocketType::Udp, main_));

#ifdef CHECK_BUF_SIZE
  int32 SendBufLength, RecvBufLength;
  socket_->GetSendBufferSize(&SendBufLength);
  socket_->GetRecvBufferSize(&RecvBufLength);
#endif

  // Associate to IOCP.
  socket_->SetCompletionContext((ICompletionContext*)this);

  if (!socket_->Bind(udp_local_addr)) {
    socket_.Reset();
    ErrorReporter::Report(String::Format(
        "UdpSocket_C.CreateSocket - UDP local address binding to %s failed.",
        *udp_local_addr.ToString()));
    return false;
  }

  main_->manager_->completion_port_->AssociateSocket(socket_.Get());

  if (!RefreshLocalAddress()) {
    socket_.Reset();
    ErrorReporter::Report(String::Format(
        "UdpSocket_C.CreateSocket - UDP local address to %s.  The getsockname "
        "result after the binding is unexpectedly %s.",
        *udp_local_addr.ToString(), *local_addr_.ToString()));
    return false;
  }

  // zero copy send is fast, but too dangerous to cause nonpaged.
  // So let's stop this.  We you have better performance?
#ifdef ALLOW_ZERO_COPY_SEND
  socket_->SetSendBufferSize(0);
#endif
  NetUtil::SetUdpDefaultBehavior(socket_.Get());

  return true;
}

bool UdpSocket_C::RestoreSocket() {
  fun_check_ptr(owner_peer_);

  socket_->Restore(false);

  // Get the local address of the TCP socket that was associated with the server
  // and bind it to it. Otherwise, if there are more than two local NICs and one
  // is dedicated to the internal network, communication may be bad.
  InetAddress udp_local_addr = main_->Get_ToServerTcp()->local_addr_;
  if (!udp_local_addr.IsUnicast()) {
    ErrorReporter::Report(String::Format(
        "UdpSocket_C.RestoreSocket - TCP connection must already exist before "
        "creating a UDP socket, but %s is now!",
        *udp_local_addr.ToString()));
    socket_->CloseSocketHandleOnly();
    return false;
  }

  // Any port
  udp_local_addr.SetPort(0);

  if (!socket_->Bind(udp_local_addr)) {
    socket_->CloseSocketHandleOnly();
    ErrorReporter::Report(String::Format(
        "UdpSocket_C.RestoreSocket - UDP local address binding to %s failed.",
        *udp_local_addr.ToString()));
    return false;
  }

  if (!RefreshLocalAddress()) {
    socket_->CloseSocketHandleOnly();
    ErrorReporter::Report(String::Format(
        "UdpSocket_C.RestoreSocket - UDP local address to %s The getsockname "
        "result after the binding is unexpectedly %s.",
        *udp_local_addr.ToString(), *local_addr_.ToString()));
    return false;
  }

  //소켓 객체가 renew된게 아니므로, 기존 값은 유지되고 있을것임.
  // socket_->SetCompletionContext(ICompletionContext*)this);
  main_->LockMain_AssertIsLockedByCurrentThread();

  // Associate to IOCP
  main_->manager_->completion_port_->AssociateSocket(socket_.Get());

  // Remove the socket's send buffer. Since CSocketBuffer is non swappable, it
  // is safe. Remove send buffer is required for coalesce and throttling. recv
  // buffer removal is free of charge

  // zero copy send is fast, but too dangerous to cause non-paged.
  // So let's stop this.  We you have better performance?
#ifdef ALLOW_ZERO_COPY_SEND
  socket_->SetSendBufferSize(0);
#endif

  NetUtil::SetUdpDefaultBehavior(socket_.Get());

  // The larger the recv buffer, the larger the OS load.
  // it is best not to touch it unless it is a special case.
  // socket_->SetRecvBufferSize(NetConfig::ClientUdpRecvBufferLength);

  fun_check(main_->manager_->IsThisWorkerThread());

  ConditionalIssueRecvFrom();  // first

  return true;
}

void UdpSocket_C::ConditionalIssueSend() {
  // Note: This function should not be recursive.
  main_->LockMain_AssertIsLockedByCurrentThread();

  if (recycle_binned_time_ == 0 && !send_issued_ && socket &&
      !socket_->IsClosed()) {
    // Find something and send it.
    auto XXX = send_issued_frag_.Get();
    if (packet_fragger_->PopAnySendQueueFilledOneWithCoalesce(
            *XXX, main_->GetAbsoluteTime()) &&
        send_issued_frag_->send_frag_frag.buffer.Count() > 0) {
      send_issued_ = true;

      const SocketErrorCode socket_error = socket_->IssueSendTo_NoCopy_TempTtl(
          send_issued_frag_->send_frag_frag, send_issued_frag_->sendto,
          send_issued_frag_->ttl);
      if (socket_error != SocketErrorCode::Ok) {
        send_issued_ = false;
      } else {
        // TODO error tracking?
      }
    }
  }
}

void UdpSocket_C::ConditionalIssueRecvFrom() {
  // main_->GetMutex().AssertIsLockedByCurrentThread();
  main_->LockMain_AssertIsLockedByCurrentThread();

  // Do not issue if the socket is already closed.
  // If you issue a closed socket, completion occurs anyway.
  // Scraping the memory.
  if (recycle_binned_time_ == 0 && !recv_issued_ && socket &&
      !socket_->IsClosedOrClosing()) {
    recv_issued_ = true;

    const SocketErrorCode socket_error =
        socket_->IssueRecvFrom(NetConfig::udp_issue_recv_length);
    // MSDN says there is no completion other than this.
    // So this should only be checked.
    if (socket_error == SocketErrorCode::Ok) {
      last_udp_recv_issued_time_ = main_->GetAbsoluteTime();
    } else {
      recv_issued_ = false;
    }
  }
}

void UdpSocket_C::SendWhenReady(HostId final_dest_id,
                                FilterTag::Type filter_tag,
                                const InetAddress& sendto,
                                const SendFragRefs& data_to_send,
                                double added_time,
                                const UdpSendOption& send_opt) {
  main_->LockMain_AssertIsLockedByCurrentThread();

  fun_check(sendto.IsUnicast());

  SendFragRefs final_send_data;
  MessageOut header;
  MessageStream::AddStreamHeader(data_to_send, final_send_data, header);

  // Add to send queue.
  // UDP sends the first send issue by coalesce.
  // So you do not have to post a signal to send it to send it immediately
  // because it is sent after 10ms at most.
  packet_fragger_->AddNewPacket(final_dest_id, filter_tag, sendto,
                                final_send_data, added_time, send_opt);
}

UdpSocket_C::~UdpSocket_C() {
  main_->LockMain_AssertIsLockedByCurrentThread();

  packet_defragger_.Reset();
  send_issued_frag_.Reset();
  packet_fragger_.Reset();
}

void UdpSocket_C::CloseSocketHandleOnly() {
  main_->LockMain_AssertIsLockedByCurrentThread();

  // fun_check_ptr(socket_->GetAssociatedCompletionPort());
  // In CS lock state, close socket must be done to prevent the side effect of
  // closing before IOCP association is done in other thread.
  socket_->CloseSocketHandleOnly();

  // udp socket record the closed statement
  if (main_->settings_.emergency_log_line_count > 0) {
    main_->AddEmergencyLogList(LogCategory::UDP,
                               "Call CloseSocketHandleOnly()");
  }
}

bool UdpSocket_C::RefreshLocalAddress() {
  local_addr_ = socket_->GetSockName();

  if (!local_addr_.IsUnicast()) {
    // If you do not create a UDP socket when you are already connected to TCP,
    // If you shoot to the opponent and the opponent punches the hole with
    // the recognized address, there may be an underwater situation.
    // For example, two or more game companies' LAN cards.
    ErrorReporter::Report(
        String::Format("UdpSocket_C.RefreshLocalAddress - The IP address of "
                       "the client-generated UDP socket is unbound (%s)!",
                       *local_addr_.ToString()));
    return false;
  }

  return true;
}

void UdpSocket_C::LongTick(double absolute_time) {
  // Processing for outgoing queues
  packet_fragger_->LongTick(absolute_time);

  // also handle the receive queue
  if (packet_defragger_) {
    packet_defragger_->LongTick(absolute_time);
  }
}

void UdpSocket_C::ResetPacketFragState(RemotePeer_C* new_owner_peer) {
  // NOTE 내부 메모리 의존성 때문에 먼저 해주어야할런지??
  send_issued_frag_.Reset();
  packet_fragger_.Reset();
  packet_defragger_.Reset();

  packet_fragger_.Reset(
      new UdpPacketFragger((IUdpPacketFraggerDelegate*)new_owner_peer));
  packet_fragger_->InitHashTableForClient();
  packet_defragger_.Reset(new UdpPacketDefragger(main_));

  send_issued_frag_.Reset(new UdpPacketFraggerOutput);

  owner_peer_ = new_owner_peer;
}

void UdpSocket_C::OnCloseSocketAndMakeOrphant() {
  CloseSocketHandleOnly();

  // Do not change this to nullptr.  Instead, owner_peer_ determines if it is in
  // the trash. udp_socket->main_ = nullptr;

  owner_peer_ = nullptr;
}

bool UdpSocket_C::IsSocketClosed() {
  return !socket_.IsValid() || socket_->IsClosedOrClosing();
}

}  // namespace net
}  // namespace fun
