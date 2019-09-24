#pragma once


namespace fun {
namespace net {

class HttpRequest {
 public:
  enum Method   {
    kInvalid,
    kGet,
    kPost,
    kHead,
    kPut,
    kDelete
  };

  enum Version {
    kUnknown,
    kHttp10,
    kHttp11
  };


  HttpRequest()
    : method_(kInvalid)
    //TODO 그냥 kHttp11로 고정하는게 좋을듯...??
    , version_(kUnknown) {
  }

  void SetVersion(Version v) {
    version_ = v;
  }

  Version GetVersion() const {
    return version_;
  }

  bool SetMethod(const char* start, const char* end) {
    assert(method_ == kInvalid);
    String m(start, end);
    if (m == "GET") {
      method_ = kGet;
    } else if (m == "POST") {
      method_ = kPost;
    } else if (m == "HEAD") {
      method_ = kHead;
    } else if (m == "PUT") {
      method_ = kPut;
    } else if (m == "DELETE") {
      method_ = kDelete;
    } else {
      method_ = kInvalid;
    }
    return method_ != kInvalid;
  }

  Method GetMethod() const {
    return method_;
  }

  const char* MethodString() const {
    const char* result = "UNKNOWN";
    switch(method_) {
      case kGet:
        result = "GET";
        break;
      case kPost:
        result = "POST";
        break;
      case kHead:
        result = "HEAD";
        break;
      case kPut:
        result = "PUT";
        break;
      case kDelete:
        result = "DELETE";
        break;
      default:
        break;
    }
    return result;
  }

  void SetPath(const char* start, const char* end) {
    path_.assign(start, end);
  }

  const String& GetPath() const {
    return path_;
  }

  void SetQuery(const char* start, const char* end) {
    query_.assign(start, end);
  }

  const String& GetQuery() const {
    return query_;
  }

  void SetReceivedTime(const Timestamp& t) {
    received_time_ = t;
  }

  Timestamp received_time() const {
    return received_time_;
  }

  void AddHeader(const char* start, const char* colon, const char* end) {
    String field(start, colon);
    ++colon;
    while (colon < end && isspace(*colon)) {
      ++colon;
    }
    String value(colon, end);
    while (!value.empty() && isspace(value[value.size()-1])) {
      value.resize(value.size()-1);
    }
    headers_[field] = value;
  }

  String GetHeader(const String& field) const {
    String result;
    std::map<String, String>::const_iterator it = headers_.find(field);
    if (it != headers_.end()) {
      result = it->second;
    }
    return result;
  }

  const std::map<String, String>& GetHeaders() const {
    return headers_;
  }

  void Swap(HttpRequest& rhs) {
    fun::Swap(method_, rhs.method_);
    fun::Swap(version_, rhs.version_);
    path_.Swap(rhs.path_);
    query_.Swap(rhs.query_);
    received_time_.Swap(rhs.received_time_);
    headers_.Swap(rhs.headers_);
  }

 private:
  Method method_;
  Version version_;
  String path_;
  String query_;
  Timestamp received_time_;
  std::map<String, String> headers_;
};

} // namespace net
} // namespace fun
