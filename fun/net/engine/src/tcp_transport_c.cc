#include "fun/net/net.h"
#include "tcp_transport_c.h"

namespace fun {
namespace net {

TcpTransport_C::TcpTransport_C(ITcpTransportOwner_C* owner)
    : recv_stream_(NetConfig::stream_grow_by) {
  owner_ = owner;
  owner_->LockMain_AssertIsLockedByCurrentThread();

  send_issued_ = false;
  recv_issued_ = false;
  total_tcp_issued_send_bytes_ = 0;

  socket_.Reset(new InternalSocket(SocketType::Tcp, owner_, owner_->GetIoCompletionDelegate()));

#ifdef CHECK_BUF_SIZE
  int32 send_buffer_length;
  int32 recv_buffer_length;
  socket_->GetSendBufferSize(&send_buffer_length);
  socket_->GetRecvBufferSize(&recv_buffer_length);
#endif

  socket_->SetCompletionContext((ICompletionContext*)this);
  owner_->AssociateSocket(socket_.Get());

  // socket의 send buffer를 없앤다. CSocketBuffer가 non swappable이므로 안전하다.
  // recv buffer 제거는 백해무익이므로 즐
#ifdef ALLOW_ZERO_COPY_SEND // zero copy send는 빠르지만 너무 많은 nonpaged를 유발 위험. 따라서 이걸로 막자. 막으니 성능도 더 나은데?
  socket_->SetSendBufferSize(0);
#endif

  NetUtil::SetTcpDefaultBehavior(socket_.Get());

  last_recv_invoke_warning_time_ = 0;
  last_send_invoked_warning_time_ = 0;

  // recv buffer를 크게 잡을수록 OS 부하가 커진다. 특별한 경우가 아니면 이것은 건드리지 않는 것이 좋다.
  //socket_->SetRecvBufferSize(NetConfig::ClientTcpRecvBufferLength);
}

void TcpTransport_C::SendWhenReady( const SendFragRefs& data_to_send,
                                    const TcpSendOption& send_opt) {
  // remote lock혹은 toServerTcp lock이 걸려있어야 함을 확인

  // add message to send queue for each remote host
  SendFragRefs final_send_data;
  MessageOut header;
  AddStreamHeader(data_to_send, final_send_data, header);

  //const bool bSignal = (send_queue_.GetLength() == 0);

  // 송신큐에 넣음
  // 최초 송신 이슈는 10ms마다 하므로(사람이 못느끼며 TCP nagle off시 coalesce 효과) iocp post는 불필요.
  // 결론적으로 10ms 내의 송신 데이터들은 뭉쳐서(coalesce) 보낸다는 얘기..
  send_queue_.EnqueueCopy(final_send_data, send_opt);

  //// 워커스레드에서 10ms마다 최초 송신 이슈를 걸기 때문에 신호를 던질 필요 없음
  //// 그러나 최초 송신 필요함 리스트에는 등록을 해야 함
  //owner_->EnqueSendIssueSignal(false);
}

void TcpTransport_C::ConditionalIssueSend() {
  // main lock이 걸려있지 않음을 확인
  //owner_->LockMain_AssertIsNotLockedByCurrentThread();
  //owner_->LockRemote_AssertIsLockedByCurrentThread();

  // 소켓이 건재하고 송신 미이슈 상태에 한해
  if (!socket_->IsClosed() && !send_issued_) {
    const int32 braked_send_amount = GetBrakedSendAmount(owner_->GetAbsoluteTime());
    if (braked_send_amount > 0) {
      // 상한선이 넘어도 문제 없다. 어차피 버퍼는 safe하고 TCP socket이 가득 차면 어차피 completion만 늦을 뿐이니까.

      FragmentedBuffer send_buffer;
      send_queue_.FillSendBuf(send_buffer, braked_send_amount);

      send_issued_ = true;
      const SocketErrorCode result = socket_->IssueSend_NoCopy(send_buffer);
      // 송신 실패(ERROR_IO_PENDING은 이미 제외됐음)가 오면 소켓을 닫는다
      //   => TCP issue recv or recv completion에서 소켓 닫힘이 감지되며, OnLeftFromServer로 이어질 것임.
      if (result != SocketErrorCode::Ok) {
        send_issued_ = false;
        socket_->CloseSocketHandleOnly(); // induce disconnecting.

        owner_->OnSocketWarning(socket_.Get(), String::Format("%s : issue-send failed -error:%d", __FUNCTION__, (int32)result));
      } else {
        send_queue_.send_brake_.Accumulate(braked_send_amount);
        send_queue_.send_speed_.Accumulate(braked_send_amount, owner_->GetAbsoluteTime());

        last_send_invoked_warning_time_ = 0;

        total_tcp_issued_send_bytes_ += send_buffer.Length();
      }
    }
  }
}

SocketErrorCode TcpTransport_C::IssueRecvAndCheck() {
  fun_check(!recv_issued_);

  if (!recv_issued_ && !socket_->IsClosed()) {
    recv_issued_ = true;

    const SocketErrorCode socket_error = socket_->IssueRecv(NetConfig::tcp_issue_recv_length);
    if (socket_error != SocketErrorCode::Ok) {
      recv_issued_ = false;
      return socket_error;
    } else {
      last_recv_invoke_warning_time_ = 0;
    }
  }

  return SocketErrorCode::Ok;
}

void TcpTransport_C::OnCloseSocketAndMakeOrphant() {
  socket_->CloseSocketHandleOnly();
}

TcpTransport_C::~TcpTransport_C() {
  owner_->LockMain_AssertIsLockedByCurrentThread();

  fun_check(!send_issued_);
  fun_check(!recv_issued_);
}

int32 TcpTransport_C::GetBrakedSendAmount(double absolute_time) {
  const int32 queued_data_length = send_queue_.GetLength();
  return MathBase::Min<int32>(send_queue_.send_brake_.GetMaxSendAmount(absolute_time, send_queue_.allowed_max_send_speed_.GetValue()), queued_data_length);
}

void TcpTransport_C::LongTick(double absolute_time) {
  send_queue_.LongTick(absolute_time);
}

void TcpTransport_C::SetEnableNagleAlgorithm(bool enable) {
  socket_->EnableNagleAlgorithm(enable);
}

void TcpTransport_C::RefreshLocalAddress() {
  local_addr_ = socket_->GetSockName();

  if (!local_addr_.IsUnicast()) {
    //서버와 아직 연결이 안된 상태에서는 "0.0.0.0"의 주소가 나올것임.
    //그러나, binding된 port번호를 얻기만 하면 되므로 이 러한 상황이 필요할 수 있음.
    //다른곳에서 오류가 안나도록 로컬 주소를 찾아서 대입시켜주자.
    //단, 포트 번호는 유지해야함.

    //TODO 어드레스 패밀리를 고려해야하지 않을런지??

    IpAddress fallback_addr;
    const auto& local_addresses = Dns::ThisHost().Addresses();
    for (const auto& addr : local_addresses) {
      if (addr.GetAddressFamily() == AddressFamily::IPv4) {
        fallback_addr = addr;
        break;
      }
    }

    if (fallback_addr.IsUnicast()) {
      local_addr_ = InetAddress(fallback_addr, local_addr_.GetPort());
    }
  }
}

bool TcpTransport_C::IsSocketClosed() {
  return socket_.IsValid() == false || socket_->IsClosedOrClosing();
}

} // namespace net
} // namespace fun
