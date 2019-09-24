#pragma once

#include "fun/base/base.h"
#include "fun/base/container/array.h"
//#include "fun/base/string/string.h"

namespace fun {

class FUN_BASE_API PathImpl {
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
  static String GetSystemImpl();
  static String ExpandImpl(const String& path);
  static void ListRootsImpl(Array<String>& roots);

  enum { MAX_PATH_LEN = 32767 };
};

} // namespace fun
