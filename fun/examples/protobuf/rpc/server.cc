#include <examples/protobuf/rpc/sudoku.pb.h>

#include <red/net/protorpc/RpcServer.h>
#include "fun/base/logging.h"
#include "fun/net/event_loop.h"

#include <boost/bind.hpp>

#include <unistd.h>

using namespace fun;
using namespace fun::net;

namespace sudoku {

class SudokuServiceImpl : public SudokuService {
 public:
  virtual void Solve(::google::protobuf::RpcController* controller,
                     const ::sudoku::SudokuRequest* request,
                     ::sudoku::SudokuResponse* response,
                     ::google::protobuf::Closure* done) {
    LOG_INFO << "SudokuServiceImpl::Solve";
    response->set_solved(true);
    response->set_checkerboard("1234567");
    done->Run();
  }
};

}  // namespace sudoku

int main() {
  LOG_INFO << "pid = " << Process::CurrentPid();
  EventLoop loop;
  InetAddress listen_addr(9981);
  sudoku::SudokuServiceImpl impl;
  RpcServer server(&loop, listen_addr);
  server.RegisterService(&impl);
  server.Start();
  loop.Loop();
  google::protobuf::ShutdownProtobufLibrary();
}
