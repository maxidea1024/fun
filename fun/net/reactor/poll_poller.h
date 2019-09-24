#pragma once

#include "fun/net/reactor/poller.h"
#include "fun/base/container/array.h"

struct pollfd;

namespace fun {
namespace net {

/**
 * IO Multiplexing with poll(2).
 */
class PollPoller : public Poller {
 public:
  PollPoller(EventLoop* loop);
  virtual ~PollPoller();

  // Poller interface
  Timestamp Poll(int32 timeout_msecs, ChannelList* active_channels) override;
  void UpdateChannel(Channel* channel) override;
  void RemoveChannel(Channel* channel) override;

 private:
  void FillActiveChannels(int32 event_count,
                          ChannelList* active_channels) const;

  typedef Array<pollfd> PollFdList;
  PollFdList poll_fds_;
};

} // namespace net
} // namespace fun
