#include <red/base/ThreadLocal.h>
#include <red/net/EventLoopThreadPool.h>
#include <red/net/protorpc/RpcCodec.h>
#include <red/net/protorpc/rpc.pb.h>
#include "fun/base/logging.h"
#include "fun/net/event_loop.h"
#include "fun/net/tcp_client.h"
#include "fun/net/tcp_server.h"

#include <boost/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <stdio.h>
#include <unistd.h>

using namespace fun;
using namespace fun::net;

class BackendSession : Noncopyable {
 public:
  BackendSession(EventLoop* loop, const InetAddress& backend_addr,
                 const String& name)
      : loop_(loop),
        client_(loop, backend_addr, name),
        codec_(boost::bind(&BackendSession::OnRpcMessage, this, _1, _2, _3)),
        next_id_(0) {
    client_.SetConnectionCallback(
        boost::bind(&BackendSession::OnConnection, this, _1));
    client_.SetMessageCallback(
        boost::bind(&RpcCodec::OnMessage, &codec_, _1, _2, _3));
    client_.EnableRetry();
  }

  void Connect() { client_.Connect(); }

  // FIXME: add health check
  bool Send(RpcMessage& msg, const TcpConnectionPtr& client_conn) {
    loop_->AssertInLoopThread();
    if (conn_) {
      uint64_t id = ++next_id_;
      Request r = {msg.id(), client_conn};
      fun_check(outstandings_.find(id) == outstandings_.end());
      outstandings_[id] = r;
      msg.set_id(id);
      codec_.Send(conn_, msg);
      // LOG_DEBUG << "forward " << r.origId << " from " <<
      // client_conn->GetName()
      //           << " as " << id << " to " << conn_->GetName();
      return true;
    } else
      return false;
  }

 private:
  void OnConnection(const TcpConnectionPtr& conn) {
    loop_->AssertInLoopThread();
    LOG_INFO << "Backend " << conn->GetLocalAddress().ToIpPort() << " -> "
             << conn->GetPeerAddress().ToIpPort() << " is "
             << (conn->IsConnected() ? "UP" : "DOWN");
    if (conn->IsConnected()) {
      conn_ = conn;
    } else {
      conn_.Reset();
      // FIXME: reject pending
    }
  }

  void OnRpcMessage(const TcpConnectionPtr&, const RpcMessagePtr& msg,
                    const Timestamp&) {
    loop_->AssertInLoopThread();
    std::map<uint64_t, Request>::iterator it = outstandings_.find(msg->id());
    if (it != outstandings_.end()) {
      uint64_t origId = it->second.origId;
      TcpConnectionPtr client_conn = it->second.client_conn.lock();
      outstandings_.erase(it);

      if (client_conn) {
        // LOG_DEBUG << "Send back " << origId << " of " <<
        // client_conn->GetName()
        //           << " using " << msg.id() << " from " << conn_->GetName();
        msg->set_id(origId);
        codec_.Send(client_conn, *msg);
      }
    } else {
      // LOG_ERROR
    }
  }

  struct Request {
    uint64_t origId;
    WeakPtr<TcpConnection> client_conn;
  };

  EventLoop* loop_;
  TcpClient client_;
  RpcCodec codec_;
  TcpConnectionPtr conn_;
  uint64_t next_id_;
  std::map<uint64_t, Request> outstandings_;
};

class Balancer : Noncopyable {
 public:
  Balancer(EventLoop* loop, const InetAddress& listen_addr, const String& name,
           const std::vector<InetAddress>& backends)
      : loop_(loop),
        server_(loop, listen_addr, name),
        codec_(boost::bind(&Balancer::OnRpcMessage, this, _1, _2, _3)),
        backends_(backends) {
    server_.SetThreadInitCallback(
        boost::bind(&Balancer::InitPerThread, this, _1));
    server_.SetConnectionCallback(
        boost::bind(&Balancer::OnConnection, this, _1));
    server_.SetMessageCallback(
        boost::bind(&RpcCodec::OnMessage, &codec_, _1, _2, _3));
  }

  ~Balancer() {}

  void SetThreadCount(int thread_count) {
    server_.SetThreadCount(thread_count);
  }

  void Start() { server_.Start(); }

 private:
  struct PerThread {
    size_t current;
    boost::ptr_vector<BackendSession> backends;

    PerThread() : current(0) {}
  };

  void InitPerThread(EventLoop* ioLoop) {
    int count = thread_count_.getAndAdd(1);
    LOG_INFO << "IO thread " << count;
    PerThread& t = t_backends_.value();
    t.current = count % backends_.size();

    for (size_t i = 0; i < backends_.size(); ++i) {
      char buf[32];
      snprintf(buf, sizeof buf, "%s#%d", backends_[i].ToIpPort().c_str(),
               count);
      t.backends.push_back(new BackendSession(ioLoop, backends_[i], buf));
      t.backends.back().Connect();
    }
  }

  void OnConnection(const TcpConnectionPtr& conn) {
    LOG_INFO << "Client " << conn->GetPeerAddress().ToIpPort() << " -> "
             << conn->GetLocalAddress().ToIpPort() << " is "
             << (conn->IsConnected() ? "UP" : "DOWN");
    if (!conn->IsConnected()) {
      // FIXME: cancel outstanding calls, otherwise, memory leak
    }
  }

  void OnRpcMessage(const TcpConnectionPtr& conn, const RpcMessagePtr& msg,
                    const Timestamp&) {
    PerThread& t = t_backends_.value();
    bool succeed = false;
    for (size_t i = 0; i < t.backends.size() && !succeed; ++i) {
      succeed = t.backends[t.current].Send(*msg, conn);
      t.current = (t.current + 1) % t.backends.size();
    }
    if (!succeed) {
      // FIXME: no backend available
    }
  }

  EventLoop* loop_;
  TcpServer server_;
  RpcCodec codec_;
  std::vector<InetAddress> backends_;
  AtomicInt32 thread_count_;
  ThreadLocal<PerThread> t_backends_;
};

int main(int argc, char* argv[]) {
  LOG_INFO << "pid = " << Process::CurrentPid();

  if (argc < 3) {
    fprintf(stderr, "Usage: %s listen_port backend_ip:port [backend_ip:port]\n",
            argv[0]);
  } else {
    std::vector<InetAddress> backends;
    for (int i = 2; i < argc; ++i) {
      String hostport = argv[i];
      size_t colon = hostport.find(':');
      if (colon != String::npos) {
        String ip = hostport.substr(0, colon);
        uint16_t port =
            static_cast<uint16_t>(atoi(hostport.c_str() + colon + 1));
        backends.push_back(InetAddress(ip, port));
      } else {
        fprintf(stderr, "invalid backend address %s\n", argv[i]);
        return 1;
      }
    }
    uint16_t port = static_cast<uint16_t>(atoi(argv[1]));
    InetAddress listen_addr(port);

    EventLoop loop;
    Balancer balancer(&loop, listen_addr, "RpcBalancer", backends);
    balancer.SetThreadCount(4);
    balancer.Start();
    loop.Loop();
  }

  google::protobuf::ShutdownProtobufLibrary();
}
