#ifdef __GNUC__
// TODO: determine g++ version able to do the right thing without these specializations

#include "fun/sql/dynamic_lob.h"
#include "fun/sql/lob.h"
#include "fun/base/dynamic/var.h"

namespace fun {
namespace Dynamic {

using fun::sql::CLOB;
using fun::sql::BLOB;

template <>
Var::operator CLOB () const {
  VarHolder* holder = GetContent();

  if (!holder) {
    throw InvalidAccessException("Can not convert empty value.");
  }

  if (typeid(CLOB) == holder->Type()) {
    return Extract<CLOB>();
  } else {
    String result;
    holder->Convert(result);
    return CLOB(result);
  }
}

template <>
Var::operator BLOB () const {
  VarHolder* holder = GetContent();

  if (!holder) {
    throw InvalidAccessException("Can not convert empty value.");
  }

  if (typeid(BLOB) == holder->Type()) {
    return Extract<BLOB>();
  } else {
    String result;
    holder->Convert(result);
    return BLOB(reinterpret_cast<const unsigned char*>(result.data()), result.size());
  }
}

} // namespace sql
} // namespace fun

#endif // __GNUC__
