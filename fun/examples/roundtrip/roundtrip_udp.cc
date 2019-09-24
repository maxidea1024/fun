#include "fun/base/logging.h"
#include "fun/net/channel.h"
#include "fun/net/event_loop.h"
#include "fun/net/socket.h"
#include "fun/net/sockets_ops.h"
#include "fun/net/tcp_client.h"
#include "fun/net/tcp_server.h"

#include <boost/bind.hpp>

#include <stdio.h>

using namespace fun;
using namespace fun::net;

const size_t FRAME_LENGTH = 2 * sizeof(int64);

int CreateNonblockingUDP() {
  int sock_fd =
      ::socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_UDP);
  if (sock_fd < 0) {
    LOG_SYSFATAL << "::socket";
  }
  return sock_fd;
}

//
// Server
//

void ServerReadCallback(int sock_fd, const Timestamp& received_time) {
  int64 message[2];
  struct sockaddr peer_addr;
  bzero(&peer_addr, sizeof peer_addr);
  socklen_t addr_len = sizeof peer_addr;
  ssize_t nr =
      ::recvfrom(sock_fd, message, sizeof message, 0, &peer_addr, &addr_len);

  char addr_str[64];
  sockets::ToIpPort(addr_str, sizeof addr_str, &peer_addr);
  LOG_DEBUG << "received " << nr << " bytes from " << addr_str;

  if (nr < 0) {
    LOG_SYSERR << "::recvfrom";
  } else if (ImplicitCast<size_t>(nr) == FRAME_LENGTH) {
    message[1] = received_time.microSecondsSinceEpoch();
    ssize_t nw =
        ::sendto(sock_fd, message, sizeof message, 0, &peer_addr, addr_len);
    if (nw < 0) {
      LOG_SYSERR << "::sendto";
    } else if (ImplicitCast<size_t>(nw) != FRAME_LENGTH) {
      LOG_ERROR << "Expect " << FRAME_LENGTH << " bytes, wrote " << nw
                << " bytes.";
    }
  } else {
    LOG_ERROR << "Expect " << FRAME_LENGTH << " bytes, received " << nr
              << " bytes.";
  }
}

void RunServer(uint16_t port) {
  Socket sock(CreateNonblockingUDP());
  sock.BindAddress(InetAddress(port));
  EventLoop loop;
  Channel channel(&loop, sock.fd());
  channel.SetReadCallback(boost::bind(&ServerReadCallback, sock.fd(), _1));
  channel.EnableReading();
  loop.Loop();
}

//
// Client
//

void ClientReadCallback(int sock_fd, const Timestamp& received_time) {
  int64 message[2];
  ssize_t nr = sockets::read(sock_fd, message, sizeof message);

  if (nr < 0) {
    LOG_SYSERR << "::read";
  } else if (ImplicitCast<size_t>(nr) == FRAME_LENGTH) {
    int64 send = message[0];
    int64 their = message[1];
    int64 back = received_time.microSecondsSinceEpoch();
    int64 mine = (back + send) / 2;
    LOG_INFO << "round trip " << back - send << " clock error " << their - mine;
  } else {
    LOG_ERROR << "Expect " << FRAME_LENGTH << " bytes, received " << nr
              << " bytes.";
  }
}

void SendMyTime(int sock_fd) {
  int64 message[2] = {0, 0};
  message[0] = Timestamp::Now().microSecondsSinceEpoch();
  ssize_t nw = sockets::write(sock_fd, message, sizeof message);
  if (nw < 0) {
    LOG_SYSERR << "::write";
  } else if (ImplicitCast<size_t>(nw) != FRAME_LENGTH) {
    LOG_ERROR << "Expect " << FRAME_LENGTH << " bytes, wrote " << nw
              << " bytes.";
  }
}

void RunClient(const char* ip, uint16_t port) {
  Socket sock(CreateNonblockingUDP());
  InetAddress server_addr(ip, port);
  int ret = sockets::Connect(sock.fd(), server_addr.GetSockAddr());
  if (ret < 0) {
    LOG_SYSFATAL << "::Connect";
  }

  EventLoop loop;
  Channel channel(&loop, sock.fd());
  channel.SetReadCallback(boost::bind(&ClientReadCallback, sock.fd(), _1));
  channel.EnableReading();
  loop.ScheduleEvery(0.2, boost::bind(SendMyTime, sock.fd()));
  loop.Loop();
}

int main(int argc, char* argv[]) {
  if (argc > 2) {
    uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
    if (strcmp(argv[1], "-s") == 0) {
      RunServer(port);
    } else {
      RunClient(argv[1], port);
    }
  } else {
    printf("Usage:\n%s -s port\n%s ip port\n", argv[0], argv[0]);
  }
}
