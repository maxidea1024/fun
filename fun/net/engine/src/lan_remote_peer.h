#pragma once

#include "TcpTransport_S.h"
#include "P2PGroup_C.h" // P2PGroups_C

namespace fun {
namespace net {

class LanClientImpl;
class LanRemotePeer_C;

typedef Map<HostId, LanRemotePeer_C*> LanRemotePeers_C;

/**
 * Peer의 Accepted 된 정보
 */
class AcceptedInfo
  : public IInternalSocketDelegate,
    public ICompletionContext,
    public UseCount {
  CCriticalSection2 mutex_;

 public:
  LanClientImpl* owner_;
  double accepted_time_; // 억셉이 완료된 시간
  SharedPtr<InternalSocket> socket_;
  StreamQueue recv_stream_; //받는 스트림.
  bool is_timedout_;//타임아웃 여부.
  bool recv_issued_;

  AcceptedInfo(LanClientImpl* owner);

  LeanType GetLeanType() const override { return LeanType::AcceptedInfo; }

  void OnSocketWarning(InternalSocket* socket, const String& msg) {}

  void ExtractMessagesFromTcpStream(ReceivedMessageList& out_result);
  void IssueRecvAndCheck();

  CCriticalSection2& GetMutex() { return mutex_; }

  void AssertIsLockedByCurrentThread() {
    GetMutex().AssertIsLockedByCurrentThread();
  }

  void AssertIsNotLockedByCurrentThread() {
    GetMutex().AssertIsNotLockedByCurrentThread();
  }

  virtual bool IsLockedByCurrentThread() {
    return GetMutex().IsLockedByCurrentThread();
  }
};


class LanRemotePeer_C
  : public ISendDest_C,
    public IP2PGroupMember,
    public ITaskSubject,
    public ICompletionContext,
    public ListNode<LanRemotePeer_C>,
    public ITcpTransportOwner_S,
    public UseCount,
    public IHostObject {
 private:
  const char* dispose_caller_;

 public:
  const char* GetDisposeCaller() const { return dispose_caller_; }

  TcpTransport_S tcp_transport_;

  // 검증을 위함.
  Uuid holepunch_tag_;

  double recent_ping_;

  /** 측정된 송신 대기량(tcp) */
  //int32 send_queued_amount_in_byte_;

  /** 이 peer와 server와의 랙 */
  double peer_to_server_ping_;

  FUN_ALIGNED_VOLATILE double last_ping_send_time_;

  /** 이 peer에게 local에서 가진 서버 시간차를 보내는 쿨타임 */
  FUN_ALIGNED_VOLATILE double sync_indirect_server_time_diff_cooltime_;

  // 이 값 대신 GetIndirectServerTimeDiff()를 써서 얻어라!!
  double indirect_server_time_diff_;

  // remote peer와의 랙
  double last_ping_;

  SessionKey p2p_session_key_;
  CryptoCountType encrypt_count;
  CryptoCountType decrypt_count_;

  HostId host_id_;

  double created_time_;

  //서버에게 받은 listen_addr
  InetAddress listen_addr_from_server_;

  P2PGroups_C joined_p2p_groups_;

  bool task_running_;
  LanClientImpl* owner_;

  FinalUserWorkItemQueue_S final_user_work_queue_;

  double last_report_p2p_peer_ping_cooltime_;

  double last_tcp_stream_recv_time_;

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

    inline DisposeWaiter()
      : reason(ResultCode::Ok),
        detail(ResultCode::Ok),
        socket_error(SocketErrorCode::Ok),
        created_time(0) {}
  };
  // 평소에는 null이지만 일단 세팅되면 모든 스레드에서 종료할 때까지 기다린다.
  SharedPtr<DisposeWaiter> dispose_waiter_;

  //dispose를 서버에 요청.
  bool dispose_requested_;

  LanRemotePeer_C(LanClientImpl* owner);
  virtual ~LanRemotePeer_C();

  //void GetPeerInfo(PeerInfo& out_result);

  // 이 remote peer에 직접 p2p 연결 시도를 하는 과정을 시작한다.
  void CreateP2PHolepunchAttemptContext();

  void Heartbeat(double absolute_time);
  double GetIndirectServerTimeDiff();

  //virtual double GetAbsoluteTime();

  ResultCode ExtractMessagesFromTcpStream(ReceivedMessageList& out_result);

  // ITaskSubject interface
  HostId GetHostId() const override { return host_id_; }
  bool IsFinalReceiveQueueEmpty() override;
  bool IsTaskRunning() override;
  void OnSetTaskRunningFlag(bool running) override;
  bool PopFirstUserWorkItem(FinalUserWorkItem& output) override;

  void GetPeerInfo(PeerInfo& out_result);
  NetPeerInfoPtr GetPeerInfo();

  void EnqueueLocalEvent(LocalEvent& event);
  void EnqueueUserTask(Function<void()> func);

  LeanType GetLeanType() const override { return LeanType::LanRemotePeer_C; }

  // 상속 함수들
  int32 GetOverSendSuspectingThresholdInByte();

  void WarnTooShortDisposal(const char* where);
  void LockMain_AssertIsLockedByCurrentThread();
  void LockMain_AssertIsNotLockedByCurrentThread();

  void EnqueueIssueSendReadyRemotes();
  bool IsDispose();
  double GetAbsoluteTime();
  void IssueDispose(ResultCode result_code,
                    ResultCode detail_code,
                    const ByteArray& comment,
                    const char* where,
                    SocketErrorCode socket_error);
  bool IsValidEnd();

  bool IsLockedByCurrentThread();

  SocketErrorCode IssueSend(double absolute_time);
  CCriticalSection2& GetSendMutex();
  CCriticalSection2& GetMutex();
  void Decrease();
  void OnIssueSendFail(const char* where, SocketErrorCode socket_error);
  void SendWhenReady( HostId sender_id,
                      const InetAddress& sender_addr,
                      HostId dest_id,
                      const SendFragRefs& data,
                      const UdpSendOption& option);
};

} // namespace net
} // namespace fun
