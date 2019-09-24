#include <examples/curl/Curl.h>
#include <stdio.h>
#include <boost/bind.hpp>
#include "fun/net/event_loop.h"

using namespace fun::net;

EventLoop* g_loop = NULL;

void OnData(const char* data, int len) { printf("len %d\n", len); }

void Done(curl::Request* c, int code) {
  printf("Done %p %s %d\n", c, c->GetEffectiveUrl(), code);
}

void Done2(curl::Request* c, int code) {
  printf("Done2 %p %s %d %d\n", c, c->GetRedirectUrl(), c->GetResponseCode(),
         code);
  // g_loop->Quit();
}

int main(int argc, char* argv[]) {
  EventLoop loop;
  g_loop = &loop;
  loop.ScheduleAfter(30.0, boost::bind(&EventLoop::Quit, &loop));
  curl::Curl::Initialize(curl::Curl::kCURLssl);
  curl::Curl curl(&loop);

  curl::RequestPtr req = curl.GetUrl("http://chenshuo.com");
  req->SetDataCallback(OnData);
  req->SetDoneCallback(Done);

  curl::RequestPtr req2 = curl.GetUrl("https://github.com");
  // req2->AllowRedirect(5);
  req2->SetDataCallback(OnData);
  req2->SetDoneCallback(Done);

  curl::RequestPtr req3 = curl.GetUrl("http://example.com");
  // req3->AllowRedirect(5);
  req3->SetDataCallback(OnData);
  req3->SetDoneCallback(Done2);

  loop.Loop();
}
