#pragma once

#include "fun/framework/framework.h"

#ifndef FUN_NO_JSONCONFIGURATION

#include <istream>
#include "fun/framework/configuration_base.h"
#include "fun/json/object.h"

namespace fun {
namespace framework {

/**
 * This configuration class extracts configuration properties
 * from a JSON object. An XPath-like syntax for property
 * names is supported to allow full access to the JSON object.
 *
 * Given the following JSON object as an example:
 *   {
 *     "config" : {
 *        "prop1" : "value1",
 *        "prop2" : 10,
 *        "prop3" : [
 *          "element1",
 *          "element2"
 *        ],
 *        "prop4" : {
 *          "prop5" : false,
 *          "prop6" : null
 *        }
 *     }
 *   }
 *
 * The following property names would be valid and would
 * yield the shown values:
 *
 * config.prop1       --> "value1"
 * config.prop3[1]    --> "element2"
 * config.prop4.prop5 --> false
 */
class FUN_FRAMEWORK_API JsonFileConfiguration : public ConfigurationBase {
 public:
  /** Creates an empty configuration */
  JsonFileConfiguration();

  /** Creates a configuration and loads the JSON structure from the given file
   */
  JsonFileConfiguration(const String& path);

  /** Creates a configuration and loads the JSON structure from the given stream
   */
  JsonFileConfiguration(std::istream& istr);

  /** Creates a configuration from the given JSON object */
  JsonFileConfiguration(const JSON::Object::Ptr& object);

  /** Destructor */
  virtual ~JsonFileConfiguration();

  /** Loads the configuration from the given file */
  void Load(const String& path);

  /** Loads the configuration from the given stream */
  void Load(std::istream& istr);

  /** Loads an empty object containing only a root object with the given name.
   */
  void LoadEmpty(const String& root);

  /** Saves the configuration to the given stream */
  void Save(std::ostream& ostr, unsigned int indent = 2) const;

  virtual void SetInt(const String& key, int value);
  virtual void SetBool(const String& key, bool value);
  virtual void SetDouble(const String& key, double value);
  virtual void SetString(const String& key, const String& value);
  virtual void RemoveRaw(const String& key);

 protected:
  bool GetRaw(const String& key, String& value) const;
  void SetRaw(const String& key, const String& value);
  void Enumerate(const String& key, Keys& range) const;

 private:
  JSON::Object::Ptr FindStart(const String& key, String& last_part);
  void GetIndexes(String& name, Array<int>& indexes);
  void SetValue(const String& key, const fun::dynamicAny& value);

  JSON::Object::Ptr object_;
};

}  // namespace framework
}  // namespace fun

#endif  // FUN_NO_JSONCONFIGURATION
