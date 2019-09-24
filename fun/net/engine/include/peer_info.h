#pragma once

#include "fun/net/net.h"

namespace fun {
namespace net {

typedef SharedPtr<class PeerInfo> NetPeerInfoPtr;

class PeerInfo {
 public:
  InetAddress udp_addr_from_server;
  InetAddress udp_addr_internal;
  HostId host_id;
  bool relayed_p2p;
  HostIdSet joined_p2p_groups;
  bool behind_nat;
  bool real_udp_enabled;
  double recent_ping;
  int32 send_queued_amount_in_byte;
  void* host_tag;
  double direct_p2p_peer_frame_rate;
  int32 to_remote_peer_send_udp_message_attempt_count;
  int32 to_remote_peer_send_udp_message_success_count;

  FUN_NETX_API PeerInfo();
  FUN_NETX_API String ToString(bool at_server);
};

} // namespace net
} // namespace fun
