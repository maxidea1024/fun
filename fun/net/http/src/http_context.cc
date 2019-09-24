#include "fun/net/http/context.h"

namespace fun {
namespace http {

// TODO body handling
bool Context::ParseRequest(Buffer* buf, const Timestamp& received_time) {
  bool ok = true;
  bool has_more = true;
  while (has_more) {
    if (state_ == kExpectRequestLine) {
      const char* crlf = buf->FindCRLF();
      if (crlf) {
        ok = ProcessRequestLine(buf->Peek(), crlf);
        if (ok) {
          request_.SetReceiveTime(receiveTime);
          buf->RetrieveUntil(crlf + 2);
          state_ = kExpectHeaders;
        } else {
          has_more = false;
        }
      } else {
        has_more = false;
      }
    } else if (state_ == kExpectHeaders) {
      const char* crlf = buf->FindCRLF();
      if (crlf) {
        const char* colon = std::find(buf->Peek(), crlf, ':');
        if (colon != crlf) {
          request_.AddHeader(buf->Peek(), colon, crlf);
        } else {
          // empty line, end of header
          // FIXME:
          state_ = kGotAll;
          has_more = false;
        }
        buf->RetrieveUntil(crlf + 2);
      } else {
        has_more = false;
      }
    } else if (state_ == kExpectBody) {
      // FIXME:
    }
  }

  return ok;
}

bool Context::ProcessRequestLine(const char* begin, const char* end) {
  bool succeeded = false;
  const char* start = begin;
  const char* space = std::find(start, end, ' ');
  if (space != end && request_.SetMethod(start, space)) {
    start = space + 1;
    space = std::find(start, end, ' ');
    if (space != end) {
      const char* question = std::find(start, space, '?');
      if (question != space) {
        request_.SetPath(start, question);
        request_.SetQuery(question, space);
      } else {
        request_.SetPath(start, space);
      }

      start = space + 1;

      succeeded = (end - start) == 8 && std::equal(start, end - 1, "HTTP/1.");
      if (succeeded) {
        if (*(end - 1) == '1') {
          request_.SetVersion(Version::Http11);
        } else if (*(end - 1) == '0') {
          request_.SetVersion(Version::Http10);
        } else {
          // unsupported http protoco version.
          succeeded = false;
        }
      }
    }
  }

  return succeeded;
}

}  // namespace http
}  // namespace fun
