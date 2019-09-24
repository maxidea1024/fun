

namespace fun {
namespace net {


namespace detail {

void DefaultHttpCallback(const HttpRequest&, HttpResponse* resp) {
  resp->SetStatusCode(HttpResponse::k404NotFound);
  resp->SetStatusMessage("Not Found");
  resp->SetCloseConnection(true);
}

} // namespace detail


HttpServer::HttpServer(EventLoop* loop,
                       const InetAddress& listen_addr,
                       const String& name,
                       TcpServer::Option option)
  : server_(loop, listen_addr, name, option)
  , http_cb_(detail::DefaultHttpCallback) {
  server_.SetConnectionCallback(boost::bind(&HttpServer::OnConnection, this, _1));
  server_.SetMessageCallback(boost::bind(&HttpServer::OnMessage, this, _1, _2, _3));
}

HttpServer::~HttpServer() {}

void HttpServer::Start() {
  LOG_WARN << "HttpServer[" << server_.GetName()
    << "] starts listenning on " << server_.ipPort();
  server_.Start();
}

void HttpServer::OnConnection(const TcpConnectionPtr& conn) {
  if (conn->IsConnected()) {
    conn->SetContext(HttpContext());
  }
}

void HttpServer::OnMessage(const TcpConnectionPtr& conn,
                           Buffer* buf,
                           const Timestamp& received_time) {
  HttpContext* context = boost::any_cast<HttpContext>(conn->GetMutableContext());

  if (!context->ParseRequest(buf, received_time)) {
    conn->Send("HTTP/1.1 400 Bad Request\r\n\r\n");
    conn->Shutdown();
  }

  if (context->GotAll()) {
    OnRequest(conn, context->GetRequest());
    context->Reset();
  }
}

void HttpServer::OnRequest( const TcpConnectionPtr& conn,
                            const HttpRequest& req) {
  const String& connection = req.GetHeader("Connection");
  bool close = connection == "close" ||
    (req.GetVersion() == HttpRequest::kHttp10 && connection != "Keep-Alive");
  HttpResponse response(close);
  http_cb_(req, &response);
  Buffer buf;
  response.AppendToBuffer(&buf);
  conn->Send(&buf);

  //TODO
  //여기서 바로 끊어버리면 안됨.  long transaction일 경우에는 connection을
  //유지해주어야함!
  if (response.CloseConnection()) {
    conn->Shutdown();
  }
}

} // namespace net
} // namespace fun
