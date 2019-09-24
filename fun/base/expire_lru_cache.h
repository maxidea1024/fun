#pragma once

#include "fun/base/base.h"
#include "fun/base/cache_base.h"
#include "fun/base/strategy_collection.h"
#include "fun/base/expire_strategy.h"
#include "fun/base/lru_strategy.h"

namespace fun {

/**
 * An ExpireLruCache combines LRU caching and time based expire caching.
 * It cache entries for a fixed time period (per default 10 minutes)
 * but also limits the size of the cache (per default: 1024).
 */
template <
  typename KeyType,
  typename ValueType,
  typename MutexType = FastMutex,
  typename EventMutexType = FastMutex
>
class ExpireLruCache : public CacheBase<KeyType, ValueType, StrategyCollection<KeyType, ValueType>, MutexType, EventMutexType> {
 public:
  ExpireLruCache(:size_t cache_size = 1024, Timestamp::TimeDiff expire = 600000)
    : CacheBase<KeyType, ValueType, StrategyCollection<KeyType, ValueType>, MutexType, EventMutexType>(StrategyCollection<KeyType, ValueType>()) {
    this->strategy_.Add(new LruStrategy<KeyType, ValueType>(cache_size));
    this->strategy_.Add(new ExpireStrategy<KeyType, ValueType>(expire));
  }

  ~ExpireLruCache() {}

  ExpireLruCache(const ExpireLruCache&) = delete;
  ExpireLruCache& operator = (const ExpireLruCache&) = delete;
};

} // namespace fun
