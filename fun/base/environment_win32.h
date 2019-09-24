#pragma once

#include "fun/base/base.h"

namespace fun {

class FUN_BASE_API EnvironmentImpl {
 public:
  /** Ehternet address. */
  typedef uint8 NodeId[6];

  static String GetImpl(const String& name);
  static bool HasImpl(const String& name);
  static void SetImpl(const String& name, const String& value);
  static String GetOsNameImpl();
  static String GetOsDisplayNameImpl();
  static String GetOsVersionImpl();
  static String GetOsArchitectureImpl();
  static String GetNodeNameImpl();
  static void GetNodeIdImpl(NodeId& id);
  static uint32 GetProcessorCountImpl();
};

}  // namespace fun
