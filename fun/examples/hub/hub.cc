#include "codec.h"

#include "fun/base/logging.h"
#include "fun/net/event_loop.h"
#include "fun/net/tcp_server.h"

#include <boost/bind.hpp>

#include <stdio.h>
#include <map>
#include <set>

using namespace fun;
using namespace fun::net;

namespace pubsub {

typedef std::set<String> ConnectionSubscription;

class Topic {
 public:
  Topic(const String& topic) : topic_(topic) {}

  void Add(const TcpConnectionPtr& conn) {
    audiences_.insert(conn);

    if (last_published_time_.IsValid()) {
      conn->Send(MakeMessage());
    }
  }

  void Remove(const TcpConnectionPtr& conn) { audiences_.erase(conn); }

  void Publish(const String& content, const Timestamp& time) {
    content_ = content;
    last_published_time_ = time;
    String message = MakeMessage();
    for (std::set<TcpConnectionPtr>::iterator it = audiences_.begin();
         it != audiences_.end(); ++it) {
      (*it)->Send(message);
    }
  }

 private:
  String MakeMessage() const {
    return "pub " + topic_ + "\r\n" + content_ + "\r\n";
  }

  String topic_;
  String content_;
  Timestamp last_published_time_;
  std::set<TcpConnectionPtr> audiences_;
};

class PubSubServer : Noncopyable {
 public:
  PubSubServer(EventLoop* loop, const InetAddress& listen_addr)
      : loop_(loop), server_(loop, listen_addr, "PubSubServer") {
    server_.SetConnectionCallback(
        boost::bind(&PubSubServer::OnConnection, this, _1));
    server_.SetMessageCallback(
        boost::bind(&PubSubServer::OnMessage, this, _1, _2, _3));

    loop_->ScheduleEvery(1.0, boost::bind(&PubSubServer::TimePublish, this));
  }

  void Start() { server_.Start(); }

 private:
  void OnConnection(const TcpConnectionPtr& conn) {
    if (conn->IsConnected()) {
      conn->SetContext(ConnectionSubscription());
    } else {
      const ConnectionSubscription& conn_sub =
          boost::any_cast<const ConnectionSubscription&>(conn->GetContext());
      // subtle: DoUnsubscribe will erase *it, so increase before calling.
      for (ConnectionSubscription::const_iterator it = conn_sub.begin();
           it != conn_sub.end();) {
        DoUnsubscribe(conn, *it++);
      }
    }
  }

  void OnMessage(const TcpConnectionPtr& conn, Buffer* buf,
                 const Timestamp& received_time) {
    ParseResult result = kSuccess;
    while (result == kSuccess) {
      String cmd, topic, content;
      result = ParseMessage(buf, &cmd, &topic, &content);
      if (result == kSuccess) {
        if (cmd == "pub") {
          DoPublish(conn->GetName(), topic, content, received_time);
        } else if (cmd == "sub") {
          LOG_INFO << conn->GetName() << " subscribes " << topic;
          DoSubscribe(conn, topic);
        } else if (cmd == "unsub") {
          DoUnsubscribe(conn, topic);
        } else {
          conn->Shutdown();
          result = kError;
        }
      } else if (result == kError) {
        conn->Shutdown();
      }
    }
  }

  void TimePublish() {
    Timestamp now = Timestamp::Now();
    DoPublish("internal", "utc_time", now.ToFormattedString(), now);
  }

  void DoSubscribe(const TcpConnectionPtr& conn, const String& topic) {
    ConnectionSubscription* conn_sub =
        boost::any_cast<ConnectionSubscription>(conn->GetMutableContext());

    conn_sub->insert(topic);
    GetTopic(topic).Add(conn);
  }

  void DoUnsubscribe(const TcpConnectionPtr& conn, const String& topic) {
    LOG_INFO << conn->GetName() << " unsubscribes " << topic;
    GetTopic(topic).remove(conn);
    // topic could be the one to be destroyed, so don't use it after erasing.
    ConnectionSubscription* conn_sub =
        boost::any_cast<ConnectionSubscription>(conn->GetMutableContext());
    conn_sub->erase(topic);
  }

  void DoPublish(const String& source, const String& topic,
                 const String& content, const Timestamp& time) {
    GetTopic(topic).Publish(content, time);
  }

  Topic& GetTopic(const String& topic) {
    std::map<String, Topic>::iterator it = topics_.find(topic);
    if (it == topics_.end()) {
      it = topics_.insert(make_pair(topic, Topic(topic))).first;
    }
    return it->second;
  }

  EventLoop* loop_;
  TcpServer server_;
  std::map<String, Topic> topics_;
};

}  // namespace pubsub

int main(int argc, char* argv[]) {
  if (argc > 1) {
    uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
    if (argc > 2) {
      // int inspectPort = atoi(argv[2]);
    }

    EventLoop loop;
    pubsub::PubSubServer server(&loop, InetAddress(port));
    server.Start();
    loop.Loop();
  } else {
    printf("Usage: %s pubsub_port [inspect_port]\n", argv[0]);
  }
}
