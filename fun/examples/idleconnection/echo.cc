#include "echo.h"

#include "fun/base/logging.h"
#include "fun/net/event_loop.h"

#include <boost/bind.hpp>

#include <fun_check.h>
#include <stdio.h>

using namespace fun;
using namespace fun::net;

EchoServer::EchoServer(EventLoop* loop,
                       const InetAddress& listen_addr,
                       int idle_seconds)
  : server_(loop, listen_addr, "EchoServer")
  , connection_buckets_(idle_seconds) {
  server_.SetConnectionCallback(boost::bind(&EchoServer::OnConnection, this, _1));
  server_.SetMessageCallback(boost::bind(&EchoServer::OnMessage, this, _1, _2, _3));
  loop->ScheduleEvery(1.0, boost::bind(&EchoServer::OnTimer, this));
  connection_buckets_.resize(idle_seconds);
  DumpConnectionBuckets();
}

void EchoServer::Start() {
  server_.Start();
}

void EchoServer::OnConnection(const TcpConnectionPtr& conn) {
  LOG_INFO << "EchoServer - " << conn->GetPeerAddress().ToIpPort() << " -> "
           << conn->GetLocalAddress().ToIpPort() << " is "
           << (conn->IsConnected() ? "UP" : "DOWN");

  if (conn->IsConnected()) {
    EntryPtr entry(new Entry(conn));
    connection_buckets_.back().insert(entry);
    DumpConnectionBuckets();
    WeakEntryPtr weak_entry(entry);
    conn->SetContext(weak_entry);
  }
  else {
    fun_check(!conn->GetContext().empty());
    WeakEntryPtr weak_entry(boost::any_cast<WeakEntryPtr>(conn->GetContext()));
    LOG_DEBUG << "Entry use_count = " << weak_entry.use_count();
  }
}

void EchoServer::OnMessage(const TcpConnectionPtr& conn,
                           Buffer* buf,
                           const Timestamp& time) {
  String msg(buf->ReadAllAsString());
  LOG_INFO << conn->GetName() << " echo " << msg.size()
           << " bytes at " << time.ToString();
  conn->Send(msg);

  fun_check(!conn->GetContext().empty());
  WeakEntryPtr weak_entry(boost::any_cast<WeakEntryPtr>(conn->GetContext()));
  EntryPtr entry(weak_entry.lock());
  if (entry) {
    connection_buckets_.back().insert(entry);
    DumpConnectionBuckets();
  }
}

void EchoServer::OnTimer() {
  connection_buckets_.push_back(Bucket());
  DumpConnectionBuckets();
}

void EchoServer::DumpConnectionBuckets() const {
  LOG_INFO << "size = " << connection_buckets_.size();
  int idx = 0;
  for (WeakConnectionList::const_iterator bucketI = connection_buckets_.begin();
      bucketI != connection_buckets_.end();
      ++bucketI, ++idx) {
    const Bucket& bucket = *bucketI;
    printf("[%d] len = %zd : ", idx, bucket.size());
    for (Bucket::const_iterator it = bucket.begin();
        it != bucket.end();
        ++it) {
      bool connection_dead = (*it)->weak_conn_.expired();
      printf("%p(%ld)%s, ", get_pointer(*it), it->use_count(),
          connection_dead ? " DEAD" : "");
    }
    puts("");
  }
}
