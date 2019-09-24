#pragma once

#include "fun/base/runtime_class.h"
#include "fun/net/message/message.h"

namespace fun {
namespace net {

/*
#define FUN_DECLARE_GENERATED_STRUCT(struct_name) \

동적으로 생성이 가능하다면, 더 좋을듯 한데...??
팩토리 지원 같은...
*/

//문자열로 익스포트/임포트 가능하게 하는건 어떨까?
//아니면, json, xml 같은 형태로 뽑아 내는것도 좋을듯 한데...
//더 필요한 부분이 있다면, 심도 있게 고민해보도록 하자..
class FUN_NET_API GeneratedStruct : public RuntimeObject {
 public:
  FUN_DECLARE_RUNTIME_CLASS(GeneratedStruct, RuntimeObject);

  static const char* StaticTypeName() { return "GeneratedStruct"; }
  virtual const char* GetTypeName();

 public:
  GeneratedStruct() {}
  virtual ~GeneratedStruct() {}

  /**
   * Clear all fields of the message and set them to their default values.
   * Clear() avoids freeing memory, assuming that any memory allocated
   * to hold parts of the message will be needed again to hold the next
   * message.  If you actually want to free the memory used by a Message,
   * you must delete it.
   */
  virtual void Clear();

  virtual bool IsDefaults() const;

  virtual void Write(IMessageOut& output) const;

  virtual bool Read(IMessageIn& input);

  virtual int32 GetByteLength() const;

  virtual String ToString() const;
};

FUN_ALWAYS_INLINE String ToString(const GeneratedStruct& value) {
  return value.ToString();
}

FUN_ALWAYS_INLINE String ToString(const GeneratedStruct* value) {
  return value ? value->ToString() : "<null>";
}

}  // namespace net
}  // namespace fun
