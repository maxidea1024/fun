#include "fun/base/process_info.h"
#include "fun/net/event_loop.h"
#include "pubsub.h"

#include <stdio.h>
#include <boost/bind.hpp>
#include <vector>

using namespace fun;
using namespace fun::net;
using namespace pubsub;

EventLoop* g_loop = NULL;
std::vector<String> g_topics;

void Subscription(const String& topic, const String& content,
                  const Timestamp&) {
  printf("%s: %s\n", topic.c_str(), content.c_str());
}

void Connection(PubSubClient* client) {
  if (client->IsConnected()) {
    for (std::vector<String>::iterator it = g_topics.begin();
         it != g_topics.end(); ++it) {
      client->Subscribe(*it, Subscription);
    }
  } else {
    g_loop->Quit();
  }
}

int main(int argc, char* argv[]) {
  if (argc > 2) {
    String host_port = argv[1];
    size_t colon = host_port.find(':');
    if (colon != String::npos) {
      String host_ip = host_port.substr(0, colon);
      uint16_t port =
          static_cast<uint16_t>(atoi(host_port.c_str() + colon + 1));
      for (int i = 2; i < argc; ++i) {
        g_topics.push_back(argv[i]);
      }

      EventLoop loop;
      g_loop = &loop;
      String name = ProcessInfo::username() + "@" + ProcessInfo::hostname();
      name += ":" + ProcessInfo::pidString();
      PubSubClient client(&loop, InetAddress(host_ip, port), name);
      client.SetConnectionCallback(Connection);
      client.Start();
      loop.Loop();
    } else {
      printf("Usage: %s hub_ip:port topic [topic ...]\n", argv[0]);
    }
  } else {
    printf("Usage: %s hub_ip:port topic [topic ...]\n", argv[0]);
  }
}
