#include "dispatcher.h"

#include <examples/protobuf/codec/query.pb.h>

#include <iostream>

using std::cout;
using std::endl;

typedef fun::SharedPtr<fun::Query> QueryPtr;
typedef fun::SharedPtr<fun::Answer> AnswerPtr;

void test_down_pointer_cast()
{
  ::fun::SharedPtr<google::protobuf::Message> msg(new fun::Query);
  ::fun::SharedPtr<fun::Query> query(fun::down_pointer_cast<fun::Query>(msg));
  fun_check(msg && query);
  if (!query)
  {
    abort();
  }
}

void OnQuery(const fun::net::TcpConnectionPtr&,
             const QueryPtr& message,
             const fun::Timestamp&)
{
  cout << "OnQuery: " << message->GetTypeName() << endl;
}

void OnAnswer(const fun::net::TcpConnectionPtr&,
              const AnswerPtr& message,
              const fun::Timestamp&)
{
  cout << "OnAnswer: " << message->GetTypeName() << endl;
}

void OnUnknownMessageType(const fun::net::TcpConnectionPtr&,
                          const MessagePtr& message,
                          fun::Timestamp)
{
  cout << "OnUnknownMessageType: " << message->GetTypeName() << endl;
}


int main()
{
  GOOGLE_PROTOBUF_VERIFY_VERSION;
  test_down_pointer_cast();

  ProtobufDispatcher dispatcher(OnUnknownMessageType);
  dispatcher.RegisterMessageCallback<fun::Query>(OnQuery);
  dispatcher.RegisterMessageCallback<fun::Answer>(OnAnswer);

  fun::net::TcpConnectionPtr conn;
  fun::Timestamp t;

  fun::SharedPtr<fun::Query> query(new fun::Query);
  fun::SharedPtr<fun::Answer> answer(new fun::Answer);
  fun::SharedPtr<fun::Empty> empty(new fun::Empty);
  dispatcher.OnProtobufMessage(conn, query, t);
  dispatcher.OnProtobufMessage(conn, answer, t);
  dispatcher.OnProtobufMessage(conn, empty, t);

  google::protobuf::ShutdownProtobufLibrary();
}
