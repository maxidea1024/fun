#pragma once


namespace fun {
namespace net {

class Buffer;

class HttpResponse {
 public:
  enum HttpStatusCode {
    kUnknown,
    k200Ok = 200,
    k301MovedPermanently = 301,
    k400BadRequest = 400,
    k404NotFound = 404,
  };


  explicit HttpResponse(bool close)
    : status_code_(kUnknown), close_connection_(close) {}

  void SetStatusCode(HttpStatusCode code) {
    status_code_ = code;
  }

  void SetStatusMessage(const String& message) {
    status_message_ = message;
  }

  void SetCloseConnection(bool on) {
    close_connection_ = on;
  }

  bool CloseConnection() const {
    return close_connection_;
  }

  void SetContentType(const String& contentType) {
    AddHeader("Content-Type", contentType);
  }

  // FIXME: replace String with StringPiece
  void AddHeader(const String& key, const String& value) {
    headers_[key] = value;
  }

  void SetBody(const String& body) {
    body_ = body;
  }

  void AppendToBuffer(Buffer* output) const;

 private:
  std::map<String, String> headers_;
  HttpStatusCode status_code_;
  // FIXME: add http version
  String status_message_;
  bool close_connection_;
  String body_;
};


} // namespace net
} // namespace fun
