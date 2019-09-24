#pragma once

#include "fun/net/net.h"

namespace fun {
namespace net {

class ISendSpeedMeasurerDelegate {
 public:
  virtual ~ISendSpeedMeasurerDelegate() {}

  virtual void Log(const char* text) {}

  virtual void OnMeasureComplete(double speed) = 0;
};

class SendSpeedMeasurer : public IInternalSocketDelegate,
                          public ICompletionPortCallbacks,
                          public ICompletionContext,
                          public Runnable {
 private:
  // send for UDP, recv for ICMP
  InternalSocketPtr send_socket_;
  InternalSocketPtr recv_socket_;

  double start_time_;
  bool measuring_;
  int32 measure_heartbeat_count_;
  int32 send_complete_length1_;
  double last_measured_send_speed1_;

  bool force_measure_now_;

  enum {
    PAKCET_LENGTH = 600  // MTU보다는 작아야 하지만 너무 작아서 패킷 헤더 낭비도
                         // 줄여야 한다.
  };

  static uint8 g_send_packet[PAKCET_LENGTH];
  static InetAddress g_sendto;

  uint8 icmp_recv_buffer_[PAKCET_LENGTH];

  FUN_ALIGNED_VOLATILE bool send_issued_;
  FUN_ALIGNED_VOLATILE bool recv_issued_;

 public:
  void IssueSendTo();
  void ConditionalIssueRecvFrom();

  virtual void OnSocketWarning(InternalSocket* socket, const String& msg);

  void ProcessIoCompletion(CompletionStatus& completion);

  void Heartbeat();

  SharedPtr<CompletionPort> completion_port_;

  virtual void OnCompletionPortWarning(CompletionPort* port, const char* msg) {}

  CCriticalSection2 mutex_;

  // 클라 스레드를 공유할 경우 frame move 처리 코스트가 클 경우 부정확한
  // 결과가 나온다. 따라서 송신량 측정기는 별도의 스레드를 가진다.
  // 단, NetClientManager가 이것을 하나 가지고 있어서 여러 클라들이
  // 공유해서 쓰도록 한다.
  UniquePtr<RunnableThread> thread_;
  FUN_ALIGNED_VOLATILE bool should_stop_;

  Clock clock_;

  ISendSpeedMeasurerDelegate* delegate_;

  double GetAbsoluteTime() { return clock_.AbsoluteSeconds(); }

  static void StaticThreadProc(void* context);
  void ThreadProc();

  double CalcMeasuredSendSpeed();

  // Runnable interface
  void Run() override;

 public:
  double GetLastMeasuredSendSpeed() { return last_measured_send_speed1_; }

  void RequestProcess();
  void RequestStopProcess();

  SendSpeedMeasurer(ISendSpeedMeasurerDelegate* delegate);
  ~SendSpeedMeasurer();

  // 사용자는 종료시 이걸 콜 해야 함
  void IssueTermination();

  // 사용자는 종료시 이걸 콜 해야 함
  void WaitUntilNoIssueGuaranteed();
};

typedef SharedPtr<SendSpeedMeasurer> SendSpeedMeasurerPtr;

}  // namespace net
}  // namespace fun
