#pragma once

#include "fun/base/base.h"
#include "fun/base/key_value_args.h"
#include "fun/base/valid_args.h"
#include "fun/base/strategy_base.h"
#include "fun/base/debug.h"
#include "fun/base/timestamp.h"
#include "fun/base/timespan.h"
#include "fun/base/event_args.h"
#include "fun/base/unique_expire_strategy.h"

#include <set>
#include <map>

namespace fun {

/**
 * An UniqueExpireStrategy implements time based expiration of cache entries. In contrast
 * to ExpireStrategy which only allows to set a per cache expiration value, it allows to define
 * expiration per CacheEntry.
 * Each ValueType object must thus offer the following method:
 *
 *    const Timestamp& GetTimeout() const;
 *
 * which returns the timespan for how long an object will be valid without being accessed.
 */
template <
  typename KeyType,
  typename ValueType
>
class UniqueAccessExpireStrategy : public StrategyBase<KeyType, ValueType> {
 public:
  typedef std::pair<KeyType, Timespan> KeyExpire;
  typedef std::multimap<Timestamp, KeyExpire> TimeIndex;
  typedef typename TimeIndex::iterator IndexIterator;
  typedef typename TimeIndex::const_iterator ConstIndexIterator;
  typedef std::map<KeyType, IndexIterator> Keys;
  typedef typename Keys::iterator Iterator;

 public:
  /**
   * Create an unique expire strategy.
   */
  UniqueAccessExpireStrategy() {}
  ~UniqueAccessExpireStrategy() {}

  void OnAdd(const void*, const KeyValueArgs <KeyType, ValueType>& args) {
    // the expire value defines how many millisecs in the future the
    // value will expire, even insert negative values!
    Timestamp expire;
    expire += args.Value().GetTimeout().TotalMicroseconds();

    IndexIterator it = key_index_.insert(std::make_pair(expire, std::make_pair(args.Key(), args.Value().GetTimeout())));
    std::pair<Iterator, bool> stat = keys_.insert(std::make_pair(args.Key(), it));
    if (!stat.second) {
      key_index_.erase(stat.first->second);
      stat.first->second = it;
    }
  }

  void OnRemove(const void*, const KeyType& key) {
    Iterator it = keys_.find(key);
    if (it != keys_.end()) {
      key_index_.erase(it->second);
      keys_.erase(it);
    }
  }

  void OnGet(const void*, const KeyType& key) {
    // get updates the expiration time stamp
    Iterator it = keys_.find(key);
    if (it != keys_.end()) {
      KeyExpire ke = it->second->second;
      // gen new absolute expire value
      Timestamp expire;
      expire += ke.second.TotalMicroseconds();
      // delete old index
      key_index_.erase(it->second);
      IndexIterator itt = key_index_.insert(std::make_pair(expire, ke));
      // update iterator
      it->second = itt;
    }
  }

  void OnClear(const void*, const EventArgs& args) {
    keys_.clear();
    key_index_.clear();
  }

  void OnIsValid(const void*, ValidArgs<KeyType>& args) {
    Iterator it = keys_.find(args.Key());
    if (it != keys_.end()) {
      Timestamp now(ForceInit);
      if (it->second->first <= now) {
        args.Invalidate();
      }
    } else { //not found: probably removed by OnReplace
      args.Invalidate();
    }
  }

  void OnReplace(const void*, std::set<KeyType>& elements_to_remove) {
    // Note: replace only informs the cache which elements
    // it would like to remove!
    // it does not remove them on its own!
    IndexIterator it = key_index_.begin();
    Timestamp now(ForceInit);
    while (it != key_index_.end() && it->first < now) {
      elements_to_remove.insert(it->second.first);
      ++it;
    }
  }

 protected:
  /** For faster replacement of keys, the iterator points to the key_index_ map */
  Keys keys_;
  /** Maps time to key value */
  TimeIndex key_index_;
};

} // namespace fun
