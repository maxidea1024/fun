#pragma once

#include "fun/net/net.h"
#include "ILanClientCallbacks.h"
#include "ILanServerCallbacks.h"

namespace fun {
namespace net {

class LanServerStats {
 public:
  int32 thread_pool_wait_count;
  double thread_pool_wait_time_sum;
  uint64 total_tcp_recv_count;
  uint64 total_tcp_recv_bytes;
  uint64 total_tcp_send_count;
  uint64 total_tcp_send_bytes;
  int32 p2p_connection_pair_count;
  int32 p2p_direct_connection_pair_count;
  int32 client_count;
  int32 total_queued_message_count;

  inline LanServerStats() {
    Reset();
  }

  inline void Reset() {
    p2p_connection_pair_count = 0;
    p2p_direct_connection_pair_count = 0;
    total_tcp_recv_bytes = 0;
    total_tcp_recv_count = 0;
    total_tcp_send_bytes = 0;
    total_tcp_send_count = 0;
    client_count = 0;
    total_queued_message_count = 0;
  }
};


class LanServer : public RpcHost {
 public:
  static FUN_NETX_API LanServer* New();

  virtual bool CloseConnection(HostId client_id) = 0;

  virtual void CloseAllConnections() = 0;

  virtual HostId CreateP2PGroup(const HostId* member_list,
                                int32 member_count,
                                const ByteArray& custom_field = ByteArray()) = 0;

  inline HostId CreateP2PGroup(const ByteArray& custom_field = ByteArray()) {
    return CreateP2PGroup(nullptr, 0, custom_field);
  }

  inline HostId CreateP2PGroup( HostId member_id,
                                const ByteArray& custom_field = ByteArray()) {
    return CreateP2PGroup(&member_id, 1, custom_field);
  }

  virtual bool DestroyP2PGroup(HostId group_id) = 0;

  virtual void DestroyEmptyP2PGroups() = 0;

  virtual String DumpGroupStatus() = 0;

  virtual int32 GetClientCount() = 0;

  virtual void GetStats(LanServerStats& out_stats) = 0;

  virtual int32 GetClientHostIds(HostId* output, int32 output_length) = 0;

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

  virtual bool GetP2PGroupInfo(HostId group_id, P2PGroupInfo& out_info) = 0;

  virtual bool IsValidHostId(HostId host_id) = 0;

  virtual void GetP2PGroups(P2PGroupInfos& out_group_info_list) = 0;

  virtual int32 GetP2PGroupCount() = 0;

  virtual bool GetClientInfo(HostId client_id, NetClientInfo& out_info) = 0;

  virtual bool IsConnectedClient(HostId client_id) = 0;

  virtual bool SetHostTag(HostId host_id, void* host_tag) = 0;

  virtual double GetTime() = 0;

  virtual bool JoinP2PGroup(HostId member_id,
                            HostId group_id,
                            const ByteArray& custom_field = ByteArray()) = 0;

  virtual bool LeaveP2PGroup(HostId member_id, HostId group_id) = 0;

  virtual void SetCallbacks(ILanServerCallbacks* callbacks) = 0;

  virtual bool Start(const LanStartServerArgs& args, SharedPtr<ResultInfo>& out_error) = 0;

  virtual void Stop() = 0;

  virtual void AllowEmptyP2PGroup(bool allow) = 0;

  virtual NamedInetAddress GetRemoteIdentifiableLocalAddr() = 0;

  virtual InetAddress GetTcpListenerLocalAddr() = 0;

  virtual void SetDefaultTimeoutTimeMilisec(uint32 timeout_msec) = 0;

  virtual void SetDefaultTimeoutTimeSec(double timeout_sec) = 0;

  virtual void EnableIntraLog(const char* log_filename) = 0;
  virtual void DisableIntraLog() = 0;

  virtual void SetMessageMaxLength(int32 max_length) = 0;

  virtual bool IsNagleAlgorithmEnabled() = 0;

  virtual uint32 GetInternalVersion() = 0;

  virtual bool GetP2PConnectionStats(HostId remote_id, P2PConnectionStats& out_stats) = 0;

  virtual void GetUserWorkerThreadInfo(Array<ThreadInfo>& out_info) = 0;
  virtual void GetNetWorkerThreadInfo(Array<ThreadInfo>& out_info) = 0;

  virtual bool SendFreeform(const HostId* rpc_sendto_list,
                            int32 rpc_sendto_count,
                            const RpcCallOption& rpc_call_opt,
                            const uint8* payload,
                            int32 payload_length) = 0;

  inline bool SendFreeform( HostId rpc_sendto,
                            const RpcCallOption& rpc_call_opt,
                            const uint8* payload,
                            int32 payload_length) {
    return SendFreeform(&rpc_sendto, 1, rpc_call_opt, payload, payload_length);
  }
};


class LanClientStats {
 public:
  uint64 total_tcp_recv_bytes;
  uint64 total_tcp_send_bytes;
  int32 remote_peer_count;

  FUN_NETX_API LanClientStats();

  FUN_NETX_API String ToString() const;
};


class LanClient : public RpcHost {
 public:
  static FUN_NETX_API LanClient* New();

  //TODO args를 immutable로 처리하는게 좋겠다. 물론 내부에서 교정되는 부분들이
  //있을 수 있으므로, 별도의 목록을 따로 받아서 결과를 처리하는게 좋으려나?
  virtual bool Connect(LanConnectionArgs& args, SharedPtr<ResultInfo>& out_error) = 0;

  virtual void Disconnect(double graceful_disconnect_timeout_sec, const ByteArray& comment) = 0;

  virtual void Disconnect() = 0;

  //virtual void DisconnectNoWait(double graceful_disconnect_timeout_sec, const ByteArray& comment) = 0;

  virtual String DumpGroupStatus() = 0;

  virtual void GetGroupMembers(HostId group_id, HostIdArray& output) = 0;

  virtual double GetIndirectServerTime(HostId peer_id) = 0;

  virtual HostId GetLocalHostId() = 0;

  virtual void GetLocalJoinedP2PGroups(HostIdArray& output) = 0;

  virtual void GetStats(LanClientStats& out_stats) = 0;

  virtual double GetP2PServerTime(HostId group_id) = 0;

  virtual InetAddress GetServerAddress() = 0;

  virtual bool GetPeerInfo(HostId peer_id, PeerInfo& out_info) = 0;

  virtual bool SetHostTag(HostId host_id, void* host_tag) = 0;

  virtual double GetServerTime() = 0;

  virtual double GetServerTimeDiff() = 0;

  virtual ConnectionState GetServerConnectionState() = 0;

  inline bool HasServerConnection() {
    return GetServerConnectionState() == ConnectionState::Connected;
  }

  virtual void SetCallbacks(ILanClientCallbacks* callbacks) = 0;

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

  virtual double GetSendToServerSpeed() = 0;

  virtual uint32 GetInternalVersion() = 0;

  virtual InetAddress GetPublicAddress() = 0;

  virtual void GetNetWorkerThreadInfo(Array<ThreadInfo>& out_info) = 0;

  virtual void GetUserWorkerThreadInfo(Array<ThreadInfo>& out_info) = 0;

  virtual bool SendFreeform(const HostId* rpc_sendto_list,
                            int32 rpc_sendto_count,
                            const RpcCallOption& rpc_call_opt,
                            const uint8* payload,
                            int32 payload_length) = 0;

  inline bool SendFreeform( HostId rpc_sendto,
                            const RpcCallOption& rpc_call_opt,
                            const uint8* payload,
                            int32 payload_length) {
    return SendFreeform(&rpc_sendto, 1, rpc_call_opt, payload, payload_length);
  }

  template <typename Allocator>
  inline bool SendFreeform( const Array<HostId, Allocator>& rpc_sendto_list,
                            const RpcCallOption& rpc_call_opt,
                            const uint8* payload,
                            int32 payload_length) {
    return SendFreeform(rpc_sendto_list.ConstData(),
                        rpc_sendto_list.Count(),
                        rpc_call_opt,
                        payload,
                        payload_length);
  }

  virtual void EnableIntraLogging(const char* log_filename) = 0;
};

} // namespace net
} // namespace fun
