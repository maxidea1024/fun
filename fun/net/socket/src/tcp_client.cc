#include "fun/net/tcp_client.h"

namespace fun {

TcpClient::TcpClient()
  : io_service_(SocketIoService::GetDefaultIoService()),
    connected_cb_(nullptr),
    disconnected_cb_(nullptr) {}

TcpClient::TcpClient(const StreamSocket& socket)
  : io_service_(SocketIoService::GetDefaultIoService()),
    connected_cb_(nullptr),
    disconnected_cb_(nullptr),
    socket_(socket) {}

TcpClient::~TcpClient() {
  Disconnect(true);
}

void TcpClient::SetConnectedCallback(const ConnectedCallback& cb) {
  connected_cb_ = cb;
}

void TcpClient::SetDisconnectedCallback(const DisconnectedCallback& cb) {
  disconnected_cb_ = cb;
}

bool TcpClient::IsConnected() const {
  return is_connected_;
}

bool TcpClient::IsConnecting() const {
  return is_connecting_;
}

bool TcpClient::IsDisconnected() const {
  return !(is_connected_ || is_connecting_);
}

void TcpClient::ConnectSync(const String& addr, uint32 port) {
  if (IsConnected() || IsConnecting()) {
    //TODO throw "already connected"
  }

  try {
    InetAddress ep = InetAddress(addr, port);
    socket_.Connect(ep);

    //TODO connection handler 추가
    //io_service_->Associate(socket_, ConnectionHandler, nullptr, nullptr);
    io_service_->Associate(socket_, nullptr, nullptr);
  } catch (Exception& e) {
    // 예외 발생시, 소켓을 닫아줍니다. 재접속 시도시 Socket::Connect()에서 소켓이 닫힌 경우 다시 생성하므로 문제 될것은 없습니다.
    socket_.Close();

    e.Rethrow();
  }

  // 연결되었음.
  is_connected_ = true;
  is_connecting_ = false;
}

void TcpClient::ConnectAsync(const String& addr, uint32 port) {
  if (IsConnected() || IsConnecting()) {
    //TODO throw "already connected"
  }

  try {
    InetAddress ep = InetAddress(addr, port);
    socket_.ConnectNB(ep);

    //TODO
    
    //TODO connection handler 추가
    //io_service_->Associate(socket_, ConnectionHandler, nullptr, nullptr);
    io_service_->Associate(socket_, nullptr, nullptr);
  } catch (Exception& e) {
    // 예외 발생시, 소켓을 닫아줍니다. 재접속 시도시 Socket::Connect()에서 소켓이 닫힌 경우 다시 생성하므로 문제 될것은 없습니다.
    socket_.Close();

    e.Rethrow();
  }

  // 아직, 연결되지 않았음. 연결중임...
  is_connected_ = false;
  is_connecting_ = true;
}


// 현재는 접속되어 있는 상태에서만 처리가 되는 구조인데,
// pending connecting일 경우에도 operation이 가능 하도록 수정하는게 바람직해보임.
// pending에 대한 처리도 해주어야할듯함.
// connect 요청시에 요청큐를 비워주어야 할것이고,
// connect 실패시에도 실패 사실을 알 수 있어야함.

void TcpClient::AsyncRead(const ReadRequest& request) {
  if (request.size <= 0) {
    return;
  }

  ScopedLock<Mutex> lock(read_requests_mutex_);

  //TODO 연결중이라면, 쌓아놔야함!!

  if (IsConnected()) {
    io_service_->SetReadCallback(socket_, [this](SOCKET fd) { OnReadAvailable(fd); });
    read_requests_.Enqueue(request);
  } else {
    //TODO logging
    //warning: tcp_client is disconnected
  }
}

void TcpClient::AsyncWrite(const WriteRequest& request) {
  if (request.buffer.Count() == 0) {
    return;
  }

  ScopedLock<Mutex> lock(write_requests_mutex_);

  if (IsConnected()) {
    io_service_->SetWriteCallback(socket_, [this](SOCKET fd) { OnWriteAvailable(fd); });
    write_requests_.Enqueue(request);
  } else {
    //TODO logging
    //warning: tcp_client is disconnected
  }
}

void TcpClient::OnConnected(SOCKET fd) {
  //TODO
  //connect에 실패할수도 있으므로, 그에 따른 성공여부를 호출자 쪽에서도 알 수 있어야함.
}

TcpClient::AsyncReadCallback TcpClient::ProcessRead(ReadResult& result) {
  ScopedLock<Mutex> lock(read_requests_mutex_);

  ReadRequest request;
  if (!read_requests_.Dequeue(request)) {
    return nullptr;
  }

  auto cb = request.async_read_cb; //이름을 그냥 Callback으로 하는게 좋을듯도...

  try {
    //버퍼 객체가 지속적으로 생성되는 문제가 있어보임.
    //재활용할 수 있는 방법이 있지 않을까??

    result.buffer.Resize(request.size);
    int32 received_byte_count = socket_.ReceiveBytes(result.buffer.GetData(), result.buffer.Count());
    result.buffer.Resize(received_byte_count);

    result.success = true;
  } catch (Exception& e) {
    result.success = false;

    LOG(LogCore, Warning, "%s", *e.GetDisplayText(), e.GetCode());
  }

  //더이상의 읽기 요청이 없는 경우에는 IoService에서 클리어해줌.
  if (read_requests_.IsEmpty()) {
    io_service_->SetReadCallback(socket_, nullptr);
  }

  return cb;
}

//TODO connect 실패시, 어떤 이벤트가 셋되는지??

void TcpClient::OnReadAvailable(SOCKET fd) {
  //소켓 버퍼내에 읽기가 가능한 데이터가 있으므로, 읽어들인 후 읽기요청에 대한 처리를 한다.
  ReadResult result;
  auto cb = ProcessRead(result);

  //읽기 동작이 실패한 경우에는 접속 자체를 끊어버린다.
  //재연결 매커니즘을 넣어주는것도 좋을듯 싶음.
  if (!result.success) {
    //TODO logging "read operation failure"
    Disconnect();
  }

  //지정한 콜백이 있을 경우 호출해줌.
  if (cb) {
    cb(result);
  }

  //읽기 동작이 실패한 경우에는 접속이 해제 되었으므로, 해제 되었음을 알려줌.
  if (!result.success) {
    CallDisconnectedHandler();
  }
}

void TcpClient::OnWriteAvailable(SOCKET fd) {
  WriteResult result;
  auto cb = ProcessWrite(result);
  if (!result.success) {
    //TODO logging "write operation failure"
    Disconnect();
  }

  if (cb) {
    cb(result);
  }

  if (!result.success) {
    CallDisconnectedHandler();
  }
}

TcpClient::AsyncWriteCallback TcpClient::ProcessWrite(WriteResult& result) {
  ScopedLock<Mutex> lock(write_requests_mutex_);

  WriteRequest request;
  if (!write_requests_.Dequeue(request)) {
    return nullptr;
  }

  auto cb = request.async_write_cb;
  try {
    result.size = socket_.SendBytes(request.buffer.GetData(), request.buffer.Count());
    result.success = true;
  } catch (Exception&) {
    result.success = false;
  }

  if (write_requests_.IsEmpty()) {
    io_service_->SetWriteCallback(socket_, nullptr);
  }

  return cb;
}

void TcpClient::CallDisconnectedHandler() {
  if (disconnected_cb_) {
    disconnected_cb_();
  }
}

void TcpClient::Disconnect(bool wait_for_removal) {
  if (!(IsConnected() || IsConnecting())) {
    return;
  }

  is_connected_ = false;
  is_connecting_ = false;

  ClearReadRequests();
  ClearWriteRequests();

  io_service_->Unassociate(socket_, wait_for_removal);

  socket_.Close();
}

void TcpClient::ClearReadRequests() {
  ScopedLock<Mutex> lock(read_requests_);
  
  Queue<ReadRequest> empty;
  Swap(read_requests_, empty);
}

void TcpClient::ClearWriteRequests() {
  ScopedLock<Mutex> lock(write_requests_);
  
  Queue<WriteRequest> empty;
  Swap(write_requests_, empty);
}

} // namespace fun
