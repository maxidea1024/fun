#include "fun/redis/builders/builder_factory.h"
#include "fun/redis/builders/simple_string_builder.h"
#include "fun/redis/builders/error_builder.h"
#include "fun/redis/builders/integer_builder.h"
#include "fun/redis/builders/bulk_string_builder.h"
#include "fun/redis/builders/array_builder.h"

namespace fun {
namespace redis {

UniquePtr<IBuilder> Builders::CreateBuilder(char resp_tag)
{
  switch (resp_tag) {
  case '+': return UniquePtr<SimpleStringBuilder>(new SimpleStringBuilder);
  case '-': return UniquePtr<ErrorBuilder>(new ErrorBuilder);
  case ':': return UniquePtr<IntegerBuilder>(new IntegerBuilder);
  case '$': return UniquePtr<BulkStringBuilder>(new BulkStringBuilder);
  case '*': return UniquePtr<ArrayBuilder>(new ArrayBuilder);
  default:
    //TODO throw exception
    //"invalid data"
    return nullptr;
  }
}

} // namespace redis
} // namespace fun
