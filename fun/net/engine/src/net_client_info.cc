#include "CorePrivatePCH.h"
#include "fun/net/net.h"

namespace fun {
namespace net {

NetClientInfo::NetClientInfo()
    : real_udp_enabled(false),
      relayed_p2p(true),
      host_id(HostId_None),
      tcp_addr_from_server(InetAddress::None),
      udp_addr_from_server(InetAddress::None),
      udp_addr_internal(InetAddress::None),
      recent_ping(0),
      send_queued_amount_in_byte(0),
      behind_nat(false),
      host_tag(nullptr),
      recent_frame_rate(0),
      host_id_recycle_count(0),
      to_server_send_udp_message_attempt_count(0),
      to_server_send_udp_message_success_count(0) {}

String NetClientInfo::ToString(bool at_server) const {
  String result;

  if (at_server) {
    return String::Format(
        TEXT("{host_id: %d, tcp_addr_from_server: %s, joined_p2p_group_count: "
             "%d, is_behind_nat: %d, real_udp_enabled: %d, nat_device_name: "
             "\"%s\", host_id_recycle_count: %u, "
             "to_server_send_udp_message_attempt_count: %u, "
             "to_server_send_udp_message_success_count: %u}"),
        (int32)host_id, *tcp_addr_from_server.ToString(),
        joined_p2p_groups_.Count(), behind_nat, real_udp_enabled,
        *nat_device_name, host_id_recycle_count,
        to_server_send_udp_message_attempt_count,
        to_server_send_udp_message_success_count);
  } else {
    return String::Format(
        TEXT(
            "{host_id: %d, relayed_p2p: %d, joined_p2p_group_count: %d, "
            "is_behind_nat: %d, real_udp_enabled: %d, nat_device_name: \"%s\""),
        (int32)host_id, relayed_p2p, joined_p2p_groups_.Count(), behind_nat,
        real_udp_enabled, *nat_device_name);
  }
}

}  // namespace net
}  // namespace fun
