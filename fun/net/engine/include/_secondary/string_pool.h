//@deprecated
#pragma once

namespace fun {
namespace net {

class PooledString;

class StringPool : protected Singleton<StringPool>
{
 public:
  NETENGINE_API static StringPool& Get();

  NETENGINE_API StringPool();
  NETENGINE_API ~StringPool();

  NETENGINE_API const char* Mapping(const String& str);
  NETENGINE_API String DumpStatus();

 private:
  friend class PooledString;

  typedef Map<String, int32> MapType;

  MapType map_;
  CCriticalSection2 mutex_;
};

} // namespace net
} // namespace fun
