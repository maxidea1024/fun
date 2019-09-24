#pragma once

#include "fun/framework/framework.h"
#include "fun/framework/configuration_base.h"
#include "fun/base/string/string.h"
#include "fun/base/windows_less.h"

namespace fun {
namespace framework {

/**
 * An implementation of ConfigurationBase that stores configuration data
 * in the Windows registry.
 *
 * Removing key is not supported. An attempt to remove a key results
 * in a NotImplementedException being thrown.
 */
class FUN_FRAMEWORK_API WinRegistryConfiguration : public ConfigurationBase {
 public:
  /**
   * Creates the WinRegistryConfiguration.
   * The root_path must start with one of the root key names
   * like HKEY_CLASSES_ROOT, e.g. HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services.
   * All further keys are relative to the root path and can be
   * dot separated, e.g. the path MyService.ServiceName will be converted to
   * HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\MyService\ServiceName.
   * The extra_sam parameter will be passed along to WinRegistryKey, to control
   * registry virtualization for example.
   */
  WinRegistryConfiguration(const String& root_path, REGSAM extra_sam = 0);

 protected:
  /**
   * Destroys the WinRegistryConfiguration.
   */
  ~WinRegistryConfiguration();

  // ConfigurationBase interface.
  bool GetRaw(const String& key, String& value) const override;
  void SetRaw(const String& key, const String& value) override;
  void Enumerate(const String& key, Keys& range) const override;
  void RemoveRaw(const String& key) override;

  /**
   * Takes a key in the Format of A.B.C and converts it to
   * registry Format A\B\C, the last entry is the key_name,
   * the rest is returned as path
   */
  String ConvertToRegFormat(const String& key, String& key_name) const;

  friend class WinConfigurationTest;

 private:
  String root_path_;
  REGSAM extra_sam_;
};

} // namespace framework
} // namespace fun
