//TODO 어느정도 작업이 진행되면 제거할 코드임!
//TODO 어짜피 제거할 놈이므로, public쪽으로 안빼줌.. 조용히 사라지면됨...
#pragma once

#include "fun/net/net.h"

namespace fun {
namespace net {

class Socket2;

class SocketSelectContext {
 public:
  static SocketSelectContext* New();

  SocketSelectContext() {}
  virtual ~SocketSelectContext() {}

  virtual void AddWriteWaiter(Socket2& socket) = 0;
  virtual void AddExceptionWaiter(Socket2& socket) = 0;
  virtual void Wait(uint32 timeout_msec) = 0;
  virtual bool GetConnectResult(Socket2& socket, SocketErrorCode& out_code) = 0;
};

/**
 * 소켓에서 발생항 에러를 캐치하기 위함인데, 구지 이게 필요할까 싶은데...??
 */
class ISocketDelegate {
 public:
  virtual ~ISocketDelegate() {}

  virtual void OnSocketWarning(Socket2* soket, const string& text) = 0;
};

/**
 * 범용 소켓.
 * 조금 보완이 필요해 보임..
 * IPv6도 지원해야하고....
 */
class Socket2 {
 public:
  static Socket2* New(SOCKET exisiting_socket, ISocketDelegate* delegate);
  static Socket2* New(SocketType socket_type, ISocketDelegate* delegate);

  virtual ~Socket2() {}

  virtual bool Bind() = 0;
  virtual bool Bind(int32 Port) = 0;
  virtual bool Bind(const char* ip, int32 port) = 0;

  virtual SocketErrorCode Connect(const string& host, int32 port) = 0;

  virtual SocketErrorCode IssueRecvFrom(int32 len) = 0;
  virtual SocketErrorCode IssueSendTo(const uint8* data, int32 len, const InetAddress& sendto) = 0;

  virtual SocketErrorCode IssueRecv(int32 len) = 0;
  virtual SocketErrorCode IssueSend(const uint8* data, int32 len) = 0;

  //@todo 타임아웃은 지정할 수 없는걸까??
  virtual bool GetRecvOverlappedResult(bool wait_until_complete, OverlappedResult& out_overlapped_result) = 0;
  virtual bool GetSendOverlappedResult(bool wait_until_complete, OverlappedResult& out_overlapped_result) = 0;
  virtual bool GetAcceptExOverlappedResult(bool wait_until_complete, OverlappedResult& out_overlapped_result) = 0;

  virtual InetAddress GetSockName() = 0;
  virtual InetAddress GetPeerName() = 0;

  virtual void SetBlockingMode(bool blocking) = 0;

  virtual uint8* GetRecvBufferPtr() = 0;
};

class SocketSelectContextImpl : public SocketSelectContext {
 public:
  SocketSelectContextImpl() {}

  void AddWriteWaiter(Socket2& socket);
  void AddExceptionWaiter(Socket2& socket);
  void Wait(uint32 MSec);
  bool GetConnectResult(Socket2& socket, SocketErrorCode& out_code);

 private:
  InternalSocketSelectContext selection_context_;
};

class Socket2Impl
  : public Socket2
  , public IInternalSocketDelegate {
 public:
  Socket2Impl(SOCKET exisiting_socket, ISocketDelegate* delegate);
  Socket2Impl(SocketType socket_type, ISocketDelegate* delegate);

  bool Bind();
  bool Bind(int32 port);
  bool Bind(const char* ip, int32 port);

  SocketErrorCode Connect(const String& host, int32 port);

  SocketErrorCode IssueRecvFrom(int32 len);
  SocketErrorCode IssueSendTo(const uint8* data, int32 len, const InetAddress& sendto);
  SocketErrorCode IssueRecv(int32 len);
  SocketErrorCode IssueSend(const uint8* data, int32 len);

  bool GetRecvOverlappedResult(bool wait_until_complete, OverlappedResult& overlapped_result);
  bool GetSendOverlappedResult(bool wait_until_complete, OverlappedResult& overlapped_result);
  bool GetAcceptExOverlappedResult(bool wait_until_complete, OverlappedResult& overlapped_result);

  InetAddress GetSockName();
  InetAddress GetPeerName();

  void SetBlockingMode(bool blocking);

  uint8* GetRecvBufferPtr();

 private:
  friend class SocketSelectContextImpl;

  // FunNet InternalSocket Instance
  InternalSocketPtr internal_socket_;
  // socket Delegate
  ISocketDelegate* internal_delegate_;

  void OnSocketWarning(InternalSocket* socket, const String& msg) override {
    internal_delegate_->OnSocketWarning(this, msg);
  }
};

} // namespace net
} // namespace fun
