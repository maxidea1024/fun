#pragma once

#include "fun/net/reactor/poller.h"

struct pollfd;

namespace fun {
namespace net {

/// IO Multiplexing with poll(2).
class PollPoller : public Poller
{
public:
  PollPoller(EventLoop* loop);
  virtual ~PollPoller();
  
  virtual Timestamp Poll(int timeout_msec, ChannelList* active_channels);
  virtual void UpdateChannel(Channel* channel);
  virtual void RemoveChannel(Channel* channel);
  
private:
  void FillActiveChannels(int32 event_count, ChannelList* active_channels) const;

  typedef Array<struct pollfd> PollFdList;
  PollFdList poll_fds_;
};

} // namespace net
} // namespace fun
