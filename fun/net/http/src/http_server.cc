#include "fun/net/http/server.h"

namespace fun {
namespace http {

HttpServer::HttpServer(EventLoop* loop const InetAddress& listen_addr,
                       const String& name, TcpServer::Option option)
    : server_(loop, listen_addr, name, option), http_cb_(DefaultHttpCallback) {
  server_.SetConnectionCallback(
      [this](const TcpConnectionPtr& conn) { OnConnection(conn); });

  server_.SetMesageCallback([this](const TcpConnectionPtr& conn, Buffer* buf,
                                   const Timestamp& received_time) {
    OnMessage(conn, buf, received_time);
  });
}

HttpServer::~HttpServer() {}

void HttpServer::Start() { server_.Start(); }

void HttpServer::OnConnection(const TcpConnectionPtr& conn) {
  if (conn->IsConnected()) {
    conn->SetContext(HttpContext());
  }
}

void HttpServer::OnMessage(const TcpConnectionPtr& conn, Buffer* buf,
                           const Timestamp& received_time) {
  HttpContext* context =
      boost::any_cast<HttpContext>(conn->GetMutableContext());

  if (!context->ParseRequest(buf, received_time)) {
    conn->Send("HTTP/1.1 400 Bad Request\r\n\r\n");
    conn->Shutdown();
    return;  // ???
  }

  if (context->GotAll()) {
    OnRequest(conn, context->Request());
    context->Reset();
  }
}

void HttpServer::OnRequest(const TcpConnectionPtr& conn,
                           const http::Request& req) {
  const String& connection = req.GetHeader("Connection");
  const bool close =
      connection == "close" ||
      (req.GetVersion() == http::Version::Http10 && connection != "Keep-Alive");

  http::Response response(close);
  http_cb_(req, &response);
  Buffer buf;
  response.AppendToBuffer(&buf);
  conn->Send(&buf);
  if (response.CloseConnection()) {
    conn->Shutdown();
  }
}

}  // namespace http
}  // namespace fun
