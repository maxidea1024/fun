#pragma once

namespace fun {
namespace net {

typedef Map<HostId, class IP2PGroupMember*> P2PGroupMembers_C;

class ISendDest_C {
 public:
  static ISendDest_C None;

  virtual ~ISendDest_C() {}

  virtual HostId GetHostId() const { return HostId_None; }

  // note sort에서 포인터 타입이 안되는 문제가 있어서 임시로.
  bool operator==(const ISendDest_C& other) const {
    return GetHostId() == other.GetHostId();
  }
  bool operator<(const ISendDest_C& other) const {
    return GetHostId() < other.GetHostId();
  }
};

class IP2PGroupMember {
 public:
  virtual ~IP2PGroupMember() {}

  virtual HostId GetHostId() const = 0;
  virtual double GetIndirectServerTimeDiff() = 0;
};

}  // namespace net
}  // namespace fun
