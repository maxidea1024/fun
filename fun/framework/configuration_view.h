#pragma once

#include "fun/framework/framework.h"
#include "fun/framework/configuration_base.h"

namespace fun {
namespace framework {

/**
 * This configuration implements a "view" into a sub-hierarchy
 * of another configuration.
 *
 * For example, given a configuration with the following properties:
 *     config.value1
 *     config.value2
 *     config.sub.value1
 *     config.sub.value2
 * and a ConfigurationView with the prefix "config", then
 * the above properties will be available via the view as
 *     value1
 *     value2
 *     sub.value1
 *     sub.value2
 *
 * A ConfigurationView is most useful in combination with a
 * LayeredConfiguration.
 *
 * If a property is not found in the view, it is searched in
 * the original configuration. Given the above example configuration,
 * the property named "config.value1" will still be found in the view.
 *
 * The main reason for this is that placeholder expansion (${property})
 * still works as expected given a ConfigurationView.
 */
class FUN_FRAMEWORK_API ConfigurationView : public ConfigurationBase {
  //TODO
  //FUN_DECLARE_RTCLASS(ConfigurationView, ConfigurationBase)

 public:
  /**
   * Creates the ConfigurationView. The ConfigurationView
   * retains (shared) ownership of the passed configuration.
   */
  ConfigurationView(const String& prefix, ConfigurationBase::Ptr config);

 protected:
  // ConfigurationBase interface.
  bool GetRaw(const String& key, String& out_value) const override;
  void SetRaw(const String& key, const String& value) override;
  void Enumerate(const String& key, Array<String>& out_keys) const override;
  void RemoveRaw(const String& key) override;

 private:
  String TranslateKey(const String& key) const;

  String prefix_;
  ConfigurationBase::Ptr config_;
};

} // namespace framework
} // namespace fun
