#pragma once

#include "fun/base/base.h"
#include "fun/base/cache_base.h"
#include "fun/base/unique_access_expire_strategy.h"
#include "fun/base/mutex.h"

namespace fun {

/**
 * An UniqueAccessExpireCache caches entries for a given time span. In contrast
 * to ExpireCache which only allows to set a per cache expiration value, it allows to define
 * expiration per CacheEntry.
 * Each ValueType object must thus offer the following method:
 *
 *    const Timespan& GetTimeout() const;
 *
 * which returns the relative timespan for how long the entry should be valid without being accessed!
 * The absolute expire timepoint is calculated as Now() + GetTimeout().
 * Accessing an object will update this absolute expire timepoint.
 * You can use the AccessExpirationDecorator to add the GetExpiration
 * method to values that do not have a GetExpiration function.
 *
 * Be careful when using an UniqueAccessExpireCache. A cache is often used
 * like cache.Has(x) followed by cache.Get x). Note that it could happen
 * that the "has" call works, then the current execution thread gets descheduled, time passes,
 * the entry gets invalid, thus leading to an empty SharedPtr being returned
 * when "Get" is invoked.
 */
template <
  typename KeyType,
  typename ValueType,
  typename MutexType = FastMutex,
  typename EventMutexType = FastMutex
>
class UniqueAccessExpireCache : public CacheBase<KeyType, ValueType, UniqueAccessExpireStrategy<KeyType, ValueType>, MutexType, EventMutexType> {
 public:
  UniqueAccessExpireCache()
    : CacheBase<KeyType, ValueType, UniqueAccessExpireStrategy<KeyType, ValueType>, MutexType, EventMutexType>(UniqueAccessExpireStrategy<KeyType, ValueType>()) {}

  ~UniqueAccessExpireCache() {}

  UniqueAccessExpireCache(const UniqueAccessExpireCache&) = delete;
  UniqueAccessExpireCache& operator = (const UniqueAccessExpireCache&) = delete;
};

} // namespace fun
