#pragma once

#include "fun/net/net.h"

namespace fun {
namespace net {

/**
 * RPC reception hint.
 */
class RpcHint {
 public:
  /** Whether you received data with Relay or DirectP2P. */
  bool relayed;

  /** User object pointer associated with the received object. */
  void* host_tag;

  // 아래의 두 필드는 RpcHeader에서 가져옴.

  /** result code. */
  int32 result_code;

  /** Error message. */
  String error_message;

  /** Default constructor. */
  RpcHint();

  /** Construct with relayed and host_tag. */
  RpcHint(bool relayed,
          void* host_tag,
          int32 result_code = 0,
          const String& error_message = String());

  /** Returns whther succeeded or not. */
  bool Ok() const;
};


//
// inlines
//

inline RpcHint::RpcHint()
  : relayed(true)
  , host_tag(nullptr)
  , result_code(0)
  , error_message() {
}

inline RpcHint::RpcHint(bool relayed,
                        void* host_tag,
                        int32 result_code,
                        const String& error_message)
  : relayed(relayed)
  , host_tag(host_tag)
  , result_code(result_code)
  , error_message(error_message) {
}

inline bool RpcHint::Ok() const {
  return result_code == 0;
}

} // namespace net
} // namespace fun
