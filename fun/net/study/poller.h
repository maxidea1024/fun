#pragma once

#include "fun/net/io/event_loop.h"

namespace fun {
namespace net {

class Channel;

/// Base class for IO Multiplexing
///
/// This class doesn't own the Channel objects.
class Poller : Noncopyable
{
public:
  typedef Array<Channel*> ChannelList;

  Poller(EventLoop* loop);
  virtual ~Poller();

  /// Polls the I/O events.
  /// Must be called in the loop thread.
  virtual Timestamp Poll(int timeout_msec, ChannelList* active_channels);

  /// Changes the interested I/O events.
  /// Must be called in the loop thread.
  virtual void UpdateChannel(Channel* channel);

  /// Remove the channel, when it destructs.
  /// Must be called in the loop thread.
  virtual void RemoveChannel(Channel* channel);

  virtual bool HasChannel(Channel* channel) const;

  static Poller* CreateDefaultPoller(EventLoop* loop);

  void AssertInLoopThread() const
  {
    owner_loop_->AssertInLoopThread();
  }

protected:
  typedef Map<int, Channel*> ChannelMap;
  ChannelMap channels_;

private:
  EventLoop* owner_loop_;
};


} // namespace net
} // namespace fun
