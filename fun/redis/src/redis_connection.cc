#include "fun/redis/connection.h"

namespace fun {
namespace redis {

#define READ_BUFFER_SIZE 4096

Connection::Connection()
    : tcp_client_(MakeShareable<TcpClient>(new TcpClient)),
      connected_cb_(nullptr),
      disconnected_cb_(nullptr),
      reply_cb_(nullptr) {}

Connection::Connection(const SharedPtr<TcpClient>& tcp_client)
    : tcp_client_(tcp_client),
      connected_cb_(nullptr),
      disconnected_cb_(nullptr),
      reply_cb_(nullptr) {}

Connection::~Connection() { tcp_client_->Disconnect(true); }

void Connection::ConnectSync(const String& host, int32 port,
                             const DisconnectedCallback& disconnected_cb,
                             const ReplyCallback& reply_cb) {
  try {
    tcp_client_->SetOnConnectedHandler(nullptr);
    tcp_client_->SetOnDisconnectedHandler([this]() { OnTcpDisconnected(); });

    // Connect.
    tcp_client_->ConnectSync(host, port);

    // initial issue receive
    TcpClient::ReadRequest request = {
        READ_BUFFER_SIZE,
        [this](const TcpClient::ReadResult& result) { OnTcpReceive(result); }};
    tcp_client_->AsyncRead(request);
  } catch (Exception& e) {
    //아래와 같이 "throw e"를 통해서 rethrow할 경우, CException으로 복사되어서
    //예외가 전달됩니다. 이때 예외 클래스의 정보가 생략되어, CException의 정보로
    //대체되는 부작용이 있을 수 있습니다. throw Ex;
    e.Rethrow();
  }

  connected_cb_ = nullptr;
  disconnected_cb_ = disconnected_cb;
  reply_cb_ = reply_cb;
}

void Connection::ConnectAsync(const String& host, int32 port,
                              const ConnectedCallback& connected_cb,
                              const DisconnectedCallback& disconnected_cb,
                              const ReplyCallback& reply_cb) {
  try {
    tcp_client_->SetOnConnectedHandler([this]() { OnTcpConnected(); });
    tcp_client_->SetOnDisconnectedHandler([this]() { OnTcpDisconnected(); });

    tcp_client_->ConnectAsync(host, port);

    TcpClient::ReadRequest request = {
        READ_BUFFER_SIZE,
        [this](const TcpClient::ReadResult& result) { OnTcpReceive(result); }};
    tcp_client_->AsyncRead(request);
  } catch (Exception& e) {
    e.Rethrow();
  }

  connected_cb_ = connected_cb;
  disconnected_cb_ = disconnected_cb;
  reply_cb_ = reply_cb;
}

void Connection::Disconnect(bool wait_for_removal) {
  tcp_client_->Disconnect(wait_for_removal);
}

bool Connection::IsConnected() const { return tcp_client_->IsConnected(); }

bool Connection::IsConnecting() const { return tcp_client_->IsConnecting(); }

bool Connection::IsDisconnected() const {
  return tcp_client_->IsDisconnected();
}

String Connection::BuildCommand(const TArray<String>& redis_cmd) {
  String cmd;
  cmd << "*" << String::FromNumber(redis_cmd.Count()) << "\r\n";

  for (const auto& element : redis_cmd) {
    cmd << "$" << String::FromNumber(element.Len()) << "\r\n"
        << element << "\r\n";
  }

  return MoveTemp(cmd);
}

Connection& Connection::Send(const TArray<String>& redis_cmd) {
  ScopedLock lock(buffer_mutex_);
  buffer_ += BuildCommand(redis_cmd);
  return *this;
}

Connection& Connection::Commit() {
  ScopedLock lock(buffer_mutex_);

  if (buffer_.Len() == 0) {
    return *this;
  }

  try {
    //한번에 최대로 보낼수 있는 크기를 제한하는 기능도 넣으면 좋을듯...??

    TArray<char> block;
    block.ResizeUninitialized(buffer_.Len());
    UnsafeMemory::Memcpy(block.GetData(), buffer_.ConstData(), buffer_.Len());
    buffer_.Clear();  // clear

    TcpClient::WriteRequest request = {block, nullptr};
    tcp_client_->AsyncWrite(request);
  } catch (Exception& e) {
    // TODO throw exception
    (void)e;
  }

  return *this;
}

void Connection::CallDisconnectionHandler() {
  if (disconnected_cb_) {
    disconnected_cb_(*this);
  }
}

void Connection::OnTcpReceive(const TcpClient::ReadResult& result) {
  // 정상적으로 패킷을 수신하지 못한 경우에는 그냥 무시하도록 함.
  // 그냥 로그를 찍어주는 것도 한 방법이기는 하겠지만..
  if (!result.success) {
    return;
  }

  try {
    // builder_ << String(result.buffer_.GetData(), result.buffer_.Count());
    //복사를 줄이기 위해서, Raw 형태로 넘겨줌. 단 이때 result.Buffer는
    //완료시점까지 파괴되어서는 안됨.
    builder_ << String::FromRawData(result.buffer_.GetData(),
                                    result.buffer_.Count());
  } catch (Exception&) {
    // TODO 오류 발생시 Disconnection Handler를 호출할뿐, Disconnect()를
    // 호출하지 않는다. 어떤식으로 처리하는게 바람직할까??
    CallDisconnectionHandler();
    return;
  }

  // 유효한 reply가 큐에 있을 경우 처리함.
  while (builder_.ReplyAvailable()) {
    auto reply = builder_.GetFront();
    builder_.PopFront();

    if (reply_cb_) {
      reply_cb_(*this, reply);
    }
  }

  // re-issue
  try {
    TcpClient::ReadRequest request = {
        READ_BUFFER_SIZE,
        [this](const TcpClient::ReadResult& result) { OnTcpReceive(result); }};
    tcp_client_->AsyncRead(request);
  } catch (Exception&) {
    // 어짜피 접속이 끊겼을 것이므로, 별다른 처리는 필요치 않다...
  }
}

void Connection::OnTcpConnected() {
  if (connected_cb_) {
    connected_cb_(*this);
  }
}

void Connection::OnTcpDisconnected() {
  if (disconnected_cb_) {
    disconnected_cb_(*this);
  }
}

}  // namespace redis
}  // namespace fun
