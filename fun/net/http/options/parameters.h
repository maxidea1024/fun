#pragma once

#include "fun/http/http.h"
#include <initializer_list>
#include <memory>


namespace fun {
namespace http {


class Parameter
{
public:
  template <typename KeyType, typename ValueType>
  Parameter(KeyType&& key, ValueType&& value)
    : key{FORWARD(key)}
    , value{FORWARD(value)}
  {
  }

  /// Name of parameter
  String key;
  /// value of parameter
  String value;
};


class Parameters
{
public:
  Parameters() = default;

  Parameters(const std::initializer_list<Parameter>& args)
  {
    for (const auto& param : args)
    {
      AddParameter(param);
    }
  }

  void AddParameter(const Parameter& param)
  {
    if (!content.IsEmpty())
    {
      content += '&';
    }

    const auto escaped_key = Uri::Encode(param.key);
    if (param.value.IsEmpty())
    {
      content += escaped_key;
    }
    else
    {
      const auto escaped_value = Uri::Encode(param.value);
      content += escaped_key + '=' + escaped_value;
    }
  }

  /// Combined content
  String content;
};


} // namespace http
} // namespace fun
