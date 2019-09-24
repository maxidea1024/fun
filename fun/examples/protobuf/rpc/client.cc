#include <examples/protobuf/rpc/sudoku.pb.h>

#include <red/net/protorpc/RpcChannel.h>
#include "fun/base/logging.h"
#include "fun/net/event_loop.h"
#include "fun/net/inet_address.h"
#include "fun/net/tcp_client.h"
#include "fun/net/tcp_connection.h"

#include <boost/bind.hpp>

#include <stdio.h>
#include <unistd.h>

using namespace fun;
using namespace fun::net;

class RpcClient : Noncopyable {
 public:
  RpcClient(EventLoop* loop, const InetAddress& server_addr)
      : loop_(loop),
        client_(loop, server_addr, "RpcClient"),
        channel_(new RpcChannel),
        stub_(get_pointer(channel_)) {
    client_.SetConnectionCallback(
        boost::bind(&RpcClient::OnConnection, this, _1));
    client_.SetMessageCallback(
        boost::bind(&RpcChannel::OnMessage, get_pointer(channel_), _1, _2, _3));
    // client_.EnableRetry();
  }

  void Connect() { client_.Connect(); }

 private:
  void OnConnection(const TcpConnectionPtr& conn) {
    if (conn->IsConnected()) {
      // channel_.Reset(new RpcChannel(conn));
      channel_->setConnection(conn);
      sudoku::SudokuRequest request;
      request.set_checkerboard("001010");
      sudoku::SudokuResponse* response = new sudoku::SudokuResponse;

      stub_.Solve(NULL, &request, response,
                  NewCallback(this, &RpcClient::solved, response));
    } else {
      loop_->Quit();
    }
  }

  void solved(sudoku::SudokuResponse* resp) {
    LOG_INFO << "solved:\n" << resp->DebugString().c_str();
    client_.Disconnect();
  }

  EventLoop* loop_;
  TcpClient client_;
  RpcChannelPtr channel_;
  sudoku::SudokuService::Stub stub_;
};

int main(int argc, char* argv[]) {
  LOG_INFO << "pid = " << Process::CurrentPid();
  if (argc > 1) {
    EventLoop loop;
    InetAddress server_addr(argv[1], 9981);

    RpcClient rpcClient(&loop, server_addr);
    rpcClient.Connect();
    loop.Loop();
  } else {
    printf("Usage: %s host_ip\n", argv[0]);
  }
  google::protobuf::ShutdownProtobufLibrary();
}
