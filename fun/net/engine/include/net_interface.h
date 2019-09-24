// TODO common/server/client로 각각 나누어서 빼주도록 하자.
// TODO SuperPeer지원은 없애는게 좋을듯 한데??
#pragma once

#include "INetClientCallbacks.h"
#include "INetServerCallbacks.h"
#include "fun/net/net.h"

namespace fun {
namespace net {

class MessageSummary;
class MessageIn;
class NetConnectionArgs;
class PeerInfo;
class NetClientInfo;
class P2PGroupInfo;
class P2PGroupInfos;
class SendFragRefs;
class ResultInfo;
class INetClientCallbacks;
class INetServerCallbacks;
class RpcProxy;
class RpcStub;
class RUdpHostStats;
class SendOption;

class TestStats;
class TestStats2;

class RpcHost {
 public:
  virtual ~RpcHost() {}

  virtual CCriticalSection2& GetMutex() = 0;

  /** Get the local host id. */
  virtual HostId GetLocalHostId() = 0;

  virtual void ConvertGroupToIndividualsAndUnion(int32 sendto_count,
                                                 const HostId* sendto_list,
                                                 HostIdArray& out_result) = 0;

  virtual void ShowError_NOLOCK(SharedPtr<ResultInfo> result_info) = 0;

  /** Running user functions asynchronously. (C ++ 11 lambda support) */
  virtual bool RunAsync(HostId task_owner_id, Function<void()> UserFunc) = 0;

  /** Attach a RPC proxy object. */
  virtual void AttachProxy(RpcProxy* proxy) = 0;
  /** Detach a RPC proxy object. */
  virtual void DetachProxy(RpcProxy* proxy) = 0;

  /** Attach a RPC stub object. */
  virtual void AttachStub(RpcStub* stub) = 0;
  /** Detach a RPC stub object. */
  virtual void DetachStub(RpcStub* stub) = 0;

  // TODO 아래의 함수들은 예외로 처리하고, 그냥 제거하는게 좋을듯함.  전반적으로
  // 상황을 살핀후 제거하도록 하자.

  virtual void ShowNotImplementedRpcWarning(RpcId rpc_id,
                                            const char* rpc_name) = 0;
  virtual void PostCheckReadMessage(IMessageIn& msg, RpcId rpc_id,
                                    const char* rpc_name) = 0;

  /**\name RPC proxy/stub tracking.
   * @{ */
  /** Called whenever the RPC proxy function is called. Can be used to debug
   * sending content. */
  virtual void NotifySendByProxy(HostId sendto, const MessageSummary& summary,
                                 const RpcCallOption& rpc_call_opt) {}

  /** Called whenever RPC stub function is called. Can be used to debug incoming
   * content. */
  virtual void NotifyCallFromStub(RpcId rpc_id, const String& rpc_name,
                                  const String& params_as_string) {}
  /** Called just before calling the RPC stub function. */
  virtual void BeforeRpcInvocation(const BeforeRpcSummary& summary) {}
  /** Called immediately after the RPC stub function is called. (You can check
   * the time required.) */
  virtual void AfterRpcInvocation(const AfterRpcSummary& summary) {}
  /** @} */

  /**\name Visuallier
   * @{ */
  virtual void EnableVizAgent(const char* viz_server_ip, int32 viz_server_port,
                              const String& login_key) = 0;

  virtual void Viz_NotifySendByProxy(const HostId* rpc_sendto_list,
                                     int32 rpc_sendto_count,
                                     const MessageSummary& summary,
                                     const RpcCallOption& rpc_call_opt) = 0;

  virtual void Viz_NotifyRecvToStub(HostId rpc_recvfrom, RpcId rpc_id,
                                    const char* rpc_name,
                                    const char* params_as_string) = 0;
  /** @} */

 protected:
  friend class RpcProxy;

  virtual bool Send(const SendFragRefs& send_data, const SendOption& send_opt,
                    const HostId* sendto_list, int32 sendto_count) = 0;
};

class NetServerStats {
 public:
  uint64 total_tcp_recv_count;
  uint64 total_tcp_recv_bytes;
  uint64 total_tcp_send_count;
  uint64 total_tcp_send_bytes;
  uint64 total_udp_send_count;
  uint64 total_udp_send_bytes;
  uint64 total_udp_recv_count;
  uint64 total_udp_recv_bytes;

  int32 p2p_group_count;
  int32 p2p_connection_pair_count;
  int32 p2p_direct_connection_pair_count;
  int32 client_count;
  int32 real_udp_enabled_client_count;
  int32 occupied_udp_port_count;

  inline uint64 GetTotalSendCount() const {
    return total_tcp_send_count + total_udp_send_count;
  }
  inline uint64 GetTotalReceiveCount() const {
    return total_tcp_recv_count + total_udp_recv_count;
  }
  inline uint64 GetTotalSendBytes() const {
    return total_tcp_send_bytes + total_udp_send_bytes;
  }
  inline uint64 GetTotalReceiveBytes() const {
    return total_tcp_recv_bytes + total_udp_recv_bytes;
  }

  FUN_NETX_API NetServerStats();
  FUN_NETX_API void Reset();
  FUN_NETX_API String ToString() const;

  inline friend TextStream& operator<<(TextStream& stream,
                                       const NetServerStats& v) {
    return stream << v.ToString();
  }
};

class NetClientStats {
 public:
  uint64 total_tcp_recv_bytes;
  uint64 total_tcp_send_bytes;
  uint64 total_udp_send_count;
  uint64 total_udp_send_bytes;
  uint64 total_udp_recv_count;
  uint64 total_udp_recv_bytes;
  int32 remote_peer_count;
  bool server_udp_enabled;
  int32 direct_p2p_enabled_peer_count;

  inline uint64 GetTotalSendBytes() const {
    return total_tcp_send_bytes + total_udp_send_bytes;
  }
  inline uint64 GetTotalReceiveBytes() const {
    return total_tcp_recv_bytes + total_udp_recv_bytes;
  }

  FUN_NETX_API NetClientStats();
  FUN_NETX_API void Reset();
  FUN_NETX_API String ToString() const;

  inline friend TextStream& operator<<(TextStream& stream,
                                       const NetClientStats& v) {
    return stream << v.ToString();
  }
};

class TickResult {
 public:
  /** The number of processed messages. */
  int32 processed_message_count;

  /** The number of processed events. */
  int32 processed_event_count;

  /** Default constructor. */
  inline TickResult() : processed_message_count(0), processed_event_count(0) {}
};

class ServerConnectionState {
 public:
  bool real_udp_enabled;

  inline ServerConnectionState() : real_udp_enabled(false) {}
};

class DirectP2PInfo {
 public:
  InetAddress local_udp_socket_addr;
  InetAddress local_to_remote_addr;
  InetAddress remote_to_local_addr;

  inline DirectP2PInfo()
      : local_to_remote_addr(InetAddress::None),
        remote_to_local_addr(InetAddress::None),
        local_udp_socket_addr(InetAddress::None) {}

  /**
   * 홀펀칭이 정상적으로 이루어져 있는지 확인.
   *
   * 홀펀칭이 완료 되려면 다음의 주소들이 접근할 수 있는 주소여야 함.
   *    1. 로컬 UDP 소켓 주소.
   *    2. 로컬에서 원격지로 접근할 수 있는 소켓 주소.
   *    3. 원격지에서 로컬로 접근할 수 있는 소켓 주소.
   */
  inline bool HasBeenHolepunched() const {
    return local_udp_socket_addr.IsUnicast() &&
           local_to_remote_addr.IsUnicast() && remote_to_local_addr.IsUnicast();
  }
};

class ApplicationHint {
 public:
  double recent_frame_rate;

  inline ApplicationHint() : recent_frame_rate(0) {}
};

class SuperPeerRating {
 public:
  HostId host_id;
  double rating;
  bool real_udp_enabled;
  bool behind_nat;
  double recent_ping;
  double p2p_group_total_recent_ping;
  double send_speed;
  double joined_time;
  double frame_rate;
};

// SuperPeerSelectionPolicy
class SuperPeerSelectionPolicy {
 public:
  double real_udp_weight;
  double no_nat_device_weight;
  double server_lag_weight;
  double peer_lag_weight;
  double send_speed_weight;
  double frame_rate_weight;
  double exclude_newjoinee_duration_time;

  FUN_NETX_API SuperPeerSelectionPolicy();

  inline friend bool operator==(const SuperPeerSelectionPolicy& a,
                                const SuperPeerSelectionPolicy& b) {
    return a.real_udp_weight == b.real_udp_weight &&
           a.no_nat_device_weight == b.no_nat_device_weight &&
           a.server_lag_weight == b.server_lag_weight &&
           a.peer_lag_weight == b.peer_lag_weight &&
           a.send_speed_weight == b.send_speed_weight &&
           a.frame_rate_weight == b.frame_rate_weight;
  }

  inline friend bool operator!=(const SuperPeerSelectionPolicy& a,
                                const SuperPeerSelectionPolicy& b) {
    return !(a == b);
  }

  FUN_NETX_API static SuperPeerSelectionPolicy GetOrdinary();
  FUN_NETX_API static SuperPeerSelectionPolicy GetNull();
};

class P2PConnectionStats {
 public:
  /** The number of P2P connections. */
  int32 total_p2p_count;
  /** The number of direct P2P */
  int32 direct_p2p_count;
  /** The number of attempts which is send messages to remote-peer. */
  int32 to_remote_peer_send_udp_message_attempt_count;
  /** The number of success which is send messages to remote-peer. */
  int32 to_remote_peer_send_udp_message_success_count;
};

class P2PPairConnectionStats {
 public:
  int32 to_remote_b_send_udp_message_attempt_count;
  int32 to_remote_b_send_udp_message_success_count;
  int32 to_remote_a_send_udp_message_attempt_count;
  int32 to_remote_a_send_udp_message_success_count;
};

/**
@todo Thread관련된곳으로 옮기는것도... 하지만 고민이 좀 필요할듯.. 놔두는것도
나쁘지 않은 선택인듯 싶다.
*/
class ThreadInfo {
 public:
  uint32 thread_id;
  // TODO platform dependent
  HANDLE thread_handle;
};

class ClientWorkerInfo {
 public:
  bool is_worker_thread_null;
  bool is_worker_thread_ended;
  int32 connect_call_count;
  int32 disconnect_call_count;
  ConnectionState connection_state;
  int32 final_work_item_count;
  double last_tcp_stream_time;
  double current_time;
  int32 peer_count;
  int32 worker_thread_id;
};

class SocketInfo {
 public:
  /** TCP socket handle */
  SOCKET tcp_socket;
  /** UDP socket handle */
  SOCKET udp_socket;

  inline SocketInfo()
      : tcp_socket(INVALID_SOCKET), udp_socket(INVALID_SOCKET) {}
};

/**
 * NetClient::Disconnect()에 사용하는 파라메터 블럭.
 */
class DisconnectArgs {
 public:
  int64 graceful_disconnect_timeout_msec;
  uint32 disconnect_sleep_interval_msec;
  ByteArray comment;

  FUN_NETX_API DisconnectArgs();

  FUN_NETX_API static const DisconnectArgs Default;
};

class NetServer : public RpcHost {
 public:
  /**
   * Create a server instance.
   */
  static FUN_NETX_API NetServer* New();

  /**
   * Close a client connection.
   *
   * \param client_id - Client's host id to closing.
   *
   * \return Whether closed successfully or not.
   */
  virtual bool CloseConnection(HostId client_id) = 0;

  /**
   * Close all connections.
   */
  virtual void CloseAllConnections() = 0;

  /**
   * Create a P2P group.
   *
   * \param member_list - Member client id list.
   * \param member_count - Number of member client id list.
   * \param custom_field - Extra hint data.
   * \param option - Option for group creation.
   * \param assigned_host_id - Not allocate new group id if this value is
   * specified.
   *
   * \return Group host id which is created.
   */
  virtual HostId CreateP2PGroup(
      const HostId* member_list, int32 member_count,
      const ByteArray& custom_field = ByteArray(),
      const P2PGroupOption& option = P2PGroupOption::Default,
      HostId assigned_host_id = HostId_None) = 0;

  /**
   * Create a empty P2P group.
   *
   * \param custom_field - Extra hint data.
   * \param option - Option for group creation.
   * \param assigned_host_id - Not allocate new group id if this value is
   * specified.
   *
   * \return Group host id which is created.
   */
  inline HostId CreateP2PGroup(
      const ByteArray& custom_field = ByteArray(),
      const P2PGroupOption& option = P2PGroupOption::Default,
      HostId assigned_host_id = HostId_None) {
    return CreateP2PGroup(nullptr, 0, custom_field, option, assigned_host_id);
  }

  /**
   * Create a alone P2P group.
   *
   * \param member_id - Member client id.
   * \param custom_field - Extra hint data.
   * \param option - Option for group creation.
   * \param assigned_host_id - Not allocate new group id if this value is
   * specified.
   *
   * \return Group host id which is created.
   */
  inline HostId CreateP2PGroup(
      HostId member_id, const ByteArray& custom_field = ByteArray(),
      const P2PGroupOption& option = P2PGroupOption::Default,
      HostId assigned_host_id = HostId_None) {
    return CreateP2PGroup(&member_id, 1, custom_field, option,
                          assigned_host_id);
  }

  /**
   * Destroy a P2P group.
   *
   * \param group_id - Group host id to destroy.
   */
  virtual bool DestroyP2PGroup(HostId group_id) = 0;

  /**
   * Destroy all empty P2P groups.
   */
  virtual void DestroyEmptyP2PGroups() = 0;

  /**
   * Dump group status.
   */
  virtual String DumpGroupStatus() = 0;

  /**
   * Gets the number of connected clients.
   */
  virtual int32 GetClientCount() = 0;

  virtual void GetStats(NetServerStats& out_stats) = 0;

  virtual int32 GetClientHostIds(HostId* output, int32 output_length) = 0;

  // TODO predicate를 통한 enumeration을 지원.

  virtual bool GetJoinedP2PGroups(HostId client_id, HostIdArray& output) = 0;

  virtual double GetLastPingSec(HostId client_id) = 0;

  inline int32 GetLastPingMilisec(HostId client_id) {
    const double sec = GetLastPingSec(client_id);
    return sec < 0 ? (int32)sec : (int32)(sec * 1000);
  }

  virtual double GetRecentPingSec(HostId client_id) = 0;

  inline int32 GetRecentPingMilisec(HostId client_id) {
    const double sec = GetRecentPingSec(client_id);
    return sec < 0 ? (int32)sec : (int32)(sec * 1000);
  }

  virtual double GetP2PRecentPing(HostId host_a, HostId host_b) = 0;

  virtual bool GetP2PGroupInfo(HostId group_id,
                               P2PGroupInfo& out_group_info) = 0;

  virtual bool IsValidHostId(HostId host_id) = 0;

  virtual void GetP2PGroups(P2PGroupInfos& out_groups) = 0;

  virtual int32 GetP2PGroupCount() = 0;

  virtual bool GetClientInfo(HostId client_id, NetClientInfo& out_info) = 0;

  virtual bool IsConnectedClient(HostId client_id) = 0;

  virtual bool SetHostTag(HostId host_id, void* host_tag) = 0;

  virtual double GetTime() = 0;

  virtual bool JoinP2PGroup(HostId member_id, HostId group_id,
                            const ByteArray& custom_field = ByteArray()) = 0;

  virtual bool LeaveP2PGroup(HostId member_id, HostId group_id) = 0;

  virtual void SetCallbacks(INetServerCallbacks* callbacks) = 0;

  virtual bool Start(const StartServerArgs& args,
                     SharedPtr<ResultInfo>& out_error) = 0;

  virtual void Stop() = 0;

  virtual void AllowEmptyP2PGroup(bool allow) = 0;

  virtual NamedInetAddress GetRemoteIdentifiableLocalAddr() = 0;

  // TODO 여러개의 리스닝 포트를 지원하게 되면, 목록 형태로 받아야할듯...
  virtual InetAddress GetTcpListenerLocalAddr() = 0;

  virtual void GetUdpListenerLocalAddrs(
      Array<InetAddress>& out_address_list) = 0;

  virtual void SetDefaultTimeoutTimeMilisec(uint32 timeout_msec) = 0;

  virtual void SetDefaultTimeoutTimeSec(double timeout_sec) = 0;

  virtual void SetDefaultFallbackMethod(FallbackMethod fallback_method) = 0;

  virtual void EnableIntraLogging(const char* log_filename) = 0;
  virtual void DisableIntraLogging() = 0;

  virtual void SetSpeedHackDetectorReckRatio(double Ratio) = 0;

  virtual bool EnableSpeedHackDetector(HostId client_id, bool enable) = 0;

  virtual void SetMessageMaxLength(int32 MaxLength) = 0;

  virtual bool IsNagleAlgorithmEnabled() = 0;

  virtual void SetMaxDirectP2PConnectionCount(HostId client_id,
                                              int32 max_count) = 0;

  virtual bool SetDirectP2PStartCondition(
      DirectP2PStartCondition condition) = 0;

  virtual HostId GetMostSuitableSuperPeerInGroup(
      HostId group_id,
      const SuperPeerSelectionPolicy& policy =
          SuperPeerSelectionPolicy::GetOrdinary(),
      const Array<HostId>& excludess = Array<HostId>()) = 0;

  virtual HostId GetMostSuitableSuperPeerInGroup(
      HostId group_id, const SuperPeerSelectionPolicy& policy,
      HostId excludee) = 0;

  virtual int32 GetSuitableSuperPeerRankListInGroup(
      HostId group_id, super_peer_rating_* out_ratings,
      int32 out_rantings_max_count,
      const SuperPeerSelectionPolicy& policy =
          SuperPeerSelectionPolicy::GetOrdinary(),
      const Array<HostId>& excludess = Array<HostId>()) = 0;

  virtual void GetUdpSocketAddrList(Array<InetAddress>& OutAddr) = 0;

  //@maxidea: newer
  // virtual ResultCode GetUnreliablemessagingLossRatioPercentage(HostId
  // remote_peer_id, int32& out_percentage) = 0;

  virtual uint32 GetInternalVersion() = 0;

  virtual bool GetP2PConnectionStats(HostId remote_id,
                                     P2PConnectionStats& out_stats) = 0;

  virtual bool GetP2PConnectionStats(HostId remote_a, HostId remote_b,
                                     P2PPairConnectionStats& out_stats) = 0;

  virtual void GetUserWorkerThreadInfo(Array<ThreadInfo>& out_info) = 0;

  virtual void GetNetWorkerThreadInfo(Array<ThreadInfo>& out_info) = 0;

  virtual bool SendFreeform(const HostId* rpc_sendto_list,
                            int32 rpc_sendto_count,
                            const RpcCallOption& rpc_call_opt,
                            const uint8* payload, int32 payload_length) = 0;

  inline bool SendFreeform(HostId rpc_sendto, const RpcCallOption& rpc_call_opt,
                           const uint8* payload, int32 payload_length) {
    return SendFreeform(&rpc_sendto, 1, rpc_call_opt, payload, payload_length);
  }

  virtual void TEST_SetOverSendSuspectingThresholdInBytes(int32 threshold) = 0;
  virtual void TEST_SetTestping(HostId host_id0, HostId host_id1,
                                double ping) = 0;
};

class NetClient : public RpcHost {
 public:
  static FUN_NETX_API NetClient* New();

  //@deprecated
  // 가상으로 랙 유발을 시킨다. 송신, 수신시에 모두 적용된다.
  // 통상적으로 300, 100이 약간 심한 랙 환경을 흉내낸다.
#ifdef DEPRECATE_SIMLAG
  virtual void SimulateBadTraffic(int32 min_extra_ping,
                                  int32 extra_ping_variance) = 0;
#endif

  virtual bool Connect(const NetConnectionArgs& args) = 0;

  virtual void Disconnect() = 0;

  virtual void Disconnect(const DisconnectArgs& args) = 0;

  virtual String DumpGroupStatus() = 0;

  virtual void Tick(TickResult* out_result = nullptr) = 0;

  virtual void GetGroupMembers(HostId group_id,
                               HostIdArray& out_member_list) = 0;

  virtual double GetIndirectServerTime(HostId peer_id) = 0;

  virtual HostId GetLocalHostId() = 0;

  virtual String GetNatDeviceName() = 0;

  virtual void GetLocalJoinedP2PGroups(HostIdArray& output) = 0;

  virtual void GetStats(NetClientStats& out_stats) = 0;

  virtual double GetP2PServerTime(HostId group_id) = 0;

  virtual InetAddress GetLocalUdpSocketAddr(HostId peer_id) = 0;

  virtual bool GetDirectP2PInfo(HostId peer_id, DirectP2PInfo& out_info) = 0;

  virtual InetAddress GetServerAddress() = 0;

  //@maxidea: deprecated.
  virtual bool GetPeerInfo(HostId peer_id, PeerInfo& out_info) = 0;

  //@maxidea: newer...
  // virtual bool GetClientInfo(HostId client_id, NetClientInfo& out_info) = 0;

  virtual bool SetHostTag(HostId host_id, void* host_tag) = 0;

  virtual double GetServerTime() = 0;

  virtual double GetServerTimeDiff() = 0;

  virtual ConnectionState GetServerConnectionState() = 0;
  virtual ConnectionState GetServerConnectionState(
      ServerConnectionState& out_state) = 0;

  virtual void GetWorkerState(ClientWorkerInfo& out_info) = 0;

  inline bool HasServerConnection() {
    return GetServerConnectionState() == ConnectionState::Connected;
  }

  virtual void SetCallbacks(INetClientCallbacks* callbacks) = 0;

  virtual double GetLastPingSec(HostId remote_id) = 0;

  inline int32 GetLastPingMilisec(HostId remote_id) {
    const double sec = GetLastPingSec(remote_id);
    return sec < 0 ? (int32)sec : (int32)(sec * 1000);
  }

  virtual double GetRecentPingSec(HostId remote_id) = 0;

  inline int32 GetRecentPingMilisec(HostId remote_id) {
    const double sec = GetRecentPingSec(remote_id);
    return sec < 0 ? (int32)sec : (int32)(sec * 1000);
  }

  virtual bool InvalidateUdpSocket(HostId peer_id, DirectP2PInfo& out_info) = 0;

  virtual bool RestoreUdpSocket(HostId peer_id) = 0;

  //@deprecated
  // 사용자는 이걸 쓰지 말것
  virtual void TEST_FallbackUdpToTcp(FallbackMethod Mode) = 0;
  virtual void TEST_EnableVirtualSpeedHack(double multiplied_speed) = 0;

  virtual bool GetPeerRUdpStats(HostId peer_id, RUdpHostStats& out_stats) = 0;

  virtual bool IsLocalHostBehindNAT(bool& out_is_behind_nat) = 0;

  virtual double GetSendToServerSpeed() = 0;

  virtual uint32 GetInternalVersion() = 0;

  virtual int32 GetMessageMaxLength() = 0;

  virtual InetAddress GetPublicAddress() = 0;

  virtual void GetNetWorkerThreadInfo(ThreadInfo& out_info) = 0;

  virtual bool GetSocketInfo(HostId remote_id, SocketInfo& out_info) = 0;

  virtual void SetApplicationHint(const ApplicationHint& hint) = 0;

  virtual bool SendFreeform(const HostId* rpc_sendto_list,
                            int32 rpc_sendto_count,
                            const RpcCallOption& rpc_call_opt,
                            const uint8* payload, int32 payload_length) = 0;

  inline bool SendFreeform(HostId rpc_sendto, const RpcCallOption& rpc_call_opt,
                           const uint8* payload, int32 payload_length) {
    return SendFreeform(&rpc_sendto, 1, rpc_call_opt, payload, payload_length);
  }

  virtual bool SendEmergencyLogData(const String& server_ip,
                                    int32 server_port) = 0;

  virtual void SetDefaultTimeoutTimeSec(double timeout_sec) = 0;

  virtual InetAddress GetTcpLocalAddr() = 0;
  virtual InetAddress GetUdpLocalAddr() = 0;

  virtual String TEST_GetDebugText() = 0;
  virtual void TEST_GetTestStats(TestStats2& out_stats) = 0;

  //@deprecated
#ifdef TEST_DISCONNECT_CLEANUP
  virtual bool IsAllCleanup() = 0;
#endif

  virtual double GetAbsoluteTime() = 0;
};

class RUdpHostStats {
 public:
  int32 received_frame_count;
  int32 received_stream_count;
  int32 total_recv_stream_length;
  int32 total_ack_frame_count;
  int32 recent_receive_speed;
  FrameNumber expected_frame_number;
  FrameNumber last_recv_data_frame_number;

  int32 send_stream_count;
  int32 first_send_frame_count;
  int32 resend_frame_count;
  int32 total_send_stream_length;
  int32 total_resend_count;

  int32 total_first_send_count;

  int32 recent_send_frame_to_udp_speed;
  int32 send_speed_limit;

  FrameNumber first_sender_window_last_frame;
  FrameNumber resend_window_last_frame;
  FrameNumber last_expected_frame_number_at_sender;

  int32 total_receive_data_count;
};

/**
 * @deprecated
 *  주의:
 * 이걸 여러 스레드에서 너무 자주 접근할 시, 멀티코어 환경에서
 *  메인보드상에서의 병목이 유발되더라.
 */
class TestStats {
 public:
  static double test_allowed_max_send_speed;
  static double test_recent_recv_speed;
  static double test_recent_send_speed_at_receiver_side;
};

/**
 * @deprecated
 *  주의:
 * 이걸 여러 스레드에서 너무 자주 접근할 시, 멀티코어 환경에서
 *  메인보드상에서의 병목이 유발되더라.
 */
class TestStats2 {
 public:
  bool c2s_reply_recent_recv_speed_done;
  bool c2c_request_recent_recv_speed_done;
  bool c2c_reply_recent_recv_speed_done;

  inline TestStats2()
      : c2s_reply_recent_recv_speed_done(false),
        c2c_request_recent_recv_speed_done(false),
        c2c_reply_recent_recv_speed_done(false) {}
};

}  // namespace net
}  // namespace fun
