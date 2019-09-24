#pragma once

#include "fun/sql/odbc/odbc.h"

#ifdef FUN_PLATFORM_WINDOWS_FAMILY
#include <windows.h>
#endif

#include <sqltypes.h>

namespace fun {
namespace sql {
namespace odbc {

/**
 * ODBC environment handle class
 */
class FUN_ODBC_API EnvironmentHandle {
 public:
  /**
   * Creates the EnvironmentHandle.
   */
  EnvironmentHandle();

  /**
   * Creates the EnvironmentHandle which doesn't own the handle
   */
  explicit EnvironmentHandle(const SQLHENV* henv);

  /**
   * Destroys the EnvironmentHandle.
   */
  ~EnvironmentHandle();

  /**
   * Const conversion operator into reference to native type.
   */
  operator const SQLHENV& () const;

  /**
   * Returns const reference to handle.
   */
  const SQLHENV& GetHandle() const;

 private:
  /**
   * Conversion operator into reference to native type.
   */
  operator SQLHENV& ();

  void Init();

  EnvironmentHandle(const EnvironmentHandle&) = delete;
  const EnvironmentHandle& operator=(const EnvironmentHandle&) = delete;

  SQLHENV henv_;
  bool is_owner_;
};


//
// inlines
//

inline EnvironmentHandle::operator const SQLHENV& () const {
  return GetHandle();
}

inline const SQLHENV& EnvironmentHandle::GetHandle() const {
  return henv_;
}

inline EnvironmentHandle::operator SQLHENV& () {
  return henv_;
}

} // namespace odbc
} // namespace sql
} // namespace fun
