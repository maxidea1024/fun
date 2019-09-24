#pragma once

#include "LeanType.h"
#include "InternalSocket.h"

#include "Misc/clock.h" //@TEMP

namespace fun {
namespace net {

class InternalSocket;
class CompletionPort;
class CompletionStatus;

/** Completion conext */
class ICompletionContext {
 public:
  virtual ~ICompletionContext() {}

  //virtual LeanType GetLeanType() const { return LeanType::None; }
  virtual LeanType GetLeanType() const = 0;
};


/** I/O completion Types */
enum class CompletionType {
  /** Custom value */
  ReferCustomValue,
  /** Send */
  Send,
  /** Receive */
  Receive,
  /** AcceptEx */
  AcceptEx,
  /** ConnectEx */
  ConnectEx,
  Last,
};


/** IOCP에서 완료된 결과 */
class CompletionStatus {
 public:
  /** 완료된 결과와 관련된 객체. 이 객체 값은 completion_port_.AssociateSocket에서 주어진 object이다. */
  ICompletionContext* completion_context;
  /** 송수신이 완료된 크기 */
  int32 completed_length;
  /** 완료가 실패시 여기 에러 코드가 온다. */
  SocketErrorCode socket_error;
  /** 완료 타입 */
  CompletionType type;
  /** 완료 타입이 custom일때 저장되는 유저 정의 값 */
  UINT_PTR custom_value;
  /** recv, recvfrom에서 수신된 flags값. overlapped I/O에도 업뎃되므로 유지해야 하기에 멤버로 선언했다. */
  DWORD recv_flags;
  /** async UDP recvfrom에 의한 결과에서나 유효하다. */
  InetAddress recvfrom_addr;
  /** 이건 단지 테스트용... */
  InetAddress sendto_addr;
  /** Key */
  ICompletionKey* key;

  CompletionStatus()
    : completion_context(nullptr),
      socket_error(SocketErrorCode::Ok),
      custom_value(0),
      completed_length(0),
      type(CompletionType::Last),
      recv_flags(0),
      recvfrom_addr(InetAddress::None),
      sendto_addr(InetAddress::None), //@note 이건 단지 테스트용..
      key(nullptr) {
  }
};


// TODO 구지 필요할까???
class ICompletionPortCallbacks {
 public:
  virtual ~ICompletionPortCallbacks() {}
  virtual void OnCompletionPortWarning(CompletionPort* port, const char* msg) = 0;
};


/**
 * IOCP wrapper and fake IOCP implementation.
 * 사용법: socket.SetCompletionContext=>completion.AssociateSocket=>socket.Issue* or socket.AcceptEx
 */
class CompletionPort {
 public:
  static const ULONG GQCS_EX_REMOVED_COUNT = 10;

  CompletionPort( ICompletionPortCallbacks* callbacks,
                  int32 max_concurrent_thread_count);
  ~CompletionPort();

  void AssociateSocket(InternalSocket* socket);

  void Post(ICompletionKey* key, UINT_PTR custom_value);

  bool Dequeue(CompletionStatus* out_status, uint32 timeout_msec = INFINITE);

  bool Dequeue( Array<CompletionStatus>& out_statuses,
                int32& dequeue_count,
                uint32 timeout_msec = INFINITE);

 private:
  /** documentation */
  HANDLE iocp_handle_;
  /** documentation */
  ICompletionPortCallbacks* event_;

 private:
  void PostWin32Warning(const char* where);
};

} // namespace net
} // namespace fun
