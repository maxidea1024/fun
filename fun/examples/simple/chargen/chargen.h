#pragma once

#include "fun/net/tcp_server.h"

// RFC 864
class ChargenServer {
 public:
  ChargenServer(fun::net::EventLoop* loop,
                const fun::net::InetAddress& listen_addr, bool print = false);

  void Start();

 private:
  void OnConnection(const fun::net::TcpConnectionPtr& conn);

  void OnMessage(const fun::net::TcpConnectionPtr& conn, fun::net::Buffer* buf,
                 const fun::Timestamp& time);

  void OnWriteComplete(const fun::net::TcpConnectionPtr& conn);
  void PrintThroughput();

  fun::net::TcpServer server_;

  String message_;
  int64_t transferred_;
  fun::Timestamp start_time_;
};
