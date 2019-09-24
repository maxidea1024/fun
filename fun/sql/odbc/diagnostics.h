#pragma once

#include "fun/sql/odbc/odbc.h"
#include <vector>
#include <cstring>

#ifdef FUN_PLATFORM_WINDOWS_FAMILY
#include <windows.h>
#endif

#include <sqlext.h>

namespace fun {
namespace sql {
namespace odbc {

/**
 * Utility class providing functionality for retrieving ODBC diagnostic
 * records. Diagnostics object must be created with corresponding handle
 * as constructor argument. During construction, diagnostic records fields
 * are populated and the object is ready for querying.
 */
template <typename H, SQLSMALLINT handle_type>
class Diagnostics {
 public:
  static const unsigned int SQL_STATE_SIZE = SQL_SQLSTATE_SIZE + 1;
  static const unsigned int SQL_MESSAGE_LENGTH = SQL_MAX_MESSAGE_LENGTH + 1;
  static const unsigned int SQL_NAME_LENGTH = 128;
  static const String  DATA_TRUNCATED;

  // SQLGetDiagRec fields
  struct DiagnosticFields {
    SQLCHAR sql_state[SQL_STATE_SIZE];
    SQLCHAR message[SQL_MESSAGE_LENGTH];
    SQLINTEGER native_error;
  };

  typedef std::vector<DiagnosticFields> FieldVec;
  typedef typename FieldVec::const_iterator Iterator;

  /**
   * Creates and initializes the Diagnostics.
   */
  explicit Diagnostics(const H& handle) {
    UnsafeMemory::Memset(connection_name_, 0, sizeof(connection_name_));
    UnsafeMemory::Memset(server_name_, 0, sizeof(server_name_));
    diagnostics(handle);
  }

  /**
   * Destroys the Diagnostics.
   */
  ~Diagnostics() {}

  /**
   * Returns sql state.
   */
  String sqlState(int index) const {
    fun_check(index < count());
    return String((char*) fields_[index].sql_state);
  }

  /**
   * Returns error message.
   */
  String message(int index) const {
    fun_check(index < count());
    return String((char*) fields_[index].message);
  }

  /**
   * Returns native error code.
   */
  long nativeError(int index) const {
    fun_check(index < count());
    return fields_[index].native_error;
  }

  /**
   * Returns the connection name.
   * If there is no active connection, connection name defaults to NONE.
   * If connection name is not applicable for query context (such as when querying environment handle),
   * connection name defaults to NOT_APPLICABLE.
   */
  String GetConnectionName() const {
    return String((char*)connection_name_);
  }

  /**
   * Returns the server name.
   * If the connection has not been established, server name defaults to NONE.
   * If server name is not applicable for query context (such as when querying environment handle),
   * connection name defaults to NOT_APPLICABLE.
   */
  String GetServerName() const {
    return String((char*)server_name_);
  }

  /**
   * Returns the number of contained diagnostic records.
   */
  int Count() const {
    return (int) fields_.size();
  }

  /**
   * Resets the diagnostic fields container.
   */
  void Reset() {
    fields_.clear();
  }

  const FieldVec& fields() const {
    return fields_;
  }

  Iterator Begin() const {
    return fields_.begin();
  }

  Iterator End() const {
    return fields_.end();
  }

  const Diagnostics& diagnostics(const H& handle) {
    DiagnosticFields df;
    SQLSMALLINT count = 1;
    SQLSMALLINT messageLength = 0;
    static const String none = "None";
    static const String na = "Not applicable";

    reset();

    while (!Utility::IsError(SQLGetDiagRec( handle_type,
                                            handle,
                                            count,
                                            df.sql_state,
                                            &df.native_error,
                                            df.message,
                                            SQL_MESSAGE_LENGTH,
                                            &messageLength))) {
      if (1 == count) {
        // success of the following two calls is optional
        // (they fail if connection has not been established yet
        //  or return empty string if not applicable for the context)
        if (Utility::IsError(SQLGetDiagField( handle_type,
                                              handle,
                                              count,
                                              SQL_DIAG_CONNECTION_NAME,
                                              connection_name_,
                                              sizeof(connection_name_),
                                              &messageLength))) {
          size_t len = sizeof(connection_name_) > none.length() ?
            none.length() : sizeof(connection_name_) - 1;
          UnsafeMemory::Memcpy(connection_name_, none.c_str(), len);

        } else if (0 == connection_name_[0]) {
          size_t len = sizeof(connection_name_) > na.length() ?
            na.length() : sizeof(connection_name_) - 1;
          UnsafeMemory::Memcpy(connection_name_, na.c_str(), len);
        }

        if (Utility::IsError(SQLGetDiagField(handle_type,
                                              handle,
                                              count,
                                              SQL_DIAG_SERVER_NAME,
                                              server_name_,
                                              sizeof(server_name_),
                                              &messageLength))) {
          size_t len = sizeof(server_name_) > none.length() ?
            none.length() : sizeof(server_name_) - 1;
          UnsafeMemory::Memcpy(server_name_, none.c_str(), len);

        } else if (0 == server_name_[0]) {
          size_t len = sizeof(server_name_) > na.length() ?
            na.length() : sizeof(server_name_) - 1;
          UnsafeMemory::Memcpy(server_name_, na.c_str(), len);
        }
      }

      fields_.push_back(df);

      UnsafeMemory::Memset(df.sql_state, 0, SQL_STATE_SIZE);
      UnsafeMemory::Memset(df.message, 0, SQL_MESSAGE_LENGTH);
      df.native_error = 0;

      ++count;
    }

    return *this;
  }

 private:
  Diagnostics();

  /** SQLGetDiagField fields */
  SQLCHAR connection_name_[SQL_NAME_LENGTH];
  SQLCHAR server_name_[SQL_NAME_LENGTH];

  /** Diagnostics container */
  FieldVec fields_;
};


typedef Diagnostics<SQLHENV, SQL_HANDLE_ENV> EnvironmentDiagnostics;
typedef Diagnostics<SQLHDBC, SQL_HANDLE_DBC> ConnectionDiagnostics;
typedef Diagnostics<SQLHSTMT, SQL_HANDLE_STMT> StatementDiagnostics;
typedef Diagnostics<SQLHDESC, SQL_HANDLE_DESC> DescriptorDiagnostics;

} // namespace odbc
} // namespace sql
} // namespace fun
