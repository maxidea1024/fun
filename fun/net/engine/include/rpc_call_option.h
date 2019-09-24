#pragma once

#include "fun/net/net.h"

namespace fun {
namespace net {

/**
 * TODO
 */
class RpcCallOption {
 public:
  /** If unreliable send, can perform routed multicast. This is where to decide
   * how many of them are allowed. */
  int32 unrealiable_s2c_routed_multicast_max_count;

  /** If unreliable messaging, can perform routed multicast. This is where to
   * decide which rate of laggy P2P communications are allowed. */
  double unreliable_s2c_routed_multicast_max_ping;

  /** The maximum number of direct multicast can be performed */
  int32 max_direct_p2p_multicast_count;

  /** Unique ID for Transmitting the latest message only. */
  uint64 unique_id;

  /** Message trnasmission priority. */
  MessagePriority priority;

  /** Message trnasmission method. */
  MessageReliability reliability;

  /** While this value is false if there are 2 or more received_msg receivers
   * then excludes the messaging to itself(loopback). Default is true. */
  bool bounce;

  /** TODO 제거하자... 구태야 옵션으로 처리할 필요가 있을까?? */
  bool enable_p2p_jit_trigger;

  /** While this value is false if opponent is relay mode then do not send it.
   */
  bool allow_relayed_send;

  /** Forced relay critical rate value. If you change this value, it can select
   * relay instead of P2P communication when relay is faster than P2P
   * communication. */
  double force_relay_threshold_ratio;

  /**
   * Identifying value that message for only FunNet or not.
   *
   * @warning Do not change this value. (System internal usage only!)
   */
  bool engine_only_specific;

  /** Encryption mode. */
  EncryptionMode encryption_mode;

  /** Compression mode. */
  CompressionMode compression_mode;

  /** */
  bool conditional_fragging;

  /**
   * Default constructor.
   */
  RpcCallOption();

  /**
   * Construct with parameters.
   */
  RpcCallOption(MessagePriority priority, MessageReliability reliability,
                int32 unrealiable_s2c_routed_multicast_max_count,
                EncryptionMode encryption_mode);

  //
  // Predefined values
  //

  FUN_NETX_API static RpcCallOption Reliable;
  FUN_NETX_API static RpcCallOption WeakSecureReliable;
  FUN_NETX_API static RpcCallOption StrongSecureReliable;
  FUN_NETX_API static RpcCallOption Unreliable;
  FUN_NETX_API static RpcCallOption WeakSecureUnreliable;
  FUN_NETX_API static RpcCallOption StrongSecureUnreliable;
  FUN_NETX_API static RpcCallOption UnreliableS2CRoutedMulticast;

  // Debugging helper
  FUN_NETX_API void AssureValidation() const;
};

//
// inlines
//

inline RpcCallOption::RpcCallOption()
    : bounce(true),
      enable_p2p_jit_trigger(true),
      priority(MessagePriority::Medium),
      Reliability(MessageReliability::Reliable),
      unrealiable_s2c_routed_multicast_max_count(0),
      unreliable_s2c_routed_multicast_max_ping(
          NetConfig::unreliable_s2c_routed_multicast_max_ping_default),
      max_direct_p2p_multicast_count(
          NetConfig::default_max_direct_p2p_multicast_count),
      unique_id(0),
      allow_relayed_send(true),
      force_relay_threshold_ratio(0),
      engine_only_specific(false),
      encryption_mode(EncryptionMode::None),
      compression_mode(CompressionMode::None),
      conditional_fragging(NetConfig::conditional_fragging_by_default) {}

inline RpcCallOption::RpcCallOption(
    MessagePriority priority, MessageReliability reliability,
    int32 unrealiable_s2c_routed_multicast_max_count,
    EncryptionMode encryption_mode)
    : priority(priority),
      reliability(reliability),
      encryption_mode(encryption_mode),
      bounce(true),
      enable_p2p_jit_trigger(true),
      unrealiable_s2c_routed_multicast_max_count(
          unrealiable_s2c_routed_multicast_max_count),
      unreliable_s2c_routed_multicast_max_ping(
          NetConfig::unreliable_s2c_routed_multicast_max_ping_default),
      max_direct_p2p_multicast_count(
          NetConfig::default_max_direct_p2p_multicast_count),
      unique_id(0),
      allow_relayed_send(true),
      force_relay_threshold_ratio(0),
      engine_only_specific(false),
      compression_mode(CompressionMode::None),
      conditional_fragging(NetConfig::conditional_fragging_by_default) {}

}  // namespace net
}  // namespace fun
