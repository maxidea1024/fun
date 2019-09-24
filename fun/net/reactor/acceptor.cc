#include "fun/net/reactor/acceptor.h"

namespace fun {
namespace net {
namespace reactor {

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listen_addr,
                   bool reuse_port)
    : loop_(loop),
      accept_socket_(
          sockets::CreateNonBlockingOrDie(listen_addr.GetAddressFamily())),
      accept_channel_(loop, accept_socket_.fd()),
      listening_(false),
      idle_fd_(::open("/dev/null", O_RDONLY | O_CLOEXEC)) {
  fun_check(idle_fd_ >= 0);
  accept_socket_.SetReuseAddr(true);
  accept_socket_.SetReusePort(reuse_port);
  accept_socket_.BindAddress(listen_addr);
  accept_channel_.SetReadCallback([this]() { HandleRead(); });
}

Acceptor::~Acceptor() {
  accept_channel_.DisableAll();
  accept_channel_.Remove();
  ::close(idle_fd_);
}

void Acceptor::Listen() {
  loop_->AssertInLoopThread();
  listenning_ = true;
  accept_socket_.Listen();
  accept_channel_.EnableReading();
}

void Acceptor::HandleRead() {
  loop_->AssertInLoopThread();

  InetAddress peer_addr;
  // FIXME loop until no more
  int conn_fd = accept_socket_.Accept(&peer_addr);
  if (conn_fd >= 0) {
    // string hostport = peer_addr.ToIpPort();
    // LOG_TRACE << "Accepts of " << hostport;
    if (new_connection_cb_) {
      new_connection_cb_(conn_fd, peer_addr);
    } else {
      sockets::close(conn_fd);
    }
  } else {
    LOG_SYSERR << "in Acceptor::HandleRead";
    // Read the section named "The special problem of
    // accept()ing when you can't" in libev's doc.
    // By Marc Lehmann, author of libev.
    if (errno == EMFILE) {
      ::close(idle_fd_);
      idle_fd_ = ::accept(accept_socket_.fd(), NULL, NULL);
      ::close(idle_fd_);
      idle_fd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
    }
  }
}

}  // namespace reactor
}  // namespace net
}  // namespace fun
