#include "fun/net/net_peer_info.h"

namespace fun {
namespace net {

PeerInfo::PeerInfo()
  : real_udp_enabled(false),
    relayed_p2p(true),
    host_id(HostId_None),
    udp_addr_from_server(InetAddress::None),
    udp_addr_internal(InetAddress::None),
    recent_ping(0),
    send_queued_amount_in_byte(0),
    behind_nat(false),
    host_tag(nullptr),
    direct_p2p_peer_frame_rate(0),
    to_remote_peer_send_udp_message_attempt_count(0),
    to_remote_peer_send_udp_message_success_count(0) {}

String PeerInfo::ToString(bool at_server) {
  if (at_server) {
    return String::Format("{host_id: %d, joined_p2p_group_count: %d, is_behind_nat: %d, real_udp_enabled: %d, to_remote_peer_send_udp_message_attempt_count: %u, to_remote_peer_send_udp_message_success_count: %u}",
          (int32)host_id,
          joined_p2p_groups_.Count(),
          behind_nat,
          real_udp_enabled,
          to_remote_peer_send_udp_message_attempt_count,
          to_remote_peer_send_udp_message_success_count
        );
  } else {
    return String::Format("{host_id: %d, relayed_p2p: %d, joined_p2p_group_count: %d, is_behind_nat: %d, real_udp_enabled: %d}",
          (int32)host_id,
          relayed_p2p,
          joined_p2p_groups_.Count(),
          behind_nat,
          real_udp_enabled
        );
  }
}

} // namespace net
} // namespace fun
