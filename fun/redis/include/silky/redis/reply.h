#pragma once

#include "fun/redis/redis.h"

namespace fun {
namespace redis {

class FUN_REDIS_API Reply {
 public:
  enum class Type {
    Error = 0,
    BulkString = 1,
    SimpleString = 2,
    Nil = 3,
    Integer = 4,
    Array = 5,
  };

  enum class StringType {
    Error = 0,
    BulkString = 1,
    SimpleString = 2,
  };

  Reply();
  Reply(const String& value, StringType reply_type);
  Reply(int64 value);
  Reply(const TArray<Reply>& rows);

  ~Reply() = default;
  Reply(const Reply&) = default;
  Reply& operator=(const Reply&) = default;

  bool IsArray() const;
  bool IsString() const;
  bool IsSimpleString() const;
  bool IsBulkString() const;
  bool IsError() const;
  bool IsInteger() const;
  bool IsNil() const;

  bool Ok() const;
  const String& Error() const;

  const TArray<Reply>& AsArray() const;
  const String& AsString() const;
  int64 AsInteger() const;

  void Set();
  void Set(const String& value, StringType reply_type);
  void Set(int64 value);
  void Set(const TArray<Reply>& rows);
  Reply& operator<<(const Reply& reply);

  Type GetType() const;
  const String GetTypeName() const;

  const String ToDiagnosticString() const;

 private:
  Type type_;
  TArray<Reply> rows_;
  String string_value_;
  int64 integer_value_;
};

}  // namespace redis
}  // namespace fun
