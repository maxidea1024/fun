#pragma once

#include "fun/net/reactor/http/http_request.h"

namespace fun {
namespace net {

class Buffer;

class HttpContext {
 public:
  enum HttpRequestParseState {
    kExpectRequestLine,
    kExpectHeaders,
    kExpectBody,
    kGotAll,
  };

  HttpContext() : state_(kExpectRequestLine) {}

  // default copy-ctor, dtor and assignment are fine

  // return false if any error
  bool ParseRequest(Buffer* buf, const Timestamp& received_time);

  bool GotAll() const {
    return state_ == kGotAll;
  }

  void Reset() {
    state_ = kExpectRequestLine;
    HttpRequest dummy;
    request_.swap(dummy);
  }

  const HttpRequest& GetRequest() const {
    return request_;
  }

  HttpRequest& GetRequest() {
    return request_;
  }

 private:
  bool ProcessRequestLine(const char* begin, const char* end);

  HttpRequestParseState state_;
  HttpRequest request_;
};

} // namespace net
} // namespace fun
