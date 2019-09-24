//TODO 계속사용할지 여부 검토.
#include "fun/net/net.h"
#include "send_speed_mesasurer.h"

namespace fun {
namespace net {

uint8 SendSpeedMeasurer::g_send_packet[SendSpeedMeasurer::PAKCET_LENGTH] = {0, };
InetAddress SendSpeedMeasurer::g_sendto = InetAddress("211.214.161.39", 80);  // TODO: 나중에 제대로 고칠것

SendSpeedMeasurer::SendSpeedMeasurer(ISendSpeedMeasurerDelegate* delegate) {
  completion_port_.Reset(new CompletionPort(this, true, 1));

  force_measure_now_ = false;
  delegate_ = InDelegate;
  should_stop_ = false;

  send_issued_ = false;
  recv_issued_ = false;

  // UDP 소켓 생성 및 ttl 수정
  send_socket_.Reset(new InternalSocket(SocketType::Udp, this));
  send_socket_->Bind();
  send_socket_->SetSendBufferSize(0); // 이렇게 해야 송신량 제어가 된다.
  send_socket_->SetTTL(4);

  // 수신용 소켓
  recv_socket_.Reset(new InternalSocket(SocketType::Raw, this));
  recv_socket_->Bind();

  // IOCP에 UDP 소켓 등록
  send_socket_->SetCompletionContext((ICompletionContext*)this);
  completion_port_->AssociateSocket(send_socket_.Get());

  recv_socket_->SetCompletionContext((ICompletionContext*)this);
  completion_port_->AssociateSocket(recv_socket_.Get());

  start_time_ = 0;
  measuring_ = false;

  last_measured_send_speed1_ = 0;

  clock_.Start();

  thread_.Reset(RunnableThread::Create(this, "SendSpeedMeasurer"));
}

SendSpeedMeasurer::~SendSpeedMeasurer() {
  IssueTermination();
  WaitUntilNoIssueGuaranteed();

  recv_socket_.Reset();
  send_socket_.Reset();

  completion_port_.Reset();

  fun_check(!measuring_); // 이미 청소된 상태이어야 한다.
}

void SendSpeedMeasurer::Heartbeat() {
  mutex_.AssertIsNotLockedByCurrentThread(); // 이벤트 콜백땜시
  ScopedLock<FastMutex> guard(mutex_);

  if (measuring_) {
    measure_heartbeat_count_++;

    if (measure_heartbeat_count_ == 1) {
      // first issue를 건다.
      IssueSendTo();
      ConditionalIssueRecvFrom();
    } else {
      if ((GetAbsoluteTime() - start_time_) > NetConfig::measure_send_speed_duration_sec || force_measure_now_) {
        // 총계를 산정한다.
        last_measured_send_speed1_ = CalcMeasuredSendSpeed();

        force_measure_now_ = false;
        measuring_ = false;

        double last_measured_send_speed = last_measured_send_speed1_;

        guard.Unlock(); // 이벤트 콜백 전에 필수

        delegate_->OnMeasureComplete(last_measured_send_speed);
      }
    }
  }
}

void SendSpeedMeasurer::RequestProcess() {
  if (!measuring_ && send_socket_.IsValid() && !send_socket_->IsClosed()) {
    measuring_ = true;
    measure_heartbeat_count_ = 0;

    start_time_ = GetAbsoluteTime();

    send_complete_length1_ = 0;
  }
}

void SendSpeedMeasurer::IssueTermination() {
  ScopedLock<FastMutex> guard(mutex_);

  if (send_socket_.IsValid()) {
    send_socket_->CloseSocketHandleOnly();
  }

  if (recv_socket_.IsValid()) {
    recv_socket_->CloseSocketHandleOnly();
  }
}

void SendSpeedMeasurer::WaitUntilNoIssueGuaranteed() {
  while (true) {
    ScopedLock<FastMutex> guard(mutex_);

    // 프로세스 종료중이면 아무것도 하지 않는다.
    //TODO
    //if (CThread::bDllProcessDetached_INTERNAL) {
    //  return;
    //}

    if (!send_issued_ && !recv_issued_) {
      measuring_ = false;

      send_socket_.Reset(); // invalidate
      recv_socket_.Reset(); // invalidate

      guard.Unlock();

      should_stop_ = true;
      thread_->Join();

      return;
    } else {
      guard.Unlock();

      CPlatformProcess::Sleep(0.01f);
    }
  }
}

void SendSpeedMeasurer::OnSocketWarning(InternalSocket* socket, const string& msg) {
  int32 x = 0;
}

void SendSpeedMeasurer::ProcessIoCompletion(CompletionStatus& completion) {
  ScopedLock<FastMutex> guard(mutex_);

  fun_check(NetConfig::measure_send_speed_duration_sec > 0.1);
  fun_check(NetConfig::measure_send_speed_duration_sec < 1.0); // 너무 짧으면 측정 부정확. 너무 길면 호스트 과부하.

  // <=0 이 맞음..
  if (completion.completed_length <= 0) {
    force_measure_now_ = true;
  }

  // 송신완료시
  if (completion.type == CompletionType::Send) {
    if (measuring_) {
      // 총계 누적
      if (completion.completed_length > 0) {
        send_complete_length1_ += completion.completed_length;

        // 다음 이슈
        IssueSendTo();
      }
    } else {
      send_issued_ = false;
    }
  } else if (completion.type == CompletionType::Receive) { // 수신 완료시
    if (measuring_) {
      // ICMP 패킷 분석해서 결과 처리하기
      if (completion.completed_length > 0) {
        fun_check(packet_len_ > 40);
        //FUN_TRACE("LLL = %d\n", completion.completed_length);
        //send_complete_length1_ += completion.completed_length;
      }

      // 다음 이슈, 에러코드와 무관하게
      ConditionalIssueRecvFrom();
    } else {
      recv_issued_ = false;
    }
  }
}

void SendSpeedMeasurer::IssueSendTo() {
  mutex_.AssertIsLockedByCurrentThread();

  send_issued_ = true;
  if (send_socket_->IssueSendTo(g_send_packet, packet_len_, g_sendto) != SocketErrorCode::Ok) {
    send_issued_ = false;
    measuring_ = false;
  }
}

void SendSpeedMeasurer::ConditionalIssueRecvFrom() {
  mutex_.AssertIsLockedByCurrentThread();

  if (!recv_socket_->IsClosedOrClosing()) {
    recv_issued_ = true;
    if (recv_socket_->IssueRecvFrom(packet_len_) != SocketErrorCode::Ok) {
      recv_issued_ = false;
      measuring_ = false;
    }
  }
}

void SendSpeedMeasurer::RequestStopProcess() {
  measuring_ = false;
}

void SendSpeedMeasurer::Run() {
  while (!should_stop_) {
    Heartbeat();

    CompletionStatus status;
    if (completion_port_->GetQueuedCompletionStatus(&status, 10)) {
      ProcessIoCompletion(status);
    }
  }
}

double SendSpeedMeasurer::CalcMeasuredSendSpeed() {
  const double time_len = GetAbsoluteTime() - start_time_;
  const double total_len = (double)send_complete_length1_;

  double speed;
  if (time_len > 0) {
    speed = total_len / time_len;
  } else {
    speed = 0;
  }

  return MathBase::Max(last_measured_send_speed1_, speed);
}

} // namespace net
} // namespace fun
