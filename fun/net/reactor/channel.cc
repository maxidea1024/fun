#include "fun/net/reactor/channel.h"

namespace fun {
namespace net {

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop* loop, int fd)
    : loop_(loop),
      fd_(fd),
      events_(0),
      revents_(0),
      index_(-1),
      log_hup_(true),
      tied_(false),
      events_handling_(false),
      added_to_loop_(false) {}

Channel::~Channel() {
  fun_check(!events_handling_);
  fun_check(!added_to_loop_);

  if (loop_->IsInLoopThread()) {
    fun_check(!loop_->HasChannel(this));
  }
}

void Channel::HandleEvent(const Timestamp& received_time) {
  if (tied_) {
    SharedPtr<void> guard = tie_.Lock();
    if (guard) {
      HandleEventWithGuard(received_time);
    }
  } else {
    HandleEventWithGuard(received_time);
  }
}

void Channel::HandleEventWithGuard(const Timestamp& received_time) {
  events_handling_ = true;

  LOG_TRACE << ReventsToString();

  if ((revents_ & POLLHUP) && !(revents_ & POLLIN)) {
    if (log_hup_) {
      LOG_WARN << "fd = " << fd_ << " Channel::HandleEvent() POLLHUP";
    }

    if (close_cb_) {
      close_cb_();
    }
  }

  if (revents_ & POLLNVAL) {
    LOG_WARN << "fd = " << fd_ << " Channel::HandleEvent() POLLNVAL";
  }

  if (revents_ & (POLLERR | POLLNVAL)) {
    if (error_cb_) {
      error_cb_();
    }
  }

  if (revents_ & (POLLIN | POLLPRI | POLLRDHUP)) {
    if (read_cb_) {
      read_cb_(receiveTime);
    }
  }

  if (revents_ & POLLOUT) {
    if (write_cb_) {
      write_cb_();
    }
  }

  events_handling_ = false;
}

void Channel::Update() {
  added_to_loop_ = true;
  loop_->UpdateChannel(this);
}

void Channel::Remove() {
  fun_check(IsNoneEvent());
  added_to_loop_ = false;
  loop_->RemoveChannel(this);
}

void Channel::Tie(SharedPtr<void>& ptr) {
  tie_ = ptr;
  tied_ = tie_.IsValid();
}

string Channel::ReventsToString() const {
  return EventsToString(fd_, revents_);
}

string Channel::EventsToString() const { return EventsToString(fd_, events_); }

string Channel::EventsToString(int fd, int ev) {
  std::ostringstream oss;
  oss << fd << ": ";
  if (ev & POLLIN) {
    oss << "IN ";
  }
  if (ev & POLLPRI) {
    oss << "PRI ";
  }
  if (ev & POLLOUT) {
    oss << "OUT ";
  }
  if (ev & POLLHUP) {
    oss << "HUP ";
  }
  if (ev & POLLRDHUP) {
    oss << "RDHUP ";
  }
  if (ev & POLLERR) {
    oss << "ERR ";
  }
  if (ev & POLLNVAL) {
    oss << "NVAL ";
  }

  return oss.str().c_str();
}

}  // namespace net
}  // namespace fun
