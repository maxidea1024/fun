#pragma once

//#include "RUdpConfig.h"
#include "RUdpFrame.h"
#include "StreamQueue.h"

namespace fun {
namespace net {

class RUdpHost;

/**
 * Reliable UDP 수신기
 * window size나 ack, frame 등의 용어는 일반적인 data link layer 및 TCP
 * 프로토콜의 용어에서와 같다. 단독으로는 쓰이지 못하고, RUdpSender와 쌍을
 * 맺어야 한다. RUdpSender를 통해 ack를 보내니까. 주요 메서드:
 * ProcessRawReceivedFrames, Tick, FlushAckAccumulatedToOutput
 */
class RUdpReceiver {
 private:
  RUdpHost* owner_;

 public:
  RUdpReceiver(RUdpHost* owner, FrameNumber first_frame_number);

  void Tick(float elapsed_time);
  void SendGatheredAcks();
  void ProcessReceivedFrame(RUdpFrame& frame);
  void ProcessDataFrame(RUdpFrame& frame);
  void FlushAckAccumulatedToOutput();
  void AddToReceivedFrames(RUdpFrame& received_frame);
  void ProcessAckFrame(RUdpFrame& frame);
  bool IsTooOldFrame(FrameNumber frame_id) const;

 public:
  double last_send_gathered_acks_time_;

  /**
  이번에 플러쉬될 프레임의 기대하는 ID. 즉 수신자에서 이 값의 패킷이 올 차례가
  되어야 한다.
  */
  FrameNumber expected_frame_number_;

  FrameNumber last_received_data_frame_number_;

  /**
  수신된 데이터 프레임에 대한 ack를 보내되 한번에 여러개를 보내기 위해 일단
  모아놓는 공간 짧은 순간에만 사용되는 변수
  */
  Array<FrameNumber> acks_to_send_;

  /** UDP socket에서 꺼내긴 했지만 아직 스트림으로 조립되지 않고 대기중인
   * 프레임들. 즉 receiver window이다. */
  List<ReceiverFrame> receiver_window_;

  /** 최종적으로 받은 데이터 스트림. 이건 TCP처럼 쓰이며 유저에게 넘어간다. */
  StreamQueue recv_stream_;

  /** 통계 */
  int32 total_received_stream_length_;

  /** 수신측에서 ack 프레임을 쏜 횟수 */
  int32 total_ack_frame_count_;

  /** 최근에 여기서 수신한 바이트 수 */
  int32 recent_receive_frame_count_;

  /** 최근에 여기서 수신한 바이트 수 기준 시작 시간 */
  double recent_receive_frame_count_start_time_;

  /** 최근에 여기서 수신한 속도의 최근 근사치 */
  int32 recent_receive_speed_;

  int32 total_receive_data_frame_count_;
};

}  // namespace net
}  // namespace fun
