//TODO 코드정리
// 바로 데이터를 전송하는 형태가 아닌, SendQueue에 데이터를 넣어준 후
// 상황에 맞추어서(제어) 전송하는 형태임

#include "fun/net/net.h"
#include "tcp_transport_s.h"
#include "send_brake.h"

namespace fun {
namespace net {

TcpTransport_S::TcpTransport_S( ITcpTransportOwner_S* owner,
                                InternalSocket* socket,
                                const InetAddress& remote_addr)
    : recv_stream_(NetConfig::stream_grow_by) {
  owner_ = owner;
  cached_remote_addr_ = remote_addr;

  send_issued_ = false;
  recv_issued_ = false;

  if (socket) {
    socket_.Reset(socket);

#ifdef CHECK_BUF_SIZE
    int32 send_buffer_length
    int32 recv_buffer_length;
    socket_->GetSendBufferSize(send_buffer_length);
    socket_->GetRecvBufferSize(recv_buffer_length);
#endif

    // socket의 send buffer를 없앤다. CSocketBuffer가 non swappable이므로 안전하다.
    // recv buffer 제거는 백해무익이므로 즐
    //
    // zero copy send는 빠르지만 너무 많은 nonpaged를 유발 위험.
    // 따라서 이걸로 막자. 막으니 성능도 더 나은데?
#ifdef ALLOW_ZERO_COPY_SEND
    socket_->SetSendBufferSize(0);
#endif

    NetUtil::SetTcpDefaultBehavior(socket_);
  }

#if TRACE_ISSUE_DELAY_LOG
  last_recv_issue_warning_time_ = 0;
  last_send_issue_warning_time_ = 0;
#endif

  total_tcp_issued_send_bytes_ = 0;

  // recv buffer를 크게 잡을수록 OS 부하가 커진다.
  // 따라서, 특별한 경우가 아니면 이것은 건드리지 않는 것이 좋다.
  //socket_->SetRecvBufferSize(NetConfig::ServerTcpRecvBufferLength);
}

//@note SendQueue에 데이터를 넣어 놓으면 스케줄러가 보내는 형태인듯 함.
void TcpTransport_S::SendWhenReady(const SendFragRefs& data_to_send, const TcpSendOption& send_opt) {
  AssertIsSendQueueLockedByCurrentThread();

  SendFragRefs final_send_data;
  MessageOut header;
  AddStreamHeader(data_to_send, final_send_data, header);

  // 바로 보내지 않고 send-qeueue에 집어 넣어주고, 스케줄러가 필요에 따라서 보내는 형태로 처리함.
  // 복사를 제거할 수 있는 방법이 있다면 좋을듯..
  // 그렇게 하기 위해서는, CSendFragRefs가 raw포인터가 아닌 참조포인터를 가져야할듯..
  send_queue_.EnqueueCopy(final_send_data, send_opt);

  // 아직 이슈된 상태라면, 이슈된작업이 끝나자마자 큐잉된 작업을 시도하겠지만,
  // 아직 이슈된 작업이 없다면, 여기서 큐잉해주어야함..
  if (!send_issued_) {
    //TODO 이 함수가 주기적으로 타이머에 의해서 호출되지 않던가??
    //보내야할 대상에 링크시킴. Send 스케줄러는 정해진 간격(0.003초)마다 모아서(coalesce) 전송할것임.
    owner_->EnqueueIssueSendReadyRemotes();
  }
}

//SendBreak(Throttling)는 사용하지 않지만, 일정시간동안 모아서 전송하는 역활은 하므로, 아예 필요 없지는 않을듯 싶음.

// 이놈은 가끔씩 불리면서, 큐에 들어있는 데이터를 네트워크 너머로 보내는 역활을 수행.
SocketErrorCode TcpTransport_S::ConditionalIssueSend(double absolute_time) {
  owner_->LockMain_AssertIsNotLockedByCurrentThread(); // not main lock
  AssertIsLockedByCurrentThread();//cs lock

  SocketErrorCode result = SocketErrorCode::Ok;

  // 이미 닫힌 소켓이면 issue를 하지 않는다.
  // 만약 닫힌 소켓에 issue하면 completion이 어쨌거나 또 발생한다. 메모리도 긁으면서.
  // 송신할게 있는지 체크하기 전에 이걸 먼저 체크해야 한다. 반드시!
  if (!socket_->IsClosedOrClosing() && !send_issued_ && !owner_->IsDispose()) {
    CScopedLock2 send_queue_guard(GetSendQueueMutex());

    const int32 braked_send_amount = GetBrakedSendAmount(absolute_time);
    if (braked_send_amount > 0) {
      // 상한선이 넘어도 문제 없다. 어차피 버퍼는 safe하고
      // TCP socket이 가득 차면 어차피 completion만 늦을 뿐이니까.

      // 최종적으로 소켓 send 함수에 넘겨질 형태로 만듬.
      FragmentedBuffer send_buf;
      send_queue_.FillSendBuf(send_buf, braked_send_amount);

      // 아래 두라인 issueSend_NoCopy아래 있었으나, lock관계로 위로 옮김.
      send_queue_.send_brake_.Accumulate(braked_send_amount);
      send_queue_.send_speed_.Accumulate(braked_send_amount, absolute_time);

      send_queue_guard.Unlock();

      send_issued_ = true;
      result = socket_->IssueSend_NoCopy(send_buf);
      if (result != SocketErrorCode::Ok) {
        send_issued_ = false;
      } else {
#if TRACE_ISSUE_DELAY_LOG
        last_send_issue_warning_time_ = 0;
#endif

        total_tcp_issued_send_bytes_ += send_buf.Length();
      }
    }
  }

  return result;
}

SocketErrorCode TcpTransport_S::IssueRecvAndCheck() {
  owner_->LockMain_AssertIsNotLockedByCurrentThread();
  AssertIsLockedByCurrentThread();

  if (!recv_issued_ && !socket_->IsClosedOrClosing()) {
    if (!owner_->IsDispose()) {
      recv_issued_ = true;
      const SocketErrorCode socket_error = socket_->IssueRecv(NetConfig::tcp_issue_recv_length);
      if (socket_error != SocketErrorCode::Ok) {
        recv_issued_ = false;
        return socket_error; // 외부에서 종료 절차를 밟을것임.
      } else {
#if TRACE_ISSUE_DELAY_LOG
        last_recv_issue_warning_time_ = 0;
#endif
      }
    }
  }

  return SocketErrorCode::Ok;
}

TcpTransport_S::~TcpTransport_S() {
  fun_check(!recv_issued_ && !send_issued_);

  owner_->LockMain_AssertIsLockedByCurrentThread();

#ifdef _DEBUG
  //@todo IsTearDown() 이런 이름은 어떨런지??
  //      가만, 그런데 의미 없다. 조 위의 fun_check(!recv_issued_ && !send_issued_) 는 머냐??
  //if (!owner_->IsValidEnd())//owner_->owner_->tcp_listening_socket_.IsValid()) {
  //  // 서버 종료 상황이 아니라면 아래 체크를 한다.
  //  fun_check(send_issued_ == false);
  //  fun_check(recv_issued_ == false);
  //}
#endif
}

int32 TcpTransport_S::GetBrakedSendAmount(double absolute_time) {
  AssertIsSendQueueLockedByCurrentThread();

  const int32 queued_data_length = send_queue_.GetLength();
  return MathBase::Min(send_queue_.send_brake_.GetMaxSendAmount(absolute_time, send_queue_.allowed_max_send_speed_.GetValue()), queued_data_length);
}

void TcpTransport_S::LongTick(double absolute_time) {
  AssertIsSendQueueLockedByCurrentThread();

  send_queue_.LongTick(absolute_time);
}

void TcpTransport_S::SetEnableNagleAlgorithm(bool enabled) {
  socket_->EnableNagleAlgorithm(enabled);
}

} // namespace net
} // namespace fun
