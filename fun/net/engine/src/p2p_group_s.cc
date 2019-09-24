#include "fun/net/net.h"
#include "P2PGroup_S.h"

namespace fun {
namespace net {

//
// P2PGroup_S
//

void P2PGroup_S::GetInfo(P2PGroupInfo& out_result) const {
  out_result.group_id = group_id;

  out_result.members_.Clear(members_.Count());
  for (const auto& member_pair : members) {
    out_result.members_.Add(member_pair.key);
  }
}

P2PGroupInfoPtr P2PGroup_S::GetInfo() const {
  P2PGroupInfoPtr info(new P2PGroupInfo);
  GetInfo(*info);
  return info;
}

P2PGroup_S::P2PGroup_S()
  : group_id(HostId_None),
    super_peer_selection_policy(SuperPeerSelectionPolicy::GetNull()),
    super_peer_selection_policy_is_valid(false) {
}

bool P2PGroup_S::AddMemberAckWaiterList::AckWaitingItemExists(HostId joining_member_host_id, uint32 event_id) {
  for (int32 i = 0; i < Count(); ++i) {
    const auto& ack_waiter = (*this)[i];

    if (ack_waiter.joining_member_host_id == joining_member_host_id && ack_waiter.event_id == event_id) {
      return true;
    }
  }
  return false;
}

bool P2PGroup_S::AddMemberAckWaiterList::AckWaitingItemExists(HostId joining_member_host_id) {
  for (int32 i = 0; i < Count(); ++i) {
    const auto& ack_waiter = (*this)[i];

    if (ack_waiter.joining_member_host_id == joining_member_host_id) {
      return true;
    }
  }
  return false;
}

bool P2PGroup_S::AddMemberAckWaiterList::RemoveEqualItem(HostId old_member_host_id, HostId new_member_host_id, uint32 event_id) {
  for (int32 i = 0; i < Count(); ++i) {
    auto& ack_waiter = (*this)[i];

    if (ack_waiter.old_member_host_id == old_member_host_id &&
        ack_waiter.joining_member_host_id == new_member_host_id &&
        ack_waiter.event_id == event_id) {
      RemoveAt(i);
      return true;
    }
  }
  return false;
}


//
// LanP2PGroup_S
//

void LanP2PGroup_S::GetInfo(P2PGroupInfo& out_result) const {
  out_result.group_id = group_id;

  out_result.members_.Clear(members_.Count());
  for (const auto& pair : members) {
    out_result.members_.Add(pair.key);
  }
}

P2PGroupInfoPtr LanP2PGroup_S::GetInfo() const {
  P2PGroupInfoPtr info(new P2PGroupInfo);
  GetInfo(*info);
  return info;
}

LanP2PGroup_S::LanP2PGroup_S()
  : group_id(HostId_None), all_peers_connected(false) {}

bool LanP2PGroup_S::AddMemberAckWaiterList::AckWaitingItemExists(HostId joining_member_host_id, uint32 event_id) {
  for (int32 i = 0; i < Count(); ++i) {
    const auto& ack_waiter = (*this)[i];

    if (ack_waiter.joining_member_host_id == joining_member_host_id && ack_waiter.event_id == event_id) {
      return true;
    }
  }
  return false;
}

bool LanP2PGroup_S::AddMemberAckWaiterList::AckWaitingItemExists(HostId joining_member_host_id) {
  for (int32 i = 0; i < Count(); ++i) {
    const auto& ack_waiter = (*this)[i];

    if (ack_waiter.joining_member_host_id == joining_member_host_id) {
      return true;
    }
  }
  return false;
}

bool LanP2PGroup_S::AddMemberAckWaiterList::RemoveEqualItem(
      HostId old_member_host_id, HostId new_member_host_id, uint32 event_id) {
  for (int32 i = 0; i < Count(); ++i) {
    auto& ack_waiter = (*this)[i];

    if (ack_waiter.old_member_host_id == old_member_host_id &&
        ack_waiter.joining_member_host_id == new_member_host_id &&
        ack_waiter.event_id == event_id) {
      RemoveAt(i);
      return true;
    }
  }
  return false;
}

} // namespace net
} // namespace fun
