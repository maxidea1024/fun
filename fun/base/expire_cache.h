#pragma once

#include "fun/base/base.h"
#include "fun/base/cache_base.h"
#include "fun/base/expire_strategy.h"

namespace fun {

/**
 * An ExpireCache caches entries for a fixed time period (per default 10
 * minutes). Entries expire independently of the access pattern, i.e. after a
 * constant time. If you require your objects to expire after they were not
 * accessed for a given time period use a fun::AccessExpireCache.
 *
 * Be careful when using an ExpireCache. A cache is often used
 * like cache.Has(x) followed by cache.Get x). Note that it could happen
 * that the "Has" call works, then the current execution thread gets
 * descheduled, time passes, the entry gets invalid, thus leading to an empty
 * SharedPtr being returned when "Get" is invoked.
 */
template <typename KeyType, typename ValueType, typename MutexType = FastMutex,
          typename EventMutexType = FastMutex>
class ExpireCache
    : public CacheBase<KeyType, ValueType, ExpireStrategy<KeyType, ValueType>,
                       MutexType, EventMutexType> {
 public:
  ExpireCache(Timestamp::TimeDiff expire = 600000)
      : CacheBase<KeyType, ValueType, ExpireStrategy<KeyType, ValueType>,
                  MutexType, EventMutexType>(
            ExpireStrategy<KeyType, ValueType>(expire)) {}

  ~ExpireCache() {}

  ExpireCache(const ExpireCache&) = delete;
  ExpireCache& operator=(const ExpireCache&) = delete;
};

}  // namespace fun
