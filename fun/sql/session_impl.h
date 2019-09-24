#pragma once

#include "fun/base/any.h"
#include "fun/base/format.h"
#include "fun/base/string.h"
#include "fun/ref_counted_object.h"
#include "fun/ref_counted_ptr.h"
#include "fun/sharedptr.h"
#include "fun/sql/sql.h"

namespace fun {
namespace sql {

class StatementImpl;

/**
 * Interface for Session functionality that subclasses must extend.
 * SessionImpl objects are noncopyable. {
 */
class FUN_SQL_API SessionImpl : public fun::RefCountedObject public
    : typedef RefCountedPtr<SessionImpl>
          Ptr;

/**
 * Infinite connection/login timeout.
 */
static const size_t LOGIN_TIMEOUT_INFINITE = 0;

/**
 * Default connection/login timeout in seconds.
 */
static const size_t LOGIN_TIMEOUT_DEFAULT = 60;

/**
 * Infinite connection/login timeout.
 */
static const size_t CONNECTION_TIMEOUT_INFINITE = 0;

/**
 * Default connection/login timeout in seconds.
 */
static const size_t CONNECTION_TIMEOUT_DEFAULT = CONNECTION_TIMEOUT_INFINITE;

/**
 * Creates the SessionImpl.
 */
SessionImpl(const String& connection_string,
            size_t timeout = LOGIN_TIMEOUT_DEFAULT);

/**
 * Destroys the SessionImpl.
 */
virtual ~SessionImpl();

/**
 * Creates a StatementImpl.
 */
virtual SharedPtr<StatementImpl> CreateStatementImpl() = 0;

/**
 * Opens the session using the supplied string.
 * Can also be used with default empty string to Reconnect
 * a disconnected session.
 * If the connection is not established within requested timeout
 * (specified in seconds), a ConnectionFailedException is thrown.
 * Zero timeout means indefinite
 */
virtual void Open(const String& connection_string = "") = 0;

/**
 * Closes the connection.
 */
virtual void Close() = 0;

/**
 * Reset connection with dababase and clears session state, but without
 * disconnecting
 */
virtual void Reset() = 0;

/**
 * Returns true if session is connected, false otherwise.
 */
virtual bool IsConnected() const = 0;

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
virtual void SetConnectionTimeout(size_t timeout) = 0;

/**
 * Returns the session connection timeout value.
 */
virtual size_t GetConnectionTimeout() const = 0;

/**
 * Closes the connection and opens it again.
 */
void Reconnect();

/**
 * Starts a transaction.
 */
virtual void Begin() = 0;

/**
 * Commits and ends a transaction.
 */
virtual void Commit() = 0;

/**
 * Aborts a transaction.
 */
virtual void Rollback() = 0;

/**
 * Returns true if session has transaction capabilities.
 */
virtual bool CanTransact() const = 0;

/**
 * Returns true if a transaction is a transaction is in progress, false
 * otherwise.
 */
virtual bool IsInTransaction() const = 0;

/**
 * Sets the transaction isolation level.
 */
virtual void SetTransactionIsolation(uint32) = 0;

/**
 * Returns the transaction isolation level.
 */
virtual uint32 GetTransactionIsolation() const = 0;

/**
 * Returns true if the transaction isolation level corresponding
 * to the supplied bitmask is supported.
 */
virtual bool HasTransactionIsolation(uint32) const = 0;

/**
 * Returns true if the transaction isolation level corresponds
 * to the supplied bitmask.
 */
virtual bool IsTransactionIsolation(uint32) const = 0;

/**
 * Returns the name of the connector.
 */
virtual const String& GetConnectorName() const = 0;

/**
 * Returns the connection string.
 */
const String& GetConnectionString() const;

/**
 * Returns formatted URI.
 */
static String GetUri(const String& connector, const String& connection_string);

/**
 * Returns the URI for this session.
 */
String GetUri() const;

/**
 * Set the state of a feature.
 *
 * Features are a generic extension mechanism for session implementations.
 * and are defined by the underlying SessionImpl instance.
 *
 * Throws a NotSupportedException if the requested feature is
 * not supported by the underlying implementation.
 */
virtual void SetFeature(const String& name, bool state) = 0;

/**
 * Look up the state of a feature.
 *
 * Features are a generic extension mechanism for session implementations.
 * and are defined by the underlying SessionImpl instance.
 *
 * Throws a NotSupportedException if the requested feature is
 * not supported by the underlying implementation.
 */
virtual bool GetFeature(const String& name) const = 0;

/**
 * Set the value of a property.
 *
 * Properties are a generic extension mechanism for session implementations.
 * and are defined by the underlying SessionImpl instance.
 *
 * Throws a NotSupportedException if the requested property is
 * not supported by the underlying implementation.
 */
virtual void SetProperty(const String& name, const fun::Any& value) = 0;

/**
 * Look up the value of a property.
 *
 * Properties are a generic extension mechanism for session implementations.
 * and are defined by the underlying SessionImpl instance.
 *
 * Throws a NotSupportedException if the requested property is
 * not supported by the underlying implementation.
 */
virtual fun::Any GetProperty(const String& name) const = 0;

/**
 * Used by pooled sessions to put them back to the
 * owning pool available session list.
 * Defaults to no-op.
 */
virtual void PutBack();

protected:
/**
 * Sets the connection string. Should only be called on
 * disconnected sessions. Throws InvalidAccessException when called on
 * a connected session.
 */
void SetConnectionString(const String& connection_string);

private:
SessionImpl();
SessionImpl(const SessionImpl&);
SessionImpl& operator=(const SessionImpl&);

String connection_string_;
size_t login_timeout_;
};  // namespace sql

//
// inlines
//

inline const String& SessionImpl::GetConnectionString() const {
  return connection_string_;
}

inline void SessionImpl::SetLoginTimeout(size_t timeout) {
  login_timeout_ = timeout;
}

inline size_t SessionImpl::GetLoginTimeout() const { return login_timeout_; }

inline String SessionImpl::GetUri(const String& connector,
                                  const String& connection_string) {
  return String::Format("%s:///%s", connector, connection_string);
}

inline String SessionImpl::GetUri() const {
  return GetUri(GetConnectorName(), GetConnectionString());
}

inline void SessionImpl::PutBack() {}

}  // namespace fun
}  // namespace fun
