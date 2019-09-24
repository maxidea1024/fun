#pragma once

#include "fun/net/net.h"

namespace fun {
namespace net {

typedef SharedPtr<class P2PGroupInfo> P2PGroupInfoPtr;

class P2PGroupInfo {
 public:
  /** Group ID */
  HostId group_id;
  /** Member list */
  HostIdSet members;

  inline P2PGroupInfo()
    : group_id(HostId_None), members() {}
};

class P2PGroupInfos : public Map<HostId, P2PGroupInfoPtr> {};
//전방선언 문제로 인해서, 아래와 같은 선언은 유효하지 않음.
//typedef Map<HostId, P2PGroupInfoPtr> P2PGroupInfos;

} // namespace net
} // namespace fun
