#include "fun/redis/client.h"

namespace cpp_redis {

Client::Client()
  : reconnecting_(false)
  , cancel_(false)
  , callbacks_running_(0) {
  __CPP_REDIS_LOG(debug, "cpp_redis::Client created");
}

Client::Client(const SharedPtr<network::tcp_client_iface>& tcp_client)
  : conn_(tcp_client)
  , sentinel_(tcp_client)
  , reconnecting_(false)
  , cancel_(false)
  , callbacks_running_(0) {
  __CPP_REDIS_LOG(debug, "cpp_redis::Client created");
}

Client::~Client() {
  // ensure we stopped reconnection attempts
  if (!cancel_) {
    CancelReconnect();
  }

  // If for some reason Sentinel is connected then disconnect now.
  if (sentinel_.IsConnected()) {
    sentinel_.disconnect(true);
  }

  // disconnect underlying tcp socket
  if (conn_.IsConnected()) {
    conn_.disconnect(true);
  }

  __CPP_REDIS_LOG(debug, "cpp_redis::Client destroyed");
}

void
Client::connect(
  const String& name,
  const ConnectCallback& connect_callback,
  uint32 timeout_msecs,
  int32 max_reconnects,
  uint32 reconnect_interval_msecs) {
  // Save for auto reconnects
  master_name_ = name;

  // We rely on the Sentinel to tell us which redis server is currently the master.
  if (sentinel_.get_master_addr_by_name(name, redis_server_, redis_port_, true)) {
    connect(redis_server_, redis_port_, connect_callback, timeout_msecs, max_reconnects, reconnect_interval_msecs);
  }
  else {
    throw redis_error("cpp_redis::Client::connect() could not find master for name " + name);
  }
}

void
Client::Connect(
  const String& host, int32 port,
  const ConnectCallback& connect_callback,
  uint32 timeout_msecs,
  int32 max_reconnects,
  uint32 reconnect_interval_msecs) {
  __CPP_REDIS_LOG(debug, "cpp_redis::Client attempts to connect");

  // Save for auto reconnects
  redis_server_             = host;
  redis_port_               = port;
  connect_callback_         = connect_callback;
  max_reconnects_           = max_reconnects;
  reconnect_interval_msecs_ = reconnect_interval_msecs;

  // notify start
  if (connect_callback_) {
    connect_callback_(host, port, ConnectState::start);
  }

  auto disconnection_handler = tl::bind(&Client::OnConnectionDisconnected, this, tl::placeholders::_1);
  auto receive_handler       = tl::bind(&Client::OnConnectionReceive, this, tl::placeholders::_1, tl::placeholders::_2);
  conn_.connect(host, port, disconnection_handler, receive_handler, timeout_msecs);

  __CPP_REDIS_LOG(info, "cpp_redis::Client connected");

  // notify end
  if (connect_callback_) {
    connect_callback_(redis_server_, redis_port_, ConnectState::ok);
  }
}

void Client::Disconnect(bool wait_for_removal) {
  __CPP_REDIS_LOG(debug, "cpp_redis::Client attempts to disconnect");

  // close connection
  conn_.disconnect(wait_for_removal);

  // make sure we clear buffer of unsent commands
  ClearCallbacks();

  __CPP_REDIS_LOG(info, "cpp_redis::Client disconnected");
}

bool Client::IsConnected() const {
  return conn_.IsConnected();
}

void Client::CancelReconnect() {
  cancel_ = true;
}

bool Client::IsReconnecting() const {
  return reconnecting_;
}

void Client::AddSentinel(const String& host, int32 port, uint32 timeout_msecs) {
  sentinel_.AddSentinel(host, port, timeout_msecs);
}

const Sentinel& Client::GetSentinel() const {
  return sentinel_;
}

Sentinel& Client::GetSentinel() {
  return sentinel_;
}

void Client::ClearSentinels() {
  sentinel_.ClearSentinels();
}

Client& Client::Send(const Array<String>& redis_cmd, const ReplyCallback& callback) {
  ScopedLock<FastMutex> lock_callback(callbacks_mutex_);

  __CPP_REDIS_LOG(info, "cpp_redis::Client attempts to store new command in the Send buffer");
  UnprotectedSend(redis_cmd, callback);
  __CPP_REDIS_LOG(info, "cpp_redis::Client stored new command in the Send buffer");

  return *this;
}

void Client::UnprotectedSend(const Array<String>& redis_cmd, const ReplyCallback& callback) {
  conn_.Send(redis_cmd);
  commands_.push({redis_cmd, callback});
}

// Commit pipelined transaction
Client& Client::Commit() {
  // no need to call Commit in case of reconnection
  // the reconnection flow will do it for us
  if (!IsReconnecting()) {
    TryCommit();
  }

  return *this;
}

Client& Client::SyncCommit() {
  // no need to call Commit in case of reconnection
  // the reconnection flow will do it for us
  if (!IsReconnecting()) {
    TryCommit();
  }

  tl::unique_lock<FastMutex> lock_callback(callbacks_mutex_);
  __CPP_REDIS_LOG(debug, "cpp_redis::Client waiting for callbacks to complete");
  sync_cv_.wait(lock_callback, [=] { return callbacks_running_ == 0 && commands_.IsEmpty(); });
  __CPP_REDIS_LOG(debug, "cpp_redis::Client finished waiting for callback completion");
  return *this;
}


void Client::TryCommit() {
  try {
    __CPP_REDIS_LOG(debug, "cpp_redis::Client attempts to Send pipelined commands");
    conn_.Commit();
    __CPP_REDIS_LOG(info, "cpp_redis::Client sent pipelined commands");
  }
  catch (const cpp_redis::redis_error&) {
    __CPP_REDIS_LOG(error, "cpp_redis::Client could not Send pipelined commands");
    // ensure commands are flushed
    ClearCallbacks();
    throw;
  }
}


void Client::OnConnectionReceive(Connection&, Reply& reply) {
  ReplyCallback callback = nullptr;

  __CPP_REDIS_LOG(info, "cpp_redis::Client received reply"); {
    ScopedLock<FastMutex> lock(callbacks_mutex_);
    callbacks_running_ += 1;

    if (commands_.Count()) {
      callback = commands_.front().callback;
      commands_.pop();
    }
  }

  if (callback) {
    __CPP_REDIS_LOG(debug, "cpp_redis::Client executes reply callback");
    callback(reply);
  }
 {
    ScopedLock<FastMutex> lock(callbacks_mutex_);
    callbacks_running_ -= 1;
    sync_cv_.notify_all();
  }
}


void Client::ClearCallbacks() {
  if (commands_.IsEmpty()) {
    return;
  }

  // dequeue commands and move them to a local variable
  tl::queue<CommandRequest> commands = tl::move(commands_);

  callbacks_running_ += __CPP_REDIS_LENGTH(commands.Count());

  tl::thread t([=]() mutable {
    while (!commands.IsEmpty()) {
      const auto& callback = commands.front().callback;

      if (callback)
      {
        reply r = {"network failure", reply::string_type::error};
        callback(r);
      }

      --callbacks_running_;
      commands.pop();
    }

    sync_cv_.notify_all();
  });
  t.detach();
}


void Client::ResendUnsentCommands() {
  if (commands_.IsEmpty()) {
    return;
  }

  // dequeue commands and move them to a local variable
  tl::queue<CommandRequest> commands = tl::move(commands_);

  while (commands.Count() > 0) {
    // Reissue the pending command and its callback.
    UnprotectedSend(commands.front().command, commands.front().callback);

    commands.pop();
  }
}


// timer를 통해서 처리해야하는데, 현재는 sleep으로 인해서 블럭킹이 발생함...
void Client::OnConnectionDisconnected(Connection&) {
  // leave right now if we are already dealing with reconnection
  if (IsReconnecting()) {
    return;
  }

  // initiate reconnection process
  reconnecting_ = true;
  current_reconnect_attempts_ = 0;

  __CPP_REDIS_LOG(warn, "cpp_redis::Client has been disconnected");

  if (connect_callback_) {
    connect_callback_(redis_server_, redis_port_, ConnectState::dropped);
  }

  // Lock the callbacks mutex of the base class to prevent more Client commands from being issued until our Reconnect has completed.
  ScopedLock<FastMutex> lock_callback(callbacks_mutex_);

  while (SouldReconnect()) {
    SleepBeforeNextReconnectAttempt();
    Reconnect();
  }

  if (!IsConnected()) {
    ClearCallbacks();

    // Tell the user we gave up!
    if (connect_callback_) {
      connect_callback_(redis_server_, redis_port_, ConnectState::stopped);
    }
  }

  // terminate reconnection
  reconnecting_ = false;
}


void Client::SleepBeforeNextReconnectAttempt() {
  if (reconnect_interval_msecs_ <= 0) {
    return;
  }

  if (connect_callback_) {
    connect_callback_(redis_server_, redis_port_, ConnectState::sleeping);
  }

  tl::this_thread::sleep_for(tl::chrono::milliseconds(reconnect_interval_msecs_));
}


bool Client::SouldReconnect() const {
  return !IsConnected() && !cancel_ && (max_reconnects_ == -1 || current_reconnect_attempts_ < max_reconnects_);
}


void Client::ReAuth() {
  if (password_.IsEmpty()) {
    return;
  }

  UnprotectedAuth(password_, [&](cpp_redis::Reply& reply) {
    if (reply.is_string() && reply.as_string() == "OK") {
      __CPP_REDIS_LOG(warn, "Client successfully re-authenticated");
    }
    else {
      __CPP_REDIS_LOG(warn, String("Client failed to re-authenticate: " + reply.as_string()).c_str());
    }
  });
}


void Client::ReSelect() {
  if (database_index_ <= 0) {
    return;
  }

  UnprotectedSelect(database_index_, [&](cpp_redis::Reply& reply) {
    if (reply.is_string() && reply.as_string() == "OK") {
      __CPP_REDIS_LOG(warn, "Client successfully re-selected redis database");
    }
    else {
      __CPP_REDIS_LOG(warn, String("Client failed to re-select database: " + reply.as_string()).c_str());
    }
  });
}


void Client::Reconnect() {
  // increase the number of attempts to Reconnect
  ++current_reconnect_attempts_;


  // We rely on the Sentinel to tell us which redis server is currently the master.
  if (!master_name_.IsEmpty() && !sentinel_.get_master_addr_by_name(master_name_, redis_server_, redis_port_, true)) {
    if (connect_callback_) {
      connect_callback_(redis_server_, redis_port_, ConnectState::lookup_failed);
    }
    return;
  }

  // Try catch block because the redis Client throws an error if connection cannot be made.
  try {
    connect(redis_server_, redis_port_, connect_callback_, connect_timeout_msecs_, max_reconnects_, reconnect_interval_msecs_);
  }
  catch (...) {
  }

  if (!IsConnected()) {
    if (connect_callback_) {
      connect_callback_(redis_server_, redis_port_, ConnectState::failed);
    }
    return;
  }

  // notify end
  if (connect_callback_) {
    connect_callback_(redis_server_, redis_port_, ConnectState::ok);
  }

  __CPP_REDIS_LOG(info, "Client reconnected ok");

  ReAuth();
  ReSelect();
  ResendUnsentCommands();
  TryCommit();
}

//fun::ToString 으로 대체 가능할듯...
String Client::aggregate_method_to_string(AggregateMethod method) const {
  switch (method) {
    case AggregateMethod::sum: return "SUM";
    case AggregateMethod::min: return "MIN";
    case AggregateMethod::max: return "MAX";
    default: return "";
  }
}

//fun::ToString 으로 대체 가능할듯...
String Client::geo_unit_to_string(GeoUnit unit) const {
  switch (unit) {
    case GeoUnit::m: return "m";
    case GeoUnit::km: return "km";
    case GeoUnit::ft: return "ft";
    case GeoUnit::mi: return "mi";
    default: return "";
  }
}

//fun::ToString 으로 대체 가능할듯...
String Client::bitfield_operation_type_to_string(BitfieldOperationType operation) const {
  switch (operation) {
    case BitfieldOperationType::get: return "GET";
    case BitfieldOperationType::set: return "SET";
    case BitfieldOperationType::incrby: return "INCRBY";
    default: return "";
  }
}

//fun::ToString 으로 대체 가능할듯...
String Client::overflow_type_to_string(OverflowType type) const {
  switch (type) {
    case OverflowType::wrap: return "WRAP";
    case OverflowType::sat: return "SAT";
    case OverflowType::fail: return "FAIL";
    default: return "";
  }
}

Client::BitfieldOperation
Client::BitfieldOperation::get(const String& type, int offset, OverflowType overflow) {
  return {BitfieldOperationType::get, type, offset, 0, overflow};
}

Client::BitfieldOperation
Client::BitfieldOperation::set(const String& type, int offset, int value, OverflowType overflow) {
  return {BitfieldOperationType::set, type, offset, value, overflow};
}

Client::BitfieldOperation
Client::BitfieldOperation::incrby(const String& type, int offset, int increment, OverflowType overflow) {
  return {BitfieldOperationType::incrby, type, offset, increment, overflow};
}





//
// Redis commands
// Callback-based
//

Client& Client::append(const String& key, const String& value, const ReplyCallback& reply_cb) {
  Send({"APPEND", key, value}, reply_cb);
  return *this;
}

Client& Client::auth(const String& password, const ReplyCallback& reply_cb) {
  ScopedLock<FastMutex> lock(callbacks_mutex_);

  UnprotectedAuth(password, reply_cb);

  return *this;
}

void Client::UnprotectedAuth(const String& password, const ReplyCallback& reply_cb) {
  // save the password for Reconnect attempts.
  password_ = password;
  // store command in pipeline
  UnprotectedSend({"AUTH", password}, reply_cb);
}

Client& Client::bgrewriteaof(const ReplyCallback& reply_cb) {
  Send({"BGREWRITEAOF"}, reply_cb);
  return *this;
}

Client& Client::bgsave(const ReplyCallback& reply_cb) {
  Send({"BGSAVE"}, reply_cb);
  return *this;
}

Client& Client::bitcount(const String& key, const ReplyCallback& reply_cb) {
  Send({"BITCOUNT", key}, reply_cb);
  return *this;
}

Client& Client::bitcount(const String& key, int start, int end, const ReplyCallback& reply_cb) {
  Send({"BITCOUNT", key, fun::ToString(start), fun::ToString(end)}, reply_cb);
  return *this;
}

Client& Client::bitfield(const String& key, const Array<BitfieldOperation>& operations, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"BITFIELD", key};

  for (const auto& operation : operations) {
    cmd << bitfield_operation_type_to_string(operation.operation_type);
    cmd << operation.type;
    cmd << fun::ToString(operation.offset);

    if (operation.operation_type == BitfieldOperationType::set || operation.operation_type == BitfieldOperationType::incrby) {
      cmd << fun::ToString(operation.value);
    }

    if (operation.overflow != OverflowType::server_default) {
      cmd << "OVERFLOW";
      cmd << overflow_type_to_string(operation.overflow);
    }
  }

  Send(cmd, reply_cb);
  return *this;
}

Client& Client::bitop(const String& operation, const String& destkey, const Array<String>& keys, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"BITOP", operation, destkey};
  cmd << keys;
  Send(cmd, reply_cb);
  return *this;
}

Client& Client::bitpos(const String& key, int bit, const ReplyCallback& reply_cb) {
  Send({"BITPOS", key, fun::ToString(bit)}, reply_cb);
  return *this;
}

Client& Client::bitpos(const String& key, int bit, int start, const ReplyCallback& reply_cb) {
  Send({"BITPOS", key, fun::ToString(bit), fun::ToString(start)}, reply_cb);
  return *this;
}

Client& Client::bitpos(const String& key, int bit, int start, int end, const ReplyCallback& reply_cb) {
  Send({"BITPOS", key, fun::ToString(bit), fun::ToString(start), fun::ToString(end)}, reply_cb);
  return *this;
}

Client& Client::blpop(const Array<String>& keys, int timeout, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"BLPOP"};
  cmd << keys;
  cmd << fun::ToString(timeout);
  Send(cmd, reply_cb);
  return *this;
}

Client& Client::brpop(const Array<String>& keys, int timeout, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"BRPOP"};
  cmd << keys;
  cmd << fun::ToString(timeout);
  Send(cmd, reply_cb);
  return *this;
}

Client& Client::brpoplpush(const String& src, const String& dst, int timeout, const ReplyCallback& reply_cb) {
  Send({"BRPOPLPUSH", src, dst, fun::ToString(timeout)}, reply_cb);
  return *this;
}

Client& Client::client_list(const ReplyCallback& reply_cb) {
  Send({"CLIENT", "LIST"}, reply_cb);
  return *this;
}

Client& Client::client_getname(const ReplyCallback& reply_cb) {
  Send({"CLIENT", "GETNAME"}, reply_cb);
  return *this;
}

Client& Client::client_pause(int timeout, const ReplyCallback& reply_cb) {
  Send({"CLIENT", "PAUSE", fun::ToString(timeout)}, reply_cb);
  return *this;
}

Client& Client::client_reply(const String& mode, const ReplyCallback& reply_cb) {
  Send({"CLIENT", "REPLY", mode}, reply_cb);
  return *this;
}

Client& Client::client_setname(const String& name, const ReplyCallback& reply_cb) {
  Send({"CLIENT", "SETNAME", name}, reply_cb);
  return *this;
}

Client& Client::cluster_addslots(const Array<String>& p_slots, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"CLUSTER", "ADDSLOTS"};
  cmd << p_slots;
  Send(cmd, reply_cb);
  return *this;
}

Client& Client::cluster_count_failure_reports(const String& node_id, const ReplyCallback& reply_cb) {
  Send({"CLUSTER", "COUNT-FAILURE-REPORTS", node_id}, reply_cb);
  return *this;
}

Client& Client::cluster_countkeysinslot(const String& slot, const ReplyCallback& reply_cb) {
  Send({"CLUSTER", "COUNTKEYSINSLOT", slot}, reply_cb);
  return *this;
}

Client& Client::cluster_delslots(const Array<String>& p_slots, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"CLUSTER", "DELSLOTS"};
  cmd << p_slots;
  Send(cmd, reply_cb);
  return *this;
}

Client& Client::cluster_failover(const ReplyCallback& reply_cb) {
  Send({"CLUSTER", "FAILOVER"}, reply_cb);
  return *this;
}

Client& Client::cluster_failover(const String& mode, const ReplyCallback& reply_cb) {
  Send({"CLUSTER", "FAILOVER", mode}, reply_cb);
  return *this;
}

Client& Client::cluster_forget(const String& node_id, const ReplyCallback& reply_cb) {
  Send({"CLUSTER", "FORGET", node_id}, reply_cb);
  return *this;
}

Client& Client::cluster_getkeysinslot(const String& slot, int count, const ReplyCallback& reply_cb) {
  Send({"CLUSTER", "GETKEYSINSLOT", slot, fun::ToString(count)}, reply_cb);
  return *this;
}

Client& Client::cluster_info(const ReplyCallback& reply_cb) {
  Send({"CLUSTER", "INFO"}, reply_cb);
  return *this;
}

Client& Client::cluster_keyslot(const String& key, const ReplyCallback& reply_cb) {
  Send({"CLUSTER", "KEYSLOT", key}, reply_cb);
  return *this;
}

Client& Client::cluster_meet(const String& ip, int port, const ReplyCallback& reply_cb) {
  Send({"CLUSTER", "MEET", ip, fun::ToString(port)}, reply_cb);
  return *this;
}

Client& Client::cluster_nodes(const ReplyCallback& reply_cb) {
  Send({"CLUSTER", "NODES"}, reply_cb);
  return *this;
}

Client& Client::cluster_replicate(const String& node_id, const ReplyCallback& reply_cb) {
  Send({"CLUSTER", "REPLICATE", node_id}, reply_cb);
  return *this;
}

Client& Client::cluster_reset(const ReplyCallback& reply_cb) {
  Send({"CLUSTER", "RESET"}, reply_cb);
  return *this;
}

Client& Client::cluster_reset(const String& mode, const ReplyCallback& reply_cb) {
  Send({"CLUSTER", "RESET", mode}, reply_cb);
  return *this;
}

Client& Client::cluster_saveconfig(const ReplyCallback& reply_cb) {
  Send({"CLUSTER", "SAVECONFIG"}, reply_cb);
  return *this;
}

Client& Client::cluster_set_config_epoch(const String& epoch, const ReplyCallback& reply_cb) {
  Send({"CLUSTER", "SET-CONFIG-EPOCH", epoch}, reply_cb);
  return *this;
}

Client& Client::cluster_setslot(const String& slot, const String& mode, const ReplyCallback& reply_cb) {
  Send({"CLUSTER", "SETSLOT", slot, mode}, reply_cb);
  return *this;
}

Client& Client::cluster_setslot(const String& slot, const String& mode, const String& node_id, const ReplyCallback& reply_cb) {
  Send({"CLUSTER", "SETSLOT", slot, mode, node_id}, reply_cb);
  return *this;
}

Client& Client::cluster_slaves(const String& node_id, const ReplyCallback& reply_cb) {
  Send({"CLUSTER", "SLAVES", node_id}, reply_cb);
  return *this;
}

Client& Client::cluster_slots(const ReplyCallback& reply_cb) {
  Send({"CLUSTER", "SLOTS"}, reply_cb);
  return *this;
}

Client& Client::command(const ReplyCallback& reply_cb) {
  Send({"COMMAND"}, reply_cb);
  return *this;
}

Client& Client::command_count(const ReplyCallback& reply_cb) {
  Send({"COMMAND", "COUNT"}, reply_cb);
  return *this;
}

Client& Client::command_getkeys(const ReplyCallback& reply_cb) {
  Send({"COMMAND", "GETKEYS"}, reply_cb);
  return *this;
}

Client& Client::command_info(const Array<String>& command_name, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"COMMAND", "COUNT"};
  cmd << command_name;
  Send(cmd, reply_cb);
  return *this;
}

Client& Client::config_get(const String& param, const ReplyCallback& reply_cb) {
  Send({"CONFIG", "GET", param}, reply_cb);
  return *this;
}

Client& Client::config_rewrite(const ReplyCallback& reply_cb) {
  Send({"CONFIG", "REWRITE"}, reply_cb);
  return *this;
}

Client& Client::config_set(const String& param, const String& val, const ReplyCallback& reply_cb) {
  Send({"CONFIG", "SET", param, val}, reply_cb);
  return *this;
}

Client& Client::config_resetstat(const ReplyCallback& reply_cb) {
  Send({"CONFIG", "RESETSTAT"}, reply_cb);
  return *this;
}

Client& Client::dbsize(const ReplyCallback& reply_cb) {
  Send({"DBSIZE"}, reply_cb);
  return *this;
}

Client& Client::debug_object(const String& key, const ReplyCallback& reply_cb) {
  Send({"DEBUG", "OBJECT", key}, reply_cb);
  return *this;
}

Client& Client::debug_segfault(const ReplyCallback& reply_cb) {
  Send({"DEBUG", "SEGFAULT"}, reply_cb);
  return *this;
}

Client& Client::decr(const String& key, const ReplyCallback& reply_cb) {
  Send({"DECR", key}, reply_cb);
  return *this;
}

Client& Client::decrby(const String& key, int val, const ReplyCallback& reply_cb) {
  Send({"DECRBY", key, fun::ToString(val)}, reply_cb);
  return *this;
}

Client& Client::del(const Array<String>& key, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"DEL"};
  cmd << key;
  Send(cmd, reply_cb);
  return *this;
}

Client& Client::discard(const ReplyCallback& reply_cb) {
  Send({"DISCARD"}, reply_cb);
  return *this;
}

Client& Client::dump(const String& key, const ReplyCallback& reply_cb) {
  Send({"DUMP", key}, reply_cb);
  return *this;
}

Client& Client::echo(const String& msg, const ReplyCallback& reply_cb) {
  Send({"ECHO", msg}, reply_cb);
  return *this;
}

Client& Client::eval(const String& script, int numkeys, const Array<String>& keys, const Array<String>& args, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"EVAL", script, fun::ToString(numkeys)};
  cmd << keys;
  cmd << args;
  Send(cmd, reply_cb);
  return *this;
}

Client& Client::evalsha(const String& sha1, int numkeys, const Array<String>& keys, const Array<String>& args, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"EVALSHA", sha1, fun::ToString(numkeys)};
  cmd << keys;
  cmd << args;
  Send(cmd, reply_cb);
  return *this;
}

Client& Client::exec(const ReplyCallback& reply_cb) {
  Send({"EXEC"}, reply_cb);
  return *this;
}

Client& Client::exists(const Array<String>& keys, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"EXISTS"};
  cmd << keys;
  Send(cmd, reply_cb);
  return *this;
}

Client& Client::expire(const String& key, int seconds, const ReplyCallback& reply_cb) {
  Send({"EXPIRE", key, fun::ToString(seconds)}, reply_cb);
  return *this;
}

Client& Client::expireat(const String& key, int timestamp, const ReplyCallback& reply_cb) {
  Send({"EXPIREAT", key, fun::ToString(timestamp)}, reply_cb);
  return *this;
}

Client& Client::flushall(const ReplyCallback& reply_cb) {
  Send({"FLUSHALL"}, reply_cb);
  return *this;
}

Client& Client::flushdb(const ReplyCallback& reply_cb) {
  Send({"FLUSHDB"}, reply_cb);
  return *this;
}

//TODO Tuple
Client& Client::geoadd(const String& key, const Array<tl::tuple<String, String, String>>& long_lat_memb, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"GEOADD", key};
  for (const auto& obj : long_lat_memb) {
    cmd << tl::get<0>(obj);
    cmd << tl::get<1>(obj);
    cmd << tl::get<2>(obj);
  }
  Send(cmd, reply_cb);
  return *this;
}

Client& Client::geohash(const String& key, const Array<String>& members, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"GEOHASH", key};
  cmd << members;
  Send(cmd, reply_cb);
  return *this;
}

Client& Client::geopos(const String& key, const Array<String>& members, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"GEOPOS", key};
  cmd << members;
  Send(cmd, reply_cb);
  return *this;
}

Client& Client::geodist(const String& key, const String& member_1, const String& member_2, const ReplyCallback& reply_cb) {
  Send({"GEODIST", key, member_1, member_2}, reply_cb);
  return *this;
}

Client& Client::geodist(const String& key, const String& member_1, const String& member_2, const String& unit, const ReplyCallback& reply_cb) {
  Send({"GEODIST", key, member_1, member_2, unit}, reply_cb);
  return *this;
}

Client& Client::georadius(const String& key, double longitude, double latitude, double radius, GeoUnit unit, bool with_coord, bool with_dist, bool with_hash, bool asc_order, const ReplyCallback& reply_cb) {
  return georadius(key, longitude, latitude, radius, unit, with_coord, with_dist, with_hash, asc_order, 0, "", "", reply_cb);
}

Client& Client::georadius(const String& key, double longitude, double latitude, double radius, GeoUnit unit, bool with_coord, bool with_dist, bool with_hash, bool asc_order, int32 count, const ReplyCallback& reply_cb) {
  return georadius(key, longitude, latitude, radius, unit, with_coord, with_dist, with_hash, asc_order, count, "", "", reply_cb);
}

Client& Client::georadius(const String& key, double longitude, double latitude, double radius, GeoUnit unit, bool with_coord, bool with_dist, bool with_hash, bool asc_order, const String& store_key, const ReplyCallback& reply_cb) {
  return georadius(key, longitude, latitude, radius, unit, with_coord, with_dist, with_hash, asc_order, 0, store_key, "", reply_cb);
}

Client& Client::georadius(const String& key, double longitude, double latitude, double radius, GeoUnit unit, bool with_coord, bool with_dist, bool with_hash, bool asc_order, const String& store_key, const String& storedist_key, const ReplyCallback& reply_cb) {
  return georadius(key, longitude, latitude, radius, unit, with_coord, with_dist, with_hash, asc_order, 0, store_key, storedist_key, reply_cb);
}

Client& Client::georadius(const String& key, double longitude, double latitude, double radius, GeoUnit unit, bool with_coord, bool with_dist, bool with_hash, bool asc_order, int32 count, const String& store_key, const ReplyCallback& reply_cb) {
  return georadius(key, longitude, latitude, radius, unit, with_coord, with_dist, with_hash, asc_order, count, store_key, "", reply_cb);
}

Client& Client::georadius(const String& key, double longitude, double latitude, double radius, GeoUnit unit, bool with_coord, bool with_dist, bool with_hash, bool asc_order, int32 count, const String& store_key, const String& storedist_key, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"GEORADIUS", key, fun::ToString(longitude), fun::ToString(latitude), fun::ToString(radius), geo_unit_to_string(unit)};

  // with_coord (optional)
  if (with_coord) {
    cmd << "WITHCOORD";
  }

  // with_dist (optional)
  if (with_dist) {
    cmd << "WITHDIST";
  }

  // with_hash (optional)
  if (with_hash) {
    cmd << "WITHHASH";
  }

  // order (optional)
  cmd << asc_order ? "ASC" : "DESC";

  // count (optional)
  if (count > 0) {
    cmd << "COUNT";
    cmd << fun::ToString(count);
  }

  // store_key (optional)
  if (!store_key.IsEmpty()) {
    cmd << "STOREDIST";
    cmd << storedist_key;
  }

  // storedist_key (optional)
  if (!storedist_key.IsEmpty()) {
    cmd << "STOREDIST";
    cmd << storedist_key;
  }

  Send(cmd, reply_cb);
  return *this;
}

Client& Client::georadiusbymember(const String& key, const String& member, double radius, GeoUnit unit, bool with_coord, bool with_dist, bool with_hash, bool asc_order, const ReplyCallback& reply_cb) {
  return georadiusbymember(key, member, radius, unit, with_coord, with_dist, with_hash, asc_order, 0, "", "", reply_cb);
}

Client& Client::georadiusbymember(const String& key, const String& member, double radius, GeoUnit unit, bool with_coord, bool with_dist, bool with_hash, bool asc_order, int32 count, const ReplyCallback& reply_cb) {
  return georadiusbymember(key, member, radius, unit, with_coord, with_dist, with_hash, asc_order, count, "", "", reply_cb);
}

Client& Client::georadiusbymember(const String& key, const String& member, double radius, GeoUnit unit, bool with_coord, bool with_dist, bool with_hash, bool asc_order, const String& store_key, const ReplyCallback& reply_cb) {
  return georadiusbymember(key, member, radius, unit, with_coord, with_dist, with_hash, asc_order, 0, store_key, "", reply_cb);
}

Client& Client::georadiusbymember(const String& key, const String& member, double radius, GeoUnit unit, bool with_coord, bool with_dist, bool with_hash, bool asc_order, const String& store_key, const String& storedist_key, const ReplyCallback& reply_cb) {
  return georadiusbymember(key, member, radius, unit, with_coord, with_dist, with_hash, asc_order, 0, store_key, storedist_key, reply_cb);
}

Client& Client::georadiusbymember(const String& key, const String& member, double radius, GeoUnit unit, bool with_coord, bool with_dist, bool with_hash, bool asc_order, int32 count, const String& store_key, const ReplyCallback& reply_cb) {
  return georadiusbymember(key, member, radius, unit, with_coord, with_dist, with_hash, asc_order, count, store_key, "", reply_cb);
}

Client& Client::georadiusbymember(const String& key, const String& member, double radius, GeoUnit unit, bool with_coord, bool with_dist, bool with_hash, bool asc_order, int32 count, const String& store_key, const String& storedist_key, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"GEORADIUSBYMEMBER", key, member, fun::ToString(radius), geo_unit_to_string(unit)};

  // with_coord (optional)
  if (with_coord) {
    cmd << "WITHCOORD";
  }

  // with_dist (optional)
  if (with_dist) {
    cmd << "WITHDIST";
  }

  // with_hash (optional)
  if (with_hash) {
    cmd << "WITHHASH";
  }

  // order (optional)
  cmd << asc_order ? "ASC" : "DESC";

  // count (optional)
  if (count > 0) {
    cmd << "COUNT";
    cmd << fun::ToString(count);
  }

  // store_key (optional)
  if (!store_key.IsEmpty()) {
    cmd << "STOREDIST";
    cmd << storedist_key;
  }

  // storedist_key (optional)
  if (!storedist_key.IsEmpty()) {
    cmd << "STOREDIST";
    cmd << storedist_key;
  }

  Send(cmd, reply_cb);
  return *this;
}

Client& Client::get(const String& key, const ReplyCallback& reply_cb) {
  Send({"GET", key}, reply_cb);
  return *this;
}

Client& Client::getbit(const String& key, int offset, const ReplyCallback& reply_cb) {
  Send({"GETBIT", key, fun::ToString(offset)}, reply_cb);
  return *this;
}

Client& Client::getrange(const String& key, int start, int end, const ReplyCallback& reply_cb) {
  Send({"GETRANGE", key, fun::ToString(start), fun::ToString(end)}, reply_cb);
  return *this;
}

Client& Client::getset(const String& key, const String& val, const ReplyCallback& reply_cb) {
  Send({"GETSET", key, val}, reply_cb);
  return *this;
}

Client& Client::hdel(const String& key, const Array<String>& fields, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"HDEL", key};
  cmd << fields;
  Send(cmd, reply_cb);
  return *this;
}

Client& Client::hexists(const String& key, const String& field, const ReplyCallback& reply_cb) {
  Send({"HEXISTS", key, field}, reply_cb);
  return *this;
}

Client& Client::hget(const String& key, const String& field, const ReplyCallback& reply_cb) {
  Send({"HGET", key, field}, reply_cb);
  return *this;
}

Client& Client::hgetall(const String& key, const ReplyCallback& reply_cb) {
  Send({"HGETALL", key}, reply_cb);
  return *this;
}

Client& Client::hincrby(const String& key, const String& field, int incr, const ReplyCallback& reply_cb) {
  Send({"HINCRBY", key, field, fun::ToString(incr)}, reply_cb);
  return *this;
}

Client& Client::hincrbyfloat(const String& key, const String& field, float incr, const ReplyCallback& reply_cb) {
  Send({"HINCRBYFLOAT", key, field, fun::ToString(incr)}, reply_cb);
  return *this;
}

Client& Client::hkeys(const String& key, const ReplyCallback& reply_cb) {
  Send({"HKEYS", key}, reply_cb);
  return *this;
}

Client& Client::hlen(const String& key, const ReplyCallback& reply_cb) {
  Send({"HLEN", key}, reply_cb);
  return *this;
}

Client& Client::hmget(const String& key, const Array<String>& fields, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"HMGET", key};
  cmd << fields;
  Send(cmd, reply_cb);
  return *this;
}

Client& Client::hmset(const String& key, const Array<Pair<String, String>>& field_val, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"HMSET", key};
  for (const auto& obj : field_val) {
    cmd << obj.key);
    cmd << obj.value);
  }
  Send(cmd, reply_cb);
  return *this;
}

Client& Client::hscan(const String& key, int32 cursor, const ReplyCallback& reply_cb) {
  return hscan(key, cursor, "", 0, reply_cb);
}

Client& Client::hscan(const String& key, int32 cursor, const String& pattern, const ReplyCallback& reply_cb) {
  return hscan(key, cursor, pattern, 0, reply_cb);
}

Client& Client::hscan(const String& key, int32 cursor, int32 count, const ReplyCallback& reply_cb) {
  return hscan(key, cursor, "", count, reply_cb);
}

Client& Client::hscan(const String& key, int32 cursor, const String& pattern, int32 count, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"HSCAN", key, fun::ToString(cursor)};

  if (!pattern.IsEmpty()) {
    cmd << "MATCH";
    cmd << pattern;
  }

  if (count > 0) {
    cmd << "COUNT";
    cmd << fun::ToString(count);
  }

  Send(cmd, reply_cb);
  return *this;
}

Client& Client::hset(const String& key, const String& field, const String& value, const ReplyCallback& reply_cb) {
  Send({"HSET", key, field, value}, reply_cb);
  return *this;
}

Client& Client::hsetnx(const String& key, const String& field, const String& value, const ReplyCallback& reply_cb) {
  Send({"HSETNX", key, field, value}, reply_cb);
  return *this;
}

Client& Client::hstrlen(const String& key, const String& field, const ReplyCallback& reply_cb) {
  Send({"HSTRLEN", key, field}, reply_cb);
  return *this;
}

Client& Client::hvals(const String& key, const ReplyCallback& reply_cb) {
  Send({"HVALS", key}, reply_cb);
  return *this;
}

Client& Client::incr(const String& key, const ReplyCallback& reply_cb) {
  Send({"INCR", key}, reply_cb);
  return *this;
}

Client& Client::incrby(const String& key, int incr, const ReplyCallback& reply_cb) {
  Send({"INCRBY", key, fun::ToString(incr)}, reply_cb);
  return *this;
}

Client& Client::incrbyfloat(const String& key, float incr, const ReplyCallback& reply_cb) {
  Send({"INCRBYFLOAT", key, fun::ToString(incr)}, reply_cb);
  return *this;
}

Client& Client::info(const ReplyCallback& reply_cb) {
  Send({"INFO"}, reply_cb);
  return *this;
}

Client& Client::info(const String& section, const ReplyCallback& reply_cb) {
  Send({"INFO", section}, reply_cb);
  return *this;
}

Client& Client::keys(const String& pattern, const ReplyCallback& reply_cb) {
  Send({"KEYS", pattern}, reply_cb);
  return *this;
}

Client& Client::lastsave(const ReplyCallback& reply_cb) {
  Send({"LASTSAVE"}, reply_cb);
  return *this;
}

Client& Client::lindex(const String& key, int index, const ReplyCallback& reply_cb) {
  Send({"LINDEX", key, fun::ToString(index)}, reply_cb);
  return *this;
}

Client& Client::linsert(const String& key, const String& before_after, const String& pivot, const String& value, const ReplyCallback& reply_cb) {
  Send({"LINSERT", key, before_after, pivot, value}, reply_cb);
  return *this;
}

Client& Client::llen(const String& key, const ReplyCallback& reply_cb) {
  Send({"LLEN", key}, reply_cb);
  return *this;
}

Client& Client::lpop(const String& key, const ReplyCallback& reply_cb) {
  Send({"LPOP", key}, reply_cb);
  return *this;
}

Client& Client::lpush(const String& key, const Array<String>& values, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"LPUSH", key};
  cmd << values;
  Send(cmd, reply_cb);
  return *this;
}

Client& Client::lpushx(const String& key, const String& value, const ReplyCallback& reply_cb) {
  Send({"LPUSHX", key, value}, reply_cb);
  return *this;
}

Client& Client::lrange(const String& key, int start, int stop, const ReplyCallback& reply_cb) {
  Send({"LRANGE", key, fun::ToString(start), fun::ToString(stop)}, reply_cb);
  return *this;
}

Client& Client::lrem(const String& key, int count, const String& value, const ReplyCallback& reply_cb) {
  Send({"LREM", key, fun::ToString(count), value}, reply_cb);
  return *this;
}

Client& Client::lset(const String& key, int index, const String& value, const ReplyCallback& reply_cb) {
  Send({"LSET", key, fun::ToString(index), value}, reply_cb);
  return *this;
}

Client& Client::ltrim(const String& key, int start, int stop, const ReplyCallback& reply_cb) {
  Send({"LTRIM", key, fun::ToString(start), fun::ToString(stop)}, reply_cb);
  return *this;
}

Client& Client::mget(const Array<String>& keys, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"MGET"};
  cmd << keys;
  Send(cmd, reply_cb);
  return *this;
}

Client& Client::migrate(const String& host, int port, const String& key, const String& dest_db, int timeout, const ReplyCallback& reply_cb) {
  Send({"MIGRATE", host, fun::ToString(port), key, dest_db, fun::ToString(timeout)}, reply_cb);
  return *this;
}

Client& Client::migrate(const String& host, int port, const String& key, const String& dest_db, int timeout, bool copy, bool replace, const Array<String>& keys, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"MIGRATE", host, fun::ToString(port), key, dest_db, fun::ToString(timeout)};
  if (copy) { cmd << "COPY"; }
  if (replace) { cmd << "REPLACE"; }
  if (keys.Count()) {
    cmd << "KEYS";
    cmd << keys;
  }
  Send(cmd, reply_cb);
  return *this;
}

Client& Client::monitor(const ReplyCallback& reply_cb) {
  Send({"MONITOR"}, reply_cb);
  return *this;
}

Client& Client::move(const String& key, const String& db, const ReplyCallback& reply_cb) {
  Send({"MOVE", key, db}, reply_cb);
  return *this;
}

Client& Client::mset(const Array<Pair<String, String>>& key_vals, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"MSET"};
  for (const auto& obj : key_vals) {
    cmd << obj.key;
    cmd << obj.value;
  }
  Send(cmd, reply_cb);
  return *this;
}

Client& Client::msetnx(const Array<Pair<String, String>>& key_vals, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"MSETNX"};
  for (const auto& obj : key_vals) {
    cmd << obj.key;
    cmd << obj.value;
  }
  Send(cmd, reply_cb);
  return *this;
}

Client& Client::multi(const ReplyCallback& reply_cb) {
  Send({"MULTI"}, reply_cb);
  return *this;
}

Client& Client::object(const String& subcommand, const Array<String>& args, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"OBJECT", subcommand};
  cmd << args;
  Send(cmd, reply_cb);
  return *this;
}

Client& Client::persist(const String& key, const ReplyCallback& reply_cb) {
  Send({"PERSIST", key}, reply_cb);
  return *this;
}

Client& Client::pexpire(const String& key, int milliseconds, const ReplyCallback& reply_cb) {
  Send({"PEXPIRE", key, fun::ToString(milliseconds)}, reply_cb);
  return *this;
}

Client& Client::pexpireat(const String& key, int milliseconds_timestamp, const ReplyCallback& reply_cb) {
  Send({"PEXPIREAT", key, fun::ToString(milliseconds_timestamp)}, reply_cb);
  return *this;
}

Client& Client::pfadd(const String& key, const Array<String>& elements, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"PFADD", key};
  cmd << elements;
  Send(cmd, reply_cb);
  return *this;
}

Client& Client::pfcount(const Array<String>& keys, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"PFCOUNT"};
  cmd << keys;
  Send(cmd, reply_cb);
  return *this;
}

Client& Client::pfmerge(const String& destkey, const Array<String>& sourcekeys, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"PFMERGE", destkey};
  cmd << sourcekeys;
  Send(cmd, reply_cb);
  return *this;
}

Client& Client::ping(const ReplyCallback& reply_cb) {
  Send({"PING"}, reply_cb);
  return *this;
}

Client& Client::ping(const String& message, const ReplyCallback& reply_cb) {
  Send({"PING", message}, reply_cb);
  return *this;
}

Client& Client::psetex(const String& key, int milliseconds, const String& val, const ReplyCallback& reply_cb) {
  Send({"PSETEX", key, fun::ToString(milliseconds), val}, reply_cb);
  return *this;
}

Client& Client::publish(const String& channel, const String& message, const ReplyCallback& reply_cb) {
  Send({"PUBLISH", channel, message}, reply_cb);
  return *this;
}

Client& Client::pubsub(const String& subcommand, const Array<String>& args, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"PUBSUB", subcommand};
  cmd << args;
  Send(cmd, reply_cb);
  return *this;
}

Client& Client::pttl(const String& key, const ReplyCallback& reply_cb) {
  Send({"PTTL", key}, reply_cb);
  return *this;
}

Client& Client::quit(const ReplyCallback& reply_cb) {
  Send({"QUIT"}, reply_cb);
  return *this;
}

Client& Client::randomkey(const ReplyCallback& reply_cb) {
  Send({"RANDOMKEY"}, reply_cb);
  return *this;
}

Client& Client::readonly(const ReplyCallback& reply_cb) {
  Send({"READONLY"}, reply_cb);
  return *this;
}

Client& Client::readwrite(const ReplyCallback& reply_cb) {
  Send({"READWRITE"}, reply_cb);
  return *this;
}

Client& Client::rename(const String& key, const String& newkey, const ReplyCallback& reply_cb) {
  Send({"RENAME", key, newkey}, reply_cb);
  return *this;
}

Client& Client::renamenx(const String& key, const String& newkey, const ReplyCallback& reply_cb) {
  Send({"RENAMENX", key, newkey}, reply_cb);
  return *this;
}

Client& Client::restore(const String& key, int ttl, const String& serialized_value, const ReplyCallback& reply_cb) {
  Send({"RESTORE", key, fun::ToString(ttl), serialized_value}, reply_cb);
  return *this;
}

Client& Client::restore(const String& key, int ttl, const String& serialized_value, const String& replace, const ReplyCallback& reply_cb) {
  Send({"RESTORE", key, fun::ToString(ttl), serialized_value, replace}, reply_cb);
  return *this;
}

Client& Client::role(const ReplyCallback& reply_cb) {
  Send({"ROLE"}, reply_cb);
  return *this;
}

Client& Client::rpop(const String& key, const ReplyCallback& reply_cb) {
  Send({"RPOP", key}, reply_cb);
  return *this;
}

Client& Client::rpoplpush(const String& source, const String& destination, const ReplyCallback& reply_cb) {
  Send({"RPOPLPUSH", source, destination}, reply_cb);
  return *this;
}

Client& Client::rpush(const String& key, const Array<String>& values, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"RPUSH", key};
  cmd << values;
  Send(cmd, reply_cb);
  return *this;
}

Client& Client::rpushx(const String& key, const String& value, const ReplyCallback& reply_cb) {
  Send({"RPUSHX", key, value}, reply_cb);
  return *this;
}

Client& Client::sadd(const String& key, const Array<String>& members, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"SADD", key};
  cmd << members;
  Send(cmd, reply_cb);
  return *this;
}

Client& Client::save(const ReplyCallback& reply_cb) {
  Send({"SAVE"}, reply_cb);
  return *this;
}

Client& Client::scan(int32 cursor, const ReplyCallback& reply_cb) {
  return scan(cursor, "", 0, reply_cb);
}

Client& Client::scan(int32 cursor, const String& pattern, const ReplyCallback& reply_cb) {
  return scan(cursor, pattern, 0, reply_cb);
}

Client& Client::scan(int32 cursor, int32 count, const ReplyCallback& reply_cb) {
  return scan(cursor, "", count, reply_cb);
}

Client& Client::scan(int32 cursor, const String& pattern, int32 count, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"SCAN", fun::ToString(cursor)};

  if (!pattern.IsEmpty()) {
    cmd << "MATCH";
    cmd << pattern;
  }

  if (count > 0) {
    cmd << "COUNT";
    cmd << fun::ToString(count);
  }

  Send(cmd, reply_cb);
  return *this;
}

Client& Client::scard(const String& key, const ReplyCallback& reply_cb) {
  Send({"SCARD", key}, reply_cb);
  return *this;
}

Client& Client::script_debug(const String& mode, const ReplyCallback& reply_cb) {
  Send({"SCRIPT", "DEBUG", mode}, reply_cb);
  return *this;
}

Client& Client::script_exists(const Array<String>& scripts, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"SCRIPT", "EXISTS"};
  cmd << scripts;
  Send(cmd, reply_cb);
  return *this;
}

Client& Client::script_flush(const ReplyCallback& reply_cb) {
  Send({"SCRIPT", "FLUSH"}, reply_cb);
  return *this;
}

Client& Client::script_kill(const ReplyCallback& reply_cb) {
  Send({"SCRIPT", "KILL"}, reply_cb);
  return *this;
}

Client& Client::script_load(const String& script, const ReplyCallback& reply_cb) {
  Send({"SCRIPT", "LOAD", script}, reply_cb);
  return *this;
}

Client& Client::sdiff(const Array<String>& keys, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"SDIFF"};
  cmd << keys;
  Send(cmd, reply_cb);
  return *this;
}

Client& Client::sdiffstore(const String& destination, const Array<String>& keys, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"SDIFFSTORE", destination};
  cmd << keys;
  Send(cmd, reply_cb);
  return *this;
}

Client& Client::select(int index, const ReplyCallback& reply_cb) {
  ScopedLock<FastMutex> lock(callbacks_mutex_);

  UnprotectedSelect(index, reply_cb);

  return *this;
}

void Client::UnprotectedSelect(int index, const ReplyCallback& reply_cb) {
  // save the index of the database for Reconnect attempts.
  database_index_ = index;
  // save command in the pipeline
  UnprotectedSend({"SELECT", fun::ToString(index)}, reply_cb);
}

Client& Client::set(const String& key, const String& value, const ReplyCallback& reply_cb) {
  Send({"SET", key, value}, reply_cb);
  return *this;
}

Client& Client::set_advanced(const String& key, const String& value, const ReplyCallback& reply_cb) {
  Send({"SET", key, value}, reply_cb);
  return *this;
}

Client& Client::set_advanced(const String& key, const String& value, bool ex, int ex_sec, bool px, int px_milli, bool nx, bool xx, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"SET", key, value};

  if (ex) {
    cmd << "EX";
    cmd << fun::ToString(ex_sec);
  }

  if (px) {
    cmd << "PX";
    cmd << fun::ToString(px_milli);
  }

  if (nx) { cmd << "NX"; }
  if (xx) { cmd << "XX"; }

  Send(cmd, reply_cb);
  return *this;
}

Client& Client::setbit_(const String& key, int offset, const String& value, const ReplyCallback& reply_cb) {
  Send({"SETBIT", key, fun::ToString(offset), value}, reply_cb);
  return *this;
}

Client& Client::setex(const String& key, int seconds, const String& value, const ReplyCallback& reply_cb) {
  Send({"SETEX", key, fun::ToString(seconds), value}, reply_cb);
  return *this;
}

Client& Client::setnx(const String& key, const String& value, const ReplyCallback& reply_cb) {
  Send({"SETNX", key, value}, reply_cb);
  return *this;
}

Client& Client::setrange(const String& key, int offset, const String& value, const ReplyCallback& reply_cb) {
  Send({"SETRANGE", key, fun::ToString(offset), value}, reply_cb);
  return *this;
}

Client& Client::shutdown(const ReplyCallback& reply_cb) {
  Send({"SHUTDOWN"}, reply_cb);
  return *this;
}

Client& Client::shutdown(const String& save, const ReplyCallback& reply_cb) {
  Send({"SHUTDOWN", save}, reply_cb);
  return *this;
}

Client& Client::sinter(const Array<String>& keys, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"SINTER"};
  cmd << keys;
  Send(cmd, reply_cb);
  return *this;
}

Client& Client::sinterstore(const String& destination, const Array<String>& keys, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"SINTERSTORE", destination};
  cmd << keys;
  Send(cmd, reply_cb);
  return *this;
}

Client& Client::sismember(const String& key, const String& member, const ReplyCallback& reply_cb) {
  Send({"SISMEMBER", key, member}, reply_cb);
  return *this;
}

Client& Client::slaveof(const String& host, int port, const ReplyCallback& reply_cb) {
  Send({"SLAVEOF", host, fun::ToString(port)}, reply_cb);
  return *this;
}

Client& Client::slowlog(const String subcommand, const ReplyCallback& reply_cb) {
  Send({"SLOWLOG", subcommand}, reply_cb);
  return *this;
}

Client& Client::slowlog(const String subcommand, const String& argument, const ReplyCallback& reply_cb) {
  Send({"SLOWLOG", subcommand, argument}, reply_cb);
  return *this;
}

Client& Client::smembers(const String& key, const ReplyCallback& reply_cb) {
  Send({"SMEMBERS", key}, reply_cb);
  return *this;
}

Client& Client::smove(const String& source, const String& destination, const String& member, const ReplyCallback& reply_cb) {
  Send({"SMOVE", source, destination, member}, reply_cb);
  return *this;
}


Client& Client::sort(const String& key, const ReplyCallback& reply_cb) {
  Send({"SORT", key}, reply_cb);
  return *this;
}

Client& Client::sort(const String& key, const Array<String>& get_patterns, bool asc_order, bool alpha, const ReplyCallback& reply_cb) {
  return sort(key, "", false, 0, 0, get_patterns, asc_order, alpha, "", reply_cb);
}

Client& Client::sort(const String& key, int32 offset, int32 count, const Array<String>& get_patterns, bool asc_order, bool alpha, const ReplyCallback& reply_cb) {
  return sort(key, "", true, offset, count, get_patterns, asc_order, alpha, "", reply_cb);
}

Client& Client::sort(const String& key, const String& by_pattern, const Array<String>& get_patterns, bool asc_order, bool alpha, const ReplyCallback& reply_cb) {
  return sort(key, by_pattern, false, 0, 0, get_patterns, asc_order, alpha, "", reply_cb);
}

Client& Client::sort(const String& key, const Array<String>& get_patterns, bool asc_order, bool alpha, const String& store_dest, const ReplyCallback& reply_cb) {
  return sort(key, "", false, 0, 0, get_patterns, asc_order, alpha, store_dest, reply_cb);
}

Client& Client::sort(const String& key, int32 offset, int32 count, const Array<String>& get_patterns, bool asc_order, bool alpha, const String& store_dest, const ReplyCallback& reply_cb) {
  return sort(key, "", true, offset, count, get_patterns, asc_order, alpha, store_dest, reply_cb);
}

Client& Client::sort(const String& key, const String& by_pattern, const Array<String>& get_patterns, bool asc_order, bool alpha, const String& store_dest, const ReplyCallback& reply_cb) {
  return sort(key, by_pattern, false, 0, 0, get_patterns, asc_order, alpha, store_dest, reply_cb);
}

Client& Client::sort(const String& key, const String& by_pattern, int32 offset, int32 count, const Array<String>& get_patterns, bool asc_order, bool alpha, const ReplyCallback& reply_cb) {
  return sort(key, by_pattern, true, offset, count, get_patterns, asc_order, alpha, "", reply_cb);
}

Client& Client::sort(const String& key, const String& by_pattern, int32 offset, int32 count, const Array<String>& get_patterns, bool asc_order, bool alpha, const String& store_dest, const ReplyCallback& reply_cb) {
  return sort(key, by_pattern, true, offset, count, get_patterns, asc_order, alpha, store_dest, reply_cb);
}

Client& Client::sort(const String& key, const String& by_pattern, bool limit, int32 offset, int32 count, const Array<String>& get_patterns, bool asc_order, bool alpha, const String& store_dest, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"SORT", key};

  // add by pattern (optional)
  if (!by_pattern.IsEmpty()) {
    cmd << "BY";
    cmd << by_pattern;
  }

  // add limit (optional)
  if (limit) {
    cmd << "LIMIT";
    cmd << fun::ToString(offset);
    cmd << fun::ToString(count);
  }

  // add get pattern (optional)
  for (const auto& get_pattern : get_patterns) {
    if (get_pattern.IsEmpty()) {
      continue;
    }

    cmd << "GET";
    cmd << get_pattern;
  }

  // add order by (optional)
  cmd << (asc_order ? "ASC" : "DESC");

  // add alpha (optional)
  if (alpha) {
    cmd << "ALPHA";
  }

  // add store dst (optional)
  if (!store_dest.IsEmpty()) {
    cmd << "STORE";
    cmd << store_dest;
  }

  Send(cmd, reply_cb);
  return *this;
}

Client& Client::spop(const String& key, const ReplyCallback& reply_cb) {
  Send({"SPOP", key}, reply_cb);
  return *this;
}

Client& Client::spop(const String& key, int count, const ReplyCallback& reply_cb) {
  Send({"SPOP", key, fun::ToString(count)}, reply_cb);
  return *this;
}

Client& Client::srandmember(const String& key, const ReplyCallback& reply_cb) {
  Send({"SRANDMEMBER", key}, reply_cb);
  return *this;
}

Client& Client::srandmember(const String& key, int count, const ReplyCallback& reply_cb) {
  Send({"SRANDMEMBER", key, fun::ToString(count)}, reply_cb);
  return *this;
}

Client& Client::srem(const String& key, const Array<String>& members, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"SREM", key};
  cmd << members;
  Send(cmd, reply_cb);
  return *this;
}

Client& Client::sscan(const String& key, int32 cursor, const ReplyCallback& reply_cb) {
  return sscan(key, cursor, "", 0, reply_cb);
}

Client& Client::sscan(const String& key, int32 cursor, const String& pattern, const ReplyCallback& reply_cb) {
  return sscan(key, cursor, pattern, 0, reply_cb);
}

Client& Client::sscan(const String& key, int32 cursor, int32 count, const ReplyCallback& reply_cb) {
  return sscan(key, cursor, "", count, reply_cb);
}

Client& Client::sscan(const String& key, int32 cursor, const String& pattern, int32 count, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"SSCAN", key, fun::ToString(cursor)};

  if (!pattern.IsEmpty()) {
    cmd << "MATCH";
    cmd << pattern;
  }

  if (count > 0) {
    cmd << "COUNT";
    cmd << fun::ToString(count);
  }

  Send(cmd, reply_cb);
  return *this;
}

Client& Client::strlen(const String& key, const ReplyCallback& reply_cb) {
  Send({"STRLEN", key}, reply_cb);
  return *this;
}

Client& Client::sunion(const Array<String>& keys, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"SUNION"};
  cmd << keys;
  Send(cmd, reply_cb);
  return *this;
}

Client& Client::sunionstore(const String& destination, const Array<String>& keys, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"SUNIONSTORE", destination};
  cmd << keys;
  Send(cmd, reply_cb);
  return *this;
}

Client& Client::sync(const ReplyCallback& reply_cb) {
  Send({"SYNC"}, reply_cb);
  return *this;
}

Client& Client::time(const ReplyCallback& reply_cb) {
  Send({"TIME"}, reply_cb);
  return *this;
}

Client& Client::ttl(const String& key, const ReplyCallback& reply_cb) {
  Send({"TTL", key}, reply_cb);
  return *this;
}

Client& Client::Type(const String& key, const ReplyCallback& reply_cb) {
  Send({"TYPE", key}, reply_cb);
  return *this;
}

Client& Client::unwatch(const ReplyCallback& reply_cb) {
  Send({"UNWATCH"}, reply_cb);
  return *this;
}

Client& Client::wait(int numslaves, int timeout, const ReplyCallback& reply_cb) {
  Send({"WAIT", fun::ToString(numslaves), fun::ToString(timeout)}, reply_cb);
  return *this;
}

Client& Client::watch(const Array<String>& keys, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"WATCH"};
  cmd << keys;
  Send(cmd, reply_cb);
  return *this;
}

Client& Client::zadd(const String& key, const Array<String>& options, const tl::multimap<String, String>& score_members, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"ZADD", key};

  // options
  cmd << options;

  // score members
  for (auto& sm : score_members) {
    cmd << sm.key;
    cmd << sm.value;
  }

  Send(cmd, reply_cb);
  return *this;
}

Client& Client::zcard(const String& key, const ReplyCallback& reply_cb) {
  Send({"ZCARD", key}, reply_cb);
  return *this;
}

Client& Client::zcount(const String& key, int min, int max, const ReplyCallback& reply_cb) {
  Send({"ZCOUNT", key, fun::ToString(min), fun::ToString(max)}, reply_cb);
  return *this;
}

Client& Client::zcount(const String& key, double min, double max, const ReplyCallback& reply_cb) {
  Send({"ZCOUNT", key, fun::ToString(min), fun::ToString(max)}, reply_cb);
  return *this;
}

Client& Client::zcount(const String& key, const String& min, const String& max, const ReplyCallback& reply_cb) {
  Send({"ZCOUNT", key, min, max}, reply_cb);
  return *this;
}

Client& Client::zincrby(const String& key, int incr, const String& member, const ReplyCallback& reply_cb) {
  Send({"ZINCRBY", key, fun::ToString(incr), member}, reply_cb);
  return *this;
}

Client& Client::zincrby(const String& key, double incr, const String& member, const ReplyCallback& reply_cb) {
  Send({"ZINCRBY", key, fun::ToString(incr), member}, reply_cb);
  return *this;
}

Client& Client::zincrby(const String& key, const String& incr, const String& member, const ReplyCallback& reply_cb) {
  Send({"ZINCRBY", key, incr, member}, reply_cb);
  return *this;
}

Client& Client::zinterstore(const String& destination, int32 numkeys, const Array<String>& keys, const Array<int32> weights, AggregateMethod method, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"ZINTERSTORE", destination, fun::ToString(numkeys)};

  // keys
  for (const auto& key : keys) {
    cmd << key;
  }

  // weights (optional)
  if (!weights.IsEmpty()) {
    cmd << "WEIGHTS";

    for (auto weight : weights) {
      cmd << fun::ToString(weight);
    }
  }

  // aggregate method
  if (method != AggregateMethod::server_default) {
    cmd << "AGGREGATE";
    cmd << aggregate_method_to_string(method);
  }

  Send(cmd, reply_cb);
  return *this;
}

Client& Client::zlexcount(const String& key, int min, int max, const ReplyCallback& reply_cb) {
  Send({"ZLEXCOUNT", key, fun::ToString(min), fun::ToString(max)}, reply_cb);
  return *this;
}

Client& Client::zlexcount(const String& key, double min, double max, const ReplyCallback& reply_cb) {
  Send({"ZLEXCOUNT", key, fun::ToString(min), fun::ToString(max)}, reply_cb);
  return *this;
}

Client& Client::zlexcount(const String& key, const String& min, const String& max, const ReplyCallback& reply_cb) {
  Send({"ZLEXCOUNT", key, min, max}, reply_cb);
  return *this;
}

Client& Client::zrange(const String& key, int start, int stop, const ReplyCallback& reply_cb) {
  Send({"ZRANGE", key, fun::ToString(start), fun::ToString(stop)}, reply_cb);
  return *this;
}

Client& Client::zrange(const String& key, int start, int stop, bool withscores, const ReplyCallback& reply_cb) {
  if (withscores)
    Send({"ZRANGE", key, fun::ToString(start), fun::ToString(stop), "WITHSCORES"}, reply_cb);
  else
    Send({"ZRANGE", key, fun::ToString(start), fun::ToString(stop)}, reply_cb);
  return *this;
}

Client& Client::zrange(const String& key, double start, double stop, const ReplyCallback& reply_cb) {
  Send({"ZRANGE", key, fun::ToString(start), fun::ToString(stop)}, reply_cb);
  return *this;
}

Client& Client::zrange(const String& key, double start, double stop, bool withscores, const ReplyCallback& reply_cb) {
  if (withscores)
    Send({"ZRANGE", key, fun::ToString(start), fun::ToString(stop), "WITHSCORES"}, reply_cb);
  else
    Send({"ZRANGE", key, fun::ToString(start), fun::ToString(stop)}, reply_cb);
  return *this;
}

Client& Client::zrange(const String& key, const String& start, const String& stop, const ReplyCallback& reply_cb) {
  Send({"ZRANGE", key, start, stop}, reply_cb);
  return *this;
}

Client& Client::zrange(const String& key, const String& start, const String& stop, bool withscores, const ReplyCallback& reply_cb) {
  if (withscores)
    Send({"ZRANGE", key, start, stop, "WITHSCORES"}, reply_cb);
  else
    Send({"ZRANGE", key, start, stop}, reply_cb);
  return *this;
}

Client& Client::zrangebylex(const String& key, int min, int max, const ReplyCallback& reply_cb) {
  return zrangebylex(key, fun::ToString(min), fun::ToString(max), false, 0, 0, false, reply_cb);
}

Client& Client::zrangebylex(const String& key, int min, int max, bool withscores, const ReplyCallback& reply_cb) {
  return zrangebylex(key, fun::ToString(min), fun::ToString(max), false, 0, 0, withscores, reply_cb);
}

Client& Client::zrangebylex(const String& key, double min, double max, const ReplyCallback& reply_cb) {
  return zrangebylex(key, fun::ToString(min), fun::ToString(max), false, 0, 0, false, reply_cb);
}

Client& Client::zrangebylex(const String& key, double min, double max, bool withscores, const ReplyCallback& reply_cb) {
  return zrangebylex(key, fun::ToString(min), fun::ToString(max), false, 0, 0, withscores, reply_cb);
}

Client& Client::zrangebylex(const String& key, const String& min, const String& max, const ReplyCallback& reply_cb) {
  return zrangebylex(key, min, max, false, 0, 0, false, reply_cb);
}

Client& Client::zrangebylex(const String& key, const String& min, const String& max, bool withscores, const ReplyCallback& reply_cb) {
  return zrangebylex(key, min, max, false, 0, 0, withscores, reply_cb);
}

Client& Client::zrangebylex(const String& key, int min, int max, int32 offset, int32 count, const ReplyCallback& reply_cb) {
  return zrangebylex(key, fun::ToString(min), fun::ToString(max), true, offset, count, false, reply_cb);
}

Client& Client::zrangebylex(const String& key, int min, int max, int32 offset, int32 count, bool withscores, const ReplyCallback& reply_cb) {
  return zrangebylex(key, fun::ToString(min), fun::ToString(max), true, offset, count, withscores, reply_cb);
}

Client& Client::zrangebylex(const String& key, double min, double max, int32 offset, int32 count, const ReplyCallback& reply_cb) {
  return zrangebylex(key, fun::ToString(min), fun::ToString(max), true, offset, count, false, reply_cb);
}

Client& Client::zrangebylex(const String& key, double min, double max, int32 offset, int32 count, bool withscores, const ReplyCallback& reply_cb) {
  return zrangebylex(key, fun::ToString(min), fun::ToString(max), true, offset, count, withscores, reply_cb);
}

Client& Client::zrangebylex(const String& key, const String& min, const String& max, int32 offset, int32 count, const ReplyCallback& reply_cb) {
  return zrangebylex(key, min, max, true, offset, count, false, reply_cb);
}

Client& Client::zrangebylex(const String& key, const String& min, const String& max, int32 offset, int32 count, bool withscores, const ReplyCallback& reply_cb) {
  return zrangebylex(key, min, max, true, offset, count, withscores, reply_cb);
}

Client& Client::zrangebylex(const String& key, const String& min, const String& max, bool limit, int32 offset, int32 count, bool withscores, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"ZRANGEBYLEX", key, min, max};

  // withscores (optional)
  if (withscores) {
    cmd << "WITHSCORES";
  }

  // limit (optional)
  if (limit) {
    cmd << "LIMIT";
    cmd << fun::ToString(offset);
    cmd << fun::ToString(count);
  }

  Send(cmd, reply_cb);
  return *this;
}

Client& Client::zrangebyscore(const String& key, int min, int max, const ReplyCallback& reply_cb) {
  return zrangebyscore(key, fun::ToString(min), fun::ToString(max), false, 0, 0, false, reply_cb);
}

Client& Client::zrangebyscore(const String& key, int min, int max, bool withscores, const ReplyCallback& reply_cb) {
  return zrangebyscore(key, fun::ToString(min), fun::ToString(max), false, 0, 0, withscores, reply_cb);
}

Client& Client::zrangebyscore(const String& key, double min, double max, const ReplyCallback& reply_cb) {
  return zrangebyscore(key, fun::ToString(min), fun::ToString(max), false, 0, 0, false, reply_cb);
}

Client& Client::zrangebyscore(const String& key, double min, double max, bool withscores, const ReplyCallback& reply_cb) {
  return zrangebyscore(key, fun::ToString(min), fun::ToString(max), false, 0, 0, withscores, reply_cb);
}

Client& Client::zrangebyscore(const String& key, const String& min, const String& max, const ReplyCallback& reply_cb) {
  return zrangebyscore(key, min, max, false, 0, 0, false, reply_cb);
}

Client& Client::zrangebyscore(const String& key, const String& min, const String& max, bool withscores, const ReplyCallback& reply_cb) {
  return zrangebyscore(key, min, max, false, 0, 0, withscores, reply_cb);
}

Client& Client::zrangebyscore(const String& key, int min, int max, int32 offset, int32 count, const ReplyCallback& reply_cb) {
  return zrangebyscore(key, fun::ToString(min), fun::ToString(max), true, offset, count, false, reply_cb);
}

Client& Client::zrangebyscore(const String& key, int min, int max, int32 offset, int32 count, bool withscores, const ReplyCallback& reply_cb) {
  return zrangebyscore(key, fun::ToString(min), fun::ToString(max), true, offset, count, withscores, reply_cb);
}

Client& Client::zrangebyscore(const String& key, double min, double max, int32 offset, int32 count, const ReplyCallback& reply_cb) {
  return zrangebyscore(key, fun::ToString(min), fun::ToString(max), true, offset, count, false, reply_cb);
}

Client& Client::zrangebyscore(const String& key, double min, double max, int32 offset, int32 count, bool withscores, const ReplyCallback& reply_cb) {
  return zrangebyscore(key, fun::ToString(min), fun::ToString(max), true, offset, count, withscores, reply_cb);
}

Client& Client::zrangebyscore(const String& key, const String& min, const String& max, int32 offset, int32 count, const ReplyCallback& reply_cb) {
  return zrangebyscore(key, min, max, true, offset, count, false, reply_cb);
}

Client& Client::zrangebyscore(const String& key, const String& min, const String& max, int32 offset, int32 count, bool withscores, const ReplyCallback& reply_cb) {
  return zrangebyscore(key, min, max, true, offset, count, withscores, reply_cb);
}

Client& Client::zrangebyscore(const String& key, const String& min, const String& max, bool limit, int32 offset, int32 count, bool withscores, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"ZRANGEBYSCORE", key, min, max};

  // withscores (optional)
  if (withscores) {
    cmd << "WITHSCORES";
  }

  // limit (optional)
  if (limit) {
    cmd << "LIMIT";
    cmd << fun::ToString(offset);
    cmd << fun::ToString(count);
  }

  Send(cmd, reply_cb);
  return *this;
}

Client& Client::zrank(const String& key, const String& member, const ReplyCallback& reply_cb) {
  Send({"ZRANK", key, member}, reply_cb);
  return *this;
}

Client& Client::zrem(const String& key, const Array<String>& members, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"ZREM", key};
  cmd << members;
  Send(cmd, reply_cb);
  return *this;
}

Client& Client::zremrangebylex(const String& key, int min, int max, const ReplyCallback& reply_cb) {
  Send({"ZREMRANGEBYLEX", key, fun::ToString(min), fun::ToString(max)}, reply_cb);
  return *this;
}

Client& Client::zremrangebylex(const String& key, double min, double max, const ReplyCallback& reply_cb) {
  Send({"ZREMRANGEBYLEX", key, fun::ToString(min), fun::ToString(max)}, reply_cb);
  return *this;
}

Client& Client::zremrangebylex(const String& key, const String& min, const String& max, const ReplyCallback& reply_cb) {
  Send({"ZREMRANGEBYLEX", key, min, max}, reply_cb);
  return *this;
}

Client& Client::zremrangebyrank(const String& key, int start, int stop, const ReplyCallback& reply_cb) {
  Send({"ZREMRANGEBYRANK", key, fun::ToString(start), fun::ToString(stop)}, reply_cb);
  return *this;
}

Client& Client::zremrangebyrank(const String& key, double start, double stop, const ReplyCallback& reply_cb) {
  Send({"ZREMRANGEBYRANK", key, fun::ToString(start), fun::ToString(stop)}, reply_cb);
  return *this;
}

Client& Client::zremrangebyrank(const String& key, const String& start, const String& stop, const ReplyCallback& reply_cb) {
  Send({"ZREMRANGEBYRANK", key, start, stop}, reply_cb);
  return *this;
}

Client& Client::zremrangebyscore(const String& key, int min, int max, const ReplyCallback& reply_cb) {
  Send({"ZREMRANGEBYSCORE", key, fun::ToString(min), fun::ToString(max)}, reply_cb);
  return *this;
}

Client& Client::zremrangebyscore(const String& key, double min, double max, const ReplyCallback& reply_cb) {
  Send({"ZREMRANGEBYSCORE", key, fun::ToString(min), fun::ToString(max)}, reply_cb);
  return *this;
}

Client& Client::zremrangebyscore(const String& key, const String& min, const String& max, const ReplyCallback& reply_cb) {
  Send({"ZREMRANGEBYSCORE", key, min, max}, reply_cb);
  return *this;
}

Client& Client::zrevrange(const String& key, int start, int stop, const ReplyCallback& reply_cb) {
  Send({"ZREVRANGE", key, fun::ToString(start), fun::ToString(stop)}, reply_cb);
  return *this;
}

Client& Client::zrevrange(const String& key, int start, int stop, bool withscores, const ReplyCallback& reply_cb) {
  if (withscores)
    Send({"ZREVRANGE", key, fun::ToString(start), fun::ToString(stop), "WITHSCORES"}, reply_cb);
  else
    Send({"ZREVRANGE", key, fun::ToString(start), fun::ToString(stop)}, reply_cb);
  return *this;
}

Client& Client::zrevrange(const String& key, double start, double stop, const ReplyCallback& reply_cb) {
  Send({"ZREVRANGE", key, fun::ToString(start), fun::ToString(stop)}, reply_cb);
  return *this;
}

Client& Client::zrevrange(const String& key, double start, double stop, bool withscores, const ReplyCallback& reply_cb) {
  if (withscores)
    Send({"ZREVRANGE", key, fun::ToString(start), fun::ToString(stop), "WITHSCORES"}, reply_cb);
  else
    Send({"ZREVRANGE", key, fun::ToString(start), fun::ToString(stop)}, reply_cb);
  return *this;
}

Client& Client::zrevrange(const String& key, const String& start, const String& stop, const ReplyCallback& reply_cb) {
  Send({"ZREVRANGE", key, start, stop}, reply_cb);
  return *this;
}

Client& Client::zrevrange(const String& key, const String& start, const String& stop, bool withscores, const ReplyCallback& reply_cb) {
  if (withscores)
    Send({"ZREVRANGE", key, start, stop, "WITHSCORES"}, reply_cb);
  else
    Send({"ZREVRANGE", key, start, stop}, reply_cb);
  return *this;
}

Client& Client::zrevrangebylex(const String& key, int max, int min, const ReplyCallback& reply_cb) {
  return zrevrangebylex(key, fun::ToString(max), fun::ToString(min), false, 0, 0, false, reply_cb);
}

Client& Client::zrevrangebylex(const String& key, int max, int min, bool withscores, const ReplyCallback& reply_cb) {
  return zrevrangebylex(key, fun::ToString(max), fun::ToString(min), false, 0, 0, withscores, reply_cb);
}

Client& Client::zrevrangebylex(const String& key, double max, double min, const ReplyCallback& reply_cb) {
  return zrevrangebylex(key, fun::ToString(max), fun::ToString(min), false, 0, 0, false, reply_cb);
}

Client& Client::zrevrangebylex(const String& key, double max, double min, bool withscores, const ReplyCallback& reply_cb) {
  return zrevrangebylex(key, fun::ToString(max), fun::ToString(min), false, 0, 0, withscores, reply_cb);
}

Client& Client::zrevrangebylex(const String& key, const String& max, const String& min, const ReplyCallback& reply_cb) {
  return zrevrangebylex(key, max, min, false, 0, 0, false, reply_cb);
}

Client& Client::zrevrangebylex(const String& key, const String& max, const String& min, bool withscores, const ReplyCallback& reply_cb) {
  return zrevrangebylex(key, max, min, false, 0, 0, withscores, reply_cb);
}

Client& Client::zrevrangebylex(const String& key, int max, int min, int32 offset, int32 count, const ReplyCallback& reply_cb) {
  return zrevrangebylex(key, fun::ToString(max), fun::ToString(min), true, offset, count, false, reply_cb);
}

Client& Client::zrevrangebylex(const String& key, int max, int min, int32 offset, int32 count, bool withscores, const ReplyCallback& reply_cb) {
  return zrevrangebylex(key, fun::ToString(max), fun::ToString(min), true, offset, count, withscores, reply_cb);
}

Client& Client::zrevrangebylex(const String& key, double max, double min, int32 offset, int32 count, const ReplyCallback& reply_cb) {
  return zrevrangebylex(key, fun::ToString(max), fun::ToString(min), true, offset, count, false, reply_cb);
}

Client& Client::zrevrangebylex(const String& key, double max, double min, int32 offset, int32 count, bool withscores, const ReplyCallback& reply_cb) {
  return zrevrangebylex(key, fun::ToString(max), fun::ToString(min), true, offset, count, withscores, reply_cb);
}

Client& Client::zrevrangebylex(const String& key, const String& max, const String& min, int32 offset, int32 count, const ReplyCallback& reply_cb) {
  return zrevrangebylex(key, max, min, true, offset, count, false, reply_cb);
}

Client& Client::zrevrangebylex(const String& key, const String& max, const String& min, int32 offset, int32 count, bool withscores, const ReplyCallback& reply_cb) {
  return zrevrangebylex(key, max, min, true, offset, count, withscores, reply_cb);
}

Client& Client::zrevrangebylex(const String& key, const String& max, const String& min, bool limit, int32 offset, int32 count, bool withscores, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"ZREVRANGEBYLEX", key, max, min};

  // withscores (optional)
  if (withscores) {
    cmd << "WITHSCORES";
  }

  // limit (optional)
  if (limit) {
    cmd << "LIMIT";
    cmd << fun::ToString(offset);
    cmd << fun::ToString(count);
  }

  Send(cmd, reply_cb);
  return *this;
}

Client& Client::zrevrangebyscore(const String& key, int max, int min, const ReplyCallback& reply_cb) {
  return zrevrangebyscore(key, fun::ToString(max), fun::ToString(min), false, 0, 0, false, reply_cb);
}

Client& Client::zrevrangebyscore(const String& key, int max, int min, bool withscores, const ReplyCallback& reply_cb) {
  return zrevrangebyscore(key, fun::ToString(max), fun::ToString(min), false, 0, 0, withscores, reply_cb);
}

Client& Client::zrevrangebyscore(const String& key, double max, double min, const ReplyCallback& reply_cb) {
  return zrevrangebyscore(key, fun::ToString(max), fun::ToString(min), false, 0, 0, false, reply_cb);
}

Client& Client::zrevrangebyscore(const String& key, double max, double min, bool withscores, const ReplyCallback& reply_cb) {
  return zrevrangebyscore(key, fun::ToString(max), fun::ToString(min), false, 0, 0, withscores, reply_cb);
}

Client& Client::zrevrangebyscore(const String& key, const String& max, const String& min, const ReplyCallback& reply_cb) {
  return zrevrangebyscore(key, max, min, false, 0, 0, false, reply_cb);
}

Client& Client::zrevrangebyscore(const String& key, const String& max, const String& min, bool withscores, const ReplyCallback& reply_cb) {
  return zrevrangebyscore(key, max, min, false, 0, 0, withscores, reply_cb);
}

Client& Client::zrevrangebyscore(const String& key, int max, int min, int32 offset, int32 count, const ReplyCallback& reply_cb) {
  return zrevrangebyscore(key, fun::ToString(max), fun::ToString(min), true, offset, count, false, reply_cb);
}

Client& Client::zrevrangebyscore(const String& key, int max, int min, int32 offset, int32 count, bool withscores, const ReplyCallback& reply_cb) {
  return zrevrangebyscore(key, fun::ToString(max), fun::ToString(min), true, offset, count, withscores, reply_cb);
}

Client& Client::zrevrangebyscore(const String& key, double max, double min, int32 offset, int32 count, const ReplyCallback& reply_cb) {
  return zrevrangebyscore(key, fun::ToString(max), fun::ToString(min), true, offset, count, false, reply_cb);
}

Client& Client::zrevrangebyscore(const String& key, double max, double min, int32 offset, int32 count, bool withscores, const ReplyCallback& reply_cb) {
  return zrevrangebyscore(key, fun::ToString(max), fun::ToString(min), true, offset, count, withscores, reply_cb);
}

Client& Client::zrevrangebyscore(const String& key, const String& max, const String& min, int32 offset, int32 count, const ReplyCallback& reply_cb) {
  return zrevrangebyscore(key, max, min, true, offset, count, false, reply_cb);
}

Client& Client::zrevrangebyscore(const String& key, const String& max, const String& min, int32 offset, int32 count, bool withscores, const ReplyCallback& reply_cb) {
  return zrevrangebyscore(key, max, min, true, offset, count, withscores, reply_cb);
}

Client& Client::zrevrangebyscore(const String& key, const String& max, const String& min, bool limit, int32 offset, int32 count, bool withscores, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"ZREVRANGEBYSCORE", key, max, min};

  // withscores (optional)
  if (withscores) {
    cmd << "WITHSCORES";
  }

  // limit (optional)
  if (limit) {
    cmd << "LIMIT";
    cmd << fun::ToString(offset);
    cmd << fun::ToString(count);
  }

  Send(cmd, reply_cb);
  return *this;
}

Client& Client::zrevrank(const String& key, const String& member, const ReplyCallback& reply_cb) {
  Send({"ZREVRANK", key, member}, reply_cb);
  return *this;
}

Client& Client::zscan(const String& key, int32 cursor, const ReplyCallback& reply_cb) {
  return zscan(key, cursor, "", 0, reply_cb);
}

Client& Client::zscan(const String& key, int32 cursor, const String& pattern, const ReplyCallback& reply_cb) {
  return zscan(key, cursor, pattern, 0, reply_cb);
}

Client& Client::zscan(const String& key, int32 cursor, int32 count, const ReplyCallback& reply_cb) {
  return zscan(key, cursor, "", count, reply_cb);
}

Client& Client::zscan(const String& key, int32 cursor, const String& pattern, int32 count, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"ZSCAN", key, fun::ToString(cursor)};

  if (!pattern.IsEmpty()) {
    cmd << "MATCH";
    cmd << pattern;
  }

  if (count > 0) {
    cmd << "COUNT";
    cmd << fun::ToString(count);
  }

  Send(cmd, reply_cb);
  return *this;
}

Client& Client::zscore(const String& key, const String& member, const ReplyCallback& reply_cb) {
  Send({"ZSCORE", key, member}, reply_cb);
  return *this;
}

Client& Client::zunionstore(const String& destination, int32 numkeys, const Array<String>& keys, const Array<int32> weights, AggregateMethod method, const ReplyCallback& reply_cb) {
  Array<String> cmd = {"ZUNIONSTORE", destination, fun::ToString(numkeys)};

  // keys
  for (const auto& key : keys) {
    cmd << key;
  }

  // weights (optional)
  if (!weights.IsEmpty()) {
    cmd << "WEIGHTS";

    for (auto weight : weights) {
      cmd << fun::ToString(weight);
    }
  }

  // aggregate method
  if (method != AggregateMethod::server_default) {
    cmd << "AGGREGATE";
    cmd << aggregate_method_to_string(method);
  }

  Send(cmd, reply_cb);
  return *this;
}



//
// Redis Commands
// Future-based
//

Future<Reply>
Client::Execute(const Function<Client&(const ReplyCallback&)>& f) {
  auto prms = tl::make_shared<tl::promise<Reply>>();

  f([prms](Reply& reply) {
    prms->set_value(reply);
  });

  return prms->get_future();
}

Future<Reply>
Client::Send(const Array<String>& redis_cmd) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return Send(redis_cmd, cb); });
}

Future<Reply>
Client::append(const String& key, const String& value) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return append(key, value, cb); });
}

Future<Reply>
Client::auth(const String& password) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return auth(password, cb); });
}

Future<Reply>
Client::bgrewriteaof() {
  return Execute([=](const ReplyCallback& cb) -> Client& { return bgrewriteaof(cb); });
}

Future<Reply>
Client::bgsave() {
  return Execute([=](const ReplyCallback& cb) -> Client& { return bgsave(cb); });
}

Future<Reply>
Client::bitcount(const String& key) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return bitcount(key, cb); });
}

Future<Reply>
Client::bitcount(const String& key, int start, int end) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return bitcount(key, start, end, cb); });
}

Future<Reply>
Client::bitfield(const String& key, const Array<BitfieldOperation>& operations) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return bitfield(key, operations, cb); });
}

Future<Reply>
Client::bitop(const String& operation, const String& destkey, const Array<String>& keys) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return bitop(operation, destkey, keys, cb); });
}

Future<Reply>
Client::bitpos(const String& key, int bit) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return bitpos(key, bit, cb); });
}

Future<Reply>
Client::bitpos(const String& key, int bit, int start) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return bitpos(key, bit, start, cb); });
}

Future<Reply>
Client::bitpos(const String& key, int bit, int start, int end) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return bitpos(key, bit, start, end, cb); });
}

Future<Reply>
Client::blpop(const Array<String>& keys, int timeout) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return blpop(keys, timeout, cb); });
}

Future<Reply>
Client::brpop(const Array<String>& keys, int timeout) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return brpop(keys, timeout, cb); });
}

Future<Reply>
Client::brpoplpush(const String& src, const String& dst, int timeout) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return brpoplpush(src, dst, timeout, cb); });
}

Future<Reply>
Client::client_list() {
  return Execute([=](const ReplyCallback& cb) -> Client& { return client_list(cb); });
}

Future<Reply>
Client::client_getname() {
  return Execute([=](const ReplyCallback& cb) -> Client& { return client_getname(cb); });
}

Future<Reply>
Client::client_pause(int timeout) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return client_pause(timeout, cb); });
}

Future<Reply>
Client::client_reply(const String& mode) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return client_reply(mode, cb); });
}

Future<Reply>
Client::client_setname(const String& name) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return client_setname(name, cb); });
}

Future<Reply>
Client::cluster_addslots(const Array<String>& p_slots) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return cluster_addslots(p_slots, cb); });
}

Future<Reply>
Client::cluster_count_failure_reports(const String& node_id) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return cluster_count_failure_reports(node_id, cb); });
}

Future<Reply>
Client::cluster_countkeysinslot(const String& slot) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return cluster_countkeysinslot(slot, cb); });
}

Future<Reply>
Client::cluster_delslots(const Array<String>& p_slots) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return cluster_delslots(p_slots, cb); });
}

Future<Reply>
Client::cluster_failover() {
  return Execute([=](const ReplyCallback& cb) -> Client& { return cluster_failover(cb); });
}

Future<Reply>
Client::cluster_failover(const String& mode) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return cluster_failover(mode, cb); });
}

Future<Reply>
Client::cluster_forget(const String& node_id) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return cluster_forget(node_id, cb); });
}

Future<Reply>
Client::cluster_getkeysinslot(const String& slot, int count) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return cluster_getkeysinslot(slot, count, cb); });
}

Future<Reply>
Client::cluster_info() {
  return Execute([=](const ReplyCallback& cb) -> Client& { return cluster_info(cb); });
}

Future<Reply>
Client::cluster_keyslot(const String& key) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return cluster_keyslot(key, cb); });
}

Future<Reply>
Client::cluster_meet(const String& ip, int port) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return cluster_meet(ip, port, cb); });
}

Future<Reply>
Client::cluster_nodes() {
  return Execute([=](const ReplyCallback& cb) -> Client& { return cluster_nodes(cb); });
}

Future<Reply>
Client::cluster_replicate(const String& node_id) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return cluster_replicate(node_id, cb); });
}

Future<Reply>
Client::cluster_reset(const String& mode) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return cluster_reset(mode, cb); });
}

Future<Reply>
Client::cluster_saveconfig() {
  return Execute([=](const ReplyCallback& cb) -> Client& { return cluster_saveconfig(cb); });
}

Future<Reply>
Client::cluster_set_config_epoch(const String& epoch) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return cluster_set_config_epoch(epoch, cb); });
}

Future<Reply>
Client::cluster_setslot(const String& slot, const String& mode) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return cluster_setslot(slot, mode, cb); });
}

Future<Reply>
Client::cluster_setslot(const String& slot, const String& mode, const String& node_id) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return cluster_setslot(slot, mode, node_id, cb); });
}

Future<Reply>
Client::cluster_slaves(const String& node_id) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return cluster_slaves(node_id, cb); });
}

Future<Reply>
Client::cluster_slots() {
  return Execute([=](const ReplyCallback& cb) -> Client& { return cluster_slots(cb); });
}

Future<Reply>
Client::command() {
  return Execute([=](const ReplyCallback& cb) -> Client& { return command(cb); });
}

Future<Reply>
Client::command_count() {
  return Execute([=](const ReplyCallback& cb) -> Client& { return command_count(cb); });
}

Future<Reply>
Client::command_getkeys() {
  return Execute([=](const ReplyCallback& cb) -> Client& { return command_getkeys(cb); });
}

Future<Reply>
Client::command_info(const Array<String>& command_name) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return command_info(command_name, cb); });
}

Future<Reply>
Client::config_get(const String& param) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return config_get(param, cb); });
}

Future<Reply>
Client::config_rewrite() {
  return Execute([=](const ReplyCallback& cb) -> Client& { return config_rewrite(cb); });
}

Future<Reply>
Client::config_set(const String& param, const String& val) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return config_set(param, val, cb); });
}

Future<Reply>
Client::config_resetstat() {
  return Execute([=](const ReplyCallback& cb) -> Client& { return config_resetstat(cb); });
}

Future<Reply>
Client::dbsize() {
  return Execute([=](const ReplyCallback& cb) -> Client& { return dbsize(cb); });
}

Future<Reply>
Client::debug_object(const String& key) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return debug_object(key, cb); });
}

Future<Reply>
Client::debug_segfault() {
  return Execute([=](const ReplyCallback& cb) -> Client& { return debug_segfault(cb); });
}

Future<Reply>
Client::decr(const String& key) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return decr(key, cb); });
}

Future<Reply>
Client::decrby(const String& key, int val) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return decrby(key, val, cb); });
}

Future<Reply>
Client::del(const Array<String>& key) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return del(key, cb); });
}

Future<Reply>
Client::discard() {
  return Execute([=](const ReplyCallback& cb) -> Client& { return discard(cb); });
}

Future<Reply>
Client::dump(const String& key) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return dump(key, cb); });
}

Future<Reply>
Client::echo(const String& msg) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return echo(msg, cb); });
}

Future<Reply>
Client::eval(const String& script, int numkeys, const Array<String>& keys, const Array<String>& args) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return eval(script, numkeys, keys, args, cb); });
}

Future<Reply>
Client::evalsha(const String& sha1, int numkeys, const Array<String>& keys, const Array<String>& args) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return evalsha(sha1, numkeys, keys, args, cb); });
}

Future<Reply>
Client::exec() {
  return Execute([=](const ReplyCallback& cb) -> Client& { return exec(cb); });
}

Future<Reply>
Client::exists(const Array<String>& keys) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return exists(keys, cb); });
}

Future<Reply>
Client::expire(const String& key, int seconds) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return expire(key, seconds, cb); });
}

Future<Reply>
Client::expireat(const String& key, int timestamp) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return expireat(key, timestamp, cb); });
}

Future<Reply>
Client::flushall() {
  return Execute([=](const ReplyCallback& cb) -> Client& { return flushall(cb); });
}

Future<Reply>
Client::flushdb() {
  return Execute([=](const ReplyCallback& cb) -> Client& { return flushdb(cb); });
}

Future<Reply>
Client::geoadd(const String& key, const Array<tl::tuple<String, String, String>>& long_lat_memb) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return geoadd(key, long_lat_memb, cb); });
}

Future<Reply>
Client::geohash(const String& key, const Array<String>& members) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return geohash(key, members, cb); });
}

Future<Reply>
Client::geopos(const String& key, const Array<String>& members) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return geopos(key, members, cb); });
}

Future<Reply>
Client::geodist(const String& key, const String& member_1, const String& member_2, const String& unit) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return geodist(key, member_1, member_2, unit, cb); });
}

Future<Reply>
Client::georadius(const String& key, double longitude, double latitude, double radius, GeoUnit unit, bool with_coord, bool with_dist, bool with_hash, bool asc_order, int32 count, const String& store_key, const String& storedist_key) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return georadius(key, longitude, latitude, radius, unit, with_coord, with_dist, with_hash, asc_order, count, store_key, storedist_key, cb); });
}

Future<Reply>
Client::georadiusbymember(const String& key, const String& member, double radius, GeoUnit unit, bool with_coord, bool with_dist, bool with_hash, bool asc_order, int32 count, const String& store_key, const String& storedist_key) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return georadiusbymember(key, member, radius, unit, with_coord, with_dist, with_hash, asc_order, count, store_key, storedist_key, cb); });
}

Future<Reply>
Client::get(const String& key) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return get(key, cb); });
}

Future<Reply>
Client::getbit(const String& key, int offset) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return getbit(key, offset, cb); });
}

Future<Reply>
Client::getrange(const String& key, int start, int end) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return getrange(key, start, end, cb); });
}

Future<Reply>
Client::getset(const String& key, const String& val) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return getset(key, val, cb); });
}

Future<Reply>
Client::hdel(const String& key, const Array<String>& fields) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return hdel(key, fields, cb); });
}

Future<Reply>
Client::hexists(const String& key, const String& field) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return hexists(key, field, cb); });
}

Future<Reply>
Client::hget(const String& key, const String& field) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return hget(key, field, cb); });
}

Future<Reply>
Client::hgetall(const String& key) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return hgetall(key, cb); });
}

Future<Reply>
Client::hincrby(const String& key, const String& field, int incr) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return hincrby(key, field, incr, cb); });
}

Future<Reply>
Client::hincrbyfloat(const String& key, const String& field, float incr) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return hincrbyfloat(key, field, incr, cb); });
}

Future<Reply>
Client::hkeys(const String& key) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return hkeys(key, cb); });
}

Future<Reply>
Client::hlen(const String& key) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return hlen(key, cb); });
}

Future<Reply>
Client::hmget(const String& key, const Array<String>& fields) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return hmget(key, fields, cb); });
}

Future<Reply>
Client::hmset(const String& key, const Array<Pair<String, String>>& field_val) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return hmset(key, field_val, cb); });
}

Future<Reply>
Client::hscan(const String& key, int32 cursor) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return hscan(key, cursor, cb); });
}

Future<Reply>
Client::hscan(const String& key, int32 cursor, const String& pattern) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return hscan(key, cursor, pattern, cb); });
}

Future<Reply>
Client::hscan(const String& key, int32 cursor, int32 count) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return hscan(key, cursor, count, cb); });
}

Future<Reply>
Client::hscan(const String& key, int32 cursor, const String& pattern, int32 count) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return hscan(key, cursor, pattern, count, cb); });
}

Future<Reply>
Client::hset(const String& key, const String& field, const String& value) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return hset(key, field, value, cb); });
}

Future<Reply>
Client::hsetnx(const String& key, const String& field, const String& value) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return hsetnx(key, field, value, cb); });
}

Future<Reply>
Client::hstrlen(const String& key, const String& field) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return hstrlen(key, field, cb); });
}

Future<Reply>
Client::hvals(const String& key) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return hvals(key, cb); });
}

Future<Reply>
Client::incr(const String& key) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return incr(key, cb); });
}

Future<Reply>
Client::incrby(const String& key, int incr) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return incrby(key, incr, cb); });
}

Future<Reply>
Client::incrbyfloat(const String& key, float incr) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return incrbyfloat(key, incr, cb); });
}

Future<Reply>
Client::info(const String& section) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return info(section, cb); });
}

Future<Reply>
Client::keys(const String& pattern) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return keys(pattern, cb); });
}

Future<Reply>
Client::lastsave() {
  return Execute([=](const ReplyCallback& cb) -> Client& { return lastsave(cb); });
}

Future<Reply>
Client::lindex(const String& key, int index) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return lindex(key, index, cb); });
}

Future<Reply>
Client::linsert(const String& key, const String& before_after, const String& pivot, const String& value) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return linsert(key, before_after, pivot, value, cb); });
}

Future<Reply>
Client::llen(const String& key) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return llen(key, cb); });
}

Future<Reply>
Client::lpop(const String& key) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return lpop(key, cb); });
}

Future<Reply>
Client::lpush(const String& key, const Array<String>& values) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return lpush(key, values, cb); });
}

Future<Reply>
Client::lpushx(const String& key, const String& value) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return lpushx(key, value, cb); });
}

Future<Reply>
Client::lrange(const String& key, int start, int stop) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return lrange(key, start, stop, cb); });
}

Future<Reply>
Client::lrem(const String& key, int count, const String& value) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return lrem(key, count, value, cb); });
}

Future<Reply>
Client::lset(const String& key, int index, const String& value) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return lset(key, index, value, cb); });
}

Future<Reply>
Client::ltrim(const String& key, int start, int stop) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return ltrim(key, start, stop, cb); });
}

Future<Reply>
Client::mget(const Array<String>& keys) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return mget(keys, cb); });
}

Future<Reply>
Client::migrate(const String& host, int port, const String& key, const String& dest_db, int timeout, bool copy, bool replace, const Array<String>& keys) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return migrate(host, port, key, dest_db, timeout, copy, replace, keys, cb); });
}

Future<Reply>
Client::monitor() {
  return Execute([=](const ReplyCallback& cb) -> Client& { return monitor(cb); });
}

Future<Reply>
Client::move(const String& key, const String& db) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return move(key, db, cb); });
}

Future<Reply>
Client::mset(const Array<Pair<String, String>>& key_vals) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return mset(key_vals, cb); });
}

Future<Reply>
Client::msetnx(const Array<Pair<String, String>>& key_vals) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return msetnx(key_vals, cb); });
}

Future<Reply>
Client::multi() {
  return Execute([=](const ReplyCallback& cb) -> Client& { return multi(cb); });
}

Future<Reply>
Client::object(const String& subcommand, const Array<String>& args) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return object(subcommand, args, cb); });
}

Future<Reply>
Client::persist(const String& key) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return persist(key, cb); });
}

Future<Reply>
Client::pexpire(const String& key, int milliseconds) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return pexpire(key, milliseconds, cb); });
}

Future<Reply>
Client::pexpireat(const String& key, int milliseconds_timestamp) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return pexpireat(key, milliseconds_timestamp, cb); });
}

Future<Reply>
Client::pfadd(const String& key, const Array<String>& elements) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return pfadd(key, elements, cb); });
}

Future<Reply>
Client::pfcount(const Array<String>& keys) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return pfcount(keys, cb); });
}

Future<Reply>
Client::pfmerge(const String& destkey, const Array<String>& sourcekeys) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return pfmerge(destkey, sourcekeys, cb); });
}

Future<Reply>
Client::ping() {
  return Execute([=](const ReplyCallback& cb) -> Client& { return ping(cb); });
}

Future<Reply>
Client::ping(const String& message) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return ping(message, cb); });
}

Future<Reply>
Client::psetex(const String& key, int milliseconds, const String& val) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return psetex(key, milliseconds, val, cb); });
}

Future<Reply>
Client::publish(const String& channel, const String& message) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return publish(channel, message, cb); });
}

Future<Reply>
Client::pubsub(const String& subcommand, const Array<String>& args) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return pubsub(subcommand, args, cb); });
}

Future<Reply>
Client::pttl(const String& key) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return pttl(key, cb); });
}

Future<Reply>
Client::quit() {
  return Execute([=](const ReplyCallback& cb) -> Client& { return quit(cb); });
}

Future<Reply>
Client::randomkey() {
  return Execute([=](const ReplyCallback& cb) -> Client& { return randomkey(cb); });
}

Future<Reply>
Client::readonly() {
  return Execute([=](const ReplyCallback& cb) -> Client& { return readonly(cb); });
}

Future<Reply>
Client::readwrite() {
  return Execute([=](const ReplyCallback& cb) -> Client& { return readwrite(cb); });
}

Future<Reply>
Client::rename(const String& key, const String& newkey) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return rename(key, newkey, cb); });
}

Future<Reply>
Client::renamenx(const String& key, const String& newkey) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return renamenx(key, newkey, cb); });
}

Future<Reply>
Client::restore(const String& key, int ttl, const String& serialized_value) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return restore(key, ttl, serialized_value, cb); });
}

Future<Reply>
Client::restore(const String& key, int ttl, const String& serialized_value, const String& replace) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return restore(key, ttl, serialized_value, replace, cb); });
}

Future<Reply>
Client::role() {
  return Execute([=](const ReplyCallback& cb) -> Client& { return role(cb); });
}

Future<Reply>
Client::rpop(const String& key) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return rpop(key, cb); });
}

Future<Reply>
Client::rpoplpush(const String& src, const String& dst) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return rpoplpush(src, dst, cb); });
}

Future<Reply>
Client::rpush(const String& key, const Array<String>& values) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return rpush(key, values, cb); });
}

Future<Reply>
Client::rpushx(const String& key, const String& value) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return rpushx(key, value, cb); });
}

Future<Reply>
Client::sadd(const String& key, const Array<String>& members) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return sadd(key, members, cb); });
}

Future<Reply>
Client::scan(int32 cursor) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return scan(cursor, cb); });
}

Future<Reply>
Client::scan(int32 cursor, const String& pattern) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return scan(cursor, pattern, cb); });
}

Future<Reply>
Client::scan(int32 cursor, int32 count) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return scan(cursor, count, cb); });
}

Future<Reply>
Client::scan(int32 cursor, const String& pattern, int32 count) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return scan(cursor, pattern, count, cb); });
}

Future<Reply>
Client::save() {
  return Execute([=](const ReplyCallback& cb) -> Client& { return save(cb); });
}

Future<Reply>
Client::scard(const String& key) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return scard(key, cb); });
}

Future<Reply>
Client::script_debug(const String& mode) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return script_debug(mode, cb); });
}

Future<Reply>
Client::script_exists(const Array<String>& scripts) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return script_exists(scripts, cb); });
}

Future<Reply>
Client::script_flush() {
  return Execute([=](const ReplyCallback& cb) -> Client& { return script_flush(cb); });
}

Future<Reply>
Client::script_kill() {
  return Execute([=](const ReplyCallback& cb) -> Client& { return script_kill(cb); });
}

Future<Reply>
Client::script_load(const String& script) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return script_load(script, cb); });
}

Future<Reply>
Client::sdiff(const Array<String>& keys) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return sdiff(keys, cb); });
}

Future<Reply>
Client::sdiffstore(const String& dst, const Array<String>& keys) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return sdiffstore(dst, keys, cb); });
}

Future<Reply>
Client::select(int index) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return select(index, cb); });
}

Future<Reply>
Client::set(const String& key, const String& value) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return set(key, value, cb); });
}

Future<Reply>
Client::set_advanced(const String& key, const String& value, bool ex, int ex_sec, bool px, int px_milli, bool nx, bool xx) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return set_advanced(key, value, ex, ex_sec, px, px_milli, nx, xx, cb); });
}

Future<Reply>
Client::setbit_(const String& key, int offset, const String& value) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return setbit_(key, offset, value, cb); });
}

Future<Reply>
Client::setex(const String& key, int seconds, const String& value) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return setex(key, seconds, value, cb); });
}

Future<Reply>
Client::setnx(const String& key, const String& value) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return setnx(key, value, cb); });
}

Future<Reply>
Client::setrange(const String& key, int offset, const String& value) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return setrange(key, offset, value, cb); });
}

Future<Reply>
Client::shutdown() {
  return Execute([=](const ReplyCallback& cb) -> Client& { return shutdown(cb); });
}

Future<Reply>
Client::shutdown(const String& save) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return shutdown(save, cb); });
}

Future<Reply>
Client::sinter(const Array<String>& keys) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return sinter(keys, cb); });
}

Future<Reply>
Client::sinterstore(const String& dst, const Array<String>& keys) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return sinterstore(dst, keys, cb); });
}

Future<Reply>
Client::sismember(const String& key, const String& member) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return sismember(key, member, cb); });
}

Future<Reply>
Client::slaveof(const String& host, int port) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return slaveof(host, port, cb); });
}

Future<Reply>
Client::slowlog(const String& subcommand) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return slowlog(subcommand, cb); });
}

Future<Reply>
Client::slowlog(const String& subcommand, const String& argument) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return slowlog(subcommand, argument, cb); });
}

Future<Reply>
Client::smembers(const String& key) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return smembers(key, cb); });
}

Future<Reply>
Client::smove(const String& src, const String& dst, const String& member) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return smove(src, dst, member, cb); });
}

Future<Reply>
Client::sort(const String& key) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return sort(key, cb); });
}

Future<Reply>
Client::sort(const String& key, const Array<String>& get_patterns, bool asc_order, bool alpha) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return sort(key, get_patterns, asc_order, alpha, cb); });
}

Future<Reply>
Client::sort(const String& key, int32 offset, int32 count, const Array<String>& get_patterns, bool asc_order, bool alpha) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return sort(key, offset, count, get_patterns, asc_order, alpha, cb); });
}

Future<Reply>
Client::sort(const String& key, const String& by_pattern, const Array<String>& get_patterns, bool asc_order, bool alpha) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return sort(key, by_pattern, get_patterns, asc_order, alpha, cb); });
}

Future<Reply>
Client::sort(const String& key, const Array<String>& get_patterns, bool asc_order, bool alpha, const String& store_dest) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return sort(key, get_patterns, asc_order, alpha, store_dest, cb); });
}

Future<Reply>
Client::sort(const String& key, int32 offset, int32 count, const Array<String>& get_patterns, bool asc_order, bool alpha, const String& store_dest) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return sort(key, offset, count, get_patterns, asc_order, alpha, store_dest, cb); });
}

Future<Reply>
Client::sort(const String& key, const String& by_pattern, const Array<String>& get_patterns, bool asc_order, bool alpha, const String& store_dest) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return sort(key, by_pattern, get_patterns, asc_order, alpha, store_dest, cb); });
}

Future<Reply>
Client::sort(const String& key, const String& by_pattern, int32 offset, int32 count, const Array<String>& get_patterns, bool asc_order, bool alpha) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return sort(key, by_pattern, offset, count, get_patterns, asc_order, alpha, cb); });
}

Future<Reply>
Client::sort(const String& key, const String& by_pattern, int32 offset, int32 count, const Array<String>& get_patterns, bool asc_order, bool alpha, const String& store_dest) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return sort(key, by_pattern, offset, count, get_patterns, asc_order, alpha, store_dest, cb); });
}

Future<Reply>
Client::spop(const String& key) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return spop(key, cb); });
}

Future<Reply>
Client::spop(const String& key, int count) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return spop(key, count, cb); });
}

Future<Reply>
Client::srandmember(const String& key) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return srandmember(key, cb); });
}

Future<Reply>
Client::srandmember(const String& key, int count) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return srandmember(key, count, cb); });
}

Future<Reply>
Client::srem(const String& key, const Array<String>& members) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return srem(key, members, cb); });
}

Future<Reply>
Client::sscan(const String& key, int32 cursor) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return sscan(key, cursor, cb); });
}

Future<Reply>
Client::sscan(const String& key, int32 cursor, const String& pattern) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return sscan(key, cursor, pattern, cb); });
}

Future<Reply>
Client::sscan(const String& key, int32 cursor, int32 count) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return sscan(key, cursor, count, cb); });
}

Future<Reply>
Client::sscan(const String& key, int32 cursor, const String& pattern, int32 count) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return sscan(key, cursor, pattern, count, cb); });
}

Future<Reply>
Client::strlen(const String& key) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return strlen(key, cb); });
}

Future<Reply>
Client::sunion(const Array<String>& keys) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return sunion(keys, cb); });
}

Future<Reply>
Client::sunionstore(const String& dst, const Array<String>& keys) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return sunionstore(dst, keys, cb); });
}

Future<Reply>
Client::sync() {
  return Execute([=](const ReplyCallback& cb) -> Client& { return sync(cb); });
}

Future<Reply>
Client::time() {
  return Execute([=](const ReplyCallback& cb) -> Client& { return time(cb); });
}

Future<Reply>
Client::ttl(const String& key) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return ttl(key, cb); });
}

Future<Reply>
Client::Type(const String& key) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return type(key, cb); });
}

Future<Reply>
Client::unwatch() {
  return Execute([=](const ReplyCallback& cb) -> Client& { return unwatch(cb); });
}

Future<Reply>
Client::wait(int numslaves, int timeout) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return wait(numslaves, timeout, cb); });
}

Future<Reply>
Client::watch(const Array<String>& keys) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return watch(keys, cb); });
}

Future<Reply>
Client::zadd(const String& key, const Array<String>& options, const tl::multimap<String, String>& score_members) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zadd(key, options, score_members, cb); });
}

Future<Reply>
Client::zcard(const String& key) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zcard(key, cb); });
}

Future<Reply>
Client::zcount(const String& key, int min, int max) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zcount(key, min, max, cb); });
}

Future<Reply>
Client::zcount(const String& key, double min, double max) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zcount(key, min, max, cb); });
}

Future<Reply>
Client::zcount(const String& key, const String& min, const String& max) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zcount(key, min, max, cb); });
}

Future<Reply>
Client::zincrby(const String& key, int incr, const String& member) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zincrby(key, incr, member, cb); });
}

Future<Reply>
Client::zincrby(const String& key, double incr, const String& member) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zincrby(key, incr, member, cb); });
}

Future<Reply>
Client::zincrby(const String& key, const String& incr, const String& member) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zincrby(key, incr, member, cb); });
}

Future<Reply>
Client::zinterstore(const String& destination, int32 numkeys, const Array<String>& keys, const Array<int32> weights, AggregateMethod method) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zinterstore(destination, numkeys, keys, weights, method, cb); });
}

Future<Reply>
Client::zlexcount(const String& key, int min, int max) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zlexcount(key, min, max, cb); });
}

Future<Reply>
Client::zlexcount(const String& key, double min, double max) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zlexcount(key, min, max, cb); });
}

Future<Reply>
Client::zlexcount(const String& key, const String& min, const String& max) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zlexcount(key, min, max, cb); });
}

Future<Reply>
Client::zrange(const String& key, int start, int stop, bool withscores) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zrange(key, start, stop, withscores, cb); });
}

Future<Reply>
Client::zrange(const String& key, double start, double stop, bool withscores) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zrange(key, start, stop, withscores, cb); });
}

Future<Reply>
Client::zrange(const String& key, const String& start, const String& stop, bool withscores) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zrange(key, start, stop, withscores, cb); });
}

Future<Reply>
Client::zrangebylex(const String& key, int min, int max, bool withscores) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zrangebylex(key, min, max, withscores, cb); });
}

Future<Reply>
Client::zrangebylex(const String& key, double min, double max, bool withscores) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zrangebylex(key, min, max, withscores, cb); });
}

Future<Reply>
Client::zrangebylex(const String& key, const String& min, const String& max, bool withscores) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zrangebylex(key, min, max, withscores, cb); });
}

Future<Reply>
Client::zrangebylex(const String& key, int min, int max, int32 offset, int32 count, bool withscores) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zrangebylex(key, min, max, offset, count, withscores, cb); });
}

Future<Reply>
Client::zrangebylex(const String& key, double min, double max, int32 offset, int32 count, bool withscores) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zrangebylex(key, min, max, offset, count, withscores, cb); });
}

Future<Reply>
Client::zrangebylex(const String& key, const String& min, const String& max, int32 offset, int32 count, bool withscores) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zrangebylex(key, min, max, offset, count, withscores, cb); });
}

Future<Reply>
Client::zrangebyscore(const String& key, int min, int max, bool withscores) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zrangebyscore(key, min, max, withscores, cb); });
}

Future<Reply>
Client::zrangebyscore(const String& key, double min, double max, bool withscores) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zrangebyscore(key, min, max, withscores, cb); });
}

Future<Reply>
Client::zrangebyscore(const String& key, const String& min, const String& max, bool withscores) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zrangebyscore(key, min, max, withscores, cb); });
}

Future<Reply>
Client::zrangebyscore(const String& key, int min, int max, int32 offset, int32 count, bool withscores) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zrangebyscore(key, min, max, offset, count, withscores, cb); });
}

Future<Reply>
Client::zrangebyscore(const String& key, double min, double max, int32 offset, int32 count, bool withscores) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zrangebyscore(key, min, max, offset, count, withscores, cb); });
}

Future<Reply>
Client::zrangebyscore(const String& key, const String& min, const String& max, int32 offset, int32 count, bool withscores) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zrangebyscore(key, min, max, offset, count, withscores, cb); });
}

Future<Reply>
Client::zrank(const String& key, const String& member) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zrank(key, member, cb); });
}

Future<Reply>
Client::zrem(const String& key, const Array<String>& members) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zrem(key, members, cb); });
}

Future<Reply>
Client::zremrangebylex(const String& key, int min, int max) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zremrangebylex(key, min, max, cb); });
}

Future<Reply>
Client::zremrangebylex(const String& key, double min, double max) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zremrangebylex(key, min, max, cb); });
}

Future<Reply>
Client::zremrangebylex(const String& key, const String& min, const String& max) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zremrangebylex(key, min, max, cb); });
}

Future<Reply>
Client::zremrangebyrank(const String& key, int start, int stop) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zremrangebyrank(key, start, stop, cb); });
}

Future<Reply>
Client::zremrangebyrank(const String& key, double start, double stop) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zremrangebyrank(key, start, stop, cb); });
}

Future<Reply>
Client::zremrangebyrank(const String& key, const String& start, const String& stop) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zremrangebyrank(key, start, stop, cb); });
}

Future<Reply>
Client::zremrangebyscore(const String& key, int min, int max) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zremrangebyscore(key, min, max, cb); });
}

Future<Reply>
Client::zremrangebyscore(const String& key, double min, double max) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zremrangebyscore(key, min, max, cb); });
}

Future<Reply>
Client::zremrangebyscore(const String& key, const String& min, const String& max) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zremrangebyscore(key, min, max, cb); });
}

Future<Reply>
Client::zrevrange(const String& key, int start, int stop, bool withscores) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zrevrange(key, start, stop, withscores, cb); });
}

Future<Reply>
Client::zrevrange(const String& key, double start, double stop, bool withscores) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zrevrange(key, start, stop, withscores, cb); });
}

Future<Reply>
Client::zrevrange(const String& key, const String& start, const String& stop, bool withscores) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zrevrange(key, start, stop, withscores, cb); });
}

Future<Reply>
Client::zrevrangebylex(const String& key, int max, int min, bool withscores) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zrevrangebylex(key, max, min, withscores, cb); });
}

Future<Reply>
Client::zrevrangebylex(const String& key, double max, double min, bool withscores) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zrevrangebylex(key, max, min, withscores, cb); });
}

Future<Reply>
Client::zrevrangebylex(const String& key, const String& max, const String& min, bool withscores) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zrevrangebylex(key, max, min, withscores, cb); });
}

Future<Reply>
Client::zrevrangebylex(const String& key, int max, int min, int32 offset, int32 count, bool withscores) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zrevrangebylex(key, max, min, offset, count, withscores, cb); });
}

Future<Reply>
Client::zrevrangebylex(const String& key, double max, double min, int32 offset, int32 count, bool withscores) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zrevrangebylex(key, max, min, offset, count, withscores, cb); });
}

Future<Reply>
Client::zrevrangebylex(const String& key, const String& max, const String& min, int32 offset, int32 count, bool withscores) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zrevrangebylex(key, max, min, offset, count, withscores, cb); });
}

Future<Reply>
Client::zrevrangebyscore(const String& key, int max, int min, bool withscores) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zrevrangebyscore(key, max, min, withscores, cb); });
}

Future<Reply>
Client::zrevrangebyscore(const String& key, double max, double min, bool withscores) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zrevrangebyscore(key, max, min, withscores, cb); });
}

Future<Reply>
Client::zrevrangebyscore(const String& key, const String& max, const String& min, bool withscores) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zrevrangebyscore(key, max, min, withscores, cb); });
}

Future<Reply>
Client::zrevrangebyscore(const String& key, int max, int min, int32 offset, int32 count, bool withscores) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zrevrangebyscore(key, max, min, offset, count, withscores, cb); });
}

Future<Reply>
Client::zrevrangebyscore(const String& key, double max, double min, int32 offset, int32 count, bool withscores) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zrevrangebyscore(key, max, min, offset, count, withscores, cb); });
}

Future<Reply>
Client::zrevrangebyscore(const String& key, const String& max, const String& min, int32 offset, int32 count, bool withscores) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zrevrangebyscore(key, max, min, offset, count, withscores, cb); });
}

Future<Reply>
Client::zrevrank(const String& key, const String& member) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zrevrank(key, member, cb); });
}

Future<Reply>
Client::zscan(const String& key, int32 cursor) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zscan(key, cursor, cb); });
}

Future<Reply>
Client::zscan(const String& key, int32 cursor, const String& pattern) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zscan(key, cursor, pattern, cb); });
}

Future<Reply>
Client::zscan(const String& key, int32 cursor, int32 count) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zscan(key, cursor, count, cb); });
}

Future<Reply>
Client::zscan(const String& key, int32 cursor, const String& pattern, int32 count) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zscan(key, cursor, pattern, count, cb); });
}

Future<Reply>
Client::zscore(const String& key, const String& member) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zscore(key, member, cb); });
}

Future<Reply>
Client::zunionstore(const String& destination,
                    int32 numkeys,
                    const Array<String>& keys,
                    const Array<int32> weights,
                    AggregateMethod method) {
  return Execute([=](const ReplyCallback& cb) -> Client& { return zunionstore(destination, numkeys, keys, weights, method, cb); });
}

} // namespace cpp_redis
