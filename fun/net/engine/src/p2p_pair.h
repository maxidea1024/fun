#pragma once

namespace fun {
namespace net {

class NetServerImpl;
class LanServerImpl;
class RemoteClient_S;
class LanClient_S;

class P2PConnectionState {
 private:
  NetServerImpl* owner;

  /** 직빵 p2p 연결을 못하는 중인가? */
  bool is_relayed;

  struct PeerAddrInfo {
    HostId id;
    /** 각 클라는 상대 Peer마다 서로 다른 external, internal ADDR을 가지므로
     * 필수 */
    InetAddress external_addr;
    /** 각 클라는 상대 Peer마다 서로 다른 external, internal ADDR을 가지므로
     * 필수 */
    InetAddress internal_addr;
    /** NOTE: 여기에 CRemoteClient_S를 넣지말것 */
    bool member_join_acked;

    inline PeerAddrInfo()
        : id(HostId_None),
          member_join_acked(false),
          external_addr(InetAddress::None),
          internal_addr(InetAddress::None) {}
  };
  PeerAddrInfo peers[2];

 public:
  /**
  PeerHolepunchedInfo는 P2P pair가 재활용 대기 상태에 있을때, 즉 릴리즈 되었을때
  PeerAddrInfo를 백업해둔 값이다.
  */
  struct PeerHolepunchedInfo {
    HostId id;
    InetAddress internal_addr;
    InetAddress external_addr;
    InetAddress sendto_addr;
    InetAddress recvfrom_addr;

    PeerHolepunchedInfo()
        : id(HostId_None),
          internal_addr(InetAddress::None),
          external_addr(InetAddress::None),
          sendto_addr(InetAddress::None),
          recvfrom_addr(InetAddress::None) {}
  };

  PeerHolepunchedInfo peer_holepunched_infos[2];

  RemoteClient_S* first_client;
  RemoteClient_S* second_client;

  bool jit_direct_p2p_requested;  // JIT P2P를 요청받은 적이 있나?
  int32 dup_count;  // 쌍방 peer끼리 P2P group이 몇개 중복됐는가?

  // 자기 자신과의 연결인 경우 p2p connection count에서 제외해야 하므로
  // (참고: 자기 자신과의 연결에 대해서는 UDP 홀펀칭 관련 메시지가 안온다.)
  bool is_loopback_connection;

  // 예전 버전에서는 session key를 서버에서 만들어 두 peer간 session key를 직접
  // 전송하는 방식이었다. 그러나 Windows crypto api는 이것을 어렵게 한다. public
  // key만 쌩 blob 추출이 허용되기 때문이다. 이런 경우 random blob을 만들어서
  // 그것을 클라에 전송하고, 클라는 그것을 crypto hash로 받아들인 후
  // CryptDeriveKey를 통해 session key로 만들어 쓰도록 해야 한다.
  //
  // 물론 서버에서 session key를 만들어 두 peer에게 전송할 수도 있다. 하지만
  // 이런 경우 두 클라가 미리 받아 놓은 public key를 위한 암호화 과정을 거쳐야
  // 하는데, 이 부하도 만만치 않다. 그러므로 hash를 만들어 전송한다.
  ByteArray P2PAESSessionKey;
  ByteArray P2PRC4SessionKey;

  FrameNumber p2p_first_frame_number;
  Uuid holepunch_tag;  // P2P 홀펀칭 과정에서 사용될 값

  // 클라에서 보고한, 피어간 ping
  double recent_ping;

  double ReleaseTime;
  double last_holepunch_success_time_;

  // 두 피어로부터 ack가 오는 것이 아직 대기중인 경우에만 true이다.
  bool member_join_acked_start_;

  // pair간 udp message 시도,성공횟수
  int32 to_remote_peer_send_udp_message_success_count;
  int32 to_remote_peer_send_udp_message_attempt_count;

  bool ContainsHostId(HostId peer_id);

  bool SetRecycleSuccess(HostId peer_id);

  bool MemberJoinAckedAllComplete();
  void MemberJoinAcked(HostId peer_id);

  void SetRelayed(bool relayed);
  bool GetRelayed();

  int32 GetServerHolepunchOkCount();
  void SetServerHolepunchOk(HostId peer_id, const InetAddress& internal_addr,
                            const InetAddress& external_addr);
  void SetPeerHolepunchOk(HostId peer_id, const InetAddress& sendto_addr,
                          const InetAddress& recvfrom_addr);
  InetAddress GetInternalAddr(HostId peer_id);
  InetAddress GetExternalAddr(HostId peer_id);

  bool CanRecycle();
  void MemberJoinStart(HostId PeerIdA, HostId PeerIdB);

  InetAddress GetHolepunchedInternalAddr(HostId peer_id);

  InetAddress GetHolepunchedSendToAddr(HostId peer_id);
  InetAddress GetHolepunchedRecvFromAddr(HostId peer_id);

  void ResetPeerInfo();

  P2PConnectionState(NetServerImpl* owner, bool is_loopback_connection);
  ~P2PConnectionState();
};

typedef SharedPtr<P2PConnectionState> P2PConnectionStatePtr;

/**
LanServer용 P2PConnectionState
무조건 relayed 이다. ( TCP 연결만 가능 )
*/
class LanP2PConnectionState {
 private:
  LanServerImpl* owner;

  struct PeerAddrInfo {
    HostId id;
    InetAddress external_addr;  // 각 클라는 상대 Peer마다 서로 다른 external
                                // ADDR을 가지므로 필수
    // NOTE: 여기에 CLanClient_S를 넣지말것

    PeerAddrInfo() : id(HostId_None), external_addr(InetAddress::None) {}
  };
  PeerAddrInfo peers[2];

 public:
  LanClient_S* first_client;
  LanClient_S* second_client;

  int32 dup_count;  // 쌍방 peer끼리 P2P group이 몇개 중복됐는가?

  // 예전 버전에서는 session key를 서버에서 만들어 두 peer간 session key를 직접
  // 전송하는 방식이었다. 그러나 Windows crypto api는 이것을 어렵게 한다. public
  // key만 쌩 blob 추출이 허용되기 때문이다. 이런 경우 random blob을 만들어서
  // 그것을 클라에 전송하고, 클라는 그것을 crypto hash로 받아들인 후
  // CryptDeriveKey를 통해 session key로 만들어 쓰도록 해야 한다.
  //
  // 물론 서버에서 session key를 만들어 두 peer에게 전송할 수도 있다. 하지만
  // 이런 경우 두 클라가 미리 받아 놓은 public key를 위한 암호화 과정을 거쳐야
  // 하는데, 이 부하도 만만치 않다. 그러므로 hash를 만들어 전송한다.
  ByteArray P2PAESSessionKey;
  ByteArray P2PRC4SessionKey;
  Uuid holepunch_tag;  // P2P 인증에서 사용할 값 ( 해킹이나 다른 connection을
                       // 방지하기 위함. )

  enum class P2PConnectState {
    // 초기화
    None = 0,
    // P2P Member Join중
    P2PJoining,
    // P2P Tcp Connect중
    P2PConnecting,
    // P2P Connection 완료
    P2PConnected,
  };

  P2PConnectState P2PConnectState;  // 현재 P2P 연결상태 (TCP)

  // 클라에서 보고한, 피어간 ping
  double recent_ping;

  void SetExternalAddr(HostId peer_id, const InetAddress& external_addr);
  InetAddress GetExternalAddr(HostId peer_id);

  LanP2PConnectionState(LanServerImpl* owner);
  ~LanP2PConnectionState();
};

typedef SharedPtr<LanP2PConnectionState> LanP2PConnectionStatePtr;

}  // namespace net
}  // namespace fun
