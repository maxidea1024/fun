#pragma once

#include "Net/Engine/P2PGroup.h"

namespace fun {
namespace net {

class P2PGroupInfo;
class P2PGroup_C;

typedef SharedPtr<P2PGroup_C> P2PGroupPtr_C;
typedef Map<HostId, P2PGroupPtr_C> P2PGroups_C;

class P2PGroup_C {
 public:
  /** 그룹 ID */
  HostId group_id;
  /** 그룹에 소속된 client peer들의 host_id */
  P2PGroupMembers_C members;

  /** Default constructor. */
  P2PGroup_C() : group_id(HostId_None) {}

  void GetInfo(P2PGroupInfo& out_info) const;
};

}  // namespace net
}  // namespace fun
