#pragma once

#include "fun/base/container/array.h"
#include "fun/http/http.h"

namespace fun {
namespace http {

class QueryParam {
 public:
  String name;
  String value;

 public:
  QueryParam() : name(), value() {}

  QueryParam(const String& name, const String& value)
      : name(name), value(value) {}
};

class Request {
 public:
  Version version;
  Method method;
  String base_uri;
  String uri;
  Headers headers;
  Array<QueryParam> query_params;
  ContentType content_type;
  int32 content_length;
  String content;
};

}  // namespace http
}  // namespace fun
