#include "fun/net/reactor/poller.h"

#include "poll_poller.h"
#include "epoll_poller.h"

namespace fun {
namespace net {

Poller::Poller(EventLoop* loop)
  : owner_loop_(fun_check_ptr(loop)) {}

Poller::~Poller() {}

bool Poller::HasChannel(Channel* channel) const {
  fun_check_ptr(channel);
  AssertInLoopThread();

  Channel* test = nullptr;
  if (channels_.TryGetValue(channel->fd(), test)) {
    return test == channel;
  }

  return false;
}

Poller* Poller::Create(EventLoop* loop) {
  //TODO 뭔가 다른 방법을 사용하는게 좋을듯... command-line ??
  //서버 입장에서는 환경변수로 관리하는게 더 유리하다.
  //dockerize
  if (getenv("FUN_USE_POLL")) {
    return new PollPoller(loop);
  } else {
    return new EPollPoller(loop);
  }
}

} // namespace net
} // namespace fun
