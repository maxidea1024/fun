#include "examples/cdns/resolver.h"

#include "fun/base/logging.h"
#include "fun/net/channel.h"
#include "fun/net/event_loop.h"

#include <boost/bind.hpp>

#include <ares.h>
#include <netdb.h>
#include <arpa/inet.h>  // inet_ntop
#include <netinet/in.h>

#include <stdlib.h>
#include <stdio.h>
#include <fun_check.h>

using namespace fun;
using namespace fun::net;
using namespace cdns;

namespace {

double GetSeconds(struct timeval* tv) {
  if (tv) {
    return double(tv->tv_sec) + double(tv->tv_usec) / 1000000.0;
  } else {
    return -1.0;
  }
}

const char* GetSocketType(int type) {
  if (type == SOCK_DGRAM) {
    return "UDP";
  } else if (type == SOCK_STREAM) {
    return "TCP";
  } else {
    return "Unknown";
  }
}

const bool kDebug = false;

} // namespace

Resolver::Resolver(EventLoop* loop, Option opt)
  : loop_(loop)
  , context_(nullptr)
  , timer_active_(false) {
  static char lookups[] = "b";
  struct ares_options options;
  int optmask = ARES_OPT_FLAGS;
  options.flags = ARES_FLAG_NOCHECKRESP;
  options.flags |= ARES_FLAG_STAYOPEN;
  options.flags |= ARES_FLAG_IGNTC; // UDP only
  optmask |= ARES_OPT_SOCK_STATE_CB;
  options.sock_state_cb = &Resolver::ares_sock_state_callback;
  options.sock_state_cb_data = this;
  optmask |= ARES_OPT_TIMEOUT;
  options.timeout = 2;
  if (opt == kDnsOnly) {
    optmask |= ARES_OPT_LOOKUPS;
    options.lookups = lookups;
  }

  int status = ares_init_options(&context_, &options, optmask);
  if (status != ARES_SUCCESS) {
    fun_check(0);
  }
  ares_set_socket_callback(context_, &Resolver::ares_sock_create_callback, this);
}

Resolver::~Resolver() {
  ares_destroy(context_);
}

bool Resolver::Resolve(StringArg hostname, const Callback& cb) {
  loop_->AssertInLoopThread();

  QueryData* query_data = new QueryData(this, cb);
  ares_gethostbyname(context_, hostname.c_str(), AF_INET,
      &Resolver::ares_host_callback, query_data);
  struct timeval tv;
  struct timeval* tvp = ares_timeout(context_, nullptr, &tv);
  double timeout = GetSeconds(tvp);

  LOG_DEBUG << "timeout " <<  timeout << " active " << timer_active_;

  if (!timer_active_) {
    loop_->ScheduleAfter(timeout, boost::bind(&Resolver::OnTimer, this));
    timer_active_ = true;
  }

  return query_data != nullptr;
}

void Resolver::OnRead(int sock_fd, const Timestamp& t) {
  LOG_DEBUG << "OnRead " << sock_fd << " at " << t.ToString();

  ares_process_fd(context_, sock_fd, ARES_SOCKET_BAD);
}

void Resolver::OnTimer() {
  fun_check(timer_active_ == true);
  ares_process_fd(context_, ARES_SOCKET_BAD, ARES_SOCKET_BAD);
  struct timeval tv;
  struct timeval* tvp = ares_timeout(context_, NULL, &tv);
  double timeout = GetSeconds(tvp);
  LOG_DEBUG << loop_->GetPollReturnTime().ToString() << " next timeout " <<  timeout;

  if (timeout < 0) {
    timer_active_ = false;
  } else {
    loop_->ScheduleAfter(timeout, boost::bind(&Resolver::OnTimer, this));
  }
}

void Resolver::OnQueryResult( int status,
                              struct hostent* result,
                              const Callback& callback) {
  LOG_DEBUG << "OnQueryResult " << status;

  struct sockaddr_in addr;
  bzero(&addr, sizeof addr);
  addr.sin_family = AF_INET;
  addr.sin_port = 0;
  if (result) {
    addr.sin_addr = *reinterpret_cast<in_addr*>(result->h_addr);
    if (kDebug) {
      printf("h_name %s\n", result->h_name);
      for (char** alias = result->h_aliases; *alias != NULL; ++alias) {
        printf("alias: %s\n", *alias);
      }
      // printf("ttl %d\n", ttl);
      // printf("h_length %d\n", result->h_length);
      for (char** haddr = result->h_addr_list; *haddr != NULL; ++haddr) {
        char buf[32];
        inet_ntop(AF_INET, *haddr, buf, sizeof buf);
        printf("  %s\n", buf);
      }
    }
  }
  InetAddress inet(addr);
  callback(inet);
}

void Resolver::OnSockCreate(int sock_fd, int type) {
  loop_->AssertInLoopThread();

  fun_check(channels_.find(sock_fd) == channels_.end());
  Channel* channel = new Channel(loop_, sock_fd);
  channel->SetReadCallback(boost::bind(&Resolver::OnRead, this, sock_fd, _1));
  channel->EnableReading();
  channels_.insert(sock_fd, channel);
}

void Resolver::OnSockStateChange(int sock_fd, bool read, bool write) {
  loop_->AssertInLoopThread();

  ChannelList::iterator it = channels_.find(sock_fd);
  fun_check(it != channels_.end());
  if (read) {
    // update
    // if (write) { } else { }
  } else {
    // remove
    it->second->DisableAll();
    it->second->Remove();
    channels_.erase(it);
  }
}


//
// ARES callbacks
//

void Resolver::ares_host_callback(void* data,
                                  int status,
                                  int timeouts,
                                  struct hostent* hostent) {
  QueryData* query = static_cast<QueryData*>(data);

  query->owner->OnQueryResult(status, hostent, query->callback);
  delete query;
}

int Resolver::ares_sock_create_callback(int sock_fd, int type, void* data) {
  LOG_TRACE << "sock_fd=" << sock_fd << " type=" << GetSocketType(type);
  static_cast<Resolver*>(data)->OnSockCreate(sock_fd, type);
  return 0;
}

void Resolver::ares_sock_state_callback(void* data,
                                        int sock_fd,
                                        int read,
                                        int write) {
  LOG_TRACE << "sock_fd=" << sock_fd << " read=" << read << " write=" << write;
  static_cast<Resolver*>(data)->OnSockStateChange(sock_fd, read, write);
}
