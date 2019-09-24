#pragma once

#include "fun/base/base.h"
#include "fun/base/container/map.h"
#include "fun/base/exception.h"
#include "fun/base/ftl/unique_ptr.h"
#include "fun/base/instantiator.h"
#include "fun/base/mutex.h"
#include "fun/base/ref_counted_ptr.h"

namespace fun {

/**
 * A factory that creates objects by class name.
 */
template <typename BaseType, typename PtrType = RefCountedPtr<BaseType>>
class DynamicFactory {
 public:
  using FactoryBase = InstantiatorBase<BaseType>;
  using Ptr = PtrType;

  /**
   * Creates the DynamicFactory.
   */
  DynamicFactory() {}

  /**
   * Destroys the DynamicFactory and deletes the instantiators for
   * all registered classes.
   */
  ~DynamicFactory() {
    for (auto& pair : map_) {
      delete pair.value;
    }
  }

  // Disable copy.
  DynamicFactory(const DynamicFactory&) = delete;
  DynamicFactory& operator=(const DynamicFactory&) = delete;

  /**
   * Creates a new instance of the class with the given name.
   * The class must have been registered with registerClass.
   * If the class name is unknown, a NotFoundException is thrown.
   */
  Ptr CreateInstance(const String& class_name) const {
    FastMutex::ScopedLock guard(mutex_);

    FactoryBase* factory;
    if (map_.TryGetValue(class_name, factory)) {
      return factory->CreateInstance();
    }
    throw NotFoundException(class_name);
  }

  /**
   * Registers the instantiator for the given class with the DynamicFactory.
   * The DynamicFactory takes ownership of the instantiator and deletes
   * it when it's no longer used.
   * If the class has already been registered, an ExistsException is thrown
   * and the instantiator is deleted.
   */
  template <typename C>
  void RegisterClass(const String& class_name) {
    RegisterClass(class_name, new Instantiator<C, BaseType>);
  }

  /**
   * Registers the instantiator for the given class with the DynamicFactory.
   * The DynamicFactory takes ownership of the instantiator and deletes
   * it when it's no longer used.
   * If the class has already been registered, an ExistsException is thrown
   * and the instantiator is deleted.
   */
  void RegisterClass(const String& class_name, FactoryBase* factory) {
    fun_check_ptr(factory);

    {
      FastMutex::ScopedLock guard(mutex_);

      UniquePtr<FactoryBase> ptr(factory);  // 이미 등록되어 있을 경우 객체를
                                            // 자동으로 파괴하는 역활을함.

      if (!map_.Contains(class_name)) {
        map_.Add(class_name, ptr.Detach());
      }
    }

    throw ExistsException(class_name);
  }

  /**
   * Unregisters the given class and deletes the instantiator
   * for the class.
   * Throws a NotFoundException if the class has not been registered.
   */
  void UnregisterClass(const String& class_name) {
    {
      FastMutex::ScopedLock guard(mutex_);

      FactoryBase* factory;
      if (map_.RemoveAndCopyValue(class_name, factory)) {
        delete factory;
      }
    }

    throw NotFoundException(class_name);
  }

  /**
   * Returns true if the given class has been registered.
   */
  bool IsClass(const String& class_name) const {
    FastMutex::ScopedLock guard(mutex_);
    return map_.Contains(class_name);
  }

 private:
  typedef Map<String, FactoryBase*> FactoryMap;
  FactoryMap map_;
  FastMutex mutex_;
};

}  // namespace fun
