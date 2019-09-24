#pragma once

#include "fun/net/reactor/poller.h"

struct epoll_event;

namespace fun {
namespace net {

/// IO Multiplexing with poll(2).
class EPollPoller : public Poller
{
public:
  EPollPoller(EventLoop* loop);
  virtual ~EPollPoller();
  
  virtual Timestamp Poll(int timeout_msec, ChannelList* active_channels);
  virtual void UpdateChannel(Channel* channel);
  virtual void RemoveChannel(Channel* channel);
  
private:
  static const int kInitEventListSize = 16;
  
  static const char* OperatorToString(int op);

  void FillActiveChannels(int32 event_count, ChannelList* active_channels) const;
  
  void Update(int operation, Channel* channel);

  typedef Array<struct epoll_event> EventList;
  int epoll_fd_;
  EventList events_;
};

} // namespace net
} // namespace fun
