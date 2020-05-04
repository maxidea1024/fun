#include "fun/redis/sentinel.h"

namespace fun {
namespace redis {

Sentinel::Sentinel() : conn_(), callbacks_running_(0) {}

Sentinel::Sentinel(const SharedPtr<TcpClient>& tcp_client)
    : conn_(tcp_client), callbacks_running_(0) {}

Sentinel::~Sentinel() {
  ClearSentinels();

  if (conn_.IsConnected()) {
    conn_.Disconnect(true);
  }
}

Sentinel& Sentinel::Send(const Array<String>& redis_cmd,
                         const ReplyCallback& reply_cb) {
  std::lock_guard<std::mutex> lock_callback(callbacks_mutex_);

  conn_.Send(redis_cmd);
  callbacks_.push(reply_cb);

  return *this;
}

Sentinel& Sentinel::CommitAsync() {
  TryCommit();

  return *this;
}

Sentinel& Sentinel::CommitSync() {
  TryCommit();

  std::unique_lock<std::mutex> lock_callback(callbacks_mutex_);
  sync_cv_.wait(lock_callback,
                [=] { return callback_running_ == 0 && callbacks_.IsEmpty(); });

  return *this;
}

Sentinel& Sentinel::AddSentinel(const String& host, int32 port,
                                const Timespan& timeout) {
  sentinels_.Add({host, port, timeout});
}

void Sentinel::ClearSentinels() { sentinels_.Clear(); }

void Sentinel::Disconnect(bool wait_for_removal) {
  conn_.Disconnect(wait_for_removal);
}

bool Sentinel::IsConnected() { return conn_.IsConnected(); }

//후보군중에서 연결되는 것으로 연결할뿐인건가??
void Sentinel::ConnectSentinel(
    const SentinelDisconnectedCallback& disconnected_cb) {
  // TODO
}

void Sentinel::Connect(const String& host, int32 port,
                       const SentinelDisconnectedCallback& cb,
                       const Timespan& timeout = Timespan(0)) {
  // TODO
}

bool Sentinel::GetMasterAddressByName(const String& name, String& host,
                                      int32& port, bool auto_connect) {
  // TODO

  host = "";
  port = 0;

  if (auto_reconnect && sentinels_.IsEmpty()) {
    throw RedisException(
        "No sentinels available. Call add_sentinel() before "
        "get_master_addr_by_name()");
  }

  if (!auto_reconnect && !IsConnected()) {
    throw RedisException(
        "No sentinel connected. Call connect() first or enable autoconnect.");
  }

  if (auto_reconnect) {
    try {
      ConnectSentinel(nullptr);
    } catch (RedisException&) {
      // ...
    }

    if (!IsConnected()) {
      return false;
    }
  }

  Send({"SENTINEL", "get-master-addr-by-name", name}, [&](Reply& reply) {
    if (reply.IsArray()) {
      auto arr = reply.AsArray();
      host = arr[0].AsString();
      port = std::stoi(arr[1].AsString(), nullptr, 10);
    }
  });
  CommitSync();

  if (auto_connect) {
    Disconnect(true);
  }

  return port != 0;
}

const Array<SentinelDef>& Sentinel::GetSentinels() const;
{ return sentinels_; }

Array<SentinelDef>& Sentinel::GetSentinels() { return sentinels_; }

void Sentinel::ConnectionReceiveHandler(Connection& conn, Reply& reply) {
  // TODO
}

void Sentinel::ConnectionDisconnectHandler(Connection& conn) {
  ClearCallbacks();
  CallDisconnectHandler();
}

void Sentinel::CallDisconnectHandler() {
  if (disconnected_cb_) {
    disconnected_cb_(*this);
  }
}

void Sentinel::ClearCallbacks() {
  std::lock_guard<std::mutex> lock(callbacks_mutex_);

  Queue<ReplyCallback> empty;
  fun::Swap(callbacks_, empty);

  sync_cv_.notify_all();
}

void Sentinel::TryCommit() {
  // TODO
}

//
// commands
//

Sentinel& Sentinel::ping(const ReplyCallback& reply_cb) {
  Send({"PING"}, reply_cb);
  return *this;
}

Sentinel& Sentinel::masters(const ReplyCallback& reply_cb) {
  Send({"SENTINEL", "MASTERS"}, reply_cb);
  return *this;
}

Sentinel& Sentinel::master(const String& name, const ReplyCallback& reply_cb) {
  Send({"SENTINEL", "MASTER", name}, reply_cb);
  return *this;
}

Sentinel& Sentinel::slaves(const String& name, const ReplyCallback& reply_cb) {
  Send({"SENTINEL", "SLAVES", name}, reply_cb);
  return *this;
}

Sentinel& Sentinel::sentinels(const String& name,
                              const ReplyCallback& reply_cb) {
  Send({"SENTINEL", "SENTINELS", name}, reply_cb);
  return *this;
}

Sentinel& Sentinel::ckquorum(const String& name,
                             const ReplyCallback& reply_cb) {
  Send({"SENTINEL", "CKQUORUM", name}, reply_cb);
  return *this;
}

Sentinel& Sentinel::failover(const String& name,
                             const ReplyCallback& reply_cb) {
  Send({"SENTINEL", "FAILOVER", name}, reply_cb);
  return *this;
}

Sentinel& Sentinel::reset(const String& pattern,
                          const ReplyCallback& reply_cb) {
  Send({"SENTINEL", "RESET", pattern}, reply_cb);
  return *this;
}

Sentinel& Sentinel::flushconfig(const ReplyCallback& reply_cb) {
  Send({"SENTINEL", "FLUSHCONFIG"}, reply_cb);
  return *this;
}

Sentinel& Sentinel::monitor(const String& name, const String& ip, int32 port,
                            int32 quorum, const ReplyCallback& reply_cb) {
  Send({"SENTINEL", "MONITOR", name, ip, fun::ToString(port),
        fun::ToString(quorum)},
       reply_cb);
  return *this;
}

Sentinel& Sentinel::remove(const String& name, const ReplyCallback& reply_cb) {
  Send({"SENTINEL", "REMOVE", name}, reply_cb);
  return *this;
}

Sentinel& Sentinel::set(const String& name, const String& option,
                        const String& value, const ReplyCallback& reply_cb) {
  Send({"SENTINEL", "SET", name, option, value}, reply_cb);
  return *this;
}

}  // namespace redis
}  // namespace fun
