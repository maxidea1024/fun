#pragma once

#include "fun/base/base.h"
#include "fun/base/container/list.h"

namespace fun {

/**
 * An AutoReleasePool implements simple garbage collection for
 * reference-counted objects.
 *
 * It temporarily takes ownership of reference-counted objects that
 * nobody else wants to take ownership of and releases them
 * at a later, appropriate point in time.
 * 
 * Note: The correct way to add an object hold by an RefCountedPtr<> to
 * an AutoReleasePool is by invoking the RefCountedPtr's AddRef()
 * method. Example:
 *   AutoReleasePool<T> arp;
 *   RefCountedPtr<T> ptr = new T;
 *   ...
 *   arp.Add(ptr.AddRef());
 */
template <typename T>
class AutoReleasePool {
 public:
  /**
   * Creates the AutoReleasePool.
   */
  AutoReleasePool() {}

  /**
   * Destroys the AutoReleasePool and releases
   * all objects it currently holds.
   */
  ~AutoReleasePool() {
    Release();
  }

  /**
   * Adds the given object to the AutoReleasePool.
   * The object's reference count is not modified
   */
  void Add(T* object) {
    //TODO reference count가 이미 1일 경우에만 정상동작함.
    //하지만, 현재 사용되고 있는 RefCountedObject는 최초에
    //reference count가 0이므로, 문제가 발생하게됨.
    //수정하도록 하자.
    if (FUN_LIKELY(object)) {
      list_.PushBack(object);
    }
  }

  /**
   * Releases all objects the AutoReleasePool currently holds
   * by calling each object's Release() method.
   * */
  void Release() {
    while (!list_.IsEmpty()) {
      list_.Front()->Release();
      list_.PopFront();
    }
  }

 private:
  List<T*> list_;
};

} // namespace fun
