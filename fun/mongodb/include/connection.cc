#include "fun/mongodb/connection.h"

namespace fun {
namespace mongodb {

//
// Connection::socket_factory
//

Connection::SocketFactory::SocketFactory() {}

Connection::SocketFactory::~SocketFactory() {}

StreamSocket Connection::SocketFactory::CreateSocket(
                const String& host,
                int32 port,
                const Timespan& connect_timeout,
                bool secure) {
  if (!secure) {
    InetAddress address(host, port);
    StreamSocket socket;

    if (connect_timeout > 0) {
      socket.Connect(address, connect_timeout);
    } else {
      socket.Connect(address);
    }
    return socket;
  } else {
    throw NotImplementedException("Default socket_factory implementation does not support SecureStreamSocket");
  }
}


//
// Connection
//

Connection::Connection()
  : address_(), socket_() {}

Connection::Connection(const String& host_and_port)
  : address_(host_and_port), socket_() {
  Connect();
}

Connection::Connection(const String& uri, SocketFactory& socket_factory)
  : address_(), socket_() {
  Connect(uri, socket_factory);
}

Connection::Connection(const String& host, int32 port)
  : address_(host, port), socket_() {
  Connect();
}

Connection::Connection(const InetAddress& address)
  : address_(address), socket_() {
  Connect();
}

Connection::Connection(const StreamSocket& socket)
  : address_(socket.PeerAddress()), socket_(socket) {
  Connect();
}

Connection::~Connection() {
  try {
    Disconnect();
  } catch (...) {}
}

const InetAddress Connection::GetAddress() const {
  return address_;
}

void Connection::Connect(const String& host_and_port) {
  address_ = InetAddress(host_and_port);

  Connect();
}

// https://docs.mongodb.com/v3.0/reference/conn-string/
void Connection::Connect(const String& uri_str, SocketFactory& socket_factory) {
  Uri uri(uri_str);
  if (uri.GetScheme() != "mongodb") {
    throw UnknownUriSchemeException(uri_str);
  }

  String user_info = uri.GetUserInfo();
  String host = uri.GetHost();
  int32 port = uri.GetPort();
  if (port == 0) {
    port = 27017;
  }

  String db_name = uri.GetPath();
  if (db_name.len() > 0 && db_name[0] == '/') {
    db_name = db_name.Substring(1);
  }
  if (db_name.IsEmpty()) {
    db_name = "admin"; //기본 db이름이 "admin"인가??
  }

  bool with_ssl = false;
  Timespan connect_timeout;
  Timespan socket_timeout;
  String auth_mechanism = Database::AUTH_SCRAM_SHA1;

  //TODO parse query parameters
  //TODO parse query parameters
  //TODO parse query parameters
  //TODO parse query parameters
  //TODO parse query parameters
  //TODO parse query parameters
  //TODO parse query parameters

  Connect(socket_factory.CreateSocket(host, port, connect_timeout, with_ssl));

  if (socket_timeout > 0) {
    socket_.SetSendTimeout(socket_timeout);
    socket_.SetReceiveTimeout(socket_timeout);
  }

  if (not user_info.IsEmpty()) {
    String user_name;
    String password;
    int32 colon_pos = user_info.IndexOf(':');
    if (colon_pos >= 0) {
      user_name = user_info.Substring(0, colon_pos);
      password = user_info.Substring(colon_pos + 1);
    } else {
      user_name = user_info;
    }

    Database db(db_name);
    if (!db.Authenticate(*this, user_name, password, auth_mechanism)) {
      throw NoPermissionException(String::Format("access to MongoDB database %s denied for user %s", *db_name, user_name));
    }
  }
}

void Connection::Connect(const String& host, int32 port) {
  address_ = InetAddress(host, port);

  Connect();
}

void Connection::Connect(const InetAddress& address) {
  address_ = address;

  Connect();
}

void Connection::Connect(const StreamSocket& socket) {
  address_ = socket.GetPeerAddress();

  Connect();
}

void Connection::Connect() {
  socket_.Connect(address_);
}

void Connection::Disconnect() {
  socket_.Close();
}

void Connection::Call(Request& request) {
  //TODO
}

void Connection::Call(Request& request, Response& response) {
  //TODO
}

} // namespace mongodb
} // namespace fun
