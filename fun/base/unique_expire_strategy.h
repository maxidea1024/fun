#pragma once

#include "fun/base/base.h"
#include "fun/base/debug.h"
#include "fun/base/event_args.h"
#include "fun/base/key_value_args.h"
#include "fun/base/strategy_base.h"
#include "fun/base/timestamp.h"

namespace fun {

/**
 * An UniqueExpireStrategy implements time based expiration of cache entries. In
 * contrast to ExpireStrategy which only allows to set a per cache expiration
 * value, it allows to define expiration per CacheEntry. Each ValueType object
 * must thus offer the following method:
 *
 *    const fun::Timestamp& GetExpiration() const;
 *
 * which returns the absolute timepoint when the entry will be invalidated.
 */
template <typename KeyType, typename ValueType>
class UniqueExpireStrategy : public StrategyBase<KeyType, ValueType> {
 public:
  typedef std::multimap<Timestamp, KeyType> TimeIndex;
  typedef typename TimeIndex::iterator IndexIterator;
  typedef typename TimeIndex::const_iterator ConstIndexIterator;
  typedef std::map<KeyType, IndexIterator> Keys;
  typedef typename Keys::iterator Iterator;

  UniqueExpireStrategy() {}
  ~UniqueExpireStrategy() {}

  void OnAdd(const void* sender,
             const KeyValueArgs<KeyType, ValueType>& args) override {
    const Timestamp& expire = args.Value().GetExpiration();
    IndexIterator it = key_index_.insert(std::make_pair(expire, args.Key()));
    std::pair<Iterator, bool> stat =
        keys_.insert(std::make_pair(args.Key(), it));
    if (!stat.second) {
      key_index_.erase(stat.first->second);
      stat.first->second = it;
    }
  }

  void OnRemove(const void* sender, const KeyType& key) override {
    Iterator it = keys_.find(key);
    if (it != keys_.end()) {
      key_index_.erase(it->second);
      keys_.erase(it);
    }
  }

  void OnGet(const void* sender, const KeyType& key) override {
    // get triggers no changes in an expire
  }

  void OnClear(const void* sender, const EventArgs& args) override {
    keys_.Clear();
    key_index_.Clear();
  }

  void OnIsValid(const void* sender, ValidArgs<KeyType>& key) override {
    Iterator it = keys_.find(args.Key());
    if (it != keys_.end()) {
      Timestamp now;
      if (it->second->first <= now) {
        args.Invalidate();
      }
    } else {  // not found: probably removed by OnReplace
      args.Invalidate();
    }
  }

  void OnReplace(const void* sender, Set<KeyType>& out_elements_to_remove) {
    // Note: replace only informs the cache which elements
    // it would like to remove!
    // it does not remove them on its own!
    IndexIterator it = key_index_.begin();
    Timestamp now;
    while (it != key_index_.end() && it->first < now) {
      elemsToRemove.insert(it->second);
      ++it;
    }
  }

 protected:
  /** For faster replacement of keys, the iterator points to the key_index_ map
   */
  Keys keys_;
  /** Maps time to key value */
  TimeIndex key_index_;
};

}  // namespace fun
