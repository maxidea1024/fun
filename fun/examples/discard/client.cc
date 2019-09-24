#include "fun/net/inet_address.h"
#include "fun/net/reactor/event_loop.h"
#include "fun/net/reactor/tcp_client.h"

class DiscardClient : Noncopyable {
 public:
  DiscardClient(EventLoop* loop, const InetAddress& server_addr, int size)
      : loop_(loop),
        client_(loop, server_addr, "DiscardClient"),
        message_(size, 'H') {
    client_.SetConnectionCallback(
        boost::bind(&DiscardClient::OnConnection, this, _1));
    client_.SetMessageCallback(
        boost::bind(&DiscardClient::OnMessage, this, _1, _2, _3));
    client_.SetWriteCompleteCallback(
        boost::bind(&DiscardClient::OnWriteComplete, this, _1));

    // client_.EnableRetry();
  }

  void Connect() { client_.Connect(); }

 private:
  void OnConnection(const TcpConnectionPtr& conn) {
    LOG_TRACE << conn->GetLocalAddress().ToIpPort() << " -> "
              << conn->GetPeerAddress().ToIpPort() << " is "
              << (conn->IsConnected() ? "UP" : "DOWN");

    if (conn->IsConnected()) {
      conn->SetTcpNoDelay(true);
      conn->Send(message_);
    } else {
      loop_->Quit();
    }
  }

  void OnMessage(const TcpConnectionPtr& conn, Buffer* buf,
                 const Timestamp& received_time) {
    buf->DrainAll();
  }

  void OnWriteComplete(const TcpConnectionPtr& conn) {
    LOG_INFO << "write complete " << message_.size();
    conn->Send(message_);
  }

  //
  // Member variables
  //

  EventLoop* loop_;
  TcpClient client_;
  String message_;
};

int main(int argc, char* argv[]) {
  LOG_INFO << "pid = " << Process::CurrentPid()
           << ", tid = " << Thread::CurrentTid();

  if (argc > 1) {
    g_thread_count = atoi(argv[1]);
  }

  EventLoop loop;
  InetAddress listen_addr(2009);
  DiscardClient client(&loop, listen_addr);
  client.Connect();
  loop.Loop();  // TODO 클라이언트에서는 무한 루프를 돌리기가...??
}
