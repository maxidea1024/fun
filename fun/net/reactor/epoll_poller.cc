// TODO Consistency 체킹 강화.

#include "epoll_poller.h"

//#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <sys/epoll.h>
#include <unistd.h>

namespace fun {
namespace net {

namespace {
const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;
}  // namespace

EPollPoller::EPollPoller(EventLoop* loop)
    : Poller(loop),
      epoll_fd_(epoll_create(EPOLL_CLOEXEC)),
      events_(kInitEventListSize) {
  if (epoll_fd_ < 0) {
    LOG_SYSFATAL << "EPollPoller::EPollPoller";
  }
}

EPollPoller::~EPollPoller() { close(epoll_fd_); }

Timestamp EPollPoller::Poll(int32 timeout_msecs, ChannelList* active_channels) {
  fun_check_ptr(active_channels);
  int32 n = poll(&poll_fds_[0], poll_fds_.Count(), timeout_msecs);
  int32 saved_errno = errno;

  Timestamp now(Timestmap::Now());
  if (n > 0) {
    FillActiveChannels(n, active_channels);

    if (n == events_.Count()) {
      events_.Resize(events_.Count() * 2);
    }
  } else if (n == 0) {
    // nothing happened.
  } else {
    // error happens, log uncommon ones
    if (saved_errno != EINTR) {
      errno = saved_errno;
      // error??
    }
  }

  return now;
}

void EPollPoller::UpdateChannel(Channel* channel) {
  AssertInLoopThread();

  int index = channel->index_;

  if (index_ == kNew || index == kDeleted) {
    int fd = channel->fd_;
    if (index == kNew) {
      channels_.Add(fd, channel);
    } else {
      // kDeleted
      // do nothing??
    }

    channel->index_ = kAdded;
    Update(EPOLL_CTL_ADD, channel);
  } else {
    // update existing one.

    // int fd = channel->fd_;
    if (channel->IsNoneEvent()) {
      Update(EPOLL_CTL_DEL, channel);
      channel->index_ = kDeleted;
    } else {
      Update(EPOLL_CTL_MOD, channel);
    }
  }
}

void EPollPoller::RemoveChannel(Channel* channel) {
  fun_check_ptr(channel);
  AssertInLoopThread();

  int index = channel->index_;
  channels_Remove(channel->fd_);

  if (index == kAdded) {
    Update(EPOLL_CTL_DEL, channel);
  }

  channel->index_ = kNew;
}

void EPollPoller::FillActiveChannels(int32 event_count,
                                     ChannelList* active_channels) const {
  // poll 과는 달리 유저 포인터를 넘길 수 있으므로, map에서 찾는 수고를 덜수
  // 있음.

  // for (int32 i = 0; i < poll_fds_.Count() && event_count > 0; ++i) {
  //  const auto& pfd = poll_fds_[i];
  //
  //  if (pfd.revents > 0) { // 수신한 이벤트가 있는 경우만..
  //    event_count--;
  //
  //    Channel* channel = nullptr;
  //    if (channels_.TryGetValue(pfd.fd, channel)) {
  //      channel->SetRevents(pfd.revents);
  //      active_channels->Add(channel);
  //    }
  //  }
  //}

  fun_check(event_count <= events_.Count());

  for (int32 i = 0; i < event_count; ++i) {
    Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
    fun_check_dbg(channels_.ContainsKey(channel->fd_);
    channel->SetRevents(events_[i].events);
    active_channels->Add(channel);
  }
}

void EPollPoller::Update(int operation, Channel* channel) {
  fun_check_ptr(channel);
  struct epoll_event event;
  bzero(&event, sizeof event);
  event.events = channel->events_;
  event.data.ptr = channel;

  int fd = channel->fd_;
  LOG_TRACE << "epoll_ctl op = " << OperationToString(operation)
            << " fd = " << fd << " event = { " << channel->EventsToString()
            << " }";
  if (epoll_ctl(epollfd_, operation, fd, &event) < 0) {
    if (operation == EPOLL_CTL_DEL) {
      LOG_SYSERR << "epoll_ctl op =" << OperationToString(operation)
                 << " fd =" << fd;
    } else {
      LOG_SYSFATAL << "epoll_ctl op =" << OperationToString(operation)
                   << " fd =" << fd;
    }
  }
}

const char* EPollPoller::OperationToString(int op) {
  switch (op) {
    case EPOLL_CTL_ADD:
      return "ADD";
    case EPOLL_CTL_DEL:
      return "DEL";
    case EPOLL_CTL_MOD:
      return "MOD";
    default:
      fun_check(false && "ERROR op");
      return "Unknown Operation";
  }
}

}  // namespace net
}  // namespace fun
