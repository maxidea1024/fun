#include "tunnel.h"

#include <red/net/Endian.h>
#include <stdio.h>
#include <netdb.h>
#include <unistd.h>

using namespace fun;
using namespace fun::net;

EventLoop* g_event_loop;
std::map<String, TunnelPtr> g_tunnels;

void OnServerConnection(const TcpConnectionPtr& conn)
{
  LOG_DEBUG << conn->GetName() << (conn->IsConnected() ? " UP" : " DOWN");
  if (conn->IsConnected()) {
    conn->SetTcpNoDelay(true);
  }
  else {
    std::map<String, TunnelPtr>::iterator it = g_tunnels.find(conn->GetName());
    if (it != g_tunnels.end()) {
      it->second->Disconnect();
      g_tunnels.erase(it);
    }
  }
}

void OnServerMessage( const TcpConnectionPtr& conn,
                      Buffer* buf, const Timestamp&)
{
  LOG_DEBUG << conn->GetName() << " " << buf->GetReadableLength();
  if (g_tunnels.find(conn->GetName()) == g_tunnels.end()) {
    if (buf->GetReadableLength() > 128) {
      conn->Shutdown();
    }
    else if (buf->GetReadableLength() > 8) {
      const char* begin = buf->GetReadablePtr() + 8;
      const char* end = buf->GetReadablePtr() + buf->GetReadableLength();
      const char* where = std::find(begin, end, '\0');
      if (where != end) {
        char ver = buf->GetReadablePtr()[0];
        char cmd = buf->GetReadablePtr()[1];
        const void* port = buf->GetReadablePtr() + 2;
        const void* ip = buf->GetReadablePtr() + 4;

        sockaddr_in addr;
        bzero(&addr, sizeof addr);
        addr.sin_family = AF_INET;
        addr.sin_port = *static_cast<const in_port_t*>(port);
        addr.sin_addr.s_addr = *static_cast<const uint32_t*>(ip);

        bool socks4a = sockets::networkToHost32(addr.sin_addr.s_addr) < 256;
        bool okay = false;
        if (socks4a) {
          const char* endOfHostName = std::find(where+1, end, '\0');
          if (endOfHostName != end) {
            String hostname = where+1;
            where = endOfHostName;
            LOG_INFO << "Socks4a host name " << hostname;
            InetAddress tmp;
            if (InetAddress::Resolve(hostname, &tmp)) {
              addr.sin_addr.s_addr = tmp.ipNetEndian();
              okay = true;
            }
          }
          else {
            return;
          }
        }
        else {
          okay = true;
        }

        InetAddress server_addr(addr);
        if (ver == 4 && cmd == 1 && okay) {
          TunnelPtr tunnel(new Tunnel(g_event_loop, server_addr, conn));
          tunnel->setup();
          tunnel->Connect();
          g_tunnels[conn->GetName()] = tunnel;
          buf->DrainUntil(where+1);
          char response[] = "\000\x5aUVWXYZ";
          UnsafeMemory::Memcpy(response+2, &addr.sin_port, 2);
          UnsafeMemory::Memcpy(response+4, &addr.sin_addr.s_addr, 4);
          conn->Send(response, 8);
        }
        else {
          char response[] = "\000\x5bUVWXYZ";
          conn->Send(response, 8);
          conn->Shutdown();
        }
      }
    }
  }
  else if (!conn->GetContext().empty()) {
    const TcpConnectionPtr& client_conn
      = boost::any_cast<const TcpConnectionPtr&>(conn->GetContext());
    client_conn->Send(buf);
  }
}

int main(int argc, char* argv[])
{
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <listen_port>\n", argv[0]);
  }
  else {
    LOG_INFO << "pid = " << Process::CurrentPid() << ", tid = " << Thread::CurrentTid();

    uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
    InetAddress listen_addr(port);

    EventLoop loop;
    g_event_loop = &loop;

    TcpServer server(&loop, listen_addr, "Socks4");
    server.SetConnectionCallback(OnServerConnection);
    server.SetMessageCallback(OnServerMessage);
    server.Start();
    loop.Loop();
  }
}
