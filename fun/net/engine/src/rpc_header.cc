#include "fun/net/net.h"

namespace fun {
namespace net {

using lf = LiteFormat;

RpcHeader RpcHeader::Ok(0, "");

bool RpcHeader::Read(IMessageIn& input) {
  FUN_DO_CHECKED(lf::ReadOptimalInt32(input, result_code));
  FUN_DO_CHECKED(lf::Read(input, error_message));
  return true;
}

void RpcHeader::Write(IMessageOut& output,
                      int32 result_code,
                      const String& error_message) {
  lf::WriteOptimalInt32(output, result_code);
  lf::Write(output, error_message);
}

void RpcHeader::WriteOk(IMessageOut& output) {
  output.WriteFixed8(0); // OK
  output.WriteFixed8(0); // Empty error message (zero-length)
}

} // namespace net
} // namespace fun
