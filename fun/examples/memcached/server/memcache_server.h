#pragma once

#include "item.h"
#include "session.h"

#include "fun/base/mutex.h"
#include "fun/net/tcp_server.h"

#include <examples/wordcount/hash.h>

#include <boost/array.hpp>
#include <boost/noncopyable.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>

class MemcacheServer : Noncopyable {
 public:
  struct Options {
    Options();
    uint16_t tcpport;
    uint16_t udpport;
    uint16_t gperfport;
    int thread_count;
  };

  MemcacheServer(fun::net::EventLoop* loop, const Options&);
  ~MemcacheServer();

  void SetThreadCount(int thread_count) {
    server_.SetThreadCount(thread_count);
  }
  void Start();
  void stop();

  time_t startTime() const { return start_time_; }

  bool StoreItem(const ItemPtr& item, Item::UpdatePolicy policy, bool* exists);
  ConstItemPtr GetItem(const ConstItemPtr& key) const;
  bool DeleteItem(const ConstItemPtr& key);

 private:
  void OnConnection(const fun::net::TcpConnectionPtr& conn);

  struct Stats;

  fun::net::EventLoop* loop_;  // not own
  Options options_;
  const time_t start_time_;

  mutable fun::MutexLock mutex_;
  boost::unordered_map<String, SessionPtr> sessions_;

  // a complicated solution to save memory
  struct Hash {
    size_t operator()(const ConstItemPtr& x) const { return x->hash(); }
  };

  struct Equal {
    bool operator()(const ConstItemPtr& x, const ConstItemPtr& y) const {
      return x->key() == y->key();
    }
  };

  typedef boost::unordered_set<ConstItemPtr, Hash, Equal> ItemMap;

  struct MapWithLock {
    ItemMap items;
    mutable fun::MutexLock mutex;
  };

  const static int kShards = 4096;

  boost::array<MapWithLock, kShards> shards_;

  // NOT guarded by mutex_, but here because server_ has to destructs before
  // sessions_
  fun::net::TcpServer server_;
  fun::SharedPtr<Stats> stats_;
};
