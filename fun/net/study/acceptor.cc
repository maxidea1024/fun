#include "fun/net/io/acceptor.h"

namespace fun {
namespace net {

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listen_addr, bool reuse_port)
  : loop_(loop)
  , listen_socket_(sockets::createNonblockingOrDie(listen_addr.family()))
  , listen_channel_(loop, listen_socket_.fd())
  , listenning_(false)
  , idle_fd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))
{
  assert(idle_fd_ >= 0);
  listen_socket_.setReuseAddr(true);
  listen_socket_.setReusePort(reuseport);
  listen_socket_.bindAddress(listen_addr);
  listen_channel_.setReadCallback(
      boost::bind(&Acceptor::handleRead, this));
}


Acceptor::~Acceptor()
{
  listen_channel_.disableAll();
  listen_channel_.remove();
  ::close(idle_fd_);
}


void Acceptor::Listen()
{
  loop_->AssertInLoopThread();
  listenning_ = true;
  listen_socket_.listen();
  listen_channel_.enableReading();
}


void Acceptor::HandleRead()
{
  loop_->AssertInLoopThread();

  InetAddress client_addr;
  //FIXME loop until no more
  int client_fd = listen_socket_.accept(&client_addr);
  if (client_fd >= 0)
  {
    // string hostport = client_addr.ToIpPort();
    // LOG_TRACE << "Accepts of " << hostport;
    if (new_connection_cb_)
    {
      new_connection_cb_(client_fd, client_addr);
    }
    else
    {
      sockets::close(client_fd);
    }
  }
  else
  {
    LOG_SYSERR << "in Acceptor::handleRead";
    // Read the section named "The special problem of
    // accept()ing when you can't" in libev's doc.
    // By Marc Lehmann, author of libev.
    if (errno == EMFILE)
    {
      ::close(idle_fd_);
      idle_fd_ = ::accept(listen_socket_.fd(), NULL, NULL);
      ::close(idle_fd_);
      idle_fd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
    }
  }
}


} // namespace net
} // namespace fun
