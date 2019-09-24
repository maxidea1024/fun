#pragma once

#include "fun/net/net.h"
#include "tcp_transport.h"

namespace fun {
namespace net {

class NetClientWorker;
class InternalSocket;
class ISocketIoCompletionDelegate;

class ITcpTransportOwner_C : public IInternalSocketDelegate {
 public:
  ITcpTransportOwner_C() : last_tcp_send_completion_time_(0.0) {}
  virtual ~ITcpTransportOwner_C() {}

 public:
  virtual double GetAbsoluteTime() = 0;
  virtual NetClientWorker* GetWorker() { return nullptr; }
  virtual void AssociateSocket(InternalSocket* socket) = 0;
  virtual ISocketIoCompletionDelegate* GetIoCompletionDelegate() {
    return nullptr;
  }
  virtual void LockMain_AssertIsLockedByCurrentThread() = 0;
  virtual void LockMain_AssertIsNotLockedByCurrentThread() = 0;

 public:
  /** 마지막으로 tcp send completion을 발생한 시간 */
  double last_tcp_send_completion_time_;
};

/**
 * socket, event 등의 객체가 같은 lifetime을 가지므로 이렇게 뜯어냈다.
 */
class TcpTransport_C : public MessageStream,
                       public ICompletionContext,
                       public IHasOverlappedIo {
 public:
  TcpTransport_C(ITcpTransportOwner_C* owner);
  virtual ~TcpTransport_C();

  void SetEnableNagleAlgorithm(bool enable);
  void SendWhenReady(const SendFragRefs& data, const TcpSendOption& option);
  void ConditionalIssueSend();
  SocketErrorCode IssueRecvAndCheck();
  void LongTick(double absolute_time);
  int32 GetBrakedSendAmount(double absolute_time);
  void RefreshLocalAddress();
  LeanType GetLeanType() const override { return LeanType::TcpTransport_C; }

  // IHasOverlappedIo interface
  void OnCloseSocketAndMakeOrphant() override;
  bool IsSocketClosed() override;

  /**
  서버에서 인식한 클라 호스트의 주소.
  클라가 공유기 뒤에 있으면 이 주소는 외부 주소임.

  TODO 이름을 바꾸어야할듯. 변수명만으로는 의미전달이 잘안된다.
  */
  InetAddress local_addr_at_server_;

  // 클라 내부에서 인식된 호스트의 주소.
  // 클라가 공유기 뒤에 있으면 이 주소는 내부 주소이다.
  InetAddress local_addr_;

  ITcpTransportOwner_C* owner_;
  InternalSocketPtr socket_;
  StreamQueue recv_stream_;
  TcpSendQueue send_queue_;

  double last_recv_invoke_warning_time_;
  double last_send_invoked_warning_time_;

  int32 total_tcp_issued_send_bytes_;
};

typedef SharedPtr<TcpTransport_C> TcpTransportPtr_C;

}  // namespace net
}  // namespace fun
