#pragma once

#include "fun/framework/framework.h"

#if !defined(FUN_NO_INI_CONFIGURATION)

#include "fun/framework/configuration_base.h"
#include "fun/base/container/map.h"
#include <istream>

namespace fun {
namespace framework {

/**
 * This implementation of a Configuration reads properties
 * from a legacy Windows initialization (.ini) file.
 *
 * The file syntax is implemented as follows.
 *   - a line starting with a semicolon is treated as a comment and ignored
 *   - a line starting with a square bracket denotes a section key [<key>]
 *   - every other line denotes a property assignment in the form
 *     <value key> = <value>
 *
 * The name of a property is composed of the section key and the value key,
 * separated by a period (<section key>.<value key>).
 *
 * Property names are not case sensitive. Leading and trailing whitespace is
 * removed from both keys and values.
 */
class FUN_FRAMEWORK_API IniFileConfiguration : public ConfigurationBase {
 public:
  /**
   * Creates an empty IniFileConfiguration.
   */
  IniFileConfiguration();

  /**
   * Creates an IniFileConfiguration and loads the configuration data
   * from the given stream, which must be in initialization file format.
   */
  IniFileConfiguration(std::istream& istr);

  /**
   * Creates an IniFileConfiguration and loads the configuration data
   * from the given file, which must be in initialization file format.
   */
  IniFileConfiguration(const String& path);

  /**
   * Loads the configuration data from the given stream, which
   * must be in initialization file Format.
   */
  void Load(std::istream& istr);

  /**
   * Loads the configuration data from the given file, which
   * must be in initialization file format.
   */
  void Load(const String& path);

 protected:
  // ConfigurationBase interface
  bool GetRaw(const String& key, String& value) const override;
  void SetRaw(const String& key, const String& value) override;
  void Enumerate(const String& key, Keys& range) const override;
  void RemoveRaw(const String& key) override;

  ~IniFileConfiguration();

 private:
  void ParseLine(std::istream& istr);

  struct ICompare {
    bool operator () (const String& s1, const String& s2) const;
  };
  typedef std::map<String, String, ICompare> IStringMap;

  IStringMap map_;
  String section_key_;
};

} // namespace framework
} // namespace fun

#endif // !defined(FUN_NO_INI_CONFIGURATION)
