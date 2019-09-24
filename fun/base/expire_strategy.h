#pragma once

#include "fun/base/base.h"
#include "fun/base/debug.h"
#include "fun/base/event_args.h"
#include "fun/base/exception.h"
#include "fun/base/key_value_args.h"
#include "fun/base/strategy_base.h"
#include "fun/base/timestamp.h"

namespace fun {

/**
 * An ExpireStrategy implements time based expiration of cache entries
 */
template <typename KeyType, typename ValueType>
class ExpireStrategy : public StrategyBase<KeyType, ValueType> {
 public:
  /**
   * Create an expire strategy. Note that the smallest allowed caching time is
   * 25ms. Anything lower than that is not useful with current operating
   * systems.
   */
  ExpireStrategy(Timestamp::TimeDiff expire_time)
      : expire_time_(expire_time * 1000) {
    if (expire_time_ < 25000) {
      throw InvalidArgumentException("expire_time must be at least 25 ms");
    }
  }

  ~ExpireStrategy() {}

  void OnAdd(const void*,
             const KeyValueArgs<KeyType, ValueType>& args) override {
    Timestamp now;
    typename TimeIndex::value_type tiValue(now, args.key());
    IndexIterator it = key_index_.insert(tiValue);
    typename Keys::value_type kValue(args.key(), it);
    std::pair<Iterator, bool> stat = keys_.insert(kValue);
    if (!stat.second) {
      key_index_.erase(stat.first->second);
      stat.first->second = it;
    }
  }

  void OnRemove(const void*, const KeyType& key) override {
    Iterator it = keys_.find(key);
    if (it != keys_.end()) {
      key_index_.erase(it->second);
      keys_.erase(it);
    }
  }

  void OnGet(const void*, const KeyType& key) override {
    // get triggers no changes in an expire
  }

  void OnClear(const void*, const EventArgs& args) override {
    keys_.Clear();
    key_index_.Clear();
  }

  void OnIsValid(const void*, ValidArgs<KeyType>& args) override {
    Iterator it = keys_.find(args.key());
    if (it != keys_.end()) {
      if (it->second->first.IsElapsed(expire_time_)) {
        args.invalidate();
      }
    } else {  // not found: probably removed by OnReplace
      args.invalidate();
    }
  }

  void OnReplace(const void*, std::set<KeyType>& elemsToRemove) override {
    // Note: replace only informs the cache which elements
    // it would like to remove!
    // it does not remove them on its own!
    IndexIterator it = key_index_.begin();
    while (it != key_index_.end() && it->first.IsElapsed(expire_time_)) {
      elemsToRemove.insert(it->second);
      ++it;
    }
  }

 protected:
  Timestamp::TimeDiff expire_time_;

  /** For faster replacement of keys, the iterator points to the key_index_ map
   */
  Keys keys_;

  /** Maps time to key value */
  TimeIndex key_index_;
};

}  // namespace fun
