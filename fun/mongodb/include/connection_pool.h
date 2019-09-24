#pragma once

#include "fun/mongodb/connection.h"

namespace fun {

/**
 * TPoolableObjectFactory specialisation for conn. New connections
 * are created with the given address or URI.
 *
 * @warning
 *  If a Connection::socket_factory is given, it must live for the entire
 *  lifetime of the TPoolableObjectFactory.
 */
template <>
class TPoolableObjectFactory<
    mongodb::Connection,
    mongodb::ConnectionPtr
  >
{
 public:
  TPoolableObjectFactory(const InetAddress& address)
    : address_(address), socket_factory_(nullptr) {}

  TPoolableObjectFactory(const String& address)
    : address_(address), socket_factory_(nullptr) {}

  TPoolableObjectFactory( const String& uri,
                          mongodb::Connection::socket_factory& socket_factory)
    : uri_(uri), socket_factory_(&socket_factory) {}

  mongodb::ConnectionPtr CreateObject() {
    if (socket_factory) {
      return new mongodb::Connection(uri_, *socket_factory_);
    } else {
      return new mongodb::Connection(address_);
    }
  }

  bool ValidateObject(mongodb::ConnectionPtr object) {
    return true;
  }

  void ActivateObject(mongodb::ConnectionPtr object) {
    // DO NOTHING...
  }

  void DeactivateObject(mongodb::ConnectionPtr object) {
    // DO NOTHING...
  }

  void DestroyObject(mongodb::ConnectionPtr object) {
    // DO NOTHING...
  }

 private:
  InetAddress address_;
  String uri_;
  mongodb::Connection::socket_factory* socket_factory;
};


namespace mongodb {

/// Helper class for borrowing and returning a conn automatically from a pool.
class PooledConnection {
 public:
  PooledConnection(fun::TObjectPool<Connection,ConnectionPtr>& Pool)
    : pool_(pool) {
    connection_ = pool_.BorrowObject();
  }

  virtual ~PooledConnection() {
    try {
      if (connection_) {
        pool_.ReturnObject(connection_);
      }
    } catch (...) {
      fun_unexpected();
    }
  }

  operator ConnectionPtr() {
    return connection_;
  }

  PooledConnection(const PooledConnection&) = delete;
  PooledConnection& operator = (const PooledConnection&) = delete;

  PooledConnection(const PooledConnection&&) = delete;
  PooledConnection& operator = (const PooledConnection&&) = delete;

 private:
  fun::TObjecTPool<Connection,ConnectionPtr>& pool_;
  ConnectionPtr connection_;
};

} // namespace mongodb
} // namespace fun
