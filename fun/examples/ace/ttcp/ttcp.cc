#include <examples/ace/ttcp/common.h>

#include "fun/base/logging.h"
#include "fun/net/event_loop.h"
#include "fun/net/tcp_client.h"
#include "fun/net/tcp_server.h"

#include <boost/bind.hpp>

#include <stdio.h>

using namespace fun;
using namespace fun::net;

EventLoop* g_loop;

struct Context {
  int count;
  int64_t bytes;
  SessionMessage session;
  Buffer output;

  Context() : count(0), bytes(0) {
    session.number = 0;
    session.length = 0;
  }
};

/////////////////////////////////////////////////////////////////////
// T R A N S M I T
/////////////////////////////////////////////////////////////////////

namespace trans {

void OnConnection(const Options& opt, const TcpConnectionPtr& conn) {
  if (conn->IsConnected()) {
    printf("connected\n");
    Context context;
    context.count = 1;
    context.bytes = opt.length;
    context.session.number = opt.number;
    context.session.length = opt.length;
    context.output.AppendInt32(opt.length);
    context.output.ensureWritableBytes(opt.length);
    for (int i = 0; i < opt.length; ++i) {
      context.output.beginWrite()[i] = "0123456789ABCDEF"[i % 16];
    }
    context.output.hasWritten(opt.length);
    conn->SetContext(context);

    SessionMessage sessionMessage = {0, 0};
    sessionMessage.number = htonl(opt.number);
    sessionMessage.length = htonl(opt.length);
    conn->Send(&sessionMessage, sizeof(sessionMessage));

    conn->Send(context.output.ToStringPiece());
  } else {
    const Context& context = boost::any_cast<Context>(conn->GetContext());
    LOG_INFO << "payload bytes " << context.bytes;
    conn->GetLoop()->Quit();
  }
}

void OnMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time) {
  Context* context = boost::any_cast<Context>(conn->getMutableContext());
  while (buf->GetReadableLength() >= sizeof(int32_t)) {
    int32_t length = buf->ReadInt32();
    if (length == context->session.length) {
      if (context->count < context->session.number) {
        conn->Send(context->output.ToStringPiece());
        ++context->count;
        context->bytes += length;
      } else {
        conn->Shutdown();
        break;
      }
    } else {
      conn->Shutdown();
      break;
    }
  }
}

}  // namespace trans

void transmit(const Options& opt) {
  InetAddress addr(opt.port);
  if (!InetAddress::Resolve(opt.host, &addr)) {
    LOG_FATAL << "Unable to Resolve " << opt.host;
  }
  fun::Timestamp start(fun::Timestamp::Now());
  EventLoop loop;
  g_loop = &loop;
  TcpClient client(&loop, addr, "TtcpClient");
  client.SetConnectionCallback(boost::bind(&trans::OnConnection, opt, _1));
  client.SetMessageCallback(boost::bind(&trans::OnMessage, _1, _2, _3));
  client.Connect();
  loop.Loop();
  double elapsed = TimeDifference(fun::Timestamp::Now(), start);
  double total_mb = 1.0 * opt.length * opt.number / 1024 / 1024;
  printf("%.3f MiB transferred\n%.3f MiB/s\n", total_mb, total_mb / elapsed);
}

/////////////////////////////////////////////////////////////////////
// R E C E I V E
/////////////////////////////////////////////////////////////////////

namespace receiving {

void OnConnection(const TcpConnectionPtr& conn) {
  if (conn->IsConnected()) {
    Context context;
    conn->SetContext(context);
  } else {
    const Context& context = boost::any_cast<Context>(conn->GetContext());
    LOG_INFO << "payload bytes " << context.bytes;
    conn->GetLoop()->Quit();
  }
}

void OnMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time) {
  while (buf->GetReadableLength() >= sizeof(int32_t)) {
    Context* context = boost::any_cast<Context>(conn->getMutableContext());
    SessionMessage& session = context->session;
    if (session.number == 0 && session.length == 0) {
      if (buf->GetReadableLength() >= sizeof(SessionMessage)) {
        session.number = buf->ReadInt32();
        session.length = buf->ReadInt32();
        context->output.AppendInt32(session.length);
        printf("receive number = %d\nreceive length = %d\n", session.number,
               session.length);
      } else {
        break;
      }
    } else {
      const unsigned total_len =
          session.length + static_cast<int>(sizeof(int32_t));
      const int32_t length = buf->peekInt32();
      if (length == session.length) {
        if (buf->GetReadableLength() >= total_len) {
          buf->Drain(total_len);
          conn->Send(context->output.ToStringPiece());
          ++context->count;
          context->bytes += length;
          if (context->count >= session.number) {
            conn->Shutdown();
            break;
          }
        } else {
          break;
        }
      } else {
        printf("wrong length %d\n", length);
        conn->Shutdown();
        break;
      }
    }
  }
}

}  // namespace receiving

void receive(const Options& opt) {
  EventLoop loop;
  g_loop = &loop;
  InetAddress listen_addr(opt.port);
  TcpServer server(&loop, listen_addr, "TtcpReceive");
  server.SetConnectionCallback(boost::bind(&receiving::OnConnection, _1));
  server.SetMessageCallback(boost::bind(&receiving::OnMessage, _1, _2, _3));
  server.Start();
  loop.Loop();
}
