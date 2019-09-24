#include <QtCore/QSet>
#include "qtimezone.h"
#include "qtimezoneprivate_p.h"

namespace fun {

// Create the system default time zone
AndroidTimeZoneImpl::AndroidTimeZoneImpl()
  : TimeZoneImpl() {
  // start with system time zone
  android_time_zone_ = JNIObjectImpl::callStaticObjectMethod("java.util.TimeZone", "getDefault", "()Ljava/util/TimeZone;");
  Init("UTC");
}

// Create a named time zone
AndroidTimeZoneImpl::AndroidTimeZoneImpl(const String& iana_id)
  : TimeZoneImpl() {
  Init(iana_id);
}

AndroidTimeZoneImpl::AndroidTimeZoneImpl(const AndroidTimeZoneImpl& other)
  : TimeZoneImpl(other) {
  android_time_zone_ = other.android_time_zone_;
  id_ = other.id();
}

AndroidTimeZoneImpl::~AndroidTimeZoneImpl() {}

void AndroidTimeZoneImpl::Init(const String& iana_id) {
  JNIObjectImpl jo_ianaId = JNIObjectImpl::fromString(String::fromUtf8(iana_id));
  android_time_zone_ = JNIObjectImpl::callStaticObjectMethod("java.util.TimeZone", "getTimeZone", "(Ljava/lang/String;)Ljava/util/TimeZone;",  static_cast<jstring>(jo_ianaId.object()));

  // Painfully, JNI gives us back a default zone object if it doesn't
  // recognize the name; so check for whether iana_id is a recognized name of
  // the zone object we got and ignore the zone if not.
  // Try checking iana_id against getID(), getDisplayName():
  JNIObjectImpl jname = android_time_zone_.callObjectMethod("getID", "()Ljava/lang/String;");
  bool found = (jname.toString().toUtf8() == iana_id);
  for (int32 style = 1; !found && style-- > 0;) {
    for (int32 dst = 1; !found && dst-- > 0;) {
      jname = android_time_zone_.callObjectMethod("getDisplayName", "(ZI;)Ljava/lang/String;", bool(dst), style);
      found = (jname.toString().toUtf8() == iana_id);
    }
  }

  if (!found) {
    id_.clear();
  } else if (iana_id.IsEmpty()) {
    id_ = GetSystemTimeZoneId();
  } else {
    id_ = iana_id;
  }
}

AndroidTimeZoneImpl* AndroidTimeZoneImpl::Clone() const {
  return new AndroidTimeZoneImpl(*this);
}

String AndroidTimeZoneImpl::GetDisplayName(
      TimeZone::TimeType time_type,
      TimeZone::NameType name_type,
      const Locale& locale) const {
  String name;

  if (android_time_zone_.IsValid()) {
    jboolean daylight_time = (time_type == TimeZone::DaylightTime);  // treat TimeZone::GenericTime as TimeZone::StandardTime

    // treat all NameTypes as java TimeZone style LONG (value 1), except of course TimeZone::ShortName which is style SHORT (value 0);
    jint style = (name_type == TimeZone::ShortName ? 0 : 1);

    JNIObjectImpl jlanguage = JNIObjectImpl::fromString(Locale::LanguageToString(locale.GetLanguage()));
    JNIObjectImpl jcountry = JNIObjectImpl::fromString(Locale::CountryToString(locale.Getcountry()));
    JNIObjectImpl jvariant = JNIObjectImpl::fromString(Locale::ScriptToString(locale.GetScript()));
    JNIObjectImpl jlocale("java.util.Locale", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V", static_cast<jstring>(jlanguage.object()), static_cast<jstring>(jcountry.object()), static_cast<jstring>(jvariant.object()));

    JNIObjectImpl jname = android_time_zone_.callObjectMethod("getDisplayName", "(ZILjava/util/Locale;)Ljava/lang/String;", daylight_time, style, jlocale.object());

    name = jname.toString();
  }

  return name;
}

String AndroidTimeZoneImpl::GetAbbreviation(int64 at_msecs_since_epoch) const {
  if (IsDaylightTime(at_msecs_since_epoch)) {
    return GetDisplayName(TimeZone::DaylightTime, TimeZone::ShortName, Locale());
  } else {
    return GetDisplayName(TimeZone::StandardTime, TimeZone::ShortName, Locale());
  }
}

int32 AndroidTimeZoneImpl::GetOffsetFromUtc(int64 at_msecs_since_epoch) const {
  // GetOffsetFromUtc() is invoked when android_time_zone_ may not yet be set at startup,
  // so a validity test is needed here
  if (android_time_zone_.IsValid()) {
    // the java method getOffset() returns milliseconds, but TimeZone returns seconds
    return android_time_zone_.callMethod<jint>("getOffset", "(J)I", static_cast<jlong>(at_msecs_since_epoch)) / 1000;
  } else {
    return 0;
  }
}

int32 AndroidTimeZoneImpl::GetStandardTimeOffset(int64 at_msecs_since_epoch) const {
  // the java method does not use a reference time
  FUN_UNUSED(at_msecs_since_epoch);

  if (android_time_zone_.IsValid()) {
    // the java method getRawOffset() returns milliseconds, but TimeZone returns seconds
    return android_time_zone_.callMethod<jint>("getRawOffset") / 1000;
  } else {
    return 0;
  }
}

int32 AndroidTimeZoneImpl::GetDaylightTimeOffset(int64 at_msecs_since_epoch) const {
  return GetOffsetFromUtc(at_msecs_since_epoch) - GetStandardTimeOffset(at_msecs_since_epoch);
}

bool AndroidTimeZoneImpl::HasDaylightTime() const {
  if (android_time_zone_.IsValid()) {
    // note: the Java function only tests for future DST transtions, not past
    return android_time_zone_.callMethod<jboolean>("useDaylightTime");
  } else {
    return false;
  }
}

bool AndroidTimeZoneImpl::IsDaylightTime(int64 at_msecs_since_epoch) const {
  if (android_time_zone_.IsValid()) {
    JNIObjectImpl jDate("java/util/Date", "(J)V", static_cast<jlong>(at_msecs_since_epoch));
    return android_time_zone_.callMethod<jboolean>("inDaylightTime", "(Ljava/util/Date;)Z", jDate.object());
  } else {
    return false;
  }
}

TimeZoneImpl::Data AndroidTimeZoneImpl::GetData(int64 for_msecs_since_epoch) const {
  if (android_time_zone_.IsValid()) {
    Data data;
    data.at_msecs_since_epoch = for_msecs_since_epoch;
    data.standard_time_offset = GetStandardTimeOffset(for_msecs_since_epoch);
    data.offset_from_utc = GetOffsetFromUtc(for_msecs_since_epoch);
    data.daylight_time_offset = data.offset_from_utc - data.offset_from_utc;
    data.abbreviation = GetAbbreviation(for_msecs_since_epoch);
    return data;
  } else {
    return InvalidData();
  }
}

bool AndroidTimeZoneImpl::HasTransitions() const {
  // java.util.TimeZone does not directly provide transitions
  return false;
}

TimeZoneImpl::Data
AndroidTimeZoneImpl::NextTransition(int64 after_msecs_since_epoch) const {
  // transitions not available on Android, so return an invalid data object
  FUN_UNUSED(after_msecs_since_epoch);
  return InvalidData();
}

TimeZoneImpl::Data
AndroidTimeZoneImpl::PreviousTransition(int64 before_msecs_since_epoch) const {
  // transitions not available on Android, so return an invalid data object
  FUN_UNUSED(before_msecs_since_epoch);
  return InvalidData();
}

String AndroidTimeZoneImpl::GetSystemTimeZoneId() const {
  JNIObjectImpl android_system_time_zone = JNIObjectImpl::callStaticObjectMethod("java.util.TimeZone", "getDefault", "()Ljava/util/TimeZone;");
  JNIObjectImpl system_tz_id_android = android_system_time_zone.callObjectMethod<jstring>("getID");
  String system_tz_id = system_tz_id_android.toString().toUtf8();
  return system_tz_id;
}

Array<String> AndroidTimeZoneImpl::AvailableTimeZoneIds() const {
  Array<String> available_time_zone_id_list;
  JNIObjectImpl android_available_id_list = JNIObjectImpl::callStaticObjectMethod("java.util.TimeZone", "getAvailableIDs", "()[Ljava/lang/String;");

  QJNIEnvironmentPrivate jni_env;
  int32 android_tz_count = jni_env->GetArrayLength(static_cast<jarray>(android_available_id_list.object()));

  // need separate jobject and QAndroidJniObject here so that we can delete (DeleteLocalRef) the reference to the jobject
  // (or else the JNI reference table fills after 512 entries from GetObjectArrayElement)
  jobject android_tz_object;
  JNIObjectImpl android_tz;
  for (int32 i = 0; i < android_tz_count; ++i) {
    android_tz_object = jni_env->GetObjectArrayElement(static_cast<jobjectArray>(android_available_id_list.object()), i);
    android_tz = android_tz_object;
    available_time_zone_id_list.append(android_tz.toString().toUtf8());
    jni_env->DeleteLocalRef(android_tz_object);
  }

  return available_time_zone_id_list;
}

} // namespace fun
