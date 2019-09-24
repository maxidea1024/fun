#include "fun/base/logging.h"
#include "fun/net/event_loop.h"
#include "fun/net/tcp_client.h"
#include "fun/net/tcp_server.h"

#include <stdio.h>

using namespace fun;
using namespace fun::net;

const size_t FRAME_LENGTH = 2*sizeof(int64);

void ServerConnectionCallback(const TcpConnectionPtr& conn) {
  LOG_TRACE << conn->GetName() << " "
        << conn->GetPeerAddress().ToIpPort() << " -> "
        << conn->GetLocalAddress().ToIpPort() << " is "
        << (conn->IsConnected() ? "UP" : "DOWN");

  if (conn->IsConnected()) {
    conn->SetTcpNoDelay(true);
  }
  else {
  }
}

void ServerMessageCallback(const TcpConnectionPtr& conn,
                           Buffer* buffer,
                           const Timestamp& received_time) {
  int64 message[2];
  while (buffer->GetReadableLength() >= FRAME_LENGTH) {
    UnsafeMemory::Memcpy(message, buffer->GetReadablePtr(), FRAME_LENGTH);
    buffer->Drain(FRAME_LENGTH);
    message[1] = received_time.microSecondsSinceEpoch();
    conn->Send(message, sizeof message);
  }
}

void RunServer(uint16 port) {
  EventLoop loop;
  TcpServer server(&loop, InetAddress(port), "ClockServer");
  server.SetConnectionCallback(ServerConnectionCallback);
  server.SetMessageCallback(ServerMessageCallback);
  server.Start();
  loop.Loop();
}

TcpConnectionPtr client_connection;

void ClientConnectionCallback(const TcpConnectionPtr& conn) {
  LOG_TRACE << conn->GetLocalAddress().ToIpPort() << " -> "
        << conn->GetPeerAddress().ToIpPort() << " is "
        << (conn->IsConnected() ? "UP" : "DOWN");

  if (conn->IsConnected()) {
    client_connection = conn;
    conn->SetTcpNoDelay(true);
  }
  else {
    client_connection.Reset();
  }
}

void ClientMessageCallback(const TcpConnectionPtr&,
                           Buffer* buffer,
                           const Timestamp& received_time) {
  int64 message[2];
  while (buffer->GetReadableLength() >= FRAME_LENGTH) {
    UnsafeMemory::Memcpy(message, buffer->GetReadablePtr(), FRAME_LENGTH);
    buffer->Drain(FRAME_LENGTH);

    int64 send = message[0];
    int64 their = message[1];
    int64 back = received_time.microSecondsSinceEpoch();
    int64 mine = (back + send) / 2;
    LOG_INFO << "round trip " << back - send
             << " clock error " << their - mine;
  }
}

void SendMyTime() {
  if (client_connection) {
    int64 message[2] = { 0, 0 };
    message[0] = Timestamp::Now().microSecondsSinceEpoch();
    client_connection->Send(message, sizeof message);
  }
}

void RunClient(const char* ip, uint16 port) {
  EventLoop loop;
  TcpClient client(&loop, InetAddress(ip, port), "ClockClient");
  client.EnableRetry();
  client.SetConnectionCallback(ClientConnectionCallback);
  client.SetMessageCallback(ClientMessageCallback);
  client.Connect();
  loop.ScheduleEvery(0.2, SendMyTime);
  //TODO 클라이언트에서 이 루프가 무한으로 반복되면, 프레임이 갱신이 안되므로
  //스레드로 돌려야하나? 아니면, timeout을 0으로 주어서 polling형태로 처리해야할까??
  loop.Loop();
}

int main(int argc, char* argv[]) {
  if (argc > 2) {
    uint16 port = static_cast<uint16>(atoi(argv[2]));

    if (strcmp(argv[1], "-s") == 0) {
      RunServer(port);
    }
    else {
      RunClient(argv[1], port);
    }
  }
  else {
    printf("Usage:\n%s -s port\n%s ip port\n", argv[0], argv[0]);
  }
}
