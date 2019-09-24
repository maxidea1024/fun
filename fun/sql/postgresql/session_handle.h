#pragma once

#include "fun/base/mutex.h"
#include "fun/types.h"

#include <map>
#include <string>
#include <vector>

#include <libpq-fe.h>

namespace fun {
namespace sql {
namespace postgresql {

/**
 * PostgreSQL session parameters
 */
class SessionParameters {
 public:
  enum HOW_TO_DISPLAY {
    HTD_ASIS,   // as is
    HTD_HIDE,   // do not display (e.g. passwords)
    HID_DEBUG   // debug use only
  };

  SessionParameters(const String& keyword,
                    const String& environment_variable,
                    const String& compiled_default,
                    const String& current_value,
                    const String& display_label,
                    const String& how_to_display,
                    int display_size);

  ~SessionParameters();

  String keyword() const;
  String enviromentVariable() const;
  String compiledDefault() const;
  String currentValue() const;
  String displayLabel() const;
  HOW_TO_DISPLAY howToDisplay() const;
  int displaySize() const;

 private:
  String keyword_; // The keyword of the option
  String environment_variable_; // Fallback environment variable name
  String compiled_default_; // Fallback compiled in default value
  String current_value_; // Option's current value, or NULL
  String display_label_; // Label for field in a connect dialog
  HOW_TO_DISPLAY how_to_display_; // Indicates how to display this field
  int display_size_; // Field size in characters for connect dialog
};

typedef std::map<String, SessionParameters> SessionParametersMap;

/**
 * PostgreSQL connection(session) handle
 */
class SessionHandle {
 public:
  /**
   * Creates session handle
   */
  explicit SessionHandle();

  /**
   * Destroy handle, close connection
   */
  ~SessionHandle();

  /**
   * Connect to server
   */
  void Connect(const String& connection_string);

  void Connect(const char* connection_string);

  void Connect(const char* host, const char* user, const char* password,
    const char* database, unsigned short port, unsigned int connection_timeout);

  /**
   * is a connection established?
   */
  bool IsConnected() const;

  /**
   * Close connection
   */
  void Disconnect();

  /**
   * reset the connection
   */
  bool Reset();

  /**
   * last error on the connection
   */
  String GetLastError() const;

  /**
   * Start transaction
   */
  void StartTransaction();

  /**
   * Returns true if a transaction is a transaction is in progress, false otherwise.
   */
  bool IsInTransaction();

  /**
   * Commit transaction
   */
  void Commit();

  /**
   * Rollback trabsaction
   */
  void Rollback();

  /**
   * is the connection in auto commit mode?
   */
  bool IsAutoCommit();

  /**
   * is the connection in auto commit mode?
   */
  void SetAutoCommit(bool aShouldAutoCommit = true);

  /**
   * is the connection in Asynchronous commit mode?
   */
  bool IsAsynchronousCommit();

  /**
   * is the connection in Asynchronous commit mode?
   */
  void SetAsynchronousCommit(bool aShouldAsynchronousCommit = true);

  /**
   * Attempts to cancel in-process statements
   */
  void Cancel();

  /**
   * Sets the transaction isolation level.
   */
  void SetTransactionIsolation(uint32 ti);

  /**
   * Returns the transaction isolation level.
   */
  uint32 GetTransactionIsolation();

  /**
   * Returns true if the transaction isolation level corresponding
   * to the supplied bitmask is supported.
   */
  bool HasTransactionIsolation(uint32 ti);

  /**
   * deallocates a previously prepared statement
   */
  void DeallocatePreparedStatement(const String& aPreparedStatementToDeAllocate);

  /**
   * remote server version
   */
  int GetServerVersion() const;

  /**
   * the process ID of the remotee server process
   */
  int GetServerProcessId() const;

  /**
   * the protocol version between the client and the server
   */
  int GetProtocoVersion() const;

  /**
   * returns the client encoding
   */
  String GetClientEncoding() const;

  /**
   * returns the version of libpq
   */
  int GetLibpqVersion() const;

  /**
   * returns the default parameters used on a connection
   */
  static SessionParametersMap GetConnectionDefaultParameters();

  /**
   * returns the parameters used on the connection
   */
  SessionParametersMap GetConnectionParameters() const;

  /**
   * returns the string used to connect
   */
  String GetConnectionString() const;

  /**
   * Get the PostgreSQL connection pointer
   */
  operator PGconn* ();

  /**
   * Get the sessionHandle mutex to protect the connection pointer
   */
  fun::FastMutex& GetMutex();

 private:
  static SessionParametersMap SetConnectionInfoParameters(PQconninfoOption* aConnectionInfoOptionsPtr);

  void DeallocateStoredPreparedStatements();

  void DeallocatePreparedStatementNoLock(const String& aPreparedStatementToDeAllocate);
  bool IsConnectedNoLock() const;
  String GetLastErrorNoLock() const;

  SessionHandle(const SessionHandle&) = delete;
  SessionHandle& operator=(const SessionHandle&) = delete;

 private:
  mutable fun::FastMutex session_mutex_;
  PGconn* connection_ptr_;
  String connection_string_;
  bool in_transaction_;
  bool is_auto_commit_;
  bool is_asynchronous_commit_;
  uint32 transaction_isolation_level_;
  std::vector <String> prepared_statements_to_be_deallocated_;

  //static const String POSTGRESQL_READ_UNCOMMITTED; // NOT SUPPORTED
  static const String POSTGRESQL_READ_COMMITTED;
  static const String POSTGRESQL_REPEATABLE_READ;
  static const String POSTGRESQL_SERIALIZABLE;
};


//
// inlines
//

// SessionParameters

inline SessionParameters::SessionParameters(const String& aKeyword,
                                            const String& environment_variable,
                                            const String& compiled_default,
                                            const String& current_value,
                                            const String& display_label,
                                            const String& how_to_display,
                                            int display_size)
  : keyword_(aKeyword),
    environment_variable_(environment_variable),
    compiled_default_(compiled_default),
    current_value_(current_value),
    display_label_(display_label),
    how_to_display_(HTD_ASIS),
    display_size_(display_size) {
  if (how_to_display == "*") {
    how_to_display_ = HTD_HIDE;
  }

  if (how_to_display == "D") {
    how_to_display_ = HID_DEBUG;
  }
}

inline SessionParameters::~SessionParameters() {}

inline String SessionParameters::GetKeyword() const {
  return keyword_;
}

inline String SessionParameters::GetEnviromentVariable() const {
  return environment_variable_;
}

inline String SessionParameters::GetCompiledDefault() const {
  return compiled_default_;
}

inline String SessionParameters::GetCurrentValue() const {
  return current_value_;
}

inline String SessionParameters::GetDisplayLabel() const {
  return display_label_;
}

inline SessionParameters::HOW_TO_DISPLAY SessionParameters::GetHowToDisplay() const {
  return how_to_display_;
}

inline int SessionParameters::GetDisplaySize() const {
  return display_size_;
}


// SessionHandle

inline SessionHandle::operator PGconn * () {
  return connection_ptr_;
}

inline fun::FastMutex&SessionHandle::GetMutex() {
  return session_mutex_;
}

inline String SessionHandle::GetConnectionString() const {
  return connection_string_;
}

inline bool SessionHandle::IsInTransaction() {
  return in_transaction_;
}

inline bool SessionHandle::IsAutoCommit() {
  return is_auto_commit_;
}

inline bool SessionHandle::IsAsynchronousCommit() {
  return is_asynchronous_commit_;
}

} // namespace postgresql
} // namespace sql
} // namespace fun
