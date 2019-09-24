#include "fun/base/process_info.h"
#include "fun/net/event_loop.h"
#include "fun/net/event_loop_thread.h"
#include "pubsub.h"

#include <stdio.h>
#include <iostream>

using namespace fun;
using namespace fun::net;
using namespace pubsub;

EventLoop* g_loop = nullptr;
String g_topic;
String g_content;

void Connection(PubSubClient* client) {
  if (client->IsConnected()) {
    client->Publish(g_topic, g_content);
    client->Stop();
  } else {
    g_loop->Quit();
  }
}

int main(int argc, char* argv[]) {
  if (argc == 4) {
    String host_port = argv[1];
    size_t colon = host_port.find(':');
    if (colon != String::npos) {
      String host_ip = host_port.substr(0, colon);
      uint16_t port =
          static_cast<uint16_t>(atoi(host_port.c_str() + colon + 1));
      g_topic = argv[2];
      g_content = argv[3];

      String name = ProcessInfo::username() + "@" + ProcessInfo::hostname();
      name += ":" + ProcessInfo::pidString();

      if (g_content == "-") {
        EventLoopThread loop_thread;
        g_loop = loop_thread.StartLoop();
        PubSubClient client(g_loop, InetAddress(host_ip, port), name);
        client.Start();

        String line;
        while (getline(std::cin, line)) {
          client.Publish(g_topic, line);
        }
        client.Stop();
        CurrentThread::sleepUsec(1000 * 1000);
      } else {
        EventLoop loop;
        g_loop = &loop;
        PubSubClient client(g_loop, InetAddress(host_ip, port), name);
        client.SetConnectionCallback(Connection);
        client.Start();
        loop.Loop();
      }
    } else {
      printf("Usage: %s hub_ip:port topic content\n", argv[0]);
    }
  } else {
    printf(
        "Usage: %s hub_ip:port topic content\n"
        "Read contents from stdin:\n"
        "  %s hub_ip:port topic -\n",
        argv[0], argv[0]);
  }
}
