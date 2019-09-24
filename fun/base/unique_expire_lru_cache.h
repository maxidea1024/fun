#pragma once

#include "fun/base/base.h"
#include "fun/base/strategy_collection.h"
#include "fun/base/unique_expire_strategy.h"
#include "fun/base/lru_strategy.h"

namespace fun {

/**
 * A UniqueExpireLRUCache combines LRU caching and time based per entry expire caching.
 * One can define for each cache entry a separate timepoint
 * but also limit the size of the cache (per default: 1024).
 * Each TValue object must thus offer the following method:
 *
 *    const Timestamp& GetExpiration() const;
 *
 * which returns the absolute timepoint when the entry will be invalidated.
 * Accessing an object will NOT update this absolute expire timepoint.
 * You can use the ExpirationDecorator to add the GetExpiration
 * method to values that do not have a GetExpiration function.
 */
template <
  typename KeyType,
  typename ValueType,
  typename MutexType = FastMutex,
  typename EventMutexType = FastMutex,
  >
class UniqueExpireLruCache : public CacheBase<KeyType, ValueType, StrategyCollection<KeyType, ValueType>, MutexType, EventMutexType> {
 public:
  UniqueExpireLruCache(size_t cache_size)
    : CacheBase<KeyType, ValueType, UniqueExpireStrategy<KeyType, ValueType>, MutexType, EventMutexType>(StrategyCollection<KeyType, ValueType>()) {
    this->strategy_.Add(new LryStrategy<KeyType, ValueType>(cache_size));
    this->strategy_.Add(new UniqueExpireStrategy<KeyType, ValueType>());
  }

  ~UniqueExpireLruCache() {}

  UniqueExpireLruCache(const UniqueExpireLruCache&) = delete;
  UniqueExpireLruCache& operator = (const UniqueExpireLruCache&) = delete;
};

} // namespace fun
