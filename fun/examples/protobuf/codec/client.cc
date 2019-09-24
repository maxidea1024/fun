#include "dispatcher.h"
#include "codec.h"
#include <examples/protobuf/codec/query.pb.h>

#include "fun/base/logging.h"
#include "fun/base/mutex.h"
#include "fun/net/event_loop.h"
#include "fun/net/tcp_client.h"

#include <boost/bind.hpp>

#include <stdio.h>
#include <unistd.h>

using namespace fun;
using namespace fun::net;

typedef fun::SharedPtr<fun::Empty> EmptyPtr;
typedef fun::SharedPtr<fun::Answer> AnswerPtr;

google::protobuf::Message* message_to_send;

class QueryClient : Noncopyable
{
 public:
  QueryClient(EventLoop* loop, const InetAddress& server_addr)
  : loop_(loop)
  , client_(loop, server_addr, "QueryClient")
  , dispatcher_(boost::bind(&QueryClient::OnUnknownMessage, this, _1, _2, _3))
  , codec_(boost::bind(&ProtobufDispatcher::OnProtobufMessage, &dispatcher_, _1, _2, _3))
  {
    dispatcher_.RegisterMessageCallback<fun::Answer>(boost::bind(&QueryClient::OnAnswer, this, _1, _2, _3));
    dispatcher_.RegisterMessageCallback<fun::Empty>(boost::bind(&QueryClient::OnEmpty, this, _1, _2, _3));
    client_.SetConnectionCallback(boost::bind(&QueryClient::OnConnection, this, _1));
    client_.SetMessageCallback(boost::bind(&ProtobufCodec::OnMessage, &codec_, _1, _2, _3));
  }

  void Connect()
  {
    client_.Connect();
  }

 private:
  void OnConnection(const TcpConnectionPtr& conn)
  {
    LOG_INFO << conn->GetLocalAddress().ToIpPort() << " -> "
        << conn->GetPeerAddress().ToIpPort() << " is "
        << (conn->IsConnected() ? "UP" : "DOWN");

    if (conn->IsConnected())
    {
      codec_.Send(conn, *message_to_send);
    }
    else
    {
      loop_->Quit();
    }
  }

  void OnUnknownMessage(const TcpConnectionPtr&,
                        const MessagePtr& message,
                        const Timestamp&)
  {
    LOG_INFO << "OnUnknownMessage: " << message->GetTypeName();
  }

  void OnAnswer(const fun::net::TcpConnectionPtr&,
                const AnswerPtr& message,
                const Timestamp&)
  {
    LOG_INFO << "OnAnswer:\n" << message->GetTypeName() << message->DebugString();
  }

  void OnEmpty(const fun::net::TcpConnectionPtr&,
               const EmptyPtr& message,
               const Timestamp&)
  {
    LOG_INFO << "OnEmpty: " << message->GetTypeName();
  }

  EventLoop* loop_;
  TcpClient client_;
  ProtobufDispatcher dispatcher_;
  ProtobufCodec codec_;
};


int main(int argc, char* argv[])
{
  LOG_INFO << "pid = " << Process::CurrentPid();
  if (argc > 2)
  {
    EventLoop loop;
    uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
    InetAddress server_addr(argv[1], port);

    fun::Query query;
    query.set_id(1);
    query.set_questioner("Chen Shuo");
    query.add_question("Running?");
    fun::Empty empty;
    message_to_send = &query;

    if (argc > 3 && argv[3][0] == 'e')
    {
      message_to_send = &empty;
    }

    QueryClient client(&loop, server_addr);
    client.Connect();
    loop.Loop();
  }
  else
  {
    printf("Usage: %s host_ip port [q|e]\n", argv[0]);
  }
}
