#pragma once

#include "fun/net/net.h"

namespace fun {
namespace net {

/**
 * RPC header.
 */
class RpcHeader {
 public:
  FUN_NETX_API static RpcHeader Ok;

  /** result code(0=OK) */
  int32 result_code;

  /** Error message */
  String error_message;

  /** Default empty constructor. */
  RpcHeader();

  /** Construct with each field values. */
  RpcHeader(int32 result_code, const String& error_message);

  /** Copy constructor. */
  RpcHeader(const RpcHeader& rhs);

  /** Assignment operator. */
  RpcHeader& operator=(const RpcHeader& rhs);

  /** Reads member field from the stream. */
  FUN_NETX_API bool Read(IMessageIn& input);

  FUN_NETX_API static void Write(IMessageOut& output, int32 result_code,
                                 const String& error_message);

  FUN_NETX_API static void WriteOk(IMessageOut& output);
};

//
// inlines
//

inline RpcHeader::RpcHeader() : result_code(0), error_message() {}

inline RpcHeader::RpcHeader(int32 result_code, const String& error_message)
    : result_code(result_code), error_message(error_message) {}

inline RpcHeader::RpcHeader(const RpcHeader& rhs)
    : result_code(rhs.result_code), error_message(rhs.error_message) {}

inline RpcHeader& RpcHeader::operator=(const RpcHeader& rhs) {
  if (FUN_LIKELY(&rhs != this)) {
    result_code = rhs.result_code;
    error_message = rhs.error_message;
  }
  return *this;
}

}  // namespace net
}  // namespace fun
