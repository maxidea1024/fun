#pragma once

#include "fun/base/string_piece.h"
#include "fun/base/timestamp.h"
#include "fun/net/inet_address.h"

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_map.hpp>

extern "C" {
  struct hostent;
  struct ares_channeldata;
  typedef struct ares_channeldata* ares_channel;
}

namespace fun {
namespace net {
class Channel;
class EventLoop;
}
}

namespace cdns {

class Resolver : Noncopyable {
 public:
  typedef Function<void(const fun::net::InetAddress&)> Callback;

  enum Option {
    kDnsAndHostsFile,
    kDnsOnly,
  };

  explicit Resolver(fun::net::EventLoop* loop,
                    Option opt = kDnsAndHostsFile);

  ~Resolver();

  bool Resolve(fun::StringArg hostname, const Callback& cb);

 private:
  struct QueryData {
    Resolver* owner;
    Callback callback;

    QueryData(Resolver* o, const Callback& cb)
      : owner(o)
      , callback(cb) {
    }
  };

  fun::net::EventLoop* loop_;
  ares_channel context_;
  bool timer_active_;
  typedef boost::ptr_map<int, fun::net::Channel> ChannelList;
  ChannelList channels_;

  void OnRead(int sock_fd, const fun::Timestamp& t);
  void OnTimer();
  void OnQueryResult(int status, struct hostent* result, const Callback& cb);
  void OnSockCreate(int sock_fd, int type);
  void OnSockStateChange(int sock_fd, bool read, bool write);

  //
  // ARES callbacks
  //

  static void ares_host_callback(void* data, int status, int timeouts, struct hostent* hostent);
  static int ares_sock_create_callback(int sock_fd, int type, void* data);
  static void ares_sock_state_callback(void* data, int sock_fd, int read, int write);
};

} // namespace cdns
