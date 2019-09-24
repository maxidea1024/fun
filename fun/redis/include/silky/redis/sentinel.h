#pragma once

namespace fun {
namespace redis {

class FUN_REDIS_API Sentinel {
 public:
  Sentinel();
  Sentinel(const SharedPtr<TcpClient>& tcp_client);
  ~Sentinel();

  Sentinel(const Sentinel&) = delete;
  Sentinel& operator = (const Sentinel&) = delete;

 public:
  using ReplyCallback = Function<void (Reply&)>;

  Sentinel& Send(const Array<String>& redis_cmd, const ReplyCallback reply_cb = nullptr);
  Sentinel& CommitAsync();
  Sentinel& CommitSync();

  //TODO
  /*
  template <class Rep, class Period>
  Sentinel& CommitSync(const std::chrono::duration<Rep,Period>& timeout)
  {
    TryCommit();

    std::unique_lock<std::mutex> lock_callback(callbacks_mutex_);
    if (!sync_cv_.wait_for(lock_callback, timeout, [=] {
        return callbacks_running_ == 0 && callbacks_.IsEmpty();
      }))
    {
      // done...
    }
    else
    {
      // timeout
      // 예외를 던지거나 로깅을 해야할건데??
    }

    return *this;
  }
  */

 public:
  Sentinel& AddSentinel(const String& host, int32 port, const Timespan& timeout);
  void ClearSentinels();

 public:
  void Disconnect(bool wait_for_removal = false);

  bool IsConnected();

  using SentinelDisconnectedCallback = Function<void (Sentinel&)>;

  void ConnectSentinel(const SentinelDisconnectedCallback& cb = nullptr);

  void Connect(const String& host, int32 port, const SentinelDisconnectedCallback& cb = nullptr, const Timespan& timeout = Timespan(0));

  bool GetMasterAddressByName(const String& name, String& host, int32& port, bool auto_connect = true);

 public:
  Sentinel& ckquorum(const String& name, const ReplyCallback& reply_cb = nullptr);
  Sentinel& failover(const String& name, const ReplyCallback& reply_cb = nullptr);
  Sentinel& flushconfig(const ReplyCallback& reply_cb = nullptr);
  Sentinel& master(const String& name, const ReplyCallback& reply_cb = nullptr);
  Sentinel& masters(const ReplyCallback& reply_cb = nullptr);
  Sentinel& monitor(const String& name, const String& ip, int32 port, int32 quorum, const ReplyCallback& reply_cb = nullptr);
  Sentinel& ping(const ReplyCallback& reply_cb = nullptr);
  Sentinel& remove(const String& name, const ReplyCallback& reply_cb = nullptr);
  Sentinel& reset(const String& pattern, const ReplyCallback& reply_cb = nullptr);
  Sentinel& sentinels(const String& name, const ReplyCallback& reply_cb = nullptr);
  Sentinel& set(const String& name, const String& option, const String& value, const ReplyCallback& reply_cb = nullptr);
  Sentinel& slaves(const String& name, const ReplyCallback& reply_cb = nullptr);

  class SentinelDef {
   public:
    SentinelDef(const String& host, int32 port, const Timespan& timeout)
      : host_(host), port_(port), timeout_(timeout) {}

    ~SentinelDef() = default;

    const String& GetHost() const { return host_; }
    int32 GetPort() const { return port_; }
    const Timespan& GetTimeout() const { return timeout_; }
    void SetTimeout(const Timespan& timeout) { timeout_ = timeout; }

   private:
    String host_;
    int32 port_;
    Timespan timeout_;
  };

  const Array<SentinelDef>& GetSentinels() const;
  Array<SentinelDef>& GetSentinels();

 private:
  void ConnectionReceiveHandler(Connection& conn, Reply& reply);
  void ConnectionDisconnectHandler(Connection& conn);
  void CallDisconnectHandler();
  void ClearCallbacks();
  void TryCommit();

 private:
  Array<SentinelDef> sentinels_;
  Connection conn_;
  Queue<ReplyCallback> callbacks_;
  SentinelDisconnectHandler disconnect_cb_;
  std::mutex callabacks_mutex_;
  std::condition_variable sync_cv;
  std::atomic<unsigned int> callbacks_running_;
};

} // namespace redis
} // namespace fun
