#pragma once

#include "fun/base/access_expire_strategy.h"
#include "fun/base/base.h"
#include "fun/base/cache_base.h"
#include "fun/base/lru_strategy.h"
#include "fun/base/strategy_collection.h"

namespace fun {

/**
 * An AccessExpireLruCache combines LRU caching and time based expire caching.
 * It cache entries for a fixed time period (per default 10 minutes)
 * but also limits the size of the cache (per default: 1024).
 */
template <typename KeyType, typename ValueType, typename MutexType = FastMutex,
          typename EventMutexType = FastMutex>
class AccessExpireLruCache
    : public CacheBase<KeyType, ValueType,
                       StrategyCollection<KeyType, ValueType>, MutexType,
                       EventMutexType> {
 public:
  AccessExpireLruCache(size_t cache_size = 1024,
                       Timestamp::TimeDiff expire = 600000)
      : CacheBase<KeyType, ValueType, StrategyCollection<KeyType, ValueType>,
                  MutexType, EventMutexType>(
            StrategyCollection<KeyType, ValueType>()) {
    this->strategy_.Add(new LruStrategy<KeyType, ValueType>(cache_size));
    this->strategy_.Add(new AccessExpireStrategy<KeyType, ValueType>(expire));
  }

  ~AccessExpireLruCache() {}

  AccessExpireLruCache(const AccessExpireLruCache&) = delete;
  AccessExpireLruCache& operator=(const AccessExpireLruCache&) = delete;
};

}  // namespace fun
