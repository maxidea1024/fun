#pragma once

namespace fun {
namespace net {

// Directly connected client information
class RCPair {
 public:
  HostId first;
  HostId second;

  RCPair() : first(HostId_None), second(HostId_None) {}

  RCPair(HostId first, HostId second) : first(first), second(second) {}

  bool operator==(const RCPair& other) const {
    return first == other.first && second == other.second;
  }

  bool operator!=(const RCPair& other) const {
    return first != other.first || second != other.second;
  }

  friend uint32 HashOf(const RCPair& v) {
    return HashOf(v.first) ^ HashOf(v.second);
  }
};

}  // namespace net
}  // namespace fun
