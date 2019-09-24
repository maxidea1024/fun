#include "fun/net/connector.h"

namespace fun {
namespace net {

const int Connector::kMaxRetryDelayMs;

Connector::Connector(EventLoop* loop, const InetAddress& server_addr)
    : loop_(loop),
      server_addr_(server_addr),
      connect_(false),
      state_(kDisconnected),
      retry_delay_msecs_(kInitRetryDelayMs) {
  LOG_DEBUG << "ctor[" << this << "]";
}

Connector::~Connector() {
  LOG_DEBUG << "dtor[" << this << "]";
  fun_check(!channel_);
}

void Connector::Start() {
  connect_ = true;
  loop_->RunInLoop(
      boost::bind(&Connector::StartInLoop, this));  // FIXME: unsafe
}

void Connector::StartInLoop() {
  loop_->AssertInLoopThread();
  fun_check(state_ == kDisconnected);
  if (connect_) {
    Connect();
  } else {
    LOG_DEBUG << "do not Connect";
  }
}

void Connector::Stop() {
  connect_ = false;
  loop_->QueueInLoop(
      boost::bind(&Connector::StopInLoop, this));  // FIXME: unsafe
  // FIXME: cancel timer
}

void Connector::StopInLoop() {
  loop_->AssertInLoopThread();
  if (state_ == kConnecting) {
    SetState(kDisconnected);
    int sock_fd = RemoveAndResetChannel();
    Retry(sock_fd);
  }
}

void Connector::Connect() {
  int sock_fd = sockets::CreateNonblockingOrDie(server_addr_.family());
  int ret = sockets::Connect(sock_fd, server_addr_.GetSockAddr());
  int saved_errno = (ret == 0) ? 0 : errno;
  switch (saved_errno) {
    case 0:
    case EINPROGRESS:
    case EINTR:
    case EISCONN:
      Connecting(sock_fd);
      break;

    case EAGAIN:
    case EADDRINUSE:
    case EADDRNOTAVAIL:
    case ECONNREFUSED:
    case ENETUNREACH:
      Retry(sock_fd);
      break;

    case EACCES:
    case EPERM:
    case EAFNOSUPPORT:
    case EALREADY:
    case EBADF:
    case EFAULT:
    case ENOTSOCK:
      LOG_SYSERR << "Connect error in Connector::StartInLoop " << saved_errno;
      sockets::close(sock_fd);
      break;

    default:
      LOG_SYSERR << "Unexpected error in Connector::StartInLoop "
                 << saved_errno;
      sockets::close(sock_fd);
      // connectErrorCallback_();
      break;
  }
}

void Connector::Restart() {
  loop_->AssertInLoopThread();
  SetState(kDisconnected);
  retry_delay_msecs_ = kInitRetryDelayMs;
  connect_ = true;
  StartInLoop();
}

void Connector::Connecting(int sock_fd) {
  SetState(kConnecting);

  fun_check(channel_ == nullptr);
  channel_.Reset(new Channel(loop_, sock_fd));
  channel_->SetWriteCallback(
      boost::bind(&Connector::HandleWrite, this));  // FIXME: unsafe
  channel_->SetErrorCallback(
      boost::bind(&Connector::HandleError, this));  // FIXME: unsafe

  // channel_->Tie(SharedFromThis()); is not working,
  // as channel_ is not managed by shared_ptr
  channel_->EnableWriting();
}

int Connector::RemoveAndResetChannel() {
  channel_->DisableAll();
  channel_->Remove();
  int sock_fd = channel_->fd();
  // Can't Reset channel_ here, because we are inside Channel::handleEvent
  loop_->QueueInLoop(
      boost::bind(&Connector::ResetChannel, this));  // FIXME: unsafe
  return sock_fd;
}

void Connector::ResetChannel() { channel_.Reset(); }

void Connector::HandleWrite() {
  LOG_TRACE << "Connector::HandleWrite " << state_;

  if (state_ == kConnecting) {
    int sock_fd = RemoveAndResetChannel();
    int err = sockets::GetSocketError(sock_fd);
    if (err) {
      LOG_WARN << "Connector::HandleWrite - SO_ERROR = " << err << " "
               << strerror_tl(err);

      Retry(sock_fd);
    } else if (sockets::IsSelfConnect(sock_fd)) {
      LOG_WARN << "Connector::HandleWrite - Self Connect";

      Retry(sock_fd);
    } else {
      SetState(kConnected);

      if (connect_) {
        new_connection_cb_(sock_fd);
      } else {
        sockets::Close(sock_fd);
      }
    }
  } else {
    // what happened?
    fun_check(state_ == kDisconnected);
  }
}

void Connector::HandleError() {
  LOG_ERROR << "Connector::HandleError state=" << state_;
  if (state_ == kConnecting) {
    int sock_fd = RemoveAndResetChannel();
    int err = sockets::GetSocketError(sock_fd);
    LOG_TRACE << "SO_ERROR = " << err << " " << strerror_tl(err);
    Retry(sock_fd);
  }
}

void Connector::Retry(int sock_fd) {
  sockets::Close(sock_fd);

  SetState(kDisconnected);

  // Reconnect...
  if (connect_) {
    LOG_INFO << "Connector::Retry - Retry connecting to "
             << server_addr_.ToIpPort() << " in " << retry_delay_msecs_
             << " milliseconds. ";

    loop_->ScheduleAfter(
        retry_delay_msecs_ / 1000.0,
        boost::bind(&Connector::StartInLoop, SharedFromThis()));
    retry_delay_msecs_ =
        MathBase::Min(retry_delay_msecs_ * 2, kMaxRetryDelayMs);
  } else {
    LOG_DEBUG << "do not connect";
  }
}

}  // namespace net
}  // namespace fun
