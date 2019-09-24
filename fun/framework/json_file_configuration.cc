#include "fun/framework/json_file_configuration.h"

#ifndef FUN_NO_JSONCONFIGURATION

#include "fun/base/file_stream.h"
#include "fun/base/number_parser.h"
#include "fun/base/regex.h"
#include "fun/json/parser.h"
#include "fun/json/query.h"

namespace fun {
namespace framework {

JsonFileConfiguration::JsonFileConfiguration() : object_(new JSON::Object()) {}

JsonFileConfiguration::JsonFileConfiguration(const String& path) { Load(path); }

JsonFileConfiguration::JsonFileConfiguration(std::istream& istr) { Load(istr); }

JsonFileConfiguration::JsonFileConfiguration(const JSON::Object::Ptr& object)
    : object_(object) {}

JsonFileConfiguration::~JsonFileConfiguration() {}

void JsonFileConfiguration::Load(const String& path) {
  fun::FileInputStream fis(path);
  Load(fis);
}

void JsonFileConfiguration::Load(std::istream& istr) {
  JSON::Parser parser;
  parser.parse(istr);
  DynamicAny result = parser.result();
  if (result.type() == typeid(JSON::Object::Ptr)) {
    object_ = result.extract<JSON::Object::Ptr>();
  }
}

void JsonFileConfiguration::LoadEmpty(const String& root) {
  object_ = new JSON::Object();
  JSON::Object::Ptr root_object = new JSON::Object();
  object_->Set(root, root_object);
}

bool JsonFileConfiguration::GetRaw(const String& key, String& value) const {
  JSON::Query query(object_);
  fun::dynamicAny result = query.Find(key);
  if (!result.IsEmpty()) {
    value = result.convert<String>();
    return true;
  }
  return false;
}

void JsonFileConfiguration::GetIndexes(String& name,
                                       std::vector<int>& indexes) {
  indexes.clear();

  Regex::MatchVec matches;
  int first_offset = -1;
  int offset = 0;
  Regex regex("\\[([0-9]+)\\]");
  while (regex.match(name, offset, matches) > 0) {
    if (first_offset == -1) {
      first_offset = static_cast<int>(matches[0].offset);
    }
    String num = name.Substr(matches[1].offset, matches[1].length);
    indexes.push_back(NumberParser::parse(num));
    offset = static_cast<int>(matches[0].offset + matches[0].length);
  }

  if (first_offset != -1) {
    name = name.Substr(0, first_offset);
  }
}

JSON::Object::Ptr JsonFileConfiguration::FindStart(const String& key,
                                                   String& last_part) {
  JSON::Object::Ptr current_object = object_;

  StringTokenizer tokenizer(key, ".");
  last_part = tokenizer[tokenizer.Count() - 1];

  for (int32 i = 0; i < tokenizer.Count() - 1; ++i) {
    std::vector<int> indexes;
    String name = tokenizer[i];
    GetIndexes(name, indexes);

    DynamicAny result = current_object->get(name);

    if (result.IsEmpty()) {     // Not found
      if (indexes.IsEmpty()) {  // We want an object, create it
        JSON::Object::Ptr new_object = new JSON::Object();
        current_object->Set(name, new_object);
        current_object = new_object;
      } else {  // We need an array
        JSON::Array::Ptr newArray;
        JSON::Array::Ptr parentArray;
        JSON::Array::Ptr topArray;
        for (std::vector<int>::iterator it = indexes.begin();
             it != indexes.end(); ++it) {
          newArray = new JSON::Array();
          if (topArray.isNull()) {
            topArray = newArray;
          }

          if (!parentArray.isNull()) {
            parentArray->Add(newArray);
          }

          for (int j = 0; j <= *it - 1; ++j) {
            fun::dynamicAny nullValue;
            newArray->Add(nullValue);
          }

          parentArray = newArray;
        }

        current_object->Set(name, topArray);
        current_object = new JSON::Object();
        newArray->Add(current_object);
      }
    } else {                    // We have a value
      if (indexes.IsEmpty()) {  // We want an object
        if (result.type() == typeid(JSON::Object::Ptr)) {
          current_object = result.extract<JSON::Object::Ptr>();
        } else {
          throw SyntaxException("Expected a JSON object");
        }
      } else {
        if (result.type() == typeid(JSON::Array::Ptr)) {
          JSON::Array::Ptr arr = result.extract<JSON::Array::Ptr>();

          for (std::vector<int>::iterator it = indexes.begin();
               it != indexes.end() - 1; ++it) {
            JSON::Array::Ptr currentArray = arr;
            arr = arr->GetArray(*it);
            if (arr.isNull()) {
              arr = new JSON::Array();
              currentArray->Add(arr);
            }
          }

          result = arr->get(*indexes.rbegin());
          if (result.IsEmpty()) {  // Index doesn't exist
            JSON::Object::Ptr new_object = new JSON::Object();
            arr->Add(new_object);
            current_object = new_object;
          } else {  // Index is available
            if (result.type() == typeid(JSON::Object::Ptr)) {
              current_object = result.extract<JSON::Object::Ptr>();
            } else {
              throw SyntaxException("Expected a JSON object");
            }
          }
        } else {
          throw SyntaxException("Expected a JSON array");
        }
      }
    }
  }

  return current_object;
}

void JsonFileConfiguration::SetValue(const String& key,
                                     const fun::dynamicAny& value) {
  String sValue;

  value.convert<String>(sValue);
  KeyValue kv(key, sValue);

  if (eventsEnabled()) {
    propertyChanging(this, kv);
  }

  String last_part;
  JSON::Object::Ptr parent_object = FindStart(key, last_part);

  std::vector<int> indexes;
  GetIndexes(last_part, indexes);

  if (indexes.IsEmpty()) {  // No Array
    parent_object->Set(last_part, value);
  } else {
    DynamicAny result = parent_object->get(last_part);
    if (result.IsEmpty()) {
      result = JSON::Array::Ptr(new JSON::Array());
      parent_object->Set(last_part, result);
    } else if (result.type() != typeid(JSON::Array::Ptr)) {
      throw SyntaxException("Expected a JSON array");
    }

    JSON::Array::Ptr arr = result.extract<JSON::Array::Ptr>();
    for (std::vector<int>::iterator it = indexes.begin();
         it != indexes.end() - 1; ++it) {
      JSON::Array::Ptr next_array = arr->GetArray(*it);
      if (next_array.isNull()) {
        for (int i = static_cast<int>(arr->size()); i <= *it; ++i) {
          fun::dynamicAny nullValue;
          arr->Add(nullValue);
        }
        next_array = new JSON::Array();
        arr->Add(next_array);
      }
      arr = next_array;
    }
    arr->Set(indexes.back(), value);
  }

  if (eventsEnabled()) {
    propertyChanged(this, kv);
  }
}

void JsonFileConfiguration::SetString(const String& key, const String& value) {
  SetValue(key, value);
}

void JsonFileConfiguration::SetRaw(const String& key, const String& value) {
  SetValue(key, value);
}

void JsonFileConfiguration::SetInt(const String& key, int value) {
  SetValue(key, value);
}

void JsonFileConfiguration::SetBool(const String& key, bool value) {
  SetValue(key, value);
}

void JsonFileConfiguration::SetDouble(const String& key, double value) {
  SetValue(key, value);
}

void JsonFileConfiguration::Enumerate(const String& key, Keys& range) const {
  JSON::Query query(object_);
  fun::dynamicAny result = query.Find(key);
  if (result.type() == typeid(JSON::Object::Ptr)) {
    JSON::Object::Ptr object = result.extract<JSON::Object::Ptr>();
    object->getNames(range);
  }
}

void JsonFileConfiguration::Save(std::ostream& ostr,
                                 unsigned int indent) const {
  object_->stringify(ostr, indent);
}

void JsonFileConfiguration::RemoveRaw(const String& key) {
  String last_part;
  JSON::Object::Ptr parent_object = FindStart(key, last_part);
  std::vector<int> indexes;
  GetIndexes(last_part, indexes);

  if (indexes.IsEmpty()) {  // No Array
    parent_object->Remove(last_part);
  } else {
    DynamicAny result = parent_object->get(last_part);
    if (!result.IsEmpty() && result.type() == typeid(JSON::Array::Ptr)) {
      JSON::Array::Ptr arr = result.extract<JSON::Array::Ptr>();
      for (std::vector<int>::iterator it = indexes.begin();
           it != indexes.end() - 1; ++it) {
        arr = arr->GetArray(*it);
      }
      arr->Remove(indexes.back());
    }
  }
}

}  // namespace framework
}  // namespace fun

#endif  // FUN_NO_JSONCONFIGURATION
