#include <examples/protobuf/codec/query.pb.h>
#include "codec.h"
#include "dispatcher.h"

#include "fun/base/logging.h"
#include "fun/base/mutex.h"
#include "fun/net/event_loop.h"
#include "fun/net/tcp_server.h"

#include <boost/bind.hpp>

#include <stdio.h>
#include <unistd.h>

using namespace fun;
using namespace fun::net;

typedef fun::SharedPtr<fun::Query> QueryPtr;
typedef fun::SharedPtr<fun::Answer> AnswerPtr;

class QueryServer : Noncopyable {
 public:
  QueryServer(EventLoop* loop, const InetAddress& listen_addr)
      : server_(loop, listen_addr, "QueryServer"),
        dispatcher_(
            boost::bind(&QueryServer::OnUnknownMessage, this, _1, _2, _3)),
        codec_(boost::bind(&ProtobufDispatcher::OnProtobufMessage, &dispatcher_,
                           _1, _2, _3)) {
    dispatcher_.RegisterMessageCallback<fun::Query>(
        boost::bind(&QueryServer::OnQuery, this, _1, _2, _3));
    dispatcher_.RegisterMessageCallback<fun::Answer>(
        boost::bind(&QueryServer::OnAnswer, this, _1, _2, _3));
    server_.SetConnectionCallback(
        boost::bind(&QueryServer::OnConnection, this, _1));
    server_.SetMessageCallback(
        boost::bind(&ProtobufCodec::OnMessage, &codec_, _1, _2, _3));
  }

  void Start() { server_.Start(); }

 private:
  void OnConnection(const TcpConnectionPtr& conn) {
    LOG_INFO << conn->GetLocalAddress().ToIpPort() << " -> "
             << conn->GetPeerAddress().ToIpPort() << " is "
             << (conn->IsConnected() ? "UP" : "DOWN");
  }

  void OnUnknownMessage(const TcpConnectionPtr& conn, const MessagePtr& message,
                        const Timestamp&) {
    LOG_INFO << "OnUnknownMessage: " << message->GetTypeName();
    conn->Shutdown();
  }

  void OnQuery(const fun::net::TcpConnectionPtr& conn, const QueryPtr& message,
               const fun::Timestamp&) {
    LOG_INFO << "OnQuery:\n"
             << message->GetTypeName() << message->DebugString();
    Answer answer;
    answer.set_id(1);
    answer.set_questioner("Chen Shuo");
    answer.set_answerer("blog.csdn.net/Solstice");
    answer.add_solution("Jump!");
    answer.add_solution("Win!");
    codec_.Send(conn, answer);

    conn->Shutdown();
  }

  void OnAnswer(const fun::net::TcpConnectionPtr& conn,
                const AnswerPtr& message, const fun::Timestamp&) {
    LOG_INFO << "OnAnswer: " << message->GetTypeName();
    conn->Shutdown();
  }

  TcpServer server_;
  ProtobufDispatcher dispatcher_;
  ProtobufCodec codec_;
};

int main(int argc, char* argv[]) {
  LOG_INFO << "pid = " << Process::CurrentPid();

  if (argc > 1) {
    EventLoop loop;
    uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
    InetAddress server_addr(port);
    QueryServer server(&loop, server_addr);
    server.Start();
    loop.Loop();
  } else {
    printf("Usage: %s port\n", argv[0]);
  }
}
