#pragma once

#include <map>
#include "fun/base/mutex.h"
#include "fun/base/shared_ptr.h"
#include "fun/base/string.h"
#include "fun/sql/connector.h"
#include "fun/sql/session.h"
#include "fun/sql/sql.h"

namespace fun {
namespace sql {

/**
 * A SessionFactory is a singleton class that stores Connectors and allows to
 * create Sessions of the required type:
 *
 *     Session ses(SessionFactory::Instance().Create(connector,
 * connection_string));
 *
 * where the first param presents the type of session one wants to create (e.g.
 * for SQLite one would choose "SQLite") and the second param is the connection
 * string that the connector requires to connect to the database.
 *
 * A concrete example to open an SQLite database stored in the file "dummy.db"
 * would be
 *
 *     Session ses(SessionFactory::Instance().Create(SQLite::Connector::KEY,
 * "dummy.db"));
 *
 * An even simpler way to create a session is to use the two argument
 * constructor of Session, which automatically invokes the SessionFactory:
 *
 *      Session ses("SQLite", "dummy.db");
 */
class FUN_SQL_API SessionFactory {
 public:
  /**
   * returns the static instance of the singleton.
   */
  static SessionFactory& Instance();

  /**
   * Registers a Connector under its key at the factory. If a registration for
   * that key is already active, the first registration will be kept, only its
   * reference count will be increased. Always takes ownership of parameter
   * connector.
   */
  void Add(Connector::Ptr connector);

  /**
   * Lowers the reference count for the Connector registered under that key.
   * If the count reaches zero, the object is removed.
   */
  void Remove(const String& key);

  /**
   * Creates a Session for the given key with the connection_string.
   * Throws an fun:sql::UnknownDataBaseException
   * if no Connector is registered for that key.
   */
  Session Create(const String& key, const String& connection_string,
                 size_t timeout = Session::LOGIN_TIMEOUT_DEFAULT);

  /**
   * Creates a Session for the given URI (must be in key:///connection_string
   * format). Throws a fun:sql::UnknownDataBaseException if no Connector is
   * registered for the key.
   */
  Session Create(const String& uri,
                 size_t timeout = Session::LOGIN_TIMEOUT_DEFAULT);

 private:
  struct SessionInfo {
    int32 cnt;
    Connector::Ptr connector;
    SessionInfo(Connector::Ptr connector);
  };

  typedef std::map<String, SessionInfo, fun::CILess> Connectors;
  Connectors connectors_;
  fun::FastMutex mutex_;

 public:
  SessionFactory() = delete;
  ~SessionFactory() = delete;
  SessionFactory(const SessionFactory&) = delete;
  SessionFactory& operator=(const SessionFactory&) = delete;
};

}  // namespace sql
}  // namespace fun
