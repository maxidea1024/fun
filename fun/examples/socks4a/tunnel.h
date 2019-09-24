#pragma once

#include "fun/base/logging.h"
#include "fun/net/event_loop.h"
#include "fun/net/inet_address.h"
#include "fun/net/tcp_client.h"
#include "fun/net/tcp_server.h"

//#include <boost/bind.hpp>
//#include <boost/enable_shared_from_this.hpp>
#include "fun/base/weak_ptr.h"

class Tunnel : public EnableSharedFromThis<Tunnel>, Noncopyable {
 public:
  Tunnel(fun::net::EventLoop* loop, const fun::net::InetAddress& server_addr,
         const fun::net::TcpConnectionPtr& server_conn)
      : client_(loop, server_addr, server_conn->GetName()),
        server_conn_(server_conn) {
    LOG_INFO << "Tunnel " << server_conn->GetPeerAddress().ToIpPort() << " <-> "
             << server_addr.ToIpPort();
  }

  ~Tunnel() { LOG_INFO << "~Tunnel"; }

  void Setup() {
    client_.SetConnectionCallback(
        boost::bind(&Tunnel::OnClientConnection, SharedFromThis(), _1));
    client_.SetMessageCallback(
        boost::bind(&Tunnel::OnClientMessage, SharedFromThis(), _1, _2, _3));
    server_conn_->SetHighWaterMarkCallback(
        boost::bind(&Tunnel::OnHighWaterMarkWeak,
                    WeakPtr<Tunnel>(SharedFromThis()), kServer, _1, _2),
        1024 * 1024);
  }

  void Connect() { client_.Connect(); }

  void Disconnect() {
    client_.Disconnect();
    // server_conn_.Reset();
  }

 private:
  void Teardown() {
    client_.SetConnectionCallback(fun::net::DefaultConnectionCallback);
    client_.SetMessageCallback(fun::net::DefaultMessageCallback);

    if (server_conn_) {
      server_conn_->SetContext(boost::any());
      server_conn_->Shutdown();
    }

    client_conn_.Reset();
  }

  void OnClientConnection(const fun::net::TcpConnectionPtr& conn) {
    LOG_DEBUG << (conn->IsConnected() ? "UP" : "DOWN");

    if (conn->IsConnected()) {
      conn->SetTcpNoDelay(true);
      // TODO High water mark�� ��������??
      conn->SetHighWaterMarkCallback(
          boost::bind(&Tunnel::OnHighWaterMarkWeak,
                      WeakPtr<Tunnel>(SharedFromThis()), kClient, _1, _2),
          1024 * 1024);
      server_conn_->SetContext(conn);
      server_conn_->StartRead();
      client_conn_ = conn;
      if (server_conn_->GetInputBuffer()->GetReadableLength() > 0) {
        conn->Send(server_conn_->GetInputBuffer());
      }
    } else {
      Teardown();
    }
  }

  void OnClientMessage(const fun::net::TcpConnectionPtr& conn,
                       fun::net::Buffer* buf, const fun::Timestamp&) {
    LOG_DEBUG << conn->GetName() << " " << buf->GetReadableLength();

    if (server_conn_) {
      server_conn_->Send(buf);
    } else {
      buf->DrainAll();
      abort();
    }
  }

  enum ServerClient { kServer, kClient };

  void OnHighWaterMark(ServerClient which,
                       const fun::net::TcpConnectionPtr& conn,
                       size_t bytes_to_sent) {
    LOG_INFO << (which == kServer ? "server" : "client") << " OnHighWaterMark "
             << conn->GetName() << " bytes " << bytes_to_sent;

    if (which == kServer) {
      if (server_conn_->GetOutputBuffer()->GetReadableLength() > 0) {
        client_conn_->StopRead();
        server_conn_->SetWriteCompleteCallback(
            boost::bind(&Tunnel::OnWriteCompleteWeak,
                        WeakPtr<Tunnel>(SharedFromThis()), kServer, _1));
      }
    } else {
      if (client_conn_->GetOutputBuffer()->GetReadableLength() > 0) {
        server_conn_->StopRead();
        client_conn_->SetWriteCompleteCallback(
            boost::bind(&Tunnel::OnWriteCompleteWeak,
                        WeakPtr<Tunnel>(SharedFromThis()), kClient, _1));
      }
    }
  }

  static void OnHighWaterMarkWeak(const WeakPtr<Tunnel>& weak_tunnel,
                                  ServerClient which,
                                  const fun::net::TcpConnectionPtr& conn,
                                  size_t bytes_to_sent) {
    SharedPtr<Tunnel> tunnel = weak_tunnel.Lock();
    if (tunnel) {
      tunnel->OnHighWaterMark(which, conn, bytes_to_sent);
    }
  }

  void OnWriteComplete(ServerClient which,
                       const fun::net::TcpConnectionPtr& conn) {
    LOG_INFO << (which == kServer ? "server" : "client") << " OnWriteComplete "
             << conn->GetName();
    if (which == kServer) {
      client_conn_->StartRead();
      server_conn_->SetWriteCompleteCallback(fun::net::WriteCompleteCallback());
    } else {
      server_conn_->StartRead();
      client_conn_->SetWriteCompleteCallback(fun::net::WriteCompleteCallback());
    }
  }

  static void OnWriteCompleteWeak(const WeakPtr<Tunnel>& weak_tunnel,
                                  ServerClient which,
                                  const fun::net::TcpConnectionPtr& conn) {
    SharedPtr<Tunnel> tunnel = weak_tunnel.Lock();
    if (tunnel) {
      tunnel->OnWriteComplete(which, conn);
    }
  }

 private:
  fun::net::TcpClient client_;
  fun::net::TcpConnectionPtr server_conn_;
  fun::net::TcpConnectionPtr client_conn_;
};

typedef SharedPtr<Tunnel> TunnelPtr;
