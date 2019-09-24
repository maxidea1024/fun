
static int32 g_thread_count = 0;


class DiscardServer
{
 public:
  DiscardServer(EventLoop* loop, const InetAddress& listen_addr)
    : server_(loop, listen_addr, "DiscardServer")
    , started_at_(Timestamp::Now())
  {
    server_.SetConnectionCallback([this](const ConnectionPtr& conn) { OnConnection(conn); });
    server_.SetMessageCallback([this](const ConnectionPtr& conn, Buffer* buf, const Timestamp& received_time) { OnMessage(conn, buf, received_time); });

    server_.SetThreadCount(g_thread_count);

    loop->ScheduleEvery(3.0, [this]() { PrintThroughput(); });
  }

  void Start()
  {
    LOG_INFO << "starting " << thread_count << " threads.";

    server_.Start();
  }

 private:
  void OnConnection(const ConnectionPtr& conn)
  {
    LOG_TRACE << conn->GetPeerAddress().ToIpPort() << " -> "
              << conn->GetLocalAddress().ToIpPort() << " is "
              << (conn->IsConnected() ? "UP" : "DOWN");
  }

  void OnMessage( const ConnectionPtr& conn,
                  Buffer* buf,
                  const Timestamp& received_time)
  {
    size_t len = buf->GetReadableLength();
    transferred_.Add(len);
    received_messages_.IncrementAndGet();
    buf->DrainAll();
  }

  void PrintThroughput()
  {
    Timestamp end_time = Timestamp::Now();
    int64 new_counter = transferred_.Get();
    int64 byte_count = new_counter - old_counter_;
    int64 msg_count = received_messages_.GetAndSet(0);
    double time = TimeDifference(end_time, started_at_);

    printf("%4.3f MiB/s %4.3f Ki Msgs/s %6.2f bytes per msg\n",
        static_cast<double>(byte_count)/time/1024/1024,
        static_cast<double>(msg_count)/time/1024,
        static_cast<double>(byte_count)/static_cast<double>(msg_count));

    old_counter_ = new_counter;
    started_at_ = end_time;
  }

  TcpServer server_;
  AtomicInt64 transferred_;
  AtomicInt64 received_messages_;
  int64 old_counter_;
  Timestamp started_at_;
};


int main(int argc, char* argv[])
{
  LOG_INFO << "pid = " << Process::CurrentPid() << ", tid = " << Thread::CurrentTid();

  if (argc > 1)
  {
    g_thread_count = atoi(argv[1]);
  }

  EventLoop loop;
  InetAddress listen_addr(2009);
  DiscardServer server(&loop, listen_addr);
  server.Start();
  loop.Loop();
}
