#pragma once

#include "P2PGroup_S.h"     // LanP2PGroupMemberBase_S
#include "P2PPair.h"        // LanP2PConnectionStatePtr
#include "TcpTransport_S.h" // ITcpTransportOwner_S

namespace fun {
namespace net {

class LanP2PConnectionState;

typedef Map<HostId, class LanClient_S*> LanClients_S;

class LanClient_S
  : public ISendDest_S,
    public ITaskSubject,
    public ICompletionContext,
    public ListNode<LanClient_S>,
    public LanP2PGroupMemberBase_S,
    public ITcpTransportOwner_S,
    public UseCount,
    public IHostObject {
 private:
  const char* dispose_caller_;

 public:
  inline const char* GetDisposeCaller() const { return dispose_caller_; }

  // 가장 마지막에 TCP 스트림을 받은 시간
  // 디스 여부를 감지하기 위함
  FUN_ALIGNED_VOLATILE double last_tcp_stream_recv_time_;

  double last_request_measure_send_speed_time_;
  double send_speed_;

  double max_direct_p2p_connect_count_;

  // 측정된 랙
  double last_ping_;
  double recent_ping;

  // 측정된 송신대기량(바이트단위, TCP+UDP)
  int32 send_queued_amount_in_byte;

  // 클라와 graceful 디스 과정에서만 채워진다.
  ByteArray shutdown_comment_;

  struct DisposeWaiter {
    // Reason for dispose.
    ResultCode reason;
    // Detail reason for dispose.
    ResultCode detail;
    // Data sent by the client when disconnecting.
    ByteArray comment;
    // The time Dispose was started.
    double created_time;
    // 문제 발생시 추적을 위함
    SocketErrorCode socket_error;

    DisposeWaiter()
      : reason(ResultCode::Ok),
        detail(ResultCode::Ok),
        created_time_(0),
        socket_error(SocketErrorCode::Ok) {}
  };
  // 평소에는 null이지만 일단 세팅되면 모든 스레드에서 종료할 때까지 기다린다.
  UniquePtr<DisposeWaiter> dispose_waiter_;

  bool task_running_;
  LanServerImpl* owner_;

  // 최종적으로 처리되어야할 유저 워크 큐
  FinalUserWorkItemQueue_S final_user_work_queue_;

  // Session-key.
  SessionKey session_key_;
  // Whether session-key is received or not.
  bool session_key_received_;
  // Encryption sequence counter.
  CryptoCountType encrypt_count;
  // Decruption sequence counter.
  CryptoCountType decrypt_count_;

  // Created time.
  double created_time_;
  // Whether purge requested or not.
  bool purge_requested_;

  typedef Set<LanP2PConnectionStatePtr> LanP2PConnectionPairs;
  LanP2PConnectionPairs lan_p2p_connection_pairs_;

  // Self host-id.
  HostId host_id_;
  // Self external address.
  InetAddress external_addr_;

  // 클라이언트와 통신할 TCP 소켓
  TcpTransport_S* to_client_tcp_;

  // 이 클라가 디스될 때 그룹들이 파괴되는데, 이를 클라 디스 이벤트 콜백에서
  // 전달되게 하기 위해 여기에 백업.
  Array<HostId> had_joined_p2p_groups_;

  // 자진탈퇴 요청을 건 시간
  double request_auto_prune_start_time_;

  LanClient_S(LanServerImpl* owner_,
              InternalSocket* new_socket,
              const InetAddress& tcp_remote_addr);
  ~LanClient_S();

  bool IsAuthed() const { return host_id_ != HostId_None; }

  ResultCode ExtractMessagesFromTcpStream(ReceivedMessageList& out_result);

  void GetClientInfo(NetClientInfo& out_info);
  NetClientInfoPtr GetClientInfo();

  // ITaskSubject interface
  HostId GetHostId() const override { return host_id_; }
  bool IsFinalReceiveQueueEmpty() override;
  bool IsTaskRunning() override;
  bool PopFirstUserWorkItem(FinalUserWorkItem& output) override;
  void OnSetTaskRunningFlag(bool running) override;

  void DetectSpeedHack();

  void EnqueueLocalEvent(LocalEvent& event);
  void EnqueueUserTask(Function<void()> func);

  void AssertIsLockedByCurrentThread() {
    fun_check(IsLockedByCurrentThread() == true);
  }

  void AssertIsNotLockedByCurrentThread() {
    fun_check(IsLockedByCurrentThread() == false);
  }

  LeanType GetLeanType() const override {
    return LeanType::LanClient_S;
  }

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
