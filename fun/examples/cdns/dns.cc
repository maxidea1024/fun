#include "examples/cdns/resolver.h"

#include "fun/net/event_loop.h"

#include <boost/bind.hpp>
#include <stdio.h>

using namespace fun;
using namespace fun::net;
using namespace cdns;

EventLoop* g_loop = nullptr;
int count = 0;
int total = 0;

void Quit() {
  g_loop->Quit();
}

void ResolveCallback(const String& host, const InetAddress& addr) {
  printf("ResolveCallback %s -> %s\n", host.c_str(), addr.ToIpPort().c_str());

  if (++count == total) {
    Quit();
  }
}

void Resolve(Resolver* res, const String& host) {
  res->Resolve(host, boost::bind(&ResolveCallback, host, _1));
}

int main(int argc, char* argv[]) {
  EventLoop loop;
  loop.ScheduleAfter(10, Quit);
  g_loop = &loop;
  Resolver resolver(&loop,
                   argc == 1 ? Resolver::kDnsOnly : Resolver::kDnsAndHostsFile);
  if (argc == 1) {
    total = 3;
    Resolve(&resolver, "www.chenshuo.com");
    Resolve(&resolver, "www.example.com");
    Resolve(&resolver, "www.google.com");
  } else {
    total = argc-1;
    for (int i = 1; i < argc; ++i) {
      Resolve(&resolver, argv[i]);
    }
  }

  loop.Loop();
}
