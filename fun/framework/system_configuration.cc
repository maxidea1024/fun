#include "fun/framework/system_configuration.h"

#include "fun/base/date_time.h"
#include "fun/base/environment.h"
#include "fun/base/path.h"

//#include "fun/base/date_time_formatter.h"
//#include "fun/base/date_time_format.h"
//#include "fun/base/number_formatter.h"

#if FUN_PLATFORM != FUN_PLATFORM_VXWORKS
#include "fun/base/process.h"
#endif
#include "fun/base/exception.h"
#include "fun/base/str.h"

#include <cstdio>

namespace fun {
namespace framework {

const String SystemConfiguration::OSNAME = "system.os_name";
const String SystemConfiguration::OSVERSION = "system.os_version";
const String SystemConfiguration::OSARCHITECTURE = "system.os_architecture";
const String SystemConfiguration::NODENAME = "system.node_name";
const String SystemConfiguration::NODEID = "system.node_id";
const String SystemConfiguration::CURRENTDIR = "system.current_dir";
const String SystemConfiguration::HOMEDIR = "system.home_dir";
const String SystemConfiguration::CONFIGHOMEDIR = "system.config_home_dir";
const String SystemConfiguration::CACHEHOMEDIR = "system.cache_home_dir";
const String SystemConfiguration::DATAHOMEDIR = "system.data_home_dir";
const String SystemConfiguration::TEMPDIR = "system.temp_dir";
const String SystemConfiguration::CONFIGDIR = "system.config_dir";
const String SystemConfiguration::DATETIME = "system.datetime";
#if FUN_PLATFORM != FUN_PLATFORM_VXWORKS
const String SystemConfiguration::PID = "system.pid";
#endif
const String SystemConfiguration::ENV = "system.env.";

SystemConfiguration::SystemConfiguration() {}

SystemConfiguration::~SystemConfiguration() {}

bool SystemConfiguration::GetRaw(const String& key, String& value) const {
  if (icompare(key, OSNAME) == 0) {
    value = Environment::GetOsName();
  } else if (icompare(key, OSVERSION) == 0) {
    value = Environment::GetOsVersion();
  } else if (icompare(key, OSARCHITECTURE) == 0) {
    value = Environment::GetOsArchitecture();
  } else if (icompare(key, NODENAME) == 0) {
    value = Environment::GetNodeName();
  } else if (icompare(key, NODEID) == 0) {
    try {
      fun::Environment::NodeId id;
      fun::Environment::GetNodeId(id);
      char result[13];
      std::sprintf(result, "%02x%02x%02x%02x%02x%02x", id[0], id[1], id[2],
                   id[3], id[4], id[5]);
      value = result;
    } catch (...) {
      value = "000000000000";
    }
  } else if (icompare(key, CURRENTDIR) == 0) {
    value = Path::Current();
  } else if (icompare(key, HOMEDIR) == 0) {
    value = Path::GetHome();
  } else if (icompare(key, CONFIGHOMEDIR) == 0) {
    value = Path::GetConfigHome();
  } else if (icompare(key, CACHEHOMEDIR) == 0) {
    value = Path::GetCacheHome();
  } else if (icompare(key, DATAHOMEDIR) == 0) {
    value = Path::GetDataHome();
  } else if (icompare(key, TEMPDIR) == 0) {
    value = Path::GetTemp();
  } else if (icompare(key, CONFIGDIR) == 0) {
    value = Path::GetConfig();
  } else if (icompare(key, DATETIME) == 0) {
    // TODO
    fun_check(0);
    // value = fun::DateTimeFormatter::Format(fun::DateTime(),
    // fun::DateTimeFormat::ISO8601_FORMAT);
  }
#if FUN_PLATFORM != FUN_PLATFORM_VXWORKS
  else if (icompare(key, PID) == 0) {
    value = "0";
    value = String::FromNumber(fun::Process::CurrentPid());
  }
#endif
  // else if (key.compare(0, ENV.Len(), ENV) == 0) {
  else if (key.StartsWith(ENV)) {
    // system.env.[env_var_name]
    return GetEnv(key.Mid(ENV.Len()), value);
  } else {
    return false;
  }

  return true;
}

void SystemConfiguration::SetRaw(const String& key, const String& value) {
  FUN_UNUSED(value);

  throw fun::InvalidAccessException("Attempt to modify a system property", key);
}

void SystemConfiguration::Enumerate(const String& key, Keys& range) const {
  if (key.IsEmpty()) {
    range.Add("system");
  } else if (icompare(key, "system") == 0) {
    range.Add(OSNAME);
    range.Add(OSVERSION);
    range.Add(OSARCHITECTURE);
    range.Add(NODENAME);
    range.Add(NODEID);
    range.Add(CURRENTDIR);
    range.Add(HOMEDIR);
    range.Add(CONFIGHOMEDIR);
    range.Add(CACHEHOMEDIR);
    range.Add(DATAHOMEDIR);
    range.Add(TEMPDIR);
    range.Add(CONFIGDIR);
    range.Add(DATETIME);
#if FUN_PLATFORM != FUN_PLATFORM_VXWORKS
    range.Add(PID);
#endif
    range.Add(ENV);
  }
}

void SystemConfiguration::RemoveRaw(const String& key) {
  FUN_UNUSED(key);

  throw NotImplementedException("Removing a key in a SystemConfiguration");
}

bool SystemConfiguration::GetEnv(const String& name, String& value) {
  if (Environment::Has(name)) {
    value = Environment::Get(name);
    return true;
  }

  return false;
}

}  // namespace framework
}  // namespace fun
