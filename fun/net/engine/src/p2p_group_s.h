#pragma once

#include "Net/Engine/P2PGroup.h"

namespace fun {
namespace net {

class P2PGroupInfo;
class RemoteClient_S;
class LanClient_S;
class P2PGroup_S;
class LanP2PGroup_S;
typedef SharedPtr<P2PGroup_S> P2PGroupPtr_S;
typedef SharedPtr<LanP2PGroup_S> LanP2PGroupPtr_S;

typedef Map<HostId, P2PGroupPtr_S> P2PGroups_S;
typedef Map<HostId, LanP2PGroupPtr_S> LanP2PGroups_S;

class JoinedP2PGroupInfo {
 public:
  P2PGroupPtr_S group_ptr;
};

typedef Map<HostId, JoinedP2PGroupInfo> JoinedP2PGroups_S;

class JoinedLanP2PGroupInfo {
 public:
  LanP2PGroupPtr_S lan_group_ptr;
};

typedef Map<HostId, JoinedLanP2PGroupInfo> JoinedLanP2PGroups_S;

/**
 * 서버 또는 RC가 이 객체를 상속받는다.
 */
class P2PGroupMemberBase_S {
 public:
  /** 이 P2P 멤버가 참여하고 있던 P2P 그룹들 */
  JoinedP2PGroups_S joined_p2p_groups_;

  virtual HostId GetHostId() const = 0;

  virtual ~P2PGroupMemberBase_S() {}
};

/**
 * Lan 서버 또는 LC가 상속받음
 */
class LanP2PGroupMemberBase_S {
 public:
  /** 이 P2P 멤버가 참여하고 있던 P2P 그룹들 */
  JoinedLanP2PGroups_S joined_lan_p2p_groups_;

  virtual HostId GetHostId() const = 0;

  virtual ~LanP2PGroupMemberBase_S() {}
};

class P2PGroupMember_S {
 public:
  double joined_time_;
  P2PGroupMemberBase_S* ptr_;
};

class LanP2PGroupMember_S {
 public:
  double joined_time_;
  LanP2PGroupMemberBase_S* ptr_;
};

// 클라에서 unreliable relay가 왔을때 G,~a,~b,~c로부터 차집합을 얻는 과정이
// 있다. 거기서 두 집합이 정렬되어 있으면 merge sort와 비슷한 방법으로 빨리
// 차집합을 얻을 수 있다. 그래서 이 map은 ordered이어야 하고, 그것이 std.map을
// 쓰는 이유다.

// TODO
// TOrderedMap으로 교체해야함!!
// TOrderedMap으로 교체해야함!!
// TOrderedMap으로 교체해야함!!
// TOrderedMap으로 교체해야함!!
// TOrderedMap으로 교체해야함!!
// TOrderedMap으로 교체해야함!!
// TOrderedMap으로 교체해야함!!
// TOrderedMap으로 교체해야함!!
// TOrderedMap으로 교체해야함!!
// TOrderedMap으로 교체해야함!!
// TOrderedMap으로 교체해야함!!
// TOrderedMap으로 교체해야함!!
// TOrderedMap으로 교체해야함!!
// TOrderedMap으로 교체해야함!!
// TOrderedMap으로 교체해야함!!
// TOrderedMap으로 교체해야함!!
// TOrderedMap으로 교체해야함!!
// TOrderedMap으로 교체해야함!!
// TOrderedMap으로 교체해야함!!
// TOrderedMap으로 교체해야함!!
// TOrderedMap으로 교체해야함!!
// typedef std::map<HostId, P2PGroupMember_S> P2PGroupMembers_S;
typedef Map<HostId, P2PGroupMember_S> P2PGroupMembers_S;
typedef Map<HostId, LanP2PGroupMember_S> LanP2PGroupMembers_S;

class P2PGroup_S {
 public:
  struct AddMemberAckWaiter {
    HostId joining_member_host_id;
    HostId old_member_host_id;  // 이 피어에서 새 멤버에 대한 OnP2PMemberJoined
                                // 콜백이 있어야 한다.
    uint32 event_id;
    double event_time;  // 너무 오랫동안 보유되면 제거하기 위함
  };

  /* 그룹 ID */
  HostId group_id_;

  /* 그룹에 소속된 Client peer들의 host_id */
  P2PGroupMembers_S members_;

  /* P2P 그룹이 super peer를 놓고 통신할 경우 가장 적합하다고 추정되는 순으로
   * 정렬된 host_id 리스트 */
  Array<super_peer_rating_> ordered_super_peer_suitables_;

  bool super_peer_selection_policy_is_valid_;
  SuperPeerSelection_policy super_peer_selection_policy_;

  // P2PGroup add ack가 도착할 때까지 기다리고 있는 것들의 목록
  struct AddMemberAckWaiterList : public Array<AddMemberAckWaiter> {
    bool RemoveEqualItem(HostId old_member_host_id, HostId new_member_host_id,
                         uint32 event_id);
    bool AckWaitingItemExists(HostId joining_member_host_id, uint32 event_id);
    bool AckWaitingItemExists(HostId joining_member_host_id);
  };
  AddMemberAckWaiterList add_member_ack_waiters_;

  // FunNet.NetServerImpl.CreateP2PGroup의 파라메터로부터 전파받음
  P2PGroupOption option_;

  void GetInfo(P2PGroupInfo& out_info) const;
  P2PGroupInfoPtr GetInfo() const;

  P2PGroup_S();
};

/**
 * Lan(서버간 통신용) P2PGroup_S
 */
class LanP2PGroup_S {
 public:
  struct AddMemberAckWaiter {
    HostId joining_member_host_id;
    HostId old_member_host_id; /** 이 피어에서 새 멤버에 대한 OnP2PMemberJoined
                                  콜백이 있어야 한다. */
    uint32 event_id;
    double event_time; /** 너무 오랫동안 보유되면 제거하기 위함 */
  };

  /** 그룹 ID */
  HostId group_id_;

  /** 그룹에 소속된 Client peer들의 host_id */
  LanP2PGroupMembers_S members_;

  /** 그룹의 모든 peer의 연결이 성사되었는지에 대한 여부 */
  bool all_peers_connected_;

  /** P2PGroup add ack가 도착할 때까지 기다리고 있는 것들의 목록 */
  struct AddMemberAckWaiterList : public Array<AddMemberAckWaiter> {
    bool RemoveEqualItem(HostId old_member_host_id, HostId new_member_host_id,
                         uint32 event_id);
    bool AckWaitingItemExists(HostId joining_member_host_id, uint32 event_id);
    bool AckWaitingItemExists(HostId joining_member_host_id);
  };
  AddMemberAckWaiterList add_member_ack_waiters_;

  void GetInfo(P2PGroupInfo& out_info) const;
  P2PGroupInfoPtr GetInfo() const;

  LanP2PGroup_S();
};

}  // namespace net
}  // namespace fun
