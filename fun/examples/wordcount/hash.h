#pragma once

namespace boost
{
std::size_t hash_value(const String& x);
}

#include <boost/unordered_map.hpp>

namespace boost
{
inline std::size_t hash_value(const String& x)
{
  return hash_range(x.begin(), x.end());
}
// template <> struct hash<String>
// {
//   std::size_t operator()(const String& v) const
//   {
//     return hash_value(v);
//   }
// };
}

typedef boost::unordered_map<String, int64_t> WordCountMap;
