#include "codec.h"

using namespace fun;
using namespace fun::net;

namespace pubsub {

ParseResult ParseMessage(Buffer* buf, String* cmd, String* topic,
                         String* content) {
  ParseResult result = kError;
  const char* crlf = buf->FindCRLF();
  if (crlf) {
    const char* space = std::find(buf->GetReadablePtr(), crlf, ' ');
    if (space != crlf) {
      cmd->Assign(buf->GetReadablePtr(), space);
      topic->Assign(space + 1, crlf);
      if (*cmd == "pub") {
        const char* start = crlf + 2;
        crlf = buf->FindCRLF(start);
        if (crlf) {
          content->Assign(start, crlf);
          buf->DrainUntil(crlf + 2);
          result = kSuccess;
        } else {
          result = kContinue;
        }
      } else {
        buf->DrainUntil(crlf + 2);
        result = kSuccess;
      }
    } else {
      result = kError;
    }
  } else {
    result = kContinue;
  }
  return result;
}

}  // namespace pubsub
