#include "fun/net/message/generated_struct.h"

namespace fun {
namespace net {

FUN_IMPLEMENT_RUNTIME_CLASS(GeneratedStruct);

const char* GeneratedStruct::GetTypeName() {
  return StaticTypeName();
}

void GeneratedStruct::Clear() {
  //@todo
}

bool GeneratedStruct::IsDefaults() const {
  //@todo
  return true;
}

void GeneratedStruct::Write(IMessageOut& output) const {
  // do nothing by default..
}

bool GeneratedStruct::Read(IMessageIn& input) {
  // do nothing by default..
  return true;
}

int32 GeneratedStruct::GetByteLength() const {
  return -1;
}

String GeneratedStruct::ToString() const {
  return "{}";
}

} // namespace net
} // namespace fun
