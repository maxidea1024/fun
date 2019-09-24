#pragma once

namespace fun {
namespace sql {
namespace odbc {

/**
 * Utility function for conversion from UTF-8 to UTF-16
 */
void makeUTF16(SQLCHAR* pSQLChar, SQLINTEGER length, String& target);

/**
 * Utility function for conversion from UTF-8 to UTF-16.
 */
inline void makeUTF16(SQLCHAR* pSQLChar, SQLSMALLINT length, String& target)
{
  makeUTF16(pSQLChar, (SQLINTEGER) length, target);
}

/**
 * Utility function for conversion from UTF-16 to UTF-8.
 */
void makeUTF8(fun::Buffer<SQLWCHAR>& buffer, SQLINTEGER length, SQLPOINTER pTarget, SQLINTEGER targetLength);

} // namespace odbc
} // namespace sql
} // namespace fun
