#pragma once

#include "fun/net/reactor/tcp_server.h"

namespace fun {
namespace net {

class HttpRequest;
class HttpResponse;

/**
 * A simple embeddable HTTP server designed for report status of a program.
 * it is not a fully HTTP 1.1 compliant server, but provides minimum features
 * that can communicate with HttpClient and Web browser.
 * it is synchronous, just like Java Servlet.
 */
class HttpServer : Noncopyable {
 public:
  typedef Function<void (const HttpRequest&, HttpResponse*)> HttpCallback;

  HttpServer(EventLoop* loop,
             const InetAddress& listen_addr,
             const string& name,
             TcpServer::Option option = TcpServer::kNoReusePort);

  ~HttpServer(); // force out-line dtor, for scoped_ptr members_.

  EventLoop* GetLoop() const {
    return server_.GetLoop();
  }

  /**
   * Not thread safe, callback be registered before calling Start().
   */
  void SetHttpCallback(const HttpCallback& cb) {
    http_cb_ = cb;
  }

  void SetThreadCount(int thread_count) {
    server_.SetThreadCount(thread_count);
  }

  void Start();

 private:
  void OnConnection(const TcpConnectionPtr& conn);

  void OnMessage(const TcpConnectionPtr& conn,
                 Buffer* buf,
                 const Timestamp& received_time);

  void OnRequest(const TcpConnectionPtr&, const HttpRequest&);

  TcpServer server_;
  HttpCallback http_cb_;
};

} // namespace net
} // namespace fun
