#pragma once

#include "fun/net/tcp_server.h"
//#include "fun/base/types.h"

#include <boost/circular_buffer.hpp>
#include <boost/unordered_set.hpp>
#include <boost/version.hpp>

#if BOOST_VERSION < 104700
namespace boost {
template <typename T>
inline size_t hash_value(const fun::SharedPtr<T>& x) {
  return boost::hash_value(x.get());
}
}  // namespace boost
#endif

// RFC 862
class EchoServer {
 public:
  EchoServer(fun::net::EventLoop* loop,
             const fun::net::InetAddress& listen_addr, int idle_seconds);

  void Start();

 private:
  void OnConnection(const fun::net::TcpConnectionPtr& conn);

  void OnMessage(const fun::net::TcpConnectionPtr& conn, fun::net::Buffer* buf,
                 const fun::Timestamp& time);

  void OnTimer();

  void DumpConnectionBuckets() const;

  typedef WeakPtr<fun::net::TcpConnection> WeakTcpConnectionPtr;

  struct Entry : public fun::copyable {
    explicit Entry(const WeakTcpConnectionPtr& weak_conn)
        : weak_conn_(weak_conn) {}

    ~Entry() {
      fun::net::TcpConnectionPtr conn = weak_conn_.lock();
      if (conn) {
        conn->Shutdown();
      }
    }

    WeakTcpConnectionPtr weak_conn_;
  };
  typedef fun::SharedPtr<Entry> EntryPtr;
  typedef fun::WeakPtr<Entry> WeakEntryPtr;
  typedef boost::unordered_set<EntryPtr> Bucket;
  typedef boost::circular_buffer<Bucket> WeakConnectionList;

  fun::net::TcpServer server_;
  WeakConnectionList connection_buckets_;
};
