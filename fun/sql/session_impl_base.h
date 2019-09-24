#pragma once

#include <map>
#include "fun/sql/session_impl.h"
#include "fun/sql/sql.h"
#include "fun/sql/sql_exception.h"

namespace fun {
namespace sql {

/**
 * A partial implementation of SessionImpl, providing
 * features and properties management.
 *
 * To implement a certain feature or property, a subclass
 * must provide setter and getter methods and register
 * them with AddFeature() or AddProperty().
 */
template <typename C>
class SessionImplBase : public SessionImpl {
 public:
  /** The setter method for a feature. */
  typedef void (C::*FeatureSetter)(const String&, bool);

  /** The getter method for a feature. */
  typedef bool (C::*FeatureGetter)(const String&) const;

  /** The setter method for a property. */
  typedef void (C::*PropertySetter)(const String&, const fun::Any&);

  /** The getter method for a property. */
  typedef fun::Any (C::*PropertyGetter)(const String&) const;

  /**
   * Creates the SessionImplBase.
   *
   * Adds "storage" property and sets the default internal storage container
   * type to std::deque.
   * The storage is created by statements automatically whenever a query
   * returning results is executed but external storage is provided by the user.
   * Storage type can be reconfigured at runtime both globally (for the
   * duration of the session) and locally (for a single statement execution
   * only). See StatementImpl for details on how this property is used at
   * runtime.
   *
   * Adds "handle" property which, if set by the back end, returns native handle
   * for the back end DB.
   *
   * Adds "bulk" feature and sets it to false.
   * Bulk feature determines whether the session is capable of bulk operations.
   * Connectors that are capable of it must set this feature prior to attempting
   * bulk operations.
   *
   * Adds "empty_string_is_null" feature and sets it to false. This feature
   * should be set to true in order to modify the behavior of the databases that
   * distinguish between zero-length character strings as nulls. Setting this
   * feature to true shall disregard any difference between empty character
   * strings and nulls, causing the framework to treat them the same (i.e.
   * behave like Oracle).
   *
   * Adds "force_empty_string" feature and sets it to false. This feature should
   * be set to true in order to force the databases that do not distinguish
   * empty strings from nulls (e.g. Oracle) to always report empty string.
   *
   * The "empty_string_is_null" and "force_empty_string" features are mutually
   * exclusive. While these features can not both be true at the same time, they
   * can both be false, resulting in default underlying database behavior.
   */
  SessionImplBase(const String& connection_string,
                  size_t timeout = LOGIN_TIMEOUT_DEFAULT)
      : SessionImpl(connection_string, timeout),
        storage_(String("deque")),
        bulk_(false),
        empty_string_is_null_(false),
        force_empty_string_(false) {
    AddProperty("storage", &SessionImplBase<C>::SetStorage,
                &SessionImplBase<C>::GetStorage);

    AddProperty("handle", &SessionImplBase<C>::SetHandle,
                &SessionImplBase<C>::GetHandle);

    AddFeature("bulk", &SessionImplBase<C>::SetBulk,
               &SessionImplBase<C>::GetBulk);

    AddFeature("empty_string_is_null",
               &SessionImplBase<C>::SetEmptyStringIsNull,
               &SessionImplBase<C>::GetEmptyStringIsNull);

    AddFeature("force_empty_string", &SessionImplBase<C>::SetForceEmptyString,
               &SessionImplBase<C>::GetForceEmptyString);
  }

  /**
   * Destroys the SessionImplBase.
   */
  ~SessionImplBase() {}

  /**
   * Looks a feature up in the features map
   * and calls the feature's setter, if there is one.
   */
  void SetFeature(const String& name, bool state) {
    typename FeatureMap::const_iterator it = features_.find(name);
    if (it != features_.end()) {
      if (it->second.setter) {
        (static_cast<C*>(this)->*it->second.setter)(name, state);
      } else {
        throw NotImplementedException("set", name);
      }
    } else {
      throw NotSupportedException(name);
    }
  }

  /**
   * Looks a feature up in the features map
   * and calls the feature's getter, if there is one.
   */
  bool GetFeature(const String& name) const {
    typename FeatureMap::const_iterator it = features_.find(name);
    if (it != features_.end()) {
      if (it->second.getter) {
        return (static_cast<const C*>(this)->*it->second.getter)(name);
      } else {
        throw NotImplementedException("get", name);
      }
    } else {
      throw NotSupportedException(name);
    }
  }

  /**
   * Looks a property up in the properties map
   * and calls the property's setter, if there is one.
   */
  void SetProperty(const String& name, const fun::Any& value) {
    typename PropertyMap::const_iterator it = properties_.find(name);
    if (it != properties_.end()) {
      if (it->second.setter) {
        (static_cast<C*>(this)->*it->second.setter)(name, value);
      } else {
        throw NotImplementedException("set", name);
      }
    } else {
      throw NotSupportedException(name);
    }
  }

  /**
   * Looks a property up in the properties map
   * and calls the property's getter, if there is one.
   */
  fun::Any GetProperty(const String& name) const {
    typename PropertyMap::const_iterator it = properties_.find(name);
    if (it != properties_.end()) {
      if (it->second.getter) {
        return (static_cast<const C*>(this)->*it->second.getter)(name);
      } else {
        throw NotImplementedException("set", name);
      }
    } else {
      throw NotSupportedException(name);
    }
  }

  /**
   * Sets the storage type.
   */
  void SetStorage(const String& value) { storage_ = value; }

  /**
   * Sets the storage type.
   */
  void SetStorage(const String& /*name*/, const fun::Any& value) {
    storage_ = fun::RefAnyCast<String>(value);
  }

  /**
   * Returns the storage type
   */
  fun::Any GetStorage(const String& /*name*/ = "") const { return storage_; }

  /**
   * Sets the native session handle.
   */
  void SetHandle(const String& /*name*/, const fun::Any& handle) {
    handle_ = handle;
  }

  /**
   * Returns the native session handle.
   */
  fun::Any GetHandle(const String& /*name*/ = "") const { return handle_; }

  /**
   * Sets the execution type.
   */
  void SetBulk(const String& /*name*/, bool bulk) { bulk_ = bulk; }

  /**
   * Returns the execution type
   */
  bool GetBulk(const String& /*name*/ = "") const { return bulk_; }

  /**
   * Sets the behavior regarding empty variable length strings.
   * Those are treated as NULL by Oracle and as empty string by
   * most other databases.
   * When this feature is true, empty strings are treated as NULL.
   */
  void SetEmptyStringIsNull(const String& /*name*/, bool empty_string_is_null) {
    if (empty_string_is_null && force_empty_string_) {
      throw InvalidAccessException("Features mutually exclusive");
    }

    empty_string_is_null_ = empty_string_is_null;
  }

  /**
   * Returns the setting for the behavior regarding empty variable
   * length strings. See SetEmptyStringIsNull(const String&, bool)
   * and this class documentation for feature rationale and details.
   */
  bool GetEmptyStringIsNull(const String& /*name*/ = "") const {
    return empty_string_is_null_;
  }

  /**
   * Sets the behavior regarding empty variable length strings.
   * Those are treated as NULL by Oracle and as empty string by
   * most other databases.
   * When this feature is true, both empty strings and NULL values
   * are reported as empty strings.
   */
  void SetForceEmptyString(const String& /*name*/, bool force_empty_string) {
    if (force_empty_string && empty_string_is_null_) {
      throw InvalidAccessException("Features mutually exclusive");
    }

    force_empty_string_ = force_empty_string;
  }

  /**
   * Returns the setting for the behavior regarding empty variable
   * length strings. See SetForceEmptyString(const String&, bool)
   * and this class documentation for feature rationale and details.
   */
  bool GetForceEmptyString(const String& /*name*/ = "") const {
    return force_empty_string_;
  }

 protected:
  /**
   * Adds a feature to the map of supported features.
   *
   * The setter or getter can be null, in case setting or getting a feature
   * is not supported.
   */
  void AddFeature(const String& name, FeatureSetter setter,
                  FeatureGetter getter) {
    Feature feature;
    feature.setter = setter;
    feature.getter = getter;
    features_[name] = feature;
  }

  /**
   * Adds a property to the map of supported properties.
   *
   * The setter or getter can be null, in case setting or getting a property
   * is not supported.
   */
  void AddProperty(const String& name, PropertySetter setter,
                   PropertyGetter getter) {
    Property property;
    property.setter = setter;
    property.getter = getter;
    properties_[name] = property;
  }

 private:
  struct Feature {
    FeatureSetter setter;
    FeatureGetter getter;
  };

  struct Property {
    PropertySetter setter;
    PropertyGetter getter;
  };

  typedef std::map<String, Feature> FeatureMap;
  typedef std::map<String, Property> PropertyMap;

  FeatureMap features_;
  PropertyMap properties_;
  String storage_;
  bool bulk_;
  bool empty_string_is_null_;
  bool force_empty_string_;
  fun::Any handle_;
};

}  // namespace sql
}  // namespace fun
