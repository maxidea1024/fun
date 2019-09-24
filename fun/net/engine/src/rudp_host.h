#pragma once

#include "RUdpReceiver.h"
#include "RUdpSender.h"

namespace fun {
namespace net {

class RUdpFrame;
class RUdpHostStats;

/**
 * Reliable UDP과 유저가 제공하는 UDP 층을 연결하는 방법
 * - 먼저 IRUdpHostDelegate를 구현한다. 그리고 RUdpHost 객체에 연결한다.
 * - UDP에서 받은 것을 처리: UDP frame인지 먼저 확인 후
 * RUdpHost.TakeReceivedFrame()에 넣는다.
 * - UDP로 보내기: IRUdpHostDelegate의 SendOneFrameToUdp() 호출
 * - 일정 시간마다: RUdpHost.Tick() 호출
 *
 * Reliable UDP를 통해 메시지 주고받는 방법
 * - 먼저 Reliable UDP에 최초 frame number를 지정해야 한다. 이는 생성자에서
 * 한다.
 * - reliable UDP의 송신 메인: RUdpHost.Send() 호출
 * - reliable UDP의 수신 메인: RUdpHost.PopReceivedStream() 호출
 *
 * 상대와의 연결 해제 기능은 내장되어 있지 않다. 어차피 relay server를 통해서
 * 중재하니까. 그러나 상대와의 연결이 정상이 아닌지 여부를 판단하는 방법으로
 * 지속적으로 P2P간 메시지를 주고 받으면서 최종 도착한 ack의 시간을 체크하는
 * 방법이 있다. 이를 위해 GetLastAckTimeElapsed()가 있다.
 *
 * 통계 정보 얻기: GetStats()
 */
class IRUdpHostDelegate {
 public:
  virtual ~IRUdpHostDelegate() {}

  virtual void SendOneFrameToUdpTransport(RUdpFrame& frame) = 0;

  /**
   * TCP 서버 릴레이 모드 등 UDP가 아닌, 신뢰성있는 통신 채널의
   * reliability를 타고 움직이는거면 true를 리턴한다.
   */
  virtual bool IsReliableChannel() = 0;

  /**
   *
   */
  virtual double GetAbsoluteTime() = 0;

  /**
   * UDP socket layer에서 아직 미송출된 송신대기 데이터의 총 길이를 리턴한다.
   */
  virtual int32 GetUdpSendBufferPacketFilledCount() = 0;

  /**
   * 이 함수가 별도로 빠져 있는건, 성능 최적화하다보니 필요해서.
   */
  virtual bool IsUdpSendBufferPacketEmpty() = 0;

  virtual HostId TEST_GetHostId() = 0;

  /**
   * 핑을 아직 못 체크했으면 <=0 이어야 한다.
   */
  virtual double GetRecentPing() = 0;
};

class RUdpHost {
 public:
  IRUdpHostDelegate* delegate_;
  RUdpSender sender_;
  RUdpReceiver receiver_;

  RUdpHost(IRUdpHostDelegate* delegate, FrameNumber first_frame_number);

  void TakeReceivedFrame(RUdpFrame& frame);
  StreamQueue* GetReceivedStream();
  void Send(const uint8* stream, int32 length);
  void Tick(float elapsed_time);
  void GetStats(RUdpHostStats& out_stats);
  FrameNumber YieldFrameNumberForSendingLongFrameReliably();
  FrameNumber GetRecvExpectFrameNumber();
};

}  // namespace net
}  // namespace fun
