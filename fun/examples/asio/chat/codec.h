#pragma once

#include "fun/base/logging.h"
#include "fun/net/buffer.h"
#include "fun/net/endian.h"
#include "fun/net/tcp_connection.h"

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

class LengthHeaderCodec : Noncopyable {
 public:
  typedef Function<void (const fun::net::TcpConnectionPtr&,
                                const String& message,
                                const fun::Timestamp&)> StringMessageCallback;

  explicit LengthHeaderCodec(const StringMessageCallback& cb)
    : message_cb_(cb) {
  }

  void OnMessage(const fun::net::TcpConnectionPtr& conn,
                 fun::net::Buffer* buf,
                 const fun::Timestamp& received_time) {
    while (buf->GetReadableLength() >= kHeaderLen) { // kHeaderLen == 4
      // FIXME: use Buffer::peekInt32()
      const void* data = buf->GetReadablePtr();
      int32_t be32 = *static_cast<const int32_t*>(data); // SIGBUS
      const int32_t len = fun::net::sockets::networkToHost32(be32);
      if (len > 65536 || len < 0) {
        LOG_ERROR << "Invalid length " << len;
        conn->Shutdown();  // FIXME: disable reading
        break;
      }
      else if (buf->GetReadableLength() >= len + kHeaderLen) {
        buf->Drain(kHeaderLen);
        String message(buf->GetReadablePtr(), len);
        message_cb_(conn, message, received_time);
        buf->Drain(len);
      }
      else {
        break;
      }
    }
  }

  // FIXME: TcpConnectionPtr
  void Send(fun::net::TcpConnection* conn, const fun::StringPiece& message) {
    fun::net::Buffer buf;
    buf.append(message.data(), message.size());
    int32_t len = static_cast<int32_t>(message.size());
    int32_t be32 = fun::net::sockets::hostToNetwork32(len);
    // 메시지 앞쪽에 메시지 길이를 넣어줌.
    buf.Prepend(&be32, sizeof be32);
    conn->Send(&buf);
  }

 private:
  StringMessageCallback message_cb_;

  const static size_t kHeaderLen = sizeof(int32_t);
};
