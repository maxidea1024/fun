#include "RUdpHost.h"
#include "RUdpSender.h"
#include "fun/net/net.h"

namespace fun {
namespace net {

RUdpSender::RUdpSender(RUdpHost* InOwner, FrameNumber InFirstFrameNumber)
    : send_stream_(NetConfig::stream_grow_by) {
  // 적정한 값이 들어있는지 체크
  // 행여나 컴파일러의 버그로 잘못된 값이 들어가는 경우 찾기 위해서다.
  fun_check(RUdpConfig::max_ack_count_in_one_frame > 0);
  fun_check(RUdpConfig::frame_length > 10);

  owner = InOwner;

  last_do_stream_to_sender_window_time_ = 0;

  last_acK_send_time_elapsed_ = 0;
  last_data_send_time_elapsed_ = 0;

  total_send_stream_length_ = 0;
  total_resend_count_ = 0;
  total_first_send_count_ = 0;

  recent_send_frame_to_udp_count_ = RUdpConfig::received_speed_before_update;
  recent_send_frame_to_udp_start_time_ = 0;
  recent_send_frame_to_udp_speed_ = RUdpConfig::received_speed_before_update;

  send_speed_limit_ = RUdpConfig::max_send_speed_in_frame_count;
  remote_recv_speed_ = RUdpConfig::received_speed_before_update;

  current_frame_number_ = InFirstFrameNumber;

  last_expected_frame_number_at_sender_ = (FrameNumber)0;

  max_resend_elapsed_time_ = 0;

  last_received_ack_frame_number_ = InFirstFrameNumber;
  last_received_ack_time_ = 0;

  total_ack_frame_recv_count_ = 0;
}

void RUdpSender::SendViaReliableUDP(const uint8* StreamToAdd, int32 Length) {
  send_stream_.EnqueueCopy(StreamToAdd, Length);

  // TODO 이게 누적치인가?  어디서도 빼주는데가 없네... 흠... 좀더 확인을...
  total_send_stream_length_ += Length;

  // LOG(LogNetEngine,Warning,"send_stream_ length: %d",
  // send_stream_.GetLength());
}

void RUdpSender::Tick(float elapsed_time) {
  CalcRecentSendSpeed();

  // last ack send time 증가
  last_acK_send_time_elapsed_ += elapsed_time;
  last_data_send_time_elapsed_ += elapsed_time;

  // if recent send speed is too greater than remote receive speed...
  const double send_speed = (double)recent_send_frame_to_udp_speed_;
  const double remote_recv_speed = (double)remote_recv_speed_;
  if ((send_speed * RUdpConfig::brake_max_send_speed_threshold) >
      remote_recv_speed) {
    send_speed_limit_ = (int32)remote_recv_speed + 1;
  } else {
    send_speed_limit_ = RUdpConfig::max_send_speed_in_frame_count;
  }

  // 마지막 stream enque 직후 not on need에 의해 stream to frames가 실행되지
  // 않아있을 수 있다. 따라서 매 짧은 순간마다 아래 메서드의 호출은 필수.
  ConditionalStreamToSenderWindow(false);

  //이 두개 함수의 순서는 바뀌어선 안된다. 초송신 되었던것이, 재송신이 일어나면
  //안되므로...
  ConditionalResendWindowToUdpSender(elapsed_time);
  ConditionalFirstSenderWindowToUdpSender();
  // ConditionalSenderWindowToUdpSender(elapsed_time);
}

void RUdpSender::CalcRecentSendSpeed() {
  const double absolute_time = owner->Delegate->GetAbsoluteTime();

  if (recent_send_frame_to_udp_start_time_ == 0) {
    recent_send_frame_to_udp_start_time_ = absolute_time;
  }

  if ((absolute_time - recent_send_frame_to_udp_start_time_) >
      RUdpConfig::calc_recent_receive_interval) {
    const double T =
        0.1 / (absolute_time - recent_send_frame_to_udp_start_time_);
    recent_send_frame_to_udp_speed_ =
        (int32)MathBase::Lerp((double)recent_send_frame_to_udp_speed_,
                              (double)recent_send_frame_to_udp_count_, T);
    recent_send_frame_to_udp_count_ = 0;
    recent_send_frame_to_udp_start_time_ = absolute_time;
  }
}

// 초송신 window에 있는 것들을 하위 layer에 쏜다.
// 초송신된것은 resend window로 옮긴다.
void RUdpSender::ConditionalFirstSenderWindowToUdpSender() {
  // 유지보수중에 누군가가 값을 잘못 넣으면 뻑 내기 위함
  fun_check(RUdpConfig::first_resend_cooltime <=
            RUdpConfig::max_resend_cooltime);

  const double absolute_time = owner->Delegate->GetAbsoluteTime();

  // 상대측과의 랙이 처음 재송신 조건에 영향을 줌
  // double recent_ping = owner->Delegate->GetRecentPing() * 2; // round
  // trip이므로 *2

  // cached here
  const bool is_reliable_channel = owner->Delegate->IsReliableChannel();

  // 송신 큐에 있는 것들 중에서 초송신 시행
  for (auto it = first_sender_window_.CreateIterator(); it; ++it) {
    auto& frame = *it;

    // 시간값 갱신
    frame.last_send_time = absolute_time;
    frame.first_send_time = absolute_time;

#ifdef OLD_FIRST_RESEND_INTERVAL
    // 상대와의 핑(랙) 시간값 만큼 초송신 시간을 설정한다.
    if (recent_ping > 0) {
      frame.resend_cooltime = recent_ping;
    } else {
      frame.resend_cooltime = RUdpConfig::first_resend_cooltime;
    }
#else
    // 초기 재송신 인터벌을 핑 등 너무 짧은 값을 쓰니 패킷 로스 발생이
    // 조금이라도 나면 트래픽이 확 증가. 따라서 핑이 아니라 제법 긴 값을 그냥
    // 쓰도록 하자.
    frame.resend_cooltime = RUdpConfig::first_resend_cooltime;
#endif
    // 그래도 상한선은 지키도록 하자. 핑 시간이 잘못 측정된 경우를 위해.
    frame.resend_cooltime =
        MathBase::Min(frame.resend_cooltime, RUdpConfig::max_resend_cooltime);

    // 총 초송신 횟수 기록
    total_first_send_count_++;

    // 이제 보내자
    SendOneFrame(frame, false);  // first send

    // 초송신을 보내자마자 재송신 윈도우에 넣어둠.
    // 재송신큐에 있는 것들은 일정시간 ACK가 오지 않을 경우, 재전송되는 형태임.
    // 만약 일정시간 즉, frame.ResendCoolTime안에 ACK가 오게 되면 정상적으로
    // 수신된걸로 처리하고 ResendWindow에서 해당 프레임을 제거한다.

    // 타 reliable 채널로 보내진것이 아니면, Resend Window에 넣는다.
    // 아니라면, Resend Window에 넣을 필요가 없다(재 전송을 하지 않을것이므로, )
    if (!is_reliable_channel) {
      resend_window_.Append(frame);
    }

    // 초송신 윈도우에서 제거
    first_sender_window_.Remove(it);
  }
}

// 현재 이 함수에서 엄청난 부하가 발생하고 있다. 왜일까...
// ++It가 누락되어서, 무한 루프에 빠져 있었음. VS Profiler 쌩유!
void RUdpSender::ConditionalResendWindowToUdpSender(double elapsed_time) {
  const double absolute_time = owner->Delegate->GetAbsoluteTime();

  // 상대측과의 랙이 처음 재송신 조건에 영향을 줌
  // double recent_ping = owner->Delegate->GetRecentPing() * 2; // round
  // trip이므로 *2

  // 이번 턴에서 최대 몇 개까지 보낼 것인가
  // (일단은 float로 연산 후 int로 캐스팅한다.)
  double fLimitCount =
      double(resend_window_.Count()) * RUdpConfig::resend_limit_ratio;

  // 초당 재송신 가능한 최대 & 최소 범위로 조절
  fLimitCount =
      MathBase::Max(fLimitCount, double(RUdpConfig::min_resend_limit_count));
  fLimitCount =
      MathBase::Min(fLimitCount, double(RUdpConfig::max_resend_limit_count));

  // 초당 재송신 가능한거니까
  fLimitCount *= elapsed_time;

  // 클라가 버벅대고 있는 상황(즉 스레드 기아화)에서는 elapTime이 상당히
  // 길어진다. 이런 상황에서는 재전송할 것들이 계속 누적되는데, 이를 계속
  // 재전송하려 하면 악순한의 고리가 이어진다. 이 루틴은
  // client_heartbeat_interval_msec 간격으로 콜 되고 있다. 그리고 기아화가
  // 발생하면 elapsed time은 길어질 것이다. 기아화 상황에서는 초당 보내는 양을
  // 감하도록 하자.
  const double ExpectedElapTime = NetConfig::rudp_heartbeat_interval_sec;
  if (elapsed_time >
      ExpectedElapTime * 5) {  // 어느 정도 제법 격차가 차이나야 기아화를 의심할
                               // 수 있으므로 이런 조건문을 넣는다.
    const double DecayRatio =
        elapsed_time / ExpectedElapTime;  // 5보다는 클 것임
    fLimitCount /= DecayRatio;
  }

  // 그래도 최소 1개는 되어야 한다.
  int32 LimitCount = MathBase::Max(1, int32(fLimitCount));

  // 현재 reliable 채널을 사용하고 있는지 여부 확인.
  // reliable 채널을 사용중이라면, reliable 채널(TCP)로 이미 보내졌을 것이므로,
  // 여기서는 보낸걸로 간주하고, resend 윈도우에서 제거합니다.
  const bool is_reliable_channel = owner->Delegate->IsReliableChannel();

  // 재송신 큐에 있는 것들 중에서 재송신
  for (auto it = resend_window_.CreateIterator(); it && LimitCount > 0; ++it) {
    auto& FrameToResend = *it;

    bool bRemoved = false;

    if ((absolute_time - FrameToResend.last_send_time) >
        FrameToResend.resend_cooltime) {  // 재송신 조건을 만족시
      // 최악의 재송신 시도라면, 기록을 남긴다.
      if (FrameToResend.first_send_time > 0) {
        max_resend_elapsed_time_ =
            MathBase::Max(max_resend_elapsed_time_,
                          absolute_time - FrameToResend.first_send_time);
      }

      // 재송신

      // 재송신 횟수가 실패할수록 재송신을 좀 더 느긋하게 보내도록 한다.
      // FrameToResend.resend_cooltime *=
      // RUdpConfig::enlarge_resend_cooltime_ratio;
      FrameToResend.resend_cooltime = MathBase::Min(
          FrameToResend.resend_cooltime, RUdpConfig::max_resend_cooltime);
      FrameToResend.last_send_time = absolute_time;
      FrameToResend.resend_count++;

      total_resend_count_++;

      // 프레임 하나를 네트워크로 전송함.
      SendOneFrame(FrameToResend, true);  // resend

      // 타 reliable 채널로 보내진거면(TCP 릴레이) resend가 불필요하므로
      // 송신큐에서 제거한다.
      // TODO 이미 타 채널로 보내진 상황이라면 재송신을 아예 말아야하지 않을까??
      if (is_reliable_channel) {
        resend_window_.Remove(it);
      }

      // 한번에 재송신 가능한 횟수 제한이 있다. 이를 체크한다.
      --LimitCount;
    }
  }
}

// 스트림에 있는 데이터를 보낼 프레임으로 변환한다.
// 이 함수는 매 프레임마다 및 유저로부터 스트림 송신 요청이 있을 때 호출된다.
// 유저로부터 스트림 송신 요청이 있을 때 호출
void RUdpSender::ConditionalStreamToSenderWindow(bool move_now) {
  const double absolute_time = owner->delegate_->GetAbsoluteTime();

  if (!move_now) {
    /*
    아직 UDP layer에서 미송출된 데이터가 남아있는 상황(UDP layer filled) 이라면
    stream에 있는 것을 sender window로 보내는 것이 불필요하다.

    아직 미송출 초송신 frame이 남아있는 판국에 stream에 조금이라도 쌓인것을 족족
    frame으로 변환해봤자 frame의 payload 크기만 작은 것이 다수 생길 뿐이니까.

    이때는 차라리 stream이 더 많이 쌓이게 냅둬야 coalesce 효과가 느므로 ok.

    그렇다고 UDP layer filled의 종료를 마냥 기다리면 starvation이 발생하므로
    최대 대기 한계 시간을 두도록 한다.
    */
    if ((absolute_time - last_do_stream_to_sender_window_time_) >
            RUdpConfig::stream_to_sender_window_coalesce_interval ||
        owner->delegate_->IsUdpSendBufferPacketEmpty()) {
      move_now = true;
    }
  }

  if (move_now) {
    last_do_stream_to_sender_window_time_ = absolute_time;

    /*
    stream의 내용을 Frame으로 쪼개서 send queue에 넣는다.
    단, 지정된 최대 속도를 넘어갈 수는 없다.(위에서 얻은 양을 넘을 수 없다)
    TODO: (나중에 위 루틴을 1초동안 총 보낸 양이 속도 제한을 넘으면 안되게
    구현할 것. 즉 silly window syndrome을 제거할 것) *

    V회사 제보에 의하면 클라20개가 대량 통신시 이게 막히는 경우가 있었다고 한다!
    분석 결과, 클라 20개에서 send_speed_limit_<102까지 떨어진다!
    잘못 측정된 SendSpeedLimit는 없느니만 못하다. 따라서 아래와 같이 불필요
    부분을 막았다.
    */
    while (send_stream_.GetLength() > 0
      /*&& MovedToFrameCountTotal < (send_speed_limit_ - CurrentSendQueueTotalInFrameCount)*/) {
      const int32 Length =
          MathBase::Min(RUdpConfig::frame_length, send_stream_.GetLength());

      SenderFrame frame;
      frame.type = RUdpFrameType::Data;
      frame.frame_number = NextFrameNumber();
      frame.data.Append((const char*)send_stream_.ConstData(), Length);  // copy
      // frame.data = MessageIn(ByteArray((const char*)send_stream_.ConstData(),
      // Length)); // copy
      first_sender_window_.Append(frame);
      send_stream_.DequeueNoCopy(Length);

      // MovedToFrameCountTotal++;
    }
  }
}

// 다음 프레임 넘버를 생성.
FrameNumber RUdpSender::NextFrameNumber() {
  FrameNumber Next = current_frame_number_;
  // (++를 직접 사용하지 않는 이유는 overflow대응을 위해서)
  current_frame_number_ =
      FrameNumberUtil::NextFrameNumber(current_frame_number_);
  return Next;
}

// 프레임 하나를 네트워크로 전송.
void RUdpSender::SendOneFrame(RUdpFrame& frame, bool bResend) {
  owner->Delegate->SendOneFrameToUdpTransport(frame);

  if (frame.type == RUdpFrameType::Data) {
    // if (bResend) {
    //  LOG(LogNetEngine,Info,"* RUdpSender.SendOneFrame: [RESEND]    type=%s,
    //  frame=%d, len=%d", *ToString(frame.type), (int32)frame.frame_number,
    //  frame.data.Len());
    //} else {
    //  LOG(LogNetEngine,Info,"* RUdpSender.SendOneFrame: [FRISTSEND] type=%s,
    //  frame=%d, len=%d", *ToString(frame.type), (int32)frame.frame_number,
    //  frame.data.Len());
    //}

    recent_send_frame_to_udp_count_++;
    last_data_send_time_elapsed_ = 0;
  }
}

// 지정한 프레임 제거.
bool RUdpSender::RemoveFromSenderWindow(FrameNumber frame_id) {
  return resend_window_.RemoveOneIf([frame_id](const SenderFrame& frame) {
    return frame.frame_number == frame_id;
  });
}

// 주어진 프레임 넘버보다 이전 프레임 모두 제거.
void RUdpSender::RemoveSpecifiedAndItsPastsFromSenderWindow(
    FrameNumber frame_id) {
  for (auto it = resend_window_.CreateIterator(); it; ++it) {
    auto& frame = *it;

    if (FrameNumberUtil::Compare(frame.frame_number, frame_id) < 0) {
      resend_window_.Remove(it);
    }
  }
}

}  // namespace net
}  // namespace fun
