#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * An AbstractStrategy is the interface for all strategies.
 */
template <typename KeyType, typename ValueType>
class StrategyBase {
 public:
  StrategyBase() = default;
  virtual ~StrategyBase() = default;

  virtual void OnUpdate(const void* sender,
                        const Pair<KeyType, ValueType>& args) {
    OnRemove(sender, args.Key());
    OnAdd(sender, args);
  }

  virtual void OnAdd(const void* sender,
                     const Pair<KeyType, ValueType>& key) = 0;
  virtual void OnRemove(const void* sender, const KeyType& key) = 0;
  virtual void OnGet(const void* sender, const KeyType& key) = 0;
  virtual void OnClear(const void* sender,
                       const Pair<KeyType, ValueType>& args) = 0;
  virtual void OnIsValid(const void* sender, const ValidArgs<KeyType>& key) = 0;
  virtual void OnReplace(const void* sender,
                         Set<KeyType>& out_elements_to_remove) = 0;
};

}  // namespace fun
