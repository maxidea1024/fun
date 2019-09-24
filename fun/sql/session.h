#pragma once

#include <algorithm>
#include "fun/base/any.h"
#include "fun/ref_counted_ptr.h"
#include "fun/sql/binding.h"
#include "fun/sql/session_impl.h"
#include "fun/sql/sql.h"
#include "fun/sql/statement.h"
#include "fun/sql/statement_creator.h"

namespace fun {
namespace sql {

class StatementImpl;

/**
 * A Session holds a connection to a Database and creates Statement objects.
 *
 * Sessions are always created via the SessionFactory:
 *
 *     Session ses(SessionFactory::instance().create(connectorKey,
 * connection_string));
 *
 * where the first param presents the type of session one wants to create (e.g.,
 * for SQLite one would choose "SQLite", for ODBC the key is "ODBC") and the
 * second param is the connection string that the session implementation
 * requires to connect to the database. The format of the connection string is
 * specific to the actual connector.
 *
 * A simpler form to create the session is to pass the connector key and
 * connection string directly to the Session constructor.
 *
 * A concrete example to open an SQLite database stored in the file "dummy.db"
 * would be
 *
 *     Session ses("SQLite", "dummy.db");
 *
 * Via a Session one can create two different types of statements. First,
 * statements that should only be executed once and immediately, and second,
 * statements that should be executed multiple times, using a separate Execute()
 * call. The simple one is immediate execution:
 *
 *     ses << "CREATE TABLE Dummy (data INTEGER(10))", now;
 *
 * The now at the end of the statement is required, otherwise the statement
 * would not be executed.
 *
 * If one wants to reuse a Statement (and avoid the overhead of repeatedly
 * parsing an sql statement) one uses an explicit Statement object and its
 * Execute() method:
 *
 *     int i = 0;
 *     Statement stmt = (ses << "INSERT INTO Dummy VALUES(:data)", use(i));
 *
 *     for (i = 0; i < 100; ++i) {
 *         stmt.Execute();
 *     }
 *
 * The above example assigns the variable i to the ":data" placeholder in the
 * sql query. The query is parsed and compiled exactly once, but executed 100
 * times. At the end the values 0 to 99 will be present in the Table "DUMMY".
 *
 * A faster implementation of the above code will simply create a vector of int
 * and use the vector as parameter to the use clause (you could also use set or
 * multiset instead):
 *
 *     std::vector<int> data;
 *     for (int i = 0; i < 100; ++i) {
 *         data.push_back(i);
 *     }
 *     ses << "INSERT INTO Dummy VALUES(:data)", use(data);
 *
 * NEVER try to bind to an empty collection. This will give a BindingException
 * at run-time!
 *
 * Retrieving data from a database works similar, you could use simple data
 * types, vectors, sets or multiset as your targets:
 *
 *     std::set<int> retData;
 *     ses << "SELECT * FROM Dummy", into(retData));
 *
 * Due to the blocking nature of the above call it is possible to partition the
 * data retrieval into chunks by setting a limit to the maximum number of rows
 * retrieved from the database:
 *
 *     std::set<int> retData;
 *     Statement stmt = (ses << "SELECT * FROM Dummy", into(retData),
 * limit(50)); while (!stmt.IsDone()) { stmt.Execute();
 *     }
 *
 * The "into" keyword is used to inform the statement where output results
 * should be placed. The limit value ensures that during each run at most 50
 * rows are retrieved. Assuming Dummy contains 100 rows, retData will contain 50
 * elements after the first run and 100 after the second run, i.e.
 * the collection is not cleared between consecutive runs. After the second
 * execute stmt.IsDone() will return true.
 *
 * A prepared Statement will behave exactly the same but a further call to
 * Execute() will simply reset the Statement, execute it again and append more
 * data to the result set.
 *
 * Note that it is possible to append several "bind" or "into" clauses to the
 * statement. Theoretically, one could also have several limit clauses but only
 * the last one that was added will be effective. Also several preconditions
 * must be met concerning binds and intos. Take the following example:
 *
 *     ses << "CREATE TABLE Person (LastName VARCHAR(30), FirstName VARCHAR, Age
 * INTEGER(3))"; std::vector<String> nameVec; // [...] add some elements
 *     std::vector<int> ageVec; // [...] add some elements
 *     ses << "INSERT INTO Person (LastName, Age) VALUES(:ln, :age)",
 * use(nameVec), use(ageVec);
 *
 * The size of all use parameters MUST be the same, otherwise an exception is
 * thrown. Furthermore, the amount of use clauses must match the number of
 * wildcards in the query (to be more precise: each binding has a
 * numberOfColumnsHandled() value which defaults to 1. The sum of all these
 * values must match the wildcard count in the query. However, this is only
 * important if you have written your own TypeHandler specializations. If you
 * plan to map complex object types to tables see the TypeHandler documentation.
 * For now, we simply assume we have written one TypeHandler for Person objects.
 * Instead of having n different vectors, we have one collection:
 *
 *     std::vector<Person> people; // [...] add some elements
 *     ses << "INSERT INTO Person (LastName, FirstName, Age) VALUES(:ln, :fn,
 * :age)", use(people);
 *
 * which will insert all Person objects from the people vector to the database
 * (and again, you can use set, multiset too, even map and multimap if Person
 * provides an operator() which returns the key for the map). The same works for
 * a SELECT statement with "into" clauses:
 *
 *     std::vector<Person> people;
 *     ses << "SELECT * FROM PERSON", into(people);
 *
 * Mixing constants or variables with manipulators is allowed provided there are
 * corresponding placeholders for the constants provided in the sql string, such
 * as in following example:
 *
 *     std::vector<Person> people;
 *     ses << "SELECT * FROM %s", into(people), "PERSON";
 *
 * Formatting only kicks in if there are values to be injected into the sql
 * string, otherwise it is skipped. If the formatting will occur and the percent
 * sign is part of the query itself, it can be passed to the query by entering
 * it twice (%%). However, if no formatting is used, one percent sign is
 * sufficient as the string will be passed unaltered. For complete list of
 * supported data types with their respective specifications, see the
 * documentation for format in Foundation.
 */
class FUN_SQL_API Session {
 public:
  static const size_t LOGIN_TIMEOUT_DEFAULT =
      SessionImpl::LOGIN_TIMEOUT_DEFAULT;
  static const uint32 TRANSACTION_READ_UNCOMMITTED = 0x00000001L;
  static const uint32 TRANSACTION_READ_COMMITTED = 0x00000002L;
  static const uint32 TRANSACTION_REPEATABLE_READ = 0x00000004L;
  static const uint32 TRANSACTION_SERIALIZABLE = 0x00000008L;

  /**
   * Creates the Session.
   */
  Session(SessionImpl::Ptr impl);

  /**
   * Creates a new session, using the given connector (which must have
   * been registered), and connection_string.
   */
  Session(const String& connector, const String& connection_string,
          size_t timeout = LOGIN_TIMEOUT_DEFAULT);

  /**
   * Creates a new session, using the given connection (must be in
   * "connection:///connection_string" format).
   */
  Session(const String& connection, size_t timeout = LOGIN_TIMEOUT_DEFAULT);

  /**
   * Creates a session by copying another one.
   */
  Session(const Session&);

  /**
   * Creates a session by moving another one.
   */
  Session(Session&&);

  /**
   * Assignment operator.
   */
  Session& operator=(const Session&);

  /**
   * Assignment move operator.
   */
  Session& operator=(Session&&);

  /**
   * Destroys the Session.
   */
  ~Session();

  /**
   * Swaps the session with another one.
   */
  void Swap(Session& other);

  /**
   * Creates a Statement with the given data as SQLContent
   */
  template <typename T>
  Statement operator<<(const T& t) {
    return statement_creator_ << t;
  }

  /**
   * Creates a StatementImpl.
   */
  StatementImpl::Ptr CreateStatementImpl();

  /**
   * Opens the session using the supplied string.
   * Can also be used with default empty string to
   * Reconnect a disconnected session.
   * If the connection is not established,
   * a ConnectionFailedException is thrown.
   * Zero timeout means indefinite
   */
  void Open(const String& connect = "");

  /**
   * Closes the session.
   */
  void Close();

  /**
   * Returns true if session is connected, false otherwise.
   */
  bool IsConnected();

  /**
   * Closes the session and opens it.
   */
  void Reconnect();

  /**
   * Sets the session login timeout value.
   */
  void SetLoginTimeout(size_t timeout);

  /**
   * Returns the session login timeout value.
   */
  size_t GetLoginTimeout() const;

  /**
   * Sets the session connection timeout value.
   */
  void SetConnectionTimeout(size_t timeout);

  /**
   * Returns the session connection timeout value.
   */
  size_t GetConnectionTimeout() const;

  /**
   * Starts a transaction.
   */
  void Begin();

  /**
   * Commits and ends a transaction.
   */
  void Commit();

  /**
   * Rolls back and ends a transaction.
   */
  void Rollback();

  /**
   * Returns true if session has transaction capabilities.
   */
  bool CanTransact();

  /**
   * Returns true if a transaction is in progress, false otherwise.
   */
  bool IsInTransaction();

  /**
   * Sets the transaction isolation level.
   */
  void SetTransactionIsolation(uint32);

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
   * Returns true if the transaction isolation level corresponds
   * to the supplied bitmask.
   */
  bool IsTransactionIsolation(uint32 ti);

  /**
   * Returns the connector name for this session.
   */
  String GetConnector() const;

  /**
   * Returns the URI for this session.
   */
  String GetUri() const;

  // TODO MakeUri�� �ٲ��ִ°� ������..?
  /**
   * Utility function that returns the URI formatted from supplied
   * arguments as "connector:///connection_string".
   */
  static String GetUri(const String& connector,
                       const String& connection_string);

  /**
   * Set the state of a feature.
   *
   * Features are a generic extension mechanism for session implementations.
   * and are defined by the underlying SessionImpl instance.
   *
   * Throws a NotSupportedException if the requested feature is
   * not supported by the underlying implementation.
   */
  void SetFeature(const String& name, bool state);

  /**
   * Look up the state of a feature.
   *
   * Features are a generic extension mechanism for session implementations.
   * and are defined by the underlying SessionImpl instance.
   *
   * Throws a NotSupportedException if the requested feature is
   * not supported by the underlying implementation.
   */
  bool GetFeature(const String& name) const;

  /**
   * Set the value of a property.
   *
   * Properties are a generic extension mechanism for session implementations.
   * and are defined by the underlying SessionImpl instance.
   *
   * Throws a NotSupportedException if the requested property is
   * not supported by the underlying implementation.
   */
  void SetProperty(const String& name, const fun::Any& value);

  /**
   * Look up the value of a property.
   *
   * Properties are a generic extension mechanism for session implementations.
   * and are defined by the underlying SessionImpl instance.
   *
   * Throws a NotSupportedException if the requested property is
   * not supported by the underlying implementation.
   */
  fun::Any GetProperty(const String& name) const;

  /**
   * Returns a pointer to the underlying SessionImpl.
   */
  SessionImpl::Ptr GetImpl();

 private:
  Session();

  SessionImpl::Ptr impl_;
  StatementCreator statement_creator_;
};

//
// inlines
//

inline StatementImpl::Ptr Session::CreateStatementImpl() {
  return impl_->CreateStatementImpl();
}

inline void Session::Open(const String& connect) { impl_->Open(connect); }

inline void Session::Close() { impl_->Close(); }

inline bool Session::IsConnected() { return impl_->IsConnected(); }

inline void Session::Reconnect() { impl_->Reconnect(); }

inline void Session::SetLoginTimeout(size_t timeout) {
  impl_->SetLoginTimeout(timeout);
}

inline size_t Session::GetLoginTimeout() const {
  return impl_->GetLoginTimeout();
}

inline void Session::SetConnectionTimeout(size_t timeout) {
  impl_->SetConnectionTimeout(timeout);
}

inline size_t Session::GetConnectionTimeout() const {
  return impl_->GetConnectionTimeout();
}

inline void Session::Begin() { return impl_->Begin(); }

inline void Session::Commit() { return impl_->Commit(); }

inline void Session::Rollback() { return impl_->Rollback(); }

inline bool Session::CanTransact() { return impl_->CanTransact(); }

inline bool Session::IsInTransaction() { return impl_->IsInTransaction(); }

inline void Session::SetTransactionIsolation(uint32 ti) {
  impl_->SetTransactionIsolation(ti);
}

inline uint32 Session::GetTransactionIsolation() {
  return impl_->GetTransactionIsolation();
}

inline bool Session::HasTransactionIsolation(uint32 ti) {
  return impl_->HasTransactionIsolation(ti);
}

inline bool Session::IsTransactionIsolation(uint32 ti) {
  return impl_->IsTransactionIsolation(ti);
}

inline String Session::GetConnector() const {
  return impl_->GetConnectorName();
}

inline String Session::GetUri(const String& connector,
                              const String& connection_string) {
  return SessionImpl::GetUri(connector, connection_string);
}

inline String Session::GetUri() const { return impl_->uri(); }

inline void Session::SetFeature(const String& name, bool state) {
  impl_->SetFeature(name, state);
}

inline bool Session::GetFeature(const String& name) const {
  return impl_->GetFeature(name);
}

inline void Session::SetProperty(const String& name, const fun::Any& value) {
  impl_->SetProperty(name, value);
}

inline fun::Any Session::GetProperty(const String& name) const {
  return impl_->GetProperty(name);
}

inline SessionImpl::Ptr Session::GetImpl() { return impl_; }

inline void Swap(Session& s1, Session& s2) { s1.Swap(s2); }

}  // namespace sql
}  // namespace fun

namespace std {

/**
 * Full template specialization of std:::Swap for Session
 */
template <>
inline void Swap<fun::sql::Session>(fun::sql::Session& s1,
                                    fun::sql::Session& s2) {
  s1.Swap(s2);
}

}  // namespace std
