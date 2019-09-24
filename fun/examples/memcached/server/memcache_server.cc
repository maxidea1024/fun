#include "MemcacheServer.h"

#include <red/base/Atomic.h>
#include "fun/base/logging.h"
#include "fun/net/event_loop.h"

#include <boost/bind.hpp>

using namespace fun;
using namespace fun::net;

fun::AtomicInt64 g_cas;

MemcacheServer::Options::Options() { bzero(this, sizeof(*this)); }

struct MemcacheServer::Stats {};

MemcacheServer::MemcacheServer(fun::net::EventLoop* loop,
                               const Options& options)
    : loop_(loop),
      options_(options),
      start_time_(::time(NULL) - 1),
      server_(loop, InetAddress(options.tcpport), "red-memcached"),
      stats_(new Stats) {
  server_.SetConnectionCallback(
      boost::bind(&MemcacheServer::OnConnection, this, _1));
}

MemcacheServer::~MemcacheServer() {}

void MemcacheServer::Start() { server_.Start(); }

void MemcacheServer::stop() {
  loop_->ScheduleAfter(3.0, boost::bind(&EventLoop::Quit, loop_));
}

bool MemcacheServer::StoreItem(const ItemPtr& item,
                               const Item::UpdatePolicy policy, bool* exists) {
  fun_check(item->neededBytes() == 0);
  MutexLock& mutex = shards_[item->hash() % kShards].mutex;
  ItemMap& items = shards_[item->hash() % kShards].items;
  ScopedLock lock(mutex);
  ItemMap::const_iterator it = items.find(item);
  *exists = it != items.end();
  if (policy == Item::kSet) {
    item->setCas(g_cas.IncrementAndGet());
    if (*exists) {
      items.erase(it);
    }
    items.insert(item);
  } else {
    if (policy == Item::kAdd) {
      if (*exists) {
        return false;
      } else {
        item->setCas(g_cas.IncrementAndGet());
        items.insert(item);
      }
    } else if (policy == Item::kReplace) {
      if (*exists) {
        item->setCas(g_cas.IncrementAndGet());
        items.erase(it);
        items.insert(item);
      } else {
        return false;
      }
    } else if (policy == Item::kAppend || policy == Item::kPrepend) {
      if (*exists) {
        const ConstItemPtr& oldItem = *it;
        int newLen =
            static_cast<int>(item->valueLength() + oldItem->valueLength() - 2);
        ItemPtr newItem(Item::makeItem(item->key(), oldItem->flags(),
                                       oldItem->rel_exptime(), newLen,
                                       g_cas.IncrementAndGet()));
        if (policy == Item::kAppend) {
          newItem->append(oldItem->value(), oldItem->valueLength() - 2);
          newItem->append(item->value(), item->valueLength());
        } else {
          newItem->append(item->value(), item->valueLength() - 2);
          newItem->append(oldItem->value(), oldItem->valueLength());
        }
        fun_check(newItem->neededBytes() == 0);
        fun_check(newItem->EndsWithCRLF());
        items.erase(it);
        items.insert(newItem);
      } else {
        return false;
      }
    } else if (policy == Item::kCas) {
      if (*exists && (*it)->cas() == item->cas()) {
        item->setCas(g_cas.IncrementAndGet());
        items.erase(it);
        items.insert(item);
      } else {
        return false;
      }
    } else {
      fun_check(false);
    }
  }
  return true;
}

ConstItemPtr MemcacheServer::GetItem(const ConstItemPtr& key) const {
  MutexLock& mutex = shards_[key->hash() % kShards].mutex;
  const ItemMap& items = shards_[key->hash() % kShards].items;
  ScopedLock lock(mutex);
  ItemMap::const_iterator it = items.find(key);
  return it != items.end() ? *it : ConstItemPtr();
}

bool MemcacheServer::DeleteItem(const ConstItemPtr& key) {
  MutexLock& mutex = shards_[key->hash() % kShards].mutex;
  ItemMap& items = shards_[key->hash() % kShards].items;
  ScopedLock lock(mutex);
  return items.erase(key) == 1;
}

void MemcacheServer::OnConnection(const TcpConnectionPtr& conn) {
  if (conn->IsConnected()) {
    SessionPtr session(new Session(this, conn));
    ScopedLock guard(mutex_);
    fun_check(sessions_.find(conn->GetName()) == sessions_.end());
    sessions_[conn->GetName()] = session;
    // fun_check(sessions_.size() == stats_.current_conns);
  } else {
    ScopedLock guard(mutex_);
    fun_check(sessions_.find(conn->GetName()) != sessions_.end());
    sessions_.erase(conn->GetName());
    // fun_check(sessions_.size() == stats_.current_conns);
  }
}
