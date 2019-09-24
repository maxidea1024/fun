#include "CorePrivatePCH.h"
#include "Net/socket/NetSocket.h"
#include "Net/socket/SocketImpl.h"

#pragma warning(disable : 4996) // warning C4996: 'GetVersionExW': deprecated

namespace fun {

namespace {

static bool CheckIsBrokenTimeout() {
#if FUN_BROKEN_TIMEOUTS
  return true;
#elif FUN_PLATFORM_WINDOWS_FAMILY
  // on Windows 7 and lower, socket timeouts have a minimum of 500ms, use poll for timeouts on this case
  // https://social.msdn.microsoft.com/Forums/en-US/76620f6d-22b1-4872-aaf0-833204f3f867/minimum-timeout-value-for-sorcvtimeo
  OSVERSIONINFO vi;
  vi.dwOSVersionInfoSize = sizeof(vi);
  if (GetVersionEx(&vi) == 0) return true; //throw SystemException(CStringLiteral("Cannot get OS version information"));
  return vi.dwMajorVersion < 6 || (vi.dwMajorVersion == 6 && vi.dwMinorVersion < 2);
#endif
  return false;
}

} // namespace

SocketImpl::SocketImpl()
  : fd_(INVALID_SOCKET),
  , blocking_(true),
  , broken_timeout_(CheckIsBrokenTimeout()) {}

SocketImpl::SocketImpl(SOCKET fd)
  : fd_(fd),
  , blocking_(true),
  , broken_timeout_(CheckIsBrokenTimeout()) {}

SocketImpl::~SocketImpl() {
  Close();
}

SocketImpl* SocketImpl::AcceptConnection(InetAddress& client_addr) {
  if (fd_ == INVALID_SOCKET) {
    throw InvalidSocketException();
  }

  sockaddr_in6 sa;
  fun_socklen_t sa_len = sizeof(sa);

  SOCKET accepted_fd;
  do {
    accepted_fd = accept(fd_, (sockaddr*)&sa, &sa_len);
  } while (accepted_fd == INVALID_SOCKET && GetLastError() == FUN_EINTR);

  if (accepted_fd != INVALID_SOCKET) {
    client_addr = InetAddress(sa);
    return new SocketImpl(accepted_fd);
  } else {
    ThrowError();
    return nullptr;
  }
}

void SocketImpl::Connect(const InetAddress& address) {
  if (fd_ == INVALID_SOCKET) {
    Init();
  }

  int32 rc;
  do {
    sockaddr_in6 sa;
    address.ToNative(sa);

    rc = connect(fd_, (const sockaddr*)&sa, sizeof(sa));
  } while (rc != 0 && GetLastError() == FUN_EINTR);

  if (rc != 0) {
    const int32 error = GetLastError();
    ThrowError(error, *address.ToString());
  }
}

void SocketImpl::Connect(const InetAddress& address, const Timespan& timeout) {
  if (fd_ == INVALID_SOCKET) {
    Init();
  }

  SetBlocking(false);

  try {
    sockaddr_in6 sa;
    address.ToNative(sa);

    int32 rc = connect(fd_, (const sockaddr*)&sa, sizeof(sa));
    if (rc != 0) {
      int32 error = GetLastError();
      if (error != FUN_EINPROGRESS && error != FUN_EWOULDBLOCK) {
        ThrowError(error, address.ToString());
      }

      if (!Poll(timeout, SELECT_READ | SELECT_WRITE | SELECT_ERROR)) {
        throw TimeoutException(CStringLiteral("connect timed out"), address.ToString());
      }

      error = GetSocketError();
      if (error != 0) {
        ThrowError(error);
      }
    }
  } catch (Exception&) {
    SetBlocking(true);
    throw;
  }

  SetBlocking(true);
}

void SocketImpl::ConnectNB(const InetAddress& address) {
  if (fd_ == INVALID_SOCKET) {
    Init();
  }

  SetBlocking(false);

  sockaddr_in6 sa;
  address.ToNative(sa);

  const int32 rc = connect(fd_, (const sockaddr*)&sa, sizeof(sa));
  if (rc != 0) {
    const int32 error = GetLastError();
    if (error != FUN_EINPROGRESS && error != FUN_EWOULDBLOCK) {
      ThrowError(error, address.ToString());
    }
  }
}

void SocketImpl::Bind(const InetAddress& address, bool reuse_addr) {
  Bind(address, reuse_addr, false);
}

void SocketImpl::Bind(const InetAddress& address, bool reuse_addr, bool reuse_port) {
  if (fd_ == INVALID_SOCKET) {
    Init();
  }

  if (reuse_addr) {
    SetReuseAddress(true);
  }

  if (reuse_port) {
    SetReusePort(true);
  }

  sockaddr_in6 sa;
  address.ToNative(sa);

  const int32 rc = bind(fd_, (const sockaddr*)&sa, sizeof(sa));
  if (rc != 0) {
    ThrowError(rc, *address.ToString());
  }
}

void SocketImpl::Bind6(const InetAddress& address, bool reuse_addr, bool ipv6_only) {
  Bind6(address, reuse_addr, false, ipv6_only);
}

void SocketImpl::Bind6(const InetAddress& address, bool reuse_addr, bool reuse_port, bool ipv6_only) {
  //if (address.Family() != InetAddress::IPv6)
  if (address.GetHost().IsIPv4MappedToIPv6()) //IPv6 전용임! {
    throw InvalidArgumentException(CStringLiteral("address must be an IPv6 address"));
  }

  if (fd_ == INVALID_SOCKET) {
    Init();
  }

#ifdef IPV6_V6ONLY
  SetOption(IPPROTO_IPV6, IPV6_V6ONLY, ipv6_only ? 1 : 0);
#else
  if (ipv6_only) {
    throw NotImplementedException(CStringLiteral("IPV6_V6ONLY not defined."));
  }
#endif

  if (reuse_addr) {
    SetReuseAddress(true);
  }

  if (reuse_port) {
    SetReusePort(true);
  }

  sockaddr_in6 sa;
  address.ToNative(sa);

  const int32 rc = bind(fd_, (const sockaddr*)&sa, sizeof(sa));
  if (rc != 0) {
    ThrowError(rc, *address.ToString());
  }
}

void SocketImpl::Listen(int32 backlog) {
  if (fd_ == INVALID_SOCKET) {
    throw InvalidSocketException();
  }

  const int32 rc = listen(fd_, backlog);
  if (rc != 0) {
    ThrowError(rc);
  }
}

//TODO 핸들값은 그대로 둔채 소켓만 닫아주어서, 오류를 캐치할 수 있도록 하는 옵션을 제공하는것도 좋을듯 싶은데...
void SocketImpl::Close() {
  if (fd_ != INVALID_SOCKET) {
    fun_closesocket(fd_);
    fd_ = INVALID_SOCKET;
  }
}

void SocketImpl::ShutdownReceive() {
  if (fd_ == INVALID_SOCKET) {
    throw InvalidSocketException();
  }

  const int32 rc = shutdown(fd_, 0);
  if (rc != 0) {
    ThrowError(rc);
  }
}

void SocketImpl::ShutdownSend() {
  if (fd_ == INVALID_SOCKET) {
    throw InvalidSocketException();
  }

  const int32 rc = shutdown(fd_, 1);
  if (rc != 0) {
    ThrowError(rc);
  }
}

void SocketImpl::Shutdown() {
  if (fd_ == INVALID_SOCKET) {
    throw InvalidSocketException();
  }

  const int32 rc = shutdown(fd_, 2);
  if (rc != 0) {
    ThrowError(rc);
  }
}

int32 SocketImpl::SendBytes(const void* data, int32 length, int32 flags) {
  if (broken_timeout_) {
    if (send_timeout_.TotalMicroseconds() != 0) {
      if (!Poll(send_timeout_, SELECT_WRITE)) {
        throw TimeoutException();
      }
    }
  }

  int32 rc;
  do {
    if (fd_ == INVALID_SOCKET) {
      throw InvalidSocketException();
    }

    rc = send(fd_, reinterpret_cast<const char*>(data), length, flags);
  } while (blocking_ && rc < 0 && GetLastError() == FUN_EINTR);

  if (rc < 0) {
    ThrowError();
  }

  return rc;
}

int32 SocketImpl::ReceiveBytes(void* buffer, int32 length, int32 flags) {
  if (broken_timeout_) {
    if (recv_timeout_.TotalMicroseconds() != 0) {
      if (!Poll(recv_timeout_, SELECT_READ)) {
        throw TimeoutException();
      }
    }
  }

  int32 rc;
  do {
    if (fd_ == INVALID_SOCKET) {
      throw InvalidSocketException();
    }

    rc = recv(fd_, reinterpret_cast<char*>(buffer), length, flags);
  } while (blocking_ && rc < 0 && GetLastError() == FUN_EINTR);

  if (rc < 0) {
    const int32 error = GetLastError();
    if (error == FUN_EAGAIN && !blocking_) {
      ;
    } else if (error == FUN_EAGAIN || error == FUN_ETIMEDOUT) {
      throw TimeoutException(error);
    } else {
      ThrowError(error);
    }
  }
  return rc;
}

int32 SocketImpl::SendTo(const void* data, int32 length, const InetAddress& address, int32 flags) {
  int32 rc;
  do {
    if (fd_ == INVALID_SOCKET) {
      throw InvalidSocketException();
    }

    sockaddr_in6 sa;
    address.ToNative(sa);

    rc = sendto(fd_, reinterpret_cast<const char*>(data), length, flags, (const sockaddr*)&sa, sizeof(sa));
  } while (blocking_ && rc < 0 && GetLastError() == FUN_EINTR);

  if (rc < 0) {
    ThrowError();
  }

  return rc;
}

int32 SocketImpl::ReceiveFrom(void* buffer, int32 length, InetAddress& address, int32 flags) {
  if (broken_timeout_) {
    if (recv_timeout_.TotalMicroseconds() != 0) {
      if (!Poll(recv_timeout_, SELECT_READ)) {
        throw TimeoutException();
      }
    }
  }

  sockaddr_in6 sa;
  fun_socklen_t sa_len = sizeof(sa);

  int32 rc;
  do {
    if (fd_ == INVALID_SOCKET) {
      throw InvalidSocketException();
    }

    rc = recvfrom(fd_, reinterpret_cast<char*>(buffer), length, flags, (sockaddr*)&sa, &sa_len);
  } while (blocking_ && rc < 0 && GetLastError() == FUN_EINTR);

  if (rc >= 0) {
    address = InetAddress(sa);
  } else {
    const int32 error = GetLastError();
    if (error == FUN_EAGAIN && !blocking_) {
      ;
    } else if (error == FUN_EAGAIN || error == FUN_ETIMEDOUT) {
      throw TimeoutException, error);
    } else {
      ThrowError(error);
    }
  }
  return rc;
}

void SocketImpl::SendUrgent(uint8 data) {
  if (fd_ == INVALID_SOCKET) {
    throw InvalidSocketException();
  }

  const int32 rc = send(fd_, reinterpret_cast<const char*>(&data), sizeof(data), MSG_OOB);
  if (rc < 0) {
    ThrowError();
  }
}

int32 SocketImpl::Available() {
  int32 rc;
  ioctl(FIONREAD, rc);
  return rc;
}

bool SocketImpl::Secure() const {
  return false;
}

bool SocketImpl::Poll(const Timespan& timeout, int32 mode) {
  SOCKET sockfd = fd_;
  if (sockfd == INVALID_SOCKET) {
    throw InvalidSocketException();
  }

#if PLATFORM_HAVE_FD_EPOLL

  int32 epoll_fd = epoll_create(1);
  if (epoll_fd < 0) {
    ThrowError(CStringLiteral("can't create epoll queue"));
  }

  struct epoll_event evin;
  UnsafeMemory::Memset(&evin, 0, sizeof(evin));

  if (mode & SELECT_READ) {
    evin.events |= EPOLLIN;
  }

  if (mode & SELECT_WRITE) {
    evin.events |= EPOLLOUT;
  }

  if (mode & SELECT_ERROR) {
    evin.events |= EPOLLERR;
  }

  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &evin) < 0) {
    ::close(epoll_fd);

    ThrowError(CStringLiteral("can't insert socket to epoll queue"));
  }

  Timespan remaining_time(timeout);
  int32 rc;
  do {
    struct epoll_event evout;
    UnsafeMemory::Memset(&evout, 0, sizeof(evout));

    Timestamp start;
    rc = epoll_wait(epoll_fd, &evout, 1, remaining_time.TotalMilliseconds());
    if (rc < 0 && GetLastError() == FUN_EINTR) {
      Timestamp end;
      Timespan waited = end - start;
      if (waited < remaining_time) {
        remaining_time -= waited;
      } else {
        remaining_time = 0;
      }
    }
  } while (rc < 0 && GetLastError() == FUN_EINTR);

  close(epoll_fd);

  if (rc < 0) {
    ThrowError();
  }

  return rc > 0;

#elif PLATFORM_HAVE_FD_POLL

  pollfd poll_fd;

  UnsafeMemory::Memset(&poll_fd, 0, sizeof(pollfd));
  poll_fd.fd = fd_;

  if (mode & SELECT_READ) {
    poll_fd.events |= POLLIN;
  }

  if (mode & SELECT_WRITE) {
    poll_fd.events |= POLLOUT;
  }

  Timespan remaining_time(timeout);
  int32 rc;
  do {
    Timestamp start;
    rc = poll(&poll_fd, 1, remaining_time.TotalMilliseconds());
    if (rc < 0 && GetLastError() == FUN_EINTR) {
      Timestamp end;
      Timespan waited = end - start;
      if (waited < remaining_time) {
        remaining_time -= waited;
      } else {
        remaining_time = 0;
      }
    }
  } while (rc < 0 && GetLastError() == FUN_EINTR);

  if (rc < 0) {
    ThrowError();
  }

  return rc > 0;

#else

  fd_set read_set;
  fd_set write_set;
  fd_set error_set;
  FD_ZERO(&read_set);
  FD_ZERO(&write_set);
  FD_ZERO(&error_set);

  if (mode & SELECT_READ) {
    FD_SET(sockfd, &read_set);
  }

  if (mode & SELECT_WRITE) {
    FD_SET(sockfd, &write_set);
  }

  if (mode & SELECT_ERROR) {
    FD_SET(sockfd, &error_set);
  }

  Timespan remaining_time(timeout);
  int32 error_code = FUN_ENOERR;
  int32 rc;
  do {
    struct timeval tv;
    tv.tv_sec = (long)remaining_time.TotalSeconds();
    tv.tv_usec = (long)remaining_time.Microseconds();

    DateTime start(DateTime::UtcNow());
    rc = select(int32(sockfd) + 1, &read_set, &write_set, &error_set, &tv);
    if (rc < 0 && (error_code = GetLastError()) == FUN_EINTR) {
      DateTime end(DateTime::UtcNow());
      const Timespan waited = end - start;
      if (waited < remaining_time) {
        remaining_time -= waited;
      } else {
        remaining_time = 0;
      }
    }
  } while (rc < 0 && error_code == FUN_EINTR);

  if (rc < 0) {
    ThrowError(error_code);
  }

  return rc > 0;

#endif // PLATFORM_HAVE_FD_EPOLL
}

void SocketImpl::SetSendBufferSize(int32 size) {
  SetOption(SOL_SOCKET, SO_SNDBUF, size);
}

int32 SocketImpl::GetSendBufferSize() {
  int32 rc;
  GetOption(SOL_SOCKET, SO_SNDBUF, rc);
  return rc;
}

void SocketImpl::SetReceiveBufferSize(int32 size) {
  SetOption(SOL_SOCKET, SO_RCVBUF, size);
}

int32 SocketImpl::GetReceiveBufferSize() {
  int32 rc;
  GetOption(SOL_SOCKET, SO_RCVBUF, rc);
  return rc;
}

void SocketImpl::SetSendTimeout(const Timespan& timeout) {
#if defined(_WIN32) && !defined(FUN_BROKEN_TIMEOUTS)
  int32 value = (int32)timeout.TotalMilliseconds();
  SetOption(SOL_SOCKET, SO_SNDTIMEO, value);
#elif !defined(FUN_BROKEN_TIMEOUTS)
  SetOption(SOL_SOCKET, SO_SNDTIMEO, timeout);
#endif

  if (broken_timeout_) {
    send_timeout_ = timeout;
  }
}

Timespan SocketImpl::GetSendTimeout() {
  Timespan rc;
#if defined(_WIN32) && !defined(FUN_BROKEN_TIMEOUTS)
  int32 value;
  GetOption(SOL_SOCKET, SO_SNDTIMEO, value);
  rc = Timespan::FromMicroseconds(value);
#elif !defined(FUN_BROKEN_TIMEOUTS)
  GetOption(SOL_SOCKET, SO_SNDTIMEO, rc);
#endif

  if (broken_timeout_) {
    rc = send_timeout_;
  }

  return rc;
}

void SocketImpl::SetReceiveTimeout(const Timespan& timeout) {
#ifndef FUN_BROKEN_TIMEOUTS
#if defined(_WIN32)
  int32 value = (int32)timeout.TotalMilliseconds();
  SetOption(SOL_SOCKET, SO_RCVTIMEO, value);
#else
  SetOption(SOL_SOCKET, SO_RCVTIMEO, timeout);
#endif
#endif

  if (broken_timeout_) {
    recv_timeout_ = timeout;
  }
}

Timespan SocketImpl::GetReceiveTimeout() {
  Timespan rc;
#if defined(_WIN32) && !defined(FUN_BROKEN_TIMEOUTS)
  int32 value;
  GetOption(SOL_SOCKET, SO_RCVTIMEO, value);
  rc = Timespan::FromMilliseconds(value);
#elif !defined(FUN_BROKEN_TIMEOUTS)
  GetOption(SOL_SOCKET, SO_RCVTIMEO, rc);
#endif

  if (broken_timeout_) {
    rc = recv_timeout_;
  }

  return rc;
}

InetAddress SocketImpl::GetSocketAddress() {
  if (fd_ == INVALID_SOCKET) {
    throw InvalidSocketException();
  }

  sockaddr_in6 sa;
  fun_socklen_t sa_len = sizeof(sa);

  const int32 rc = getsockname(fd_, (sockaddr*)&sa, &sa_len);
  if (rc == 0) {
    return InetAddress(sa);
  } else {
    ThrowError();
  }

  return InetAddress();
}

InetAddress SocketImpl::GetPeerAddress() {
  if (fd_ == INVALID_SOCKET) {
    throw InvalidSocketException();
  }

  sockaddr_in6 sa;
  fun_socklen_t sa_len = sizeof(sa);

  const int32 rc = getpeername(fd_, (sockaddr*)&sa, &sa_len);
  if (rc == 0) {
    return InetAddress(sa);
  } else {
    ThrowError();
  }

  return InetAddress();
}

void SocketImpl::SetOption(int32 level, int32 option, int32 value) {
  SetRawOption(level, option, &value, sizeof(value));
}

void SocketImpl::SetOption(int32 level, int32 option, uint32 value) {
  SetRawOption(level, option, &value, sizeof(value));
}

void SocketImpl::SetOption(int32 level, int32 option, uint8 value) {
  SetRawOption(level, option, &value, sizeof(value));
}

void SocketImpl::SetOption(int32 level, int32 option, const IpAddress& value) {
  in6_addr sa;
  value.ToNative(sa);

  SetRawOption(level, option, &sa, sizeof(sa));
}

void SocketImpl::SetOption(int32 level, int32 option, const Timespan& value) {
  struct timeval tv;
  tv.tv_sec = (long)value.TotalSeconds();
  tv.tv_usec = (long)value.Microseconds();

  SetRawOption(level, option, &tv, sizeof(tv));
}

void SocketImpl::SetRawOption(int32 level, int32 option, const void* value, fun_socklen_t length) {
  if (fd_ == INVALID_SOCKET) {
    throw InvalidSocketException();
  }

  const int32 rc = setsockopt(fd_, level, option, reinterpret_cast<const char*>(value), length);
  if (rc == -1) {
    ThrowError();
  }
}

void SocketImpl::GetOption(int32 level, int32 option, int32& value) {
  fun_socklen_t length = sizeof(value);
  GetRawOption(level, option, &value, length);
}

void SocketImpl::GetOption(int32 level, int32 option, uint32& value) {
  fun_socklen_t length = sizeof(value);
  GetRawOption(level, option, &value, length);
}

void SocketImpl::GetOption(int32 level, int32 option, uint8& value) {
  fun_socklen_t length = sizeof(value);
  GetRawOption(level, option, &value, length);
}

void SocketImpl::GetOption(int32 level, int32 option, Timespan& value) {
  struct timeval tv;
  fun_socklen_t length = sizeof(tv);
  GetRawOption(level, option, &tv, length);

  const int32 ticks = tv.tv_sec * TICKS_PER_SECOND + tv.tv_usec * 10;
  value = Timespan(ticks);
}

void SocketImpl::GetOption(int32 level, int32 option, IpAddress& value) {
  in6_addr sa;
  fun_socklen_t length = sizeof(sa);

  //TODO scopeid도 정상적으로 처리를 해주어야하는지??

  GetRawOption(level, option, &sa, length);
  value = IpAddress(sa, 0); // scopeid=0
}

void SocketImpl::GetRawOption(int32 level, int32 option, void* value, fun_socklen_t& length) {
  if (fd_ == INVALID_SOCKET) {
    throw InvalidSocketException();
  }

  const int32 rc = getsockopt(fd_, level, option, reinterpret_cast<char*>(value), &length);
  if (rc == -1) {
    ThrowError();
  }
}

void SocketImpl::SetLinger(bool on, int32 seconds) {
  struct linger l;
  l.l_onoff  = on ? 1 : 0;
  l.l_linger = seconds;
  SetRawOption(SOL_SOCKET, SO_LINGER, &l, sizeof(l));
}

void SocketImpl::GetLinger(bool& on, int32& seconds) {
  struct linger l;
  fun_socklen_t length = sizeof(l);
  GetRawOption(SOL_SOCKET, SO_LINGER, &l, length);
  on = l.l_onoff != 0;
  seconds = l.l_linger;
}

void SocketImpl::SetNoDelay(bool flag) {
  int32 value = flag ? 1 : 0;
  SetOption(IPPROTO_TCP, TCP_NODELAY, value);
}

bool SocketImpl::GetNoDelay() {
  int32 value = 0;
  GetOption(IPPROTO_TCP, TCP_NODELAY, value);
  return value != 0;
}

void SocketImpl::SetKeepAlive(bool flag) {
  int32 value = flag ? 1 : 0;
  SetOption(SOL_SOCKET, SO_KEEPALIVE, value);
}

bool SocketImpl::GetKeepAlive() {
  int32 value = 0;
  GetOption(SOL_SOCKET, SO_KEEPALIVE, value);
  return value != 0;
}

void SocketImpl::SetReuseAddress(bool flag) {
  int32 value = flag ? 1 : 0;
  SetOption(SOL_SOCKET, SO_REUSEADDR, value);
}

bool SocketImpl::GetReuseAddress() {
  int32 value = 0;
  GetOption(SOL_SOCKET, SO_REUSEADDR, value);
  return value != 0;
}

void SocketImpl::SetReusePort(bool flag) {
#ifdef SO_REUSEPORT
  try {
    int32 value = flag ? 1 : 0;
    SetOption(SOL_SOCKET, SO_REUSEPORT, value);
  } catch (IoException&) {
    // ignore error, since not all implementations support SO_REUSEPORT, even if the macro is defined.
  }
#endif
}

bool SocketImpl::GetReusePort() {
#ifdef SO_REUSEPORT
  int32 value = 0;
  GetOption(SOL_SOCKET, SO_REUSEPORT, value);
  return value != 0;
#else
  return false;
#endif
}

void SocketImpl::SetOOBInline(bool flag) {
  int32 value = flag ? 1 : 0;
  SetOption(SOL_SOCKET, SO_OOBINLINE, value);
}

bool SocketImpl::GetOOBInline() {
  int32 value = 0;
  GetOption(SOL_SOCKET, SO_OOBINLINE, value);
  return value != 0;
}

void SocketImpl::SetBroadcast(bool flag) {
  int32 value = flag ? 1 : 0;
  SetOption(SOL_SOCKET, SO_BROADCAST, value);
}

bool SocketImpl::GetBroadcast() {
  int32 value = 0;
  GetOption(SOL_SOCKET, SO_BROADCAST, value);
  return value != 0;
}

void SocketImpl::SetBlocking(bool flag) {
#if FUN_PLATFORM_WINDOWS_FAMILY
  int32 arg = flag ? 0 : 1;
  ioctl(FIONBIO, arg);
#else
  int32 arg = fcntl(F_GETFL);
  long flags = arg & ~O_NONBLOCK;
  if (!flag) flags |= O_NONBLOCK;
  (void)fcntl(F_SETFL, flags);
#endif

  blocking_ = flag;
}

int32 SocketImpl::GetSocketError() {
  int32 rc = 0;
  GetOption(SOL_SOCKET, SO_ERROR, rc);
  return rc;
}

void SocketImpl::Init() {
  InitSocket(AF_INET6, SOCK_STREAM); //warning fixed to IPv6
}

void SocketImpl::InitSocket(int32 af, int32 type, int32 proto) {
  fun_check(fd_ == INVALID_SOCKET);

  fd_ = socket(af, type, proto);
  if (fd_ == INVALID_SOCKET) {
    ThrowError();
  }

#if defined(__MACH__) && defined(__APPLE__) || defined(__FreeBSD__)
  // SIGPIPE sends a signal that if unhandled (which is the default)
  // will crash the process. This only happens on UNIX, and not Linux.
  //
  // In order to have FunNet sockets behave the same across platforms, it is
  // best to just ignore SIGPIPE all together.
  SetOption(SOL_SOCKET, SO_NOSIGPIPE, 1);
#endif
}

void SocketImpl::ioctl(fun_ioctl_request_t request, int32& arg) {
#if FUN_PLATFORM_WINDOWS_FAMILY
  const int32 rc = ioctlsocket(fd_, request, reinterpret_cast<u_long*>(&arg));
#else
  const int32 rc = ::ioctl(fd_, request, &arg);
#endif
  if (rc != 0) {
    ThrowError();
  }
}

void SocketImpl::ioctl(fun_ioctl_request_t request, void* arg) {
#if FUN_PLATFORM_WINDOWS_FAMILY
  const int32 rc = ioctlsocket(fd_, request, reinterpret_cast<u_long*>(arg));
#else
  const int32 rc = ::ioctl(fd_, request, arg);
#endif
  if (rc != 0) {
    ThrowError();
  }
}

#if !FUN_PLATFORM_WINDOWS_FAMILY
int32 SocketImpl::fcntl(fun_fcntl_request_t request) {
  const int32 rc = ::fcntl(fd_, request);
  if (rc == -1) {
    ThrowError();
  }

  return rc;
}

int32 SocketImpl::fcntl(fun_fcntl_request_t request, long arg) {
  const int32 rc = ::fcntl(fd_, request, arg);
  if (rc == -1) {
    ThrowError();
  }
  return rc;
}
#endif

void SocketImpl::Reset(SOCKET fd) {
  fd_ = fd;
}

void SocketImpl::ThrowError() {
  const int32 error = GetLastError();
  String empty;
  ThrowError(error, empty);
}

void SocketImpl::ThrowError(const String& arg) {
  ThrowError(GetLastError(), arg);
}

void SocketImpl::ThrowError(int32 code) {
  String arg;
  ThrowError(code, arg);
}

void SocketImpl::ThrowError(int32 code, const String& arg) {
  switch (code) {
    case FUN_ENOERR: fun_check(0); return; //meaningless call
    case FUN_ESYSNOTREADY: throw NetException(CStringLiteral("net subsystem not ready"), code);
    case FUN_ENOTINIT: throw NetException(CStringLiteral("net subsystem not initialized"), code);
    case FUN_EINTR: throw IoException(CStringLiteral("interrupted"), code);
    case FUN_EACCES: throw IoException(CStringLiteral("permission denied"), code);
    case FUN_EFAULT: throw IoException(CStringLiteral("bad address"), code);
    case FUN_EINVAL: throw InvalidArgumentException(CStringLiteral("an invalid argument was supplied."), code);
    case FUN_EMFILE: throw IoException(CStringLiteral("too many open files"), code);
    case FUN_EWOULDBLOCK: throw IoException(CStringLiteral("operation would block"), code);
    case FUN_EINPROGRESS: throw IoException(CStringLiteral("operation now in progress"), code);
    case FUN_EALREADY: throw IoException(CStringLiteral("operation already in progress"), code);
    case FUN_ENOTSOCK: throw IoException(CStringLiteral("socket operation attempted on non-socket"), code);
    case FUN_EDESTADDRREQ: throw NetException(CStringLiteral("destination address required"), code);
    case FUN_EMSGSIZE: throw NetException(CStringLiteral("message too long"), code);
    case FUN_EPROTOTYPE: throw NetException(CStringLiteral("wrong protocol type"), code);
    case FUN_ENOPROTOOPT: throw NetException(CStringLiteral("protocol not available"), code);
    case FUN_EPROTONOSUPPORT: throw NetException(CStringLiteral("protocol not supported"), code);
    case FUN_ESOCKTNOSUPPORT: throw NetException(CStringLiteral("socket type not supported"), code);
    case FUN_ENOTSUP: throw NetException(CStringLiteral("operation not supported"), code);
    case FUN_EPFNOSUPPORT: throw NetException(CStringLiteral("protocol family not supported"), code);
    case FUN_EAFNOSUPPORT: throw NetException(CStringLiteral("address family not supported"), code);
    case FUN_EADDRINUSE: throw NetException(CStringLiteral("address already in use"), arg, code);
    case FUN_EADDRNOTAVAIL: throw NetException(CStringLiteral("cannot assign requested address"), arg, code);
    case FUN_ENETDOWN: throw NetException(CStringLiteral("network is down"), code);
    case FUN_ENETUNREACH: throw NetException(CStringLiteral("network is unreachable"), code);
    case FUN_ENETRESET: throw NetException(CStringLiteral("network dropped connection on reset"), code);
    case FUN_ECONNABORTED: throw ConnectionAbortedException(CStringLiteral("an established connection was aborted by the software in your host machine."), code);
    case FUN_ECONNRESET: throw ConnectionResetException(CStringLiteral("an existing connection was forcibly closed by the remote host."), code);
    case FUN_ENOBUFS: throw IoException(CStringLiteral("no buffer space available"), code);
    case FUN_EISCONN: throw NetException(CStringLiteral("socket is already connected"), code);
    case FUN_ENOTCONN: throw NetException(CStringLiteral("socket is not connected"), code);
    case FUN_ESHUTDOWN: throw NetException(CStringLiteral("cannot send after socket shutdown"), code);
    case FUN_ETIMEDOUT: throw TimeoutException(CStringLiteral("a connection attempt failed because the connected party did not properly respond after a period of time, or established connection failed because connected host has failed to respond."), code);
    case FUN_ECONNREFUSED: throw ConnectionRefusedException, arg, code);
    case FUN_EHOSTDOWN: throw NetException(CStringLiteral("host is down"), arg, code);
    case FUN_EHOSTUNREACH: throw NetException(CStringLiteral("no route to host"), arg, code);
#if FUN_PLATFORM_UNIX_FAMILY
    case EPIPE: throw IoException(CStringLiteral("broken pipe"), code);
    case EBADF: throw IoException(CStringLiteral("bad socket descriptor"), code);
    case ENOENT: throw IoException(CStringLiteral("not found"), arg, code);
#endif

    default:
      throw IoException(String::FromNumber(code), arg, code);
  }
}

} // namespace fun
