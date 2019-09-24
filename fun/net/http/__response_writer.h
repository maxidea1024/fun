#pragma once

namespace fun {
namespace http {

class ResponseWriter : public Response {
 public:
  static constexpr size_t kDefaultStreamSize = 512;

  friend Async::Promise<ssize_t> ServeFile(ResponseWriter&, const char*,
                                           const Mime::MediaType&);

  friend class Handler;
  friend class Timeout;

  ResponseWriter(ResponseWriter&& rhs)
      : Response(MoveTemp(rhs)),
        peer_(MoveTemp(rhs.peer_)),
        buf_(MoveTemp(rhs.buf_)),
        transport_(rhs.transport_),
        timeout_(MoveTemp(rhs.timeout_)) {}

  ResponseWriter& operator=(ResponseWriter&& rhs) {
    if (FUN_LIKELY(&rhs != this)) {
      Response::operator=(MoveTemp(rhs));
      peer_ = MoveTemp(rhs.peer_);
      transport_ = rhs.transport_;
      buf_ = MoveTemp(rhs.buf_);
      timeout_ = MoveTemp(rhs.timeout_);
    }
    return *this;
  }

  void SetMime(const Mime::MediaType& mime) {
    auto ct = headers_.TryGet<Header::ContentType>();
    if (ct) {
      ct->SetMime(mime);
    } else {
      handlers_.Add(MakeShared<Header::ContentType>(mime));
    }
  }

  Async::Promise<ssize_t> Send(Code code) {
    code_ = code;
    return PutOnWire(nullptr, 0);
  }

  // TODO 아래 두 함수는 공용 함수하나를 빼주어서 처리하는게 좋을듯...

  Async::Promise<ssize_t> Send(
      Code code, const String& body,
      const Mime::MediaType& mime = Mime::MediaType()) {
    code_ = code;

    if (mime.IsValid()) {
      auto ct = headers_.TryGet<Header::ContentType>();
      if (ct) {
        ct->SetMime(mime);
      } else {
        handlers_.Add(MakeShared<Header::ContentType>(mime));
      }
    }

    return PutOnWire(body.c_str(), body.Len());
  }

  template <size_t N>
  Async::Promise<ssize_t> Send(
      Code code, const char (&array)[N],
      const Mime::MediaType& mime = Mime::MediaType()) {
    code_ = code;

    if (mime.IsValid()) {
      auto ct = headers_.TryGet<Header::ContentType>();
      if (ct) {
        ct->SetMime(mime);
      } else {
        handlers_.Add(MakeShared<Header::ContentType>(mime));
      }
    }

    return PutOnWire(array, N - 1);
  }

  ResponseStream Stream(Code code, size_t stream_size = kDefaultStreamSize) {
    code_ = code;

    return ResponseStream(MoveTemp(*this), peer_, transport_, MoveTemp(timeout),
                          stream_size);
  }

  ResponseWriter Clone() const { return ResponseWriter(*this); }

 private:
  ResponseWriter(TcpTransport* transport, Request request, Handler* handler)
      : Response(request.GetVersion()),
        buf_(kDefaultStreamSize),
        tansport_(transport),
        timeout_(transport, handler, MoveTemp(request)) {}

  template <typename Ptr>
  void AssociatePeer(const Ptr& peer) {
    if (
  }
};

}  // namespace http
}  // namespace fun
