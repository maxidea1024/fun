#pragma once

#include "fun/base/function.h"
#include "fun/net/inet_address.h"
#include "fun/net/net.h"
#include "fun/net/reactor/channel.h"

namespace fun {
namespace net {
namespace reactor {

class EventLoop;
class InetAddress;

/**
 * Acceptor of incoming TCP connections.
 */
class Acceptor : Noncopyable {
 public:
  typedef Function<void(int sock_fd, const InetAddress&)> NewConnectionCallback;

  /**
   * Acceptor를 여러개 설치할 경우에는 reuse_port를 true로 해주어야함.
   */
  Acceptor(EventLoop* loop, const InetAddress& listen_addr, bool reuse_port);

  ~Acceptor();

  void SetNewConnectionCallback(const NewConnectionCallback& cb) {
    new_connection_cb_ = cb;
  }

  bool IsListening() const { return listenning_; }

  void Listen();

 private:
  void HandleRead();

  EventLoop* loop_;
  Socket accept_socket_;
  Channel accept_channel_;
  NewConnectionCallback new_connection_cb_;
  bool listenning_;
  int idle_fd_;
};

}  // namespace reactor
}  // namespace net
}  // namespace fun
