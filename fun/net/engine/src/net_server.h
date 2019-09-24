#pragma once

#include "IVizAgentDelegate.h"
#include "RCPair.h"  // RCPair
#include "RemoteClient.h"
#include "ServerSocketPool.h"
#include "Tracer.h"            // LogWriter
#include "host_id_factory.h"   // IHostIdFactory
#include "thread_pool_impl.h"  // ThreadPool

#include "Misc/TickableTimer.h"

#include "GeneratedRPCs/net_NetC2S_stub.h"
#include "GeneratedRPCs/net_NetS2C_proxy.h"

namespace fun {

class NetListener_S;
class P2PGroup_MemberJoin_AckKey;
class MessageSummary;

class SendDestInfo_S {
 public:
  HostId host_id;

  ISendDest_S* object;

  // P2P routed broadcast와 연계됐는지 여부를 파악하는데 씀.
  SendDestInfo_S* p2p_route_prev_link;
  SendDestInfo_S* p2p_route_next_link;

  //실제 보내야할 대상.
  //멀티 스레드 코딩에서 추가됨.이것은 udpsocket혹은 remoteClient이다.
  IHostObject* host_object;
  InetAddress dest_addr;

 public:
  SendDestInfo_S()
      : host_id(HostId_None),
        object(nullptr),
        p2p_route_prev_link(nullptr),
        p2p_route_next_link(nullptr),
        host_object(nullptr) {}

  friend bool operator==(const SendDestInfo_S& a, const SendDestInfo_S& b) {
    return a.host_id == b.host_id;
  }

  friend bool operator!=(const SendDestInfo_S& a, const SendDestInfo_S& b) {
    return a.host_id != b.host_id;
  }

  friend bool operator<(const SendDestInfo_S& a, const SendDestInfo_S& b) {
    return a.host_id < b.host_id;
  }
};

typedef Array<SendDestInfo_S,
              InlineAllocator<NetConfig::OrdinaryHeavyS2CMulticastCount>>
    SendDestInfoList_S;
// typedef Array<SendDestInfo_S> SendDestInfoList_S;

typedef Map<RCPair, P2PConnectionStatePtr> RCPairMap;

/**
 * P2P 연결 상태 맵
 *
 * 직접 P2P 통신중이 아니더라도 등록되어 있다.
 */
class P2PPairList {
 public:
  /** 릴레이건 직접이건, P2P 연결이 활성화된 것들 */
  RCPairMap active_pairs;
  /** 비활성되어있지만 다시 재사용될 가능성이 있는 것들, 즉 최근 일정시간까지는
   * 활성화되어있던 것들 */
  RCPairMap recyclable_pairs;

  /// 직접연결된 P2P 쌍인지.
  P2PConnectionStatePtr GetPair(HostId a, HostId b);
  void RemovePairOfAnySide(RemoteClient_S* client);
  void AddPair(RemoteClient_S* client_a, RemoteClient_S* client_b,
               P2PConnectionStatePtr state);
  void ReleasePair(NetServerImpl* owner, RemoteClient_S* client_a,
                   RemoteClient_S* client_b);

  void AddRecyclePair(RemoteClient_S* client_a, RemoteClient_S* client_b,
                      P2PConnectionStatePtr state);
  void RemoveTooOldRecyclePair(double absolute_time);

  P2PConnectionStatePtr GetRecyclePair(HostId a, HostId b, bool remove = true);

  void Clear();
};

class NetServerImpl : public NetCoreImpl,
                      public IInternalSocketDelegate,
                      public ICompletionPortCallbacks,
                      public ISendDest_S,
                      public ITaskSubject,
                      public NetServer,
                      public ICompletionContext,
                      public P2PGroupMemberBase_S,
                      public IVizAgentDelegate,
                      public IUserTaskQueueOwner,
                      public ISocketIoCompletionDelegate,
                      public ICompletionKey,
                      public IThreadReferer,
                      public IThreadPoolCallbacks {
 private:
  friend class UdpSocket_S;

  CCriticalSection2 main_mutex_;
  CCriticalSection2 start_stop_phase_mutex_;

 public:
  // ICompletionContext interface
  LeanType GetLeanType() const override { return LeanType::NetServer; }

 public:
  virtual CCriticalSection2& GetMutex() override { return main_mutex_; }

  // IVizAgentDelegate interface
  // TODO 뭔가 정리가 안되는 느낌이다...
  //얘는 리얼타임을 사용하는건가???
  inline double GetAbsoluteTime() override { return clock_.AbsoluteSeconds(); }

  // 파라메터로 받은 서버의 로컬 IP
  // 서버가 공유기나 L4 스위치 뒤에 있는 경우 유용
  String server_ip_alias_;

  // 로컬 NIC가 여럿인 경우 지정된 것(혹은 미지정)
  String local_nic_addr_;

  // 엔진 버전
  uint32 internal_version_;

  NetSettings settings_;

  Timer timer_;
  uint32 timer_callback_interval_;
  void* timer_callback_context_;

  // 이 서버가 실행되는 환경에서, ICMP 패킷이 도착하는 호스트로부터의 통신을
  // 완전 차단하는 환경이 적용되어 있는지? false이면 안전하게 ttl 을 수정한 패킷
  // 교환이 가능
  bool using_over_block_icmp_environment_;

  // 이 객체 인스턴스의 GUID. 이 값은 전세계 어느 컴퓨터에서도 겹치지 않는
  // 값이어야 한다. 같은 호스트에서 여러개의 서버를 실행시 P2P 홀펀칭 시도중
  // host_id, peer addr만 갖고는 어느 서버와 연결된 것에 대한 것인지 구별이
  // 불가능하므로 이 기능을 쓰는 것이다.
  Uuid instance_tag_;

  // 여기에 등록된 클라들은 곧 파괴될 것이다

  // issue중이던 것들은 바로 dispose시 에러가 발생한다. 혹은 issuerecv/send에서
  // 이벤트 없이 에러가 발생한다. 그래서 이 변수가 쓰이는거다. host_id=0인 경우,
  // 즉 비인증 상태의 객체가 파괴되는 경우도 감안, EHostId를 키로 두지 않는다.
  // key: object, value: disposed time
  typedef Map<RemoteClient_S*, double> DisposeIssuedRemoteClientMap;
  DisposeIssuedRemoteClientMap dispose_issued_remote_clients_map_;

  // P2P 통신중인 클라이언트끼리의 리스트(직접,간접 포함)
  P2PPairList p2p_connection_pair_list_;

  // UDP 주소->클라 객체 인덱스

  // UDP 수신시 고속 검색을 위해 필요하다.
  struct UdpAddrToRemoteClientIndex : public Map<InetAddress, RemoteClient_S*> {
    UdpAddrToRemoteClientIndex();
  };
  UdpAddrToRemoteClientIndex udp_addr_to_remote_client_index_;

  struct P2PGroupSubset_S {
    HostId group_id;
    HostIdArray excludee_host_id_list;
  };

  UniquePtr<NetListener_S> listener_;
  UniquePtr<thread_pool_impl> net_thread_pool_;
  bool net_thread_external_use_;
  UniquePtr<thread_pool_impl> user_thread_pool_;
  bool user_thread_external_use_;

  FUN_ALIGNED_VOLATILE bool net_thread_pool_unregisted_;
  FUN_ALIGNED_VOLATILE bool user_thread_pool_unregisted_;

  Uuid protocol_version;
  bool user_task_is_running_;
  P2PGroups_S p2p_groups_;
  FinalUserWorkItemQueue_S final_user_work_queue_;
  UniquePtr<IHostIdFactory> host_id_factory_;

  struct C2SStub : public NetC2S::Stub {
    NetServerImpl* owner_;

    DECLARE_RPCSTUB_NetC2S_P2PGroup_MemberJoin_Ack;
    DECLARE_RPCSTUB_NetC2S_NotifyP2PHolepunchSuccess;
    DECLARE_RPCSTUB_NetC2S_P2P_NotifyDirectP2PDisconnected;
    DECLARE_RPCSTUB_NetC2S_NotifyUdpToTcpFallbackByClient;
    DECLARE_RPCSTUB_NetC2S_ReliablePing;
    DECLARE_RPCSTUB_NetC2S_ShutdownTcp;
    DECLARE_RPCSTUB_NetC2S_ShutdownTcpHandshake;
    DECLARE_RPCSTUB_NetC2S_NotifyLog;
    DECLARE_RPCSTUB_NetC2S_NotifyLogHolepunchFreqFail;
    DECLARE_RPCSTUB_NetC2S_NotifyNatDeviceName;
    DECLARE_RPCSTUB_NetC2S_NotifyPeerUdpSocketRestored;
    DECLARE_RPCSTUB_NetC2S_NotifyJitDirectP2PTriggered;
    DECLARE_RPCSTUB_NetC2S_NotifyNatDeviceNameDetected;
    DECLARE_RPCSTUB_NetC2S_NotifySendSpeed;
    DECLARE_RPCSTUB_NetC2S_ReportP2PPeerPing;
    DECLARE_RPCSTUB_NetC2S_C2S_RequestCreateUdpSocket;
    DECLARE_RPCSTUB_NetC2S_C2S_CreateUdpSocketAck;
    DECLARE_RPCSTUB_NetC2S_ReportC2CUdpMessageCount;
    DECLARE_RPCSTUB_NetC2S_ReportC2SUdpMessageTrialCount;
  };

  struct S2CProxy : public NetS2C::Proxy {};

  C2SStub c2s_stub_;
  S2CProxy s2c_proxy_;

 public:
  //
  // Stats
  //

  FUN_ALIGNED_VOLATILE uint64 total_tcp_recv_count_;
  FUN_ALIGNED_VOLATILE uint64 total_tcp_recv_bytes_;
  FUN_ALIGNED_VOLATILE uint64 total_tcp_send_count_;
  FUN_ALIGNED_VOLATILE uint64 total_tcp_send_bytes_;
  FUN_ALIGNED_VOLATILE uint64 total_udp_recv_count_;
  FUN_ALIGNED_VOLATILE uint64 total_udp_recv_bytes_;
  FUN_ALIGNED_VOLATILE uint64 total_udp_send_bytes_;
  FUN_ALIGNED_VOLATILE uint64 total_udp_send_count_;

  // RC들이 공유하는 UDP socket들

  Array<UdpSocketPtr_S> udp_sockets_;

  Map<InetAddress, UdpSocketPtr_S>
      local_addr_to_udp_socket_map_;  // UDP socket의 local 주소 -> 소켓 객체

  InternalSocketPtr tcp_listening_socket_;

  Singleton<ServerSocketPool>::CPtr server_socket_pool_;

  // cs locked인 경우 local event를 enque하기만 해야 한다. cs unlock인 경우에는
  // 콜백 허용
  INetServerCallbacks* callbacks_;

  Clock clock_;
  UniquePtr<CompletionPort> tcp_accept_cp_;

  // 송신큐를 보호할 CriticalSection을 따로 둔다.
  CCriticalSection2 tcp_issue_queue_mutex_;
  CCriticalSection2 udp_issue_queue_mutex_;

  // TCP 송신 이슈를 걸어야 할 놈들
  // sendissued가 false이고 Sendqueue.Length > 0 인놈들의 모음
  ListNode<RemoteClient_S>::ListOwner tcp_issue_send_ready_remote_clients_;

  // FunNet.NetConfig.EveryRemoteIssueSendOnNeedInterval이지만 테스트를 위해
  // 값을 수정할 경우를 위해 멤버로 떼놨다.
  double every_remote_issue_send_on_need_internal_;

  // IssueSend를 걸어야 하는 것들
  // send queue에 뭔가가 들어가는 순간, issue send = F인 것들이 여기에 등록된다.
  // issue send = T가 되는 순간 여기서 빠진다.
  // UDP 소켓을 굉장히 많이 둔 경우 모든 UDP socket에 대해 루프를 돌면
  // 비효율적이므로 이것이 필요
  ListNode<UdpSocket_S>::ListOwner udp_issued_send_ready_list_;

  // 루프백 메시지용 키
  SessionKey self_session_key_;
  CryptoCountType self_encrypt_count_;
  CryptoCountType self_decrypt_count_;

  // aes 키를 교환하기 위한 rsa key
  CryptoRSAKey self_xchg_key_;
  // 공개키는 미리 blob 으로 변환해둔다.
  ByteArray public_key_blob_;

  // Networker를 위한 heartbeat관련.
  TimerTaskIdType heartbeat_timer_id_;

  volatile int32 heartbeat_working_;
  volatile int32 on_tick_working_;

  void PostHeartbeatIssue();
  void Heartbeat();

  TickableTimer heartbeat_tickable_timer_;

  // issue send on need를 위한 timer 관련.
  TimerTaskIdType issue_send_on_need_timer_id_;

  void PostEveryRemote_IssueSend();
  void EveryRemote_IssueSendOnNeed(Array<IHostObject*>& pool);

  void IoCompletion_PerRemoteClient(CompletionStatus& completion,
                                    ReceivedMessageList& msg_list);
  void IoCompletion_UdpConnectee(CompletionStatus& completion,
                                 ReceivedMessageList& msg_list);
  void CatchThreadExceptionAndPurgeClient(RemoteClient_S* rc, const char* where,
                                          const char* reason);

  void IoCompletion_TcpCustomValueCase(CompletionStatus& completion,
                                       RemoteClient_S* rc);
  void IoCompletion_TcpSendCompletionCase(CompletionStatus& completion,
                                          RemoteClient_S* rc);
  void IoCompletion_TcpRecvCompletionCase(CompletionStatus& completion,
                                          RemoteClient_S* rc,
                                          ReceivedMessageList& msg_list);
  void IoCompletion_UdpRecvCompletionCase(CompletionStatus& completion,
                                          UdpSocket_S* udp_socket,
                                          ReceivedMessageList& msg_list);
  void IoCompletion_UdpSendCompletionCase(CompletionStatus& completion,
                                          UdpSocket_S* udp_socket);

  void IoCompletion_NewClientCase(RemoteClient_S* rc);
  void IoCompletion_ProcessMessageOrMoveToFinalRecvQueue(
      RemoteClient_S* rc, ReceivedMessageList& extracted_msg_list,
      UdpSocket_S* udp_socket);
  bool IoCompletion_ProcessMessage_EngineLayer(ReceivedMessage& received_msg,
                                               RemoteClient_S* rc,
                                               UdpSocket_S* udp_socket);
  bool IoCompletion_ProcessMessage_FromUnknownClient(const InetAddress& From,
                                                     MessageIn& received_msg,
                                                     UdpSocket_S* udp_socket);

  void IoCompletion_ProcessMessage_ServerHolepunch(MessageIn& msg,
                                                   const InetAddress& From,
                                                   UdpSocket_S* udp_socket);
  void IoCompletion_ProcessMessage_PeerUdp_ServerHolepunch(
      MessageIn& msg, const InetAddress& From, UdpSocket_S* udp_socket);
  void IoCompletion_ProcessMessage_NotifyCSEncryptedSessionKey(
      MessageIn& msg, RemoteClient_S* rc);
  void IoCompletion_ProcessMessage_NotifyServerConnectionRequestData(
      MessageIn& msg, RemoteClient_S* rc);
  void IoCompletion_ProcessMessage_RequestServerConnectionHint(
      MessageIn& msg, RemoteClient_S* rc);
  void IoCompletion_ProcessMessage_RequestServerTimeAndKeepAlive(
      MessageIn& msg, RemoteClient_S* rc);
  void IoCompletion_ProcessMessage_SpeedHackDetectorPing(MessageIn& msg,
                                                         RemoteClient_S* rc);
  void IoCompletion_ProcessMessage_UnreliableRelay1(MessageIn& msg,
                                                    RemoteClient_S* rc);
  void IoCompletion_ProcessMessage_UnreliableRelay1_RelayDestListCompressed(
      MessageIn& msg, RemoteClient_S* rc);
  void IoCompletion_ProcessMessage_ReliableRelay1(MessageIn& msg,
                                                  RemoteClient_S* rc);
  void IoCompletion_ProcessMessage_LingerDataFrame1(MessageIn& msg,
                                                    RemoteClient_S* rc);
  void IoCompletion_ProcessMessage_NotifyHolepunchSuccess(MessageIn& msg,
                                                          RemoteClient_S* rc);
  void IoCompletion_ProcessMessage_PeerUdp_NotifyHolepunchSuccess(
      MessageIn& msg, RemoteClient_S* rc);
  void IoCompletion_ProcessMessage_RPC(ReceivedMessage& received_msg,
                                       bool msg_processed, RemoteClient_S* rc,
                                       bool is_real_udp);
  void IoCompletion_ProcessMessage_FreeformMessage(
      ReceivedMessage& received_msg, bool msg_processed, RemoteClient_S* rc,
      bool is_real_udp);
  void IoCompletion_ProcessMessage_RequestReceiveSpeedAtReceiverSide_NoRelay(
      MessageIn& msg, RemoteClient_S* rc);
  void IoCompletion_ProcessMessage_ReplyReceiveSpeedAtReceiverSide_NoRelay(
      ReceivedMessage& received_msg, RemoteClient_S* rc);
  void IoCompletion_MulticastUnreliableRelay2_AndUnlock(
      CScopedLock2* main_mutex, const HostIdArray& relay_dest,
      HostId relay_sender_host_id, MessageIn& payload, MessagePriority priority,
      uint64 unique_id);

  void NotifyProtocolVersionMismatch(RemoteClient_S* rc);

  // UserTask
  UserTaskQueue user_task_queue_;

  // 사용자 정의 OnTick을 콜 하기 위한 타이머.
  TimerTaskIdType tick_timer_id_;

  void UserTaskQueue_Add(RemoteClient_S* rc, ReceivedMessage& received_msg,
                         FinalUserWorkItemType type, bool is_real_udp);
  void PostOnTick();
  void OnTick();
  void PostUserTask();
  void DoUserTask();
  void EndCompletion();

  void UserWork_FinalReceiveRPC(FinalUserWorkItem& uwi, void* host_tag);
  void UserWork_FinalReceiveFreeformMessage(FinalUserWorkItem& uwi,
                                            void* host_tag);
  void UserWork_FinalUserTask(FinalUserWorkItem& uwi, void* host_tag);
  void UserWork_LocalEvent(FinalUserWorkItem& uwi);

  void OnThreadBegin() override;
  void OnThreadEnd() override;

  void SetCallbacks(INetServerCallbacks* callbacks);

  //@maxidea: todo: 파라메터 Params을 mutable하게 넘겨주는게 좋을듯... 내부에서
  //Params의 일부 값들이 변경 될수 있다.
  bool Start(const StartServerArgs& args,
             SharedPtr<ResultInfo>& out_error) override;
  void Stop() override;

 private:
  bool CreateTcpListenSocketAndInit(const Array<int32>& tcp_ports,
                                    SharedPtr<ResultInfo>& out_error);
  bool CreateAndInitUdpSockets(ServerUdpAssignMode udp_assign_mode,
                               const Array<int32>& udp_ports,
                               Array<int32>& failed_bind_ports,
                               SharedPtr<ResultInfo>& out_error);

  bool AsyncCallbackMayOccur() override;

 public:
  // 서버와의 연결이 완전히 완료되었으며 dispose issue가 발생하기 전의 RC들

  RemoteClients_S authed_remote_clients_;
  RandomMT random_;

  struct CandidateRemoteClients : public Map<InetAddress, RemoteClient_S*> {
    // TODO 이런 이름이 좀더 좋으려나?? RemoveByValue인데 말이지??
    // void RemoveByRC(RemoteClient_S* rc);
    void Remove(RemoteClient_S* rc);
  };
  CandidateRemoteClients candidate_remote_clients_;

  Set<RemoteClient_S*>
      remote_client_instances_;  // 실제로 RC들의 소유권을 가지는 곳

  // ClientHostId가 가리키는 peer가 참여하고 있는 P2P group 리스트를 얻는다.
  bool GetJoinedP2PGroups(HostId client_id, HostIdArray& output);

  RemoteClient_S* GetRemoteClientByUdpEndPoint_NOLOCK(
      const InetAddress& client_addr);
  RemoteClient_S* GetCandidateRemoteClientByTcpAddr(
      const InetAddress& client_addr);

  void PurgeTooOldAddMemberAckItem();

  // 너무 오랫동안 unmature 상태를 못 벗어나는 remote Client들을 청소.
  void PurgeTooOldUnmatureClient();

  UdpSocketPtr_S GetAnyUdpSocket();
  bool RemoteClient_New_ToClientUdpSocket(RemoteClient_S* rc);

  void GetP2PGroups(P2PGroupInfos& out_result);
  int32 GetP2PGroupCount();

  void OnSocketWarning(InternalSocket* socket, const String& text);
  void OnCompletionPortWarning(CompletionPort* port, const char* text) {}

  bool IsValidHostId(HostId host_id);
  bool IsValidHostId_NOLOCK(HostId host_id);

  void EnqueueClientLeaveEvent(RemoteClient_S* rc, ResultCode result_code,
                               ResultCode detail_code, const ByteArray& comment,
                               SocketErrorCode socket_error);
  void ProcessOneLocalEvent(LocalEvent& event);

 public:
  NetServerImpl();
  virtual ~NetServerImpl();

 private:
  ITaskSubject* GetTaskSubjectByHostId_NOLOCK(HostId subject_host_id);

 public:
  void AssertIsLockedByCurrentThread() {
    GetMutex().AssertIsLockedByCurrentThread();
  }

  void AssertIsNotLockedByCurrentThread() {
    GetMutex().AssertIsNotLockedByCurrentThread();
  }

 private:
  void CheckCriticalSectionDeadLock_INTERNAL(const char* where);

 public:
  void ProcessOnClientDisposeCanSafe(RemoteClient_S* rc);

  INetCoreCallbacks* GetCallbacks_NOLOCK() override;

  SessionKey* GetCryptSessionKey(HostId remote_id, String& out_error);

  bool NextEncryptCount(HostId remote_id, CryptoCountType& out_count);
  void PrevEncryptCount(HostId remote_id);
  bool GetExpectedDecryptCount(HostId remote_id, CryptoCountType& out_count);
  bool NextDecryptCount(HostId remote_id);

  virtual bool Send(const SendFragRefs& data_to_send,
                    const SendOption& send_opt, const HostId* sendto_list,
                    int32 sendto_count) override;

 public:
  bool Send_BroadcastLayer(const SendFragRefs& payload,
                           const SendOption& send_opt,
                           const HostId* sendto_list, int32 sendto_count);

  // TODO 함수명, 파라메터 이름을 변경하도록 하자.
  void ConvertGroupToIndividualsAndUnion(int32 sendto_count,
                                         const HostId* sendto_list,
                                         HostIdArray& send_dest_list);

  bool CloseConnection(HostId client_id) override;
  void CloseAllConnections() override;
  void RequestAutoPrune(RemoteClient_S* rc);

  bool SetHostTag(HostId host_id, void* host_tag) override;
  void* GetHostTag_NOLOCK(HostId host_id);

  RemoteClient_S* GetRemoteClientByHostId_NOLOCK(HostId client_id);
  RemoteClient_S* GetAuthedClientByHostId_NOLOCK(HostId client_id);
  bool IsDisposeRemoteClient_NOLOCK(RemoteClient_S* rc);

  double GetLastPingSec(HostId peer_id);
  double GetRecentPingSec(HostId peer_id);
  double GetP2PRecentPing(HostId host_a, HostId host_b) override;

  bool DestroyP2PGroup(HostId group_id);

  bool LeaveP2PGroup(HostId member_id, HostId group_id);

  HostId CreateP2PGroup(const HostId* member_list, int32 member_count,
                        const ByteArray& custom_field,
                        const P2PGroupOption& option, HostId assigned_host_id);
  bool JoinP2PGroup(HostId member_id, HostId group_id,
                    const ByteArray& custom_field);

  void EnqueueP2PAddMemberAckCompleteEvent(HostId group_id,
                                           HostId added_member_host_id,
                                           ResultCode result);

 private:
  uint32 joined_p2p_group_key_gen_;

  bool JoinP2PGroup_INTERNAL(HostId member_id, HostId group_id,
                             const ByteArray& custom_field,
                             uint32 joined_p2p_group_key_gen_);
  void AddMemberAckWaiters_RemoveRelated_MayTriggerJoinP2PMemberCompleteEvent(
      P2PGroup_S* group, HostId member_id, ResultCode reason);

 public:
  void P2PGroup_CheckConsistency();
  void P2PGroup_RefreshMostSuperPeerSuitableClientId(P2PGroup_S* group);
  void P2PGroup_RefreshMostSuperPeerSuitableClientId(RemoteClient_S* rc);
  void DestroyEmptyP2PGroups();

  int32 GetClientHostIds(HostId* output, int32 output_length);

  bool GetClientInfo(HostId client_id, NetClientInfo& out_result);

  P2PGroupPtr_S GetP2PGroupByHostId_NOLOCK(HostId group_id);
  bool GetP2PGroupInfo(HostId group_id, P2PGroupInfo& output);
  // TODO 함수명과 파라메터명 변경 검토.
  void ConvertAndAppendP2PGroupToPeerList(HostId send_to, HostIdArray& SendTo2);

  ISendDest_S* GetSendDestByHostId_NOLOCK(HostId peer_id);

  bool GetP2PConnectionStats(HostId remote_id, P2PConnectionStats& out_stats);
  bool GetP2PConnectionStats(HostId remote_a, HostId remote_b,
                             P2PPairConnectionStats& out_stats);

  // ITaskSubject interface
  HostId GetHostId() const override { return HostId_Server; }
  bool IsFinalReceiveQueueEmpty() override;
  bool IsTaskRunning() override;
  void OnSetTaskRunningFlag(bool running) override;
  bool PopFirstUserWorkItem(FinalUserWorkItem& out_item) override;

  void IssueDisposeRemoteClient(RemoteClient_S* rc, ResultCode result_code,
                                ResultCode detail_code,
                                const ByteArray& comment, const char* where,
                                SocketErrorCode socket_error);

  void RemoteClient_RemoveFromCollections(RemoteClient_S* rc);
  void DisposeIssuedRemoteClients();
  void HardDisconnect_AutoPruneGoesTooLongClient(RemoteClient_S* rc);
  void ElectSuperPeer();

  double GetTime();

  String DumpGroupStatus();

  int32 GetClientCount();
  void GetStats(NetServerStats& out_stats);

  inline void AttachProxy(RpcProxy* proxy) { NetCoreImpl::AttachProxy(proxy); }
  inline void AttachStub(RpcStub* stub) { NetCoreImpl::AttachStub(stub); }
  inline void DetachProxy(RpcProxy* proxy) { NetCoreImpl::DetachProxy(proxy); }
  inline void DetachStub(RpcStub* stub) { NetCoreImpl::DetachStub(stub); }

  void ShowError_NOLOCK(SharedPtr<ResultInfo> result_info);
  void ShowWarning_NOLOCK(SharedPtr<ResultInfo> result_info);

  void ShowNotImplementedRpcWarning(RpcId rpc_id,
                                    const char* rpc_name) override {
    NetCoreImpl::ShowNotImplementedRpcWarning(rpc_id, rpc_name);
  }

  void PostCheckReadMessage(IMessageIn& msg, RpcId rpc_id,
                            const char* rpc_name) override {
    NetCoreImpl::PostCheckReadMessage(msg, rpc_id, rpc_name);
  }

  NamedInetAddress GetRemoteIdentifiableLocalAddr();

  void SetDefaultTimeoutTimeSec(double timeout_sec);
  void SetDefaultTimeoutTimeMilisec(uint32 timeout_msec);

  bool SetDirectP2PStartCondition(DirectP2PStartCondition condition);

  // 너무 오랫동안 쓰이지 않은 UDP 송신 큐를 찾아서 제거한다.
  void TcpAndUdp_LongTick();

  // void DisconnectRemoteClientOnTimeout();
  void Heartbeat_PerClient();

  void GetUserWorkerThreadInfo(Array<ThreadInfo>& output);
  void GetNetWorkerThreadInfo(Array<ThreadInfo>& output);

  // 스레드 섞기
  void OnSocketIoCompletion(Array<IHostObject*>& send_issued_pool,
                            ReceivedMessageList& msg_list,
                            CompletionStatus& completion);
  void OnIoCompletion(Array<IHostObject*>& send_issued_pool,
                      ReceivedMessageList& msg_list,
                      CompletionStatus& completion);

  bool RunAsync(HostId task_owner_id, Function<void()> user_func) override;

 private:
  void ConditionalFallbackServerUdpToTcp(RemoteClient_S* rc,
                                         double absolute_time);
  void ConditionalArbitaryUdpTouch(RemoteClient_S* rc, double absolute_time);
  void RefreshSendQueuedAmountStat(RemoteClient_S* rc);

 public:
  // void ConditionalPruneTooOldDefragBoard();
  void LocalProcessForFallbackUdpToTcp(RemoteClient_S* rc);

  // NetServer interface
  InetAddress GetTcpListenerLocalAddr() override;

  virtual void GetUdpListenerLocalAddrs(Array<InetAddress>& output) override;

  // 정렬을 하기위해 P2PConnectionState 과 배열의 index와 directP2P Count로
  // 구성된 구조체 리스트를 만든다.
  struct ConnectionInfo {
    P2PConnectionState* state;
    int32 host_index0;
    int32 host_index1;
    int32 connect_count0;
    int32 connect_count1;

    ConnectionInfo() { Reset(); }

    bool operator<(const ConnectionInfo& other) const {
      return state->recent_ping_ < other.state->recent_ping_;
    }

    bool operator<=(const ConnectionInfo& other) const {
      return state->recent_ping_ <= other.state->recent_ping_;
    }

    bool operator>(const ConnectionInfo& other) const {
      return state->recent_ping_ > other.state->recent_ping_;
    }

    bool operator>=(const ConnectionInfo& other) const {
      return state->recent_ping_ >= other.state->recent_ping_;
    }

    void Reset() {
      state = nullptr;
      host_index0 = 0;
      host_index1 = 0;
      connect_count0 = 0;
      connect_count1 = 0;
    }
  };

 private:
  Array<ConnectionInfo> connection_info_list_;
  Array<int32> router_index_list_;

 public:
  void RemoteClient_RequestStartServerHolepunch_OnFirst(RemoteClient_S* rc);
  void RemoteClient_NewLocalUdpSocketAndRequestNewRemoteUdpSocket(
      RemoteClient_S* rc);

  void MakeP2PRouteLinks(SendDestInfoList_S& tgt,
                         int32 unreliable_s2c_routed_multicast_max_count,
                         double routed_send_max_ping);

  void Convert_NOLOCK(SendDestInfoList_S& to, HostIdArray& from);

  virtual void SetDefaultFallbackMethod(
      FallbackMethod fallback_method) override;

  UniquePtr<LogWriter> intra_logger_;

  virtual void EnableIntraLogging(const char* log_filename) override;
  virtual void DisableIntraLogging() override;

  void EnqueueHackSuspectEvent(RemoteClient_S* rc, const char* statement,
                               HackType hack_type);
  void EnqueueP2PGroupRemoveEvent(HostId group_id);

  // double GetSpeedHackLongDetectMinInterval();
  // double GetSpeedHackLongDeviationRatio();
  // double GetSpeedHackDeviationThreshold();
  // int32 GetSpeedHackDetectorSeriesLength();

  double speed_hack_detector_reck_ratio_;

  virtual void SetSpeedHackDetectorReckRatio(double ratio) override;
  virtual bool EnableSpeedHackDetector(HostId client_id, bool enable) override;

  virtual void SetMessageMaxLength(int32 MaxLength) override;

  virtual bool IsConnectedClient(HostId client_id) override;

  virtual void SetMaxDirectP2PConnectionCount(HostId client_id,
                                              int32 max_count) override;
  virtual HostId GetMostSuitableSuperPeerInGroup(
      HostId group_id, const SuperPeerSelectionPolicy& policy,
      const Array<HostId>& excludees) override;
  virtual HostId GetMostSuitableSuperPeerInGroup(
      HostId group_id, const SuperPeerSelectionPolicy& policy,
      HostId excludee) override;

  virtual int32 GetSuitableSuperPeerRankListInGroup(
      HostId group_id, super_peer_rating_* ratings, int32 ratings_buffer_count,
      const SuperPeerSelectionPolicy& policy,
      const Array<HostId>& excludees) override;

 private:
  void TouchSuitableSuperPeerCalcRequest(
      P2PGroup_S* group, const SuperPeerSelectionPolicy& policy);

 public:
  virtual void GetUdpSocketAddrList(Array<InetAddress>& output) override;

  virtual uint32 GetInternalVersion() override;

  //@maxidea: 제거 대상
  // void EnqueueUnitTestFailEvent(const String& text);

  void EnqueueError(SharedPtr<ResultInfo> result_info);
  void EnqueueWarning(SharedPtr<ResultInfo> result_info);

  void EnqueueLocalEvent(LocalEvent& Event);

  void EnqueueClientJoinApproveDetermine(const InetAddress& client_tcp_addr,
                                         const ByteArray& request);
  void ProcessOnClientJoinApproved(RemoteClient_S* rc,
                                   const ByteArray& response);
  void ProcessOnClientJoinRejected(RemoteClient_S* rc,
                                   const ByteArray& response);

 private:
  // true시 빈 P2P 그룹도 허용함
  bool empty_p2p_group_allowed_;
  bool start_create_p2p_group_;

 public:
  virtual void AllowEmptyP2PGroup(bool allow) override;

 public:
  void EnqueuePacketDefragWarning(const InetAddress& sender, const char* text);

 public:
  HostId GetSrcHostIdByAddrAtDestSide_NOLOCK(const InetAddress& addr);

 private:
  int32 GetMessageMaxLength();
  HostId GetLocalHostId();
  virtual bool IsNagleAlgorithmEnabled() override;

  // double last_prune_too_old_defragger_time_;

  int32 freq_fail_log_most_rank_;
  String freq_fail_log_most_rank_text_;
  double last_holepunch_freq_fail_logged_time_;
  AtomicFlag free_fail_need_;

 public:
  // 종료시에만 설정됨
  // Stop()이 return하기 전까지는 이벤트 콜백이 있어야하므로 삭제한다.
  // FUN_ALIGNED_VOLATILE bool tear_down_;

  bool SendFreeform(const HostId* sendto_list, int32 sendto_count,
                    const RpcCallOption& rpc_call_opt, const uint8* payload,
                    int32 payload_length) {
    return NetCoreImpl::SendFreeform(sendto_list, sendto_count, rpc_call_opt,
                                     payload, payload_length);
  }

 public:
  void ConditionalLogFreqFail();
  void LogFreqFailNow();

  //@maxidea: 제거 대상
  virtual void TEST_SetOverSendSuspectingThresholdInBytes(int32 threshold);
  //@maxidea: 제거 대상
  virtual void TEST_SetTestping(HostId host_id0, HostId host_id1, double ping);

  ServerUdpAssignMode udp_assign_mode_;

  // 미사용중인 고정 UDP 포트 목록. per-rc UDP 모드인 경우 사용됨.
  Array<int32> free_udp_ports_;

  void EnableVizAgent(const char* viz_server_ip, int32 viz_server_port,
                      const String& login_key) override;

 private:
  virtual NetClientImpl* QueryNetClient() override { return nullptr; }
  virtual NetServerImpl* QueryNetServer() override { return this; }

  virtual void Viz_NotifySendByProxy(
      const HostId* sendto_list, int32 sendto_count,
      const MessageSummary& summary,
      const RpcCallOption& rpc_call_opt) override;
  virtual void Viz_NotifyRecvToStub(HostId rpc_recvfrom, RpcId rpc_id,
                                    const char* rpc_name,
                                    const char* params_as_string) override;
};

}  // namespace fun
}  // namespace fun
