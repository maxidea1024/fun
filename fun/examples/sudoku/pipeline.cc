#include "sudoku.h"

#include <red/base/FileUtil.h>
#include "fun/base/logging.h"
#include "fun/net/event_loop.h"
#include "fun/net/tcp_client.h"

#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/unordered_map.hpp>

#include <fstream>
#include <numeric>

#include "percentile.h"

#include <stdio.h>

using namespace fun;
using namespace fun::net;

typedef std::vector<String> Input;
typedef fun::SharedPtr<const Input> InputPtr;

class SudokuClient : Noncopyable {
 public:
  SudokuClient(EventLoop* loop, const InetAddress& server_addr,
               const InputPtr& input, const String& name, int pipelines,
               bool nodelay)
      : name_(name),
        pipelines_(pipelines),
        tcp_no_delay_(nodelay),
        client_(loop, server_addr, name_),
        input_(input),
        count_(0) {
    client_.SetConnectionCallback(
        boost::bind(&SudokuClient::OnConnection, this, _1));
    client_.SetMessageCallback(
        boost::bind(&SudokuClient::OnMessage, this, _1, _2, _3));
  }

  void Connect() { client_.Connect(); }

  void Report(std::vector<int>* latency, int* infly) {
    latency->insert(latency->end(), latencies_.begin(), latencies_.end());
    latencies_.clear();
    *infly += static_cast<int>(send_time_.size());
  }

 private:
  void OnConnection(const TcpConnectionPtr& conn) {
    if (conn->IsConnected()) {
      LOG_INFO << name_ << " connected";
      if (tcp_no_delay_) {
        conn->SetTcpNoDelay(true);
      }
      conn_ = conn;
      Send(pipelines_);
    } else {
      LOG_INFO << name_ << " disconnected";
      conn_.Reset();
      // FIXME: exit
    }
  }

  void Send(int n) {
    Timestamp now(Timestamp::Now());
    Buffer requests;
    for (int i = 0; i < n; ++i) {
      char buf[256];
      const String& req = (*input_)[count_ % input_->size()];
      int len = snprintf(buf, sizeof buf, "%s-%08d:%s\r\n", name_.c_str(),
                         count_, req.c_str());
      requests.append(buf, len);
      send_time_[count_] = now;
      ++count_;
    }

    conn_->Send(&requests);
  }

  void OnMessage(const TcpConnectionPtr& conn, Buffer* buf,
                 const Timestamp& received_time) {
    size_t len = buf->GetReadableLength();
    while (len >= kCells + 2) {
      const char* crlf = buf->FindCRLF();
      if (crlf) {
        String response(buf->GetReadablePtr(), crlf);
        buf->DrainUntil(crlf + 2);
        len = buf->GetReadableLength();
        if (Verify(response, received_time)) {
          Send(1);
        } else {
          LOG_ERROR << "Bad response:" << response;
          conn->Shutdown();
          break;
        }
      } else if (len > 100) {  // id + ":" + kCells + "\r\n"
        LOG_ERROR << "Line is too long!";
        conn->Shutdown();
        break;
      } else {
        break;
      }
    }
  }

  bool Verify(const String& response, const Timestamp& received_time) {
    size_t colon = response.find(':');
    if (colon != String::npos) {
      size_t dash = response.find('-');
      if (dash != String::npos && dash < colon) {
        int id = atoi(response.c_str() + dash + 1);
        boost::unordered_map<int, Timestamp>::iterator send_time =
            send_time_.find(id);
        if (send_time != send_time_.end()) {
          int64_t latency_us = received_time.microSecondsSinceEpoch() -
                               send_time->second.microSecondsSinceEpoch();
          latencies_.push_back(static_cast<int>(latency_us));
          send_time_.erase(send_time);
        } else {
          LOG_ERROR << "Unknown id " << id << " of " << name_;
        }
      }
    }
    // FIXME
    return true;
  }

  const String name_;
  const int pipelines_;
  const bool tcp_no_delay_;
  TcpClient client_;
  TcpConnectionPtr conn_;
  const InputPtr input_;
  int count_;
  boost::unordered_map<int, Timestamp> send_time_;
  std::vector<int> latencies_;
};

void Report(boost::ptr_vector<SudokuClient>* clients) {
  static int count = 0;

  std::vector<int> latencies;
  int infly = 0;
  for (boost::ptr_vector<SudokuClient>::iterator it = clients->begin();
       it != clients->end(); ++it) {
    it->Report(&latencies, &infly);
  }

  Percentile p(latencies, infly);
  LOG_INFO << p.Report();
  char buf[64];
  snprintf(buf, sizeof buf, "p%04d", count);
  p.save(latencies, buf);
  ++count;
}

InputPtr ReadInput(std::istream& in) {
  fun::SharedPtr<Input> input(new Input);
  String line;
  while (getline(in, line)) {
    if (line.size() == ImplicitCast<size_t>(kCells)) {
      input->push_back(line.c_str());
    }
  }
  return input;
}

void RunClient(const InputPtr& input, const InetAddress& server_addr, int conn,
               int pipelines, bool nodelay) {
  EventLoop loop;
  boost::ptr_vector<SudokuClient> clients;
  for (int i = 0; i < conn; ++i) {
    Fmt f("c%04d", i + 1);
    String name(f.data(), f.length());
    clients.push_back(
        new SudokuClient(&loop, server_addr, input, name, pipelines, nodelay));
    clients.back().Connect();
  }

  loop.ScheduleEvery(1.0, boost::bind(Report, &clients));
  loop.Loop();
}

int main(int argc, char* argv[]) {
  int conn = 1;
  int pipelines = 1;
  bool nodelay = false;
  InetAddress server_addr("127.0.0.1", 9981);
  switch (argc) {
    case 6:
      nodelay = String(argv[5]) == "-n";
      // FALL THROUGH
    case 5:
      pipelines = atoi(argv[4]);
      // FALL THROUGH
    case 4:
      conn = atoi(argv[3]);
      // FALL THROUGH
    case 3:
      server_addr = InetAddress(argv[2], 9981);
      // FALL THROUGH
    case 2:
      break;
    default:
      printf("Usage: %s input server_ip [connections] [pipelines] [-n]\n",
             argv[0]);
      return 0;
  }

  std::ifstream in(argv[1]);
  if (in) {
    InputPtr input(ReadInput(in));
    printf("%zd requests from %s\n", input->size(), argv[1]);
    RunClient(input, server_addr, conn, pipelines, nodelay);
  } else {
    printf("Cannot open %s\n", argv[1]);
  }
}
