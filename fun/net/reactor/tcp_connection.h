#pragma once

#include "fun/base/shared_ptr.h"
#include "fun/net/buffer.h"
#include "fun/net/net.h"

namespace fun {
namespace net {
namespace reactor {

class Channel;
class EventLoop;
class Socket;

/**
 * TCP connection, for both client and server usage.
 *
 * This is an interface class, so don't expose too much details.
 */
class TcpConnection : Noncopyable, public EnableSharedFromThis<TcpConnection> {
 public:
  /**
  Constructs a TcpConnection with a connected sock_fd

  User should not create this object.
  */
  TcpConnection(EventLoop* loop, const String& name, int sock_fd,
                const InetAddress& local_addr, const InetAddress& peer_addr);
  ~TcpConnection();

  EventLoop* GetLoop() const { return loop_; }

  const String& GetName() const { return name_; }

  const InetAddress& GetLocalAddress() const { return local_addr_; }

  const InetAddress& GetPeerAddress() const { return peer_addr_; }

  bool IsConnected() const { return state_ == kConnected; }

  bool IsDisconnected() const { return state_ == kDisconnected; }

  // return true if success.
  bool GetTcpInfo(struct tcp_info*) const;
  String GetTcpInfoString() const;

  // void Send(String&& message); // C++11
  void Send(const void* message, int len);
  void Send(const StringPiece& message);
  // void Send(Buffer&& message); // C++11
  void Send(Buffer* message);  // this one will swap data
  void Shutdown();             // NOT thread safe, no simultaneous calling
  // void ShutdownAndForceCloseAfter(double seconds); // NOT thread safe, no
  // simultaneous calling
  void ForceClose();
  void ForceCloseWithDelay(double seconds);
  void SetTcpNoDelay(bool on);
  // reading or not
  void StartRead();
  void StopRead();
  bool IsReading() const {
    return reading_;
  };  // NOT thread safe, may race with start/StopReadInLoop

  void SetContext(const boost::any& context) { context_ = context; }

  const boost::any& GetContext() const { return context_; }

  boost::any* MetMutableContext() { return &context_; }

  void SetConnectionCallback(const ConnectionCallback& cb) {
    connection_cb_ = cb;
  }

  void SetMessageCallback(const MessageCallback& cb) { message_cb_ = cb; }

  void SetWriteCompleteCallback(const WriteCompleteCallback& cb) {
    write_complete_cb_ = cb;
  }

  // TODO HighWaterMark는 무억인지??
  void SetHighWaterMarkCallback(const HighWaterMarkCallback& cb,
                                size_t high_water_mark) {
    high_watermark_cb_ = cb;
    high_water_mark_ = high_water_mark;
  }

  /// Advanced interface
  Buffer* GetInputBuffer() { return &input_buffer_; }

  Buffer* GetOutputBuffer() { return &output_buffer_; }

  // Internal use only.
  void SetCloseCallback(const CloseCallback& cb) { close_cb_ = cb; }

  // called when TcpServer accepts a new connection
  void ConnectEstablished();  // should be called only once
  // called when TcpServer has removed me from its map
  void ConnectDestroyed();  // should be called only once

 private:
  enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };
  void HandleRead(const Timestamp& received_time);
  void HandleWrite();
  void HandleClose();
  void HandleError();
  // void SendInLoop(String&& message);
  void SendInLoop(const StringPiece& message);
  void SendInLoop(const void* message, size_t len);
  void ShutdownInLoop();
  // void ShutdownAndForceCloseInLoop(double seconds);
  void ForceCloseInLoop();
  void SetState(StateE s) { state_ = s; }
  const char* StateToString() const;
  void StartReadInLoop();
  void StopReadInLoop();

  EventLoop* loop_;
  const String name_;
  StateE state_;  // FIXME: use atomic variable
  bool reading_;
  // we don't expose those classes to client.
  SharedPtr<Socket> socket_;
  SharedPtr<Channel> channel_;
  const InetAddress local_addr_;
  const InetAddress peer_addr_;
  ConnectionCallback connection_cb_;
  MessageCallback message_cb_;
  WriteCompleteCallback write_complete_cb_;
  HighWaterMarkCallback high_watermark_cb_;
  CloseCallback close_cb_;
  size_t high_water_mark_;
  Buffer input_buffer_;
  Buffer output_buffer_;  // FIXME: use list<Buffer> as output buffer.
  // TODO json으로 처리하는게 좋으려나??
  boost::any context_;
  // FIXME: creationTime_, lastReceiveTime_
  //        bytesReceived_, bytesSent_
};

typedef SharedPtr<TcpConnection> TcpConnectionPtr;

}  // namespace reactor
}  // namespace net
}  // namespace fun
