// TODO 코드정리
#include "NetServer.h"
#include "PacketFrag.h"
#include "UdpSocket_S.h"
#include "fun/net/net.h"

namespace fun {
namespace net {

using lf = LiteFormat;

UdpSocket_S::UdpSocket_S(NetServerImpl* owner) {
  fun_check_ptr(owner);
  owner_ = owner;

  packet_fragger_.Reset(new UdpPacketFragger(this));

  // You do not have to deal with send brakes on the server side. it only causes
  // a bad bug. In addition, if you want to solve the problem with a brake,
  // short interval processing is required, which is a bad server load.
  packet_fragger_->send_break_enabled = false;

  send_issued_frag_.Reset(new UdpPacketFraggerOutput);

  send_issued_ = false;
  recv_issued_ = false;
  // SendUnderProgress = false;
  socket.Reset(new InternalSocket(SocketType::Udp, owner_, owner_));

#ifdef CHECK_BUF_SIZE
  int32 send_buffer_length, recv_buffer_length;
  socket_->GetSendBufferSize(&send_buffer_length);
  socket_->GetRecvBufferSize(&recv_buffer_length);
#endif

#ifdef USE_DisableUdpConnResetReport
  socket_->DisableUdpConnResetReport();
#endif

  cached_local_addr_ = InetAddress::None;

  packet_defragger_.Reset(new UdpPacketDefragger(this));
}

void UdpSocket_S::ConditionalIssueSend() {
  // The main lock should not get hung up, because what it does is not involved
  // in the main lock.
  owner_->AssertIsNotLockedByCurrentThread();
  AssertIsZeroUseCount();

  // lock must be on top.
  AssertIsLockedByCurrentThread();

  // Do not issue if the socket is already closed.
  // If you issue a closed socket, completion occurs anyway. Scraping the
  // memory. You should check this before checking if you want to send. must!
  if (!send_issued_ && !socket_->IsClosedOrClosing()) {
    CScopedLock2 fragger_guard(GetFraggerMutex());

    // Take something out and send it.
    auto XXX = send_issued_frag_.Get();
    if (packet_fragger_->PopAnySendQueueFilledOneWithCoalesce(
            *XXX, owner_->GetAbsoluteTime()) &&
        send_issued_frag_->send_frag_frag.Buffer.Count() > 0) {
      lock.Unlock();

      send_issued_ = true;
      const SocketErrorCode socket_error = socket_->IssueSendTo_NoCopy_TempTtl(
          send_issued_frag_->send_frag_frag, send_issued_frag_->send_to,
          send_issued_frag_->ttl);
      if (socket_error != SocketErrorCode::Ok) {
        send_issued_ = false;
      }
    }
  }
}

SocketErrorCode UdpSocket_S::IssueSend(double absolute_time) {
  ConditionalIssueSend();

  return SocketErrorCode::Ok;
}

void UdpSocket_S::ConditionalIssueRecvFrom() {
  fun_check(!recv_issued_);
  // owner_->AssertIsNotLockedByCurrentThread();
  AssertIsZeroUseCount();
  AssertIsLockedByCurrentThread();

  // Do not issue if the socket is already closed.
  // If you issue a closed socket, completion occurs anyway. Scraping the
  // memory.
  if (!recv_issued_ && !socket_->IsClosedOrClosing()) {
    recv_issued_ = true;
    const SocketErrorCode socket_error =
        socket_->IssueRecvFrom(NetConfig::udp_issue_recv_length);
    if (socket_error != SocketErrorCode::Ok) {
      recv_issued_ = false;
    }
  }
}

void UdpSocket_S::SendWhenReady(HostId final_dest_id,
                                FilterTag::Type filter_tag,
                                const InetAddress& send_to,
                                const SendFragRefs& data_to_send,
                                const UdpSendOption& send_opt) {
  // mainlock를 걸때도 있고, 아닐때도 있음.
  // owner_->AssertIsNotLockedByCurrentThread();
  AssertIsZeroUseCount();

  fun_check(send_to.IsUnicast());

  SendFragRefs final_send_data;
  MessageOut header;
  MessageStream::AddStreamHeader(data_to_send, final_send_data, header);
  {
    //걸려있어도 된다.lock 순서 보장은 위에서 하도록 하자.
    // AssertIsFraggerNotLockedByCurrentThread();
    CScopedLock2 fragger_guard(GetFraggerMutex());

    // 송신큐에 추가한다.
    packet_fragger_->AddNewPacket(final_dest_id, filter_tag, send_to,
                                  final_send_data, owner_->GetAbsoluteTime(),
                                  send_opt);
  }

  // Attach the transmit queue lock separately.
  {
    CScopedLock2 lock(owner_->udp_issue_queue_mutex_);

    if (GetListOwner() == nullptr) {
      owner_->udp_issued_send_ready_list_.Append(this);
    }
  }
}

void UdpSocket_S::SendWhenReady(HostId sender_id,
                                const InetAddress& sender_addr, HostId dest_id,
                                const SendFragRefs& data_to_send,
                                const UdpSendOption& send_opt) {
  AssertIsFraggerLockedByCurrentThread();
  AssertIsZeroUseCount();

  fun_check(sender_addr.IsUnicast());

  SendFragRefs final_send_data;
  MessageOut header;
  MessageStream::AddStreamHeader(data_to_send, final_send_data, header);

  // Add to send queue.
  const auto filter_tag = FilterTag::Make(HostId_Server, dest_id);
  packet_fragger_->AddNewPacket(sender_id, filter_tag, sender_addr,
                                final_send_data, owner_->GetAbsoluteTime(),
                                send_opt);

  // attach the transmit queue lock separately.
  {
    CScopedLock2 udp_issue_queue_guard(owner_->udp_issue_queue_mutex_);

    if (GetListOwner() == nullptr) {
      owner_->udp_issued_send_ready_list_.Append(this);
    }
  }
}

NamedInetAddress UdpSocket_S::GetRemoteIdentifiableLocalAddr(
    RemoteClient_S* rc) {
  AssertIsNotLockedByCurrentThread();  // mainlock만으로 진행이 가능할듯.

  NamedInetAddress result = NamedInetAddress(socket_->GetSockName());

  // The address of the socket that accepted the TCP connection. The most
  // successful connection rate is low. Even if there are two or more NICs, the
  // address of the TCP connection is likely to receive the UDP connection, so
  // OK.
  result.OverwriteHostNameIfExists(
      rc->to_client_tcp_->socket_->GetSockName().GetHost().ToString());

  // If the server has more than one NIC and if the NIC address is specified,
  // first choose the NIC address.
  result.OverwriteHostNameIfExists(owner_->local_nic_addr_);

  // If there is a public address for the server, use it first.
  result.OverwriteHostNameIfExists(owner_->server_ip_alias_);

  if (!result.IsUnicast()) {
    result.address = NetUtil::LocalAddressAt(0).ToString();
  }

  return result;
}

InetAddress UdpSocket_S::GetCachedLocalAddr() {
  if (cached_local_addr_ == InetAddress::None && socket_) {
    cached_local_addr_ = socket_->GetSockName();
  }

  return cached_local_addr_;
}

UdpSocket_S::~UdpSocket_S() {
  owner_->AssertIsLockedByCurrentThread();

  CScopedLock2 lock(GetMutex());
  CScopedLock2 FragboardLock(GetFraggerMutex());

  packet_defragger_.Reset();

  send_issued_frag_.Reset();
  packet_fragger_.Reset();

  UnlinkSelf();
}

double UdpSocket_S::GetAbsoluteTime() { return owner_->GetAbsoluteTime(); }

void UdpSocket_S::RequestReceiveSpeedAtReceiverSide_NoRelay(
    const InetAddress& dest_addr) {
  // Because the socket is being shared, find the appropriate destination.
  // Request the UDP reception rate from the server side.
  MessageOut msg;
  lf::Write(msg, MessageType::RequestReceiveSpeedAtReceiverSide_NoRelay);
  const auto filter_tag =
      FilterTag::Make(HostId_Server, HostId_None);  // Server -> None
  const UdpSendOption send_opt(MessagePriority::Ring1, EngineOnlyFeature);
  SendWhenReady(HostId_None, filter_tag, dest_addr, msg, send_opt);
}

int32 UdpSocket_S::GetOverSendSuspectingThresholdInByte() {
  return owner_->settings_.over_send_suspecting_threshold_in_byte;
}

void UdpSocket_S::LongTick(double absolute_time) {
  AssertIsLockedByCurrentThread();

  // Processing for the transmit queue
  {
    AssertIsFraggerNotLockedByCurrentThread();
    CScopedLock2 fragger_guard(GetFraggerMutex());

    packet_fragger_->LongTick(absolute_time);
  }

  // Processing for the receive queue
  if (packet_defragger_) {
    packet_defragger_->LongTick(absolute_time);
  }
}

void UdpSocket_S::EnqueuePacketDefragWarning(const InetAddress& sender,
                                             const char* text) {
  owner_->EnqueuePacketDefragWarning(sender, text);
}

int32 UdpSocket_S::GetMessageMaxLength() {
  return owner_->GetMessageMaxLength();
}

HostId UdpSocket_S::GetSrcHostIdByAddrAtDestSide_NOLOCK(
    const InetAddress& address) {
  return owner_->GetSrcHostIdByAddrAtDestSide_NOLOCK(address);
}

HostId UdpSocket_S::GetLocalHostId() { return owner_->GetLocalHostId(); }

void UdpSocket_S::Decrease() { DecreaseUseCount(); }

void UdpSocket_S::OnIssueSendFail(const char* where,
                                  SocketErrorCode socket_error) {}

CCriticalSection2& UdpSocket_S::GetSendMutex() {
  return udp_pakcet_fragger_mutex_;
}

CCriticalSection2& UdpSocket_S::GetMutex() { return mutex_; }

}  // namespace net
}  // namespace fun
