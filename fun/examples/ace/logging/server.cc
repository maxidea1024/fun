#include <examples/ace/logging/logrecord.pb.h>

#include "fun/base/atomic.h"
#include "fun/base/file_util.h"
#include "fun/base/logging.h"
#include "fun/net/event_loop.h"
#include "fun/net/protobuf/ProtobufCodecLite.h"
#include "fun/net/tcp_server.h"

#include <boost/bind.hpp>

#include <stdio.h>

using namespace fun;
using namespace fun::net;

namespace logging {

extern const char logtag[] = "LOG0";
typedef ProtobufCodecLiteT<LogRecord, logtag> Codec;

class Session : Noncopyable {
 public:
  explicit Session(const TcpConnectionPtr& conn)
      : codec_(boost::bind(&Session::OnMessage, this, _1, _2, _3)),
        file_(GetFileName(conn)) {
    conn->SetMessageCallback(
        boost::bind(&Codec::OnMessage, &codec_, _1, _2, _3));
  }

 private:
  // FIXME: duplicate code LogFile
  // or use LogFile instead
  String GetFileName(const TcpConnectionPtr& conn) {
    String filename;
    filename += conn->GetPeerAddress().ToIp();

    char timebuf[32];
    struct tm tm;
    time_t now = time(NULL);
    gmtime_r(&now, &tm);  // FIXME: localtime_r ?
    strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S.", &tm);
    filename += timebuf;

    char buf[32];
    snprintf(buf, sizeof buf, "%d", global_count_.IncrementAndGet());
    filename += buf;

    filename += ".log";
    LOG_INFO << "Session of " << conn->GetName() << " file " << filename;
    return filename;
  }

  void OnMessage(const TcpConnectionPtr& conn, const MessagePtr& message,
                 const Timestamp& time) {
    LogRecord* log_record = fun::down_cast<LogRecord*>(message.get());
    if (log_record->has_heartbeat()) {
      // FIXME ?
    }
    const char* sep = "==========\n";
    String str = log_record->DebugString();
    file_.append(str.c_str(), str.size());
    file_.append(sep, strlen(sep));
    LOG_DEBUG << str;
  }

  Codec codec_;
  FileUtil::AppendFile file_;
  static AtomicInt32 global_count_;
};

typedef fun::SharedPtr<Session> SessionPtr;

AtomicInt32 Session::global_count_;

class LogServer : Noncopyable {
 public:
  LogServer(EventLoop* loop, const InetAddress& listen_addr, int thread_count)
      : loop_(loop), server_(loop_, listen_addr, "AceLoggingServer") {
    server_.SetConnectionCallback(
        boost::bind(&LogServer::OnConnection, this, _1));
    if (thread_count > 1) {
      server_.SetThreadCount(thread_count);
    }
  }

  void Start() { server_.Start(); }

 private:
  void OnConnection(const TcpConnectionPtr& conn) {
    if (conn->IsConnected()) {
      SessionPtr session(new Session(conn));
      conn->SetContext(session);
    } else {
      conn->SetContext(SessionPtr());
    }
  }

  EventLoop* loop_;
  TcpServer server_;
};

}  // namespace logging

int main(int argc, char* argv[]) {
  EventLoop loop;
  int port = argc > 1 ? atoi(argv[1]) : 50000;
  LOG_INFO << "Listen on port " << port;
  InetAddress listen_addr(static_cast<uint16_t>(port));
  int thread_count = argc > 2 ? atoi(argv[2]) : 1;
  logging::LogServer server(&loop, listen_addr, thread_count);
  server.Start();
  loop.Loop();
}
