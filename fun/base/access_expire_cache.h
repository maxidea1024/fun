#pragma once

#include "fun/base/access_expire_strategy.h"
#include "fun/base/base.h"
#include "fun/base/cache_base.h"

namespace fun {

/**
 * An AccessExpireCache caches entries for a fixed time period (per default 10
 * minutes). Entries expire when they are not accessed with Get() during this
 * time period. Each access resets the start time for expiration.
 *
 * Be careful when using an AccessExpireCache. A cache is often used
 * like cache.Has(x) followed by cache.Get x). Note that it could happen
 * that the "Has" call works, then the current execution thread gets
 * descheduled, time passes, the entry gets invalid, thus leading to an empty
 * SharedPtr being returned when "Get" is invoked.
 */
template <typename KeyType, typename ValueType, typename MutexType = FastMutex,
          typename EventMutexType = FastMutex>
class AccessExpireCache
    : public CacheBase<KeyType, ValueType,
                       AccessExpireStrategy<KeyType, ValueType>, MutexType,
                       EventMutexType> {
 public:
  AccessExpireCache(Timestamp::TimeDiff expire = 600000)
      : CacheBase<KeyType, ValueType, AccessExpireStrategy<KeyType, ValueType>,
                  MutexType, EventMutexType>(
            AccessExpireStrategy<KeyType, ValueType>(expire)) {}

  ~AccessExpireCache() {}

  AccessExpireCache(const AccessExpireCache&) = delete;
  AccessExpireCache& operator=(const AccessExpireCache&) = delete;
};

}  // namespace fun
