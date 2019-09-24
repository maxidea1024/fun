#include <red/base/Logging.h>
#include <red/base/Thread.h>
#include "fun/net/channel.h"
#include <red/net/EventLoop.h>

#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <stdio.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace fun;
using namespace fun::net;

std::vector<int> g_pipes;
int pipe_count;
int active_count;
int write_count;
EventLoop* g_loop;
boost::ptr_vector<Channel> g_channels;

int g_reads, g_writes, g_fired;

void ReadCallback(Timestamp, int fd, int idx) {
  char ch;

  g_reads += static_cast<int>(::recv(fd, &ch, sizeof(ch), 0));
  if (g_writes > 0) {
    int widx = idx+1;
    if (widx >= pipe_count) {
      widx -= pipe_count;
    }
    ::send(g_pipes[2 * widx + 1], "m", 1, 0);
    g_writes--;
    g_fired++;
  }

  if (g_fired == g_reads) {
    g_loop->Quit();
  }
}

std::pair<int, int> RunOnce() {
  Timestamp beforeInit(Timestamp::Now());
  for (int i = 0; i < pipe_count; ++i) {
    Channel& channel = g_channels[i];
    channel.setReadCallback(boost::bind(ReadCallback, _1, channel.fd(), i));
    channel.enableReading();
  }

  int space = pipe_count / active_count;
  space *= 2;
  for (int i = 0; i < active_count; ++i) {
    ::send(g_pipes[i * space + 1], "m", 1, 0);
  }

  g_fired = active_count;
  g_reads = 0;
  g_writes = write_count;
  Timestamp beforeLoop(Timestamp::Now());
  g_loop->Loop();

  Timestamp end(Timestamp::Now());

  int iterTime = static_cast<int>(end.microSecondsSinceEpoch() - beforeInit.microSecondsSinceEpoch());
  int loopTime = static_cast<int>(end.microSecondsSinceEpoch() - beforeLoop.microSecondsSinceEpoch());
  return std::make_pair(iterTime, loopTime);
}

int main(int argc, char* argv[]) {
  pipe_count = 100;
  active_count = 1;
  write_count = 100;
  int c;
  while ((c = getopt(argc, argv, "n:a:w:")) != -1) {
    switch (c) {
      case 'n':
        pipe_count = atoi(optarg);
        break;
      case 'a':
        active_count = atoi(optarg);
        break;
      case 'w':
        write_count = atoi(optarg);
        break;
      default:
        fprintf(stderr, "Illegal argument \"%c\"\n", c);
        return 1;
    }
  }

  struct rlimit rl;
  rl.rlim_cur = rl.rlim_max = pipe_count * 2 + 50;
  if (::setrlimit(RLIMIT_NOFILE, &rl) == -1) {
    perror("setrlimit");
    //return 1;  // comment out this line if under valgrind
  }
  g_pipes.resize(2 * pipe_count);
  for (int i = 0; i < pipe_count; ++i) {
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, &g_pipes[i*2]) == -1) {
      perror("pipe");
      return 1;
    }
  }

  EventLoop loop;
  g_loop = &loop;

  for (int i = 0; i < pipe_count; ++i) {
    Channel* channel = new Channel(&loop, g_pipes[i*2]);
    g_channels.push_back(channel);
  }

  for (int i = 0; i < 25; ++i) {
    std::pair<int, int> t = RunOnce();
    printf("%8d %8d\n", t.first, t.second);
  }

  for (boost::ptr_vector<Channel>::iterator it = g_channels.begin();
       it != g_channels.end(); ++it) {
    it->DisableAll();
    it->Remove();
  }
  g_channels.Clear();
}
