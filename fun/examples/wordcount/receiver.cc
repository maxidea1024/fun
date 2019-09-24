#include "fun/base/logging.h"
#include "fun/net/event_loop.h"
#include "fun/net/tcp_server.h"

#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>

#include <examples/wordcount/hash.h>

#include <fstream>

#include <stdio.h>

using namespace fun;
using namespace fun::net;

class WordCountReceiver : Noncopyable
{
 public:
  WordCountReceiver(EventLoop* loop, const InetAddress& listen_addr)
    : loop_(loop)
    , server_(loop, listen_addr, "WordCountReceiver")
    , senders_(0)
  {
    server_.SetConnectionCallback(boost::bind(&WordCountReceiver::OnConnection, this, _1));
    server_.SetMessageCallback(boost::bind(&WordCountReceiver::OnMessage, this, _1, _2, _3));
  }

  void Start(int senders)
  {
    LOG_INFO << "Start " << senders << " senders";
    senders_ = senders;
    wordcounts_.clear();
    server_.Start();
  }

 private:
  void OnConnection(const TcpConnectionPtr& conn)
  {
    LOG_DEBUG << conn->GetPeerAddress().ToIpPort() << " -> "
              << conn->GetLocalAddress().ToIpPort() << " is "
              << (conn->IsConnected() ? "UP" : "DOWN");

    if (!conn->IsConnected()) {
      if (--senders_ == 0) {
        Output();
        loop_->Quit();
      }
    }
  }

  void OnMessage( const TcpConnectionPtr& conn,
                  Buffer* buf,
                  const Timestamp&)
  {
    const char* crlf = NULL;
    while ((crlf = buf->FindCRLF()) != NULL) {
      // String request(buf->GetReadablePtr(), crlf);
      // printf("%s\n", request.c_str());
      const char* tab = std::find(buf->GetReadablePtr(), crlf, '\t');
      if (tab != crlf) {
        String word(buf->GetReadablePtr(), tab);
        int64_t cnt = atoll(tab);
        wordcounts_[word] += cnt;
      }
      else {
        LOG_ERROR << "Wrong format, no tab found";
        conn->Shutdown();
      }
      buf->DrainUntil(crlf + 2);
    }
  }

  void Output()
  {
    LOG_INFO << "Writing shard";
    std::ofstream out("shard");
    for (WordCountMap::iterator it = wordcounts_.begin();
         it != wordcounts_.end(); ++it) {
      out << it->first << '\t' << it->second << '\n';
    }
  }

  EventLoop* loop_;
  TcpServer server_;
  int senders_;
  WordCountMap wordcounts_;
};

int main(int argc, char* argv[])
{
  if (argc < 3) {
    printf("Usage: %s listen_port number_of_senders\n", argv[0]);
  }
  else {
    EventLoop loop;
    int port = atoi(argv[1]);
    InetAddress addr(static_cast<uint16_t>(port));
    WordCountReceiver receiver(&loop, addr);
    receiver.Start(atoi(argv[2]));
    loop.Loop();
  }
}
