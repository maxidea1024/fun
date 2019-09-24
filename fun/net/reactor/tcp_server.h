#pragma once

#include "fun/base/container/map.h"
#include "fun/base/function.h"
#include "fun/base/shared_ptr.h"
#include "fun/net/net.h"

namespace fun {
namespace net {
namespace reactor {

class Acceptor;
class EventLoop;
class EventLoopThreadPool;

/**
TODO
*/
class TcpServer : Noncopyable {
 public:
  typedef Function<void(EventLoop*)> ThreadInitCallback;

  enum Option {
    kNoReusePort,
    kReusePort,
  };

  TcpServer(EventLoop* loop, const InetAddress& listen_addr, const string& name,
            Option option = kNoReusePort);

  ~TcpServer();

  EventLoop* GetLoop() const { return loop_; }

  void SetThreadCount(int32 thread_count);

  void SetThreadInitCallback(const ThreadInitCallback& cb) {
    thread_init_cb_ = cb;
  }

  SharedPtr<EventLoopThreadPool> GetThreadPool() { return thread_pool_; }

  void Start();

  void SetConnectionCallback(const ConnectionCallback& cb) {
    connection_cb_ = cb;
  }

  void SetMessageCallback(const MessageCallback& cb) { message_cb_ = cb; }

  void SetWriteCompleteCallback(const WriteCompleteCallback& cb) {
    write_complete_cb_ = cb;
  }

 private:
  void NewConnection(int fd, const InetAddress& peer_addr);
  void RemoveConnection(const TcpConnectionPtr& conn);
  void RemoveConnectionInLoop(const TcpConnectionPtr& conn);

  typedef Map<String, TcpConnectionPtr> ConnectionMap;

  EventLoop* loop_;
  SharedPtr<Acceptor> acceptor_;
  SharedPtr<EventLoopThreadPool> thread_pool_;
  ConnectionCallback connection_cb_;
  MessageCallback message_cb_;
  WriteCompleteCallback write_complete_cb_;
  ThreadInitCallback thread_init_cb_;
  AtomicCounter started_;
  int next_conn_id_;
  ConnectionMap connections_;
};

}  // namespace reactor
}  // namespace net
}  // namespace fun
