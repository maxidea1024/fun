// Concurrent downloading one file from HTTP

#include <examples/curl/Curl.h>
#include "fun/base/logging.h"
#include "fun/net/event_loop.h"
#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <stdio.h>
#include <sstream>

using namespace fun;
using namespace fun::net;

typedef fun::SharedPtr<FILE> FilePtr;

template <int N>
bool StartsWith(const String& str, const char (&prefix)[N]) {
  return str.size() >= N-1 && std::equal(prefix, prefix+N-1, str.begin());
}

class Piece : Noncopyable {
 public:
  Piece(const curl::RequestPtr& req,
        const FilePtr& out,
        const String& range,
        const Function<void()> done)
    : req_(req)
    , out_(out)
    , range_(range)
    , done_cb_(done) {
    LOG_INFO << "range: " << range;
    req->SetRange(range);
    req_->SetDataCallback(
        boost::bind(&Piece::OnData, this, _1, _2));
    req_->SetDoneCallback(
        boost::bind(&Piece::OnDone, this, _1, _2));
  }

 private:
  void OnData(const char* data, int len) {
    ::fwrite(data, 1, len, get_pointer(out_));
  }

  void OnDone(curl::Request* c, int code) {
    LOG_INFO << "[" << range_ << "] is done";
    req_.Reset();
    out_.Reset();
    done_cb_();
  }

  curl::RequestPtr req_;
  FilePtr out_;
  String range_;
  Function<void()> done_cb_;
};

class Downloader : Noncopyable {
 public:
  Downloader(EventLoop* loop, const String& url)
    : loop_(loop)
    , curl_(loop_)
    , url_(url)
    , req_(curl_.GetUrl(url_))
    , found_(false)
    , accept_ranges_(false)
    , length_(0)
    , pieces_(kConcurrent)
    , concurrent_(0) {
    req_->SetHeaderCallback(boost::bind(&Downloader::OnHeader, this, _1, _2));
    req_->SetDoneCallback(boost::bind(&Downloader::OnHeaderDone, this, _1, _2));
    req_->HeaderOnly();
  }

 private:
  void OnHeader(const char* data, int len) {
    String line(data, len);
    if (StartsWith(line, "HTTP/1.1 200") || StartsWith(line, "HTTP/1.0 200")) {
      found_ = true;
    }

    if (line == "Accept-Ranges: bytes\r\n") {
      accept_ranges_ = true;
      LOG_DEBUG << "Accept-Ranges";
    }
    else if (StartsWith(line, "Content-Length:")) {
      length_ = atoll(line.c_str() + strlen("Content-Length:"));
      LOG_INFO << "Content-Length: " << length_;
    }
  }

  void OnHeaderDone(curl::Request* c, int code) {
    LOG_DEBUG << code;
    if (accept_ranges_ && length_ >= kConcurrent * 4096) {
      LOG_INFO << "Downloading with " << kConcurrent << " connections";
      concurrent_ = kConcurrent;
      ConcurrentDownload();
    }
    else if (found_) {
      LOG_WARN << "Single connection download";
      FILE* fp = ::fopen("output", "wb");
      if (fp) {
        FilePtr(fp, ::fclose).swap(out_);
        req_.Reset();
        req2_ = curl_.GetUrl(url_);
        req2_->SetDataCallback(
            boost::bind(&Downloader::OnData, this, _1, _2));
        req2_->SetDoneCallback(
            boost::bind(&Downloader::onDownloadDone, this));
        concurrent_ = 1;
      }
      else {
        LOG_ERROR << "Can not create output file";
        loop_->Quit();
      }
    }
    else {
      LOG_ERROR << "File not found";
      loop_->Quit();
    }
  }

  void ConcurrentDownload() {
    const int64_t piece_len = length_ / kConcurrent;
    for (int i = 0; i < kConcurrent; ++i) {
      char buf[256];
      snprintf(buf, sizeof buf, "output-%05d-of-%05d", i, kConcurrent);
      FILE* fp = ::fopen(buf, "wb");
      if (fp) {
        FilePtr out(fp, ::fclose);
        curl::RequestPtr req = curl_.GetUrl(url_);

        std::ostringstream range;
        if (i < kConcurrent - 1) {
          range << i * piece_len << "-" << (i+1) * piece_len - 1;
        }
        else {
          range << i * piece_len << "-" << length_ - 1;
        }
        pieces_.push_back(new Piece(req,
                                    out,
                                    range.str().c_str(), // String -> String
                                    boost::bind(&Downloader::onDownloadDone, this)));
      }
      else {
        LOG_ERROR << "Can not create output file: " << buf;
        loop_->Quit();
      }
    }
  }

  void OnData(const char* data, int len) {
    ::fwrite(data, 1, len, get_pointer(out_));
  }

  void onDownloadDone() {
    if (--concurrent_ <= 0) {
      loop_->Quit();
    }
  }

  EventLoop* loop_;
  curl::Curl curl_;
  String url_;
  curl::RequestPtr req_;
  curl::RequestPtr req2_;
  bool found_;
  bool accept_ranges_;
  int64_t length_;
  FilePtr out_;
  boost::ptr_vector<Piece> pieces_;
  int concurrent_;

  const static int kConcurrent = 4;
};

int main(int argc, char* argv[]) {
  EventLoop loop;
  curl::Curl::Initialize(curl::Curl::kCURLssl);
  String url = argc > 1 ? argv[1] : "https://chenshuo-public.s3.amazonaws.com/pdf/allinone.pdf";
  Downloader downloader(&loop, url);
  loop.Loop();
}
