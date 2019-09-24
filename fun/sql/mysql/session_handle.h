#pragma once

#include <mysql.h>
#include "fun/sql/mysql/mysql_exception.h"

namespace fun {
namespace sql {
namespace mysql {

/**
 * MySQL session handle
 */
class SessionHandle {
 public:
  /**
   * Creates session handle
   */
  explicit SessionHandle(MYSQL* mysql);

  /**
   * Destroy handle, close connection
   */
  ~SessionHandle();

  /**
   * Initializes the handle if not initialized.
   */
  void Init(MYSQL* mysql = nullptr);

  /**
   * Set connection options
   */
  void SetOptions(mysql_option opt);

  /**
   * Set connection options
   */
  void SetOptions(mysql_option opt, bool b);

  /**
   * Set connection options
   */
  void SetOptions(mysql_option opt, const char* c);

  /**
   * Set connection options
   */
  void SetOptions(mysql_option opt, unsigned int i);

  /**
   * Connect to server
   */
  void Connect(const char* host, const char* user, const char* password, const char* db, unsigned int port);

  /**
   * Close connection
   */
  void Close();

  /**
   * Start transaction
   */
  void StartTransaction();

  /**
   * Commit transaction
   */
  void Commit();

  /**
   * Rollback transaction
   */
  void Rollback();

  /**
   * Reset connection with dababase and clears session state, but without disconnecting
   */
  void Reset();

  operator MYSQL* ();

 private:
  MYSQL* handle_;

 public:
  SessionHandle(const SessionHandle&) = delete;
  SessionHandle& operator=(const SessionHandle&) = delete;
};


//
// inlines
//

inline SessionHandle::operator MYSQL* () {
  return handle_;
}

} // namespace mysql
} // namespace sql
} // namespace fun
