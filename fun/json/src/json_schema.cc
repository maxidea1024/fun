#include "fun/json/schema.h"

namespace fun {
namespace json {

/*

"BidrectionalMessage":{
  "Direction":"c2s s2c",
  "Fields":{
    "RequestString":{
      "type":"string",
      "required":"c2s s2c"
    },
    "ReplyString":{
      "type":"string",
      "required":"s2c
    }
  }
}

"Test":{
  "Direction":"c2s s2c",
  "Elements":[
    { "type":"string", "required":true }
  ]
}

*/

Schema::Schema() {
  // TODO
}

bool Schema::ValidateJson(const JValue& json) const {
  // TODO
  return false;
}

}  // namespace json
}  // namespace fun
