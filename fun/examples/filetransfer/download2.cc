#include "fun/base/logging.h"
#include "fun/net/event_loop.h"
#include "fun/net/tcp_server.h"

#include <stdio.h>
#include <unistd.h>

using namespace fun;
using namespace fun::net;

void OnHighWaterMark(const TcpConnectionPtr& conn, size_t len) {
  LOG_INFO << "HighWaterMark " << len;
}

const int kBufSize = 64 * 1024;
const char* g_file = nullptr;

void OnConnection(const TcpConnectionPtr& conn) {
  LOG_INFO << "FileServer - " << conn->GetPeerAddress().ToIpPort() << " -> "
           << conn->GetLocalAddress().ToIpPort() << " is "
           << (conn->IsConnected() ? "UP" : "DOWN");

  if (conn->IsConnected()) {
    LOG_INFO << "FileServer - Sending file " << g_file << " to "
             << conn->GetPeerAddress().ToIpPort();

    conn->SetHighWaterMarkCallback(OnHighWaterMark, kBufSize + 1);

    FILE* fp = ::fopen(g_file, "rb");
    if (fp) {
      conn->SetContext(fp);
      char buf[kBufSize];
      size_t nread = ::fread(buf, 1, sizeof buf, fp);
      conn->Send(buf, static_cast<int>(nread));
    } else {
      conn->Shutdown();
      LOG_INFO << "FileServer - no such file";
    }
  } else {
    if (!conn->GetContext().empty()) {
      FILE* fp = boost::any_cast<FILE*>(conn->GetContext());
      if (fp) {
        ::fclose(fp);
      }
    }
  }
}

void OnWriteComplete(const TcpConnectionPtr& conn) {
  FILE* fp = boost::any_cast<FILE*>(conn->GetContext());
  char buf[kBufSize];
  size_t nread = ::fread(buf, 1, sizeof buf, fp);
  if (nread > 0) {
    conn->Send(buf, static_cast<int>(nread));
  } else {
    ::fclose(fp);
    fp = NULL;
    conn->SetContext(fp);
    conn->Shutdown();
    LOG_INFO << "FileServer - done";
  }
}

int main(int argc, char* argv[]) {
  LOG_INFO << "pid = " << Process::CurrentPid();

  if (argc > 1) {
    g_file = argv[1];

    EventLoop loop;
    InetAddress listen_addr(2021);
    TcpServer server(&loop, listen_addr, "FileServer");
    server.SetConnectionCallback(OnConnection);
    server.SetWriteCompleteCallback(OnWriteComplete);
    server.Start();
    loop.Loop();
  } else {
    fprintf(stderr, "Usage: %s file_for_downloading\n", argv[0]);
  }
}
