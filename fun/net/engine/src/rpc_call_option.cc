#include "fun/net/net.h"

namespace fun {
namespace net {

// Reliable UDP의 resend starvation을 막으려면 이건 우선순위가 unreliable보다
// 높아야 한다.
RpcCallOption RpcCallOption::Reliable(MessagePriority::High,
                                      MessageReliability::Reliable, 0,
                                      EncryptionMode::None);
RpcCallOption RpcCallOption::Unreliable(MessagePriority::Medium,
                                        MessageReliability::Unreliable, 0,
                                        EncryptionMode::None);

RpcCallOption RpcCallOption::WeakSecureReliable(MessagePriority::High,
                                                MessageReliability::Reliable, 0,
                                                EncryptionMode::Weak);
RpcCallOption RpcCallOption::WeakSecureUnreliable(
    MessagePriority::Medium, MessageReliability::Unreliable, 0,
    EncryptionMode::Weak);

RpcCallOption RpcCallOption::StrongSecureReliable(MessagePriority::High,
                                                  MessageReliability::Reliable,
                                                  0, EncryptionMode::Strong);
RpcCallOption RpcCallOption::StrongSecureUnreliable(
    MessagePriority::Medium, MessageReliability::Unreliable, 0,
    EncryptionMode::Strong);

RpcCallOption RpcCallOption::UnreliableS2CRoutedMulticast(
    MessagePriority::Medium, MessageReliability::Unreliable,
    NetConfig::max_s2c_multicast_route_count, EncryptionMode::None);

RpcCallOption GetReliableSend_INTERNAL(EncryptionMode encryption_mode) {
  RpcCallOption ret(MessagePriority::High, MessageReliability::Reliable, 0,
                    encryption_mode);

  // TEMP
  // ret.encryption_mode = EncryptionMode::None;

  ret.enable_p2p_jit_trigger = false;  //@remarks
  ret.engine_only_specific = true;     //@remarks

  return ret;
}

RpcCallOption GetUnreliableSend_INTERNAL(EncryptionMode encryption_mode) {
  RpcCallOption ret(MessagePriority::Medium, MessageReliability::Unreliable, 0,
                    encryption_mode);

  // TEMP
  // ret.encryption_mode = EncryptionMode::None;

  ret.enable_p2p_jit_trigger = false;  //@remarks
  ret.engine_only_specific = true;     //@remarks

  return ret;
}

RpcCallOption GReliableSend_INTERNAL =
    GetReliableSend_INTERNAL(EncryptionMode::None);
RpcCallOption GUnreliableSend_INTERNAL =
    GetUnreliableSend_INTERNAL(EncryptionMode::None);

RpcCallOption GSecureReliableSend_INTERNAL =
    GetReliableSend_INTERNAL(EncryptionMode::Strong);
RpcCallOption GSecureUnreliableSend_INTERNAL =
    GetUnreliableSend_INTERNAL(EncryptionMode::Strong);

void RpcCallOption::AssureValidation() const {
  if (Reliability == MessageReliability::Unreliable) {
    if (priority < MessagePriority::High || priority > MessagePriority::Low) {
      throw Exception("RPC messaging cannot have engine level priority.");
    }
  }
}

}  // namespace net
}  // namespace fun
