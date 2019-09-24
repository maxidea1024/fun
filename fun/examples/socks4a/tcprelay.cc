#include "tunnel.h"

#include <malloc.h>
#include <stdio.h>
#include <sys/resource.h>
#include <unistd.h>

using namespace fun;
using namespace fun::net;

EventLoop* g_event_loop;
InetAddress* g_server_addr;
std::map<String, TunnelPtr> g_tunnels;

void OnServerConnection(const TcpConnectionPtr& conn) {
  LOG_DEBUG << (conn->IsConnected() ? "UP" : "DOWN");
  if (conn->IsConnected()) {
    conn->SetTcpNoDelay(true);
    conn->StopRead();
    TunnelPtr tunnel(new Tunnel(g_event_loop, *g_server_addr, conn));
    tunnel->setup();
    tunnel->Connect();
    g_tunnels[conn->GetName()] = tunnel;
  } else {
    fun_check(g_tunnels.find(conn->GetName()) != g_tunnels.end());
    g_tunnels[conn->GetName()]->Disconnect();
    g_tunnels.erase(conn->GetName());
  }
}

void OnServerMessage(const TcpConnectionPtr& conn, Buffer* buf,
                     const Timestamp&) {
  LOG_DEBUG << buf->GetReadableLength();
  if (!conn->GetContext().empty()) {
    const TcpConnectionPtr& client_conn =
        boost::any_cast<const TcpConnectionPtr&>(conn->GetContext());
    client_conn->Send(buf);
  }
}

void memstat() { malloc_stats(); }

int main(int argc, char* argv[]) {
  if (argc < 4) {
    fprintf(stderr, "Usage: %s <host_ip> <port> <listen_port>\n", argv[0]);
  } else {
    LOG_INFO << "pid = " << Process::CurrentPid()
             << ", tid = " << Thread::CurrentTid();
    {
      // set max virtual memory to 256MB.
      size_t kOneMB = 1024 * 1024;
      rlimit rl = {256 * kOneMB, 256 * kOneMB};
      setrlimit(RLIMIT_AS, &rl);
    }
    const char* ip = argv[1];
    uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
    InetAddress server_addr(ip, port);
    g_server_addr = &server_addr;

    uint16_t accept_port = static_cast<uint16_t>(atoi(argv[3]));
    InetAddress listen_addr(accept_port);

    EventLoop loop;
    g_event_loop = &loop;
    loop.ScheduleEvery(3, memstat);

    TcpServer server(&loop, listen_addr, "TcpRelay");

    server.SetConnectionCallback(OnServerConnection);
    server.SetMessageCallback(OnServerMessage);

    server.Start();
    loop.Loop();
  }
}
