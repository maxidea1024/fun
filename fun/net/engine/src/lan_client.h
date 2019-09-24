#pragma once

#include "ServerSocketPool.h"

#include "GeneratedRPCs/lan_LanC2C_proxy.h"
#include "GeneratedRPCs/lan_LanC2C_stub.h"
#include "GeneratedRPCs/lan_LanC2S_proxy.h"
#include "GeneratedRPCs/lan_LanC2S_stub.h"
#include "GeneratedRPCs/lan_LanS2C_proxy.h"
#include "GeneratedRPCs/lan_LanS2C_stub.h"

#include "LanListener_C.h"
#include "LanRemotePeer.h"
#include "TcpTransport_C.h"
#include "Tracer.h"
#include "thread_pool_impl.h"

namespace fun {
namespace net {

class LanRemotePeer_C;
class MessageSummary;

class LanClientImpl : public NetCoreImpl,
                      public LanClient,
                      public ISocketIoCompletionDelegate,
                      // public IInternalSocketDelegate,
                      public ICompletionPortCallbacks,
                      public ICompletionContext,
                      public ITaskSubject,
                      public ISendDest_C,
                      public IP2PGroupMember,
                      public IUserTaskQueueOwner,
                      public ITcpTransportOwner_C,
                      public ICompletionKey,
                      public IThreadReferer,
                      public IThreadPoolCallbacks {
 private:
  IntervalAlaram reliable_ping_alarm_;
  CCriticalSection2 mutex_;
  CCriticalSection2 connect_disconnect_phase_mutex_;
  AtomicCounter disconnection_invoke_count_;
  AtomicCounter connect_count_;

  // RequestServerTime을 마지막으로 보낸 시간
  double last_request_server_time_time_;

 public:
  // ICompletionContext interface
  LeanType GetLeanType() const override { return LeanType::LanClient_S; }

 public:
  // RequestServerTime을 호출한 횟수
  int32 request_server_time_count_;

  UniquePtr<LanListener_C> listener_thread_;

  UniquePtr<thread_pool_impl> net_thread_pool_;
  bool net_thread_external_use_;
  FUN_ALIGNED_VOLATILE bool net_thread_pool_unregisted_;

  UniquePtr<thread_pool_impl> UserThreadPool;
  bool user_thread_external_use_;
  FUN_ALIGNED_VOLATILE bool user_thread_pool_unregisted_;

  Singleton<ServerSocketPool>::CPtr ServerSocketPool;

  InternalSocketPtr tcp_listening_socket_;
  UniquePtr<CompletionPort> completion_port_;
  UniquePtr<CompletionPort> tcp_accept_cp_;

  // 로컬 NIC가 여럿인 경우 지정된 것(혹은 미지정)
  String local_nic_addr_;

  uint32 internal_version_;

 public:
  UniquePtr<LogWriter> intra_logger_;
  Clock clock_;

  // 유저 콜백으로 쓰이는 최종 수신된 메시지 또는 로컬 이벤트 등

  FinalUserWorkItemQueue_S final_user_work_queue_;

  // 가장 마지막에 TCP 스트림을 받은 시간
  // 디스 여부를 감지하기 위함
  // 양쪽 호스트가 모두 감지해야 한다.
  // TCP는 shutdown 직후 closesocket을 호출해도 상대가 못 받을 가능성이 있기
  // 때문이다.
  FUN_ALIGNED_VOLATILE double last_tcp_stream_recv_time_;

  FUN_ALIGNED_VOLATILE double stepped_elapsed_time_;
  FUN_ALIGNED_VOLATILE double stepped_absolute_time_;

  void UpdateFixedSteppedTimeVars();

  FUN_ALIGNED_VOLATILE uint64 total_tcp_recv_bytes_;
  FUN_ALIGNED_VOLATILE uint64 total_tcp_send_bytes_;

  SessionKey to_server_session_key_;
  CryptoCountType to_server_encrypt_count_;
  CryptoCountType to_server_decrypt_count_;

  HostId local_host_id_;

  LanConnectionArgs connection_params_;

  // ToServerTcp의 송수신 버퍼용 lock을 따로 둔다.
  CCriticalSection2 to_server_tcp_mutex_;
  SharedPtr<TcpTransport_C> to_server_tcp_;

  // mutex_ locked인 경우 local event를 enque하기만 해야 한다. mutex_ unlock인
  // 경우에는 콜백 허용
  ILanClientCallbacks* callbacks_;

  NetSettings settings_;

  SessionKey self_p2p_session_key_;

  // LanRemotePeers_C remote_peers_;
  Set<LanRemotePeer_C*>
      remote_peer_instances_;  // 실제로 RP들의 소유권을 가지는 곳

  // TCP P2P 연결이 완료되기 전에는 Remotepeer 객체는 여기에 잔존한다.
  LanRemotePeers_C candidate_remote_peers_;

  // 연결이 완료되었으며 dispose issue가 발생하기 전의 RP들
  LanRemotePeers_C authed_remote_peers_;

  typedef Map<HostId, LanRemotePeer_C*> DisposeIssuedRemotePeerMap;
  DisposeIssuedRemotePeerMap dispose_issued_remote_peers_map_;

  List<LanRemotePeer_C*> remote_peer_garbages_;

  typedef Map<InetAddress, AcceptedInfo*> AcceptedPeers;
  AcceptedPeers
      accepted_peers_;  // 자신에게 연결된 peer의 정보 인증이 완료되면 삭제된다.

  bool user_task_is_running_;

  // bool intra_logging_on_;

  // 서버 인스턴스의 GUID
  // 서버 연결 성공시 발급받는다.
  Uuid server_instance_tag_;

 private:
  CryptoCountType self_encrypt_count_;
  CryptoCountType self_decrypt_count_;

 public:
  struct C2CProxy : public LanC2C::Proxy {};
  struct C2SProxy : public LanC2S::Proxy {};

  struct S2CStub : public LanS2C::Stub {
    LanClientImpl* owner_;

    DECLARE_RPCSTUB_LanS2C_P2PGroup_MemberJoin;
    DECLARE_RPCSTUB_LanS2C_P2PGroup_MemberJoin_Unencrypted;
    DECLARE_RPCSTUB_LanS2C_P2PGroup_MemberLeave;
    DECLARE_RPCSTUB_LanS2C_ReliablePong;
    DECLARE_RPCSTUB_LanS2C_P2PConnectStart;
    DECLARE_RPCSTUB_LanS2C_GroupP2PConnectionComplete;
    DECLARE_RPCSTUB_LanS2C_ShutdownTcpAck;
    DECLARE_RPCSTUB_LanS2C_RequestAutoPrune;
  };

  struct C2CStub : public LanC2C::Stub {
    LanClientImpl* owner_;
  };

  C2CProxy c2c_proxy_;
  C2CStub c2c_stub_;

  C2SProxy c2s_proxy_;
  S2CStub s2c_stub_;

 public:
  // PreFinalRecvQueue용 lock을 따로둔다.
  // (mainlock을 사용하는것보다 contention을 줄이기위함.)
  CCriticalSection2 pre_final_recv_queue_mutex_;
  List<ReceivedMessage>
      pre_final_recv_queue_;  // requires mutex_ lock before access

 public:
  // double min_extra_ping_;
  // double extra_ping_variance_;

  RandomMT random_;

  // 서버 컴퓨터의 시간.
  double dx_server_time_diff_;

  // 서버 컴퓨터에 대한 레이턴시. TCP로 핑퐁한 결과가 들어간다.
  // round trip의 반이다.
  double server_tcp_recent_ping_;
  double server_tcp_last_ping_;

  // 종료시에만 설정됨

  // Stop()이 return하기 전까지는 이벤트 콜백이 있어야하므로 삭제한다.
  // FUN_ALIGNED_VOLATILE bool tear_down_;

  bool suppress_subsequenct_disconnection_events_;
  typedef Array<ISendDest_C*,
                InlineAllocator<NetConfig::OrdinaryHeavyS2CMulticastCount>>
      ILanSendDestList_C;

 private:
  P2PGroups_C p2p_groups_;

 public:
  class ServerAsSendDest : public ISendDest_C, public IP2PGroupMember {
   public:
    LanClientImpl* owner_;
    void* host_tag_;

    inline ServerAsSendDest(LanClientImpl* owner)
        : owner_(owner), host_tag_(nullptr) {}

    HostId GetHostId() const override { return HostId_Server; }

    double GetIndirectServerTimeDiff() override {
      return owner_->GetIndirectServerTimeDiff();
    }
  };
  ServerAsSendDest server_as_send_dest_;

  virtual bool SetHostTag(HostId host_id, void* host_tag);

  // tcp 송신큐를 보호할 CriticalSection을 따로 둔다.
  CCriticalSection2 tcp_issue_queue_mutex_;

  // tcp 송신이슈가 걸린 peer리스트
  ListNode<LanRemotePeer_C>::ListOwner tcp_issue_send_ready_remote_peers_;

  // 자기자신에게 송신한 메시지. 아직 user worker로 넘어간건 아니다.
  List<MessageIn> lookback_final_recv_message_queue_;

 public:
  void OnThreadBegin() override;
  void OnThreadEnd() override;

  // UserTask
  UserTaskQueue user_task_queue_;
  void DoUserTask();
  void PostUserTask();
  void UserWork_FinalReceiveRPC(FinalUserWorkItem& uwi, void* host_tag);
  void UserWork_FinalReceiveFreeform(FinalUserWorkItem& uwi, void* host_tag);
  void UserWork_FinalUserTask(FinalUserWorkItem& uwi, void* host_tag);
  void UserWork_LocalEvent(FinalUserWorkItem& uwi);

  void EndCompletion();

  // Heartbeat

  Timer timer_;
  TimerTaskIdType heartbeat_timer_id_;
  TimerTaskIdType issue_send_on_need_timer_id_;
  TimerTaskIdType tick_timer_id_;

  IntervalAlaram purge_too_old_unmature_peer_alarm_;
  IntervalAlaram remove_tool_old_tcp_send_packet_queue_alarm_;
  IntervalAlaram dispose_issued_remote_peers_alarm_;
  IntervalAlaram disconnect_remote_peer_on_timeout_alarm_;
  IntervalAlaram accepted_peer_dispost_alarm_;
  IntervalAlaram remove_lookahead_message_alarm_;

  void PostHeartbeatIssue();
  void Heartbeat();
  void Heartbeat_Connected();
  void Heartbeat_Disconnecting();

  bool LoopbackRecvCompletionCase();
  bool ProcessMessage_EngineLayer(ReceivedMessage& received_msg,
                                  ITaskSubject* subject);
  bool IsFromRemoteClientPeer(ReceivedMessage& received_msg);
  void ProcessMessage_NotifyServerConnectionHint(MessageIn& msg);
  void ProcessMessage_NotifyCSSessionKeySuccess(MessageIn& msg);
  void ProcessMessage_NotifyProtocolVersionMismatch(MessageIn& msg);
  void ProcessMessage_ReplyServerTime(MessageIn& msg);
  void ProcessMessage_NotifyServerDeniedConnection(MessageIn& msg);
  void ProcessMessage_NotifyServerConnectSuccess(MessageIn& msg);
  void ProcessMessage_NotifyConnectPeerRequestSuccess(HostId peer_id,
                                                      MessageIn& msg);
  void ProcessMessage_P2PIndirectServerTimeAndPing(
      ReceivedMessage& received_msg);
  void ProcessMessage_P2PIndirectServerTimeAndPong(
      ReceivedMessage& received_msg);
  void ProcessMessage_RPC(ReceivedMessage& received_msg, ITaskSubject* subject,
                          bool& ref_msg_processed);
  void ProcessMessage_FreeformMessage(ReceivedMessage& received_msg,
                                      ITaskSubject* subject,
                                      bool& ref_msg_processed);

  void PostEveryRemote_IssueSend();
  void EveryRemote_IssueSendOnNeed(Array<IHostObject*>& pool);

  void PostOnTick();
  void OnTick();

 private:
  ConnectionState state_;
  FUN_ALIGNED_VOLATILE int32 disconnecting_mode_heartbeat_count_;
  FUN_ALIGNED_VOLATILE double disconnecting_mode_start_time_;
  FUN_ALIGNED_VOLATILE bool disconnecting_mode_warned_;

  FUN_ALIGNED_VOLATILE double shutdown_issued_time_;
  FUN_ALIGNED_VOLATILE double graceful_disconnect_timeout_;

 public:
  void SetState(ConnectionState new_state);
  ConnectionState GetState() const { return state_; }

  void OnIoCompletion(Array<IHostObject*>& send_issued_pool,
                      ReceivedMessageList& msg_list,
                      CompletionStatus& completion) override;
  void ProcessEveryMessageOrMoveToFinalRecvQueue_ToServerTcp();
  bool ProcessMessage_EngineLayer(ReceivedMessage& received_msg,
                                  AcceptedInfo* accepted_info);
  void ProcessMessage_NotifyConnectionPeerRequestData(
      MessageIn& msg, AcceptedInfo* accepted_info);

  void IoCompletion_ToServerTcp(CompletionStatus& completion,
                                ReceivedMessageList& received_msg_list);
  void IoCompletion_PerRemotePeer(CompletionStatus& completion,
                                  ReceivedMessageList& received_msg_list);
  void IoCompletion_AcceptedInfo(CompletionStatus& completion,
                                 ReceivedMessageList& received_msg_list);

  void IoCompletion_TcpRecvCompletionCase(
      CompletionStatus& completion, ReceivedMessageList& received_msg_list,
      LanRemotePeer_C* lp);
  void IoCompletion_TcpSendCompletionCase(CompletionStatus& completion,
                                          LanRemotePeer_C* lp);
  void IoCompletion_TcpConnectExCompletionCase(CompletionStatus& completion,
                                               LanRemotePeer_C* lp);
  void IoCompletion_TcpPeerAcceptedCase(CompletionStatus& completion,
                                        AcceptedInfo* accepted_info);
  void IoCompletion_NewAcceptedPeerCase(AcceptedInfo* accepted_info);
  void IoCompletion_TcpRecvCompletionCase(
      CompletionStatus& completion, ReceivedMessageList& received_msg_list,
      AcceptedInfo* accepted_info);

  void IoCompletion_ProcessMessageOrMoveToFinalRecvQueue(
      LanRemotePeer_C* lp, ReceivedMessageList& extracted_msg_list);

  void CatchThreadUnexpectedExit(const char* where, const char* reason);
  void CatchThreadExceptionAndPurgeClient(LanRemotePeer_C* Peer,
                                          const char* where,
                                          const char* reason);
  void Heartbeat_ConnectFailCase(SocketErrorCode Code);

  ISocketIoCompletionDelegate* GetIoCompletionDelegate() override;

  void EnableIntraLogging(const char* log_filename);

 private:
  double GetReliablePingTimerInterval();
  bool CreateTcpListenSocketAndInit(SharedPtr<ResultInfo>& out_error);
  bool CreateTcpConnectSocketAndInit(const String& ip, int32 port,
                                     SharedPtr<ResultInfo>& out_error);
  // 새로운 member가 들어와서 p2p group을 update한다.
  void UpdateP2PGroup_MemberJoin(HostId group_id, HostId member_id,
                                 const ByteArray& custom_field, uint32 event_id,
                                 const ByteArray& p2p_aes_session_key,
                                 const ByteArray& p2p_rc4_session_key,
                                 const Uuid& connection_tag);
  void ConditionalAssignRemotePeerInfo(LanRemotePeer_C* lp);
  void ProcessOnPeerDisposeCanSafe(LanRemotePeer_C* lp);

 private:
  // LookaheadP2PSend Message를 모아놓을 큐

  // typedef Deque<ByteArray> ByteArrayQueue;
  typedef List<ByteArray> ByteArrayQueue;
  typedef Map<HostId, ByteArrayQueue> LookaheadP2PSendQueueMap;
  LookaheadP2PSendQueueMap lookahead_p2p_send_queue_map_;

  /** 지금 당장 보내지 못하는 메세지들을 큐에 쌓는다. */
  void AddLookaheadP2PSendQueueMap(HostId peer_id,
                                   const ByteArray& data_to_send);

  /** 해당 peer에게 보낼 메세지를 삭제한다. */
  void RemoveLookaheadP2PSendQueueMap(LanRemotePeer_C* lp);

 public:
  /** 모든 LanRemotePeer_C의 메세지를 삭제한다. */
  void RemoveAllLookaheadP2PSendQueueMap();

  /** 쌓여있는 메세지를 해당 peer에게 보낸다. */
  void SendLookaheadP2PMessage(LanRemotePeer_C* lp);

 public:
  LanClientImpl();
  virtual ~LanClientImpl();

  bool Send_BroadcastLayer(const SendFragRefs& payload,
                           const SendOption& send_opt,
                           const HostId* sendto_list, int32 sendto_count);

  void Send_ToServer_Directly_Copy(MessageReliability Reliability,
                                   const SendFragRefs& data_to_send);

  void ConvertGroupToIndividualsAndUnion(int32 sendto_count,
                                         const HostId* sendto_list,
                                         HostIdArray& output);
  void ConvertGroupToIndividualsAndUnion(int32 sendto_count,
                                         const HostId* sendto_list,
                                         ILanSendDestList_C& SendDestList);
  bool ConvertAndAppendP2PGroupToPeerList(HostId sendto,
                                          ILanSendDestList_C& SendTo2);
  P2PGroupPtr_C GetP2PGroupByHostId_INTERNAL(HostId group_id);
  ISendDest_C* GetSendDestByHostId(HostId peer_id);
  LanRemotePeer_C* GetPeerByHostId_NOLOCK(HostId peer_id);

  INetCoreCallbacks* GetCallbacks_NOLOCK();

  bool NextEncryptCount(HostId remote_id, CryptoCountType& out_count);
  void PrevEncryptCount(HostId remote_id);
  bool GetExpectedDecryptCount(HostId remote_id, CryptoCountType& out_count);
  bool NextDecryptCount(HostId remote_id);

  SessionKey* GetCryptSessionKey(HostId remote_id, String& out_error);

  // Local events.
  void EnqueueError(SharedPtr<ResultInfo> result_info);
  void EnqueueWarning(SharedPtr<ResultInfo> result_info);
  void EnqueueDisconnectionEvent(ResultCode result_code,
                                 ResultCode detail_code);
  void EnqueueLocalEvent(LocalEvent& event);
  void EnqueueConnectFailEvent(ResultCode result_code,
                               SharedPtr<ResultInfo> result_info);
  void EnqueueConnectFailEvent(
      ResultCode result_code,
      SocketErrorCode socket_error = SocketErrorCode::Ok);
  void EnqueueHackSuspectEvent(LanRemotePeer_C* lp, const char* statement,
                               HackType hack_type);

  void OnSocketWarning(InternalSocket* socket, const String& msg);
  void OnCompletionPortWarning(CompletionPort* port, const char* msg);
  bool AsyncCallbackMayOccur();

  void EnqueueUserTask(Function<void()> func);

  virtual bool Connect(LanConnectionArgs& args,
                       SharedPtr<ResultInfo>& out_error);

  void Disconnect();
  void Disconnect(double graceful_disconnect_timeout, const ByteArray& comment);

  // void DisconnectNoWait();
  // void DisconnectNoWait(double graceful_disconnect_timeout_, const ByteArray&
  // comment);

  String DumpGroupStatus();
  void GetGroupMembers(HostId group_id, HostIdArray& output);
  void GetLocalJoinedP2PGroups(HostIdArray& output);
  void GetStats(LanClientStats& out_stats);
  double GetP2PServerTime(HostId group_id);
  InetAddress GetServerAddress();
  bool GetPeerInfo(HostId peer_id, PeerInfo& out_info);
  double GetServerTime();
  double GetServerTimeDiff();
  ConnectionState GetServerConnectionState();
  void SetCallbacks(ILanClientCallbacks* callbacks);
  double GetLastPingSec(HostId remote_id);
  double GetRecentPingSec(HostId remote_id);
  double GetSendToServerSpeed();
  uint32 GetInternalVersion();
  InetAddress GetPublicAddress();
  void GetNetWorkerThreadInfo(Array<ThreadInfo>& output);
  void GetUserWorkerThreadInfo(Array<ThreadInfo>& output);

  void AssociateSocket(InternalSocket* socket);

  void ProcessOneLocalEvent(LocalEvent& event);

  // 일정 시간이 됐으면 서버에 서버 시간을 요청한다.
  void ConditionalRequestServerTime();
  double GetIndirectServerTime(HostId peer_id);
  double GetIndirectServerTimeDiff() { return dx_server_time_diff_; }
  double GetElapsedTime();
  double GetAbsoluteTime();
  // double GetSimLag()
  //{
  //  return min_extra_ping + random_.NextDouble() * extra_ping_variance;
  //}

  ResultCode ExtractMessagesFromTcpStream(ReceivedMessageList& out_result);

  HostId GetLocalHostId();

  // ITaskSubject interface
  HostId GetHostId() const override;
  bool IsFinalReceiveQueueEmpty() override;
  bool IsTaskRunning() override;
  void OnSetTaskRunningFlag(bool running) override;
  bool PopFirstUserWorkItem(FinalUserWorkItem& out_item) override;

  void Cleanup();
  void CleanupPeerGet();
  void DisposeIssuedRemotePeers();
  void DisconnectRemotePeerOnTimeout();
  void RemoveRemotePeerIfNoGroupRelationDetected(LanRemotePeer_C* member);
  void IssueDisposeRemotePeer(LanRemotePeer_C* lp, ResultCode result_code,
                              ResultCode detail_code, const ByteArray& comment,
                              const char* where, SocketErrorCode socket_error,
                              bool remove_from_collection = true);
  void ShowError_NOLOCK(SharedPtr<ResultInfo> result_info);
  void RemotePeer_RemoveFromCollections(LanRemotePeer_C* lp);

 private:
  bool Send(const SendFragRefs& data_to_send, const SendOption& send_opt,
            const HostId* sendto_list, int32 sendto_count);

  // CNetCoreImpl에 이미 구현되어 있는데, 왜 이래 해야 abstract가 아니라고
  // 나오는걸까??
  void ShowNotImplementedRpcWarning(RpcId rpc_id, const char* rpc_name);
  // CNetCoreImpl에 이미 구현되어 있는데, 왜 이래 해야 abstract가 아니라고
  // 나오는걸까??
  void PostCheckReadMessage(IMessageIn& msg, RpcId rpc_id,
                            const char* rpc_name);

  ITaskSubject* GetTaskSubjectByHostId_NOLOCK(HostId subject_host_id);

  void EnableVizAgent(const char* server_ip, int32 server_port,
                      const String& login_key);

  void Viz_NotifySendByProxy(const HostId* sendto_list, int32 sendto_count,
                             const MessageSummary& summary,
                             const RpcCallOption& opt);
  void Viz_NotifyRecvToStub(HostId sender, RpcId rpc_id, const char* rpc_name,
                            const char* params_as_string);

  void Disconnect_INTERNAL(double graceful_disconnect_timeout,
                           const ByteArray& comment);

 public:
  bool IsValidHostId_NOLOCK(HostId host_id);
  void ConditionalIssueSend_ToServerTcp();
  void ConditionalSyncIndirectServerTime();
  void PurgeTooOldUnmaturePeer();
  void PurgeTooOldUnmatureAcceptedPeers();
  void Tcp_LongTick();
  void ConditionalReportLanP2PPeerPing();
  void NotifyP2PDisconnected(LanRemotePeer_C* lp, ResultCode reason);
  void EnqueuePeerLeaveEvent(LanRemotePeer_C* lp, ResultCode result_code,
                             ResultCode detail_code, const ByteArray& comment,
                             SocketErrorCode socket_error);
  void EnqueuePeerConnectionEstablishEvent(LanRemotePeer_C* lp);
  void EnqueuePeerDisconnectEvent(LanRemotePeer_C* lp, ResultCode reason);
  void EveryClientDispose();

  // 스레드 섞기 작업
  void OnSocketIoCompletion(Array<IHostObject*>& send_issued_pool,
                            ReceivedMessageList& msg_list,
                            CompletionStatus& completion);

 public:
  void AttachProxy(RpcProxy* proxy) { NetCoreImpl::AttachProxy(proxy); }
  void AttachStub(RpcStub* stub) { NetCoreImpl::AttachStub(stub); }
  void DetachProxy(RpcProxy* proxy) { NetCoreImpl::DetachProxy(proxy); }
  void DetachStub(RpcStub* stub) { NetCoreImpl::DetachStub(stub); }

  int32 GetMessageMaxLength();

  virtual CCriticalSection2& GetMutex() { return mutex_; }

  void CheckCriticalSectionDeadLock_INTERNAL(const char* where);

  void LockMain_AssertIsLockedByCurrentThread() {
    GetMutex().AssertIsLockedByCurrentThread();
  }

  void LockMain_AssertIsNotLockedByCurrentThread() {
    GetMutex().AssertIsNotLockedByCurrentThread();
  }

  bool SendFreeform(const HostId* sendto_list, int32 sendto_count,
                    const RpcCallOption& opt, const uint8* payload,
                    int32 payload_length) {
    return NetCoreImpl::SendFreeform(sendto_list, sendto_count, opt, payload,
                                     payload_length);
  }

  P2PGroupPtr_C CreateP2PGroupObject_INTERNAL(HostId group_id);

  bool RunAsync(HostId task_owner_id, Function<void()> func) override;
};

}  // namespace net
}  // namespace fun
