#include "fun/net/socket/socket.h"
#include "fun/net/socket/socket_impl.h"
#include "fun/net/socket/stream_socket_impl.h"

#include <algorithm>

#if PLATFORM_HAVE_FD_EPOLL
#include <sys/epoll.h>
#elif PLATFORM_HAVE_FD_POLL
#include <poll.h>
#endif

namespace fun {

Socket::Socket() : impl_(new StreamSocketImpl) {}

Socket::Socket(SocketImpl* impl) : impl_(impl) { fun_check_ptr(impl_); }

Socket::Socket(const Socket& rhs) : impl_(rhs.impl_) {
  fun_check_ptr(impl_);

  impl_->AddRef();
}

Socket& Socket::operator=(const Socket& rhs) {
  if (FUN_LIKELY(&rhs != this)) {
    if (impl_) {
      impl_->Release();
    }

    impl_ = rhs.impl_;

    if (impl_) {
      impl_->AddRef();
    }
  }

  return *this;
}

Socket::~Socket() { impl_->Release(); }

int32 Socket::Select(SocketList& read_list, SocketList& write_list,
                     SocketList& error_list, const Timespan& timeout) {
#if PLATFORM_HAVE_FD_EPOLL

  int32 epoll_size =
      read_list.Count() + write_list.Count() + error_list.Count();
  if (epoll_size == 0) {
    return 0;
  }

  int32 epoll_fd = -1;
  {
    struct epoll_event events_in[epoll_size];
    UnsafeMemory::Memzero(events_in, sizeof(events_in));
    struct epoll_event* eventLast = events_in;

    for (const auto& socket : read_list) {
      SOCKET fd = socket.GetSocketHandle();
      if (fd != FUN_INVALID_SOCKET) {
        struct epoll_event* e = events_in;
        for (; e != eventLast; ++e) {
          if (reinterpret_cast<Socket*>(e->data.ptr)->GetSocketHandle() == fd) {
            break;
          }
        }

        if (e == eventLast) {
          e->data.ptr = &(*it);
          ++eventLast;
        }

        e->events |= EPOLLIN;
      }
    }

    for (const auto& socket : write_list) {
      SOCKET fd = socket.GetSocketHandle();
      if (fd != FUN_INVALID_SOCKET) {
        struct epoll_event* e = events_in;
        for (; e != eventLast; ++e) {
          if (reinterpret_cast<Socket*>(e->data.ptr)->GetSocketHandle() == fd) {
            break;
          }
        }

        if (e == eventLast) {
          e->data.ptr = &(*it);
          ++eventLast;
        }

        e->events |= EPOLLOUT;
      }
    }

    for (const auto& socket : error_list) {
      SOCKET fd = socket.GetSocketHandle();
      if (fd != FUN_INVALID_SOCKET) {
        struct epoll_event* e = events_in;
        for (; e != eventLast; ++e) {
          if (reinterpret_cast<Socket*>(e->data.ptr)->GetSocketHandle() == fd) {
            break;
          }
        }

        if (e == eventLast) {
          e->data.ptr = &(*it);
          ++eventLast;
        }

        e->events |= EPOLLERR;
      }
    }

    epoll_size = eventLast - events_in;
    if (epoll_size == 0) {
      return 0;
    }

    epoll_fd = epoll_create(1);
    if (epoll_fd < 0) {
      SocketImpl::ThrowError(CStringLiteral("Can't create epoll queue"));
    }

    for (struct epoll_event* e = events_in; e != eventLast; ++e) {
      SOCKET fd = reinterpret_cast<Socket*>(e->data.ptr)->GetSocketHandle();
      if (fd != FUN_INVALID_SOCKET) {
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, e) < 0) {
          close(epoll_fd);
          SocketImpl::ThrowError(
              CStringLiteral("Can't insert socket to epoll queue: "));
        }
      }
    }
  }

  struct epoll_event events_out[epoll_size];
  UnsafeMemory::Memzero(events_out, sizeof(events_out));

  Timespan remaining_time(timeout);
  int32 rc;
  do {
    DateTime start(DateTime::UtcNow());
    rc = epoll_wait(epoll_fd, events_out, epoll_size,
                    remaining_time.TotalMilliseconds());
    if (rc < 0 && SocketImpl::GetLastError() == FUN_EINTR) {
      DateTime end(DateTime::UtcNow());
      const Timespan waited = end - start;
      if (waited < remaining_time) {
        remaining_time -= waited;
      } else {
        remaining_time = 0;
      }
    }
  } while (rc < 0 && SocketImpl::GetLastError() == FUN_EINTR);

  ::close(epoll_fd);

  if (rc < 0) {
    SocketImpl::ThrowError();
  }

  SocketList ready_read_list;
  SocketList ready_write_list;
  SocketList ready_error_list;
  for (int32 i = 0; i < rc; ++i) {
    if (events_out[i].events & EPOLLERR) {
      ready_error_list.Add(*reinterpret_cast<Socket*>(events_out[i].data.ptr));
    }

    if (events_out[i].events & EPOLLIN) {
      ready_read_list.Add(*reinterpret_cast<Socket*>(events_out[i].data.ptr));
    }

    if (events_out[i].events & EPOLLOUT) {
      ready_write_list.Add(*reinterpret_cast<Socket*>(events_out[i].data.ptr));
    }
  }
  Swap(read_list, ready_read_list);
  Swap(write_list, ready_write_list);
  Swap(error_list, ready_error_list);
  return read_list.Count() + write_list.Count() + error_list.Count();

#elif PLATFORM_HAVE_FD_POLL

  typedef fun::SharedPtr<pollfd, fun::ReferenceCounter,
                         fun::ReleaseArrayPolicy<pollfd> >
      SharedPollArray;

  nfds_t nfd = read_list.Count() + write_list.Count() + error_list.Count();
  if (0 == nfd) {
    return 0;
  }

  SharedPollArray pPollArr = new pollfd[nfd];

  int32 idx = 0;
  for (const auto& socket : read_list) {
    pPollArr[idx].fd = int32(socket.GetSocketHandle());
    pPollArr[idx++].events |= POLLIN;
  }

  SocketList::iterator begR = read_list.begin();
  SocketList::iterator endR = read_list.End();
  for (const auto& socket : write_list) {
    SocketList::iterator pos = std::find(begR, endR, *it);
    if (pos != endR) {
      pPollArr[pos - begR].events |= POLLOUT;
      --nfd;
    } else {
      pPollArr[idx].fd = int32(socket.GetSocketHandle());
      pPollArr[idx++].events |= POLLOUT;
    }
  }

  SocketList::iterator begW = write_list.begin();
  SocketList::iterator endW = write_list.End();
  for (const auto& socket : error_list) {
    SocketList::iterator pos = std::find(begR, endR, *it);
    if (pos != endR) {
      --nfd;
    } else {
      SocketList::iterator pos = std::find(begW, endW, *it);
      if (pos != endW) {
        --nfd;
      } else {
        pPollArr[idx++].fd = int32(socket.GetSocketHandle());
      }
    }
  }

  Timespan remaining_time(timeout);
  int32 rc;
  do {
    DateTime start(DateTime::UtcNow());
    rc = ::poll(pPollArr, nfd, timeout.TotalMilliseconds());
    if (rc < 0 && SocketImpl::GetLastError() == FUN_EINTR) {
      DateTime end(DateTime::UtcNow());
      const Timespan waited = end - start;
      if (waited < remaining_time) {
        remaining_time -= waited;
      } else {
        remaining_time = 0;
      }
    }
  } while (rc < 0 && SocketImpl::GetLastError() == FUN_EINTR);

  if (rc < 0) {
    SocketImpl::ThrowError();
  }

  SocketList ready_read_list;
  SocketList ready_write_list;
  SocketList ready_error_list;

  SocketList::iterator begE = error_list.begin();
  SocketList::iterator endE = error_list.End();
  for (int32 i = 0; i < nfd; ++i) {
    SocketList::iterator slIt =
        std::find_if(begR, endR, Socket::FDCompare(pPollArr[i].fd));
    if (POLLIN & pPollArr[i].revents && slIt != endR) {
      ready_read_list.Add(*slIt);
    }

    slIt = std::find_if(begW, endW, Socket::FDCompare(pPollArr[i].fd));
    if (POLLOUT & pPollArr[i].revents && slIt != endW) {
      ready_write_list.Add(*slIt);
    }

    slIt = std::find_if(begE, endE, Socket::FDCompare(pPollArr[i].fd));
    if (POLLERR & pPollArr[i].revents && slIt != endE) {
      ready_error_list.Add(*slIt);
    }
  }
  Swap(read_list, ready_read_list);
  Swap(write_list, ready_write_list);
  Swap(error_list, ready_error_list);
  return read_list.Count() + write_list.Count() + error_list.Count();

#else

  fd_set read_set;
  fd_set write_set;
  fd_set error_set;
  int32 nfd = 0;

  FD_ZERO(&read_set);
  for (const auto& socket : read_list) {
    const SOCKET fd = socket.GetSocketHandle();
    if (fd != FUN_INVALID_SOCKET) {
      if (int32(fd) > nfd) {
        nfd = int32(fd);
      }
      FD_SET(fd, &read_set);
    }
  }

  FD_ZERO(&write_set);
  for (const auto& socket : write_list) {
    const SOCKET fd = socket.GetSocketHandle();
    if (fd != FUN_INVALID_SOCKET) {
      if (int32(fd) > nfd) {
        nfd = int32(fd);
      }
      FD_SET(fd, &write_set);
    }
  }

  FD_ZERO(&error_set);
  for (const auto& socket : error_list) {
    const SOCKET fd = socket.GetSocketHandle();
    if (fd != FUN_INVALID_SOCKET) {
      if (int32(fd) > nfd) {
        nfd = int32(fd);
      }
      FD_SET(fd, &error_set);
    }
  }

  if (nfd == 0) {
    return 0;
  }

  Timespan remaining_time(timeout);
  int32 rc;
  do {
    struct timeval tv;
    tv.tv_sec = (long)remaining_time.TotalSeconds();
    tv.tv_usec = (long)remaining_time.Microseconds();

    DateTime start(DateTime::UtcNow());
    rc = select(nfd + 1, &read_set, &write_set, &error_set, &tv);
    if (rc < 0 && SocketImpl::GetLastError() == FUN_EINTR) {
      DateTime end(DateTime::UtcNow());
      const Timespan waited = end - start;
      if (waited < remaining_time) {
        remaining_time -= waited;
      } else {
        remaining_time = 0;
      }
    }
  } while (rc < 0 && SocketImpl::GetLastError() == FUN_EINTR);

  if (rc < 0) {
    SocketImpl::ThrowError();
  }

  SocketList ready_read_list;
  for (const auto& socket : read_list) {
    const SOCKET fd = socket.GetSocketHandle();
    if (fd != FUN_INVALID_SOCKET) {
      if (FD_ISSET(fd, &read_set)) {
        ready_read_list.Add(socket);
      }
    }
  }
  Swap(read_list, ready_read_list);

  SocketList ready_write_list;
  for (const auto& socket : write_list) {
    const SOCKET fd = socket.GetSocketHandle();
    if (fd != FUN_INVALID_SOCKET) {
      if (FD_ISSET(fd, &write_set)) {
        ready_write_list.Add(socket);
      }
    }
  }
  Swap(write_list, ready_write_list);

  SocketList ready_error_list;
  for (const auto& socket : error_list) {
    const SOCKET fd = socket.GetSocketHandle();
    if (fd != FUN_INVALID_SOCKET) {
      if (FD_ISSET(fd, &error_set)) {
        ready_error_list.Add(socket);
      }
    }
  }
  Swap(error_list, ready_error_list);

  return rc;

#endif  // PLATFORM_HAVE_FD_EPOLL
}

bool Socket::operator==(const Socket& rhs) const { return impl_ == rhs.impl_; }

bool Socket::operator!=(const Socket& rhs) const { return impl_ != rhs.impl_; }

bool Socket::operator<(const Socket& rhs) const { return impl_ < rhs.impl_; }

bool Socket::operator<=(const Socket& rhs) const { return impl_ <= rhs.impl_; }

bool Socket::operator>(const Socket& rhs) const { return impl_ > rhs.impl_; }

bool Socket::operator>=(const Socket& rhs) const { return impl_ >= rhs.impl_; }

void Socket::Close() { impl_->Close(); }

bool Socket::Poll(const Timespan& timeout, int32 mode) const {
  return impl_->Poll(timeout, mode);
}

int32 Socket::Available() const { return impl_->Available(); }

void Socket::SetSendBufferSize(int32 size) { impl_->SetSendBufferSize(size); }

int32 Socket::GetSendBufferSize() const { return impl_->GetSendBufferSize(); }

void Socket::SetReceiveBufferSize(int32 size) {
  impl_->SetReceiveBufferSize(size);
}

int32 Socket::GetReceiveBufferSize() const {
  return impl_->GetReceiveBufferSize();
}

void Socket::SetSendTimeout(const Timespan& timeout) {
  impl_->SetSendTimeout(timeout);
}

Timespan Socket::GetSendTimeout() const { return impl_->GetSendTimeout(); }

void Socket::SetReceiveTimeout(const Timespan& timeout) {
  impl_->SetReceiveTimeout(timeout);
}

Timespan Socket::GetReceiveTimeout() const {
  return impl_->GetReceiveTimeout();
}

void Socket::SetOption(int32 level, int32 option, int32 value) {
  impl_->SetOption(level, option, value);
}

void Socket::SetOption(int32 level, int32 option, uint32 value) {
  impl_->SetOption(level, option, value);
}

void Socket::SetOption(int32 level, int32 option, uint8 value) {
  impl_->SetOption(level, option, value);
}

void Socket::SetOption(int32 level, int32 option, const Timespan& value) {
  impl_->SetOption(level, option, value);
}

void Socket::SetOption(int32 level, int32 option, const IpAddress& value) {
  impl_->SetOption(level, option, value);
}

void Socket::GetOption(int32 level, int32 option, int32& value) const {
  impl_->GetOption(level, option, value);
}

void Socket::GetOption(int32 level, int32 option, uint32& value) const {
  impl_->GetOption(level, option, value);
}

void Socket::GetOption(int32 level, int32 option, uint8& value) const {
  impl_->GetOption(level, option, value);
}

void Socket::GetOption(int32 level, int32 option, Timespan& value) const {
  impl_->GetOption(level, option, value);
}

void Socket::GetOption(int32 level, int32 option, IpAddress& value) const {
  impl_->GetOption(level, option, value);
}

void Socket::SetLinger(bool on, int32 seconds) {
  impl_->SetLinger(on, seconds);
}

void Socket::GetLinger(bool& on, int32& seconds) const {
  impl_->GetLinger(on, seconds);
}

void Socket::SetNoDelay(bool flag) { impl_->SetNoDelay(flag); }

bool Socket::GetNoDelay() const { return impl_->GetNoDelay(); }

void Socket::SetKeepAlive(bool flag) { impl_->SetKeepAlive(flag); }

bool Socket::GetKeepAlive() const { return impl_->GetKeepAlive(); }

void Socket::SetReuseAddress(bool flag) { impl_->SetReuseAddress(flag); }

bool Socket::GetReuseAddress() const { return impl_->GetReuseAddress(); }

void Socket::SetReusePort(bool flag) { impl_->SetReusePort(flag); }

bool Socket::GetReusePort() const { return impl_->GetReusePort(); }

void Socket::SetOOBInline(bool flag) { impl_->SetOOBInline(flag); }

bool Socket::GetOOBInline() const { return impl_->GetOOBInline(); }

void Socket::SetBlocking(bool flag) { impl_->SetBlocking(flag); }

bool Socket::GetBlocking() const { return impl_->GetBlocking(); }

SocketImpl* Socket::GetImpl() const { return impl_; }

fun_socket_t Socket::GetSocketHandle() const {
  return impl_->GetSocketHandle();
}

InetAddress Socket::GetSocketAddress() const {
  return impl_->GetSocketAddress();
}

InetAddress Socket::GetPeerAddress() const { return impl_->GetPeerAddress(); }

bool Socket::Secure() const { return impl_->Secure(); }

bool Socket::SupportsIPv4() { return true; }

bool Socket::SupportsIPv6() {
#if FUN_PLATFORM_HAVE_IPv6
  return true;
#else
  return false;
#endif
}

void Socket::Init() { impl_->Init(); }

}  // namespace fun
