#include "fun/base/logging.h"
#include "fun/net/event_loop.h"
#include "fun/net/tcp_server.h"

#include <stdio.h>
#include <unistd.h>

using namespace fun;
using namespace fun::net;

const char* g_file = nullptr;

// FIXME: use FileUtil::ReadFile()
String ReadFile(const char* filename) {
  String content;
  FILE* fp = ::fopen(filename, "rb");
  if (fp) {
    // inefficient!!!
    const int kBufSize = 1024 * 1024;
    char iobuf[kBufSize];
    ::setbuffer(fp, iobuf, sizeof iobuf);

    char buf[kBufSize];
    size_t nread = 0;
    while ((nread = ::fread(buf, 1, sizeof buf, fp)) > 0) {
      content.Append(buf, nread);
    }
    ::fclose(fp);
  }
  return content;
}

void OnHighWaterMark(const TcpConnectionPtr& conn, size_t len) {
  LOG_INFO << "HighWaterMark " << len;
}

void OnConnection(const TcpConnectionPtr& conn) {
  LOG_INFO << "FileServer - " << conn->GetPeerAddress().ToIpPort() << " -> "
           << conn->GetLocalAddress().ToIpPort() << " is "
           << (conn->IsConnected() ? "UP" : "DOWN");

  if (conn->IsConnected()) {
    LOG_INFO << "FileServer - Sending file " << g_file << " to "
             << conn->GetPeerAddress().ToIpPort();

    conn->SetHighWaterMarkCallback(OnHighWaterMark, 64 * 1024);

    String file_content = ReadFile(g_file);
    conn->Send(file_content);
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
    server.Start();
    loop.Loop();
  } else {
    fprintf(stderr, "Usage: %s file_for_downloading\n", argv[0]);
  }
}
