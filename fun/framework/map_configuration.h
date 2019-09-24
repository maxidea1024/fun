#pragma once

#include "fun/framework/framework.h"
#include "fun/framework/configuration_base.h"
#include "fun/base/container/map.h"

namespace fun {
namespace framework {

/**
 * An implementation of ConfigurationBase that stores configuration data in a map.
 */
class FUN_FRAMEWORK_API MapConfiguration : public ConfigurationBase {
  //TODO
  //FUN_DECLARE_RTCLASS(MapConfiguration,ConfigurationBase)

 public:
  /** Creates an empty MapConfiguration. */
  MapConfiguration();

  /** Copies all configuration properties to the given configuration. */
  void CopyTo(ConfigurationBase& config);

  /** Clear the configuration. */
  void Clear();

 protected:
  bool GetRaw(const String& key, String& out_value) const override;
  void SetRaw(const String& key, const String& value) override;
  void Enumerate(const String& key, Array<String>& out_keys) const override;
  void RemoveRaw(const String& key) override;

  //TODO Map자체를 돌려주던지, 제거하자.
  //Map<String,String>::ConstIterator begin() const { return map_.begin(); }
  //Map<String,String>::ConstIterator end() const { return map_.end(); }
  const Map<String, String>& GetMap() const { return map_;  }

 private:
  Map<String, String> map_;
};

} // namespace framework
} // namespace fun
