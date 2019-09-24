#pragma once

#include "fun/base/base.h"
#include "fun/framework/argumentmap.h"

namespace fun {
namespace framework {

class Component : public Noncopyable {
 public:
  static const ArgumentMap* FindArgument(const String& component_name);

 private:
  Component();
};


} // namespace framework
} // namespace fun


#define FUN_REGISTER_COMPONENT(ComponentClass, InstallerClass) \
  extern "C" bool ComponentClass ## _Install(const fun::framework::ArgumentMap& args) { \
    return InstallerClass::Install(args); \
  } \
  extern "C" bool ComponentClass ## _Unstall() { \
    return InstallerClass::Uninstall(); \
  }

#define FUN_REGISTER_STARTABLE_COMPONENT(ComponentClass, InstallerClass) \
  FUN_REGISTER_COMPONENT(ComponentClass, InstallerClass) \
  extern "C" bool ComponentClass ## _Start() { \
    InstallerClass::Start(); \
  }

#define FUN_REGISTER_STARTABLE_COMPONENT2(ComponentClass, InstallerClass) \
  FUN_REGISTER_STARTABLE_COMPONENT(ComponentClass, InstallerClass) \
  extern "C" bool ComponentClass ## _Stop() { \
    InstallerClass::Stop(); \
  }
