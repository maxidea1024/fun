#include "fun/net/net.h"

namespace fun {
namespace net {

CCriticalSection2 NetConfig::write_mutex;

// double NetConfig::issued_state = -999;

int32 NetConfig::tcp_issue_recv_length = 1024 * 10;
int32 NetConfig::udp_issue_recv_length = 1024 * 10;

int32 NetConfig::tcp_send_buffer_length = 1024 * 8;
int32 NetConfig::udp_send_buffer_length = 1024 * 8;

double NetConfig::rudp_heartbeat_interval_sec = 0.005;  // 1.0 / 200

bool NetConfig::socket_tcp_keep_alive_option_enabled = false;

double NetConfig::tcp_socket_connect_timeout_sec = 30;
double NetConfig::client_connect_server_timeout_sec =
    tcp_socket_connect_timeout_sec * 2 + 3;

uint32 NetConfig::wait_completion_timeout_msec = 10;
uint32 NetConfig::server_heartbeat_interval_msec = 5;
uint32 NetConfig::server_user_work_max_wait_msec = 100;
uint32 NetConfig::client_heartbeat_interval_msec = 1;

const double NetConfig::INFINITE_COOLTIME = 60 * 60 * 24 * 365;  // 1 year

double NetConfig::server_holepunch_interval_sec = udp_holepunch_interval_sec;
double NetConfig::server_udp_repunch_interval_sec =
    NetConfig::server_holepunch_interval_sec * 3;
int32 NetConfig::server_udp_repunch_max_attempt_count = 1;
int32 NetConfig::server_udp_holepunch_max_attempt_count = 5;
double NetConfig::p2p_holepunch_interval_sec = udp_holepunch_interval_sec;
double NetConfig::udp_holepunch_interval_sec = 7.3;
int32 NetConfig::p2p_shotgun_start_turn = 10;
int32 NetConfig::p2p_holepunch_max_turn_count = 30;

double NetConfig::purge_too_old_unmature_client_timeout_sec = 12.2;
double NetConfig::purge_too_old_add_member_ack_timeout_sec = 15.3;
double NetConfig::dispose_issued_remote_clients_timeout_sec = 0.2;

uint32 NetConfig::lan_client_heartbeat_interval_msec = 5;
double NetConfig::lan_peer_connect_peer_timeout_sec = 20;

double NetConfig::purge_too_old_joining_timeout_interval_sec = 10;

double NetConfig::remove_lookahead_msg_timeout_interval_sec = 1;

double NetConfig::remove_too_old_recycle_pair_time_sec = 30;
double NetConfig::remove_too_old_recycle_pair_interval_sec = 10;
double NetConfig::recycle_pair_reuse_time_sec = 10;

double NetConfig::GetP2PHolepunchEndTime() {
  return p2p_holepunch_interval_sec * p2p_holepunch_max_turn_count;
}

int32 NetConfig::shotgun_attempt_count = 3;
int32 NetConfig::shotgun_range = 0;

double NetConfig::p2p_ping_interval_sec = udp_ping_interval_sec;
double NetConfig::cs_ping_interval_sec = udp_ping_interval_sec;
double NetConfig::udp_ping_interval_sec = 4.3;

//@maxidea:
// 주의 : 서버에 트래픽을 야기할 수 있으므로, 꼭 필요한 상황이 아니라면 꺼주는게
// 바람직할듯함.
bool NetConfig::report_real_udp_count_enabled = true;

//@warning
// 테스트를 위해서 임시로 인터벌을 짧게 가져감. (원래는 30초였음)
// 통계관련된 부분이므로 report_real_udp_count_enabled를 false로 해서 아예
// 기능을 끌수도 있어야할듯함. 실 서버시에는 그다지 필요없는 기능이므로.. 서버에
// 접속시 설정값을 내려주는것도 좋을듯 싶음.
double NetConfig::report_real_udp_count_interval_sec = 30;  // 30초 마다 보고

const bool NetConfig::speedhack_detector_enabled_by_default =
    false;  // 테스트에 방해가 되어서 일단 막아둠.
const double NetConfig::speedhack_detector_ping_interval_sec = 0.59;

double NetConfig::log_linear_programming_factor = 0.8;

int32 NetConfig::stream_grow_by = 1024;

// TODO 제거하던지, 의미있게 처리하도록 하자.
uint32 NetConfig::InternalNetVersion = 0x00000001;

// TODO 제거하던지, 의미있게 처리하도록 하자.
uint32 NetConfig::InternalLanVersion = 0x00000001;

//주의:
// 이 값을 수정하려거든, C#의 같은 값도 맞춰줘야함.  그렇지 않으면,
// PacketFragging에서 문제가 발생함. PacketFragging시에 MTU 단위로
// fragmentation이 일어나기 때문임.
int32 NetConfig::MTU = 500;

double NetConfig::remove_too_old_udp_send_packet_queue_timeout_sec =
    3 * 60;                                                  // 3분
double NetConfig::assemble_fragged_packet_timeout_sec = 10;  // 10초

double NetConfig::default_graceful_disconnect_timeout_sec =
    2;  // 2초 이상 접속해제 대기

int32 NetConfig::max_s2c_multicast_route_count = 4;

double NetConfig::unreliable_s2c_routed_multicast_max_ping_default = 0.1;

bool NetConfig::force_compressed_relay_dest_list_only = false;

int32 NetConfig::message_max_length = 1024 * 64;

bool NetConfig::message_prioritization_enabled = true;

CCriticalSection2& NetConfig::GetWriteMutex() { return write_mutex; }

int32 NetConfig::default_max_direct_p2p_multicast_count = int32_MAX;

//@maxidea: UDP 관련 테스트 중이라 일단은 끔...
bool NetConfig::upnp_detect_nat_device_by_default = false;
bool NetConfig::upnp_tcp_addr_port_mapping_by_default = false;

double NetConfig::elect_super_peer_interval_sec = 10;

double NetConfig::measure_client_send_speed_interval_sec = 120;

double NetConfig::measure_send_speed_duration_sec = 0.5;

// TODO 테스트임.  구태여 미리 홀펀칭할 이유는 없음.  그러나 과도기이므로
// 테스트를 위해서 미리 켜두는 형태로 처리하자. 추후에 반듯이 JIT로 변경하도록
//하자. DirectP2PStartCondition NetConfig::default_direct_p2p_start_condition =
// DirectP2PStartCondition::Jit;
DirectP2PStartCondition NetConfig::default_direct_p2p_start_condition =
    DirectP2PStartCondition::Always;

bool NetConfig::catch_unhandled_exception = true;

bool NetConfig::networker_thread_priority_is_high = true;

// double NetConfig::report_p2p_group_ping_interval_sec = 4.2;

double NetConfig::report_p2p_peer_ping_test_interval_sec = 30;

double NetConfig::report_server_time_and_ping_interval_sec = 30;

double NetConfig::report_lan_p2p_peer_ping_interval_sec = 5;

double NetConfig::udp_packet_board_long_interval_sec = 1;

double NetConfig::tcp_packet_board_long_interval_sec = 1;

int32 NetConfig::min_send_speed = 5 * 1024;

int32 NetConfig::default_over_send_suspecting_threshold_in_byte = 15 * 1024;

bool NetConfig::send_break_enabled = false;

// bool NetConfig::force_unsafe_heap_to_safe_heap = false;

// TODO 이 값은 제거하던지... 고민이 필요할듯?
double NetConfig::viz_reconnect_try_interval_sec =
    3 + client_connect_server_timeout_sec;

double NetConfig::super_peer_selection_premium = 200;

// TODO 변수명은 변경하는게 좋을듯...
double NetConfig::every_remote_issue_send_on_need_interval_sec = 0.003;

double NetConfig::host_id_recycle_allow_time_sec = 60 * 5;

int32 NetConfig::send_queue_heavy_warning_capacity = 1024 * 1024 * 10;  // 10MB
double NetConfig::send_queue_heavy_warning_time_sec = 30;
double NetConfig::send_queue_heavy_warning_check_cooltime_sec = 10;

double NetConfig::update_net_client_stat_clone_cooltime_sec = 3;

double NetConfig::manager_garbage_free_interval_sec = 1;

int32 NetConfig::manager_average_elapsed_time_sample_count = 20;

bool NetConfig::starvation_warning_enabled = false;

//#define FUN_FORCE_DISABLE_FRAGGING  1

#if FUN_FORCE_DISABLE_FRAGGING
bool NetConfig::conditional_fragging_by_default = false;
#else
bool NetConfig::conditional_fragging_by_default = true;
#endif

bool NetConfig::deadlock_checking_enabled = false;

bool NetConfig::use_is_same_lan_to_local_for_max_direct_p2p_multicast = true;

}  // namespace net
}  // namespace fun
