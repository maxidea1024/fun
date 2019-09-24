#pragma once

#include "fun/KeyValueArgs.h"
#include "fun/ValidArgs.h"
#include "fun/Mutex.h"
#include "fun/Exception.h"
#include "fun/FIFOEvent.h"
#include "fun/EventArgs.h"
#include "fun/Delegate.h"
#include "fun/SharedPtr.h"
#include <map>
#include <set>
#include <cstddef>

namespace fun {

/**
 * An CacheBase is the interface of all caches.
 */
template <class TKey, class TValue, class TStrategy, class TMutex = FastMutex, class TEventMutex = FastMutex>
class CacheBase {
 public:
  FIFOEvent<const KeyValueArgs<TKey, TValue >, TEventMutex > Add;
  FIFOEvent<const KeyValueArgs<TKey, TValue >, TEventMutex > Update;
  FIFOEvent<const TKey, TEventMutex> Remove;
  FIFOEvent<const TKey, TEventMutex> Get;
  FIFOEvent<const EventArgs, TEventMutex> Clear;

  typedef std::map<TKey, SharedPtr<TValue > > DataHolder;
  typedef typename DataHolder::iterator Iterator;
  typedef typename DataHolder::const_iterator ConstIterator;
  typedef std::set<TKey> KeySet;

  CacheBase() {
    Initialize();
  }

  CacheBase(const TStrategy& strat): strategy_(strat) {
    Initialize();
  }

  virtual ~CacheBase() {
    try {
      Uninitialize();
    } catch (...) {
      fun_unexpected();
    }
  }

  /**
   * Adds the key value pair to the cache.
   * If for the key already an entry exists, it will be overwritten.
   */
  void Add(const TKey& key, const TValue& val) {
    typename TMutex::ScopedLock lock(mutex_);
    DoAdd(key, val);
  }

  /**
   * Adds the key value pair to the cache. Note that adding a NULL SharedPtr will fail!
   * If for the key already an entry exists, it will be overwritten.
   * The difference to add is that no remove or add events are thrown in this case,
   * just a simply silent update is performed
   * If the key does not exist the behavior is equal to add, ie. an add event is thrown
   */
  void Update(const TKey& key, const TValue& val) {
    typename TMutex::ScopedLock lock(mutex_);
    DoUpdate(key, val);
  }

  /**
   * Adds the key value pair to the cache. Note that adding a NULL SharedPtr will fail!
   * If for the key already an entry exists, it will be overwritten, ie. first a remove event
   * is thrown, then a add event
   */
  void Add(const TKey& key, SharedPtr<TValue > val) {
    typename TMutex::ScopedLock lock(mutex_);
    DoAdd(key, val);
  }

  /**
   * Adds the key value pair to the cache. Note that adding a NULL SharedPtr will fail!
   * If for the key already an entry exists, it will be overwritten.
   * The difference to add is that no remove or add events are thrown in this case,
   * just an Update is thrown
   * If the key does not exist the behavior is equal to add, ie. an add event is thrown
   */
  void Update(const TKey& key, SharedPtr<TValue > val) {
    typename TMutex::ScopedLock lock(mutex_);
    DoUpdate(key, val);
  }

  /**
   * Removes an entry from the cache. If the entry is not found,
   * the remove is ignored.
   */
  void Remove(const TKey& key) {
    typename TMutex::ScopedLock lock(mutex_);
    Iterator it = data_.find(key);
    DoRemove(it);
  }

  /**
   * Returns true if the cache contains a value for the key.
   */
  bool Has(const TKey& key) const {
    typename TMutex::ScopedLock lock(mutex_);
    return DoHas(key);
  }

  /**
   * Returns a SharedPtr of the value. The SharedPointer will remain valid
   * even when cache replacement removes the element.
   * If for the key no value exists, an empty SharedPtr is returned.
   */
  SharedPtr<TValue> Get(const TKey& key) {
    typename TMutex::ScopedLock lock(mutex_);
    return DoGet(key);
  }

  /**
   * Removes all elements from the cache.
   */
  void Clear() {
    typename TMutex::ScopedLock lock(mutex_);
    DoClear();
  }

  /**
   * Returns the number of cached elements
   */
  std::size_t Count() {
    typename TMutex::ScopedLock lock(mutex_);
    DoReplace();
    return data_.Count();
  }

  /**
   * Forces cache replacement. Note that fun's cache strategy use for efficiency reason no background thread
   * which periodically triggers cache replacement. Cache Replacement is only started when the cache is modified
   * from outside, i.e. add is called, or when a user tries to access an cache element via get.
   * In some cases, i.e. expire based caching where for a long time no access to the cache happens,
   * it might be desirable to be able to trigger cache replacement manually.
   */
  void ForceReplace() {
    typename TMutex::ScopedLock lock(mutex_);
    DoReplace();
  }

  /**
   * Returns a copy of all keys stored in the cache
   */
  std::set<TKey> GetAllKeys() {
    typename TMutex::ScopedLock lock(mutex_);
    DoReplace();
    ConstIterator it = data_.begin();
    ConstIterator itEnd = data_.end();
    std::set<TKey> result;
    for (; it != itEnd; ++it)
      result.insert(it->first);

    return result;
  }

 protected:
  mutable FIFOEvent<ValidArgs<TKey> > IsValid;
  mutable FIFOEvent<KeySet> Replace;

  /**
   * Sets up event registration.
   */
  void Initialize() {
    Add += Delegate<TStrategy, const KeyValueArgs<TKey, TValue> >(&strategy_, &TStrategy::OnAdd);
    Update += Delegate<TStrategy, const KeyValueArgs<TKey, TValue> >(&strategy_, &TStrategy::OnUpdate);
    Remove += Delegate<TStrategy, const TKey>(&strategy_, &TStrategy::OnRemove);
    Get += Delegate<TStrategy, const TKey>(&strategy_, &TStrategy::OnGet);
    Clear += Delegate<TStrategy, const EventArgs>(&strategy_, &TStrategy::OnClear);
    IsValid += Delegate<TStrategy, ValidArgs<TKey> >(&strategy_, &TStrategy::OnIsValid);
    Replace += Delegate<TStrategy, KeySet>(&strategy_, &TStrategy::OnReplace);
  }

  /**
   * Reverts event registration.
   */
  void Uninitialize() {
    Add -= Delegate<TStrategy, const KeyValueArgs<TKey, TValue> >(&strategy_, &TStrategy::OnAdd );
    Update -= Delegate<TStrategy, const KeyValueArgs<TKey, TValue> >(&strategy_, &TStrategy::OnUpdate);
    Remove -= Delegate<TStrategy, const TKey>(&strategy_, &TStrategy::OnRemove);
    Get -= Delegate<TStrategy, const TKey>(&strategy_, &TStrategy::OnGet);
    Clear -= Delegate<TStrategy, const EventArgs>(&strategy_, &TStrategy::OnClear);
    IsValid -= Delegate<TStrategy, ValidArgs<TKey> >(&strategy_, &TStrategy::OnIsValid);
    Replace -= Delegate<TStrategy, KeySet>(&strategy_, &TStrategy::OnReplace);
  }

  /**
   * Adds the key value pair to the cache.
   * If for the key already an entry exists, it will be overwritten.
   */
  void DoAdd(const TKey& key, const TValue& val) {
    Iterator it = data_.find(key);
    DoRemove(it);

    KeyValueArgs<TKey, TValue> args(key, val);
    Add.Notify(this, args);
    data_.insert(std::make_pair(key, SharedPtr<TValue>(new TValue(val))));

    DoReplace();
  }

  /**
   * Adds the key value pair to the cache.
   * If for the key already an entry exists, it will be overwritten.
   */
  void DoAdd(const TKey& key, SharedPtr<TValue>& val) {
    Iterator it = data_.find(key);
    DoRemove(it);

    KeyValueArgs<TKey, TValue> args(key, *val);
    Add.Notify(this, args);
    data_.insert(std::make_pair(key, val));

    DoReplace();
  }

  /**
   * Adds the key value pair to the cache.
   * If for the key already an entry exists, it will be overwritten.
   */
  void DoUpdate(const TKey& key, const TValue& val) {
    KeyValueArgs<TKey, TValue> args(key, val);
    Iterator it = data_.find(key);
    if (it == data_.end()) {
      Add.Notify(this, args);
      data_.insert(std::make_pair(key, SharedPtr<TValue>(new TValue(val))));
    } else {
      Update.Notify(this, args);
      it->second = SharedPtr<TValue>(new TValue(val));
    }

    DoReplace();
  }

  /**
   * Adds the key value pair to the cache.
   * If for the key already an entry exists, it will be overwritten.
   */
  void DoUpdate(const TKey& key, SharedPtr<TValue>& val) {
    KeyValueArgs<TKey, TValue> args(key, *val);
    Iterator it = data_.find(key);
    if (it == data_.end()) {
      Add.Notify(this, args);
      data_.insert(std::make_pair(key, val));
    } else {
      Update.Notify(this, args);
      it->second = val;
    }

    DoReplace();
  }

  /**
   * Removes an entry from the cache. If the entry is not found
   * the remove is ignored.
   */
  void DoRemove(Iterator it) {
    if (it != data_.end()) {
      Remove.Notify(this, it->first);
      data_.erase(it);
    }
  }

  /**
   * Returns true if the cache contains a value for the key
   */
  bool DoHas(const TKey& key) const {
    // ask the strategy if the key is valid
    ConstIterator it = data_.find(key);
    bool result = false;

    if (it != data_.end()) {
      ValidArgs<TKey> args(key);
      IsValid.Notify(this, args);
      result = args.IsValid();
    }

    return result;
  }

  /**
   * Returns a SharedPtr of the cache entry, returns 0 if for
   * the key no value was found
   */
  SharedPtr<TValue> DoGet(const TKey& key) {
    Iterator it = data_.find(key);
    SharedPtr<TValue> result;

    if (it != data_.end()) {
      // inform all strategies that a read-access to an element happens
      Get.Notify(this, key);
      // ask all strategies if the key is valid
      ValidArgs<TKey> args(key);
      IsValid.Notify(this, args);

      if (!args.IsValid()) {
        DoRemove(it);
      } else {
        result = it->second;
      }
    }

    return result;
  }

  void DoClear() {
    static EventArgs empty_args;
    Clear.Notify(this, empty_args);
    data_.Clear();
  }

  void DoReplace() {
    std::set<TKey> delMe;
    Replace.Notify(this, delMe);
    // delMe contains the to be removed elements
    typename std::set<TKey>::const_iterator it = delMe.begin();
    typename std::set<TKey>::const_iterator endIt = delMe.end();

    for (; it != endIt; ++it) {
      Iterator itH = data_.find(*it);
      DoRemove(itH);
    }
  }

  TStrategy strategy_;
  mutable DataHolder data_;
  mutable TMutex mutex_;

 private:
  CacheBase(const CacheBase& cache);
  CacheBase& operator = (const CacheBase& cache);
};

} // namespace fun
