#pragma once

namespace fun {
namespace mongodb {

typedef SharedPtr<class Connection> ConnectionPtr;

/**
 * Represents a conn to a MongoDB server
 * using the MongoDB wire protocol.
 *
 * See https://docs.mongodb.com/manual/reference/mongodb-wire-protocol/
 * for more information on the wire protocol.
 */
class FUN_MONGODB_API Connection {
 public:
  class FUN_MONGODB_API SocketFactory {
   public:
    SocketFactory();
    virtual ~SocketFactory();

    virtual StreamSocket CreateSocket(const String& host, int32 port,
                                      const Timespan& connect_timeout,
                                      bool secure);
  };

  Connection();
  Connection(const String& host_and_port);
  Connection(const String& uri, SocketFactory& socket_factory);
  Connection(const String& host, int32 port);
  Connection(const InetAddress& address);
  Connection(const StreamSocket& socket);
  virtual ~Connection();

  const InetAddress GetAddress() const;

  void Connect(const String& host_and_port);
  void Connect(const String& uri, SocketFactory& socket_factory);
  void Connect(const String& host, int32 port);
  void Connect(const InetAddress& address);
  void Connect(const StreamSocket& socket);
  void Disconnect();

  void Call(Request& request);
  void Call(Request& request, Response& response);

 protected:
  void Connect();

 private:
  InetAddress address_;
  StreamSocket socket_;
};

}  // namespace mongodb
}  // namespace fun
