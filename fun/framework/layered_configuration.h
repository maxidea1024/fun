#pragma once

#include "fun/framework/framework.h"
#include "fun/framework/configuration_base.h"
#include "fun/base/ref_counted.h"
#include "fun/base/container/list.h"

namespace fun {
namespace framework {

//TODO sharable은 없어진건지 생긴건지??
//TODO 원본소스를 대조해봐야함.

/**
 * A LayeredConfiguration consists of a number of AbstractConfigurations.
 *
 * When reading a configuration property in a LayeredConfiguration,
 * all added configurations are searched, in order of their priority.
 * Configurations with lower priority values have precedence.
 *
 * When setting a property, the property is always written to the first writeable
 * configuration (see AddWritable()).
 * If no writeable configuration has been added to the LayeredConfiguration, and an
 * attempt is made to set a property, a RuntimeException is thrown.
 *
 * Every configuration added to the LayeredConfiguration has a priority value.
 * The priority determines the position where the configuration is inserted,
 * with lower priority values coming before higher priority values.
 *
 * If no priority is specified, a priority of 0 is assumed.
 */
class FUN_FRAMEWORK_API LayeredConfiguration : public ConfigurationBase {
  //TODO
  //FUN_DECLARE_RTCLASS(LayeredConfiguration, ConfigurationBase)

 public:
  using Ptr = RefCountedPtr<LayeredConfiguration>;

  /** Creates the LayeredConfiguration. */
  LayeredConfiguration();

  /**
   * Adds a read-only configuration to the back of the LayeredConfiguration.
   * The LayeredConfiguration takes shared ownership of the given configuration.
   */
  void Add(ConfigurationBase::Ptr config);

  /**
   * Adds a read-only configuration with the given label to the back of the LayeredConfiguration.
   * The LayeredConfiguration takes shared ownership of the given configuration.
   */
  void Add(ConfigurationBase::Ptr config, const String& label);

  /**
   * Adds a read-only configuration to the LayeredConfiguration.
   * The LayeredConfiguration takes shared ownership of the given configuration.
   */
  void Add(ConfigurationBase::Ptr config, int32 priority);

  /**
   * Adds a read-only configuration with the given label to the LayeredConfiguration.
   * The LayeredConfiguration takes shared ownership of the given configuration.
   */
  void Add(ConfigurationBase::Ptr config, const String& label, int32 priority);

  /**
   * Adds a configuration to the LayeredConfiguration.
   * The LayeredConfiguration takes shared ownership of the given configuration.
   */
  void Add(ConfigurationBase::Ptr config, int32 priority, bool writeable);

  /**
   * Adds a configuration with the given label to the LayeredConfiguration.
   * The LayeredConfiguration takes shared ownership of the given configuration.
   */
  void Add( ConfigurationBase::Ptr config,
            const String& label,
            int32 priority,
            bool writeable);

  /**
   * Adds a writeable configuration to the LayeredConfiguration.
   * The LayeredConfiguration does not take ownership of the given
   * configuration. In other words, the configuration's reference
   * count is incremented.
   */
  void AddWriteable(ConfigurationBase::Ptr config, int32 priority);

  /**
   * Finds and returns the configuration with the given label.
   *
   * Returns null if no such configuration can be found.
   */
  ConfigurationBase::Ptr Find(const String& label) const;

  /**
   * Removes the given configuration from the LayeredConfiguration.
   *
   * Does nothing if the given configuration is not part of the
   * LayeredConfiguration.
   */
  void RemoveConfiguration(ConfigurationBase::Ptr config);

  // Disable copy and assignment.
  LayeredConfiguration(const LayeredConfiguration&) = delete;
  LayeredConfiguration& operator = (const LayeredConfiguration&) = delete;

 protected:
  struct ConfigItem {
    ConfigurationBase::Ptr config;
    int32 priority;
    bool writeable;
    String label;
  };

  bool GetRaw(const String& key, String& value) const override;
  void SetRaw(const String& key, const String& value) override;
  void Enumerate(const String& key, Array<String>& range) const override;
  void RemoveRaw(const String& key) override;

  int32 Lowest() const;
  int32 Highest() const;
  void Insert(const ConfigItem& item);

  ~LayeredConfiguration();

 private:
  using ConfigList = List<ConfigItem>;
  ConfigList configs_;
};

} // namespace framework
} // namespace fun
