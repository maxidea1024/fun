#pragma once

#include "fun/net/socket/netsocket.h"
#include "fun/net/socket/tcp_client.h"
#include "fun/redis/builders/reply_builder.h"

namespace fun {
namespace redis {

class FUN_REDIS_API Connection {
 public:
  typedef Function<void(Connection&)> ConnectedCallback;
  typedef Function<void(Connection&)> DisconnectedCallback;
  typedef Function<void(Connection&,Reply&)> ReplyCallback;

 public:
  Connection();
  Connection(const SharedPtr<TcpClient>& tcp_client);
  ~Connection();

  void ConnectSync( const String& host = "localhost",
                    int32 port = 6379,
                    const DisconnectedCallback& disconnected_cb = nullptr,
                    const ReplyCallback& reply_cb = nullptr);
                    
  void ConnectAsync(const String& host = "localhost",
                    int32 port = 6379,
                    const ConnectedCallback& connected_cb = nullptr,
                    const DisconnectedCallback& disconnected_cb = nullptr,
                    const ReplyCallback& reply_cb = nullptr);

  void Disconnect(bool wait_for_removal = false);

  bool IsConnected() const;
  bool IsConnecting() const;
  bool IsDisconnected() const;

  Connection& Send(const TArray<String>& redis_cmd);
  Connection& Commit();

 private:
  void OnTcpReceive(const TcpClient::ReadResult& result);
  void OnTcpConnected();
  void OnTcpDisconnected();

 private:
  void CallDisconnectionHandler();
  String BuildCommand(const TArray<String>& redis_cmd);

 private:
  SharedPtr<TcpClient> tcp_client_;
  ReplyCallback reply_cb_;
  ConnectedCallback connected_cb_;
  DisconnectedCallback disconnected_cb_;
  ReplyBuilder builder_;
  String buffer_;
  Mutex buffer_mutex_;
};

} // namespace redis
} // namespace fun
