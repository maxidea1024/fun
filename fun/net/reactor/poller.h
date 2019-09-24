#pragma once

#include "fun/net/reactor/event_loop.h"
#include "fun/base/container/map.h"

namespace fun {
namespace net {

class Channel;

/**
 * Base class for IO Multiplexing
 *
 * This class doesn't own the Channel objects.
 */
class Poller : Noncopyable {
 public:
  typedef Array<Channel*> ChannelList;

  Poller(EventLoop* loop);
  virtual ~Poller();

  /**
   * Poll I/O events.
   */
  virtual Timestamp Poll(int32 timeout_msecs, ChannelList* active_channels) = 0;

  /**
   * Changes the interested I/O events.
   *
   * \warning Must be called in the loop thread.
   */
  virtual void UpdateChannel(Channel* channel) = 0;

  /**
   * Remove the channel, when it destructs.
   *
   * \warning Must be called in the loop thread.
   */
  virtual void RemoveChannel(Channel* channel) = 0;

  virtual bool HasChannel(Channel* channel) const;

  static Poller* Create(EventLoop* loop);

  void AssertInLoopThread() const {
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
