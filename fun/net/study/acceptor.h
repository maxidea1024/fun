#pragma once

namespace fun {
namespace net {

class EventLoop;

/// Acceptor of incoming TCP connections.
class Acceptor : Noncopyable
{
public:
  typedef Function<void (int fd, const InetAddress&)> NewConnectionCallback;

  Acceptor(EventLoop* loop, const InetAddress& listen_addr, bool reuse_port);
  ~Acceptor();

  void SetNewConnectionCallback(const NewConnectionCallback& cb)
  {
    new_connection_cb_ = cb;
  }

  bool IsListening() const { return listening_; }
  void Listen();

private:
  void HandleRead();

  EventLoop* loop_;
  Socket listen_socket_;
  Channel listen_channel_;
  NewConnectionCallback new_connection_cb_;
  bool listening_;
  int idle_fd_;
};


} // namespace net
} // namespace fun
