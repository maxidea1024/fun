#include "examples/fastcgi/fastcgi.h"
#include "examples/sudoku/sudoku.h"

#include "fun/base/logging.h"
#include "fun/net/event_loop.h"
#include "fun/net/tcp_server.h"

#include <boost/bind.hpp>

using namespace fun::net;

const String kPath = "/sudoku/";

void OnRequest(const TcpConnectionPtr& conn,
               FastCgiCodec::ParamMap& params,
               Buffer* in) {
  String uri = params["REQUEST_URI"];
  LOG_INFO << conn->GetName() << ": " << uri;

  for (FastCgiCodec::ParamMap::const_iterator it = params.begin();
       it != params.end(); ++it) {
    LOG_DEBUG << it->first << " = " << it->second;
  }

  if (in->GetReadableLength() > 0) {
    LOG_DEBUG << "stdin " << in->ReadAllAsString();
  }

  Buffer response;
  response.append("Context-Type: text/plain\r\n\r\n");

  if (uri.size() == kCells + kPath.size() && uri.find(kPath) == 0) {
    response.append(solveSudoku(uri.substr(kPath.size())));
  }
  else {
    // FIXME: set http status code 400
    response.append("bad request");
  }

  FastCgiCodec::respond(&response);
  conn->Send(&response);
}

void OnConnection(const TcpConnectionPtr& conn) {
  if (conn->IsConnected()) {
    typedef fun::SharedPtr<FastCgiCodec> CodecPtr;
    CodecPtr codec(new FastCgiCodec(OnRequest));
    conn->SetContext(codec);
    conn->SetMessageCallback(boost::bind(&FastCgiCodec::OnMessage, codec, _1, _2, _3));
    conn->SetTcpNoDelay(true);
  }
}

int main(int argc, char* argv[]) {
  int port = 19981;
  int thread_count = 0;
  if (argc > 1) {
    port = atoi(argv[1]);
  }
  if (argc > 2) {
    thread_count = atoi(argv[2]);
  }

  InetAddress addr(static_cast<uint16_t>(port));
  LOG_INFO << "Sudoku FastCGI listens on " << addr.ToIpPort()
           << " thread_count " << thread_count;

   fun::net::EventLoop loop;
  TcpServer server(&loop, addr, "FastCGI");
  server.SetConnectionCallback(OnConnection);
  server.SetThreadCount(thread_count);
  server.Start();
  loop.Loop();
}
