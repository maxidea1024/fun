#pragma once

#include "fun/http/http.h"
#include <initializer_list>
#include <memory>


namespace fun {
namespace http {


class CPair
{
public:
  template <
    typename KeyType, typename ValueType,
    typename std::enable_if<!std::is_integral<ValueType>::value, bool>::type = true
    >
  CPair(KeyType&& key, ValueType&& value)
    : key{FORWARD(key)}
    , value{FORWARD(value)}
  {
  }

  template <typename KeyType>
  CPair(KeyType&& key, const int32& value)
    : key{FORWARD(key)}
    , value{std::to_string(value)}
  {
  }

  /// key of pair
  String key;
  /// value of pair
  String value;
};


class Payload
{
public:
  template <typename ForwardIt>
  Payload(const ForwardIt begin, const ForwardIt end)
  {
    for (it pair = begin; pair != end; ++pair)
    {
      AddPair(*pair);
    }
  }

  Payload(const std::initializer_list<CPair>& pairs)
    : Payload(begin(pairs), end(pairs))
  {
  }

  void AddPair(const CPair& pair)
  {
    if (!content.IsEmpty())
    {
      content += '&';
    }

    auto escaped = Uri::Encode(pair.value); //TODO 불필요한 String <-> CString이 발생함.
    content += pair.key + '=' + escaped;
  }

  /// Combined content
  String content;
};


} // namespace http
} // namespace fun
