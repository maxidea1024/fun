#pragma once

#include "fun/framework/framework.h"
#include "fun/base/mutex.h"
#include "fun/base/multicast_event.h"
#include "fun/base/container/array.h"
#include "fun/base/ref_counted.h"

namespace fun {
namespace framework {

/**
 * ConfigurationBase is an abstract base class for different
 * kinds of configuration data, such as INI files, property files,
 * XML configuration files or the Windows Registry.
 *
 * Configuration property keys have a hierarchical format, consisting
 * of names separated by periods. The exact interpretation of key names
 * is up to the actual subclass implementation of ConfigurationBase.
 * Keys are case sensitive.
 *
 * All public methods are synchronized, so the class is safe for multithreaded use.
 * ConfigurationBase implements reference counting based garbage collection.
 *
 * Subclasses must override the GetRaw(), SetRaw() and Enumerate() methods.
*/
class FUN_FRAMEWORK_API ConfigurationBase : public RefCountedObject {
  //FUN_DECLARE_RTCLASS_BASE(ConfigurationBase)

 public:
  using Keys = Array<String>;
  using Ptr = RefCountedPtr<ConfigurationBase>;

  // Disable copy and assignment.
  ConfigurationBase(const ConfigurationBase&) = delete;
  ConfigurationBase& operator = (const ConfigurationBase&) = delete;

  /**
   * A key-value pair, used as event argument.
   */
  class KeyValue {
   public:
    KeyValue(const String& key, const String& value)
      : key_(key), value_(value) {}

    const String& Key() const { return key_; }
    const String& Value() const { return value_; }
    void SetValue(const String& value) { value_ = value; }

   private:
    String key_;
    String value_;
  };


  /**
   * Fired before a property value is changed or
   * a new property is created.
   *
   * Can be used to check or fix a property value,
   * or to cancel the change by throwing an exception.
   *
   * The event delegate can use one of the Get...() functions
   * to obtain the current property value.
   */
  MulticastEvent<KeyValue> property_changing;

  /**
   * Fired after a property value has been changed
   * or a property has been created.
   */
  MulticastEvent<const KeyValue> property_changed;

  /**
   * Fired before a property is removed by a
   * call to Remove().
   *
   * Note: This will even be fired if the key
   * does not exist and the remove operation will
   * fail with an exception.
   */
  MulticastEvent<const String> property_removing;

  /**
   * Fired after a property has been removed by
   * a call to Remove().
   */
  MulticastEvent<const String> property_removed;


  /**
   * Creates the ConfigurationBase.
   * ConfigurationBase();
   */
  virtual ~ConfigurationBase();

  /**
   * Returns true if the property with the given key exists.
   */
  bool HasProperty(const String& key) const;

  /**
   * Returns true if the property with the given key exists.
   *
   * Same as hasProperty().
   */
  bool HasOption(const String& key) const;

  /**
   * Returns true if the property with the given key exists.
   *
   * Same as hasProperty().
   */
  bool Has(const String& key) const;

  /**
   * Returns the string value of the property with the given name.
   * Throws a NotFoundException if the key does not exist.
   * If the value contains references to other properties (${<property>}), these
   * are expanded.
   */
  String GetString(const String& key) const;

  /**
   * If a property with the given key exists, returns the property's string value,
   * otherwise returns the given default value.
   * If the value contains references to other properties (${<property>}), these
   * are expanded.
   */
  String GetString(const String& key, const String& default_value) const;

  /**
   * Returns the raw string value of the property with the given name.
   * Throws a NotFoundException if the key does not exist.
   * References to other properties are not expanded.
   */
  String GetRawString(const String& key) const;

  /**
   * If a property with the given key exists, returns the property's raw string value,
   * otherwise returns the given default value.
   * References to other properties are not expanded.
   */
  String GetRawString(const String& key, const String& default_value) const;

  /**
   * Returns the int value of the property with the given name.
   * Throws a NotFoundException if the key does not exist.
   * Throws a SyntaxException if the property can not be converted
   * to an int.
   * Numbers starting with 0x are treated as hexadecimal.
   * If the value contains references to other properties (${<property>}), these
   * are expanded.
   */
  int32 GetInt(const String& key) const;

  /**
   * Returns the unsigned int value of the property with the given name.
   * Throws a NotFoundException if the key does not exist.
   * Throws a SyntaxException if the property can not be converted
   * to an unsigned int.
   * Numbers starting with 0x are treated as hexadecimal.
   * If the value contains references to other properties (${<property>}), these
   * are expanded.
   */
  uint32 GetUInt(const String& key) const;

  /**
   * If a property with the given key exists, returns the property's int value,
   * otherwise returns the given default value.
   * Throws a SyntaxException if the property can not be converted
   * to an int.
   * Numbers starting with 0x are treated as hexadecimal.
   * If the value contains references to other properties (${<property>}), these
   * are expanded.
   */
  int32 GetInt(const String& key, int32 default_value) const;

  /**
   * If a property with the given key exists, returns the property's unsigned int
   * value, otherwise returns the given default value.
   * Throws a SyntaxException if the property can not be converted
   * to an unsigned int.
   * Numbers starting with 0x are treated as hexadecimal.
   * If the value contains references to other properties (${<property>}), these
   * are expanded.
   */
  uint32 GetUInt(const String& key, uint32 default_value) const;

  /**
   * Returns the int64 value of the property with the given name.
   * Throws a NotFoundException if the key does not exist.
   * Throws a SyntaxException if the property can not be converted
   * to an int64.
   * Numbers starting with 0x are treated as hexadecimal.
   * If the value contains references to other properties (${<property>}), these
   * are expanded.
   */
  int64 GetInt64(const String& key) const;

  /**
   * Returns the uint64 value of the property with the given name.
   * Throws a NotFoundException if the key does not exist.
   * Throws a SyntaxException if the property can not be converted
   * to an uint64.
   * Numbers starting with 0x are treated as hexadecimal.
   * If the value contains references to other properties (${<property>}), these
   * are expanded.
   */
  uint64 GetUInt64(const String& key) const;

  /**
   * If a property with the given key exists, returns the property's int64 value,
   * otherwise returns the given default value.
   * Throws a SyntaxException if the property can not be converted
   * to an int64.
   * Numbers starting with 0x are treated as hexadecimal.
   * If the value contains references to other properties (${<property>}), these
   * are expanded.
   */
  int64 GetInt64(const String& key, int64 default_value) const;

  /**
   * If a property with the given key exists, returns the property's uint64
   * value, otherwise returns the given default value.
   * Throws a SyntaxException if the property can not be converted
   * to an uint64.
   * Numbers starting with 0x are treated as hexadecimal.
   * If the value contains references to other properties (${<property>}), these
   * are expanded.
   */
  uint64 GetUInt64(const String& key, uint64 default_value) const;

  /**
   * Returns the float value of the property with the given name.
   * Throws a NotFoundException if the key does not exist.
   * Throws a SyntaxException if the property can not be converted
   * to a float.
   * If the value contains references to other properties (${<property>}), these
   * are expanded.
   */
  float GetFloat(const String& key) const;

  /**
   * If a property with the given key exists, returns the property's float value,
   * otherwise returns the given default value.
   * Throws a SyntaxException if the property can not be converted
   * to an float.
   * If the value contains references to other properties (${<property>}), these
   * are expanded.
   */
  float GetFloat(const String& key, float default_value) const;

  /**
   * Returns the double value of the property with the given name.
   * Throws a NotFoundException if the key does not exist.
   * Throws a SyntaxException if the property can not be converted
   * to a double.
   * If the value contains references to other properties (${<property>}), these
   * are expanded.
   */
  double GetDouble(const String& key) const;

  /**
   * If a property with the given key exists, returns the property's double value,
   * otherwise returns the given default value.
   * Throws a SyntaxException if the property can not be converted
   * to an double.
   * If the value contains references to other properties (${<property>}), these
   * are expanded.
   */
  double GetDouble(const String& key, double default_value) const;

  /**
   * Returns the boolean value of the property with the given name.
   * Throws a NotFoundException if the key does not exist.
   * Throws a SyntaxException if the property can not be converted
   * to a boolean.
   * If the value contains references to other properties (${<property>}), these
   * are expanded.
   */
  bool GetBool(const String& key) const;

  /**
   * If a property with the given key exists, returns the property's boolean value,
   * otherwise returns the given default value.
   * Throws a SyntaxException if the property can not be converted
   * to a boolean.
   * The following string values can be converted into a boolean:
   *   - numerical values: non zero becomes true, zero becomes false
   *   - strings: true, yes, on become true, false, no, off become false
   * Case does not matter.
   * If the value contains references to other properties (${<property>}), these
   * are expanded.
   */
  bool GetBool(const String& key, bool default_value) const;

  /**
   * Sets the property with the given key to the given value.
   * An already existing value for the key is overwritten.
   */
  virtual void SetString(const String& key, const String& value);

  /**
   * Sets the property with the given key to the given value.
   * An already existing value for the key is overwritten.
   */
  virtual void SetInt(const String& key, int32 value);

  /**
   * Sets the property with the given key to the given value.
   * An already existing value for the key is overwritten.
   */
  virtual void SetUInt(const String& key, uint32 value);

  /**
   * Sets the property with the given key to the given value.
   * An already existing value for the key is overwritten.
   */
  virtual void SetInt64(const String& key, int64 value);

  /**
   * Sets the property with the given key to the given value.
   * An already existing value for the key is overwritten.
   */
  virtual void SetUInt64(const String& key, uint64 value);

  /**
   * Sets the property with the given key to the given value.
   * An already existing value for the key is overwritten.
   */
  virtual void SetFloat(const String& key, float value);

  /**
   * Sets the property with the given key to the given value.
   * An already existing value for the key is overwritten.
   */
  virtual void SetDouble(const String& key, double value);

  /**
   * Sets the property with the given key to the given value.
   * An already existing value for the key is overwritten.
   */
  virtual void SetBool(const String& key, bool value);

  /**
   * Returns in range the names of all keys at root level.
   */
  void GetKeys(Array<String>& out_keys) const;

  /**
   * Returns in range the names of all subkeys under the given key.
   * If an empty key is passed, all root level keys are returned.
   */
  void GetKeys(const String& key, Keys& out_keys) const;

  /**
   * Creates a non-mutable view (see ConfigurationView) into the configuration.
   */
  const ConfigurationBase::Ptr CreateView(const String& prefix) const;

  /**
   * Creates a view (see ConfigurationView) into the configuration.
   */
  ConfigurationBase::Ptr CreateView(const String& prefix);

  /**
   * Replaces all occurrences of ${<property>} in value with the
   * value of the <property>. If <property> does not exist,
   * nothing is changed.
   *
   * If a circular property reference is detected, a
   * CircularReferenceException will be thrown.
   */
  String Expand(const String& value) const;

  /**
   * Removes the property with the given key.
   *
   * Does nothing if the key does not exist.
   */
  void Remove(const String& key);

  /** Enables (or disables) events. */
  void SetEventsEnabled(bool enable = true);

  /** Returns true if events are enabled. */
  bool GetEventsEnabled() const;

 protected:
  ConfigurationBase();

  /**
   * If the property with the given key exists, stores the property's value
   * in value and returns true. Otherwise, returns false.
   *
   * Must be overridden by subclasses.
   */
  virtual bool GetRaw(const String& key, String& out_value) const = 0;

  /**
   * Sets the property with the given key to the given value.
   * An already existing value for the key is overwritten.
   *
   * Must be overridden by subclasses.
   */
  virtual void SetRaw(const String& key, const String& value) = 0;

  /**
   * Returns in range the names of all subkeys under the given key.
   * If an empty key is passed, all root level keys are returned.
   */
  virtual void Enumerate(const String& key, Keys& out_keys) const = 0;

  /**
   * Removes the property with the given key.
   *
   * Does nothing if the key does not exist.
   *
   * Should be overridden by subclasses; the default
   * implementation throws a NotImplementedException.
   */
  virtual void RemoveRaw(const String& key);

  /**
   * Returns string as signed integer.
   * Decimal and hexadecimal notation is supported.
   */
  static int32 ParseInt(const String& value);

  /**
   * Returns string as unsigned integer.
   * Decimal and hexadecimal notation is supported.
   */
  static uint32 ParseUInt(const String& value);

  /**
   * Returns string as 64-bit signed integer.
   * Decimal and hexadecimal notation is supported.
   */
  static int64 ParseInt64(const String& value);

  /**
   * Returns string as 64-bit unsigned integer.
   * Decimal and hexadecimal notation is supported.
   */
  static uint64 ParseUInt64(const String& value);

  static bool ParseBool(const String& value);

  void SetRawWithEvent(const String& key, const String& value);

 private:
  String ExpandInternal(const String& value) const;
  String UncheckedExpand(const String& value) const;

  mutable int32 depth_;
  bool events_enabled_;
  Mutex mutex_;

  friend class LayeredConfiguration;
  friend class ConfigurationView;
  friend class ConfigurationMapper;
};

} // namespace framework
} // namespace fun
