#pragma once

#include "fun/base/function.h"
#include "fun/net/tcp_client.h"
#include "fun/net/tcp_connection.h"

namespace pubsub {

// FIXME: dtor is not thread safe
class PubSubClient : Noncopyable {
 public:
  typedef Function<void(PubSubClient*)> ConnectionCallback;

  typedef Function<void(const String& topic, const String& content,
                        const Timestamp&)>
      SubscribeCallback;

  PubSubClient(EventLoop* loop, const InetAddress& hub_addr,
               const String& name);

  void Start();

  void Stop();

  bool IsConnected() const;

  void SetConnectionCallback(const ConnectionCallback& cb) {
    connection_cb_ = cb;
  }

  // TODO topic에 wildcard 혹은 regex를 적용해주는게 좋을듯...
  bool Subscribe(const String& topic, const SubscribeCallback& cb);

  void Unsubscribe(const String& topic);

  bool Publish(const String& topic, const String& content);

 private:
  void OnConnection(const TcpConnectionPtr& conn);

  void OnMessage(const TcpConnectionPtr& conn, Buffer* buf,
                 const Timestamp& received_time);

  bool Send(const String& message);

  TcpClient client_;
  TcpConnectionPtr conn_;
  ConnectionCallback connection_cb_;
  SubscribeCallback subscribe_cb_;
};

}  // namespace pubsub
