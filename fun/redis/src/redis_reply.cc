//TODO inline 형태로 처리하자.

#include "fun/redis/reply.h"

namespace fun {

//TODO 모두 inline으로 처리하는게 바람직해보임.

namespace redis {

Reply::Reply() : type_(Type::Nil) {}

Reply::Reply(const String& value, StringType reply_type)
  : type_(static_cast<Type>(reply_type)),
    string_value_(value) {}

Reply::Reply(int64 value)
  : type_(Type::Integer),
    integer_value_(value) {}

Reply::Reply(const TArray<Reply>& rows)
  : type_(Type::Array),
    rows_(rows) {}

bool Reply::IsArray() const {
  return type_ == Type::Array;
}

bool Reply::IsString() const {
  return IsSimpleString() || IsBulkString();
}

bool Reply::IsSimpleString() const {
  return type_ == Type::SimpleString;
}

bool Reply::IsBulkString() const {
  return type_ == Type::BulkString;
}

bool Reply::IsError() const {
  return type_ == Type::Error;
}

bool Reply::IsInteger() const {
  return type_ == Type::Integer;
}

bool Reply::IsNil() const {
  return type_ == Type::Nil;
}

bool Reply::Ok() const {
  return type_ != Type::Error;
}

const String& Reply::Error() const {
  fun_check(IsError()); //TODO 예외를 던지는 형태로 변경하도록 하자.

  return AsString();
}

const TArray<Reply>& Reply::AsArray() const {
  fun_check(IsArray()); //TODO 예외를 던지는 형태로 변경하도록 하자.

  return rows_;
}

const String& Reply::AsString() const {
  fun_check(IsString()); //TODO 예외를 던지는 형태로 변경하도록 하자.

  return string_value_;
}

int64 Reply::AsInteger() const {
  fun_check(IsInteger()); //TODO 예외를 던지는 형태로 변경하도록 하자.

  //TODO 타입을 스트링으로 변환하는 기능이 아직 없음.
  //TODO 타입 미스매칭은 별도로 처리하는게 바람직해보임.

  //if (!IsInteger()) {
  //  throw RedisException(String::Format("integer 타입이 요구되는데, {0} 타입입니다."));
  //}

  return integer_value_;
}

void Reply::Set() {
  type_ = Type::Nil;
}

void Reply::Set(const String& value, StringType reply_type) {
  type_ = static_cast<Type>(reply_type);
  string_value_ = value;
}

void Reply::Set(int64 value) {
  type_ = Type::Integer;
  integer_value_ = value;
}

void Reply::Set(const TArray<Reply>& rows) {
  type_ = Type::Array;
  rows_ = rows;
}

Reply& Reply::operator << (const Reply& reply) {
  type_ = Type::Array;
  rows_.Add(reply);
  return *this;
}

Reply::Type Reply::GetType() const {
  return type_;
}

const String Reply::GetTypeName() const {
  switch (type_) {
    case Type::Error: return StringLiteral("Error");
    case Type::BulkString: return StringLiteral("BulkString");
    case Type::SimpleString: return StringLiteral("SimpleString");
    case Type::Nil: return StringLiteral("Nil");
    case Type::Integer: return StringLiteral("Integer");
    case Type::Array: return StringLiteral("Array");
  }
  return StringLiteral("Unknown");
}

const String Reply::ToDiagnosticString() const {
  String string;

  switch (type_) {
    case Type::Error:
      string << Error();
      break;
    case Type::BulkString:
      string << AsString();
      break;
    case Type::SimpleString:
      string << AsString();
    case Type::Nil:
      string << StringLiteral("(nil)");
      break;
    case Type::Integer:
      string << String::FromNumber(AsInteger());
      break;
    case Type::Array:
      for (const auto& element : AsArray()) {
        string << element.ToDiagnosticString();
      }
      break;
  }

  return string;
}

} // namespace redis
} // namespace fun
