#pragma once

#include "fun/base/container/array.h"
#include "fun/net/reactor/poller.h"

struct epoll_event;

namespace fun {
namespace net {

/**
 * IO Multiplexing with poll(2).
 */
class EPollPoller : Poller {
 public:
  EPollPoller(EventLoop* loop);
  virtual ~EPollPoller();

  // Poller interface
  Timestamp Poll(int32 timeout_msecs, ChannelList* active_channels) override;
  void UpdateChannel(Channel* channel) override;
  void RemoveChannel(Channel* channel) override;

 private:
  static const int kInitEventListSize = 16;

  static const char* OperationToString(int op);

  void FillActiveChannels(int32 event_count,
                          ChannelList* active_channels) const;

  void Update(int operation, Channel* channel);

  int epoll_fd_;

  typedef Array<epoll_event> EventList;
  EventList events_;
};

}  // namespace net
}  // namespace fun
