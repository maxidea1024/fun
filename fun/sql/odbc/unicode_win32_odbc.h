#pragma once

namespace fun {
namespace sql {
namespace odbc {

/**
 * Utility function for conversion from UTF-8 to UTF-16
 */
inline void makeUTF16(SQLCHAR* pSQLChar, SQLINTEGER length,
                      std::wstring& target) {
  int len = length;
  if (SQL_NTS == len) {
    len = (int)std::strlen((const char*)pSQLChar);
  }

  UnicodeConverter::toUTF16((const char*)pSQLChar, len, target);
}

/**
 * Utility function for conversion from UTF-16 to UTF-8. Length is in bytes.
 */
inline void makeUTF8(fun::Buffer<wchar_t>& buffer, SQLINTEGER length,
                     SQLPOINTER pTarget, SQLINTEGER targetLength) {
  if (buffer.sizeBytes() < length) {
    throw InvalidArgumentException(
        "Specified length exceeds available length.");
  } else if ((length % 2) != 0) {
    throw InvalidArgumentException("Length must be an even number.");
  }

  length /= sizeof(wchar_t);
  String result;
  UnicodeConverter::toUTF8(buffer.begin(), length, result);

  UnsafeMemory::Memset(pTarget, 0, targetLength);
  std::strncpy((char*)pTarget, result.c_str(),
               result.size() < targetLength ? result.size() : targetLength);
}

}  // namespace odbc
}  // namespace sql
}  // namespace fun
