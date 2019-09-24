#pragma once

#include "fun/base/base.h"
#include "fun/base/uuid.h"
#include "fun/base/string/string.h"
#include "fun/base/crc.h"
#include "fun/base/container/set.h"

namespace fun {

namespace CustomVersionSerializationFormat {
  enum Type {
    Unknown,
    Uuids,
    Enums,
    Optimized,

    // Add new versions above this comment
    CustomVersion_Automatic_Plus_One,
    Latest = CustomVersion_Automatic_Plus_One - 1
  };
};


/**
 * Structure to hold unique custom key with its version.
 */
class FUN_BASE_API CustomVersion {
 public:
  friend class CustomVersionContainer;

  /** Unique custom key. */
  Uuid key;

  /** Custom version. */
  int32 version;

  /** Number of times this GUID has been registered */
  int32 reference_count;

  /** Constructor. */
  inline CustomVersion() {}

  /** Helper constructor. */
  inline CustomVersion(Uuid key, int32 version, const String& friendly_name)
    : key(key),
      version(version),
      reference_count(1),
      friendly_name_(friendly_name) {}

  /** Equality comparison operator for key */
  inline bool operator == (const Uuid& key) const {
    return this->key == key;
  }

  /** Inequality comparison operator for key */
  inline bool operator != (const Uuid& key) const {
    return this->key != key;
  }

  FUN_BASE_API friend Archive& operator & (Archive& ar, CustomVersion& version);

  /** Gets the friendly name for error messages or whatever */
  const String GetFriendlyName() const;

 private:
  /** Friendly name for error messages or whatever. Lazy initialized for serialized versions */
  mutable String friendly_name_;
};


struct CustomVersionKeyFuncs : BaseKeyFuncs<CustomVersion, Uuid, false> {
  static inline const Uuid& GetSetKey(const CustomVersion& element) {
    return element.key;
  }

  static inline bool Matches(const Uuid& a, const Uuid& b) {
    return a == b;
  }

  static inline uint32 GetKeyHash(const Uuid& key) {
    return Crc::Crc32(&key, sizeof(Uuid));
  }
};

typedef Set<CustomVersion, CustomVersionKeyFuncs> CustomVersionSet;

class FUN_BASE_API CustomVersionRegistration;


/**
 * Container for all available/serialized custom versions.
 */
class FUN_BASE_API CustomVersionContainer {
 public:
  friend class CustomVersionRegistration;

 public:
  /** Gets available custom versions in this container. */
  inline const CustomVersionSet& GetAllVersions() const {
    return versions_;
  }

  /**
   * Gets a custom version from the container.
   *
   * @param custom_key - Custom key for which to retrieve the version.
   *
   * @return The CustomVersion for the specified custom key, or nullptr if the key doesn't exist in the container.
   */
  const CustomVersion* GetVersion(const Uuid& custom_key) const;

  /**
   * Gets a custom version friendly name from the container.
   *
   * @param custom_key - Custom key for which to retrieve the version.
   *
   * @return The friendly name for the specified custom key, or NAME_None if the key doesn't exist in the container.
   */
  const String GetFriendlyName(const Uuid& custom_key) const;

  /**
   * Sets a specific custom version in the container.
   *
   * @param custom_key - Custom key for which to retrieve the version.
   * @param version - The version number for the specified custom key
   * @param friendly_name - A friendly name to assign to this version
   */
  void SetVersion(const Uuid& custom_key, int32 version, const String& friendly_name);

  /** Serialization. */
  void Serialize(Archive& ar, CustomVersionSerializationFormat::Type format = CustomVersionSerializationFormat::Latest);

  /**
   * Gets a singleton with the registered versions.
   *
   * @return The registered version container.
   */
  static const CustomVersionContainer& GetRegistered();

  /**
   * Empties the custom version container.
   */
  void Clear();

  /** Return a string representation of custom versions. Used for debug. */
  String ToString(const String& indent) const;

 private:
  /** Private implementation getter */
  static CustomVersionContainer& GetInstance();

  /** Array containing custom versions. */
  CustomVersionSet versions_;
};


/**
 * This class will cause the registration of a custom version number and key with the global
 * CustomVersionContainer when instantiated, and unregister when destroyed.  It should be
 * instantiated as a global variable somewhere in the module which needs a custom version number.
 */
class CustomVersionRegistration : Noncopyable {
 public:
  CustomVersionRegistration(const Uuid& key, int32 version, const String& friendly_name);
  ~CustomVersionRegistration();

 private:
  Uuid key;
};

} // namespace fun
