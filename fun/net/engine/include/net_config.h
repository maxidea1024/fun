//TODO 클라이언트 전용/서버 전용/공용 이런식으로 정리가 필요해보임.
//TODO 상수 정의와 수정가능한 값을 분리하도록 하자.

#pragma once

#include "fun/net/net.h"

namespace fun {
namespace net {

class NetConfig {
 public:
  //static double issued_state;

  static int32 udp_issue_recv_length;
  static int32 tcp_issue_recv_length;
  static int32 tcp_send_buffer_length;
  static int32 udp_send_buffer_length;

  static bool socket_tcp_keep_alive_option_enabled;
  static double rudp_heartbeat_interval_sec;

  static double tcp_socket_connect_timeout_sec;
  static double client_connect_server_timeout_sec;

  static uint32 wait_completion_timeout_msec;
  static uint32 server_heartbeat_interval_msec;
  static uint32 server_user_work_max_wait_msec;

  static uint32 client_heartbeat_interval_msec;

  //얘는 상수임.
  static const double INFINITE_COOLTIME;

  static double server_holepunch_interval_sec;
  static double udp_holepunch_interval_sec;
  static double server_udp_repunch_interval_sec;
  static int32 server_udp_repunch_max_attempt_count;
  static int32 server_udp_holepunch_max_attempt_count;
  static double p2p_holepunch_interval_sec;
  static int32 p2p_shotgun_start_turn;
  static int32 p2p_holepunch_max_turn_count;
  static double purge_too_old_unmature_client_timeout_sec;
  static double purge_too_old_add_member_ack_timeout_sec;
  static double dispose_issued_remote_clients_timeout_sec;
  static double remove_too_old_udp_send_packet_queue_timeout_sec;
  static double assemble_fragged_packet_timeout_sec;
  static double GetP2PHolepunchEndTime();
  static int32 shotgun_attempt_count;
  static int32 shotgun_range;
  static double p2p_ping_interval_sec;
  static double cs_ping_interval_sec;
  static double udp_ping_interval_sec;

  static bool report_real_udp_count_enabled;
  static double report_real_udp_count_interval_sec;

  static uint32 lan_client_heartbeat_interval_msec;

  static double lan_peer_connect_peer_timeout_sec;
  static double purge_too_old_joining_timeout_interval_sec;
  static double remove_lookahead_msg_timeout_interval_sec;

  static double remove_too_old_recycle_pair_time_sec;
  static double remove_too_old_recycle_pair_interval_sec;
  static double recycle_pair_reuse_time_sec;

  static double every_remote_issue_send_on_need_interval_sec;

  inline static double GetFallbackServerUdpToTcpTimeout() {
    // 나중에, UdpPingInterval로 개명하자. C-S UDP가 증발하건 P2P UDP가 증발하건 한쪽이 증발하면 나머지도 불안한건 매한가지 이므로.
    fun_check(cs_ping_interval_sec == p2p_ping_interval_sec);

    // 연속 3번 보낸 것을 못 받는다는 것은 80% 이상 패킷 로스라는 의미. 이 정도면 홀펀칭 되어 있어도 막장이다.
    // 따라서 10->4 하향.
    return cs_ping_interval_sec * 4;
  }

  inline static double GetFallbackP2PUdpToTcpTimeout() {
    // 나중에, UdpPingInterval로 개명하자. C-S UDP가 증발하건 P2P UDP가 증발하건 한쪽이 증발하면 나머지도 불안한건 매한가지 이므로.
    fun_check(cs_ping_interval_sec == p2p_ping_interval_sec);

    // 연속 3번 보낸 것을 못 받는다는 것은 80% 이상 패킷 로스라는 의미. 이 정도면 홀펀칭 되어 있어도 막장이다.
    // 따라서 10->4 하향.
    // GetFallbackServerUdpToTcpTimeout과 GetFallbackP2PUdpToTcpTimeout은 서로 같은 값이어야.
    // C-S UDP가 증발하건 P2P UDP가 증발하건 한쪽이 증발하면 나머지도 불안한건 매한가지 이므로.
    return p2p_ping_interval_sec * 4;
  }

  // TCP 핑 타임 아웃에 걸리는 시간
  // 참고: TCP 핑 타임 아웃을 감지하기 위해 핑을 주고 받는 주기는 NetClientImpl::GetReliablePingTimerInterval 에서 결정.
  /**
   * TCP 핑 타임 아웃에 걸리는 시간입니다.
   */
  inline static double GetDefaultNoPingTimeoutTime() {
    // 중국처럼 인터넷이 괴랄한 곳에서는 15초 동안 TCP 가 밀려도 정상적일 수 있으므로 이 정도는 잡아야.
    return 60;
  }

  /**
   * 랙을 순간적으로 변경하지 않고, 완만하게 변하게 하기 위한 linear-interpolation 팩터입니다.
   */
  static double log_linear_programming_factor;

  /**
   * Stream의 grow 수치입니다.
   */
  static int32 stream_grow_by;

  static uint32 InternalNetVersion;

  static uint32 InternalLanVersion;

  static int32 MTU;

  /**
   * Super-peer를 선출하는 간격입니다.
   */
  static double elect_super_peer_interval_sec;

  /**
   * 메시지의 최대 길이입니다.
   */
  static int32 message_max_length;
  /**
   * 메시지의 최대 길이입니다.
   */
  const static int32 MessageMaxLengthInOrdinaryCase = 64 * 1024;
  /**
   * LAN 환경에서의 메시지의 최대 길이입니다.
   */
  const static int32 MessageMaxLengthInServerLan = 1024 * 1024;

  /**
   * 메시지의 최소 길이입니다.  메시지 작성시 최소로 잡히는 메모리의 길이이며, 메시지 길이를 제한하기 위한 값이 아닙니다.
   * TODO 이름을 변경하는건 어떨런지?
   */
  static const int32 MessageMinLength = 128;

  static const int32 DefaultMessageReadRecursionLimit = 128;

  static double default_graceful_disconnect_timeout_sec;

  static int32 max_s2c_multicast_route_count;

  static double unreliable_s2c_routed_multicast_max_ping_default;

  static bool force_compressed_relay_dest_list_only;

  static const int32 MaxConnectionCount = 60000;

  static const int32 OrdinaryHeavyS2CMulticastCount = 100;

  ///**
  //*/
  //static const bool bEnableTestSplitter;

  ///**
  //*/
  //static const int32 ClientListHashTableValue = 101;

 private:
  static CCriticalSection2 write_mutex;

 public:
  static const bool speedhack_detector_enabled_by_default;

  static bool message_prioritization_enabled;

  static const double speedhack_detector_ping_interval_sec;

  static CCriticalSection2& GetWriteMutex();

  static int32 default_max_direct_p2p_multicast_count;

  static bool upnp_detect_nat_device_by_default;
  static bool upnp_tcp_addr_port_mapping_by_default;

  static double measure_client_send_speed_interval_sec;

  static double measure_send_speed_duration_sec;

  static DirectP2PStartCondition default_direct_p2p_start_condition;

  static bool catch_unhandled_exception;

  static bool networker_thread_priority_is_high;

  //static double report_p2p_group_ping_interval_sec;
  static double report_lan_p2p_peer_ping_interval_sec;
  static double report_p2p_peer_ping_test_interval_sec;
  static double report_server_time_and_ping_interval_sec;
  static double udp_packet_board_long_interval_sec;
  static double tcp_packet_board_long_interval_sec;

  static int32 min_send_speed;

  static int32 default_over_send_suspecting_threshold_in_byte;

  //static bool force_unsafe_heap_to_safe_heap;
  static bool send_break_enabled;
  static double viz_reconnect_try_interval_sec;
  static double super_peer_selection_premium;

  static double host_id_recycle_allow_time_sec;

// send_queue_

  /**
  송신큐의 내용이 과한지 여부를 판단하는 임계치입니다.
  */
  static int32 send_queue_heavy_warning_capacity;
  /**
  송신큐의 내용이 과함을 경고 형태로 알리는 간격입니다.
  */
  static double send_queue_heavy_warning_time_sec;
  /**
  송신큐의 내용이 과한지 여부를 판단하는 간격입니다.
  */
  static double send_queue_heavy_warning_check_cooltime_sec;

  /**
  emergency log를 위한 NetClientStats의 사본을 갱신할 coolTime 값입니다.
  */
  static double update_net_client_stat_clone_cooltime_sec;

  /**
  NetClientManager에서 Garbage들을 제거하는 간격입니다.
  */
  static double manager_garbage_free_interval_sec;

  /**
  NetClientManager에서 평균 소요시간을 측정하기 위한 샘플 갯수입니다.
  */
  static int32 manager_average_elapsed_time_sample_count;

  /**
  기아화(Starvation) 상황을 검출할지 여부를 설정합니다.

  클라에서 Tick() 함수가 지나치게 드물게 호출되고 있는지 여부를 감시하여,
  경고 형태로 알려줍니다.
  */
  static bool starvation_warning_enabled;

  /**
  기본적으로 필요시 Fragging을 할지 여부를 설정합니다.
  */
  static bool conditional_fragging_by_default;

  /**
  Critical-section의 dead-lock detection을 할지 여부를 설정합니다.
  */
  static bool deadlock_checking_enabled;

  /**
  MaxDirectP2PMulticast를 할 경우 같은 LAN환경에 있을경우에만 수행할지 여부를 설정합니다.
  */
  static bool use_is_same_lan_to_local_for_max_direct_p2p_multicast;
};

} // namespace net
} // namespace fun
