#include "fun/net/tcp_client.h"

namespace fun {
namespace net {

namespace internal {

void RemoveConnection(EventLoop* loop, const TcpConnectionPtr& conn) {
  loop->QueueInLoop(boost::bind(&TcpConnection::ConnectDestroyed, conn));
}

void RemoveConnector(const ConnectorPtr& connector) {
  //connector->
}

} // namespace internal


TcpClient::TcpClient(EventLoop* loop,
                     const InetAddress& server_addr,
                     const String& name)
  : loop_(CHECK_NOTNULL(loop)),
    connector_(new Connector(loop, server_addr)),
    name_(name),
    connection_cb_(DefaultConnectionCallback),
    message_cb_(DefaultMessageCallback),
    retry_(false),
    connect_(true),
    next_conn_id_(1) {
  connector_->SetNewConnectionCallback(
      boost::bind(&TcpClient::NewConnection, this, _1));

  // FIXME setConnectFailedCallback
  //LOG_INFO << "TcpClient::TcpClient[" << name_
  //         << "] - connector " << get_pointer(connector_);
}

TcpClient::~TcpClient() {
  //LOG_INFO << "TcpClient::~TcpClient[" << name_
  //         << "] - connector " << get_pointer(connector_);

  TcpConnectionPtr conn;
  bool unique = false; {
    ScopedLock guard(mutex_);
    unique = connection_.unique();
    conn = connection_;
  }

  if (conn) {
    fun_check(loop_ == conn->GetLoop());
    // FIXME: not 100% safe, if we are in different thread
    CloseCallback cb = boost::bind(&internal::RemoveConnection, loop_, _1);
    loop_->RunInLoop(boost::bind(&TcpConnection::SetCloseCallback, conn, cb));
    if (unique) {
      conn->ForceClose();
    }
  } else {
    connector_->Stop();
    // FIXME: HACK
    loop_->RunAfter(1, boost::bind(&internal::RemoveConnector, connector_));
  }
}

void TcpClient::Connect() {
  // FIXME: check state
  //LOG_INFO << "TcpClient::Connect[" << name_ << "] - connecting to "
  //         << connector_->GetServerAddress().ToIpPort();

  connect_ = true;
  connector_->Start();
}

void TcpClient::Disconnect() {
  connect_ = false;
  {
    ScopedLock guard(mutex_);
    if (connection_) {
      connection_->Shutdown();
    }
  }
}

void TcpClient::Stop() {
  connect_ = false;
  connector_->Stop();
}

void TcpClient::NewConnection(int sock_fd) {
  loop_->AssertInLoopThread();

  InetAddress peer_addr(sockets::GetPeerAddr(sock_fd));
  char buf[32];
  snprintf(buf, sizeof buf, ":%s#%d", peer_addr.ToIpPort().c_str(), next_conn_id_);
  ++next_conn_id_;
  String conn_name = name_ + buf;

  InetAddress local_addr(sockets::GetLocalAddr(sock_fd));
  // FIXME poll with zero timeout to double confirm the new connection
  // FIXME use make_shared if necessary
  TcpConnectionPtr conn(new TcpConnection(loop_,
                                          conn_name,
                                          sock_fd,
                                          local_addr,
                                          peer_addr));

  conn->SetConnectionCallback(connection_cb_);
  conn->SetMessageCallback(message_cb_);
  conn->SetWriteCompleteCallback(write_complete_cb_);
  conn->SetCloseCallback(boost::bind(&TcpClient::RemoveConnection, this, _1)); // FIXME: unsafe
  {
    ScopedLock guard(mutex_);
    connection_ = conn;
  }

  conn->ConnectEstablished();
}

void TcpClient::RemoveConnection(const TcpConnectionPtr& conn) {
  loop_->AssertInLoopThread();
  fun_check(loop_ == conn->GetLoop());
  {
    ScopedLock guard(mutex_);
    fun_check(connection_ == conn);
    connection_.Reset();
  }

  loop_->QueueInLoop(boost::bind(&TcpConnection::ConnectDestroyed, conn));

  if (retry_ && connect_) {
    //LOG_INFO << "TcpClient::Connect[" << name_ << "] - Reconnecting to "
    //         << connector_->GetServerAddress().ToIpPort();

    connector_->Restart();
  }
}

} // namespace net
} // namespace fun
