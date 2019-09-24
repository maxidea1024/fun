#pragma once

#include "IVizAgentDelegate.h"
#include "Networker_C.h"

#include "FallbackableUdpTransport_C.h"
#include "remote_peer.h"

#include "PacketFrag.h"
#include "Relayer.h"  //RelayDestList
#include "TcpTransport_C.h"

#include "Apps/EmergencyLogData.h"

#include "GeneratedRPCs/net_NetC2C_proxy.h"
#include "GeneratedRPCs/net_NetC2C_stub.h"
#include "GeneratedRPCs/net_NetC2S_proxy.h"
#include "GeneratedRPCs/net_NetC2S_stub.h"
#include "GeneratedRPCs/net_NetS2C_proxy.h"
#include "GeneratedRPCs/net_NetS2C_stub.h"

namespace fun {
namespace net {

class NetClientWorker;
class UdpSocket_C;
class TcpTransport_C;
class RemotePeer_C;
class MessageSummary;
class upnp_tcp_port_mapping_state_;
class NetServerImpl;

class NetClientImpl : public NetCoreImpl,
                      public ISendDest_C,
                      public IP2PGroupMember,
                      public NetClient,
                      public ITcpTransportOwner_C,
                      public IUdpPacketDefraggerDelegate,
                      public IUdpPacketFraggerDelegate,
                      public IVizAgentDelegate {
 private:
  IntervalAlaram reliable_ping_alarm_;

  // TcpAndUdp_ShortTick 실행 주기를 위해
  double tcp_and_udp_short_tick_last_time_;

 public:
  SessionKey self_p2p_session_key_;

 private:
  CryptoCountType self_encrypt_count_;
  CryptoCountType self_decrypt_count_;

 public:
  InetAddress Get_ToServerUdpSocketAddrAtServer();
  InetAddress Get_ToServerUdpSocketLocalAddr();

  InetAddress GetTcpLocalAddr();
  InetAddress GetUdpLocalAddr();

  virtual CCriticalSection2& GetMutex() override;
  void CheckCriticalSectionDeadLock_INTERNAL(const char* where);

  // true인 경우 연결 해제시 연이어 도착하는 여러가지 연결 해제 이벤트를
  // 잠재운다. 서버와의 연결이 TCP, UDP 등이 있기 때문에 이렇게 해주는거다.
  bool suppress_subsequent_disconnection_events_;

  // RequestServerTime을 호출한 횟수
  int32 request_server_time_count_;

  bool first_recv_disregarded_;
  int32 first_disregard_offset_;

 public:
  // 서버 컴퓨터의 시간.
  double dx_server_time_diff_;

  // 서버 컴퓨터에 대한 레이턴시. TCP fallback mode인 경우 TCP로 핑퐁한 결과가
  // 들어간다. round trip의 반이다.
  double server_udp_recent_ping_;
  double server_udp_last_ping_;

  // 서버에게 보낸 udp packet count
  int32 to_server_udp_send_count_;
  double last_report_udp_count_time_;

  int32 io_pending_count_;

 private:
  // Connect, Disconnect 과정만을 보호하는 객체
  // - CS보다 우선 잠금을 요한다.
  // - 이게 없으면 Disconnect 과정중 다른 스레드에서 Connect를 할 때 race
  // condition이 발생한다. 내부적으로 Connect와 Disconnect는 CS를 1회 이상
  // Unlock하기 때문이다.
  CCriticalSection2 connect_disconnect_phase_mutex_;
  AtomicCounter disconnection_invoke_count_;
  AtomicCounter connect_count_;

  // RequestServerTime을 마지막으로 보낸 시간
  double last_request_server_time_time_;

 public:
  // 이 객체가 존재하는 경우 서버와 연결 시도중, 연결된 상태, 연결 해제중임을
  // 의미.
  NetClientWorkerPtr worker_;
  Singleton<NetClientManager>::CPtr manager_;

 private:
  P2PGroups_C p2p_groups_;

 public:
  /** 그룹 내의 모든 피어에서 핑을 보내줄 때 사용되는 시간 */
  double enable_ping_test_end_time_;

  /** 동시 진행중인 p2p connection 시도 횟수에 따라 이 값들은 신축한다. */
  double p2p_connection_attempt_end_time_;

  /** 동시 진행중인 p2p connection 시도 횟수에 따라 이 값들은 신축한다. */
  double p2p_holepunch_interval_;

 private:
  double GetReliablePingTimerInterval();
  bool AsyncCallbackMayOccur() override;

  /**
   * P2PGroup_MemberJoin encrypt, unencrypt두가지로 나뉘게 되어 따로 함수로 만듬
   * 새로운 member가 들어와서 p2p group을 update한다.
   */
  void UpdateP2PGroup_MemberJoin(HostId group_id, HostId member_id,
                                 const ByteArray& custom_field, uint32 event_id,
                                 FrameNumber p2p_first_frame_number,
                                 const Uuid& connection_tag,
                                 const ByteArray& p2p_aes_session_key,
                                 const ByteArray& p2p_rc4_session_key,
                                 bool direct_p2p_enabled, int32 bind_port,
                                 bool real_udp_enabled);

 public:
  bool nat_device_name_detected_;

  // Emergency log 때문에 하나 백업을 받아놓자.
  NetClientStats recent_backedup_stats_;
  NetClientStats stats_;

  uint32 internal_version_;

  NetSettings settings_;

  // 서버 인스턴스의 GUID
  // 서버 연결 성공시 발급받는다.
  Uuid server_instance_tag_;

  List<ReceivedMessage> pre_final_recv_queue;  // Requires CS lock before access

  CryptoCountType to_server_encrypt_count_;
  CryptoCountType to_server_decrypt_count_;
  SessionKey to_server_session_key_;

  RandomMT random_;

  IHasOverlappedIoPtr to_server_udp_socket_;
  bool to_server_udp_socket_failed_;
  inline UdpSocket_C* Get_ToServerUdpSocket();

  // 사용자가 지정한 udp 포트 리스트
  Set<int32> unused_udp_ports_;
  // 이미 사용한 udp 포트 리스트
  Set<int32> used_udp_ports_;

  // 자기자신에게 송신한 메시지. 아직 user worker로 넘어간건 아니다.
  List<MessageIn> lookback_final_received_message_queue_;

  // Server도 P2P그룹에 넣기위해 IP2PGroupMember를 상속한다
  class ServerAsSendDest : public ISendDest_C, public IP2PGroupMember {
   public:
    NetClientImpl* owner_;
    void* host_tag_;

    ServerAsSendDest(NetClientImpl* owner)
        : owner_(owner), host_tag_(nullptr) {}

    // IP2PGroupMember interface
    HostId GetHostId() const override { return HostId_Server; }
    double GetIndirectServerTimeDiff() override {
      return owner_->GetIndirectServerTimeDiff();
    }
  };
  ServerAsSendDest server_as_send_dest_;

  FallbackableUdpTransportPtr_C to_server_udp_fallbackable_;

  //사용하지 않는 기능이므로 제거.
  // double min_extra_ping_;
  // double extra_ping_variance_;

  struct RelayDest_C {
    RemotePeer_C* remote_peer;
    FrameNumber frame_number;
  };

  struct RelayDestList_C : public Array<RelayDest_C> {
    void ToSerializable(RelayDestList& out_result);
  };

  // 패킷 최적화를 위해 unreliable relay를 할때 모든 HostId를 보내는것이 아니라
  // directp2p한 양이 적을경우 서버에 direct된 hostId를 보내 최적화한다. (부분
  // 집합이므로 subset이란 단어사용)
  struct P2PGroupSubset_C {
    /** 릴레이에서 제외되야할 host_id list */
    HostIdArray excludee_host_id_list;
  };

  struct CompressedRelayDestList_C {
    Map<HostId, P2PGroupSubset_C> p2p_group_list;
    /** 어떠한 p2p 그룹에도 등록되어 있지 않은 개별 host id list */
    HostIdArray includee_host_id_list;

    /**
     * 제외되어야할 host_id list에 추가한다.
     */
    void AddSubset(const HostIdArray& subset_group_host_id, HostId host_id);

    /**
     * individual list에 추가한다.
     */
    void AddIndividual(HostId host_id);

    /**
     * 모든 HostId의 갯수 (그룹 호스트ID까지 포함됨)
     */
    int32 GetAllHostIdCount();

    inline CompressedRelayDestList_C() {
      // 이 객체는 로컬 변수로 주로 사용되며, rehash가 일단 발생하면 대쪽
      // 느려진다. 따라서 충분한 hash table을 미리 준비해 두고 rehash를 못하게
      // 막아버리자.

      // TODO 이게참 거시기하구만... 처리를 해주긴 해야할듯...
      // p2p_group_list.InitHashTable(1627);
      // p2p_group_list.DisableAutoRehash();
    }
  };

  double GetElapsedTime();
  double GetAbsoluteTime();

  /* 이 클라이언트의 식별 ID */
  HostId local_host_id_;

  /**
   * emergency logging 처리시 사용함. 접속해제시 local_host_id_가 HostID_None이
   * 되어버려서 참조하지 못하므로, 백업을 해둠.
   * 하지만, 이게 꼭 필요한지는 조금더 생각해볼 필요가 있을듯 싶다.
   */
  HostId backuped_host_id_;

  /**
   * 잠긴 상태(CS locked)에서는 직접적으로 callback을 호출해서는 안된다.
   * 잠긴 상태에서 callback을 신속히 처리하지 못할 경우를
   * 내부처리에 지연이 발생하고, 그에 따른 문제가 발생할 수 있다.
   * 따라서 잠긴 상태에서는 큐에 이벤트를 큐잉하고, 잠긴이 해제된 상태에서
   * 큐에 대기중인 callback들을 호출하도록 한다.
   */
  INetClientCallbacks* callbacks_;

  NetConnectionArgs connection_args_;

  IHasOverlappedIoPtr to_server_tcp_;
  TcpTransport_C* Get_ToServerTcp();

  // 유저 콜백으로 쓰이는 최종 수신된 메시지 또는 로컬 이벤트 등
  final_user_work_queue_ final_user_work_queue_;
  final_user_work_queue_ postponed_final_user_work_item_list_;

  /**
   * 각 클라들이 공용으로 쓰는 타이머보다는 정밀한 결과를 주는 타이머.
   * 이름을 변경해주는게 바람직할듯 싶다.
   *
   * GetSteppedTime() / GetSteppedElapsedTime()
   * GetRealtime()
   *
   * MorePrecisionClock은 GetRealtime()에 해당하는 형태로...
   *
   * 현재 내부에서 시간 측정시에, SteppedTime인지 Realtime인지 구분이 애매한
   * 부분이 있다. 전반적으로 확인 후 확실하게 적용을 해야한다.
   */
  Clock more_precision_clock_;

  /**
   * NetClientManager::GetCachedAbsoluteTime() 보다 조금더 정밀한 시간 구하기.
   *
   * NetClientImpl::GetAbsoluteTime()과 살짝 헷갈리는데...
   * 좀더 명확히 할 수는 없으려나??
   */
  double GetMorePrecisionAbsoluteTime() const {
    return more_precision_clock_.AbsoluteSeconds();
  }

  /**
   * 가장 최근에 Tick를 호출한 시간
   * 0이면 한번도 호출 안했음, -1이면 경고를 더 이상 낼 필요가 없음.
   */
  double last_tick_invoked_time_;

  RemotePeers_C remote_peers_;

  Map<HostId, RemotePeerPtr_C> peer_garbages_;

  /**
   * 더 이상 안쓰이는 소켓들.  i/o completion 및 이슈 더 이상 없음
   * 보장시 파괴될 것들이다.
   */
  List<IHasOverlappedIoPtr> garbages_;

  /**
   * P2P 최근 홀펀칭 재사용을 위해 다시 활용될 수 있음
   */
  Map<int32, IHasOverlappedIoPtr> recycles_;

  IntervalAlaram conditional_remove_too_old_udp_send_packet_queue_alarm_;
  bool intra_logging_on_;

  inline bool IsIntraLoggingOn() const {
    return intra_logging_on_ || settings_.emergency_log_line_count > 0;
  }

  /**
   * 가상 스피드핵. 얼마나 자주 ping 메시지를 보내느냐를 조작한다.
   * 1이면 스핵을 안쓴다는 의미다.
   */
  double virtual_speed_hack_multiplication_;

  double last_check_send_queue_time_;
  double send_queue_heavy_started_time_;

  // Emergency logging
 public:
  EmergencyLogData emergency_log_data_;
  int32 total_tcp_issued_send_bytes_;
  void UpdateNetClientStatClone();
  bool SendEmergencyLogData(const String& server_ip,
                            int32 server_port);  // on-demand manner
  void AddEmergencyLogList(LogCategory category, const String& text);

 private:
  double last_update_net_client_stat_clone_time_;
  // static DWORD WINAPI RunEmergencyLogClient(void* context);
  bool GetIntersectionOfHostIdListAndP2PGroupsOfRemotePeer(
      const HostIdArray& sorted_host_id_list, RemotePeer_C* rp,
      HostIdArray* out_subset_group_host_id_list);

 public:
  struct C2CProxy : public NetC2C::Proxy {};
  struct C2SProxy : public NetC2S::Proxy {};

  struct S2CStub : public NetS2C::Stub {
    NetClientImpl* owner_;

    DECLARE_RPCSTUB_NetS2C_P2PGroup_MemberJoin;
    DECLARE_RPCSTUB_NetS2C_P2PGroup_MemberJoin_Unencrypted;
    DECLARE_RPCSTUB_NetS2C_RequestP2PHolepunch;
    DECLARE_RPCSTUB_NetS2C_NotifyDirectP2PEstablish;
    DECLARE_RPCSTUB_NetS2C_P2PGroup_MemberLeave;
    DECLARE_RPCSTUB_NetS2C_P2P_NotifyDirectP2PDisconnected2;
    DECLARE_RPCSTUB_NetS2C_ReliablePong;
    DECLARE_RPCSTUB_NetS2C_EnableIntraLogging;
    DECLARE_RPCSTUB_NetS2C_DisableIntraLogging;
    DECLARE_RPCSTUB_NetS2C_NotifyUdpToTcpFallbackByServer;
    DECLARE_RPCSTUB_NetS2C_NotifySpeedHackDetectorEnabled;
    DECLARE_RPCSTUB_NetS2C_ShutdownTcpAck;
    DECLARE_RPCSTUB_NetS2C_RenewP2PConnectionState;
    DECLARE_RPCSTUB_NetS2C_NewDirectP2PConnection;
    DECLARE_RPCSTUB_NetS2C_RequestMeasureSendSpeed;
    DECLARE_RPCSTUB_NetS2C_RequestAutoPrune;
    DECLARE_RPCSTUB_NetS2C_S2C_RequestCreateUdpSocket;
    DECLARE_RPCSTUB_NetS2C_S2C_CreateUdpSocketAck;
    DECLARE_RPCSTUB_NetS2C_P2PRecycleComplete;
  };

  struct C2CStub : public NetC2C::Stub {
    NetClientImpl* owner_;

    DECLARE_RPCSTUB_NetC2C_SuppressP2PHolepunchTrial;
    DECLARE_RPCSTUB_NetC2C_ReportUdpMessageCount;
    DECLARE_RPCSTUB_NetC2C_ReportServerTimeAndFrameRateAndPing;
    DECLARE_RPCSTUB_NetC2C_ReportServerTimeAndFrameRateAndPong;
  };

  C2CProxy c2c_proxy_;
  C2CStub c2c_stub_;
  C2SProxy c2s_proxy_;
  S2CStub s2c_stub_;

 public:
  NetClientImpl();

  bool Connect(const NetConnectionArgs& args);

  void ConditionalStartupUPnP();
  void ConditionalAddUPnPTcpPortMapping();
  void ConditionalDeleteUPnPTcpPortMapping();

 private:
  // 한개만 진행해야함. 이객체가 유효하면 즉,
  // upnp_tcp_port_mapping_state_.IsValid()이면 더 생성해서는 안됨. (이미
  // 진행중임을 나타냄)
  UniquePtr<upnp_tcp_port_mapping_state_> upnp_tcp_port_mapping_state_;

 public:
  // NetClient interface
  void SetCallbacks(INetClientCallbacks* callbacks) override;
  // NetCoreImpl interface
  INetCoreCallbacks* GetCallbacks_NOLOCK() override;

  // NetClient interface
  void Disconnect() override;
  // NetClient interface
  void Disconnect(const DisconnectArgs& args) override;

  void ExtractMessagesFromUdpRecvQueue(const uint8* udp_packet,
                                       int32 udp_packet_length,
                                       const InetAddress& remote_addr,
                                       ReceivedMessageList& out_result,
                                       ResultCode& out_error);
  void LogLastServerUdpPacketReceived();
  bool ExtractMessagesFromTcpStream(ReceivedMessageList& out_result);
  void EnqueueLocalEvent(LocalEvent& event);
  void EnqueueDisconnectionEvent(ResultCode result_code,
                                 ResultCode detail_code = ResultCode::Ok,
                                 const String& comment = String());
  void EnqueueConnectFailEvent(
      ResultCode result_code,
      SocketErrorCode socket_error = SocketErrorCode::Ok);
  void EnqueueConnectFailEvent(ResultCode result_code,
                               SharedPtr<ResultInfo> result_info);
  void EnqueueFallbackP2PToRelayEvent(HostId remote_peer_id, ResultCode reason);
  void SetInitialTcpSocketParameters();

  void FinalUserWorkItemList_RemoveReceivedMessagesOnly();

 private:
  void RequestServerUdpSocketReady_FirstTimeOnly();
  bool New_ToServerUdpSocket();

 public:
  // void WaitForGarbageCleanup();
  void CleanupEvenUnstableSituation(bool clear_work_items = true);
  void DoGarbageCollection();

  virtual ~NetClientImpl();

  // FunNet.NetConfig.EveryRemoteIssueSendOnNeedInterval이지만 테스트를 위해
  // 값을 수정할 경우를 위해 멤버로 떼놨다.
  double every_remote_issue_send_on_need_interval_sec_;

  void EveryRemote_IssueSendOnNeed();

  void OnSocketWarning(InternalSocket* socket, const String& msg);

  bool GetP2PGroupByHostId(HostId group_id, P2PGroupInfo& out_group_info);

  P2PGroupPtr_C GetP2PGroupByHostId_INTERNAL(HostId group_id);

  bool Send(const SendFragRefs& data_to_send, const SendOption& send_opt,
            const HostId* sendto_list, int32 sendto_count) override;

  bool Send_BroadcastLayer(const SendFragRefs& payload,
                           const SendOption& send_opt,
                           const HostId* sendto_list,
                           int32 sendto_count) override;

  void Send_ToServer_Directly_Copy(HostId dest_id,
                                   MessageReliability reliability,
                                   const SendFragRefs& data_to_send,
                                   const UdpSendOption& send_opt);

  // typedef Array<ISendDest_C*,
  // InlineAllocator<NetConfig::OrdinaryHeavyS2CMulticastCount>> ISendDestList_C;
  typedef Array<ISendDest_C*> ISendDestList_C;
  void ConvertGroupToIndividualsAndUnion(int32 sendto_count,
                                         const HostId* sendto_list,
                                         ISendDestList_C& send_dest_list);
  void ConvertGroupToIndividualsAndUnion(int32 sendto_count,
                                         const HostId* sendto_list,
                                         HostIdArray& output);
  // bool Send(SendFragRefs& data_to_send, MessagePriority priority,
  // MessageReliability reliability, char ordered_channel, HostId send_to)
  // override;

  bool ConvertAndAppendP2PGroupToPeerList(HostId send_to,
                                          ISendDestList_C& send_dest_list);

  ISendDest_C* GetSendDestByHostId(HostId peer_id);

  void* host_tag_;

  // NetClient interface
  bool SetHostTag(HostId host_id, void* host_tag) override;
  void* GetHostTag(HostId host_id);

  RemotePeerPtr_C GetPeerByHostId(HostId peer_id);
  InetAddress GetLocalUdpSocketAddr(HostId remote_peer_id);

  SessionKey* GetCryptSessionKey(HostId remote, String& out_error);
  bool NextEncryptCount(HostId remote, CryptoCountType& out_count);
  void PrevEncryptCount(HostId remote);
  bool GetExpectedDecryptCount(HostId remote, CryptoCountType& out_count);
  bool NextDecryptCount(HostId remote);

  RemotePeerPtr_C GetPeerByUdpAddr(const InetAddress& udp_addr);

  void SendServerHolepunch();

  void Tick(TickResult* out_result = nullptr);

  void Tick_PullPostponeeToFinalQueue();
  void ConditionalSendServerHolePunch();
  void ConditionalReportP2PPeerPing();
  void ReportRealUdpCount();

  void GetStats(NetClientStats& out_stats);

 private:
  void Tick_FinalUserWorkItem(TickResult* out_result = nullptr);
  bool PopFinalUserWorkItem(FinalUserWorkItem& out_item);
  void DoOneUserWorkItem(FinalUserWorkItem& uwi,
                         bool& out_holster_more_callback,
                         bool& out_postpone_this_callback,
                         int32& RpcProcessedCount, int32& EventProcessedCount);
  void PostponeFinalUserWorlItem(FinalUserWorkItem& uwi);
  ApplicationHint application_hint_;

 public:
  // NetClient interface
  void SetApplicationHint(const ApplicationHint& hint) override;
  void GetApplicationHint(ApplicationHint& out_hint);

  // RpcHost interface
  HostId GetLocalHostId() override;

  // NetClient interface (폐기하고, GetClientInfo로 바꿔야하나?)
  bool GetPeerInfo(HostId remote_id, PeerInfo& out_info) override;
  // NetClient interface
  void GetLocalJoinedP2PGroups(HostIdArray& output) override;

#ifdef DEPRECATE_SIMLAG
  void SimulateBadTraffic(int32 min_extra_ping, int32 extra_ping_variance_);
#endif

  // NetClient interface
  ConnectionState GetServerConnectionState() override;
  // NetClient interface
  ConnectionState GetServerConnectionState(
      ServerConnectionState& out_state) override;

  // NetClient interface
  void GetWorkerState(ClientWorkerInfo& out_info) override;

  TestStats2 TestStats2;
  // NetClient interface
  void TEST_GetTestStats(TestStats2& out_stats) override;

  // ITcpTransportOwner_C interface
  void LockMain_AssertIsLockedByCurrentThread() override {
    GetMutex().AssertIsLockedByCurrentThread();
  }

  void LockMain_AssertIsNotLockedByCurrentThread() override {
    GetMutex().AssertIsNotLockedByCurrentThread();
  }

  //현재 사용되지 않고 있음. 폐기 여부 검토.
  void LockRemote_AssertIsLockedByCurrentThread() {
    // NetClient에서는 main_ 및 remote lock의 구분이 없다. 따라서 Main의
    // Assert와 같다.
    GetMutex().AssertIsLockedByCurrentThread();
  }

  //현재 사용되지 않고 있음. 폐기 여부 검토.
  void LockRemote_AssertIsNotLockedByCurrentThread() {
    // NetClient에서는 main_ 및 remote lock의 구분이 없다. 따라서 Main의
    // Assert와 같다.
    GetMutex().AssertIsNotLockedByCurrentThread();
  }

  P2PGroupPtr_C CreateP2PGroupObject_INTERNAL(HostId group_id);

  // 일정 시간이 됐으면 서버에 서버 시간을 요청한다.
  void ConditionalRequestServerTime();
  void ConditionalSpeedHackPing();
  void CheckSendQueue();

  double GetServerTimeDiff() { return dx_server_time_diff_; }

  double GetIndirectServerTime(HostId peer_id);

  double GetServerTime();

  void ConditionalSyncIndirectServerTime();

  // String GetGroupStatus()
  //{
  //  String Ret = TEXT("<not yet>");
  //  //
  //  //{
  //  //CScopedLock2 Lock(m_critSecPtr, true);
  //  //    // //FUN_TRACE를 안쓰고 1회의 OutputDebugString으로 때려버린다.
  //  //    // 이러면 여러 프로세스의 trace line이 겹쳐서 나오는 문제가
  //  해결될라나?
  //
  //  //    String a;
  //  //    ret += "=======================\n";
  //  //    //ret+="======================="; a.Format(" --
  //  Time=%s\n",CT2A(DateTime::Now().Format())); ret+=a;
  //
  //  //    foreach (RemotePeerPtr_C p in remote_peers_.Values)
  //  //    {
  //  //        a.Format("*    peer=%d(%s)\n", p->host_id_,
  //  ToString(p->ExternalID));
  //  //        ret += a;
  //
  //  //        a.Format("*        indirect server time diff=%f\n",
  //  p->indirect_server_time_diff_);
  //  //        ret += a;
  //
  //  //        for (HostIdSet::iterator j = p->joined_p2p_groups_.begin(); j !=
  //  p->joined_p2p_groups_.end(); j++)
  //  //        {
  //  //            a.Format("*        joined P2P group=%d\n", *j);
  //  //            ret += a;
  //  //        }
  //  //    }
  //
  //  //    for (P2PGroupInfos::iterator i = p2p_groups_.begin(); i !=
  //  p2p_groups_.end(); i++)
  //  //    {
  //  //        P2PGroupInfoPtr GP = i->second;
  //  //        a.Format("*    P2PGroup=%d\n", GP->group_id_);
  //  //        ret += a;
  //  //        for (HostIdSet::iterator j = GP->members_.begin(); j !=
  //  GP->members_.end(); j++)
  //  //        {
  //  //            a.Format("*        member=%d\n", *j);
  //  //            ret += a;
  //  //        }
  //  //    }
  //  //    a.Format("=======================\n");
  //  //    ret += a;
  //  //}
  //  return Ret;
  //}

  void ConditionalFallbackServerUdpToTcp();
  void FirstChanceFallbackServerUdpToTcp(ResultCode reason);

  void FirstChanceFallbackEveryPeerUdpToTcp(ResultCode reason);
  double GetP2PServerTime(HostId group_id);

  void RemoveRemotePeerIfNoGroupRelationDetected(RemotePeerPtr_C member_rp);

  HostId GetHostId() const override { return local_host_id_; }

  InetAddress GetServerAddress();

  double GetIndirectServerTimeDiff() { return dx_server_time_diff_; }

  void TEST_FallbackUdpToTcp(FallbackMethod fallback_method);

  bool RestoreUdpSocket(HostId peer_id);
  bool InvalidateUdpSocket(HostId peer_id, DirectP2PInfo& out_info);

  bool GetDirectP2PInfo(HostId remote_peer_id,
                        DirectP2PInfo& out_info) override;

  String DumpGroupStatus();

  void GetGroupMembers(HostId group_id, HostIdArray& output);

  // Bug가 나올 가능성이 있어 기능 쓰지 않음.
  // double GetSimLag() {
  //  return min_extra_ping + random_.NextDouble() * extra_ping_variance_;
  //}

  void AttachProxy(RpcProxy* proxy) override {
    NetCoreImpl::AttachProxy(proxy);
  }
  void AttachStub(RpcStub* stub) override { NetCoreImpl::AttachStub(stub); }
  void DetachProxy(RpcProxy* proxy) override {
    NetCoreImpl::DetachProxy(proxy);
  }
  void DetachStub(RpcStub* stub) override { NetCoreImpl::DetachStub(stub); }

  String GetNatDeviceName() override;

  // CNetCoreImpl에 이미 구현되어 있는데, 왜 이래 해야 abstract가 아니라고
  // 나오는걸까??
  inline void ShowError_NOLOCK(SharedPtr<ResultInfo> result_info) {
    NetCoreImpl::ShowError_NOLOCK(result_info);
  }

  void EnqueueError(SharedPtr<ResultInfo> result_info);
  void EnqueueWarning(SharedPtr<ResultInfo> result_info);

  // CNetCoreImpl에 이미 구현되어 있는데, 왜 이래 해야 abstract가 아니라고
  // 나오는걸까??
  void ShowNotImplementedRpcWarning(RpcId rpc_id,
                                    const char* rpc_name) override {
    NetCoreImpl::ShowNotImplementedRpcWarning(rpc_id, rpc_name);
  }

  // CNetCoreImpl에 이미 구현되어 있는데, 왜 이래 해야 abstract가 아니라고
  // 나오는걸까??
  void PostCheckReadMessage(IMessageIn& msg, RpcId rpc_id,
                            const char* rpc_name) override {
    NetCoreImpl::PostCheckReadMessage(msg, rpc_id, rpc_name);
  }

  // NetClient interface
  double GetLastPingSec(HostId peer_id) override;
  double GetRecentPingSec(HostId peer_id) override;

  void TcpAndUdp_LongTick();

  // UdpPacketDefragger::LongTick() 안에서 PruneTooOldDefragBoard()을 호출하고
  // 있으므로, 이렇게 별도로 있을 필요는 없음. 차후에 코드정리시 폐기하도록 하자.
  // void ConditionalPruneTooOldDefragBoard();

  // 최초 메시지 send issue를 일괄 수행하는 타이머
  IntervalAlaram process_send_ready_remotes_alarm_;

  // 소켓을 닫아줌 -> 송/수신시 오류 발생 -> 접속종료처리 이런과정을 통해서
  // 자연스럽게 접속해제 처리를 할수 있도록 함.
  void InduceDisconnect();

  // NetClient interface
  bool GetPeerRUdpStats(HostId peer_id, RUdpHostStats& out_stats) override;

  void IntraLogToServer(LogCategory category, const char* text);
  void LogToServer_HolepunchFreqFail(int32 rank, const char* text);

  // NetClient interface
  // 개발시 테스트용도임. 폐기하거나, 직접적 사용이 불가한 상태로 만들자.
  // 원격으로 붙어서(일종의 비밀 리모컨?) 설정값을 받아서 모의시험을 하는 형태로
  // 처리한다면 엔진을 받아서 사용하는 프로그래머가 헷갈라지 않아도 되고
  // 그러할듯 싶은데...
  void TEST_EnableVirtualSpeedHack(double multiplied_speed);

  double speedhack_detect_ping_cooltime_;

  // NetClient interface
  bool IsLocalHostBehindNAT(bool& out_is_behind_nat) override;

  String GetTrafficStatText();
  virtual String TEST_GetDebugText();

  // worker thread에서 더 이상 사용하지 않음을 보장하는 변수

  // FUN_ALIGNED_VOLATILE bool TcpSendStopAcked,TcpRecvStopAcked;
  // FUN_ALIGNED_VOLATILE bool UdpSendStopAcked,UdpRecvStopAcked;
  // FUN_ALIGNED_VOLATILE bool HeartbeatStopAcked;

  // NetClient interface
  void GetNetWorkerThreadInfo(ThreadInfo& out_info) override;
  bool GetSocketInfo(HostId remote_id, SocketInfo& out_info) override;

  void RemovePeer(RemotePeerPtr_C rp);
  void DecreaseLeaveEventCount(HostId host_id);
  void GarbagePeer(RemotePeerPtr_C rp);
  void GarbageSocket(IHasOverlappedIoPtr socket);
  void UdpSocketToRecycleBin(IHasOverlappedIoPtr udp_socket);
  void AllClearRecycleToGarbage();
  void AllClearGarbagePeer();

  // 가장 마지막에 TCP 스트림을 받은 시간
  // 디스 여부를 감지하기 위함
  // 양쪽 호스트가 모두 감지해야 한다.
  // TCP는 shutdown 직후 closesocket을 호출해도 상대가 못 받을 가능성이 있기
  // 때문이다.
  FUN_ALIGNED_VOLATILE double last_tcp_stream_recv_time_;

  // double last_prune_too_old_defragger_time_;

  virtual double GetSendToServerSpeed();

  // NetClient interface
  uint32 GetInternalVersion() override;

  // IUdpPacketDefraggerDelegate interface
  void EnqueuePacketDefragWarning(const InetAddress& addr,
                                  const char* text) override;
  // NetCoreImpl interface
  int32 GetMessageMaxLength() override;
  //도대체 어디에 있는걸 재정의하는건가?
  //그냥 여기가 처음? 파악을 해보아야할듯...
  virtual HostId GetSrcHostIdByAddrAtDestSide_NOLOCK(const InetAddress& addr);

  // NetClient interface
  InetAddress GetPublicAddress() override;

  virtual void RequestReceiveSpeedAtReceiverSide_NoRelay(
      const InetAddress& dst);
  virtual int32 GetOverSendSuspectingThresholdInByte();

  // RpcHost interface
  void EnableVizAgent(const char* viz_server_ip, int32 viz_server_port,
                      const String& login_key) override;

  NetClientWorker* GetWorker() { return Worker.Get(); }
  void AssociateSocket(InternalSocket* socket);
  // void SendReadyInstances_Add();

  bool SendFreeform(const HostId* sendto_list, int32 sendto_count,
                    const RpcCallOption& rpc_call_opt, const uint8* payload,
                    int32 payload_length) {
    return NetCoreImpl::SendFreeform(sendto_list, sendto_count, rpc_call_opt,
                                     payload, payload_length);
  }

  bool CreateUdpSocket(UdpSocket_C* udp_socket,
                       InetAddress& ref_udp_local_addr);

  void SetDefaultTimeoutTimeSec(double timeout_sec);

#ifdef TEST_DISCONNECT_CLEANUP
  virtual bool IsAllCleanup();
#endif

  bool RunAsync(HostId task_owner_id, Function<void()> func) override;

  // IVizAgentDelegate interface
 private:
  virtual NetClientImpl* QueryNetClient() override { return this; }
  virtual NetServerImpl* QueryNetServer() override { return nullptr; }

  virtual void Viz_NotifySendByProxy(
      const HostId* sendto_list, int32 sendto_count,
      const MessageSummary& summary,
      const RpcCallOption& rpc_call_opt) override;
  virtual void Viz_NotifyRecvToStub(HostId sender, RpcId rpc_id,
                                    const char* rpc_name,
                                    const char* params_as_string) override;

  bool IsBehindNAT();
  // 주의: 새 멤버 추가시 NetClientManager에 의해 접근할 수 있으므로 dtor에서 CS
  // lock 필수! (primitive type은 예외)
};

}  // namespace net
}  // namespace fun
