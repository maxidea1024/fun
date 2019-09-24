#include "fun/net/reactor/tcp_server.h"

namespace fun {
namespace net {

TcpServer::TcpServer(EventLoop* loop, const InetAddress& listen_addr,
                     const string& name, Option option)
    : loop_(loop),
      name_(name),
      acceptor_(new Acceptor(loop, listen_addr, option == kReusePort)),
      thread_pool_(new EventLoopThreadPool(loop, name)),
      connection_cb_(DefaultConnectionCallback),
      message_cb_(DefaultMessageCallback),
      next_conn_id_(1) {
  acceptor_->SetNewConnectionCallback(
      [this](int fd, const InetAddress& peer_addr) {
        NewConnection(fd, peer_addr);
      });
}

TcpServer::~TcpServer() {
  loop_->AssertInLoopThread();

  for (auto& pair : connections_) {
    TcpConnectionPtr conn(pair.value);

    conn->Reset();
    conn->GetLoop()->RunInLoop([conn]() { conn.ConnectDestroyed(); });
  }
}

void TcpServer::SetThreadCount(int32 thread_count) {
  thread_pool_->SetThreadCount(thread_count);
}

void TcpServer::Start() {
  if (started_.GetAndSet(1) == 0) {
    thread_pool_->Start(thread_init_cb_);

    loop_->RunInLoop([acceptor_]() { acceptor_->Listen(); });
  }
}

void TcpServer::NewConnection(int fd, const InetAddress& peer_addr) {
  loop_->AssertInLoopThread();

  EventLoop* loop = thread_pool_->GetNextLoop();
  // TODO Makes unique name.
  // TODO 원래대로 라면 이름을 순차적으로 생성해야하는데...

  // char buf[64];
  // snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
  //++nextConnId_;
  // string connName = name_ + buf;

  string conn_name = name_ + Uuid::NewUuid().ToString();
  ++next_conn_id_;

  TcpConnectionPtr conn(
      new TcpConnection(loop, conn_name, fd, local_addr, peer_addr));
  connections_.Add(conn_name, conn);

  conn->SetConnectionCallback(connection_cb_);
  conn->SetMessageCallback(message_cb_);
  conn->SetWriteCompleteCallback(write_complete_cb_);
  // safe하지는 않을터...
  //위험을 제대로 파악하고 어찌 보완할지 살펴보자..
  conn->SetCloseCallback([this, conn]() { RemoveConnection(conn); });
  loop->RunInLoop([conn]() { conn.ConnectEstablish(); });
}

void TcpServer::RemoveConnection(const TcpConnectionPtr& conn) {
  loop_->RunInLoop([this, conn]() { RemoveConnectionLoop(conn); });
}

void TcpServer::RemoveConnectionInLoop(const TcpConnectionPtr& conn) {
  loop_->AssertInLoopThread();

  connections_.Remove(conn->GetName());

  EventLoop* loop = conn->GetLoop();
  loop->QueueInLoop([this, conn]() { conn.ConnectDestroyed(); });
}

}  // namespace net
}  // namespace fun
