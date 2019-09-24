#pragma once

#include "fun/net/net.h"
#include "tcp_transport.h"

namespace fun {
namespace net {

class RemoteClient_S;

class ITcpTransportOwner_S {
 public:
  ITcpTransportOwner_S() : last_tcp_send_completion_time_(0.) {}

  virtual ~ITcpTransportOwner_S() {}

  virtual void LockMain_AssertIsLockedByCurrentThread() = 0;
  virtual void LockMain_AssertIsNotLockedByCurrentThread() = 0;

  virtual void EnqueueIssueSendReadyRemotes() = 0;
  virtual double GetAbsoluteTime() = 0;
  virtual bool IsDispose() = 0;
  virtual void IssueDispose(ResultCode result_code, ResultCode detail_code,
                            const ByteArray& comment, const char* where,
                            SocketErrorCode socket_error) = 0;
  virtual void WarnTooShortDisposal(const char* where) = 0;
  virtual bool IsValidEnd() = 0;

  /** 마지막으로 Tcp send Completion을 발생한 시간 */
  double last_tcp_send_completion_time_;
};

/**
 * socket, event 등의 객체가 같은 lifetime을 가지므로 이렇게 뜯어냈다.
 */
class TcpTransport_S : public MessageStream {
 private:
  // sendqueue외의 mainlock로 보호되지 않는 모든 것을 보호하는 criticalsection
  CCriticalSection2 mutex_;
  // sendqueue와 sendissue를 보호하는 criticalsection
  CCriticalSection2 send_queue_mutex_;

  int32 GetBrakedSendAmount(double absolute_time);

 public:
  // mainlock으로 보호 되지 않는 것들.
  FUN_ALIGNED_VOLATILE bool send_issued_;
  FUN_ALIGNED_VOLATILE bool recv_issued_;
  TcpSendQueue send_queue_;
  StreamQueue recv_stream_;

  // 이하는 mainlock로 보호된다.
  ITcpTransportOwner_S* owner_;
  SharedPtr<InternalSocket> socket_;

#if TRACE_ISSUE_DELAY_LOG
  FUN_ALIGNED_VOLATILE double last_recv_issue_warning_time_;
  FUN_ALIGNED_VOLATILE double last_send_issue_warning_time_;
#endif

  // 서버와의 연결이 끊어진 Client socket의 peer name을 얻어봤자 Unassigned이다.
  // 그러므로 서버와의 연결이 성사됐을 때 미리 연결된 주소를 얻어둬야 나중에
  // 연결 종료 이벤트시 제대로 된 연결 정보를 유저에게 건넬 수 있다.
  InetAddress cached_remote_addr_;  // TODO CachedRemoteAddr로 이름을 변경하는게
                                    // 더 의미 있을듯..

  int32 total_tcp_issued_send_bytes_;

  TcpTransport_S(ITcpTransportOwner_S* owner, InternalSocket* socket,
                 const InetAddress& remote_addr);
  ~TcpTransport_S();

  void SetEnableNagleAlgorithm(bool enable);
  void SendWhenReady(const SendFragRefs& data_to_send,
                     const TcpSendOption& send_opt);

  SocketErrorCode ConditionalIssueSend(double absolute_time);

  // issue recv를 건 후 문제가 생기면 객체 파괴 이슈를 건다.
  SocketErrorCode IssueRecvAndCheck();

  // QuickStep, FrequentStep
  // LongStep, OccasionalStep

  void LongTick(double absolute_time);

  CCriticalSection2& GetMutex() { return mutex_; }

  void AssertIsLockedByCurrentThread() {
    GetMutex().AssertIsLockedByCurrentThread();
  }

  void AssertIsNotLockedByCurrentThread() {
    GetMutex().AssertIsNotLockedByCurrentThread();
  }

  bool IsLockedByCurrentThread() {
    return GetMutex().IsLockedByCurrentThread();
  }

  CCriticalSection2& GetSendQueueMutex() { return send_queue_mutex_; }

  void AssertIsSendQueueLockedByCurrentThread() {
    GetSendQueueMutex().AssertIsLockedByCurrentThread();
  }

  void AssertIsSendQueueNotLockedByCurrentThread() {
    GetSendQueueMutex().AssertIsNotLockedByCurrentThread();
  }

  bool IsSendQueueLockedByCurrentThread() {
    return GetSendQueueMutex().IsLockedByCurrentThread();
  }
};

}  // namespace net
}  // namespace fun
