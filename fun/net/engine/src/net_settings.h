#pragma once

namespace fun {
namespace net {

/**
최초 접속시 서버에서 받는 내용임.
*/
class NetSettings {
 public:
  FallbackMethod fallback_method;
  OptimalCounter32 message_max_length;
  double default_timeout_sec;
  DirectP2PStartCondition direct_p2p_start_condition;
  OptimalCounter32 over_send_suspecting_threshold_in_byte;
  bool bEnableNagleAlgorithm;
  // AES 대칭키의 사이즈(bit) 입니다. 128, 192, 256bit만을 지원합니다.
  OptimalCounter32 strong_encrypted_message_key_length;
  // RC4 대칭키의 사이즈(bit) 입니다. 최대 0,512,1024,2048을 지원합니다. 키값이
  // 길어도 속도에 지장을 주지 않습니다.
  OptimalCounter32 weak_encrypted_message_key_length;
  bool p2p_encrypted_messaging_enabled;
  // Server 를 P2P 그룹에 포함시킬 것인지에 대한 여부. 기본 false이다.
  bool server_as_p2p_group_member_allowed;
  // true이면 NAT router를 찾기를 시도함
  bool upnp_detect_nat_device;
  bool upnp_tcp_addr_port_mapping;
  OptimalCounter32 emergency_log_line_count;
  bool look_ahead_p2p_send_enabled;
  bool ping_test_enabled;
  bool ignore_failed_bind_port;

  NetSettings();

  void Reset();
};

template <>
struct MessageFieldTypeTraits<NetSettings> {
  typedef NetSettings CppValueType;

  static void Write(IMessageOut& output, const CppValueType& value) {
    LiteFormat::Write(output, value.fallback_method);
    LiteFormat::Write(output, value.message_max_length);
    LiteFormat::Write(output, value.default_timeout_sec);
    LiteFormat::Write(output, value.direct_p2p_start_condition);
    LiteFormat::Write(output, value.over_send_suspecting_threshold_in_byte);
    LiteFormat::Write(output, value.bEnableNagleAlgorithm);
    LiteFormat::Write(output, value.strong_encrypted_message_key_length);
    LiteFormat::Write(output, value.weak_encrypted_message_key_length);
    LiteFormat::Write(output, value.server_as_p2p_group_member_allowed);
    LiteFormat::Write(output, value.p2p_encrypted_messaging_enabled);
    LiteFormat::Write(output, value.upnp_detect_nat_device);
    LiteFormat::Write(output, value.upnp_tcp_addr_port_mapping);
    LiteFormat::Write(output, value.look_ahead_p2p_send_enabled);
    LiteFormat::Write(output, value.ping_test_enabled);
    LiteFormat::Write(output, value.ignore_failed_bind_port);
    LiteFormat::Write(output, value.emergency_log_line_count);
  }

  static bool Read(IMessageIn& input, CppValueType& out_value) {
    FUN_DO_CHECKED(LiteFormat::Read(input, out_value.fallback_method));
    FUN_DO_CHECKED(LiteFormat::Read(input, out_value.message_max_length));
    FUN_DO_CHECKED(LiteFormat::Read(input, out_value.default_timeout_sec));
    FUN_DO_CHECKED(
        LiteFormat::Read(input, out_value.direct_p2p_start_condition));
    FUN_DO_CHECKED(LiteFormat::Read(
        input, out_value.over_send_suspecting_threshold_in_byte));
    FUN_DO_CHECKED(LiteFormat::Read(input, out_value.bEnableNagleAlgorithm));
    FUN_DO_CHECKED(
        LiteFormat::Read(input, out_value.strong_encrypted_message_key_length));
    FUN_DO_CHECKED(
        LiteFormat::Read(input, out_value.weak_encrypted_message_key_length));
    FUN_DO_CHECKED(
        LiteFormat::Read(input, out_value.server_as_p2p_group_member_allowed));
    FUN_DO_CHECKED(
        LiteFormat::Read(input, out_value.p2p_encrypted_messaging_enabled));
    FUN_DO_CHECKED(LiteFormat::Read(input, out_value.upnp_detect_nat_device));
    FUN_DO_CHECKED(
        LiteFormat::Read(input, out_value.upnp_tcp_addr_port_mapping));
    FUN_DO_CHECKED(
        LiteFormat::Read(input, out_value.look_ahead_p2p_send_enabled));
    FUN_DO_CHECKED(LiteFormat::Read(input, out_value.ping_test_enabled));
    FUN_DO_CHECKED(LiteFormat::Read(input, out_value.ignore_failed_bind_port));
    FUN_DO_CHECKED(LiteFormat::Read(input, out_value.emergency_log_line_count));
    return true;
  }
};

inline String ToString(const NetSettings& value) {
  String ret(512, ReservationInit);
  ret << "{");
  ret << "fallback_method: " << ToString(value.fallback_method);
  ret << ", message_max_length: " << ToString(value.message_max_length);
  ret << ", default_timeout_sec:" << ToString(value.default_timeout_sec);
  ret << ", direct_p2p_start_condition: "
      << ToString(value.direct_p2p_start_condition);
  ret << ", over_send_suspecting_threshold_in_byte: "
      << ToString(value.over_send_suspecting_threshold_in_byte);
  ret << ", enable_nagle_algorithm: " << ToString(value.bEnableNagleAlgorithm);
  ret << ", strong_encrypted_message_key_length: "
      << ToString(value.strong_encrypted_message_key_length);
  ret << ", weak_encrypted_message_key_length: "
      << ToString(value.weak_encrypted_message_key_length);
  ret << ", enable_p2p_encrypted_messaging: "
      << ToString(value.p2p_encrypted_messaging_enabled);
  ret << ", allow_server_as_p2p_group_member: "
      << ToString(value.server_as_p2p_group_member_allowed);
  ret << ", upnp_detect_nat_device: " << ToString(value.upnp_detect_nat_device);
  ret << ", upnp_tcp_add_port_mapping: "
      << ToString(value.upnp_tcp_addr_port_mapping);
  ret << ", emergency_log_line_count: "
      << ToString(value.emergency_log_line_count);
  ret << ", enable_lookahead_p2p_send: "
      << ToString(value.look_ahead_p2p_send_enabled);
  ret << ", enable_ping_test: " << ToString(value.ping_test_enabled);
  ret << ", ignore_failed_bind_port: "
      << ToString(value.ignore_failed_bind_port);
  ret << "}");
  return ret;
}

}  // namespace net
}  // namespace fun
