#pragma once

#include "fun/net/net.h"

namespace fun {

//line 단위로 읽기 처리하는 옵션을 추가하는게 좋을듯함.
//streaming 모드 설정가능 하도록 지원.

class FUN_NETSOCKET_API TcpClient {
 public:
  struct ReadResult {
    bool success;
    Array<char> buffer;
  };

  struct WriteResult {
    bool success;
    int32 size;
  };

  typedef Function<void ()> ConnectedCallback;
  typedef Function<void ()> DisconnectedCallback;
  typedef Function<void (ReadResult&)> AsyncReadCallback;
  typedef Function<void (WriteResult&)> AsyncWriteCallback;

  //요청시점에 내용이 담김.
  //client.async_read({1024, OnNewMessage});
  struct ReadRequest {
    int32 size;
    AsyncReadCallback async_read_cb;
  };

  //요청시점에 내용이 담김.
  //client.async_write({res.buffer,nullptr});
  struct WriteRequest {
    //TODO 차라리 ByteArray 객체를 사용하는게 복사 부담을 줄일 수 있지 않을까??
    Array<char> buffer;
    AsyncWriteCallback async_write_cb;
  };

 public:
  TcpClient();
  TcpClient(const StreamSocket& socket);
  ~TcpClient();

  void ConnectSync(const String& addr, uint32 port);
  void ConnectAsync(const String& addr, uint32 port);

  void Disconnect(bool wait_for_removal = false);

  bool IsConnected() const;
  bool IsConnecting() const;
  bool IsDisconnected() const;

  void OnConnected(SOCKET fd);

  void CallDisconnectedHandler();

  void AsyncRead(const ReadRequest& request);
  void AsyncWrite(const WriteRequest& request);

  //읽기가 가능한 시점에서 호출됨.
  void OnReadAvailable(SOCKET fd);
  //쓰리가 완료된 시점에서 호출됨.
  void OnWriteAvailable(SOCKET fd);

  //읽기완료에 대한 처리
  AsyncReadCallback ProcessRead(ReadResult& result);
  //쓰기완료에 대한 처리
  AsyncWriteCallback ProcessWrite(WriteResult& result);

 public:
  //io_service_
  SharedPtr<SocketIoService> io_service_;

  //socket object
  StreamSocket socket_;
  //Whether connected or not
  ThreadSafeBool is_connected_{false};
  ThreadSafeBool is_connecting_{false};

  //Read requests queue
  Queue<ReadRequest> read_requests_;
  //Write requests queue
  Queue<WriteRequest> write_requests_;

  //Criticalsection for read-requests
  Mutex read_requests_mutex_;
  //Criticalsection for write-requests
  Mutex write_requests_mutex_;

  ConnectedCallback connected_cb_;
  DisconnectedCallback disconnected_cb_;

  void SetConnectedCallback(const ConnectedCallback& cb);
  void SetDisconnectedCallback(const DisconnectedCallback& cb);
};

} // namespace fun
