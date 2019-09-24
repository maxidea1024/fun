#pragma once

#include "fun/base/string_piece.h"

#include <boost/enable_shared_from_this.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

extern "C" {
typedef void CURLM;
typedef void CURL;
}

namespace fun {
namespace net {
class Channel;
class EventLoop;
}  // namespace net
}  // namespace fun

namespace curl {

class Curl;

class Request : EnableSharedFromThis<Request>, Noncopyable {
 public:
  typedef Function<void(const char*, int)> DataCallback;
  typedef Function<void(Request*, int)> DoneCallback;

  Request(Curl*, const char* url);
  ~Request();

  void SetDataCallback(const DataCallback& cb) { data_cb_ = cb; }

  void SetDoneCallback(const DoneCallback& cb) { done_cb_ = cb; }

  void SetHeaderCallback(const DataCallback& cb) { header_cb_ = cb; }

  // void AllowRedirect(int redirects);
  void HeaderOnly();
  void SetRange(const fun::StringArg range);

  template <typename OPT>
  int setopt(OPT opt, long p) {
    return curl_easy_setopt(curl_, opt, p);
  }

  template <typename OPT>
  int setopt(OPT opt, const char* p) {
    return curl_easy_setopt(curl_, opt, p);
  }

  template <typename OPT>
  int setopt(OPT opt, void* p) {
    return curl_easy_setopt(curl_, opt, p);
  }

  template <typename OPT>
  int setopt(OPT opt, size_t (*p)(char*, size_t, size_t, void*)) {
    return curl_easy_setopt(curl_, opt, p);
  }

  const char* GetEffectiveUrl();
  const char* GetRedirectUrl();
  int GetResponseCode();

  // internal
  fun::net::Channel* SetChannel(int fd);
  void RemoveChannel();
  void Done(int code);

  CURL* GetCurl() { return curl_; }

  fun::net::Channel* GetChannel() { return get_pointer(channel_); }

 private:
  void DataCallback(const char* buffer, int len);
  void HeaderCallback(const char* buffer, int len);
  static size_t WriteData(char* buffer, size_t size, size_t nmemb, void* userp);
  static size_t HeaderData(char* buffer, size_t size, size_t nmemb,
                           void* userp);
  void DoneCallback();

  class Curl* owner_;
  CURL* curl_;
  fun::SharedPtr<fun::net::Channel> channel_;
  DataCallback data_cb_;
  DataCallback header_cb_;
  DoneCallback done_cb_;
};

typedef fun::SharedPtr<Request> RequestPtr;

class Curl : Noncopyable {
 public:
  enum Option {
    kCURLnossl = 0,
    kCURLssl = 1,
  };

  explicit Curl(fun::net::EventLoop* loop);
  ~Curl();

  RequestPtr GetUrl(fun::StringArg url);

  static void Initialize(Option opt = kCURLnossl);

  // internal
  CURLM* GetCurlm() { return curlm_; }
  fun::net::EventLoop* GetLoop() { return loop_; }

 private:
  void OnTimer();
  void OnRead(int fd);
  void OnWrite(int fd);
  void CheckFinish();

  static int SocketCallback(CURL*, int, int, void*, void*);
  static int TimerCallback(CURLM*, long, void*);

  fun::net::EventLoop* loop_;
  CURLM* curlm_;
  int running_handles_;
  int prev_running_handles_;
};

}  // namespace curl
