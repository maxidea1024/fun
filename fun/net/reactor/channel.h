#pragma once

#include "fun/base/function.h"
#include "fun/base/shared_ptr.h"
#include "fun/base/timestamp.h"
#include "fun/net/net.h"

namespace fun {
namespace net {
namespace reactor {

/**
 * A selectable I/O channel.
 *
 * This class doesn't own the file descriptor.
 * The file descriptor could be a socket,
 * an eventfd, a timerfd, or a signalfd
 */
class Channel : Noncopyable {
 public:
  friend class EventLoop;
  friend class Poller;

  typedef Function<void()> EventCallback;
  typedef Function<void(const Timestamp&)> ReadEventCallback;

  Channel(EventLoop* loop, int fd);
  ~Channel();

  void HandleEvent(const Timestamp& received_time);

  void SetReadCallback(const ReadEventCallback& cb) { read_cb_ = cb; }

  void SetWriteCallback(const EventCallback& cb) { write_cb_ = cb; }

  void SetCloseCallback(const EventCallback& cb) { close_cb_ = cb; }

  void SetErrorCallback(const EventCallback& cb) { error_cb_ = cb; }

  // 레퍼런스 홀더...
  void Tie(SharedPtr<void>&);

  int GetFd() const { return fd_; }

  int GetEvents() { return events_; }

  void SetRevents(int revents) { revents_ = revents; }

  // IsNonInteresting ?
  void IsNoneEvent() const { return events_ == kNoneEvent; }

  void EnableReading() {
    events_ |= kReadEvent;
    Update();
  }

  void DisableReading() {
    events_ &= ~kReadEvent;
    Update();
  }

  void EnableWriting() {
    events_ |= kWriteEvent;
    Update();
  }

  void DisableWriting() {
    events_ &= ~kWriteEvent;
    Update();
  }

  void DisableAll() {
    events_ = kNoneEvent;
    Update();
  }

  bool IsWriting() const { return (events_ & kWriteEvent) != 0; }

  bool IsReading() const { return (events_ & kReadEvent) != 0; }

  // for Poller
  int GetIndex() { return index_; }

  void SetIndex(int index) { index_ = index; }

  // for debug
  String ReventsToString() const;
  String EventsToString() const;

  void DoNotLogHup() { log_hup_ = false; }

  EventLoop* GetOwnerLoop() { return loop_; }

  void Remove();

 private:
  static String EventsToString(int fd, int ev);

  void Update();
  void HandleEventWithGuard(const Timestamp& received_time);

  static const int kNoneEvent;
  static const int kReadEvent;
  static const int kWriteEvent;

  EventLoop* loop_;
  const int fd_;
  int events_;
  int revents_;  // it's the received event types of epoll or poll
  int index_;    // used by Poller.
  bool log_hup_;

  WeakPtr<void> tie_;
  bool tied_;
  bool events_handling_;
  bool added_to_loop_;
  ReadEventCallback read_cb_;
  EventCallback write_cb_;
  EventCallback close_cb_;
  EventCallback error_cb_;
};

}  // namespace reactor
}  // namespace net
}  // namespace fun
