#pragma once

namespace fun {
namespace json {

class JValue;

typedef Array<JValue> JArray;
typedef Map<String, JValue> JObject;

enum class ValueType;
enum class CommentPlacement;
class JValue;
class Reader;
class IWriter;
class CondensedWriter;
class PrettyWriter;
class Features;

}  // namespace json
}  // namespace fun
