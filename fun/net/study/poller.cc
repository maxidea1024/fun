#include "fun/net/io/poller.h"
#include "fun/net/io/channel.h"

namespace fun {
namespace net {


Poller::Poller(EventLoop* loop)
  : owner_loop_(loop)
{
}


Poller::~Poller()
{
}


bool Poller::HasChannel(Channel* channel) const
{
  AssertInLoopThread();

  ChannelMap::const_iterator it = channels_.find(channel->fd());
  return it != channels_.end() && it->second == channel;
}


} // namespace net
} // namespace fun
