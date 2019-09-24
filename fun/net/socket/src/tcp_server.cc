#include "CorePrivatePCH.h"
#include "Net/socket/NetSocket.h"
#include "Net/socket/TcpServer.h"
#include "Net/socket/SocketIoService.h"

namespace fun {

TcpServer::TcpServer()
  : io_service_(SocketIoService::GetDefaultIoService()),
    new_connection_cb_(nullptr) {}

TcpServer::~TcpServer() {
  Stop();
}

bool TcpServer::operator == (const TcpServer& rhs) const {
  return socket_ == rhs.socket_;
}

bool TcpServer::operator != (const TcpServer& rhs) const {
  return !operator == (rhs);
}

void TcpServer::Start(const String& addr, uint32 port, const NewConnectionCallback& cb) {
  if (IsRunning()) {
    //TODO throw
    //"tcp_server is already running"
  }

  InetAddress ep(String(addr), port);

  socket_.Bind(ep);
  socket_.Listen(10);

  io_service_->Associate(socket_);
  io_service_->SetReadCallback(socket_, [this](SOCKET fd) { OnReadAvailable(fd); });
  new_connection_cb_ = cb;

  is_running_ = true;
}

void TcpServer::Stop(bool wait_for_removal, bool recursive_wait_for_removal) {
  if (!IsRunning()) {
    return;
  }

  is_running_ = false;

  io_service_->Unassociate(socket_, wait_for_removal);
  socket_.Close();

  ScopedLock<Mutex> lock(clients_mutex_);
  for (auto& client : clients_) {
    client->Disconnect(recursive_wait_for_removal && wait_for_removal);
  }
  clients_.Clear();
}

bool TcpServer::IsRunning() const {
  return is_running_;
}

void TcpServer::OnReadAvailable(SOCKET fd) {
  try {
    SharedPtr<TcpClient> client = MakeShareable<TcpClient>(new TcpClient(socket_.AcceptConnection()));

    if (!new_connection_cb_ || !new_connection_cb_(client)) {
      client->SetDisconnectedCallback([client,this]() { OnClientDisconnected(client); });
      clients_.Add(client);
    }
  } catch (Exception&) {
    Stop();
  }
}

void TcpServer::OnClientDisconnected(const SharedPtr<TcpClient>& client) {
  if (!IsRunning()) {
    return;
  }

  ScopedLock<Mutex> lock(clients_mutex_);
  clients_.Remove(client);
}

const Array<SharedPtr<TcpClient>>& TcpServer::GetClients() const {
  return clients_;
}

} // namespace fun
