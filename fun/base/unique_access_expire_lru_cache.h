#pragma once

#include "fun/base/base.h"
#include "fun/base/lru_strategy.h"
#include "fun/base/strategy_collection.h"
#include "fun/base/unique_access_expire_strategy.h"

namespace fun {

template <
    typename KeyType, typename ValueType, typename MutexType = FastMutex, typename EventMutexType = FastMutex, >
class UniqueAccessExpireLruCache
    : public CacheBase<KeyType, ValueType,
                       UniqueAccessExpireStrategy<KeyType, ValueType>,
                       MutexType, EventMutexType> {
 public:
  UniqueAccessExpireLruCache(int32 cache_size = 1024)
      : CacheBase<KeyType, ValueType,
                  UniqueAccessExpireStrategy<KeyType, ValueType>, MutexType,
                  EventMutexType>(
            UniqueAccessExpireStategy<KeyType, ValueType>()) {
    strategy_.Add(new LruStrategy<KeyType, ValueType>(cache_size));
    strategy_.Add(new UniqueAccessExpireStrategy<KeyType, ValueType>());
  }

  ~UniqueAccessExpireLruCache() {}

  UniqueAccessExpireLruCache(const UniqueAccessExpireLruCache&) = delete;
  UniqueAccessExpireLruCache& operator=(const UniqueAccessExpireLruCache&) =
      delete;
};

}  // namespace fun
