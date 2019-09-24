#pragma once

#include "P2PGroup_C.h"   // P2PGroups_C
#include "PacketFrag.h"
#include "RUdp.h"

namespace fun {

class NetClientImpl;
//class ReceivedMessageList;
class ReceivedMessage;
class SendFragRefs;
class RemotePeerRUdp;

class RemotePeer_C;
typedef SharedPtr<RemotePeer_C> RemotePeerPtr_C;
typedef Map<HostId, RemotePeerPtr_C> RemotePeers_C;

/**
 * 이 remote peer에 직접 p2p 연결 시도를 상태를 담은 객체
 */
class P2PHolepunchAttemptContext {
 private:
  /** 어느 remote peer를 위한 context인지? */
  RemotePeer_C* owner_;

  /**이 객체의 수명을 재기 위함 */
  double total_elapsed_time_;

 public:
  enum class StateType {
    ServerHolepunch,
    PeerHolepunch
  };

  class StateBase {
   public:
    StateType type_;

    StateBase() {}
    StateBase(StateType type) : type_(type) {}
    virtual ~StateBase() {}
  };

  class ServerHolepunchState : public StateBase {
   public:
    double send_cooltime_;
    int32 ack_recv_count_;
    Uuid holepunch_tag_;

    inline ServerHolepunchState()
      : StateBase(StateType::ServerHolepunch),
        ack_recv_count_(0),
        send_cooltime_(0),
        holepunch_tag_(Uuid::NewRandomUuid()) {}
  };

  class PeerHolepunchState : public StateBase {
   public:
    double send_cooltime_;
    int32 send_turn_;
    int32 ack_recv_count_;
    Uuid holepunch_tag_;
    int32 offset_shotgun_countdown_;
    int16 shotgun_min_port_;

    inline PeerHolepunchState()
      : StateBase(StateType::PeerHolepunch),
        shotgun_min_port_(1023),
        offset_shotgun_countdown_(NetConfig::shotgun_attempt_count),
        ack_recv_count_(0),
        send_cooltime_(0),
        send_turn_(0),
        holepunch_tag_(Uuid::NewRandomUuid()) {}
  };

 private:
  static int32 AdjustUdpPortNumber(int32 port);

 public:
  SharedPtr<StateBase> state_;

 private:
  InetAddress GetServerUdpAddr();

  void SendPeerHolepunch( const InetAddress& a2b_send_addr,
                          const Uuid& tag,
                          const char* debug_hint);
  void SendPeerHolepunchAck(const InetAddress& b2a_send_addr,
                            const Uuid& tag,
                            const InetAddress& a2b_send_addr,
                            const InetAddress& a2b_recv_addr,
                            const char* debug_hint);

 public:
  p2p_holepunch_attempt_context_(RemotePeer_C* owner);

 public:
  bool Heartbeat();

  NetClientImpl* GetClient();

  InetAddress GetExternalAddr();
  InetAddress GetInternalAddr();

  static void ProcessPeerHolepunch(NetClientImpl* main, ReceivedMessage& received_msg);
  static void ProcessPeerHolepunchAck(NetClientImpl* main, ReceivedMessage& received_msg);

  void ProcessMessage_PeerUdp_ServerHolepunchAck( ReceivedMessage& received_msg,
                                                  const Uuid& tag,
                                                  const InetAddress& here_addr_at_server,
                                                  HostId peer_id);
};

typedef SharedPtr<p2p_holepunch_attempt_context_> P2PHolepunchAttemptContextPtr;


class RemotePeer_C
  : public ISendDest_C,
    public IP2PGroupMember,
    public IUdpPacketFraggerDelegate {
 public:
  RemotePeer_C(NetClientImpl* owner);
  ~RemotePeer_C();

  void GetPeerInfo(PeerInfo& out_info);

  bool IsBehindNAT();

  bool IsRelayedP2P() const { return is_relayed_p2p_; }
  bool IsDirectP2P() const { return !is_relayed_p2p_; }

  // 이 remote peer에 직접 p2p 연결 시도를 하는 과정을 시작한다.
  void CreateP2PHolepunchAttemptContext();

  void Heartbeat(double absolute_time);

  void ConditionalRestoreUdpSocket();

  void ConditionalNewUdpSocket();

  void FallbackP2PToRelay(bool first_chance, ResultCode reason);

  double GetIndirectServerTimeDiff();

  bool NewUdpSocketBindPort(int32 port);

  void ReserveRepunch();

  UdpSocket_C* Get_ToPeerUdpSocket();

  HostId GetHostId() const override { return host_id; }

  void GetDirectP2PInfo(DirectP2PInfo& output);

  double GetAbsoluteTime() override;

  void RequestReceiveSpeedAtReceiverSide_NoRelay(const InetAddress& dst) override;

  int32 GetOverSendSuspectingThresholdInByte() override;

  void AssureUdpSocketNotUnderIssued();

  void InitGarbage(NetClientImpl* owner);

  bool IsSameLanToLocal() const;

  // per-peer UDP socket
  // - UDP socket 생성은 RemotePeer_C 생성 후 1~2초 뒤에 하도록 한다.
  // (바로 만들면 지나친 소켓 생성/종료 문제가 야기되므로)
  // UDP 소켓은 RemotePeer_C.dtor까지 계속 사용된다.
  IHasOverlappedIoPtr udp_socket_;

  // 테스트를 위해 강제로 드랍시킨 UDP 소켓을 다시 복원하고자 할 때 이 값이 true가 된다.
  bool restore_needed_;

  // JIT P2P 기능.
  // 릴레이 모드인 피어에게 사용자가 P2P 메시징을 시작하면
  // 그제서야 피어간 홀펀칭을 시작한다.
  // 이건 그걸 발동시키는 시발점이다.
  bool jit_direct_p2p_needed_;

  // 다음에 이게 세팅된다.
  // (이때 서버에게는 요청만 간 상태이며 아직 UDP 소켓은 없음)
  bool jit_direct_p2p_triggered_;

  // 서버로부터 요청이 오면 이게 세팅된 후 곧 UDP 소켓이 준비된다.
  bool new_p2p_connection_needed;

  // 서버로부터 member join 지시받음->local port 준비 후
  // 서버에 결과 보고->서버로부터 member join의
  // 최종 결과를 받으면 true로 세팅되는 값
  bool member_join_process_end_;

  // FunNet.P2PGroupOption.bEnableDirectP2P에서 전파받음
  bool direct_p2p_enabled_;

  P2PHolepunchAttemptContextPtr p2p_holepunch_attempt_context_;

  //TODO
  // 서버에서 전파 받아야한다.
  // 피어가 Join할대도 전달하고,
  // 서버에서는 해당 RP가 참여한 모든 그룹에 전파해주어야함.
  bool real_udp_enabled_;

  // P2P hole punch 과정에서 잘못된 3rd peer로부터의 홀펀칭에 대한 에코를 걸러내기 위함
  Uuid holepunch_tag_;

  // NetClient가 해당 peer에게 realudp로 보낸 패킷 갯수
  int32 to_remote_peer_send_udp_message_attempt_count_;
  // NetClient가 해당 peer에게 realudp로 받은(성공한) 패킷 갯수
  int32 to_remote_peer_send_udp_message_success_count_;

  // 내가(NetClient)가 이 peer에게 받은 패킷 갯수
  int32 receive_udp_message_success_count_;

  // P2P unreliable 송수신을 위한 객체
  class UdpTransport {
   public:
    RemotePeer_C* owner_;

    UdpTransport(RemotePeer_C* owner) : owner_(owner) {}

    void SendWhenReady(const SendFragRefs& data, const UdpSendOption& send_opt);
    int32 GetUdpSendBufferPacketFilledCount();
    bool IsUdpSendBufferPacketEmpty();
  };

  double recent_ping_;

  /** 측정된 송신 대기량(UDP) */
  int32 send_queued_amount_in_byte_;

  /** 이 peer와 server와의 랙 */
  double peer_to_server_ping_;

  FUN_ALIGNED_VOLATILE double last_ping_send_time_;

  /** 이 UDP peer와 통신을 할 전용 소켓을 생성할 시간 */
  double udp_socket_creation_time_;

  double GetRenewalSocketCreationTime();

  // 가장 마지막에 peer로부터 UDP 패킷을 direct로 받은 시간
  // relay fallback을 위한 검증 목적
  // direct p2p mode일때만 유효한 값이다.
  FUN_ALIGNED_VOLATILE double last_direct_udp_packet_recv_time_;
  FUN_ALIGNED_VOLATILE int32 direct_udp_packet_recv_count_;
  FUN_ALIGNED_VOLATILE double last_udp_packet_recv_interval_;

  /** 이 peer에게 local에서 가진 서버 시간차를 보내는 쿨타임. */
  double sync_indirect_server_time_diff_cooltime_;

  /** 이 값 대신 GetIndirectServerTimeDiff()를 써서 얻어라! */
  double indirect_server_time_diff_;

  /** remote peer와의 랙 */
  double last_ping_;

  /** remote peer의 프레임 레이트 */
  double recent_frame_rate_;

  // Encryption
  SessionKey p2p_session_key_;
  CryptoCountType encrypt_count_;
  CryptoCountType decrypt_count_;

  HostId host_id_;

  /**
   * 상대 피어(이 호스트가 아니라!!!)가 서버와의 홀펀칭 후
   * 얻어진 서버측에서 인식된 상대 피어의 외부 주소.
   */
  InetAddress udp_addr_from_server_;

  /**
   * 이건 상대 피어 내부(이 호스트가 아니라!!!)에서 인식된 내부주소
   */
  InetAddress udp_addr_internal_;

  // local에서 remote로 전송 가능한 홀펀칭된 주소 & remote에서 local로 전송 가능한 홀펀칭된 주소
  InetAddress p2p_holepunched_local_to_remote_addr_;
  InetAddress p2p_holepunched_remote_to_local_addr_;

  P2PGroups_C joined_p2p_groups_;

  // true이면 P2P가 server를 통해 우회하며 직접 P2P 가 안됨을 의미
  bool is_relayed_p2p_;
  double relayed_p2p_disabled_time_;

  /**
   * DirectP2P -> relay 로 바뀔때 last ping을 다시 계산하기 위한 조건 default true
   */
  bool set_to_relayed_but_last_ping_is_not_calculated_yet_;

  /** 총 re-holepunch 횟수 */
  int32 repunch_count_;
  /** 0이면 시작하지 말라는 뜻 */
  double repunch_started_time_;

  /** 마지막으로 send-queue를 체크한 시각입니다. */
  double last_check_send_queue_time_;
  /** send-queue가 비대해지기 시작한 시각입니다. */
  double send_queue_heavy_start_time_;

  /** DirectP2P가 불가능한 상황인지 확인합니다. */
  bool IsRelayConditionByUdpFailure(double absolute_time);
  /** ReliableUDP가 불가능한 상황인지 확인합니다. */
  bool IsRelayConditionByRUdpFailure();

  /** 릴레이 모드로 전환합니다. */
  void SetRelayedP2P(bool relayed = true);
  /** DirectP2P 모드로 전환합니다. */
  void SetDirectP2P(bool direct = true) { SetRelayedP2P(!direct); }

  /** ReliableUDP를 처리하기 위한 객체입니다. */
  RemotePeerRUdp to_peer_rudp_;

  double to_peer_rudp_heartbeat_last_time_;
  double to_peer_report_server_time_and_ping_last_time_;
  UdpTransport to_peer_udp_;
  NetClientImpl* owner_;

  // 사용자가 지정한 remotepeer당 가지고있는 객체
  void* host_tag_;

  // garbage에 들어가서 이값이 0이되면 remove한다.
  int32 leave_event_count_;

  bool garbaged_;
};

} // namespace net
} // namespace fun
