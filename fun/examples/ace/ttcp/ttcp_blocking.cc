#include <examples/ace/ttcp/common.h>
#include "fun/base/timestamp.h"

#undef NDEBUG

#include <fun_check.h>
#include <errno.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>

#include <netinet/in.h>
#include <arpa/inet.h>

static int acceptOrDie(uint16_t port)
{
  int listenfd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  fun_check(listenfd >= 0);

  int yes = 1;
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes))) {
    perror("setsockopt");
    exit(1);
  }

  struct sockaddr_in addr;
  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;
  if (bind(listenfd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr))) {
    perror("bind");
    exit(1);
  }

  if (listen(listenfd, 5)) {
    perror("listen");
    exit(1);
  }

  struct sockaddr_in peer_addr;
  bzero(&peer_addr, sizeof(peer_addr));
  socklen_t addrlen = 0;
  int sock_fd = ::accept(listenfd, reinterpret_cast<struct sockaddr*>(&peer_addr), &addrlen);
  if (sock_fd < 0) {
    perror("accept");
    exit(1);
  }
  ::close(listenfd);
  return sock_fd;
}

static int write_n(int sock_fd, const void* buf, int length)
{
  int written = 0;
  while (written < length) {
    ssize_t nw = ::write(sock_fd, static_cast<const char*>(buf) + written, length - written);
    if (nw > 0) {
      written += static_cast<int>(nw);
    }
    else if (nw == 0) {
      break;  // EOF
    }
    else if (errno != EINTR) {
      perror("write");
      break;
    }
  }
  return written;
}

static int read_n(int sock_fd, void* buf, int length)
{
  int nread = 0;
  while (nread < length) {
    ssize_t nr = ::read(sock_fd, static_cast<char*>(buf) + nread, length - nread);
    if (nr > 0) {
      nread += static_cast<int>(nr);
    }
    else if (nr == 0) {
      break;  // EOF
    }
    else if (errno != EINTR) {
      perror("read");
      break;
    }
  }
  return nread;
}

void transmit(const Options& opt)
{
  struct sockaddr_in addr = resolveOrDie(opt.host.c_str(), opt.port);
  printf("connecting to %s:%d\n", inet_ntoa(addr.sin_addr), opt.port);

  int sock_fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  fun_check(sock_fd >= 0);
  int ret = ::Connect(sock_fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));
  if (ret) {
    perror("Connect");
    printf("Unable to Connect %s\n", opt.host.c_str());
    ::close(sock_fd);
    return;
  }

  printf("connected\n");
  fun::Timestamp start(fun::Timestamp::Now());
  struct SessionMessage sessionMessage = { 0, 0 };
  sessionMessage.number = htonl(opt.number);
  sessionMessage.length = htonl(opt.length);
  if (write_n(sock_fd, &sessionMessage, sizeof(sessionMessage)) != sizeof(sessionMessage)) {
    perror("write SessionMessage");
    exit(1);
  }

  const int total_len = static_cast<int>(sizeof(int32_t) + opt.length);
  PayloadMessage* payload = static_cast<PayloadMessage*>(::malloc(total_len));
  fun_check(payload);
  payload->length = htonl(opt.length);
  for (int i = 0; i < opt.length; ++i) {
    payload->data[i] = "0123456789ABCDEF"[i % 16];
  }

  double total_mb = 1.0 * opt.length * opt.number / 1024 / 1024;
  printf("%.3f MiB in total\n", total_mb);

  for (int i = 0; i < opt.number; ++i) {
    int nw = write_n(sock_fd, payload, total_len);
    fun_check(nw == total_len);

    int ack = 0;
    int nr = read_n(sock_fd, &ack, sizeof(ack));
    fun_check(nr == sizeof(ack));
    ack = ntohl(ack);
    fun_check(ack == opt.length);
  }

  ::free(payload);
  ::close(sock_fd);
  double elapsed = TimeDifference(fun::Timestamp::Now(), start);
  printf("%.3f seconds\n%.3f MiB/s\n", elapsed, total_mb / elapsed);
}

void receive(const Options& opt)
{
  int sock_fd = acceptOrDie(opt.port);

  struct SessionMessage sessionMessage = { 0, 0 };
  if (read_n(sock_fd, &sessionMessage, sizeof(sessionMessage)) != sizeof(sessionMessage)) {
    perror("read SessionMessage");
    exit(1);
  }

  sessionMessage.number = ntohl(sessionMessage.number);
  sessionMessage.length = ntohl(sessionMessage.length);
  printf("receive number = %d\nreceive length = %d\n",
         sessionMessage.number, sessionMessage.length);
  const int total_len = static_cast<int>(sizeof(int32_t) + sessionMessage.length);
  PayloadMessage* payload = static_cast<PayloadMessage*>(::malloc(total_len));
  fun_check(payload);

  for (int i = 0; i < sessionMessage.number; ++i) {
    payload->length = 0;
    if (read_n(sock_fd, &payload->length, sizeof(payload->length)) != sizeof(payload->length)) {
      perror("read length");
      exit(1);
    }
    payload->length = ntohl(payload->length);
    fun_check(payload->length == sessionMessage.length);
    if (read_n(sock_fd, payload->data, payload->length) != payload->length) {
      perror("read payload data");
      exit(1);
    }
    int32_t ack = htonl(payload->length);
    if (write_n(sock_fd, &ack, sizeof(ack)) != sizeof(ack)) {
      perror("write ack");
      exit(1);
    }
  }
  ::free(payload);
  ::close(sock_fd);
}
