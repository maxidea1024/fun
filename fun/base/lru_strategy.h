#pragma once

#include "fun/base/base.h"
#include "fun/base/key_value_args.h"
#include "fun/base/strategy_base.h"
#include "fun/base/debug.h"
#include "fun/base/timestamp.h"
#include "fun/base/event_args.h"

#include <list>
#include <map>
#include <cstddef>

namespace fun {

/**
 * An LruStrategy implements least recently used cache replacement.
 */
template <
    typename KeyType,
    typename ValueType
  >
class LruStrategy : public StrategyBase<KeyType, ValueType> {
 public:
  LruStrategy(size_t cache_size)
    : size_(cache_size) {
    if (size_ < 1) {
      throw InvalidArgumentException("size must be > 0");
    }
  }

  ~LruStrategy() {}

  void OnAdd(const void* sender, const KeyValueArgs<KeyType, ValueType>& key) override {
    keys_.push_front(args.Key());
    std::pair<IndexIterator, bool> stat = key_index_.insert(std::make_pair(args.Key(), keys_.begin()));
    if (!stat.second) {
      stat.first->second = keys_.begin();
    }
  }

  void OnRemove(const void* sender, const KeyType& key) override {
    IndexIterator it = key_index_.find(key);
    if (it != key_index_.end()) {
      keys_.erase(it->second);
      key_index_.erase(it);
    }
  }

  void OnGet(const void* sender, const KeyType& key) override {
    // LRU: in case of an hit, move to begin
    IndexIterator it = key_index_.find(key);
    if (it != key_index_.end()) {
      keys_.splice(keys_.begin(), keys_, it->second); //keys_.erase(it->second)+keys_.push_front(key);
      it->second = keys_.begin();
    }
  }

  void OnClear(const void* sender, const EventArgs& args) override {
    keys_.clear();
    key_index_.clear();
  }

  void OnIsValid(const void* sender, ValidArgs<KeyType>& key) override {
    if (key_index_.find(args.Key()) == key_index_.end()) {
      args.invalidate();
    }
  }

  void OnReplace(const void* sender, Set<KeyType>& out_elements_to_remove) {
    // Note: replace only informs the cache which elements
    // it would like to remove!
    // it does not remove them on its own!
    std::size_t cur_size = key_index_.size();

    if (cur_size < _size) {
      return;
    }

    std::size_t diff = cur_size - _size;
    Iterator it = --keys_.end(); //--keys can never be invoked on an empty list due to the min_size==1 requirement of LRU
    std::size_t i = 0;

    while (i++ < diff) {
      elemsToRemove.insert(*it);
      if (it != keys_.begin()) {
        --it;
      }
    }
  }

 protected:
  size_t size_;
  Keys keys_;
  TimeIndex key_index_;
};

} // namespace fun
