#pragma once

#include "fun/net/tcp_server.h"

// RFC 868
class TimeServer {
 public:
  TimeServer(fun::net::EventLoop* loop,
             const fun::net::InetAddress& listen_addr);

  void Start();

 private:
  void OnConnection(const fun::net::TcpConnectionPtr& conn);

  void OnMessage(const fun::net::TcpConnectionPtr& conn, fun::net::Buffer* buf,
                 const fun::Timestamp& time);

  fun::net::TcpServer server_;
};
