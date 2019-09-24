#pragma once

#include "P2PGroup_S.h"         // P2PGroupMemberBase_S
#include "P2PPair.h"          // P2PConnectionStatePtr
#include "FallbackableUdpTransport_S.h" // FallbackableUdpTransport_S
#include "UdpSocket_S.h"        // UdpSocketPtr_S
#include "TcpTransport_S.h"

namespace fun {
namespace net {

class P2PConnectionState;

typedef Map<HostId, class RemoteClient_S*> RemoteClients_S;

class SpeedHackDetector {
 public:
  // 스피드핵이 검출되었는지 여부
  bool hack_suspected_;
  // 마지막으로 핑을 받은 시각
  double last_ping_recv_time_;
  // 첫번째로 핑을 받은 시각
  double first_ping_recv_time_;
  // 최근 핑 주기
  double recent_ping_interval_;

 public:
  SpeedHackDetector()
    : recent_ping_interval_(NetConfig::speedhack_detector_ping_interval_sec), // 정상 수치
      first_ping_recv_time_(0),
      last_ping_recv_time_(0),
      hack_suspected_(false) {
  }
};


class RemoteClient_S
  : public ISendDest_S,
    public ITaskSubject,
    public ICompletionContext,
    public ListNode<RemoteClient_S>,
    public P2PGroupMemberBase_S,
    public ITcpTransportOwner_S,
    public UseCount,
    public IHostObject {
 private:
  const char* dispose_caller_;

 public:
  inline const char* GetDisposeCaller() { return dispose_caller_; }

 public:
  RemoteClient_S( NetServerImpl* owner,
                  InternalSocket* new_socket,
                  const InetAddress& tcp_remote_addr);
  ~RemoteClient_S();

  bool IsAuthed() const { return host_id_ != HostId_None; }

  ResultCode ExtractMessagesFromTcpStream(ReceivedMessageList& out_msg_list);

  void ExtractMessagesFromUdpRecvQueue( const uint8* udp_packet,
                                        int32 udp_packet_length,
                                        const InetAddress& udp_addr_from_here,
                                        int32 message_max_length,
                                        ReceivedMessageList& out_msg_list);

  void GetClientInfo(NetClientInfo& out_info);
  NetClientInfoPtr GetClientInfo();

  bool IsBehindNAT();

  // ITaskSubject interface
  HostId GetHostId() const override { return host_id_; }
  bool IsFinalReceiveQueueEmpty() override;
  bool IsTaskRunning() override;
  bool PopFirstUserWorkItem(FinalUserWorkItem& out_item) override;
  void OnSetTaskRunningFlag(bool running) override;

  void DetectSpeedHack();

  void EnqueueLocalEvent(LocalEvent& event);
  void EnqueueUserTask(Function<void()> func);

  double GetP2PGroupTotalRecentPing(HostId group_id);

  // 이하 상속 함수.
  LeanType GetLeanType() const override { return LeanType::RemoteClient_S; }

  void WarnTooShortDisposal(const char* where) override;
  void LockMain_AssertIsLockedByCurrentThread() override;
  void LockMain_AssertIsNotLockedByCurrentThread() override;

  void EnqueueIssueSendReadyRemotes();
  bool IsDispose();
  double GetAbsoluteTime();
  void IssueDispose(ResultCode result_code,
                    ResultCode detail_code,
                    const ByteArray& comment,
                    const char* where,
                    SocketErrorCode socket_error);
  bool IsValidEnd();

  CCriticalSection2& GetSendMutex();
  CCriticalSection2& GetMutex();

  SocketErrorCode IssueSend(double absolute_time);
  void Decrease();
  void OnIssueSendFail(const char* where, SocketErrorCode socket_error);
  void SendWhenReady( HostId sender_id,
                      const InetAddress& sender_addr,
                      HostId dest_id,
                      const SendFragRefs& data_to_send,
                      const UdpSendOption& send_opt);

 public:
  void AssertIsLockedByCurrentThread() {
    fun_check(IsLockedByCurrentThread() == true);
  }

  void AssertIsNotLockedByCurrentThread() {
    fun_check(IsLockedByCurrentThread() == false);
  }

  bool IsLockedByCurrentThread();

  //void ResetSpeedHackDetector();

 public:
  // 가장 마지막에 TCP 스트림을 받은 시간 (디스 여부를 감지하기 위함)
  FUN_ALIGNED_VOLATILE double last_tcp_stream_recv_time_;

  double last_request_measure_send_speed_time_;
  double send_speed;

  // 가장 마지막에 클라로부터 UDP 패킷을 받은 시간
  // real UDP enabled mode에서만 유효한 값이다.
  double last_udp_packet_recv_time_;

  double arbitrary_udp_touched_time_;

  double max_direct_p2p_connection_count_;

  // 수퍼 피어로서 이 클라가 얼마나 역할을 잘 해낼까.
  double super_peer_rating_;

  // 가장 마지막에 클라로부터 ping을 받은 시간
  // real UDP enabled mode에서만 유효한 값이다.
  // 클라로부터 패킷은 받지만 클라가 보내는 양이 상당히 많아서
  // 서버에서 퐁을 보내지 못해서 펄백하면 안습이다.
  // 따라서 이러한 경우를 위해 이게 쓰인다.
  double last_udp_ping_recv_time_;

  // 측정된 랙
  double last_ping_;
  double recent_ping;

  // C2C 클라끼리 보내고 받은 real udp packet count
  int32 to_remote_peer_send_udp_message_attempt_count_;
  int32 to_remote_peer_send_udp_message_success_count_;

  // C2S 서버에게 보내고 받은 real udp packet count
  int32 to_server_send_udp_message_success_count_;
  int32 to_server_send_udp_message_attempt_count_;

  // 사용자가 입력한 마지막 프레임레이트
  ApplicationHint last_application_hint_;

  // 측정된 송신대기량(바이트단위,TCP+UDP)
  // TCP, UDP를 각각 카운팅 하는건 어떨런지???
  int32 send_queued_amount_in_byte_;

  // 공유기 이름
  String nat_device_name_;

  // 클라와 graceful 디스 과정에서만 채워진다.
  ByteArray shutdown_comment_;

  // 이 리모트 클라에 대해 sendqueue가 일정이상을 찍었을때에 시작되는 타임이다.
  double send_queue_warning_start_time_;

  struct DisposeWaiter {
    // dispose를 하는 이유
    ResultCode reason;
    // dispose를 할때 detail한 사유
    ResultCode detail;
    // 클라에서 접속을 해제한 경우 마지막으로 날린 데이터를 의미한다.
    ByteArray comment;
    // 이 객체가 생성된 시간
    double created_time;
    // 문제 발생시 추적을 위함
    SocketErrorCode socket_error;

    DisposeWaiter(),
      : reason(ResultCode::Ok),
        detail(ResultCode::Ok),
        socket_error(SocketErrorCode::Ok),
        created_time(0) {}
  };
  // 평소에는 null이지만 일단 세팅되면 모든 스레드에서 종료할 때까지 기다린다.
  SharedPtr<DisposeWaiter> dispose_waiter_;

  bool task_running_;
  NetServerImpl* owner_;

  // 이 클라가 디스될 때 그룹들이 파괴되는데,
  // 이를 클라 디스 이벤트 콜백에서 전달되게 하기 위해 여기에 백업.
  Array<HostId> had_joined_p2p_groups_;

  FinalUserWorkItemQueue_S final_user_work_queue_;

  SessionKey session_key_;
  bool session_key_received_;
  CryptoCountType encrypt_count_;
  CryptoCountType decrypt_count_;

  double created_time_;
  bool purge_requested_;

  typedef Set<P2PConnectionStatePtr> P2PConnectionPairs;
  P2PConnectionPairs p2p_connection_pairs_;

  HostId host_id_;

  // 서버 소켓 중 랜덤으로 하나 선택된, 연계된 UDP 소켓
  FallbackableUdpTransport_S to_client_udp_fallbackable_;

  // 클라이언트와 통신할 TCP 소켓
  TcpTransport_S* to_client_tcp_;

  SharedPtr<SpeedHackDetector> speed_hack_detector_;

  // 자진탈퇴 요청을 건 시간
  double request_auto_prune_start_time_;

  // per-peer RC인 경우에만 설정된다.
  UdpSocketPtr_S owned_udp_socket_;
  int32 borrowed_port_number_;
};

} // namespace net
} // namespace fun
