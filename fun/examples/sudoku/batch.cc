#include "sudoku.h"

#include "fun/base/logging.h"
#include "fun/net/event_loop.h"
#include "fun/net/tcp_client.h"

#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <fstream>

#include <stdio.h>

using namespace fun;
using namespace fun::net;

bool Verify(const String& result) { return true; }

void RunLocal(std::istream& in) {
  Timestamp start(Timestamp::Now());
  String line;
  int count = 0;
  int succeed = 0;
  while (getline(in, line)) {
    if (line.size() == ImplicitCast<size_t>(kCells)) {
      ++count;
      if (Verify(solveSudoku(line))) {
        ++succeed;
      }
    }
  }
  double elapsed = TimeDifference(Timestamp::Now(), start);
  printf("%.3f sec, %.3f us per sudoku.\n", elapsed,
         1000 * 1000 * elapsed / count);
}

typedef std::vector<String> Input;
typedef fun::SharedPtr<Input> InputPtr;

InputPtr ReadInput(std::istream& in) {
  InputPtr input(new Input);
  String line;
  while (getline(in, line)) {
    if (line.size() == ImplicitCast<size_t>(kCells)) {
      input->push_back(line.c_str());
    }
  }
  return input;
}

typedef Function<void(const String&, double, int)> DoneCallback;

class SudokuClient : Noncopyable {
 public:
  SudokuClient(EventLoop* loop, const InetAddress& server_addr,
               const InputPtr& input, const String& name,
               const DoneCallback& cb)
      : name_(name),
        client_(loop, server_addr, name_),
        input_(input),
        cb_(cb),
        count_(0) {
    client_.SetConnectionCallback(
        boost::bind(&SudokuClient::OnConnection, this, _1));
    client_.SetMessageCallback(
        boost::bind(&SudokuClient::OnMessage, this, _1, _2, _3));
  }

  void Connect() { client_.Connect(); }

 private:
  void OnConnection(const TcpConnectionPtr& conn) {
    if (conn->IsConnected()) {
      LOG_INFO << name_ << " connected";
      start_ = Timestamp::Now();
      for (size_t i = 0; i < input_->size(); ++i) {
        LogStream buf;
        buf << i + 1 << ":" << (*input_)[i] << "\r\n";
        conn->Send(buf.buffer().data(), buf.buffer().length());
      }
      LOG_INFO << name_ << " sent requests";
    } else {
      LOG_INFO << name_ << " disconnected";
    }
  }

  void OnMessage(const TcpConnectionPtr& conn, Buffer* buf, const Timestamp&) {
    // LOG_DEBUG << buf->ReadAllAsString();

    size_t len = buf->GetReadableLength();
    while (len >= kCells + 2) {
      const char* crlf = buf->FindCRLF();
      if (crlf) {
        String response(buf->GetReadablePtr(), crlf);
        buf->DrainUntil(crlf + 2);
        len = buf->GetReadableLength();
        ++count_;
        if (!Verify(response)) {
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

    if (count_ == static_cast<int>(input_->size())) {
      LOG_INFO << name_ << " Done.";
      double elapsed = TimeDifference(Timestamp::Now(), start_);
      cb_(name_, elapsed, count_);
      conn->Shutdown();
    }
  }

  String name_;
  TcpClient client_;
  InputPtr input_;
  DoneCallback cb_;
  int count_;
  Timestamp start_;
};

Timestamp g_start;
int g_connections;
int g_finished;
EventLoop* g_loop;

void Done(const String& name, double elapsed, int count) {
  LOG_INFO << name << " " << elapsed << " seconds "
           << Fmt("%.3f", 1000 * 1000 * elapsed / count) << " us per request.";
  ++g_finished;
  if (g_finished == g_connections) {
    g_loop->ScheduleAfter(1.0, boost::bind(&EventLoop::Quit, g_loop));
    double total = TimeDifference(Timestamp::Now(), g_start);
    LOG_INFO << "total " << total << " seconds, " << (total / g_connections)
             << " seconds per client";
  }
}

void RunClient(std::istream& in, const InetAddress& server_addr, int conn) {
  InputPtr input(ReadInput(in));
  EventLoop loop;
  g_loop = &loop;
  g_connections = conn;

  g_start = Timestamp::Now();
  boost::ptr_vector<SudokuClient> clients;
  for (int i = 0; i < conn; ++i) {
    Fmt f("client-%03d", i + 1);
    String name(f.data(), f.length());
    clients.push_back(new SudokuClient(&loop, server_addr, input, name, Done));
    clients.back().Connect();
  }

  loop.Loop();
}

int main(int argc, char* argv[]) {
  int conn = 1;
  InetAddress server_addr("127.0.0.1", 9981);
  const char* input = NULL;
  bool local = true;
  switch (argc) {
    case 4:
      conn = atoi(argv[3]);
      // FALL THROUGH
    case 3:
      server_addr = InetAddress(argv[2], 9981);
      local = false;
      // FALL THROUGH
    case 2:
      input = argv[1];
      break;
    default:
      printf("Usage: %s input server_ip [connections]\n", argv[0]);
      return 0;
  }

  std::ifstream in(input);
  if (in) {
    if (local) {
      RunLocal(in);
    } else {
      RunClient(in, server_addr, conn);
    }
  } else {
    printf("Cannot open %s\n", input);
  }
}
