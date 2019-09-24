#include <examples/curl/Curl.h>
#include <boost/bind.hpp>
#include "fun/base/logging.h"
#include "fun/net/channel.h"
#include "fun/net/event_loop.h"

#include <curl/curl.h>
#include <fun_check.h>

using namespace curl;
using namespace fun;
using namespace fun::net;

static void Dummy(const fun::SharedPtr<Channel>&) {}

Request::Request(Curl* owner, const char* url)
    : owner_(owner), curl_(CHECK_NOTNULL(curl_easy_init())) {
  setopt(CURLOPT_URL, url);
  setopt(CURLOPT_WRITEFUNCTION, &Request::WriteData);
  setopt(CURLOPT_WRITEDATA, this);
  setopt(CURLOPT_HEADERFUNCTION, &Request::HeaderData);
  setopt(CURLOPT_HEADERDATA, this);
  setopt(CURLOPT_PRIVATE, this);
  setopt(CURLOPT_USERAGENT, "curl");
  // set useragent
  LOG_DEBUG << curl_ << " " << url;
  curl_multi_add_handle(owner_->getCurlm(), curl_);
}

Request::~Request() {
  fun_check(!channel_ || channel_->IsNoneEvent());
  curl_multi_remove_handle(owner_->getCurlm(), curl_);
  curl_easy_cleanup(curl_);
}

// NOT implemented yet
//
// void Request::AllowRedirect(int redirects) {
//   setopt(CURLOPT_FOLLOWLOCATION, 1);
//   setopt(CURLOPT_MAXREDIRS, redirects);
// }

void Request::HeaderOnly() { setopt(CURLOPT_NOBODY, 1); }

void Request::SetRange(const StringArg range) {
  setopt(CURLOPT_RANGE, range.c_str());
}

const char* Request::GetEffectiveUrl() {
  const char* p = NULL;
  curl_easy_getinfo(curl_, CURLINFO_EFFECTIVE_URL, &p);
  return p;
}

const char* Request::GetRedirectUrl() {
  const char* p = NULL;
  curl_easy_getinfo(curl_, CURLINFO_REDIRECT_URL, &p);
  return p;
}

int Request::GetResponseCode() {
  long code = 0;
  curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &code);
  return static_cast<int>(code);
}

Channel* Request::SetChannel(int fd) {
  fun_check(channel_.Get() == NULL);
  channel_.Reset(new Channel(owner_->GetLoop(), fd));
  channel_->Tie(SharedFromThis());
  return get_pointer(channel_);
}

void Request::RemoveChannel() {
  channel_->DisableAll();
  channel_->Remove();
  owner_->GetLoop()->QueueInLoop(boost::bind(Dummy, channel_));
  channel_.Reset();
}

void Request::Done(int code) {
  if (done_cb_) {
    done_cb_(this, code);
  }
}

void Request::DataCallback(const char* buffer, int len) {
  if (data_cb_) {
    data_cb_(buffer, len);
  }
}

void Request::HeaderCallback(const char* buffer, int len) {
  if (header_cb_) {
    header_cb_(buffer, len);
  }
}

size_t Request::WriteData(char* buffer, size_t size, size_t nmemb,
                          void* userp) {
  fun_check(size == 1);
  Request* req = static_cast<Request*>(userp);
  req->DataCallback(buffer, static_cast<int>(nmemb));
  return nmemb;
}

size_t Request::HeaderData(char* buffer, size_t size, size_t nmemb,
                           void* userp) {
  fun_check(size == 1);
  Request* req = static_cast<Request*>(userp);
  req->HeaderCallback(buffer, static_cast<int>(nmemb));
  return nmemb;
}

// ==================================================================

void Curl::Initialize(Option opt) {
  curl_global_init(opt == kCURLnossl ? CURL_GLOBAL_NOTHING : CURL_GLOBAL_SSL);
}

int Curl::SocketCallback(CURL* c, int fd, int what, void* userp,
                         void* socketp) {
  Curl* curl = static_cast<Curl*>(userp);
  const char* whatstr[] = {"none", "IN", "OUT", "INOUT", "REMOVE"};
  LOG_DEBUG << "Curl::SocketCallback [" << curl << "] - fd = " << fd
            << " what = " << whatstr[what];
  Request* req = NULL;
  curl_easy_getinfo(c, CURLINFO_PRIVATE, &req);
  fun_check(req->GetCurl() == c);
  if (what == CURL_POLL_REMOVE) {
    fun::net::Channel* ch = static_cast<Channel*>(socketp);
    fun_check(req->GetChannel() == ch);
    req->RemoveChannel();
    ch = NULL;
    curl_multi_assign(curl->curlm_, fd, ch);
  } else {
    fun::net::Channel* ch = static_cast<Channel*>(socketp);
    if (!ch) {
      ch = req->SetChannel(fd);
      ch->SetReadCallback(boost::bind(&Curl::OnRead, curl, fd));
      ch->SetWriteCallback(boost::bind(&Curl::OnWrite, curl, fd));
      ch->EnableReading();
      curl_multi_assign(curl->curlm_, fd, ch);
      LOG_TRACE << "new channel for fd=" << fd;
    }
    fun_check(req->GetChannel() == ch);
    // update
    if (what & CURL_POLL_OUT) {
      ch->EnableWriting();
    } else {
      ch->DisableWriting();
    }
  }
  return 0;
}

int Curl::TimerCallback(CURLM* curlm, long ms, void* userp) {
  Curl* curl = static_cast<Curl*>(userp);
  LOG_DEBUG << curl << " " << ms << " ms";
  curl->loop_->ScheduleAfter(static_cast<int>(ms) / 1000.0,
                             boost::bind(&Curl::OnTimer, curl));
  return 0;
}

Curl::Curl(EventLoop* loop)
    : loop_(loop),
      curlm_(CHECK_NOTNULL(curl_multi_init())),
      running_handles_(0),
      prev_running_handles_(0) {
  curl_multi_setopt(curlm_, CURLMOPT_SOCKETFUNCTION, &Curl::SocketCallback);
  curl_multi_setopt(curlm_, CURLMOPT_SOCKETDATA, this);
  curl_multi_setopt(curlm_, CURLMOPT_TIMERFUNCTION, &Curl::TimerCallback);
  curl_multi_setopt(curlm_, CURLMOPT_TIMERDATA, this);
}

Curl::~Curl() { curl_multi_cleanup(curlm_); }

RequestPtr Curl::GetUrl(StringArg url) {
  RequestPtr req(new Request(this, url.c_str()));
  return req;
}

void Curl::OnTimer() {
  CURLMcode rc = CURLM_OK;
  do {
    LOG_TRACE;
    rc = curl_multi_socket_action(curlm_, CURL_SOCKET_TIMEOUT, 0,
                                  &running_handles_);
    LOG_TRACE << rc << " " << running_handles_;
  } while (rc == CURLM_CALL_MULTI_PERFORM);
  CheckFinish();
}

void Curl::OnRead(int fd) {
  CURLMcode rc = CURLM_OK;
  do {
    LOG_TRACE << fd;
    rc = curl_multi_socket_action(curlm_, fd, CURL_POLL_IN, &running_handles_);
    LOG_TRACE << fd << " " << rc << " " << running_handles_;
  } while (rc == CURLM_CALL_MULTI_PERFORM);
  CheckFinish();
}

void Curl::OnWrite(int fd) {
  CURLMcode rc = CURLM_OK;
  do {
    LOG_TRACE << fd;
    rc = curl_multi_socket_action(curlm_, fd, CURL_POLL_OUT, &running_handles_);
    LOG_TRACE << fd << " " << rc << " " << running_handles_;
  } while (rc == CURLM_CALL_MULTI_PERFORM);
  CheckFinish();
}

void Curl::CheckFinish() {
  if (prev_running_handles_ > running_handles_ || running_handles_ == 0) {
    CURLMsg* msg = NULL;
    int left = 0;
    while ((msg = curl_multi_info_read(curlm_, &left)) != NULL) {
      if (msg->msg == CURLMSG_DONE) {
        CURL* c = msg->easy_handle;
        CURLcode res = msg->data.result;
        Request* req = NULL;
        curl_easy_getinfo(c, CURLINFO_PRIVATE, &req);
        fun_check(req->GetCurl() == c);
        LOG_TRACE << req << " Done";
        req->Done(res);
      }
    }
  }
  prev_running_handles_ = running_handles_;
}
