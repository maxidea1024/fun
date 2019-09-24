#include "examples/ace/logging/logrecord.pb.h"

#include "fun/base/mutex.h"
#include "fun/base/logging.h"
#include "fun/base/ProcessInfo.h"
#include "fun/net/event_loop.h"
#include "fun/net/event_loop_thread.h"
#include "fun/net/tcp_client.h"
#include "fun/net/protobuf/protobuf_codec_lite.h"

#include <boost/bind.hpp>

#include <iostream>
#include <stdio.h>

using namespace fun;
using namespace fun::net;

// just to verify the protocol, not for practical usage.

namespace logging {

extern const char logtag[] = "LOG0";
typedef ProtobufCodecLiteT<LogRecord, logtag> Codec;

// same as asio/char/client.cc
class LogClient : Noncopyable
{
 public:
  LogClient(EventLoop* loop, const InetAddress& server_addr)
    : client_(loop, server_addr, "LogClient"),
      codec_(boost::bind(&LogClient::OnMessage, this, _1, _2, _3))
  {
    client_.SetConnectionCallback(boost::bind(&LogClient::OnConnection, this, _1));
    client_.SetMessageCallback(boost::bind(&Codec::OnMessage, &codec_, _1, _2, _3));
    client_.EnableRetry();
  }

  void Connect()
  {
    client_.Connect();
  }

  void Disconnect()
  {
    client_.Disconnect();
  }

  void write(const StringPiece& message)
  {
    ScopedLock guard(mutex_);
    UpdateLogRecord(message);
    if (connection_) {
      codec_.Send(connection_, log_record_);
    }
    else {
      LOG_WARN << "NOT CONNECTED";
    }
  }

 private:
  void OnConnection(const TcpConnectionPtr& conn)
  {
    LOG_INFO << conn->GetLocalAddress().ToIpPort() << " -> "
             << conn->GetPeerAddress().ToIpPort() << " is "
             << (conn->IsConnected() ? "UP" : "DOWN");

    ScopedLock guard(mutex_);
    if (conn->IsConnected()) {
      connection_ = conn;
      LogRecord_Heartbeat* hb = log_record_.mutable_heartbeat();
      hb->set_hostname(ProcessInfo::hostname().c_str());
      hb->set_process_name(ProcessInfo::procname().c_str());
      hb->set_process_id(ProcessInfo::pid());
      hb->set_process_start_time(ProcessInfo::startTime().microSecondsSinceEpoch());
      hb->set_username(ProcessInfo::username().c_str());
      UpdateLogRecord("Heartbeat");
      codec_.Send(connection_, log_record_);
      log_record_.clear_heartbeat();
      LOG_INFO << "Type message below:";
    }
    else {
      connection_.Reset();
    }
  }

  void OnMessage(const TcpConnectionPtr&,
                 const MessagePtr& message,
                 const Timestamp&)
  {
    // SHOULD NOT HAPPEN
    LogRecord* logRecord = fun::down_cast<LogRecord*>(message.get());
    LOG_WARN << logRecord->DebugString();
  }

  void UpdateLogRecord(const StringPiece& message)
  {
    mutex_.assertLocked();
    log_record_.set_level(1);
    log_record_.set_thread_id(Thread::CurrentTid());
    log_record_.set_timestamp(Timestamp::Now().microSecondsSinceEpoch());
    log_record_.set_message(message.data(), message.size());
  }

  TcpClient client_;
  Codec codec_;
  MutexLock mutex_;
  LogRecord log_record_;
  TcpConnectionPtr connection_;
};

} // namespace logging


int main(int argc, char* argv[])
{
  if (argc < 3) {
    printf("usage: %s server_ip server_port\n", argv[0]);
  }
  else {
    EventLoopThread loop_thread;
    uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
    InetAddress server_addr(argv[1], port);

    logging::LogClient client(loop_thread.StartLoop(), server_addr);
    client.Connect();
    String line;
    while (std::getline(std::cin, line)) {
      client.write(line);
    }
    client.Disconnect();
    CurrentThread::sleepUsec(1000*1000);  // wait for Disconnect, then safe to destruct LogClient (esp. TcpClient). Otherwise mutex_ is used after dtor.
  }

  google::protobuf::ShutdownProtobufLibrary();
}
