#pragma once

#include "fun/net/net.h"

namespace fun {
namespace net {

/**
 * UDP 전송관련한 옵셥값들입니다.
 */
class UdpSendOption {
 public:
  // 전송 우선순위 (우선순위가 높은것부터 처리하고 그다음 우선순위를 처리함.
  // 낮은 우선순위의 starving은 어떤식으로?)
  MessagePriority priority;
  // Drity state tracking.
  uint64 unique_id;
  // 자기자신에게 전송할지 여부
  bool bounce;
  // ttl(Time To Live). 실제로는 Hop 카운트
  int32 ttl;
  // 엔진 내부 전용임을 표시함.
  //
  // 이 플래그가 설정되면, 내부 통계 카운팅등이 이루어지지 않는등 특수하게 다루어집니다.
  // 엔진 개발자가 아니라면, 이 플래그는 변경해서는 안됩니다.
  bool engine_only_specific;
  // false이면 defragboard를 안타고 MTU 제한 무시하고 바로 소켓으로 들어간다.
  // 릴레이 류는 이것을 쓸 것.
  bool conditional_fragging;

  // 기본 생성자입니다.
  UdpSendOption(),
    : priority(MessagePriority::Last),
      unique_id(0),
      bounce(true),
      ttl(-1),
      engine_only_specific(false),
      conditional_fragging(NetConfig::conditional_fragging_by_default) {}

  // 엔진 내부 전용입니다.
  UdpSendOption(MessagePriority priority, EngineOnlyFeatureTAG)
    : priority(priority),
      unique_id(0),
      bounce(true),
      ttl(-1),
      engine_only_specific(true),
      conditional_fragging(NetConfig::conditional_fragging_by_default) {}

  // SendOption에서 값을 가져와 초기화합니다.
  UdpSendOption(const SendOption& src)
    : priority(src.priority),
      unique_id(src.unique_id),
      bounce(src.bounce),
      ttl(src.ttl),
      engine_only_specific(src.engine_only_specific),
      conditional_fragging(src.conditional_fragging) {}

  /// RpcCallOption에서 값을 가져와 초기화합니다.
  UdpSendOption(const RpcCallOption& src)
    : priority(src.priority),
      unique_id(src.unique_id),
      bounce(src.bounce),
      ttl(-1),
      engine_only_specific(src.engine_only_specific),
      conditional_fragging(src.conditional_fragging) {}
};

} // namespace net
} // namespace fun
