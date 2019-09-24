#pragma once

#include "fun/base/any.h"
#include "fun/base/dynamic_any.h"
#include "fun/sql/binder_base.h"
#include "fun/sql/lob.h"
#include "fun/sql/sqlite/sqlite.h"
#include "sqlite3.h"

namespace fun {
namespace sql {
namespace sqlite {

/**
 * Binds placeholders in the sql query to the provided values. Performs data
 * types mapping.
 */
class FUN_SQLITE_API Binder : public fun::sql::BinderBase {
 public:
  /**
   * Creates the Binder.
   */
  Binder(sqlite3_stmt* stmt);

  /**
   * Destroys the Binder.
   */
  ~Binder();

  void Bind(size_t pos, const int8& val, Direction dir,
            const WhenNullCb& null_cb);
  void Bind(size_t pos, const uint8& val, Direction dir,
            const WhenNullCb& null_cb);
  void Bind(size_t pos, const int16& val, Direction dir,
            const WhenNullCb& null_cb);
  void Bind(size_t pos, const uint16& val, Direction dir,
            const WhenNullCb& null_cb);
  void Bind(size_t pos, const int32& val, Direction dir,
            const WhenNullCb& null_cb);
  void Bind(size_t pos, const uint32& val, Direction dir,
            const WhenNullCb& null_cb);
  void Bind(size_t pos, const int64& val, Direction dir,
            const WhenNullCb& null_cb);
  void Bind(size_t pos, const uint64& val, Direction dir,
            const WhenNullCb& null_cb);

#ifndef FUN_LONG_IS_64_BIT
  void Bind(size_t pos, const long& val, Direction dir,
            const WhenNullCb& null_cb);
  void Bind(size_t pos, const unsigned long& val, Direction dir,
            const WhenNullCb& null_cb);
#endif

  void Bind(size_t pos, const bool& val, Direction dir,
            const WhenNullCb& null_cb);
  void Bind(size_t pos, const float& val, Direction dir,
            const WhenNullCb& null_cb);
  void Bind(size_t pos, const double& val, Direction dir,
            const WhenNullCb& null_cb);
  void Bind(size_t pos, const char& val, Direction dir,
            const WhenNullCb& null_cb);
  void Bind(size_t pos, const char* const& pVal, Direction dir,
            const WhenNullCb& null_cb);
  void Bind(size_t pos, const String& val, Direction dir,
            const WhenNullCb& null_cb);
  void Bind(size_t pos, const fun::sql::BLOB& val, Direction dir,
            const WhenNullCb& null_cb);
  void Bind(size_t pos, const fun::sql::CLOB& val, Direction dir,
            const WhenNullCb& null_cb);
  void Bind(size_t pos, const Date& val, Direction dir,
            const WhenNullCb& null_cb);
  void Bind(size_t pos, const Time& val, Direction dir,
            const WhenNullCb& null_cb);
  void Bind(size_t pos, const DateTime& val, Direction dir,
            const WhenNullCb& null_cb);
  void Bind(size_t pos, const NullData& val, Direction dir,
            const std::type_info& bind_type);

 private:
  /**
   * Checks the SQLite return code and throws an appropriate exception
   * if error has occurred.
   */
  void CheckReturn(int rc);

  template <typename T>
  void BindLOB(size_t pos, const fun::sql::LOB<T>& val, Direction dir,
               const WhenNullCb& null_cb) {
    // convert a blob to a an unsigned char* array
    const T* data = reinterpret_cast<const T*>(val.GetRawContent());
    int data_len = static_cast<int>(val.size());

    int rc =
        sqlite3_bind_blob(stmt_, static_cast<int>(pos), data, data_len,
                          SQLITE_STATIC);  // no deep copy, do not free memory
    CheckReturn(rc);
  }

  sqlite3_stmt* stmt_;
};

//
// inlines
//

inline void Binder::Bind(size_t pos, const int8& val, Direction dir,
                         const WhenNullCb& null_cb) {
  int32 tmp = val;
  Bind(pos, tmp, dir, null_cb);
}

inline void Binder::Bind(size_t pos, const uint8& val, Direction dir,
                         const WhenNullCb& null_cb) {
  int32 tmp = val;
  Bind(pos, tmp, dir, null_cb);
}

inline void Binder::Bind(size_t pos, const int16& val, Direction dir,
                         const WhenNullCb& null_cb) {
  int32 tmp = val;
  Bind(pos, tmp, dir, null_cb);
}

inline void Binder::Bind(size_t pos, const uint16& val, Direction dir,
                         const WhenNullCb& null_cb) {
  int32 tmp = val;
  Bind(pos, tmp, dir, null_cb);
}

inline void Binder::Bind(size_t pos, const uint32& val, Direction dir,
                         const WhenNullCb& null_cb) {
  int32 tmp = static_cast<int32>(val);
  Bind(pos, tmp, dir, null_cb);
}

inline void Binder::Bind(size_t pos, const uint64& val, Direction dir,
                         const WhenNullCb& null_cb) {
  int64 tmp = static_cast<int64>(val);
  Bind(pos, tmp, dir, null_cb);
}

inline void Binder::Bind(size_t pos, const bool& val, Direction dir,
                         const WhenNullCb& null_cb) {
  int32 tmp = (val ? 1 : 0);
  Bind(pos, tmp, dir, null_cb);
}

inline void Binder::Bind(size_t pos, const float& val, Direction dir,
                         const WhenNullCb& null_cb) {
  double tmp = val;
  Bind(pos, tmp, dir, null_cb);
}

inline void Binder::Bind(size_t pos, const char& val, Direction dir,
                         const WhenNullCb& null_cb) {
  int32 tmp = val;
  Bind(pos, tmp, dir, null_cb);
}

inline void Binder::Bind(size_t pos, const char* const& pVal, Direction dir,
                         const WhenNullCb& null_cb) {
  String val(pVal);
  Bind(pos, val, dir, null_cb);
}

inline void Binder::Bind(size_t pos, const fun::sql::BLOB& val, Direction dir,
                         const WhenNullCb& null_cb) {
  BindLOB<fun::sql::BLOB::ValueType>(pos, val, dir, null_cb);
}

inline void Binder::Bind(size_t pos, const fun::sql::CLOB& val, Direction dir,
                         const WhenNullCb& null_cb) {
  BindLOB<fun::sql::CLOB::ValueType>(pos, val, dir, null_cb);
}

}  // namespace sqlite
}  // namespace sql
}  // namespace fun
