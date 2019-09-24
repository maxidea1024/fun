#include "pubsub.h"
#include "codec.h"

#include <boost/bind.hpp>

using namespace fun;
using namespace fun::net;
using namespace pubsub;

PubSubClient::PubSubClient(EventLoop* loop, const InetAddress& hub_addr,
                           const String& name)
    : client_(loop, hub_addr, name) {
  // FIXME: dtor is not thread safe
  client_.SetConnectionCallback(
      boost::bind(&PubSubClient::OnConnection, this, _1));
  client_.SetMessageCallback(
      boost::bind(&PubSubClient::OnMessage, this, _1, _2, _3));
}

void PubSubClient::Start() { client_.Connect(); }

void PubSubClient::Stop() { client_.Disconnect(); }

bool PubSubClient::IsConnected() const { return conn_ && conn_->IsConnected(); }

bool PubSubClient::Subscribe(const String& topic, const SubscribeCallback& cb) {
  String message = "sub " + topic + "\r\n";
  subscribe_cb_ = cb;
  return Send(message);
}

void PubSubClient::Unsubscribe(const String& topic) {
  String message = "unsub " + topic + "\r\n";
  Send(message);
}

bool PubSubClient::Publish(const String& topic, const String& content) {
  String message = "pub " + topic + "\r\n" + content + "\r\n";
  return Send(message);
}

void PubSubClient::OnConnection(const TcpConnectionPtr& conn) {
  if (conn->IsConnected()) {
    conn_ = conn;
    // FIXME: re-sub
  } else {
    conn_.Reset();
  }

  if (connection_cb_) {
    connection_cb_(this);
  }
}

void PubSubClient::OnMessage(const TcpConnectionPtr& conn, Buffer* buf,
                             const Timestamp& received_time) {
  ParseResult result = kSuccess;
  while (result == kSuccess) {
    String cmd, topic, content;
    result = ParseMessage(buf, &cmd, &topic, &content);
    if (result == kSuccess) {
      if (cmd == "pub" && subscribe_cb_) {
        subscribe_cb_(topic, content, received_time);
      }
    } else if (result == kError) {
      conn->Shutdown();
    }
  }
}

bool PubSubClient::Send(const String& message) {
  bool succeed = false;
  if (conn_ && conn_->IsConnected()) {
    conn_->Send(message);
    succeed = true;
  }
  return succeed;
}
