#include "fun/net/io/poller_epoll.h"

#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <sys/epoll.h>
#include <unistd.h>

namespace fun {
namespace net {


// On Linux, the constants of poll(2) and epoll(4)
// are expected to be the same.
BOOST_STATIC_ASSERT(EPOLLIN == POLLIN);
BOOST_STATIC_ASSERT(EPOLLPRI == POLLPRI);
BOOST_STATIC_ASSERT(EPOLLOUT == POLLOUT);
BOOST_STATIC_ASSERT(EPOLLRDHUP == POLLRDHUP);
BOOST_STATIC_ASSERT(EPOLLERR == POLLERR);
BOOST_STATIC_ASSERT(EPOLLHUP == POLLHUP);

namespace
{
  const int kNew = -1;
  const int kAdded = 1;
  const int kDeleted = 2;
}



EPollPoller::EPollPoller(EventLoop* loop)
  : Poller(loop)
  , epoll_fd_(::epoll_create1(EPOLL_CLOEXEC)),
  , events_(kInitEventListSize)
{
  if (epoll_fd_ < 0)
  {
    //TODO error logging..
  }
}


EPollPoller::~EPollPoller()
{
  ::close(epoll_fd_);
}


Timestamp EPollPoller::Poll(int timeout_msec, ChannelList* active_channels)
{
  LOG_TRACE << "fd total count " << channels_.size();
  int event_count = ::epoll_wait(epoll_fd_,
                               &*events_.begin(),
                               static_cast<int>(events_.size()),
                               timeout_msec);
  int saved_errno = errno;
  Timestamp now(Timestamp::Now());
  if (event_count > 0)
  {
    LOG_TRACE << event_count << " events happened";
    FillActiveChannels(event_count, active_channels);
    if (ImplicitCast<size_t>(event_count) == events_.size())
    {
      events_.resize(events_.size()*2);
    }
  }
  else if (event_count == 0)
  {
    LOG_TRACE << "nothing happened";
  }
  else
  {
    // error happens, log uncommon ones
    if (saved_errno != EINTR)
    {
      errno = saved_errno;
      LOG_SYSERR << "EPollPoller::poll()";
    }
  }
  return now;
}


void EPollPoller::UpdateChannel(Channel* channel)
{
  Poller::AssertInLoopThread();
  const int index = channel->index();
  LOG_TRACE << "fd = " << channel->fd()
    << " events = " << channel->events() << " index = " << index;
  if (index == kNew || index == kDeleted)
  {
    // a new one, add with EPOLL_CTL_ADD
    int fd = channel->fd();
    if (index == kNew)
    {
      assert(channels_.find(fd) == channels_.end());
      channels_[fd] = channel;
    }
    else // index == kDeleted
    {
      assert(channels_.find(fd) != channels_.end());
      assert(channels_[fd] == channel);
    }

    channel->set_index(kAdded);
    Update(EPOLL_CTL_ADD, channel);
  }
  else
  {
    // update existing one with EPOLL_CTL_MOD/DEL
    int fd = channel->fd();
    (void)fd;
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    assert(index == kAdded);
    if (channel->isNoneEvent())
    {
      Update(EPOLL_CTL_DEL, channel);
      channel->set_index(kDeleted);
    }
    else
    {
      Update(EPOLL_CTL_MOD, channel);
    }
  }
}


void EPollPoller::RemoveChannel(Channel* channel)
{
  Poller::AssertInLoopThread();
  int fd = channel->fd();
  LOG_TRACE << "fd = " << fd;
  assert(channels_.find(fd) != channels_.end());
  assert(channels_[fd] == channel);
  assert(channel->isNoneEvent());
  int index = channel->index();
  assert(index == kAdded || index == kDeleted);
  size_t n = channels_.erase(fd);
  (void)n;
  assert(n == 1);

  if (index == kAdded)
  {
    Update(EPOLL_CTL_DEL, channel);
  }
  channel->set_index(kNew);
}


const char* EPollPoller::OperatorToString(int op)
{
  switch (op)
  {
    case EPOLL_CTL_ADD:
      return "ADD";
    case EPOLL_CTL_DEL:
      return "DEL";
    case EPOLL_CTL_MOD:
      return "MOD";
    default:
      assert(false && "ERROR op");
      return "Unknown Operation";
  }
}


void EPollPoller::FillActiveChannels(int32 event_count, ChannelList* active_channels) const
{
  assert(ImplicitCast<size_t>(event_count) <= events_.size());
  for (int i = 0; i < event_count; ++i)
  {
    Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
#ifndef NDEBUG
    int fd = channel->fd();
    ChannelMap::const_iterator it = channels_.find(fd);
    assert(it != channels_.end());
    assert(it->second == channel);
#endif
    channel->set_revents(events_[i].events);
    active_channels->push_back(channel);
  }
}


void EPollPoller::Update(int operation, Channel* channel)
{
  struct epoll_event event;
  bzero(&event, sizeof event);
  event.events = channel->events();
  event.data.ptr = channel;
  int fd = channel->fd();
  LOG_TRACE << "epoll_ctl op = " << operationToString(operation)
    << " fd = " << fd << " event = { " << channel->eventsToString() << " }";
  if (::epoll_ctl(epoll_fd_, operation, fd, &event) < 0)
  {
    if (operation == EPOLL_CTL_DEL)
    {
      LOG_SYSERR << "epoll_ctl op =" << operationToString(operation) << " fd =" << fd;
    }
    else
    {
      LOG_SYSFATAL << "epoll_ctl op =" << operationToString(operation) << " fd =" << fd;
    }
  }
}


} // namespace net
} // namespace fun
