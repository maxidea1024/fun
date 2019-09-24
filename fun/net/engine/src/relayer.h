#pragma once

namespace fun {
namespace net {

/**
 * Each entry in the 'Relay end recipient list' that enters the relayed message
 */
class RelayDest {
 public:
  /** Target host id */
  HostId send_to;
  /** The frame number used in the reliable UDP layer in case of P2P reliable
   * message. */
  FrameNumber frame_number;
};

/**
 * RelayDest list.
 */
typedef Array<RelayDest> RelayDestList;

inline String ToString(const RelayDest& value) {
  return String::Format("{send_to: %d, frame_number: %d}", (int32)value.send_to,
                        (int32)value.frame_number);
}

// TODO 이건 필요 없을듯 싶음...
inline String ToString(const RelayDestList& value) {
  String result = "[";
  for (int32 i = 0; i < value.Count(); ++i) {
    if (i != 0) result << ",";
    result << ToString(value[i]);
  }
  result << "]";
  return result;
}

}  // namespace net
}  // namespace fun
