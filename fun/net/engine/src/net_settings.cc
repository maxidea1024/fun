#include "fun/net/net.h"
#include "Net/Engine/NetSettings.h"

namespace fun {
namespace net {

NetSettings::NetSettings() {
  Reset();
}

void NetSettings::Reset() {
  default_timeout_sec = NetConfig::GetDefaultNoPingTimeoutTime();
  direct_p2p_start_condition = NetConfig::default_direct_p2p_start_condition;
  fallback_method = FallbackMethod::None;
  message_max_length = NetConfig::MessageMaxLengthInOrdinaryCase;
  over_send_suspecting_threshold_in_byte = NetConfig::default_over_send_suspecting_threshold_in_byte;
  //@remarks Nagle 알고리즘을 활성화 했을경우, 문제가 발생할 수 있다함.
  //         직접처리하는게 더 효율적이다. 이건가..
  //         FunNet affords better anti-silly window syndrome technology than TCP Nagle Algorithm.
  bEnableNagleAlgorithm = false;
  server_as_p2p_group_member_allowed = false;
  p2p_encrypted_messaging_enabled = true;
  look_ahead_p2p_send_enabled = true;
  emergency_log_line_count = 0;
  // 128, 192, 256 bit 만을 지원합니다.
  strong_encrypted_message_key_length = (int32)StrongEncryptionLevel::Low;
  // 스트림형 암호방식으로 최대 2048을 지원합니다.
  // 기본적으로 꺼놈... 어짜피 서버 기동시에 파라메터에 설정하므로, 거기에서 설정 하도록 하자.
  weak_encrypted_message_key_length = (int32)WeakEncryptionLevel::Low;
  upnp_detect_nat_device = NetConfig::upnp_detect_nat_device_by_default;
  upnp_tcp_addr_port_mapping = NetConfig::upnp_tcp_addr_port_mapping_by_default;
  ping_test_enabled = false;
  ignore_failed_bind_port = false;
}

} // namespace net
} // namespace fun
