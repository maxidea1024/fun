﻿#include "poll_poller.h"

#include <assert.h>
#include <errno.h>
#include <poll.h>

namespace fun {
namespace net {
namespace reactor {

PollPoller::PollPoller(EventLoop* loop) : Poller(loop) {}

PollPoller::~PollPoller() {}

Timestamp PollPoller::Poll(int32 timeout_msecs, ChannelList* active_channels) {
  fun_check_ptr(active_channels);
  int32 n = poll(&poll_fds_[0], poll_fds_.Count(), timeout_msecs);
  int32 saved_errno = errno;

  Timestamp now(Timestmap::Now());
  if (n > 0) {
    FillActiveChannels(n, active_channels);
  } else if (n == 0) {
    // nothing happened.
  } else {
    if (saved_errno != EINTR) {
      errno = saved_errno;
      // error??
    }
  }

  return now;
}

void PollPoller::UpdateChannel(Channel* channel) {
  fun_check_ptr(channel);
  AssertInLoopThread();

  if (channel->index_ < 0) {
    // a new one, add to poll_fds_
    struct pollfd pfd;
    pfd.fd = channel->fd_;
    pfd.events = static_cast<short>(channel->events_);
    pfd.revents = 0;
    int32 index = poll_fds_.Count() - 1;
    channel->index_ = index;  // poll slot index
    channels_.Add(pfd.fd, channel);
  } else {
    // update existing one.

    fun_check(channels_.ContainsKey(channel->fd_));
    fun_check(channels_.FindRef(channel->fd_) == channel);
    fun_check(poll_fds_.IsValidIndex(channel->index_));
    fun_check(poll_fds_[channel->index_].fd == channel->fd_ ||
              poll_fds_[channel->index_].fd == (-channel->fd_ - 1));

    auto& pfd = poll_fds_[channel->index_];
    pfd.fd = channel->fd_;
    pfd.events = static_cast<short>(channel->events_);
    pfd.revents = 0;

    if (channel->IsNoneEvent()) {
      // ignore this pollfd
      pfd.fd = -channel->fd_ - 1;
    }
  }
}

void PollPoller::RemoveChannel(Channel* channel) {
  fun_check_ptr(channel);
  AssertInLoopThread();

  int32 index = channel->index_;
  const auto& pfd = poll_fds_[index];
  channels_.Remove(channel->fd_);
  if (index == poll_fds_.Count() - 1) {
    // 뒷쪽 하나 제거.
    poll_fds_.PopBack();
  } else {
    // memmove를 피하기 위해서 트릭을 사용함.
    // remove swap을 하면 되는거아닌가??

    int32 channel_at_end = poll_fds_.Back().fd;
    // iter_swap(pollfds_.begin()+idx, pollfds_.end()-1);
    Swap(poll_fds_[index], poll_fds_[poll_fds_.Count() - 1]);
    if (channel_at_end < 0) {
      // TODO 이부분에 대한 이해가 필요함.
      channel_at_end = -channel_at_end - 1;
    }
    channels_[channel_at_end]->index_ = index;
    poll_fds_.PopBack();
  }
}

void PollPoller::FillActiveChannels(int32 event_count,
                                    ChannelList* active_channels) const {
  fun_check_ptr(active_channels);
  for (int32 i = 0; i < poll_fds_.Count() && event_count > 0; ++i) {
    const auto& pfd = poll_fds_[i];

    if (pfd.revents > 0) {  // 수신한 이벤트가 있는 경우만..
      event_count--;

      Channel* channel = nullptr;
      if (channels_.TryGetValue(pfd.fd, channel)) {
        channel->SetRevents(pfd.revents);
        active_channels->Add(channel);
      }
    }
  }

  // TODO 이게 맞을려나??
  fun_check(event_count == 0);
}

}  // namespace reactor
}  // namespace net
}  // namespace fun
