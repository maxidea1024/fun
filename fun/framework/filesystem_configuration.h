#pragma once

#include "fun/base/path.h"
#include "fun/framework/configuration_base.h"
#include "fun/framework/framework.h"

namespace fun {
namespace framework {

/**
 * An implementation of ConfigurationBase that stores configuration data
 * in a directory hierarchy in the filesystem.
 *
 * Every period-separated part of a property name is represented
 * as a directory in the filesystem, relative to the base directory.
 * Values are stored in files named "data".
 *
 * All changes to properties are immediately persisted in the filesystem.
 *
 * For example, a configuration consisting of the properties
 *
 *   logging.loggers.root.channel.class = ConsoleSink
 *   logging.loggers.app.name = Application
 *   logging.loggers.app.channel = c1
 *   logging.formatters.f1.class = PatternFormatter
 *   logging.formatters.f1.pattern = [%p] %t
 *
 * is stored in the filesystem as follows:
 *
 *   logging/
 *     loggers/
 *       root/
 *        channel/
 *          class/
 *            data ("ConsoleSink")
 *       app/
 *       name/
 *          data ("Application")
 *       channel/
 *         data ("c1")
 *     formatters/
 *        f1/
 *       class/
 *         data ("PatternFormatter")
 *       pattern/
 *         data ("[%p] %t")
 */
class FUN_FRAMEWORK_API FilesystemConfiguration : public ConfigurationBase {
 public:
  /**
   * Creates a FilesystemConfiguration using the given path.
   * All directories are created as necessary.
   */
  FilesystemConfiguration(const String& path);

  /**
   * Clears the configuration by erasing the configuration
   * directory and all its subdirectories and files.
   */
  void Clear();

 protected:
  bool GetRaw(const String& key, String& value) const override;
  void SetRaw(const String& key, const String& value) override;
  void Enumerate(const String& key, Keys& range) const override;
  void RemoveRaw(const String& key) override;

  Path KeyToPath(const String& key) const;

  ~FilesystemConfiguration();

 private:
  Path path_;
};

}  // namespace framework
}  // namespace fun
