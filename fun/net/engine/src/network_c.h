#pragma once

#include "SendSpeedMesasurer.h"
#include "UPnP.h"

namespace fun {
namespace net;

class Tracer;
class NetClientImpl;
class overlapped_result;
// class ReceivedMessageList;
class ReceivedMessage;
class MessageIn;
class UdpSocket_C;

/**
 * NetClient의 실제 처리를 담당하는 객체.
 */
class NetClientWorker : public ListNode<NetClientWorker> {
 private:
  void Heartbeat_ConnectFailCase(SocketErrorCode socket_error);
  bool Main_IssueConnect(SocketErrorCode& out_socket_error);
  void IssueTcpFirstRecv();

 public:
  void Heartbeat();

  FUN_ALIGNED_VOLATILE int32 disconnecting_mode_heartbeat_count_;
  FUN_ALIGNED_VOLATILE double disconnecting_mode_start_time_;
  FUN_ALIGNED_VOLATILE bool disconnecting_mode_warned_;

 private:
  void Heartbeat_Disconnecting();
  void Heartbeat_Connected();
  void Heartbeat_Connected_AfterLock();

  void Heartbeat_DetectNatDeviceName();
  void Heartbeat_JustConnected();
  void Heartbeat_Connecting();
  void Heartbeat_IssueConnect();

  double connect_issued_time_;

  void WarnTooLongElapsedTime();

 public:
  // 서버와의 연결 해제를 시작한 시간
  // >0 이면 서버 연결 해제를 신청한 상태. 이때에는 RZ RPC를 제외하고 송신 금지.
  double shutdown_issued_time_;

  // 서버와의 연결 해제 처리를 할 수 있는 최대 시간.
  // 이 시간이 넘으면 서버에서는 연결 해제를 즉시 인식하지 못하지만 클라이언트는
  // 어쨌거나 나가게 된다.
  double graceful_disconnect_timeout_;

  // warning: 순서가 변경되면 안됨.
  enum class State {
    IssueConnect = 0,
    Connecting = 1,
    JustConnected = 2,
    Connected = 3,
    Disconnecting = 4,
    Disconnected = 5,
  };

 private:
  FUN_ALIGNED_VOLATILE State state_;

 public:
  NetClientImpl* owner_;

  NetClientWorker(NetClientImpl* owner);
  ~NetClientWorker();

  void SetState(State state);
  State GetState() const { return state_; }

  void Heartbeat_ConnectedCase();
  void Heartbeat_EveryRemotePeer();

  bool ProcessMessage_S2CRoutedMulticast1(UdpSocket_C* udp_socket,
                                          ReceivedMessage& received_msg);
  bool ProcessMessage_S2CRoutedMulticast2(UdpSocket_C* udp_socket,
                                          ReceivedMessage& received_msg);
  bool ProcessMessage_EngineLayer(UdpSocket_C* udp_socket,
                                  ReceivedMessage& received_msg);

  void ProcessMessage_RPC(ReceivedMessage& received_msg,
                          bool& ref_msg_processed);
  void ProcessMessage_FreeformMessage(ReceivedMessage& received_msg,
                                      bool& ref_msg_processed);

  bool IsFromRemoteClientPeer(ReceivedMessage& received_msg) const;
  void ProcessMessage_PeerUdp_PeerHolepunch(ReceivedMessage& received_msg);
  void ProcessMessage_PeerHolepunchAck(ReceivedMessage& received_msg);
  void ProcessMessage_P2PIndirectServerTimeAndPing(
      ReceivedMessage& received_msg);
  void ProcessMessage_P2PIndirectServerTimeAndPong(
      ReceivedMessage& received_msg);
  void ProcessMessage_RUdp_Frame(UdpSocket_C* udp_socket,
                                 ReceivedMessage& received_msg);
  void ProcessMessage_NotifyClientServerUdpMatched(MessageIn& msg);
  void ProcessMessage_ReplyServerTime(MessageIn& msg);
  void ProcessMessage_ServerHolepunchAck(ReceivedMessage& received_msg);
  void ProcessMessage_PeerUdp_ServerHolepunchAck(ReceivedMessage& received_msg);
  void ProcessMessage_RequestStartServerHolepunch(MessageIn& msg);
  void ProcessMessage_NotifyServerConnectSuccess(MessageIn& msg);
  void ProcessMessage_ConnectServerTimedout(MessageIn& msg);
  void ProcessMessage_NotifyServerConnectionHint(MessageIn& msg);
  void ProcessMessage_NotifyCSSessionKeySuccess(MessageIn& msg);
  void ProcessMessage_NotifyProtocolVersionMismatch(MessageIn& msg);
  void ProcessMessage_NotifyServerDeniedConnection(MessageIn& msg);
  void ProcessMessage_ReliableRelay2(MessageIn& msg);
  void ProcessMessage_UnreliableRelay2(UdpSocket_C* udp_socket,
                                       ReceivedMessage& received_msg);
  void ProcessMessage_LingerDataFrame2(UdpSocket_C* udp_socket,
                                       ReceivedMessage& received_msg);
  void ProcessMessage_RequestReceiveSpeedAtReceiverSide_NoRelay(
      UdpSocket_C* udp_socket, ReceivedMessage& received_msg);
  void ProcessMessage_ReplyReceiveSpeedAtReceiverSide_NoRelay(
      ReceivedMessage& received_msg);

  void TcpRecvCompletionCase(CompletionStatus& completion);
  void TcpSendCompletionCase(CompletionStatus& completion);
  void UdpRecvCompletionCase(UdpSocket_C* udp_socket,
                             CompletionStatus& completion);
  void UdpSendCompletionCase(UdpSocket_C* udp_socket,
                             CompletionStatus& completion);

  bool LoopbackRecvCompletionCase();

  void ProcessEveryMessageOrMoveToFinalRecvQueue(UdpSocket_C* udp_socket);
  void ProcessMessageOrMoveToFinalRecvQueue(UdpSocket_C* udp_socket,
                                            ReceivedMessage& received_msg);
};

typedef SharedPtr<NetClientWorker> NetClientWorkerPtr;

/**
 * NetClient 인스턴스 관리자, Heartbeat 수행 스레드,
 * io completion 수행 스레드 풀, collected garbages
 */
class NetClientManager : public ICompletionPortCallbacks,
                         public ISendSpeedMeasurerDelegate,
                         public Runnable,
                         public Singleton<NetClientManager> {
 public:
  // garbage list 등을 보호함
  CCriticalSection2 mutex_;

 private:
  Clock clock_;

  double last_send_enqueue_completion_time_;

  void Heartbeat_EveryNetClient();

  void CalculateAverageElapsedTime();
  void DoGarbageFree();
  void OnCompletionPortWarning(CompletionPort* port, const char* text) {
    // Console.WriteLine(text);
  }

  // void ProcessOldGarbages();
  void ProcessIoCompletion(uint32 timeout_msec);
  bool ProcessIoCompletion_UDP(CompletionStatus& completion);
  bool ProcessIoCompletion_TCP(CompletionStatus& completion);
  // TODO 아래와 같은 이름이 좀더 좋을듯...
  // void HandleIoCompletion(const Timespan& timeout);
  // bool HandleUdpIoCompletion(CompletionStatus& completion);
  // bool HandleTcpIoCompletion(CompletionStatus& completion);

  UniquePtr<RunnableThread> worker_thread_;
  FUN_ALIGNED_VOLATILE bool should_stop_thread_;

  /* 아래 시간 관련 변수들을 보호.
  코드 프로필링 결과 FunNet.NetClientManager.GetCachedAbsoluteTime가 상당한
  시간을 먹으므로 잠금 영역 세분화를 한다.
  */
  CCriticalSection2 time_status_mutex_;
  // 시간 관련 변수들
  FUN_ALIGNED_VOLATILE double absolute_time_;
  FUN_ALIGNED_VOLATILE double elapsed_time_;
  FUN_ALIGNED_VOLATILE double next_heartbeat_time_;
  FUN_ALIGNED_VOLATILE double recent_elapsed_time_;
  FUN_ALIGNED_VOLATILE bool timer_touched_;

  CCriticalSection2 timer_init_mutex_;

  // 넷클라 워커스레드의 기아화 감지용
  double last_heartbeat_time_;
  List<double> elapsed_time_queue_;

  // 클라에서 이슈가 남은 소켓들... 시간에 한번씩 검사하여 제거.
  List<IHasOverlappedIoPtr> overlapped_io_garbages_;
  double last_garbage_free_time_;

 public:
  double average_elapsed_time_;
  double first_average_calc_time_;

 private:
  // Runnable interface
  void Run() override;
  void CatchThreadUnexpectedExit(const char* where, const char* reason);

  Timer timer_;
  TimerTaskIdType heartbeat_signal_timer_id_;
  AtomicCounter heartbeat_pulse_;

 public:
  AtomicCounter disconnection_invoke_count_;

  bool thread_ended_;

  // SendSpeedMeasurerPtr send_speed_measurer_;

  /** 등록된 NetClient 인스턴스 레지스트리. */
  NetClientWorker::ListOwner instances_;

  /** UPnP 객체 (detection and port-mapping control) */
  SharedPtr<UPnP> upnp_;

 public:
  void Register(NetClientWorker* instance);
  void Unregister(NetClientWorker* instance);
  bool IsRegistered(NetClientWorker* instance);

 public:
  // completion_port_ class는 fake mode를 지원하므로 safe.
  UniquePtr<CompletionPort> completion_port_;

 public:
  NetClientManager();
  ~NetClientManager();

 public:
  // uint32 GetWorkerThreadId() const { return worker_thread_id; }
  // HANDLE GetWorkerThreadHandle() const { return worker_thread_->GetHandle();
  // }
  uint32 GetWorkerThreadId() const {
    return worker_thread_.IsValid() ? worker_thread_->GetTid() : 0;
  }

  double GetCachedAbsoluteTime();

  double GetCachedElapsedTime() const { return elapsed_time; }

  double GetCachedRecentAverageElapsedTime() const {
    return recent_elapsed_time_;
  }

  bool IsThisWorkerThread();

  // void SendReadyInstances_Add(NetClientImpl* inst);

  void RequestMeasureSendSpeed(bool enable);

  virtual void OnMeasureComplete(double speed);

  void HasOverlappedIoToGarbage(IHasOverlappedIoPtr overlapped_io);
};

}  // namespace fun
}  // namespace fun
