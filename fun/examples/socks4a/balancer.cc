#include "tunnel.h"

#include "fun/base/thread_local.h"

#include <stdio.h>

using namespace fun;
using namespace fun::net;

std::vector<InetAddress> g_backends;
ThreadLocal<std::map<String, TunnelPtr> > t_tunnels;
MutexLock g_mutex;
size_t g_current = 0;

void OnServerConnection(const TcpConnectionPtr& conn)
{
  LOG_DEBUG << (conn->IsConnected() ? "UP" : "DOWN");

  std::map<String, TunnelPtr>& tunnels = t_tunnels.value();
  if (conn->IsConnected()) {
    conn->SetTcpNoDelay(true);
    conn->StopRead();
    size_t current = 0;
    {
      ScopedLock guard(g_mutex);
      current = g_current;
      g_current = (g_current+1) % g_backends.size();
    }

    InetAddress backend = g_backends[current];
    TunnelPtr tunnel(new Tunnel(conn->GetLoop(), backend, conn));
    tunnel->setup();
    tunnel->Connect();

    tunnels[conn->GetName()] = tunnel;
  }
  else {
    fun_check(tunnels.find(conn->GetName()) != tunnels.end());
    tunnels[conn->GetName()]->Disconnect();
    tunnels.erase(conn->GetName());
  }
}

void OnServerMessage( const TcpConnectionPtr& conn,
                      Buffer* buf,
                      const Timestamp&)
{
  if (!conn->GetContext().empty()) {
    const TcpConnectionPtr& client_conn
      = boost::any_cast<const TcpConnectionPtr&>(conn->GetContext());
    client_conn->Send(buf);
  }
}

int main(int argc, char* argv[])
{
  if (argc < 3) {
    fprintf(stderr, "Usage: %s listen_port backend_ip:port [backend_ip:port]\n", argv[0]);
  }
  else {
    for (int i = 2; i < argc; ++i) {
      String hostport = argv[i];
      size_t colon = hostport.find(':');
      if (colon != String::npos) {
        String ip = hostport.substr(0, colon);
        uint16_t port = static_cast<uint16_t>(atoi(hostport.c_str()+colon+1));
        g_backends.push_back(InetAddress(ip, port));
      }
      else {
        fprintf(stderr, "invalid backend address %s\n", argv[i]);
        return 1;
      }
    }

    uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
    InetAddress listen_addr(port);

    EventLoop loop;
    TcpServer server(&loop, listen_addr, "TcpBalancer");
    server.SetConnectionCallback(OnServerConnection);
    server.SetMessageCallback(OnServerMessage);
    server.SetThreadCount(4);
    server.Start();
    loop.Loop();
  }
}
