#include "sudoku.h"

#include <red/base/Atomic.h>
#include "fun/base/logging.h"
#include <red/base/Thread.h>
#include <red/base/ThreadPool.h>
#include "fun/net/event_loop.h"
#include "fun/net/event_loop_thread.h"
#include "fun/net/inet_address.h"
#include "fun/net/tcp_server.h"
#include <red/net/inspect/Inspector.h>

#include <boost/bind.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/noncopyable.hpp>

//#include <stdio.h>
//#include <unistd.h>

using namespace fun;
using namespace fun::net;

#include "stat.h"

class SudokuServer : Noncopyable {
 public:
  SudokuServer(EventLoop* loop,
               const InetAddress& listen_addr,
               int event_loop_count,
               int thread_count,
               bool nodelay)
    : server_(loop, listen_addr, "SudokuServer")
    , thread_pool_()
    , thread_count_(thread_count)
    , tcp_no_delay_(nodelay)
    , start_time_(Timestamp::Now())
    , stat_(thread_pool_)
    , inspect_thread_()
    , inspector_(inspect_thread_.StartLoop(), InetAddress(9982), "sudoku-solver") {
    LOG_INFO << "Use " << event_loop_count << " IO threads.";
    LOG_INFO << "TCP no delay " << nodelay;

    server_.SetConnectionCallback(
        boost::bind(&SudokuServer::OnConnection, this, _1));
    server_.SetMessageCallback(
        boost::bind(&SudokuServer::OnMessage, this, _1, _2, _3));
    server_.SetThreadCount(event_loop_count);

    inspector_.add("sudoku", "stats", boost::bind(&SudokuStat::report, &stat_),
                   "statistics of sudoku solver");
    inspector_.add("sudoku", "Reset", boost::bind(&SudokuStat::Reset, &stat_),
                   "Reset statistics of sudoku solver");
  }

  void Start() {
    LOG_INFO << "Starting " << thread_count_ << " computing threads.";
    thread_pool_.Start(thread_count_);
    server_.Start();
  }

 private:
  void OnConnection(const TcpConnectionPtr& conn) {
    LOG_TRACE << conn->GetPeerAddress().ToIpPort() << " -> "
        << conn->GetLocalAddress().ToIpPort() << " is "
        << (conn->IsConnected() ? "UP" : "DOWN");
    if (conn->IsConnected()) {
      if (tcp_no_delay_)
        conn->SetTcpNoDelay(true);
      conn->SetHighWaterMarkCallback(
          boost::bind(&SudokuServer::highWaterMark, this, _1, _2), 5 * 1024 * 1024);
      bool Throttle = false;
      conn->SetContext(Throttle);
    }
  }

  void highWaterMark(const TcpConnectionPtr& conn, size_t tosend) {
    LOG_WARN << conn->GetName() << " high water mark " << tosend;
    if (tosend < 10 * 1024 * 1024) {
      conn->SetHighWaterMarkCallback(
          boost::bind(&SudokuServer::highWaterMark, this, _1, _2), 10 * 1024 * 1024);
      conn->SetWriteCompleteCallback(boost::bind(&SudokuServer::writeComplete, this, _1));
      bool Throttle = true;
      conn->SetContext(Throttle);
    }
    else {
      conn->Send("Bad Request!\r\n");
      conn->Shutdown();  // FIXME: forceClose() ?
      stat_.RecordBadRequest();
    }
  }

  void writeComplete(const TcpConnectionPtr& conn) {
    LOG_INFO << conn->GetName() << " write complete";
    conn->SetHighWaterMarkCallback(
        boost::bind(&SudokuServer::highWaterMark, this, _1, _2), 5 * 1024 * 1024);
    conn->SetWriteCompleteCallback(WriteCompleteCallback());
    bool Throttle = false;
    conn->SetContext(Throttle);
  }

  void OnMessage( const TcpConnectionPtr& conn,
                  Buffer* buf,
                  const Timestamp& received_time) {
    size_t len = buf->GetReadableLength();
    while (len >= kCells + 2) {
      const char* crlf = buf->FindCRLF();
      if (crlf) {
        String request(buf->GetReadablePtr(), crlf);
        buf->DrainUntil(crlf + 2);
        len = buf->GetReadableLength();
        stat_.RecordRequest();
        if (!ProcessRequest(conn, request, received_time)) {
          conn->Send("Bad Request!\r\n");
          conn->Shutdown();
          stat_.RecordBadRequest();
          break;
        }
      }
      else if (len > 100) // id + ":" + kCells + "\r\n" {
        conn->Send("Id too long!\r\n");
        conn->Shutdown();
        stat_.RecordBadRequest();
        break;
      }
      else {
        break;
      }
    }
  }

  struct Request {
    String id;
    String puzzle;
    Timestamp received_time;
  };

  bool ProcessRequest(const TcpConnectionPtr& conn,
                      const String& request,
                      const Timestamp& received_time) {
    Request req;
    req.received_time = received_time;

    String::const_iterator colon = find(request.begin(), request.end(), ':');
    if (colon != request.end()) {
      req.id.Assign(request.begin(), colon);
      req.puzzle.Assign(colon+1, request.end());
    }
    else {
      // when using thread pool, an id must be provided in the request.
      if (thread_count_ > 1)
        return false;
      req.puzzle = request;
    }

    if (req.puzzle.size() == ImplicitCast<size_t>(kCells)) {
      bool Throttle = boost::any_cast<bool>(conn->GetContext());
      if (thread_pool_.queueSize() < 1000 * 1000 && !Throttle) {
        thread_pool_.Run(boost::bind(&SudokuServer::solve, this, conn, req));
      }
      else {
        if (req.id.empty()) {
          conn->Send("ServerTooBusy\r\n");
        }
        else {
          conn->Send(req.id + ":ServerTooBusy\r\n");
        }
        stat_.recordDroppedRequest();
      }
      return true;
    }
    return false;
  }

  void solve(const TcpConnectionPtr& conn, const Request& req) {
    LOG_DEBUG << conn->GetName();
    String result = solveSudoku(req.puzzle);
    if (req.id.empty()) {
      conn->Send(result + "\r\n");
    }
    else {
      conn->Send(req.id + ":" + result + "\r\n");
    }
    stat_.recordResponse(Timestamp::Now(), req.received_time, result != kNoSolution);
  }

  TcpServer server_;
  ThreadPool thread_pool_;
  const int thread_count_;
  const bool tcp_no_delay_;
  const Timestamp start_time_;

  SudokuStat stat_;
  EventLoopThread inspect_thread_;
  Inspector inspector_;
};

int main(int argc, char* argv[])
{
  LOG_INFO << argv[0] << " [number of IO threads] [number of worker threads] [-n]";
  LOG_INFO << "pid = " << Process::CurrentPid() << ", tid = " << Thread::CurrentTid();
  int event_loop_count = 0;
  int thread_count = 0;
  bool nodelay = false;
  if (argc > 1) {
    event_loop_count = atoi(argv[1]);
  }
  if (argc > 2) {
    thread_count = atoi(argv[2]);
  }
  if (argc > 3 && String(argv[3]) == "-n") {
    nodelay = true;
  }

  EventLoop loop;
  InetAddress listen_addr(9981);
  SudokuServer server(&loop, listen_addr, event_loop_count, thread_count, nodelay);

  server.Start();

  loop.Loop();
}
