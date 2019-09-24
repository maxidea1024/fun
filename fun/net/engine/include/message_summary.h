#pragma once

#include "fun/net/net.h"

namespace fun {
namespace net {

/**
 * Message summary.
 */
class MessageSummary {
 public:
  /** Length of payload(byte unit) */
  int32 payload_length;

  /** ID of RPC */
  RpcId rpc_id;

  /** Name of RPC */
  const char* rpc_name;

  /** Encryption mode */
  EncryptionMode encryption_mode;

  /** Compression mode */
  CompressionMode compression_mode;

  /** arguments string */
  String arguments;

  /** Default empty constructor. */
  inline MessageSummary() {}

  /** Detailed constructor. */
  inline MessageSummary(int32 payload_length, RpcId rpc_id,
                        const char* rpc_name, EncryptionMode encryption_mode,
                        CompressionMode compression_mode,
                        const String& arguments)
      : payload_length(payload_length),
        rpc_id(rpc_id),
        rpc_name(rpc_name),
        encryption_mode(encryption_mode),
        compression_mode(compression_mode),
        arguments(arguments) {}
};

/**
 * Summary before RPC call
 */
class BeforeRpcSummary {
 public:
  /** ID of RPC */
  RpcId rpc_id;

  /** Name of RPC */
  const char* rpc_name;

  /** Host ID */
  HostId host_id;

  /** Host Tag */
  void* host_tag;

  /** Default empty constructor. */
  inline BeforeRpcSummary() {}

  /** Detailed constructor. */
  inline BeforeRpcSummary(RpcId rpc_id, const char* rpc_name, HostId host_id,
                          void* host_tag)
      : rpc_id(rpc_id),
        rpc_name(rpc_name),
        host_id(host_id),
        host_tag(host_tag) {}
};

/**
 * Summary after RPC call
 */
class AfterRpcSummary {
 public:
  /** ID of RPC */
  RpcId rpc_id;

  /**
   * Name of RPC(Since the string pointer allocated by the compiler is used,
   * it does not need to be in the form of string)
   */
  const char* rpc_name;

  /** Host ID */
  HostId host_id;

  /** Host Tag */
  void* host_tag;

  /** Elapsed time */
  uint32 elapsed_time;

  /** Default empty constructor. */
  inline AfterRpcSummary() {}

  /** Detailed constructor. */
  inline AfterRpcSummary(RpcId rpc_id, const char* rpc_name, HostId host_id,
                         void* host_tag, uint32 elapsed_time)
      : rpc_id(rpc_id),
        rpc_name(rpc_name),
        host_id(host_id),
        host_tag(host_tag),
        elapsed_time(elapsed_time) {}
};

}  // namespace net
}  // namespace fun
