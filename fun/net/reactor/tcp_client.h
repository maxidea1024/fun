#pragma once

#include "fun/net/net.h"
#include "fun/net/reactor/tcp_connection.h"

namespace fun {
namespace net {
namespace reactor {

class Connector;
typedef SharedPtr<Connector> ConnectorPtr;

class TcpClient : Noncopyable {
 public:
  // TcpClient(EventLoop* loop);
  // TcpClient(EventLoop* loop, const String& host, uint16_t port);
  TcpClient(EventLoop* loop, const InetAddress& server_addr,
            const String& name);
  ~TcpClient();  // force out-line dtor, for scoped_ptr members_.

  void Connect();
  void Disconnect();
  void Stop();

  TcpConnectionPtr GetConnection() const {
    ScopedLock guard(mutex_);
    return connection_;
  }

  EventLoop* GetLoop() const { return loop_; }

  bool ShouldRetry() const { return retry_; }

  void EnableRetry() { retry_ = true; }

  const String& GetName() const { return name_; }

  /**
   * Set connection callback.
   * Not thread safe.
   */
  void SetConnectionCallback(const ConnectionCallback& cb) {
    connection_cb_ = cb;
  }

  /**
   * Set message callback.
   * Not thread safe.
   */
  void SetMessageCallback(const MessageCallback& cb) { message_cb_ = cb; }

  /**
   * Set write complete callback.
   * Not thread safe.
   */
  void SetWriteCompleteCallback(const WriteCompleteCallback& cb) {
    write_complete_cb_ = cb;
  }

  void SetConnectionCallback(ConnectionCallback&& cb) {
    connection_cb_ = MoveTemp(cb);
  }

  void SetMessageCallback(MessageCallback&& cb) { message_cb_ = MoveTemp(cb); }

  void SetWriteCompleteCallback(WriteCompleteCallback&& cb) {
    write_complete_cb_ = MoveTemp(cb);
  }

 private:
  /** Not thread safe, but in loop */
  void NewConnection(int sock_fd);

  /** Not thread safe, but in loop */
  void RemoveConnection(const TcpConnectionPtr& conn);

  EventLoop* loop_;
  ConnectorPtr connector_;  // avoid revealing Connector
  const String name_;
  ConnectionCallback connection_cb_;
  MessageCallback message_cb_;
  WriteCompleteCallback write_complete_cb_;
  bool retry_;    // atomic
  bool connect_;  // atomic
  // always in loop thread
  int next_conn_id_;
  mutable MutexLock mutex_;
  TcpConnectionPtr connection_;  // @GuardedBy mutex_
};

}  // namespace reactor
}  // namespace net
}  // namespace fun
