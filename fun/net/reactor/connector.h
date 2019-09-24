#pragma once

#include "fun/net/net.h"
#include "fun/base/shared_ptr.h"
#include "fun/base/function.h"

namespace fun {
namespace net {

class Channel;
class EventLoop;

class Connector : Noncopyable, public EnableSharedFromThis<Connector> {
 public:
  typedef Function<void (int sock_fd)> NewConnectionCallback;

  Connector(EventLoop* loop, const InetAddress& server_addr);
  ~Connector();

  void SetNewConnectionCallback(const NewConnectionCallback& cb) {
    new_connection_cb_ = cb;
  }

  // can be called in any thread
  void Start();
  // must be called in loop thread
  void Restart();
  // can be called in any thread
  void Stop();

  const InetAddress& GetServerAddress() const {
    return server_addr_;
  }

 private:
  enum States { kDisconnected, kConnecting, kConnected };
  static const int kMaxRetryDelayMs = 30*1000;
  static const int kInitRetryDelayMs = 500;

  void SetState(States s) { state_ = s; }
  void StartInLoop();
  void StopInLoop();
  void Connect();
  void Connecting(int sock_fd);
  void HandleWrite();
  void HandleError();
  void Retry(int sock_fd);
  int RemoveAndResetChannel();
  void ResetChannel();

  EventLoop* loop_;
  InetAddress server_addr_;
  bool connect_; // atomic
  States state_;  // FIXME: use atomic variable
  SharedPtr<Channel> channel_;
  NewConnectionCallback new_connection_cb_;
  int retry_delay_msecs_;
};

} // namespace net
} // namespace fun
