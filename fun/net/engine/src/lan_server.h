//@deprecated
#pragma once

#include "LanRemoteClient.h"

#include "GeneratedRPCs/Lan_LanC2S_stub.h"
#include "GeneratedRPCs/Lan_LanS2C_proxy.h"

#include "thread_pool_impl.h" // ThreadPool
#include "host_id_factory.h"  // IHostIdFactory
#include "RCPair.h"           // RCPair
#include "Tracer.h"           // LogWriter

namespace fun {
namespace net {

class LanListener_S;
class P2PGroup_MemberJoin_AckKey;
class MessageSummary;
class ServerSocketPool;
class NetClientImpl;

class LanSendDestInfo_S {
 public:
  HostId host_id;
  ISendDest_S* object;

  LanSendDestInfo_S() : host_id(HostId_None), object(nullptr) {}

  friend bool operator == (const LanSendDestInfo_S& a, const LanSendDestInfo_S& b) {
    return a.host_id == b.host_id;
  }

  friend bool operator != (const LanSendDestInfo_S& a, const LanSendDestInfo_S& b) {
    return a.host_id != b.host_id;
  }

  friend bool operator < (const LanSendDestInfo_S& a, const LanSendDestInfo_S& b) {
    return a.host_id < b.host_id;
  }
};

//class LanSendDestInfoList_S : public Array<LanSendDestInfo_S, InlineAllocator<NetConfig::OrdinaryHeavyS2CMulticastCount>> {};
//class LanSendDestInfoList_S : public Array<LanSendDestInfo_S> {};
typedef Array<LanSendDestInfo_S, InlineAllocator<NetConfig::OrdinaryHeavyS2CMulticastCount>> LanSendDestInfoList_S;

typedef Map<RCPair, LanP2PConnectionStatePtr> LCPairMap;

/**
 * P2P 연결 상태 맵
 * 직접 P2P 통신(TCP)만 들어있음.
 */
class LanP2PPairList {
 public:
  LCPairMap list;

  /**
   * 직접 연결된 P2P 쌍인지.
   */
  LanP2PConnectionStatePtr GetPair(HostId a, HostId b);

  void RemovePairOfAnySide(LanClient_S* client);
  void AddPair(LanClient_S* client_a, LanClient_S* client_b, LanP2PConnectionStatePtr state);
  void ReleasePair(LanServerImpl* owner, LanClient_S* client_a, LanClient_S* client_b);
};

class LanServerImpl
  : public NetCoreImpl,
    public IInternalSocketDelegate,
    public ICompletionPortCallbacks,
    public ISendDest_S,
    public ITaskSubject,
    public LanServer,
    public ICompletionContext,
    public LanP2PGroupMemberBase_S,
    public IUserTaskQueueOwner,
    public ISocketIoCompletionDelegate,
    public ICompletionKey,
    public IThreadReferer,
    public IThreadPoolCallbacks {
 private:
  CCriticalSection2 main_mutex_;
  CCriticalSection2 start_stop_phase_mutex_;

 public:
  // ICompletionContext interface
  LeanType GetLeanType() const override { return LeanType::LanServer; }

 public:
  virtual CCriticalSection2& GetMutex() { return main_mutex_; }
  void CheckCriticalSectionDeadLock_INTERNAL(const char* where);

  FUN_ALIGNED_VOLATILE double AbsoluteTime_USE_GetAbsoluteTime;

  // 이 메서드가 따로 있는 이유: cached time 값이 0인 경우를 피하기 위함

  //주의 : AbsoluteTime_USE_GetAbsoluteTime 는 realtime이 아니고, heartbeat마다 갱신되는 값임.

  double GetAbsoluteTime() {
    if (AbsoluteTime_USE_GetAbsoluteTime == 0) {
      AbsoluteTime_USE_GetAbsoluteTime = clock_.AbsoluteSeconds();
    }

    return AbsoluteTime_USE_GetAbsoluteTime;
  }

  // 파라메터로 받은 서버의 로컬 IP
  // 서버가 공유기나 L4 스위치 뒤에 있는 경우 유용
  String server_ip_alias_;

  // 로컬 NIC가 여럿인 경우 지정된 것(혹은 미지정)
  String local_nic_addr_;

  // 엔진 버전
  uint32 internal_version_;

  NetSettings settings_;

  // 이 객체 인스턴스의 GUID. 이 값은 전세계 어느 컴퓨터에서도 겹치지 않는 값이어야 한다.
  // 같은 호스트에서 여러개의 서버를 실행시 p2p 홀펀칭 시도중 host_id, peer addr만 갖고는 어느 서버와 연결된 것에 대한 것인지
  // 구별이 불가능하므로 이 기능을 쓰는 것이다.
  Uuid instance_tag_;

  // 여기에 등록된 클라들은 곧 파괴될 것이다
  // issue중이던 것들은 바로 dispose시 에러가 발생한다. 혹은 issuerecv/send에서 이벤트 없이 에러가 발생한다.
  // 그래서 이 변수가 쓰이는거다.
  // host_id=0인 경우, 즉 비인증 상태의 객체가 파괴되는 경우도 감안, EHostId를 키로 두지 않는다.
  // key: object, value: disposed time
  typedef Map<LanClient_S*, double> DisposeIssuedLanClientMap;
  DisposeIssuedLanClientMap dispose_issued_lan_clients_map_;

  // P2P 통신중인 클라이언트끼리의 리스트(직접연결만 포함)
  LanP2PPairList lan_p2p_connection_pair_list_;

  UniquePtr<LanListener_S> listener_thread_;

  UniquePtr<thread_pool_impl> net_thread_pool_;
  bool net_thread_external_use_;
  UniquePtr<thread_pool_impl> user_thread_pool_;
  bool user_thread_external_use_;

  FUN_ALIGNED_VOLATILE bool net_thread_pool_unregisted_;
  FUN_ALIGNED_VOLATILE bool user_thread_pool_unregisted_;

  Uuid protocol_version_;
  bool user_task_is_running_;
  LanP2PGroups_S lan_p2p_groups_;
  FinalUserWorkItemQueue_S final_user_work_queue_;
  UniquePtr<IHostIdFactory> host_id_factory_;

  struct C2SStub : public LanC2S::Stub {
    LanServerImpl* owner_;

    DECLARE_RPCSTUB_LanC2S_P2PGroup_MemberJoin_Ack;
    DECLARE_RPCSTUB_LanC2S_ReliablePing;
    DECLARE_RPCSTUB_LanC2S_ReportP2PPeerPing;
    DECLARE_RPCSTUB_LanC2S_ShutdownTcp;
    DECLARE_RPCSTUB_LanC2S_ShutdownTcpHandshake;
  };

  struct S2CProxy : public LanS2C::Proxy {};

  C2SStub c2s_stub_;
  S2CProxy s2c_proxy_;

 public:
  FUN_ALIGNED_VOLATILE uint64 total_tcp_recv_count_;
  FUN_ALIGNED_VOLATILE uint64 total_tcp_recv_bytes_;
  FUN_ALIGNED_VOLATILE uint64 total_tcp_send_count;
  FUN_ALIGNED_VOLATILE uint64 total_tcp_send_bytes;

  InternalSocketPtr tcp_listening_socket_;

  Singleton<ServerSocketPool>::Ptr server_socket_pool_;

  // cs locked인 경우 local event를 enque하기만 해야 한다. cs unlock인 경우에는 콜백 허용
  ILanServerCallbacks* callbacks_;

  Clock clock_;
  //UniquePtr<CompletionPort> completion_port_;
  UniquePtr<CompletionPort> tcp_accept_cp_;

  //tcp 송신 이슈를 보호한다.
  CCriticalSection2 tcp_issue_queue_mutex_;

  // TCP 송신 이슈가 걸린 것들
  ListNode<LanClient_S>::ListOwner tcp_issue_send_ready_remote_clients_;

  // 루프백 메시지용 키
  SessionKey self_session_key_;
  CryptoCountType self_encrypt_count_;
  CryptoCountType self_decrypt_count_;

  // AES 키를 교환하기 위한 RSA key
  CryptoRSAKey self_xchg_key_;
  // 공개키는 미리 blob 으로 변환해둔다.
  ByteArray public_key_blob_;

  uint32 timer_callback_interval_;
  void* timer_callback_context_;

  Timer timer_;

  TimerTaskIdType heartbeat_timer_id_;
  TimerTaskIdType issue_send_on_need_timer_id_;
  TimerTaskIdType tick_timer_id_;

  IntervalAlaram remove_too_old_tcp_send_packet_queue_alarm_;
  IntervalAlaram disconnect_remote_client_on_timeout_alarm_;
  IntervalAlaram pure_too_old_unmature_client_alarm_;
  IntervalAlaram purge_too_old_add_member_ack_item_alarm_;
  IntervalAlaram dispose_issued_remote_clients_alarm_;

  void PostHeartbeatIssue();
  void Heartbeat();
  void Heartbeat_One(double elapsed_time);

  void PostEveryRemote_IssueSend();
  void EveryRemote_IssueSendOnNeed(Array<IHostObject*>& pool);

  void PostOnTick();
  void OnTick();

  //
  // completion handlers
  //

  void IoCompletion_TcpRecvCompletionCase(CompletionStatus& completion, LanClient_S* lc, ReceivedMessageList& received_msg_list);
  void IoCompletion_TcpSendCompletionCase(CompletionStatus& completion, LanClient_S* lc);
  void IoCompletion_TcpCustomValueCase(CompletionStatus& completion, LanClient_S* lc);

  void IoCompletion_NewClientCase(LanClient_S* lc);
  void IoCompletion_ProcessMessageOrMoveToFinalRecvQueue(LanClient_S* lc, ReceivedMessageList& extracted_msg_list);
  bool IoCompletion_ProcessMessage_EngineLayer(ReceivedMessage& received_msg, LanClient_S* lc);

  void IoCompletion_ProcessMessage_NotifyCSEncryptedSessionKey(MessageIn& msg, LanClient_S* lc);
  void IoCompletion_ProcessMessage_NotifyServerConnectionRequestData(MessageIn& msg, LanClient_S* lc);
  void IoCompletion_ProcessMessage_RequestServerTimeAndKeepAlive(MessageIn& msg, LanClient_S* lc);
  void IoCompletion_ProcessMessage_NotifyCSConnectionPeerSuccess(MessageIn& msg, LanClient_S* lc);
  void IoCompletion_ProcessMessage_NotifyCSP2PDisconnected(MessageIn& msg, LanClient_S* lc);
  void IoCompletion_ProcessMessage_RPC(ReceivedMessage& received_msg, bool msg_processed, LanClient_S* lc);
  void IoCompletion_ProcessMessage_FreeformMessage(ReceivedMessage& received_msg, bool msg_processed, LanClient_S* lc);

  void UserTaskQueue_Add(LanClient_S* lc, ReceivedMessage& received_msg, FinalUserWorkItemType type);
  void NotifyProtocolVersionMismatch(LanClient_S* lc);
  void CatchThreadExceptionAndPurgeClient(LanClient_S* lc, const char* where, const char* reason);
  void OnSocketIoCompletion(Array<IHostObject*>& send_issued_pool, ReceivedMessageList& msg_list, CompletionStatus& completion);
  void OnIoCompletion(Array<IHostObject*>& send_issued_pool, ReceivedMessageList& msg_list, CompletionStatus& completion);

  //
  // UserTask
  //

  UserTaskQueue user_task_queue_;
  void PostUserTask() override;
  void DoUserTask();

  void UserWork_FinalReceiveRPC(FinalUserWorkItem& uwi, void* host_tag);
  void UserWork_FinalReceiveFreeformMessage(FinalUserWorkItem& uwi, void* host_tag);
  void UserWork_FinalUserTask(FinalUserWorkItem& uwi, void* host_tag);
  void UserWork_LocalEvent(FinalUserWorkItem& uwi);

  virtual void OnThreadBegin() override;
  virtual void OnThreadEnd() override;

  void EndCompletion();

  //@warning Start()를 호출하기 전에 설정해야함.
  void SetCallbacks(ILanServerCallbacks* callbacks);

  bool Start(const LanStartServerArgs& args, SharedPtr<ResultInfo>& out_error);
  void Stop();

 private:
  bool CreateTcpListenSocketAndInit(int32 tcp_port, SharedPtr<ResultInfo>& out_error);
  virtual bool AsyncCallbackMayOccur();

 public:
  // 서버와의 연결이 완전히 완료되었으며 dispose issue가 발생하기 전의 LC들
  LanClients_S authed_lan_clients_;
  RandomMT random_; //@todo 이놈은 왜 필요한건가??

  struct CandidateLanClients : public Map<InetAddress, LanClient_S*> {
    void Remove(LanClient_S* lc);
  };
  CandidateLanClients candidate_lan_clients_;

  // 실제로 LC들의 소유권을 가지는 곳
  Set<LanClient_S*> lan_client_instances_;

  /** ClientHostId가 가리키는 peer가 참여하고 있는 P2P group 리스트를 얻는다. */
  bool GetJoinedP2PGroups(HostId client_id, HostIdArray& output);

  LanClient_S* GetCandidateLanClientByTcpAddr(const InetAddress& client_addr);

  // 너무 오랫동안 AcItem이 처리되지 않은 것들을 청소.
  void PurgeTooOldAddMemberAckItem();
  // 너무 오랫동안 unmature 상태를 못 벗어나는 remote Client들을 청소.
  void PurgeTooOldUnmatureClient();

  void GetP2PGroups(P2PGroupInfos& out_result);
  int32 GetP2PGroupCount();

  void OnSocketWarning(InternalSocket* socket, const String& text);
  void OnCompletionPortWarning(CompletionPort* port, const char* text) {
    //Console.WriteLine(text);
  }

  bool IsValidHostId(HostId host_id);
  bool IsValidHostId_NOLOCK(HostId host_id);
  bool IsDisposeLanClient_NOLOCK(LanClient_S* lc);

  void EnqueueClientLeaveEvent( LanClient_S* lc,
                                ResultCode result_code,
                                ResultCode detail_code,
                                const ByteArray& comment,
                                SocketErrorCode socket_error);

  void ProcessOneLocalEvent(LocalEvent& event);

  virtual bool SetHostTag(HostId host_id, void* host_tag);

  LanServerImpl();
  virtual ~LanServerImpl();

  void AssertIsLockedByCurrentThread() {
    GetMutex().AssertIsLockedByCurrentThread();
  }

  void AssertIsNotLockedByCurrentThread() {
    GetMutex().AssertIsNotLockedByCurrentThread();
  }

  void ProcessOnClientDisposeCanSafe(LanClient_S* lc);

  INetCoreCallbacks* GetCallbacks_NOLOCK() override;

  SessionKey* GetCryptSessionKey(HostId remote_id, String& out_error);
  bool NextEncryptCount(HostId remote_id, CryptoCountType& out_count);
  void PrevEncryptCount(HostId remote_id);
  bool GetExpectedDecryptCount(HostId remote_id, CryptoCountType& out_count);
  bool NextDecryptCount(HostId remote_id);

  virtual bool Send(const SendFragRefs& data_to_send, const SendOption& send_opt, const HostId* sendto_list, int32 sendto_count) override;

 public:
  bool Send_BroadcastLayer(const SendFragRefs& payload, const SendOption& send_opt, const HostId* sendto_list, int32 sendto_count);

  bool SendWithSplitter_DirectlyToClient_Copy(HostId client_id, const SendFragRefs& data_to_send, MessageReliability reliability, const SendOption& send_opt);

  void ConvertGroupToIndividualsAndUnion(int32 sendto_count, const HostId* sendto_list, HostIdArray& out_send_dest_list);

  virtual bool CloseConnection(HostId client_id);
  virtual void CloseAllConnections();
  void RequestAutoPrune(LanClient_S* lc);

  LanClient_S* GetLanClientByHostId_NOLOCK(HostId client_id);
  LanClient_S* GetAuthedLanClientByHostId_NOLOCK(HostId client_id);

  double GetLastPingSec(HostId peer_id);
  double GetRecentPingSec(HostId peer_id);

  bool DestroyP2PGroup(HostId group_id);

  bool LeaveP2PGroup(HostId member_id, HostId group_id);
  HostId CreateP2PGroup(const HostId* client_host_ids, int32 count, const ByteArray& custom_field);
  bool JoinP2PGroup(HostId member_id, HostId group_id, const ByteArray& custom_field);

  void EnqueueP2PAddMemberAckCompleteEvent(HostId group_id, HostId added_member_host_id, ResultCode result);
  void EnqueueGroupP2PConnectionCompleteEvent(HostId group_id);
  void EnqueueP2PConnectionEstablishedEvent(HostId member_id, HostId remote_id);

 private:
  uint32 joined_p2p_group_key_gen_;

  void AddMemberAckWaiters_RemoveRelated_MayTriggerJoinP2PMemberCompleteEvent(LanP2PGroup_S* group, HostId member_id, ResultCode reason);
  bool JoinP2PGroup_INTERNAL(HostId member_id, HostId group_id, const ByteArray& custom_field, uint32 joined_p2p_group_key_gen);

 public:
  // 그룹의 P2P Connection 상태를 업데이트한다.
  bool UpdateGroupP2PConnection(HostId group_id);
  void P2PGroup_CheckConsistency();
  void DestroyEmptyP2PGroups();

  int32 GetClientHostIds(HostId* output, int32 output_length);

  bool GetClientInfo(HostId client_id, NetClientInfo& out_info);

  LanP2PGroupPtr_S GetP2PGroupByHostId_NOLOCK(HostId group_id);
  bool GetP2PGroupInfo(HostId group_id, P2PGroupInfo& output);
  void ConvertAndAppendP2PGroupToPeerList(HostId send_to, HostIdArray& SendTo2);

  ISendDest_S* GetSendDestByHostId_NOLOCK(HostId peer_id);

  bool GetP2PConnectionStats(HostId remote_id, P2PConnectionStats& out_stats);

  // ITaskSubject interface
  HostId GetHostId() const override { return HostId_Server; }
  bool IsFinalReceiveQueueEmpty() override;
  bool IsTaskRunning() override;
  void OnSetTaskRunningFlag(bool running) override;
  bool PopFirstUserWorkItem(FinalUserWorkItem& output) override;

  // 클라이언트를 종료 모드로 바꾸도록 지시한다.
  // 바로 객체를 파괴할 수 없다. NetWorkerThread에서 먼저 끝내줄 때까지 기다려야 하니까.
  void IssueDisposeLanClient( LanClient_S* lc,
                              ResultCode result_code,
                              ResultCode detail_code,
                              const ByteArray& comment,
                              const char* where,
                              SocketErrorCode socket_error);

  void LanClient_RemoveFromCollections(LanClient_S* lc);
  void DisposeIssuedLanClients();
  void HardDisconnect_AutoPruneGoesTooLongClient(LanClient_S* lc);

  // Super-peer 선출
  void ElectSuperPeer();

  double GetTime();

  String DumpGroupStatus();

  int32 GetClientCount();
  void GetStats(LanServerStats& out_stats);

  inline void AttachProxy(RpcProxy* proxy) { NetCoreImpl::AttachProxy(proxy); }
  inline void AttachStub(RpcStub* stub) { NetCoreImpl::AttachStub(stub); }
  inline void DetachProxy(RpcProxy* proxy) { NetCoreImpl::DetachProxy(proxy); }
  inline void DetachStub(RpcStub* stub) { NetCoreImpl::DetachStub(stub); }

  void ShowError_NOLOCK(SharedPtr<ResultInfo> result_info);
  void ShowWarning_NOLOCK(SharedPtr<ResultInfo> result_info);

  void ShowNotImplementedRpcWarning(RpcId rpc_id, const char* rpc_name) {
    NetCoreImpl::ShowNotImplementedRpcWarning(rpc_id, rpc_name);
  }

  void PostCheckReadMessage(IMessageIn& msg, RpcId rpc_id, const char* rpc_name) {
    NetCoreImpl::PostCheckReadMessage(msg, rpc_id, rpc_name);
  }

  NamedInetAddress GetRemoteIdentifiableLocalAddr();

  void SetDefaultTimeoutTimeSec(double timeout_sec);
  void SetDefaultTimeoutTimeMilisec(uint32 timeout_msec);

  bool SetDirectP2PStartCondition(DirectP2PStartCondition condition);

  // 너무 오랫동안 쓰이지 않은 TCP 송신 큐를 찾아서 제거한다.
  void Tcp_LongTick();

  //void DisconnectLanClientOnTimeout();
  void Heartbeat_PerClient();

  void GetUserWorkerThreadInfo(Array<ThreadInfo>& output);
  void GetNetWorkerThreadInfo(Array<ThreadInfo>& output);

 private:
  void RefreshSendQueuedAmountStat(LanClient_S* lc);

 public:
  virtual InetAddress GetTcpListenerLocalAddr();

 public:
  void Convert_NOLOCK(LanSendDestInfoList_S& dst, HostIdArray& src);

  UniquePtr<LogWriter> intra_logger_;

  virtual void EnableIntraLog(const char* log_filename);
  virtual void DisableIntraLog();

  void EnqueueHackSuspectEvent(LanClient_S* lc, const char* statement, HackType hack_type);
  void EnqueueP2PGroupRemoveEvent(HostId group_id);
  void EnqueueP2PDisconnectEvent(HostId member_id, HostId remote_id, ResultCode error_type);

  virtual void SetMessageMaxLength(int32 MaxLength);

  virtual bool IsConnectedClient(HostId client_id);

 public:
  virtual uint32 GetInternalVersion();

  //void EnqueueUnitTestFailEvent(const String& text);
  void EnqueueError(SharedPtr<ResultInfo> result_info);
  void EnqueueWarning(SharedPtr<ResultInfo> result_info);

  void EnqueueLocalEvent(LocalEvent& Event);

  void EnqueueClientJoinApproveDetermine(const InetAddress& client_tcp_addr, const ByteArray& request);
  void ProcessOnClientJoinApproved(LanClient_S* lc, const ByteArray& response);
  void ProcessOnClientJoinRejected(LanClient_S* lc, const ByteArray& response);

 private:
  // true시 빈 P2P 그룹도 허용할지 여부.
  bool bAllowEmptyP2PGroup;

  ITaskSubject* GetTaskSubjectByHostId_NOLOCK(HostId subject_host_id);

 public:
  virtual void AllowEmptyP2PGroup(bool allow);

 private:
  String GetConfigString();

 public:
  virtual void EnqueueWarning(const InetAddress& sender, const char* text);

 private:
  virtual int32 GetMessageMaxLength();
  virtual HostId GetLocalHostId();
  virtual bool IsNagleAlgorithmEnabled();

//public:
  // 종료시에만 설정됨
  // Stop()이 return하기 전까지는 이벤트 콜백이 있어야하므로 삭제한다.
  //FUN_ALIGNED_VOLATILE bool tear_down_;

 public:
  //void ConditionalLogFreqFail();
  //void LogFreqFailNow();

  bool SendFreeform(const HostId* sendto_list,
                    int32 sendto_count,
                    const RpcCallOption& rpc_call_opt,
                    const uint8* payload,
                    int32 payload_length) {
    return NetCoreImpl::SendFreeform( sendto_list,
                                      sendto_count,
                                      rpc_call_opt,
                                      payload,
                                      payload_length);
  }

  bool RunAsync(HostId task_owner_id, Function<void()> func) override;

 private:
  virtual NetClientImpl* QueryNetClient() { return nullptr; }
  virtual LanServerImpl* QueryLanServer() { return this; }

  virtual void EnableVizAgent(const char* viz_server_ip, int32 viz_server_port, const String& login_key) {}
  virtual void Viz_NotifySendByProxy(const HostId* sendto_list, int32 sendto_count, const MessageSummary& summary, const RpcCallOption& rpc_call_opt) {}
  virtual void Viz_NotifyRecvToStub(HostId rpc_recvfrom, RpcId rpc_id, const char* rpc_name, const char* params_as_string) {}
};

} // namespace net
} // namespace fun
