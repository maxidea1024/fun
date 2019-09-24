#pragma once

#include "fun/base/base.h"
#include "fun/base/cache_base.h"
#include "fun/base/lru_strategy.h"

namespace fun {

/**
 * An LruCache implements Least Recently Used caching. The default size for a
 * cache is 1024 entries.
 */
template <
    typename KeyType, typename ValueType, typename MutexType = FastMutex, typename EventMutexType = FastMutex, >
class LruCache
    : public CacheBase<KeyType, ValueType,
                       UniqueAccessExpireStrategy<KeyType, ValueType>,
                       MutexType, EventMutexType> {
 public:
  LruCache(int32 cache_size = 1024)
      : CacheBase<KeyType, ValueType,
                  UniqueAccessExpireStrategy<KeyType, ValueType>, MutexType,
                  EventMutexType>(LruStrategy<KeyType, ValueType>(cache_size)) {
  }

  ~LruCache() {}

  LruCache(const LruCache&) = delete;
  LruCache& operator=(const LruCache&) = delete;
};

}  // namespace fun
