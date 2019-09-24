#include "dispatcher_lite.h"

#include <examples/protobuf/codec/query.pb.h>

#include <iostream>

using std::cout;
using std::endl;

void OnUnknownMessageType(const fun::net::TcpConnectionPtr&,
                          const MessagePtr& message, fun::Timestamp) {
  cout << "OnUnknownMessageType: " << message->GetTypeName() << endl;
}

void OnQuery(const fun::net::TcpConnectionPtr&, const MessagePtr& message,
             fun::Timestamp) {
  cout << "OnQuery: " << message->GetTypeName() << endl;
  fun::SharedPtr<fun::Query> query =
      fun::down_pointer_cast<fun::Query>(message);
  fun_check(query != NULL);
}

void OnAnswer(const fun::net::TcpConnectionPtr&, const MessagePtr& message,
              fun::Timestamp) {
  cout << "OnAnswer: " << message->GetTypeName() << endl;
  fun::SharedPtr<fun::Answer> answer =
      fun::down_pointer_cast<fun::Answer>(message);
  fun_check(answer != NULL);
}

int main() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  ProtobufDispatcherLite dispatcher(OnUnknownMessageType);
  dispatcher.RegisterMessageCallback(fun::Query::descriptor(), OnQuery);
  dispatcher.RegisterMessageCallback(fun::Answer::descriptor(), OnAnswer);

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
