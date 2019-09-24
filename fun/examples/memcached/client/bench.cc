#include "fun/base/count_down_latch.h"
#include "fun/base/logging.h"
#include "fun/net/event_loop.h"
#include "fun/net/event_loop_thread_pool.h"
#include "fun/net/tcp_client.h"

#include <boost/bind.hpp>
#include <boost/program_options.hpp>
#include <iostream>

#include <stdio.h>

namespace po = boost::program_options;
using namespace fun;
using namespace fun::net;

class Client : Noncopyable {
 public:
  enum Operation {
    kGet,
    kSet,
  };

  Client(const String& name, EventLoop* loop, const InetAddress& server_addr,
         Operation op, int requests, int keys, int value_len,
         CountDownLatch* connected, CountDownLatch* finished)
      : name_(name),
        client_(loop, server_addr, name),
        op_(op),
        sent_(0),
        acked_(0),
        requests_(requests),
        keys_(keys),
        value_len_(value_len),
        value_(value_len_, 'a'),
        connected_(connected),
        finished_(finished) {
    value_ += "\r\n";
    client_.SetConnectionCallback(boost::bind(&Client::OnConnection, this, _1));
    client_.SetMessageCallback(
        boost::bind(&Client::OnMessage, this, _1, _2, _3));
    client_.Connect();
  }

  void Send() {
    Buffer buf;
    Fill(&buf);
    conn_->Send(&buf);
  }

 private:
  void OnConnection(const TcpConnectionPtr& conn) {
    if (conn->IsConnected()) {
      conn_ = conn;
      connected_->CountDown();
    } else {
      conn_.Reset();
      client_.GetLoop()->QueueInLoop(
          boost::bind(&CountDownLatch::CountDown, finished_));
    }
  }

  void OnMessage(const TcpConnectionPtr& conn, Buffer* buffer,
                 const Timestamp& received_time) {
    if (op_ == kSet) {
      while (buffer->GetReadableLength() > 0) {
        if (const char* crlf = buffer->FindCRLF()) {
          buffer->DrainUntil(crlf + 2);
          ++acked_;
          if (sent_ < requests_) {
            Send();
          }
        } else {
          break;
        }
      }
    } else {
      while (buffer->GetReadableLength() > 0) {
        const char* end = static_cast<const char*>(
            memmem(buffer->GetReadablePtr(), buffer->GetReadableLength(),
                   "END\r\n", 5));
        if (end) {
          buffer->DrainUntil(end + 5);
          ++acked_;
          if (sent_ < requests_) {
            Send();
          }
        } else {
          break;
        }
      }
    }

    if (acked_ == requests_) {
      conn_->Shutdown();
    }
  }

  void Fill(Buffer* buf) {
    char req[256];
    if (op_ == kSet) {
      snprintf(req, sizeof req, "set %s%d 42 0 %d\r\n", name_.c_str(),
               sent_ % keys_, value_len_);
      ++sent_;
      buf->Append(req);
      buf->Append(value_);
    } else {
      snprintf(req, sizeof req, "get %s%d\r\n", name_.c_str(), sent_ % keys_);
      ++sent_;
      buf->Append(req);
    }
  }

  String name_;
  TcpClient client_;
  TcpConnectionPtr conn_;
  const Operation op_;
  int sent_;
  int acked_;
  const int requests_;
  const int keys_;
  const int value_len_;
  String value_;
  CountDownLatch* const connected_;
  CountDownLatch* const finished_;
};

int main(int argc, char* argv[]) {
  Logger::SetLogLevel(Logger::WARN);

  uint16_t tcp_port = 11211;
  String hostIp = "127.0.0.1";
  int threads = 4;
  int clients = 100;
  int requests = 100000;
  int keys = 10000;
  bool set = false;

  po::options_description desc("Allowed options");
  desc.add_options()("help,h", "Help")("port,p", po::value<uint16_t>(&tcp_port),
                                       "TCP port")(
      "ip,i", po::value<String>(&hostIp), "Host IP")(
      "threads,t", po::value<int>(&threads), "Number of worker threads")(
      "clients,c", po::value<int>(&clients), "Number of concurrent clients")(
      "requests,r", po::value<int>(&requests),
      "Number of requests per clients")("keys,k", po::value<int>(&keys),
                                        "Number of keys per clients")(
      "set,s", "Get or Set");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << desc << "\n";
    return 0;
  }
  set = vm.count("set");

  InetAddress server_addr(hostIp, tcp_port);
  LOG_WARN << "Connecting " << server_addr.ToIpPort();

  EventLoop loop;
  EventLoopThreadPool pool(&loop, "bench-memcache");

  int value_len = 100;
  Client::Operation op = set ? Client::kSet : Client::kGet;

  double memoryMiB =
      1.0 * clients * keys * (32 + 80 + value_len + 8) / 1024 / 1024;
  LOG_WARN << "estimated memcached-debug memory usage " << int(memoryMiB)
           << " MiB";

  pool.SetThreadCount(threads);
  pool.Start();

  char buf[32];
  CountDownLatch connected(clients);
  CountDownLatch finished(clients);
  boost::ptr_vector<Client> holder;
  for (int i = 0; i < clients; ++i) {
    snprintf(buf, sizeof buf, "%d-", i + 1);
    holder.push_back(new Client(buf, pool.GetNextLoop(), server_addr, op,
                                requests, keys, value_len, &connected,
                                &finished));
  }
  connected.Wait();
  LOG_WARN << clients << " clients all connected";
  Timestamp Start = Timestamp::Now();
  for (int i = 0; i < clients; ++i) {
    holder[i].Send();
  }
  finished.Wait();
  Timestamp end = Timestamp::Now();
  LOG_WARN << "All finished";
  double seconds = TimeDifference(end, Start);
  LOG_WARN << seconds << " sec";
  LOG_WARN << 1.0 * clients * requests / seconds << " QPS";
}
