#pragma once

#include "fun/framework/configuration_base.h"
#include "fun/framework/framework.h"

namespace fun {
namespace framework {

/**
 * This class implements a Configuration interface to
 * various system properties and environment variables.
 *
 * The following properties are supported:
 *   - system.os_name        : the operating system name
 *   - system.os_version     : the operating system version
 *   - system.os_architecture: the operating system architecture
 *   - system.node_name      : the node (or host) name
 *   - system.node_Id        : system ID, based on the Ethernet address (Format
 * "xxxxxxxxxxxx") of the first Ethernet adapter found on the system.
 *   - system.current_dir    : the current working directory
 *   - system.home_dir       : the user's home directory
 *   - system.config_home_dir: the base directory relative to which user
 * specific configuration files should be stored
 *   - system.cache_home_dir : the base directory relative to which user
 * specific non-essential data files should be stored
 *   - system.data_home_dir  : the base directory relative to which user
 * specific data files should be stored
 *   - system.temp_dir       : the system's temporary directory
 *   - system.config_dir     : the system's configuration directory
 *   - system.datetime       : the current UTC date and time, formatted in ISO
 * 8601 Format.
 *   - system.pid            : the current process ID.
 *   - system.env.<NAME>     : the environment variable with the given <NAME>.
 *
 * An attempt to set a system variable will result in an
 * InvalidAccessException being thrown.
 *
 * Enumerating environment variables is not supported.
 * An attempt to call keys("system.env") will return an empty range.
 *
 * Removing key is not supported. An attempt to remove a key results
 * in a NotImplementedException being thrown.
 */
class FUN_FRAMEWORK_API SystemConfiguration : public ConfigurationBase {
  // TODO
  // FUN_DECLARE_RTCLASS(SystemConfiguration, ConfigurationBase)

 public:
  SystemConfiguration();

 protected:
  // ConfigurationBase interface
  bool GetRaw(const String& key, String& out_value) const override;
  void SetRaw(const String& key, const String& value) override;
  void Enumerate(const String& key, Array<String>& out_keys) const override;
  void RemoveRaw(const String& key) override;

  ~SystemConfiguration();

 private:
  static bool GetEnv(const String& name, String& value);

  static const String OSNAME;
  static const String OSVERSION;
  static const String OSARCHITECTURE;
  static const String NODENAME;
  static const String NODEID;
  static const String CURRENTDIR;
  static const String HOMEDIR;
  static const String CONFIGHOMEDIR;
  static const String CACHEHOMEDIR;
  static const String DATAHOMEDIR;
  static const String TEMPDIR;
  static const String CONFIGDIR;
  static const String DATETIME;

#if FUN_PLATFORM != FUN_PLATFORM_VXWORKS
  static const String PID;
#endif
  static const String ENV;
};

}  // namespace framework
}  // namespace fun
