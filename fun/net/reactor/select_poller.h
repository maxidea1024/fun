#pragma once

#include "fun/net/net.h"
#include "fun/net/reactor/poller.h"
#include "fun/base/container/array.h"

//TODO 주말내내 작업을 해보도록 하자.

namespace fun {
namespace net {

/**
IO Multiplexing with select.
*/
class SelectPoller : Poller {
 public:
  SelectPoller(EventLoop* loop);
  virtual ~SelectPoller();

  // Poller interface
  Timestamp Poll(int32 timeout_msecs, ChannelList* active_channels) override;
  void UpdateChannel(Channel* channel) override;
  void RemoveChannel(Channel* channel) override;

 private:
  void FillActiveChannels(int32 event_count,
                          ChannelList* active_channels) const;

  typedef Array<int> FdList;
  FdList fds_;
};

} // namespace net
} // namespace fun
