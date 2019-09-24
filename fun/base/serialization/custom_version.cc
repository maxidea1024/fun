#include "fun/base/serialization/custom_version.h"

namespace fun {

namespace {

CustomVersion unused_custom_version_(Uuid(0, 0, 0, 0xF99D40C1), 0, "Unused custom version");

struct EnumCustomVersion_DEPRECATED {
  uint32 tag;
  int32 version;

  CustomVersion ToCustomVersion() const {
    // We'll invent a GUID from three zeroes and the original tag
    return CustomVersion(Uuid(0, 0, 0, tag), version, String::Format("EnumTag{0}", tag));
  }
};

Archive& operator & (Archive& ar, EnumCustomVersion_DEPRECATED& v) {
  // Serialize keys
  ar & v.tag;
  ar & v.version;
  return ar;
}

struct UuidCustomVersion_DEPRECATED {
  Uuid key;
  int32 version;
  String friendly_name;

  CustomVersion ToCustomVersion() const {
    // We'll invent a GUID from three zeroes and the original tag
    return CustomVersion(key, version, friendly_name);
  }
};

Archive& operator & (Archive& ar, UuidCustomVersion_DEPRECATED& v) {
  ar & v.key;
  ar & v.version;
  ar & v.friendly_name;
  return ar;
}

} // local namespace

const String CustomVersion::GetFriendlyName() const {
  if (friendly_name_ == "") {
    friendly_name_ = CustomVersionContainer::GetRegistered().GetFriendlyName(key);
  }
  return friendly_name_;
}

const CustomVersionContainer& CustomVersionContainer::GetRegistered() {
  return GetInstance();
}

void CustomVersionContainer::Clear() {
  versions_.Clear();
}

String CustomVersionContainer::ToString(const String& indent) const {
  String versions_as_string;
  for (const CustomVersion& version : versions_) {
    versions_as_string += indent;
    versions_as_string += String::Format("key={0}  version={1}  friendly name={2}\n",
                              version.key.ToString(),
                              version.version,
                              version.GetFriendlyName());
  }

  return versions_as_string;
}

CustomVersionContainer& CustomVersionContainer::GetInstance() {
  static CustomVersionContainer singleton;
  return singleton;
}

Archive& operator & (Archive& ar, CustomVersion& v) {
  ar & v.key;
  ar & v.version;
  return ar;
}

void CustomVersionContainer::Serialize(Archive& ar, CustomVersionSerializationFormat::Type format) {
  switch (format) {
    default:
      fun_check(false);

    case CustomVersionSerializationFormat::Enums: {
      // We should only ever be loading enums.
      // They should never be saved - they only exist for backward compatibility.
      fun_check(ar.IsLoading());

      Array<EnumCustomVersion_DEPRECATED> old_tags;
      ar & old_tags;

      versions_.Clear(old_tags.Count());
      for (auto it = old_tags.CreateConstIterator(); it; ++it) {
        versions_.Add(it->ToCustomVersion());
      }
    }
    break;

    case CustomVersionSerializationFormat::Uuids: {
      // We should only ever be loading old versions.
      // They should never be saved - they only exist for backward compatibility.
      fun_check(ar.IsLoading());

      Array<UuidCustomVersion_DEPRECATED> version_array;
      ar & version_array;
      versions_.Clear(version_array.Count());
      for (UuidCustomVersion_DEPRECATED& old_version : version_array) {
        versions_.Add(old_version.ToCustomVersion());
      }
    }
    break;

    case CustomVersionSerializationFormat::Optimized: {
      ar & versions_;
    }
    break;
  }
}

const CustomVersion*
CustomVersionContainer::GetVersion(const Uuid& key) const {
  // A testing tag was written out to a few archives during testing so we need to
  // handle the existence of it to ensure that those archives can still be loaded.
  if (key == unused_custom_version_.key) {
    return &unused_custom_version_;
  }

  return versions_.Find(key);
}

const String CustomVersionContainer::GetFriendlyName(const Uuid& key) const {
  String friendly_name = "";

  const CustomVersion* custom_version = GetVersion(key);
  if (custom_version) {
    friendly_name = custom_version->friendly_name_;
  }
  return friendly_name;
}

void CustomVersionContainer::SetVersion(const Uuid& custom_key,
                                        int32 version,
                                        const String& friendly_name) {
  if (custom_key == unused_custom_version_.key) {
    return;
  }

  if (CustomVersion* found = versions_.Find(custom_key)) {
    found->version = version;
    found->friendly_name_ = friendly_name;
  } else {
    versions_.Add(CustomVersion(custom_key, version, friendly_name));
  }
}

CustomVersionRegistration::CustomVersionRegistration( const Uuid& key,
                                                      int32 version,
                                                      const String& friendly_name) {
  CustomVersionSet& versions = CustomVersionContainer::GetInstance().versions_;

  // Check if this tag hasn't already been registered
  if (CustomVersion* existing_registration = versions.Find(key)) {
    // We don't allow the registration details to change across registrations - this code path only exists to support hotreload

    // If you hit this then you've probably either:
    // * Changed registration details during hotreload.
    // * Accidentally copy-and-pasted an CustomVersionRegistration object.

    //TODO
    //ENSURE_MSGF(
    //    existing_registration->version == version && existing_registration->GetFriendlyName() == friendly_name,
    //    "Custom version registrations cannot change between hotreloads - \"%s\" version %d is being reregistered as \"%s\" version %d",
    //    *existing_registration->GetFriendlyName().ToString(),
    //    existing_registration->version,
    //    friendly_name,
    //    version);

    ++existing_registration->reference_count;
  } else {
    versions.Add(CustomVersion(key, version, friendly_name));
  }

  this->key = key;
}

CustomVersionRegistration::~CustomVersionRegistration() {
  CustomVersionSet& versions = CustomVersionContainer::GetInstance().versions_;

  CustomVersion* found = versions.Find(key);

  // Ensure this tag has been registered
  fun_check_ptr(found);

  --found->reference_count;
  if (found->reference_count == 0) {
    versions.Remove(key);
  }
}

} // namespace fun
