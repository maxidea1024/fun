#include <red/base/CountDownLatch.h>
#include "fun/base/logging.h"
#include "fun/net/event_loop_thread.h"
#include "fun/net/tcp_client.h"

#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/tokenizer.hpp>

#include <examples/wordcount/hash.h>

#include <fstream>
#include <iostream>

#include <stdio.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

using namespace fun;
using namespace fun::net;

size_t g_batch_size = 65536;
const size_t kMaxHashSize = 10 * 1000 * 1000;

class SendThrottler : Noncopyable {
 public:
  SendThrottler(EventLoop* loop, const InetAddress& addr)
      : client_(loop, addr, "Sender"),
        connect_latch_(1),
        disconnect_latch_(1),
        cond_(mutex_),
        congestion_(false) {
    LOG_INFO << "SendThrottler [" << addr.ToIpPort() << "]";
    client_.SetConnectionCallback(
        boost::bind(&SendThrottler::OnConnection, this, _1));
  }

  void Connect() {
    client_.Connect();
    connect_latch_.Wait();
  }

  void Disconnect() {
    if (buffer_.GetReadableLength() > 0) {
      LOG_DEBUG << "Send " << buffer_.GetReadableLength() << " bytes";
      conn_->Send(&buffer_);
    }
    conn_->Shutdown();
    disconnect_latch_.Wait();
  }

  void Send(const String& word, int64_t count) {
    buffer_.append(word);
    // FIXME: use LogStream
    char buf[64];
    snprintf(buf, sizeof buf, "\t%" PRId64 "\r\n", count);
    buffer_.append(buf);
    if (buffer_.GetReadableLength() >= g_batch_size) {
      Throttle();
      LOG_TRACE << "Send " << buffer_.GetReadableLength();
      conn_->Send(&buffer_);
    }
  }

 private:
  void OnConnection(const TcpConnectionPtr& conn) {
    if (conn->IsConnected()) {
      conn->SetHighWaterMarkCallback(
          boost::bind(&SendThrottler::OnHighWaterMark, this), 1024 * 1024);
      conn->SetWriteCompleteCallback(
          boost::bind(&SendThrottler::OnWriteComplete, this));

      conn_ = conn;
      connect_latch_.CountDown();
    } else {
      conn_.Reset();
      disconnect_latch_.CountDown();
    }
  }

  void OnHighWaterMark() {
    ScopedLock guard(mutex_);
    congestion_ = true;
  }

  void OnWriteComplete() {
    ScopedLock guard(mutex_);
    bool oldCong = congestion_;
    congestion_ = false;
    if (oldCong) {
      cond_.notify();
    }
  }

  void Throttle() {
    ScopedLock guard(mutex_);
    while (congestion_) {
      LOG_DEBUG << "wait ";
      cond_.Wait();
    }
  }

  TcpClient client_;
  TcpConnectionPtr conn_;
  CountDownLatch connect_latch_;
  CountDownLatch disconnect_latch_;
  Buffer buffer_;

  MutexLock mutex_;
  Condition cond_;
  bool congestion_;
};

class WordCountSender : Noncopyable {
 public:
  explicit WordCountSender(const String& receivers);

  void ConnectAll() {
    for (size_t i = 0; i < buckets_.size(); ++i) {
      buckets_[i].Connect();
    }
    LOG_INFO << "All connected";
  }

  void DisconnectAll() {
    for (size_t i = 0; i < buckets_.size(); ++i) {
      buckets_[i].Disconnect();
    }
    LOG_INFO << "All disconnected";
  }

  void ProcessFile(const char* filename);

 private:
  EventLoopThread loopThread_;
  EventLoop* loop_;
  boost::ptr_vector<SendThrottler> buckets_;
};

WordCountSender::WordCountSender(const String& receivers)
    : loop_(loopThread_.StartLoop()) {
  typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
  boost::char_separator<char> sep(", ");
  tokenizer tokens(receivers, sep);
  for (tokenizer::iterator tok_iter = tokens.begin(); tok_iter != tokens.end();
       ++tok_iter) {
    String ipport = *tok_iter;
    size_t colon = ipport.find(':');
    if (colon != String::npos) {
      uint16_t port = static_cast<uint16_t>(atoi(&ipport[colon + 1]));
      InetAddress addr(ipport.substr(0, colon), port);
      buckets_.push_back(new SendThrottler(loop_, addr));
    } else {
      fun_check(0 && "Invalid address");
    }
  }
}

void WordCountSender::ProcessFile(const char* filename) {
  LOG_INFO << "ProcessFile " << filename;
  WordCountMap wordcounts;
  // FIXME: use mmap to read file
  std::ifstream in(filename);
  String word;
  // FIXME: make local hash optional.
  boost::hash<String> hash;
  while (in) {
    wordcounts.clear();
    while (in >> word) {
      wordcounts[word] += 1;
      if (wordcounts.size() > kMaxHashSize) {
        break;
      }
    }

    LOG_INFO << "Send " << wordcounts.size() << " records";
    for (WordCountMap::iterator it = wordcounts.begin(); it != wordcounts.end();
         ++it) {
      size_t idx = hash(it->first) % buckets_.size();
      buckets_[idx].Send(it->first, it->second);
    }
  }
}

int main(int argc, char* argv[]) {
  if (argc < 3) {
    printf("Usage: %s addresses_of_receivers input_file1 [input_file2]* \n",
           argv[0]);
    printf(
        "Example: %s 'ip1:port1,ip2:port2,ip3:port3' input_file1 input_file2 "
        "\n",
        argv[0]);
  } else {
    const char* batch_size = ::getenv("BATCH_SIZE");
    if (batch_size) {
      g_batch_size = atoi(batch_size);
    }

    WordCountSender sender(argv[1]);
    sender.ConnectAll();
    for (int i = 2; i < argc; ++i) {
      sender.ProcessFile(argv[i]);
    }
    sender.DisconnectAll();
  }
}
