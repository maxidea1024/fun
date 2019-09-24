#include "fun/net/tcp_connection.h"

namespace fun {
namespace net {

void DefaultConnectionCallback(const TcpConnectionPtr& conn) {
  LOG_TRACE << conn->GetLocalAddress().ToIpPort() << " -> "
            << conn->GetPeerAddress().ToIpPort() << " is "
            << (conn->IsConnected() ? "UP" : "DOWN");
  // do not call conn->ForceClose(), because some users want to register message callback only.
}

void DefaultMessageCallback(const TcpConnectionPtr& conn,
                            Buffer* buf,
                            const Timestamp&) {
  buf->RetrieveAll();
}

TcpConnection::TcpConnection(EventLoop* loop,
                             const String& name,
                             int sock_fd,
                             const InetAddress& local_addr,
                             const InetAddress& peer_addr)
  : loop_(CHECK_NOTNULL(loop)),
    name_(name),
    state_(kConnecting),
    reading_(true),
    socket_(new Socket(sock_fd)),
    channel_(new Channel(loop, sock_fd)),
    localAddr_(local_addr),
    peer_addr_(peer_addr),
    high_water_mark_(64*1024*1024) {
  channel_->SetReadCallback(boost::bind(&TcpConnection::HandleRead, this, _1));
  channel_->SetWriteCallback(boost::bind(&TcpConnection::HandleWrite, this));
  channel_->SetCloseCallback(boost::bind(&TcpConnection::HandleClose, this));
  channel_->SetErrorCallback(boost::bind(&TcpConnection::HandleError, this));

  LOG_DEBUG << "TcpConnection::ctor[" <<  name_ << "] at " << this << " fd=" << sock_fd;

  socket_->SetKeepAlive(true);
}

TcpConnection::~TcpConnection() {
  LOG_DEBUG << "TcpConnection::dtor[" <<  name_ << "] at " << this
            << " fd=" << channel_->fd()
            << " state=" << StateToString();
  fun_check(state_ == kDisconnected);
}

//TODO linux 전용 기능??
bool TcpConnection::GetTcpInfo(struct tcp_info* tcpi) const {
  return socket_->GetTcpInfo(tcpi);
}

String TcpConnection::GetTcpInfoString() const {
  char buf[1024];
  buf[0] = '\0';
  socket_->GetTcpInfoString(buf, sizeof buf);
  return buf;
}

void TcpConnection::Send(const void* data, int len) {
  Send(StringPiece(static_cast<const char*>(data), len));
}

void TcpConnection::Send(const StringPiece& message) {
  if (state_ == kConnected) {
    if (loop_->IsInLoopThread()) {
      SendInLoop(message);
    } else {
      loop_->RunInLoop(
          boost::bind(&TcpConnection::SendInLoop,
                      this, // FIXME
                      message.AsString()));
                    //std::forward<String>(message)));
    }
  }
}

// FIXME efficiency!!!
void TcpConnection::Send(Buffer* buf) {
  if (state_ == kConnected) {
    if (loop_->IsInLoopThread()) {
      SendInLoop(buf->Peek(), buf->GetReadableLength());
      buf->RetrieveAll();
    } else {
      loop_->RunInLoop(
          boost::bind(&TcpConnection::SendInLoop,
                      this, // FIXME
                      buf->RetrieveAllAsString()));
                    //std::forward<String>(message)));
    }
  }
}

void TcpConnection::SendInLoop(const StringPiece& message) {
  SendInLoop(message.data(), message.size());
}

void TcpConnection::SendInLoop(const void* data, size_t len) {
  loop_->AssertInLoopThread();

  ssize_t written_len = 0;
  size_t remaining = len;
  bool fault_encountered = false;
  if (state_ == kDisconnected) {
    LOG_WARN << "disconnected, give up writing";
    return;
  }

  // if no thing in output queue, try writing directly
  if (!channel_->IsWriting() && output_buffer_.GetReadableLength() == 0) {
    written_len = sockets::write(channel_->fd(), data, len);
    if (written_len >= 0) {
      remaining = len - written_len;
      if (remaining == 0 && write_complete_cb_) {
        loop_->QueueInLoop(boost::bind(write_complete_cb_, SharedFromThis()));
      }
    } else { // written_len < 0
      written_len = 0;

      if (errno != EWOULDBLOCK) {
        LOG_SYSERR << "TcpConnection::SendInLoop";
        if (errno == EPIPE || errno == ECONNRESET) { // FIXME: any others?
          fault_encountered = true;
        }
      }
    }
  }

  fun_check(remaining <= len);
  if (!fault_encountered && remaining > 0) {
    size_t old_len = output_buffer_.GetReadableLength();
    if (old_len + remaining >= high_water_mark_
        && old_len < high_water_mark_
        && high_watermark_cb_) {
      loop_->QueueInLoop(boost::bind(high_watermark_cb_, SharedFromThis(), old_len + remaining));
    }

    output_buffer_.Append(static_cast<const char*>(data) + written_len, remaining);

    if (!channel_->IsWriting()) {
      channel_->EnableWriting();
    }
  }
}

void TcpConnection::Shutdown() {
  // FIXME: use compare and swap
  if (state_ == kConnected) {
    SetState(kDisconnecting);

    // FIXME: SharedFromThis()?
    loop_->RunInLoop(boost::bind(&TcpConnection::ShutdownInLoop, this));
  }
}

void TcpConnection::ShutdownInLoop() {
  loop_->AssertInLoopThread();

  if (!channel_->IsWriting()) {
    // we are not writing
    socket_->ShutdownWrite();
  }
}

// void TcpConnection::ShutdownAndForceCloseAfter(double seconds) {
//   // FIXME: use compare and swap
//   if (state_ == kConnected) {
//     SetState(kDisconnecting);
//     loop_->RunInLoop(boost::bind(&TcpConnection::ShutdownAndForceCloseInLoop, this, seconds));
//   }
// }

// void TcpConnection::ShutdownAndForceCloseInLoop(double seconds) {
//   loop_->AssertInLoopThread();
//
//   if (!channel_->IsWriting()) {
//     // we are not writing
//     socket_->ShutdownWrite();
//   }
//   loop_->RunAfter(
//       seconds,
//       MakeWeakCallback(SharedFromThis(),
//                        &TcpConnection::ForceCloseInLoop));
// }

void TcpConnection::ForceClose() {
  // FIXME: use compare and swap
  if (state_ == kConnected || state_ == kDisconnecting) {
    SetState(kDisconnecting);

    loop_->QueueInLoop(boost::bind(&TcpConnection::ForceCloseInLoop, SharedFromThis()));
  }
}

void TcpConnection::ForceCloseWithDelay(double seconds) {
  // FIXME: use compare and swap
  if (state_ == kConnected || state_ == kDisconnecting) {
    SetState(kDisconnecting);

    loop_->RunAfter(
        seconds,
        MakeWeakCallback(SharedFromThis(),
                         &TcpConnection::ForceClose)); // not ForceCloseInLoop to avoid race condition
  }
}

void TcpConnection::ForceCloseInLoop() {
  loop_->AssertInLoopThread();

  if (state_ == kConnected || state_ == kDisconnecting) {
    // as if we received 0 byte in HandleRead();
    HandleClose();
  }
}

const char* TcpConnection::StateToString() const {
  switch (state_) {
    case kDisconnected:
      return "kDisconnected";
    case kConnecting:
      return "kConnecting";
    case kConnected:
      return "kConnected";
    case kDisconnecting:
      return "kDisconnecting";
    default:
      return "unknown state";
  }
}

void TcpConnection::SetTcpNoDelay(bool on) {
  socket_->SetTcpNoDelay(on);
}

void TcpConnection::StartRead() {
  loop_->RunInLoop(boost::bind(&TcpConnection::StartReadInLoop, this));
}

void TcpConnection::StartReadInLoop() {
  loop_->AssertInLoopThread();

  if (!reading_ || !channel_->IsReading()) {
    channel_->EnableReading();
    reading_ = true;
  }
}

void TcpConnection::StopRead() {
  loop_->RunInLoop(boost::bind(&TcpConnection::StopReadInLoop, this));
}

void TcpConnection::StopReadInLoop() {
  loop_->AssertInLoopThread();

  if (reading_ || channel_->IsReading()) {
    channel_->DisableReading();
    reading_ = false;
  }
}

void TcpConnection::ConnectEstablished() {
  loop_->AssertInLoopThread();

  fun_check(state_ == kConnecting);
  SetState(kConnected);

  channel_->Tie(SharedFromThis());
  channel_->EnableReading();

  connection_cb_(SharedFromThis());
}

void TcpConnection::ConnectDestroyed() {
  loop_->AssertInLoopThread();

  if (state_ == kConnected) {
    SetState(kDisconnected);
    channel_->DisableAll();

    connection_cb_(SharedFromThis());
  }

  channel_->Remove();
}

void TcpConnection::HandleRead(const Timestamp& received_time) {
  loop_->AssertInLoopThread();

  int saved_errno = 0;
  ssize_t n = input_buffer_.ReadFd(channel_->fd(), &saved_errno);
  if (n > 0) {
    //warning message_cb_에서는 받은 메시지만큼 제외해주어야함.
    message_cb_(SharedFromThis(), &input_buffer_, received_time);

    //TODO 그냥 여기서 해주어도 좋지 아니한가??
  } else if (n == 0) {
    HandleClose();
  } else {
    errno = saved_errno;
    LOG_SYSERR << "TcpConnection::HandleRead";
    HandleError();
  }
}

void TcpConnection::HandleWrite() {
  loop_->AssertInLoopThread();

  if (channel_->IsWriting()) {
    ssize_t n = sockets::write(channel_->fd(),
                               output_buffer_.GetReadablePtr(),
                               output_buffer_.GetReadableLength());
    if (n > 0) {
      output_buffer_.Drain(n);

      if (output_buffer_.GetReadableLength() == 0) {
        // 더이상 쓸게 없음.

        channel_->DisableWriting();

        if (write_complete_cb_) {
          loop_->QueueInLoop(boost::bind(write_complete_cb_, SharedFromThis()));
        }

        if (state_ == kDisconnecting) {
          ShutdownInLoop();
        }
      }
    } else {
      LOG_SYSERR << "TcpConnection::HandleWrite";
      // if (state_ == kDisconnecting) {
      //   ShutdownInLoop();
      // }
    }
  } else {
    LOG_TRACE << "Connection fd = " << channel_->fd()
              << " is down, no more writing";
  }
}

void TcpConnection::HandleClose() {
  loop_->AssertInLoopThread();

  LOG_TRACE << "fd = " << channel_->fd() << " state = " << StateToString();
  fun_check(state_ == kConnected || state_ == kDisconnecting);
  // we don't close fd, leave it to dtor, so we can find leaks easily.
  SetState(kDisconnected);
  channel_->DisableAll();

  TcpConnectionPtr guard_this(SharedFromThis());
  connection_cb_(guard_this);
  // must be the last line
  close_cb_(guard_this);
}

void TcpConnection::HandleError() {
  int err = sockets::GetSocketError(channel_->fd());

  LOG_ERROR << "TcpConnection::HandleError [" << name_
            << "] - SO_ERROR = " << err << " " << strerror_tl(err);
}

} // namespace net
} // namespace fun
