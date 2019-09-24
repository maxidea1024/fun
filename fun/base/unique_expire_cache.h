#pragma once

#include "fun/base/base.h"
#include "fun/base/cache_base.h"
#include "fun/base/unique_expire_strategy.h"

namespace fun {

/**
 * An UniqueExpireCache caches entries for a given time amount. In contrast
 * to ExpireCache which only allows to set a per cache expiration value, it allows to define
 * expiration per CacheEntry.
 * Each TValue object must thus offer the following method:
 *
 *    const Timestamp& GetExpiration() const;
 *
 * which returns the absolute timepoint when the entry will be invalidated.
 * Accessing an object will NOT update this absolute expire timepoint.
 * You can use the ExpirationDecorator to add the GetExpiration
 * method to values that do not have a GetExpiration function.
 *
 * Be careful when using an UniqueExpireCache. A cache is often used
 * like cache.Has(x) followed by cache.Get x). Note that it could happen
 * that the "Has" call works, then the current execution thread gets descheduled, time passes,
 * the entry gets invalid, thus leading to an empty SharedPtr being returned
 * when "Get" is invoked.
 */
template <
  typename KeyType,
  typename ValueType,
  typename MutexType = FastMutex,
  typename EventMutexType = FastMutex,
  >
class UniqueAccessExpireCache : public CacheBase<KeyType, ValueType, UniqueExpireStrategy<KeyType, ValueType>, MutexType, EventMutexType> {
 public:
  UniqueAccessExpireCache()
    : CacheBase<KeyType, ValueType, UniqueExpireStrategy<KeyType, ValueType>, MutexType, EventMutexType>(UniqueAccessExpireStategy<KeyType, ValueType>()) {}

  ~UniqueAccessExpireCache() {}

  UniqueAccessExpireCache(const UniqueAccessExpireCache&) = delete;
  UniqueAccessExpireCache& operator = (const UniqueAccessExpireCache&) = delete;
};

} // namespace fun
