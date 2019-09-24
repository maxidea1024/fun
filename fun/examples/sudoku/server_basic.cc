#include "sudoku.h"

#include <red/base/Atomic.h>
#include <red/base/Thread.h>
#include "fun/base/logging.h"
#include "fun/net/event_loop.h"
#include "fun/net/inet_address.h"
#include "fun/net/tcp_server.h"

#include <boost/bind.hpp>

#include <utility>

#include <stdio.h>
#include <unistd.h>

using namespace fun;
using namespace fun::net;

class SudokuServer {
 public:
  SudokuServer(EventLoop* loop, const InetAddress& listen_addr)
      : server_(loop, listen_addr, "SudokuServer"),
        start_time_(Timestamp::Now()) {
    server_.SetConnectionCallback(
        boost::bind(&SudokuServer::OnConnection, this, _1));
    server_.SetMessageCallback(
        boost::bind(&SudokuServer::OnMessage, this, _1, _2, _3));
  }

  void Start() { server_.Start(); }

 private:
  void OnConnection(const TcpConnectionPtr& conn) {
    LOG_TRACE << conn->GetPeerAddress().ToIpPort() << " -> "
              << conn->GetLocalAddress().ToIpPort() << " is "
              << (conn->IsConnected() ? "UP" : "DOWN");
  }

  void OnMessage(const TcpConnectionPtr& conn, Buffer* buf, const Timestamp&) {
    LOG_DEBUG << conn->GetName();
    size_t len = buf->GetReadableLength();
    while (len >= kCells + 2) {
      const char* crlf = buf->FindCRLF();
      if (crlf) {
        String request(buf->GetReadablePtr(), crlf);
        buf->DrainUntil(crlf + 2);
        len = buf->GetReadableLength();
        if (!ProcessRequest(conn, request)) {
          conn->Send("Bad Request!\r\n");
          conn->Shutdown();
          break;
        }
      } else if (len > 100) {  // id + ":" + kCells + "\r\n"
        conn->Send("Id too long!\r\n");
        conn->Shutdown();
        break;
      } else {
        break;
      }
    }
  }

  bool ProcessRequest(const TcpConnectionPtr& conn, const String& request) {
    String id;
    String puzzle;
    bool good_request = true;

    String::const_iterator colon = find(request.begin(), request.end(), ':');
    if (colon != request.end()) {
      id.Assign(request.begin(), colon);
      puzzle.Assign(colon + 1, request.end());
    } else {
      puzzle = request;
    }

    if (puzzle.size() == ImplicitCast<size_t>(kCells)) {
      LOG_DEBUG << conn->GetName();
      String result = solveSudoku(puzzle);
      if (id.empty()) {
        conn->Send(result + "\r\n");
      } else {
        conn->Send(id + ":" + result + "\r\n");
      }
    } else {
      good_request = false;
    }
    return good_request;
  }

  TcpServer server_;
  Timestamp start_time_;
};

int main(int argc, char* argv[]) {
  LOG_INFO << "pid = " << Process::CurrentPid()
           << ", tid = " << Thread::CurrentTid();
  EventLoop loop;
  InetAddress listen_addr(9981);
  SudokuServer server(&loop, listen_addr);

  server.Start();

  loop.Loop();
}
