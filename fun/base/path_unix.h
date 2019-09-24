#pragma once

#include "fun/base/base.h"
#include "fun/base/container/array.h"

namespace fun {

class PathImpl {
 public:
  static String GetCurrentImpl();
  static String GetHomeImpl();
  static String GetConfigHomeImpl();
  static String GetDataHomeImpl();
  static String GetCacheHomeImpl();
  static String GetTempImpl();
  static String GetSelfImpl();
  static String GetConfigImpl();
  static String GetNullImpl();
  static String ExpandImpl(const String& path);
  static void ListRootsImpl(Array<String>& roots);
};

} // namespace fun
