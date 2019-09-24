#pragma once

namespace fun {
namespace net {

//TODO struct로 묶어주는게 좋을듯 싶은데...
extern AtomicCounter g_msg_size_error_count;
extern AtomicCounter g_net_reset_error_count;
extern AtomicCounter g_conn_reset_error_count;

/** Overlapped result structure */
class OverlappedResult {
 public:
  /** Default constructor */
  OverlappedResult()
    : completed_length(0),
      received_flags(0),
      socket_error(SocketErrorCode::Ok),
      received_from(InetAddress::None) {
  }

  /** Retrieve flags */
  uint32 received_flags;
  /** socket error code */
  SocketErrorCode socket_error;
  /** Address of received */
  InetAddress received_from;
  /** Completed data length */
  int32 completed_length;
};


class InternalSocket;
class CompletionPort;
class ICompletionContext;
class CompletionStatus;
//class ReceivedMessageList;
class IHostObject;
class FinalUserWorkItem;

//TODO 구지 필요할까???
class IInternalSocketDelegate {
 public:
  virtual ~IInternalSocketDelegate() {}

  virtual void OnSocketWarning(InternalSocket* socket, const String& text) = 0;
};


/**
 * IOCP에서 인자 Key는 이 객체 타입으로 캐스팅됨
 * 이 객체를 구현하는 것으로 넷서버,랜서버,랜클라 등 메인 모듈에 한함.
 */
class ICompletionKey {
 public:
  //가상 소멸자로 선언할 경우 vtbl이 추가 되므로, 문제가 발생함.
  //virtual ~ICompletionKey() {}

  /** 스레드풀 객체 내에서의 i/o completion이 발생하면 이 루틴이 실행됨. */
  virtual void OnIoCompletion(Array<IHostObject*>& send_issued_pool,
                              ReceivedMessageList& msg_list,
                              CompletionStatus& completion) = 0;

  /**
   * 메인 모듈까지는 찾아냈지만 어떤 소켓이나
   * remote 객체에 대한 수행인지 얻으려면 이것을 사용.
   */
  virtual ICompletionContext* GetCompletionContext() const { return nullptr; }
};

class ISocketIoCompletionDelegate {
 public:
  virtual ~ISocketIoCompletionDelegate() {}

  virtual void OnSocketIoCompletion(Array<IHostObject*>& send_issued_pool,
                                    ReceivedMessageList& msg_list,
                                    CompletionStatus& completion) = 0;
};

class OverlappedEx : public OVERLAPPED {
 public:
  /** true if issue function is already called. */
  FUN_ALIGNED_VOLATILE bool issued;

  OverlappedEx() {
    UnsafeMemory::Memset((OVERLAPPED*)this, 0x00, sizeof(OVERLAPPED));
    issued = false;
  }
};

/**
 * WSABUF 모음을 이용해서, 소켓에 바로 전송하는 데이터 타입임. windows 전용임.
 * WSABUF에는 보낼 데이터의 참조만 기록되는데, 실제로 보내기가 완료되거나 중지될때까지
 * 참조를 유지해주어야한다.
 */
class FragmentedBuffer : public Noncopyable {
 public:
  Array<WSABUF, InlineAllocator<32>> buffer_;

  FragmentedBuffer() : cached_length_(0) {}

  int32 Length() const { return cached_length_; }

  void Clear() {
    buffer_.Reset(); // don't change capacity.
    cached_length_ = 0;
  }

  void Add(const uint8* data, int32 length) {
    WSABUF wsabuf;
    wsabuf.buf = (char*)data;
    wsabuf.len = (ULONG)length;
    buffer_.Add(wsabuf);

    cached_length_ += length;
  }

  void Add(const ByteArray& data) {
    Add((const uint8*)data.ConstData(), data.Len());
  }

  template <typename Allocator>
  void Add(const Array<uint8,Allocator>& data) {
    Add(data.ConstData(), data.Count());
  }

 private:
  int32 cached_length_;
};


/**
 * overlapped I/O를 할 경우 validation 값을 여기서 가지므로 소멸 자체도 worker thread에서 하는 것이 필요하다.
 *
 * IOCP에 포인터 값으로 직접 넘어가므로, 가상소멸자가 있으면 안됨.
 * 더불어, ListNode<>, ICompletionKey에도 가상소멸자가 있으면 안됨.
 *
 * 넘기기는 InternalSocket*을 어드레스 값(uintptr_t)으로 넘기는데, IOCP에서 넘겨받은 어드레스 값(uintptr_t)을 ICompletionKey*로
 * 캐스팅 후 액세스하는 부분이 있는데, 여기서 어드레스 왜곡이 생김.
 *
 * 이부분에 대해서는 좀더 면밀히 살펴봐야할것으로 보인다.
 */
class InternalSocket
  : public ListNode<InternalSocket> // Faked IOCP 구현을 위해서만 필요함. (차후에 reactor구현시 필요할수도..)
  , public ICompletionKey {
  friend class CompletionPort;

 private:
  IInternalSocketDelegate* delegate_;
  bool broadcast_option_enabled_;
  bool verbose_; // true이면 경고 이벤트 핸들러가 호출됨

  // 여러 스레드에서 접근하므로 보호되어야 한다.
  CCriticalSection2 socket_closed_mutex_;
  FUN_ALIGNED_VOLATILE bool socket_closed_or_closing_mutex_protected_; // FUN_ALIGNED_VOLATILE: IsClosedOrClosing 때문

  void InitOthers();

  OverlappedEx accept_ex_ovl_;
  Array<uint8> accept_ex_buffer_;
  DWORD accept_ex_length_; //@todo AcceptEx함수의 인자로 사용되는데, DWORD* 타입을 요구함..

  OverlappedEx recv_ovl_;
  Array<uint8> recv_buffer_;

  OverlappedEx send_ovl_;
  Array<uint8> send_buffer_;

  ICompletionContext* completion_context_;

  // 이 값은 overlapped recvfrom이 완료될 때 채워지므로 유효하게 유지해야 한다.
  //char recvfrom_addr_[128]; //temp 차후에 코드 정리를 좀 하도록!!!
  int32 recvfrom_addr_len_;
#if FUN_DISABLE_IPV6
  sockaddr_in recvfrom_addr_;
#else
  sockaddr_in6 recvfrom_addr_;
#endif

  //@note 꼭 필요한 값은 아니지만, 디버깅을 위해서 변수 형태로 가지고 있게함.  차후에 제거해도 될듯..
  SOCKADDR_IN sendto_addr_;
  int32 sendto_addr_len_;

  DWORD recv_flags_; // recv, recvfrom에서 수신된 flags값. overlapped I/O에도 업뎃되므로 유지해야 하기에 멤버로 선언했다.

  // 다소 거시기한 디자인의 멤버
  int32 ttl_to_restore_on_send_completion_;
  bool ttl_should_restore_on_send_completion_;

 public:
  // IO PENDING이 일어난 횟수
  FUN_ALIGNED_VOLATILE int32 io_pending_count_;

 public:
  //int32 RestoredCount;

  //AcceptEx 함수 호출시에만, 10038 (Not socket) 오류가 날경우
  //오류를 무시할지 여부를 설정함.
  bool ignore_not_socket_error_;

  ISocketIoCompletionDelegate* io_completion_delegate_;

  //TODO 실질적으로 CompletionEvent, bIssued는 의미가 없음. 사용도 되지 않고, 이 함수는 차라리 없애는것도 좋을듯...
  //static void ResetOverlappedStruct(OverlappedEx& obj, Event* completion_event = nullptr, bool issued = false);

private:
  FUN_ALIGNED_VOLATILE static LPFN_ACCEPTEX lpfnAcceptEx;
  //static LPFN_GETACCEPTEXSOCKADDRS lpfnGetAcceptExSockAddrs;
  void DirectBindAcceptEx();

  //TODO 제한적으로만 사용되므로, 꼭 필요치 않다면 제거하는게 좋을듯...
  bool EnsureUnicastEndpoint(const InetAddress& sendto);

#if _WIN32_WINNT >= 0x0501 //connectex 0x0501이상에서만 사용가능.
  OverlappedEx connect_ex_ovl_;
  FUN_ALIGNED_VOLATILE static LPFN_CONNECTEX lpfnConnectEx;
  void DirectBindConnectEx();
  // connectex의 Completion이 완료되었음을 확인하는 변수
  FUN_ALIGNED_VOLATILE bool connect_ex_complete_;

public:
  SocketErrorCode ConnectEx(const InetAddress& addr);
  SocketErrorCode ConnectEx(const String& host, int32 port);

  inline bool IsConnectExComplete() { return connect_ex_complete_; }
  inline bool ConnectExIssued() { return connect_ex_ovl_.issued; }

  // ConnectEx완료 처리를 한다.
  SocketErrorCode ConnectExComplete();
  bool GetConnectExOverlappedResult(bool wait_until_complete, OverlappedResult& out_result);
#endif

public:
  //@deprecated 제거하도록 하자.
  // 테스트 용도이다. overlapped io 미완료로 인한 메모리 긁기가 발생하는지 체크를 위함
  //bool bLeakPinnedInternal_TEST;

  //@deprecated 검증 용도. 제거하도록 하자.
  //bool bLastIssueSendCompletedBeforeReturn;
  CompletionPort* associated_iocp_;

  //static const int MaxBufferLength=1024*10;
  SOCKET socket_;

  InternalSocket(SOCKET exisiting_socket, IInternalSocketDelegate* delegate);
  InternalSocket(SocketType SocketType, IInternalSocketDelegate* delegate);
  InternalSocket(SOCKET exisiting_socket, IInternalSocketDelegate* delegate, ISocketIoCompletionDelegate* io_completion_delegate);
  InternalSocket(SocketType SocketType, IInternalSocketDelegate* delegate, ISocketIoCompletionDelegate* io_completion_delegate);
  //virtual ~InternalSocket();
  ~InternalSocket();

  /**
   * 소켓 내부의 버퍼 또는 변수들은 그대로 유지한채, 소켓만 재생성할 경우에 사용됨.
   * 이렇게 내부 변수들은 그대로 둔채, 소켓 객체만 다시 생성하는 동작을 Restore라 칭함.
   */
  void Restore(bool bIsTcp);

  void ForceSetCompletionContext(ICompletionContext* context) {
    completion_context_ = context;
  }

  void SetCompletionContext(ICompletionContext* context) {
    if (completion_context_) {
      throw Exception("the completion context is already set.");
    }

    completion_context_ = context;
  }

  // ICompletionKey interface
  ICompletionContext* GetCompletionContext() const override {
    return completion_context_;
  }

  int32 SetSendBufferSize(int32 size);
  int32 SetRecvBufferSize(int32 size);

  int32 GetSendBufferSize(int32& out_size);
  int32 GetRecvBufferSize(int32& out_size);

  SocketErrorCode EnableBroadcastOption(bool enable);
  SocketErrorCode AllowPacketFragmentation(bool allow);

  bool Bind();
  bool Bind(int32 port);
  bool Bind(const char* host, int32 port);
  bool Bind(const InetAddress& local_addr);

  SocketErrorCode SetTTL(int32 ttl);
  SocketErrorCode GetTtl(int32& out_ttl);
  void RestoreTtlOnCompletion();

  static void SetIPv6Only(SOCKET socket, bool flag);
  void SetIPv6Only(bool flag);

  void Listen();

  void PostSocketWarning(DWORD error_code, const char* where);

  InternalSocket* Accept(int32& error_code);
  SocketErrorCode AcceptEx(InternalSocket* exisiting_socket);
  void FinalizeAcceptEx(InternalSocket* tcp_listening_socket, InetAddress& local_addr, InetAddress& remote_addr);

  SocketErrorCode Connect(const String& host, int32 port);

  bool IsBoundSocket();

  SocketErrorCode IssueRecvFrom(int32 length);
  SocketErrorCode IssueSendTo(const uint8* data, int32 length, const InetAddress& sendto);
  inline SocketErrorCode IssueSendTo_NoCopy(FragmentedBuffer& send_buffer, const InetAddress& sendto) { return IssueSendTo_NoCopy_TempTtl(send_buffer, sendto, -1); }
  SocketErrorCode IssueSendTo_NoCopy_TempTtl(FragmentedBuffer& send_buffer, const InetAddress& sendto, int32 ttl);
  SocketErrorCode IssueRecv(int32 length);
  SocketErrorCode IssueSend(const uint8* data, int32 length);
  SocketErrorCode IssueSend_NoCopy(FragmentedBuffer& send_buffer);

  // 테스트에서 씀
  SocketErrorCode BlockedSend(const uint8* data, int32 length);
  SocketErrorCode BlockedSendTo(const uint8* data, int32 length, const InetAddress& sendto);
  SocketErrorCode BlockedRecvFrom(uint8* buff, int32 buff_length, InetAddress& out_recvfrom);

  inline bool RecvIssued() { return recv_ovl_.issued; }
  inline bool SendIssued() { return send_ovl_.issued; }
  inline bool AcceptExIssued() { return accept_ex_ovl_.issued; }

  bool GetRecvOverlappedResult(bool wait_until_complete, OverlappedResult& out_result);
  bool GetSendOverlappedResult(bool wait_until_complete, OverlappedResult& out_result);
  bool GetAcceptExOverlappedResult(bool wait_until_complete, OverlappedResult& out_result);

  InetAddress GetSockName();
  InetAddress GetPeerName();

  // 에러를 유발시켜서, 자연스럽게 종료시킬 수 있도록 소켓 핸들만 닫는다.
  // 소켓 핸들 값은 유지시킨다.  그래야만 해당 소켓 함수들이 사용하던 핸들로
  // 접근시 에러가 발생할테니까..
  void CloseSocketHandleOnly();

  bool GetVerboseFlag();
  void SetVerboseFlag(bool verbose);

  void SetBlockingMode(bool blocking);

  void EnableNagleAlgorithm(bool enable);

  uint8* GetRecvBufferPtr();
  //uint8* GetSendBufferPtr();

  bool IsClosed();

  // i/o completion 에서 자주 호출하므로 무거운 CS lock을 피하기 위함.
  bool IsClosedOrClosing() const {
    return socket_closed_or_closing_mutex_protected_; // NO CS unlock!
  }

  SocketErrorCode Shutdown(ShutdownFlag how);

  virtual void OnIoCompletion(Array<IHostObject*>& send_issued_pool,
                              ReceivedMessageList& msg_list,
                              CompletionStatus& completion) override;

#ifdef USE_DisableUdpConnResetReport
  void DisableUdpConnResetReport();
#endif

  CompletionPort* GetAssociatedCompletionPort() { return associated_iocp_; }

  //TODO CSocketUtil 쪽으로 몰아주는게 좋을듯...
  static bool IsUdpStopErrorCode(SocketErrorCode socket_error); //TODO 쓰이지 않고 있음.
  static bool IsTcpStopErrorCode(SocketErrorCode socket_error); //TODO 쓰이지 않고 있음.
  static bool IsPendingErrorCode(SocketErrorCode socket_error);
};

// SSDP socket에서도 참조하며 Socket같은건 자주 만드는게 아니니까 이렇게 해도 OK.
// 자주 접근하므로 thread safe로 만들면 곤란.
// 웬만하면 쓰지 말자. RefCount이 assign op는 thread unsafe하니까.
typedef SharedPtr<class InternalSocket> InternalSocketPtr;

class IHasOverlappedIo {
 public:
  FUN_ALIGNED_VOLATILE bool send_issued_;
  FUN_ALIGNED_VOLATILE bool recv_issued_;

  IHasOverlappedIo();
  virtual ~IHasOverlappedIo() {}

  virtual void OnCloseSocketAndMakeOrphant() = 0;
  virtual bool IsSocketClosed() = 0;
};

typedef SharedPtr<IHasOverlappedIo> IHasOverlappedIoPtr;

//TEMP 임시용임. 어느정도 쓰이다가 폐기할것임.
/**
 * socket의 select() non-block model을 위한 용도
 *
 * 주의: Wait 호출 후에는 FD_SET의 내용이 바뀐다.  따라서 이 객체는 1회성으로 쓰여야 한다.
 */
class InternalSocketSelectContext {
 public:
  InternalSocketSelectContext();

  void AddWriteWaiter(InternalSocket& socket);
  void AddExceptionWaiter(InternalSocket& socket);
  void Wait(uint32 msec);
  bool GetConnectResult(InternalSocket& socket, SocketErrorCode& out_code);

 private:
  FD_SET read_socket_list_;
  FD_SET write_socket_list_;
  FD_SET error_socket_list_;
};

} // namespace net
} // namespace fun
