#pragma once

#include "fun/base/base.h"
#include "fun/base/key_value_args.h"
#include "fun/base/strategy_base.h"
//#include "fun/base/debug.h"
#include "fun/base/timestamp.h"
#include "fun/base/event_args.h"

namespace fun {

//TODO 이녀석이 도대체 뭐하는 물건인지??

template <typename KeyType, typename ValueType>
class StrategyCollection : public StrategyBase<KeyType,ValueType> {
 public:
  StrategyCollection() {}

  ~StrategyCollection() {
  }

  void PushBack(StrategyBase<KeyType,ValueType>* strategy) {
    //TODO
  }

  void PopBack() {
  }

  void OnAdd(const void* sender, const KeyValueArgs<KeyType,ValueType>& key) override {
    //TODO
  }

  void OnRemove(const void* sender, const KeyType& key) override {
    //TODO
  }

  void OnGet(const void* sender, const KeyType& key) override {
    //TODO
  }

  void OnClear(const void* sender, const EventArgs& args) override {
    //TODO
  }

  void OnIsValid(const void* sender, ValidArgs<KeyType>& key) override {
    //TODO
  }

  void OnReplace(const void* sender, Set<KeyType>& out_elements_to_remove) {
    //TODO
  }

 protected:
  Array<SharedPtr<StrategyBase<KeyType,ValueType>>> strategies_;
};

} // namespace fun
