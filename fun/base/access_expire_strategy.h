#pragma once

#include "fun/base/base.h"
#include "fun/base/key_value_args.h"
#include "fun/base/expire_strategy.h"
#include "fun/base/debug.h"
#include "fun/base/timestamp.h"
#include "fun/base/event_args.h"

#include <set>
#include <map>

namespace fun {

/**
 * An AbstractStrategy is the interface for all strategies.
 */
template <typename KeyType, typename ValueType>
class AccessExpireStrategy : public ExpireStrategy<KeyType, ValueType> {
 public:
  //TODO Timestamp 단위가 모호하다. milliseconds 단위로 인식하는데 반해서
  //자료형은 microseconds 단위임.

  /**
   * Create an expire strategy. Note that the smallest allowed caching time is 25ms.
   * Anything lower than that is not useful with current operating systems.
   */
  AccessExpireStrategy(Timestamp::TimeDiff timeout)
    : ExpireStrategy<KeyType,ValueType>(timeout) {}

  ~AccessExpireStrategy() {}

  void OnGet(const void*, const KeyType& key) override {
    // get triggers an update to the expiration time
    typename ExpireStrategy<KeyType, ValueType>::Iterator it = this->keys_.find(key);
    if (it != this->keys_.end()) {
      this->key_index_.erase(it->second);
      Timestamp now(ForceInit);
      typename ExpireStrategy<KeyType, ValueType>::IndexIterator it_index =
        this->key_index_.insert(typename ExpireStrategy<KeyType, ValueType>::TimeIndex::value_type(now, key));
      it->second = it_index;
    }
  }
};

} // namespace fun
