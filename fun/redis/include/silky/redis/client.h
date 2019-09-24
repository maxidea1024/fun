#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <Future>
#include <map>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

#include <cpp_redis/core/Sentinel.hpp>
#include <cpp_redis/helpers/variadic_template.hpp>
#include <cpp_redis/misc/logger.hpp>
#include <cpp_redis/network/redis_connection.hpp>
#include <cpp_redis/network/tcp_client_iface.hpp>


namespace fun {
namespace redis {


//!
/// cpp_redis::Client is the class providing communication with a Redis server.
/// It is meant to be used for sending commands to the remote server and receiving its replies.
/// The Client support asynchronous requests, as well as synchronous ones. Moreover, commands pipelining is supported.
//!
class Client {
public:
  /// Client type
  /// used for Client kill
  enum class ClientType
  {
    normal,
    master,
    pubsub,
    slave
  };

  /// high availability (re)connection states
  ///  * dropped: connection has dropped
  ///  * start: attempt of connection has started
  ///  * sleeping: sleep between two attempts
  ///  * ok: connected
  ///  * failed: failed to connect
  ///  * lookup failed: failed to retrieve master Sentinel
  ///  * stopped: stop to try to Reconnect
  enum class ConnectState
  {
    dropped,
    start,
    sleeping,
    ok,
    failed,
    lookup_failed,
    stopped
  };

public:
  /// ctor
  Client();

  /// custom ctor to specify custom tcp_client
  /// \param tcp_client tcp Client to be used for network communications
  explicit Client(const SharedPtr<network::tcp_client_iface>& tcp_client);

  /// dtor
  ~Client();

  /// copy ctor
  Client(const Client&) = delete;
  /// assignment operator
  Client& operator=(const Client&) = delete;

public:
  /// connect handler, called whenever a new connection even occurred
  typedef Function<void(const String& host, int32 port, ConnectState status)> ConnectCallback;

  /// Connect to redis server
  /// \param host host to be connected to
  /// \param port port to be connected to
  /// \param connect_callback connect handler to be called on connect events (may be null)
  /// \param timeout_msecs maximum time to connect
  /// \param max_reconnects maximum attempts of reconnection if connection dropped
  /// \param reconnect_interval_msecs time between two attempts of reconnection
  void Connect( const String& host = "127.0.0.1",
                int32 port = 6379,
                const ConnectCallback& connect_callback = nullptr,
                uint32 timeout_msecs = 0,
                int32 max_reconnects = 0,
                uint32 reconnect_interval_msecs = 0);

  /// Connect to redis server
  /// \param name Sentinel name
  /// \param connect_callback connect handler to be called on connect events (may be null)
  /// \param timeout_msecs maximum time to connect
  /// \param max_reconnects maximum attempts of reconnection if connection dropped
  /// \param reconnect_interval_msecs time between two attempts of reconnection
  void Connect( const String& name,
                const ConnectCallback& connect_callback = nullptr,
                uint32 timeout_msecs = 0,
                int32 max_reconnects = 0,
                uint32 reconnect_interval_msecs = 0);

  /// \return whether we are connected to the redis server
  bool IsConnected() const;

  /// disconnect from redis server
  /// \param wait_for_removal when sets to true, disconnect blocks until the underlying
  ///      TCP Client has been effectively removed from the io_service and
  ///      that all the underlying callbacks have completed.
  void Disconnect(bool wait_for_removal = false);

  /// \return whether an attempt to Reconnect is in progress
  bool IsReconnecting() const;

  /// stop any Reconnect in progress
  void CancelReconnect();

public:
  /// reply callback called whenever a reply is received
  /// takes as parameter the received reply
  typedef Function<void(Reply&)> ReplyCallback;

  /// Send the given command
  /// the command is actually pipelined and only buffered, so nothing is sent to the network
  /// please call Commit() / SyncCommit() to flush the buffer
  /// \param redis_cmd command to be sent
  /// \param callback callback to be called on received reply
  /// \return current instance
  Client& Send(const Array<String>& redis_cmd, const ReplyCallback& callback);

  /// same as the other Send method
  /// but Future based: does not take any callback and return an tl:;Future to handle the reply
  /// \param redis_cmd command to be sent
  /// \return Future to handler redis reply
  Future<Reply> Send(const Array<String>& redis_cmd);

  /// Sends all the commands that have been stored by calling Send() since the last Commit() call to the redis server.
  /// That is, pipelining is supported in a very simple and efficient way: Client.Send(...).Send(...).Send(...).Commit() will Send the 3 commands at once (instead of sending 3 network requests, one for each command, as it would have been done without pipelining).
  /// Pipelined commands are always removed from the buffer, even in the case of an error (for example, calling Commit while the Client is not connected, something that throws an exception).
  /// Commit() works asynchronously: it returns immediately after sending the queued requests and replies are processed asynchronously.
  /// Please note that, while Commit() can safely be called from inside a reply callback, calling SyncCommit() from inside a reply callback is not permitted and will lead to undefined behavior, mostly deadlock.
  Client& Commit();

  /// same as Commit(), but synchronous
  /// will block until all pending commands have been sent and that a reply has been received for each of them and all underlying callbacks completed
  /// \return current instance
  Client& SyncCommit();

  /// same as SyncCommit, but with a timeout
  /// will simply block until it completes or timeout expires
  /// \return current instance
  template <class Rep, class Period>
  Client& SyncCommit(const tl::chrono::duration<Rep, Period>& timeout)
  {
    /// no need to call Commit in case of reconnection
    /// the reconnection flow will do it for us
    if (!IsReconnecting())
    {
      TryCommit();
    }

    tl::unique_lock<FastMutex> lock_callback(callbacks_mutex_);
    __CPP_REDIS_LOG(debug, "cpp_redis::Client waiting for callbacks to complete");
    if (!sync_cv_.wait_for(lock_callback, timeout, [=] { return callbacks_running_ == 0 && commands_.empty(); })) {
      __CPP_REDIS_LOG(debug, "cpp_redis::Client finished waiting for callback");
    }
    else {
      __CPP_REDIS_LOG(debug, "cpp_redis::Client timed out waiting for callback");
    }

    return *this;
  }

private:
  /// \return whether a reconnection attempt should be performed
  bool SouldReconnect() const;

  /// resend all pending commands that failed to be sent due to disconnection
  void ResendUnsentCommands();

  /// sleep between two Reconnect attempts if necessary
  void SleepBeforeNextReconnectAttempt();

  /// Reconnect to the previously connected host
  /// automatically re authenticate and resubscribe to subscribed channel in case of success
  void Reconnect();

  /// re authenticate to redis server based on previously used password
  void ReAuth();

  /// re select db to redis server based on previously selected db
  void ReSelect();

private:
  /// unprotected Send
  /// same as Send, but without any mutex lock
  /// \param redis_cmd cmd to be sent
  /// \param callback callback to be called whenever a reply is received
  void UnprotectedSend(const Array<String>& redis_cmd, const ReplyCallback& callback);

  /// unprotected auth
  /// same as auth, but without any mutex lock
  /// \param password password to be used for authentication
  /// \param reply_cb callback to be called whenever a reply is received
  void UnprotectedAuth(const String& password, const ReplyCallback& reply_cb);

  /// unprotected select
  /// same as select, but without any mutex lock
  /// \param index index to be used for db select
  /// \param reply_cb callback to be called whenever a reply is received
  void UnprotectedSelect(int index, const ReplyCallback& reply_cb);

public:
  /// add a Sentinel definition. Required for connect() or get_master_addr_by_name() when autoconnect is enabled.
  /// \param host Sentinel host
  /// \param port Sentinel port
  /// \param timeout_msecs maximum time to connect
  void AddSentinel(const String& host, int32 port, uint32 timeout_msecs = 0);

  /// retrieve Sentinel for current Client
  /// \return Sentinel associated to current Client
  const Sentinel& GetSentinel() const;

  /// retrieve Sentinel for current Client
  /// non-const version
  /// \return Sentinel associated to current Client
  Sentinel& GetSentinel();

  /// clear all existing sentinels.
  void ClearSentinels();

public:
  /// aggregate method to be used for some commands (like zunionstore)
  /// these match the aggregate methods supported by redis
  /// use server_default if you are not willing to specify this parameter and let the server defaults
  enum class AggregateMethod
  {
    sum,
    min,
    max,
    server_default
  };

  /// convert an AggregateMethod enum to its equivalent redis-server string
  /// \param method AggregateMethod to convert
  /// \return conversion
  String aggregate_method_to_string(AggregateMethod method) const;

public:
  /// geographic unit to be used for some commands (like georadius)
  /// these match the geo units supported by redis-server
  enum class GeoUnit
  {
    m,
    km,
    ft,
    mi
  };

  /// convert a geo unit to its equivalent redis-server string
  /// \param unit GeoUnit to convert
  /// \return conversion
  String geo_unit_to_string(GeoUnit unit) const;

public:
  /// overflow type to be used for some commands (like bitfield)
  /// these match the overflow types supported by redis-server
  /// use server_default if you are not willing to specify this parameter and let the server defaults
  enum class OverflowType
  {
    wrap,
    sat,
    fail,
    server_default
  };

  /// convert an overflow type to its equivalent redis-server string
  /// \param type overflow type to convert
  /// \return conversion
  String overflow_type_to_string(OverflowType type) const;

public:
  /// bitfield operation type to be used for some commands (like bitfield)
  /// these match the bitfield operation types supported by redis-server
  enum class BitfieldOperationType
  {
    get,
    set,
    incrby
  };

  /// convert a bitfield operation type to its equivalent redis-server string
  /// \param operation operation type to convert
  /// \return conversion
  String bitfield_operation_type_to_string(BitfieldOperationType operation) const;

public:
  /// used to store a get, set or incrby bitfield operation (for bitfield command)
  struct BitfieldOperation
  {
    /// operation type (get, set, incrby)
    BitfieldOperationType operation_type;

    /// redis type parameter for get, set or incrby operations
    String type;

    /// redis offset parameter for get, set or incrby operations
    int offset;

    /// redis value parameter for set operation, or increment parameter for incrby operation
    int value;

    /// overflow optional specification
    OverflowType overflow;

    /// build a BitfieldOperation for a bitfield get operation
    /// \param type type param of a get operation
    /// \param offset offset param of a get operation
    /// \param overflow overflow specification (leave to server_default if you do not want to specify it)
    /// \return corresponding get BitfieldOperation
    static BitfieldOperation get(const String& type, int offset, OverflowType overflow = OverflowType::server_default);

    /// build a BitfieldOperation for a bitfield set operation
    /// \param type type param of a set operation
    /// \param offset offset param of a set operation
    /// \param value value param of a set operation
    /// \param overflow overflow specification (leave to server_default if you do not want to specify it)
    /// \return corresponding set BitfieldOperation
    static BitfieldOperation set(const String& type, int offset, int value, OverflowType overflow = OverflowType::server_default);

    /// build a BitfieldOperation for a bitfield incrby operation
    /// \param type type param of a incrby operation
    /// \param offset offset param of a incrby operation
    /// \param increment increment param of a incrby operation
    /// \param overflow overflow specification (leave to server_default if you do not want to specify it)
    /// \return corresponding incrby BitfieldOperation
    static BitfieldOperation incrby(const String& type, int offset, int increment, OverflowType overflow = OverflowType::server_default);
  };

public:
  Client& append(const String& key, const String& value, const ReplyCallback& reply_cb);
  Future<Reply> append(const String& key, const String& value);

  Client& auth(const String& password, const ReplyCallback& reply_cb);
  Future<Reply> auth(const String& password);

  Client& bgrewriteaof(const ReplyCallback& reply_cb);
  Future<Reply> bgrewriteaof();

  Client& bgsave(const ReplyCallback& reply_cb);
  Future<Reply> bgsave();

  Client& bitcount(const String& key, const ReplyCallback& reply_cb);
  Future<Reply> bitcount(const String& key);

  Client& bitcount(const String& key, int start, int end, const ReplyCallback& reply_cb);
  Future<Reply> bitcount(const String& key, int start, int end);

  Client& bitfield(const String& key, const Array<BitfieldOperation>& operations, const ReplyCallback& reply_cb);
  Future<Reply> bitfield(const String& key, const Array<BitfieldOperation>& operations);

  Client& bitop(const String& operation, const String& destkey, const Array<String>& keys, const ReplyCallback& reply_cb);
  Future<Reply> bitop(const String& operation, const String& destkey, const Array<String>& keys);

  Client& bitpos(const String& key, int bit, const ReplyCallback& reply_cb);
  Future<Reply> bitpos(const String& key, int bit);

  Client& bitpos(const String& key, int bit, int start, const ReplyCallback& reply_cb);
  Future<Reply> bitpos(const String& key, int bit, int start);

  Client& bitpos(const String& key, int bit, int start, int end, const ReplyCallback& reply_cb);
  Future<Reply> bitpos(const String& key, int bit, int start, int end);

  Client& blpop(const Array<String>& keys, int timeout, const ReplyCallback& reply_cb);
  Future<Reply> blpop(const Array<String>& keys, int timeout);

  Client& brpop(const Array<String>& keys, int timeout, const ReplyCallback& reply_cb);
  Future<Reply> brpop(const Array<String>& keys, int timeout);

  Client& brpoplpush(const String& src, const String& dst, int timeout, const ReplyCallback& reply_cb);
  Future<Reply> brpoplpush(const String& src, const String& dst, int timeout);

  template <typename T, typename... Ts>
  Client& client_kill(const String& host, int port, const T& arg, const Ts&... args);
  Client& client_kill(const String& host, int port);
  template <typename... Ts>
  Client& client_kill(const char* host, int port, const Ts&... args);
  template <typename T, typename... Ts>
  Client& client_kill(const T&, const Ts&...);
  template <typename T, typename... Ts>
  Future<Reply> client_kill_future(const T, const Ts...);

  Client& client_list(const ReplyCallback& reply_cb);
  Future<Reply> client_list();

  Client& client_getname(const ReplyCallback& reply_cb);
  Future<Reply> client_getname();

  Client& client_pause(int timeout, const ReplyCallback& reply_cb);
  Future<Reply> client_pause(int timeout);

  Client& client_reply(const String& mode, const ReplyCallback& reply_cb);
  Future<Reply> client_reply(const String& mode);

  Client& client_setname(const String& name, const ReplyCallback& reply_cb);
  Future<Reply> client_setname(const String& name);

  Client& cluster_addslots(const Array<String>& p_slots, const ReplyCallback& reply_cb);
  Future<Reply> cluster_addslots(const Array<String>& p_slots);

  Client& cluster_count_failure_reports(const String& node_id, const ReplyCallback& reply_cb);
  Future<Reply> cluster_count_failure_reports(const String& node_id);

  Client& cluster_countkeysinslot(const String& slot, const ReplyCallback& reply_cb);
  Future<Reply> cluster_countkeysinslot(const String& slot);

  Client& cluster_delslots(const Array<String>& p_slots, const ReplyCallback& reply_cb);
  Future<Reply> cluster_delslots(const Array<String>& p_slots);

  Client& cluster_failover(const ReplyCallback& reply_cb);
  Future<Reply> cluster_failover();

  Client& cluster_failover(const String& mode, const ReplyCallback& reply_cb);
  Future<Reply> cluster_failover(const String& mode);

  Client& cluster_forget(const String& node_id, const ReplyCallback& reply_cb);
  Future<Reply> cluster_forget(const String& node_id);

  Client& cluster_getkeysinslot(const String& slot, int count, const ReplyCallback& reply_cb);
  Future<Reply> cluster_getkeysinslot(const String& slot, int count);

  Client& cluster_info(const ReplyCallback& reply_cb);
  Future<Reply> cluster_info();

  Client& cluster_keyslot(const String& key, const ReplyCallback& reply_cb);
  Future<Reply> cluster_keyslot(const String& key);

  Client& cluster_meet(const String& ip, int port, const ReplyCallback& reply_cb);
  Future<Reply> cluster_meet(const String& ip, int port);

  Client& cluster_nodes(const ReplyCallback& reply_cb);
  Future<Reply> cluster_nodes();

  Client& cluster_replicate(const String& node_id, const ReplyCallback& reply_cb);
  Future<Reply> cluster_replicate(const String& node_id);

  Client& cluster_reset(const ReplyCallback& reply_cb);
  Client& cluster_reset(const String& mode, const ReplyCallback& reply_cb);
  Future<Reply> cluster_reset(const String& mode = "soft");

  Client& cluster_saveconfig(const ReplyCallback& reply_cb);
  Future<Reply> cluster_saveconfig();

  Client& cluster_set_config_epoch(const String& epoch, const ReplyCallback& reply_cb);
  Future<Reply> cluster_set_config_epoch(const String& epoch);

  Client& cluster_setslot(const String& slot, const String& mode, const ReplyCallback& reply_cb);
  Future<Reply> cluster_setslot(const String& slot, const String& mode);

  Client& cluster_setslot(const String& slot, const String& mode, const String& node_id, const ReplyCallback& reply_cb);
  Future<Reply> cluster_setslot(const String& slot, const String& mode, const String& node_id);

  Client& cluster_slaves(const String& node_id, const ReplyCallback& reply_cb);
  Future<Reply> cluster_slaves(const String& node_id);

  Client& cluster_slots(const ReplyCallback& reply_cb);
  Future<Reply> cluster_slots();

  Client& command(const ReplyCallback& reply_cb);
  Future<Reply> command();

  Client& command_count(const ReplyCallback& reply_cb);
  Future<Reply> command_count();

  Client& command_getkeys(const ReplyCallback& reply_cb);
  Future<Reply> command_getkeys();

  Client& command_info(const Array<String>& command_name, const ReplyCallback& reply_cb);
  Future<Reply> command_info(const Array<String>& command_name);

  Client& config_get(const String& param, const ReplyCallback& reply_cb);
  Future<Reply> config_get(const String& param);

  Client& config_rewrite(const ReplyCallback& reply_cb);
  Future<Reply> config_rewrite();

  Client& config_set(const String& param, const String& val, const ReplyCallback& reply_cb);
  Future<Reply> config_set(const String& param, const String& val);

  Client& config_resetstat(const ReplyCallback& reply_cb);
  Future<Reply> config_resetstat();

  Client& dbsize(const ReplyCallback& reply_cb);
  Future<Reply> dbsize();

  Client& debug_object(const String& key, const ReplyCallback& reply_cb);
  Future<Reply> debug_object(const String& key);

  Client& debug_segfault(const ReplyCallback& reply_cb);
  Future<Reply> debug_segfault();

  Client& decr(const String& key, const ReplyCallback& reply_cb);
  Future<Reply> decr(const String& key);

  Client& decrby(const String& key, int val, const ReplyCallback& reply_cb);
  Future<Reply> decrby(const String& key, int val);

  Client& del(const Array<String>& key, const ReplyCallback& reply_cb);
  Future<Reply> del(const Array<String>& key);

  Client& discard(const ReplyCallback& reply_cb);
  Future<Reply> discard();

  Client& dump(const String& key, const ReplyCallback& reply_cb);
  Future<Reply> dump(const String& key);

  Client& echo(const String& msg, const ReplyCallback& reply_cb);
  Future<Reply> echo(const String& msg);

  Client& eval(const String& script, int numkeys, const Array<String>& keys, const Array<String>& args, const ReplyCallback& reply_cb);
  Future<Reply> eval(const String& script, int numkeys, const Array<String>& keys, const Array<String>& args);

  Client& evalsha(const String& sha1, int numkeys, const Array<String>& keys, const Array<String>& args, const ReplyCallback& reply_cb);
  Future<Reply> evalsha(const String& sha1, int numkeys, const Array<String>& keys, const Array<String>& args);

  Client& exec(const ReplyCallback& reply_cb);
  Future<Reply> exec();

  Client& exists(const Array<String>& keys, const ReplyCallback& reply_cb);
  Future<Reply> exists(const Array<String>& keys);

  Client& expire(const String& key, int seconds, const ReplyCallback& reply_cb);
  Future<Reply> expire(const String& key, int seconds);

  Client& expireat(const String& key, int timestamp, const ReplyCallback& reply_cb);
  Future<Reply> expireat(const String& key, int timestamp);

  Client& flushall(const ReplyCallback& reply_cb);
  Future<Reply> flushall();

  Client& flushdb(const ReplyCallback& reply_cb);
  Future<Reply> flushdb();

  Client& geoadd(const String& key, const Array<tl::tuple<String, String, String>>& long_lat_memb, const ReplyCallback& reply_cb);
  Future<Reply> geoadd(const String& key, const Array<tl::tuple<String, String, String>>& long_lat_memb);

  Client& geohash(const String& key, const Array<String>& members, const ReplyCallback& reply_cb);
  Future<Reply> geohash(const String& key, const Array<String>& members);

  Client& geopos(const String& key, const Array<String>& members, const ReplyCallback& reply_cb);
  Future<Reply> geopos(const String& key, const Array<String>& members);

  Client& geodist(const String& key, const String& member_1, const String& member_2, const ReplyCallback& reply_cb);
  Client& geodist(const String& key, const String& member_1, const String& member_2, const String& unit, const ReplyCallback& reply_cb);
  Future<Reply> geodist(const String& key, const String& member_1, const String& member_2, const String& unit = "m");

  Client& georadius(const String& key, double longitude, double latitude, double radius, GeoUnit unit, bool with_coord, bool with_dist, bool with_hash, bool asc_order, const ReplyCallback& reply_cb);
  Client& georadius(const String& key, double longitude, double latitude, double radius, GeoUnit unit, bool with_coord, bool with_dist, bool with_hash, bool asc_order, int32 count, const ReplyCallback& reply_cb);
  Client& georadius(const String& key, double longitude, double latitude, double radius, GeoUnit unit, bool with_coord, bool with_dist, bool with_hash, bool asc_order, const String& store_key, const ReplyCallback& reply_cb);
  Client& georadius(const String& key, double longitude, double latitude, double radius, GeoUnit unit, bool with_coord, bool with_dist, bool with_hash, bool asc_order, const String& store_key, const String& storedist_key, const ReplyCallback& reply_cb);
  Client& georadius(const String& key, double longitude, double latitude, double radius, GeoUnit unit, bool with_coord, bool with_dist, bool with_hash, bool asc_order, int32 count, const String& store_key, const ReplyCallback& reply_cb);
  Client& georadius(const String& key, double longitude, double latitude, double radius, GeoUnit unit, bool with_coord, bool with_dist, bool with_hash, bool asc_order, int32 count, const String& store_key, const String& storedist_key, const ReplyCallback& reply_cb);
  Future<Reply> georadius(const String& key, double longitude, double latitude, double radius, GeoUnit unit, bool with_coord = false, bool with_dist = false, bool with_hash = false, bool asc_order = false, int32 count = 0, const String& store_key = "", const String& storedist_key = "");

  Client& georadiusbymember(const String& key, const String& member, double radius, GeoUnit unit, bool with_coord, bool with_dist, bool with_hash, bool asc_order, const ReplyCallback& reply_cb);
  Client& georadiusbymember(const String& key, const String& member, double radius, GeoUnit unit, bool with_coord, bool with_dist, bool with_hash, bool asc_order, int32 count, const ReplyCallback& reply_cb);
  Client& georadiusbymember(const String& key, const String& member, double radius, GeoUnit unit, bool with_coord, bool with_dist, bool with_hash, bool asc_order, const String& store_key, const ReplyCallback& reply_cb);
  Client& georadiusbymember(const String& key, const String& member, double radius, GeoUnit unit, bool with_coord, bool with_dist, bool with_hash, bool asc_order, const String& store_key, const String& storedist_key, const ReplyCallback& reply_cb);
  Client& georadiusbymember(const String& key, const String& member, double radius, GeoUnit unit, bool with_coord, bool with_dist, bool with_hash, bool asc_order, int32 count, const String& store_key, const ReplyCallback& reply_cb);
  Client& georadiusbymember(const String& key, const String& member, double radius, GeoUnit unit, bool with_coord, bool with_dist, bool with_hash, bool asc_order, int32 count, const String& store_key, const String& storedist_key, const ReplyCallback& reply_cb);
  Future<Reply> georadiusbymember(const String& key, const String& member, double radius, GeoUnit unit, bool with_coord = false, bool with_dist = false, bool with_hash = false, bool asc_order = false, int32 count = 0, const String& store_key = "", const String& storedist_key = "");

  Client& get(const String& key, const ReplyCallback& reply_cb);
  Future<Reply> get(const String& key);

  Client& getbit(const String& key, int offset, const ReplyCallback& reply_cb);
  Future<Reply> getbit(const String& key, int offset);

  Client& getrange(const String& key, int start, int end, const ReplyCallback& reply_cb);
  Future<Reply> getrange(const String& key, int start, int end);

  Client& getset(const String& key, const String& val, const ReplyCallback& reply_cb);
  Future<Reply> getset(const String& key, const String& val);

  Client& hdel(const String& key, const Array<String>& fields, const ReplyCallback& reply_cb);
  Future<Reply> hdel(const String& key, const Array<String>& fields);

  Client& hexists(const String& key, const String& field, const ReplyCallback& reply_cb);
  Future<Reply> hexists(const String& key, const String& field);

  Client& hget(const String& key, const String& field, const ReplyCallback& reply_cb);
  Future<Reply> hget(const String& key, const String& field);

  Client& hgetall(const String& key, const ReplyCallback& reply_cb);
  Future<Reply> hgetall(const String& key);

  Client& hincrby(const String& key, const String& field, int incr, const ReplyCallback& reply_cb);
  Future<Reply> hincrby(const String& key, const String& field, int incr);

  Client& hincrbyfloat(const String& key, const String& field, float incr, const ReplyCallback& reply_cb);
  Future<Reply> hincrbyfloat(const String& key, const String& field, float incr);

  Client& hkeys(const String& key, const ReplyCallback& reply_cb);
  Future<Reply> hkeys(const String& key);

  Client& hlen(const String& key, const ReplyCallback& reply_cb);
  Future<Reply> hlen(const String& key);

  Client& hmget(const String& key, const Array<String>& fields, const ReplyCallback& reply_cb);
  Future<Reply> hmget(const String& key, const Array<String>& fields);

  Client& hmset(const String& key, const Array<Pair<String, String>>& field_val, const ReplyCallback& reply_cb);
  Future<Reply> hmset(const String& key, const Array<Pair<String, String>>& field_val);

  Client& hscan(const String& key, int32 cursor, const ReplyCallback& reply_cb);
  Future<Reply> hscan(const String& key, int32 cursor);

  Client& hscan(const String& key, int32 cursor, const String& pattern, const ReplyCallback& reply_cb);
  Future<Reply> hscan(const String& key, int32 cursor, const String& pattern);

  Client& hscan(const String& key, int32 cursor, int32 count, const ReplyCallback& reply_cb);
  Future<Reply> hscan(const String& key, int32 cursor, int32 count);

  Client& hscan(const String& key, int32 cursor, const String& pattern, int32 count, const ReplyCallback& reply_cb);
  Future<Reply> hscan(const String& key, int32 cursor, const String& pattern, int32 count);

  Client& hset(const String& key, const String& field, const String& value, const ReplyCallback& reply_cb);
  Future<Reply> hset(const String& key, const String& field, const String& value);

  Client& hsetnx(const String& key, const String& field, const String& value, const ReplyCallback& reply_cb);
  Future<Reply> hsetnx(const String& key, const String& field, const String& value);

  Client& hstrlen(const String& key, const String& field, const ReplyCallback& reply_cb);
  Future<Reply> hstrlen(const String& key, const String& field);

  Client& hvals(const String& key, const ReplyCallback& reply_cb);
  Future<Reply> hvals(const String& key);

  Client& incr(const String& key, const ReplyCallback& reply_cb);
  Future<Reply> incr(const String& key);

  Client& incrby(const String& key, int incr, const ReplyCallback& reply_cb);
  Future<Reply> incrby(const String& key, int incr);

  Client& incrbyfloat(const String& key, float incr, const ReplyCallback& reply_cb);
  Future<Reply> incrbyfloat(const String& key, float incr);

  Client& info(const ReplyCallback& reply_cb);
  Client& info(const String& section, const ReplyCallback& reply_cb);
  Future<Reply> info(const String& section = "default");

  Client& keys(const String& pattern, const ReplyCallback& reply_cb);
  Future<Reply> keys(const String& pattern);

  Client& lastsave(const ReplyCallback& reply_cb);
  Future<Reply> lastsave();

  Client& lindex(const String& key, int index, const ReplyCallback& reply_cb);
  Future<Reply> lindex(const String& key, int index);

  Client& linsert(const String& key, const String& before_after, const String& pivot, const String& value, const ReplyCallback& reply_cb);
  Future<Reply> linsert(const String& key, const String& before_after, const String& pivot, const String& value);

  Client& llen(const String& key, const ReplyCallback& reply_cb);
  Future<Reply> llen(const String& key);

  Client& lpop(const String& key, const ReplyCallback& reply_cb);
  Future<Reply> lpop(const String& key);

  Client& lpush(const String& key, const Array<String>& values, const ReplyCallback& reply_cb);
  Future<Reply> lpush(const String& key, const Array<String>& values);

  Client& lpushx(const String& key, const String& value, const ReplyCallback& reply_cb);
  Future<Reply> lpushx(const String& key, const String& value);

  Client& lrange(const String& key, int start, int stop, const ReplyCallback& reply_cb);
  Future<Reply> lrange(const String& key, int start, int stop);

  Client& lrem(const String& key, int count, const String& value, const ReplyCallback& reply_cb);
  Future<Reply> lrem(const String& key, int count, const String& value);

  Client& lset(const String& key, int index, const String& value, const ReplyCallback& reply_cb);
  Future<Reply> lset(const String& key, int index, const String& value);

  Client& ltrim(const String& key, int start, int stop, const ReplyCallback& reply_cb);
  Future<Reply> ltrim(const String& key, int start, int stop);

  Client& mget(const Array<String>& keys, const ReplyCallback& reply_cb);
  Future<Reply> mget(const Array<String>& keys);

  Client& migrate(const String& host, int port, const String& key, const String& dest_db, int timeout, const ReplyCallback& reply_cb);
  Client& migrate(const String& host, int port, const String& key, const String& dest_db, int timeout, bool copy, bool replace, const Array<String>& keys, const ReplyCallback& reply_cb);
  Future<Reply> migrate(const String& host, int port, const String& key, const String& dest_db, int timeout, bool copy = false, bool replace = false, const Array<String>& keys = {});

  Client& monitor(const ReplyCallback& reply_cb);
  Future<Reply> monitor();

  Client& move(const String& key, const String& db, const ReplyCallback& reply_cb);
  Future<Reply> move(const String& key, const String& db);

  Client& mset(const Array<Pair<String, String>>& key_vals, const ReplyCallback& reply_cb);
  Future<Reply> mset(const Array<Pair<String, String>>& key_vals);

  Client& msetnx(const Array<Pair<String, String>>& key_vals, const ReplyCallback& reply_cb);
  Future<Reply> msetnx(const Array<Pair<String, String>>& key_vals);

  Client& multi(const ReplyCallback& reply_cb);
  Future<Reply> multi();

  Client& object(const String& subcommand, const Array<String>& args, const ReplyCallback& reply_cb);
  Future<Reply> object(const String& subcommand, const Array<String>& args);

  Client& persist(const String& key, const ReplyCallback& reply_cb);
  Future<Reply> persist(const String& key);

  Client& pexpire(const String& key, int milliseconds, const ReplyCallback& reply_cb);
  Future<Reply> pexpire(const String& key, int milliseconds);

  Client& pexpireat(const String& key, int milliseconds_timestamp, const ReplyCallback& reply_cb);
  Future<Reply> pexpireat(const String& key, int milliseconds_timestamp);

  Client& pfadd(const String& key, const Array<String>& elements, const ReplyCallback& reply_cb);
  Future<Reply> pfadd(const String& key, const Array<String>& elements);

  Client& pfcount(const Array<String>& keys, const ReplyCallback& reply_cb);
  Future<Reply> pfcount(const Array<String>& keys);

  Client& pfmerge(const String& destkey, const Array<String>& sourcekeys, const ReplyCallback& reply_cb);
  Future<Reply> pfmerge(const String& destkey, const Array<String>& sourcekeys);

  Client& ping(const ReplyCallback& reply_cb);
  Future<Reply> ping();

  Client& ping(const String& message, const ReplyCallback& reply_cb);
  Future<Reply> ping(const String& message);

  Client& psetex(const String& key, int milliseconds, const String& val, const ReplyCallback& reply_cb);
  Future<Reply> psetex(const String& key, int milliseconds, const String& val);

  Client& publish(const String& channel, const String& message, const ReplyCallback& reply_cb);
  Future<Reply> publish(const String& channel, const String& message);

  Client& pubsub(const String& subcommand, const Array<String>& args, const ReplyCallback& reply_cb);
  Future<Reply> pubsub(const String& subcommand, const Array<String>& args);

  Client& pttl(const String& key, const ReplyCallback& reply_cb);
  Future<Reply> pttl(const String& key);

  Client& quit(const ReplyCallback& reply_cb);
  Future<Reply> quit();

  Client& randomkey(const ReplyCallback& reply_cb);
  Future<Reply> randomkey();

  Client& readonly(const ReplyCallback& reply_cb);
  Future<Reply> readonly();

  Client& readwrite(const ReplyCallback& reply_cb);
  Future<Reply> readwrite();

  Client& rename(const String& key, const String& newkey, const ReplyCallback& reply_cb);
  Future<Reply> rename(const String& key, const String& newkey);

  Client& renamenx(const String& key, const String& newkey, const ReplyCallback& reply_cb);
  Future<Reply> renamenx(const String& key, const String& newkey);

  Client& restore(const String& key, int ttl, const String& serialized_value, const ReplyCallback& reply_cb);
  Future<Reply> restore(const String& key, int ttl, const String& serialized_value);

  Client& restore(const String& key, int ttl, const String& serialized_value, const String& replace, const ReplyCallback& reply_cb);
  Future<Reply> restore(const String& key, int ttl, const String& serialized_value, const String& replace);

  Client& role(const ReplyCallback& reply_cb);
  Future<Reply> role();

  Client& rpop(const String& key, const ReplyCallback& reply_cb);
  Future<Reply> rpop(const String& key);

  Client& rpoplpush(const String& source, const String& destination, const ReplyCallback& reply_cb);
  Future<Reply> rpoplpush(const String& src, const String& dst);

  Client& rpush(const String& key, const Array<String>& values, const ReplyCallback& reply_cb);
  Future<Reply> rpush(const String& key, const Array<String>& values);

  Client& rpushx(const String& key, const String& value, const ReplyCallback& reply_cb);
  Future<Reply> rpushx(const String& key, const String& value);

  Client& sadd(const String& key, const Array<String>& members, const ReplyCallback& reply_cb);
  Future<Reply> sadd(const String& key, const Array<String>& members);

  Client& save(const ReplyCallback& reply_cb);
  Future<Reply> save();

  Client& scan(int32 cursor, const ReplyCallback& reply_cb);
  Future<Reply> scan(int32 cursor);

  Client& scan(int32 cursor, const String& pattern, const ReplyCallback& reply_cb);
  Future<Reply> scan(int32 cursor, const String& pattern);

  Client& scan(int32 cursor, int32 count, const ReplyCallback& reply_cb);
  Future<Reply> scan(int32 cursor, int32 count);

  Client& scan(int32 cursor, const String& pattern, int32 count, const ReplyCallback& reply_cb);
  Future<Reply> scan(int32 cursor, const String& pattern, int32 count);

  Client& scard(const String& key, const ReplyCallback& reply_cb);
  Future<Reply> scard(const String& key);

  Client& script_debug(const String& mode, const ReplyCallback& reply_cb);
  Future<Reply> script_debug(const String& mode);

  Client& script_exists(const Array<String>& scripts, const ReplyCallback& reply_cb);
  Future<Reply> script_exists(const Array<String>& scripts);

  Client& script_flush(const ReplyCallback& reply_cb);
  Future<Reply> script_flush();

  Client& script_kill(const ReplyCallback& reply_cb);
  Future<Reply> script_kill();

  Client& script_load(const String& script, const ReplyCallback& reply_cb);
  Future<Reply> script_load(const String& script);

  Client& sdiff(const Array<String>& keys, const ReplyCallback& reply_cb);
  Future<Reply> sdiff(const Array<String>& keys);

  Client& sdiffstore(const String& destination, const Array<String>& keys, const ReplyCallback& reply_cb);
  Future<Reply> sdiffstore(const String& dst, const Array<String>& keys);

  Client& select(int index, const ReplyCallback& reply_cb);
  Future<Reply> select(int index);

  Client& set(const String& key, const String& value, const ReplyCallback& reply_cb);
  Future<Reply> set(const String& key, const String& value);

  Client& set_advanced(const String& key, const String& value, const ReplyCallback& reply_cb);
  Client& set_advanced(const String& key, const String& value, bool ex, int ex_sec, bool px, int px_milli, bool nx, bool xx, const ReplyCallback& reply_cb);
  Future<Reply> set_advanced(const String& key, const String& value, bool ex = false, int ex_sec = 0, bool px = false, int px_milli = 0, bool nx = false, bool xx = false);

  Client& setbit_(const String& key, int offset, const String& value, const ReplyCallback& reply_cb);
  Future<Reply> setbit_(const String& key, int offset, const String& value);

  Client& setex(const String& key, int seconds, const String& value, const ReplyCallback& reply_cb);
  Future<Reply> setex(const String& key, int seconds, const String& value);

  Client& setnx(const String& key, const String& value, const ReplyCallback& reply_cb);
  Future<Reply> setnx(const String& key, const String& value);

  Client& setrange(const String& key, int offset, const String& value, const ReplyCallback& reply_cb);
  Future<Reply> setrange(const String& key, int offset, const String& value);

  Client& shutdown(const ReplyCallback& reply_cb);
  Future<Reply> shutdown();

  Client& shutdown(const String& save, const ReplyCallback& reply_cb);
  Future<Reply> shutdown(const String& save);

  Client& sinter(const Array<String>& keys, const ReplyCallback& reply_cb);
  Future<Reply> sinter(const Array<String>& keys);

  Client& sinterstore(const String& destination, const Array<String>& keys, const ReplyCallback& reply_cb);
  Future<Reply> sinterstore(const String& dst, const Array<String>& keys);

  Client& sismember(const String& key, const String& member, const ReplyCallback& reply_cb);
  Future<Reply> sismember(const String& key, const String& member);

  Client& slaveof(const String& host, int port, const ReplyCallback& reply_cb);
  Future<Reply> slaveof(const String& host, int port);

  Client& slowlog(const String subcommand, const ReplyCallback& reply_cb);
  Future<Reply> slowlog(const String& subcommand);

  Client& slowlog(const String subcommand, const String& argument, const ReplyCallback& reply_cb);
  Future<Reply> slowlog(const String& subcommand, const String& argument);

  Client& smembers(const String& key, const ReplyCallback& reply_cb);
  Future<Reply> smembers(const String& key);

  Client& smove(const String& source, const String& destination, const String& member, const ReplyCallback& reply_cb);
  Future<Reply> smove(const String& src, const String& dst, const String& member);

  Client& sort(const String& key, const ReplyCallback& reply_cb);
  Future<Reply> sort(const String& key);

  Client& sort(const String& key, const Array<String>& get_patterns, bool asc_order, bool alpha, const ReplyCallback& reply_cb);
  Future<Reply> sort(const String& key, const Array<String>& get_patterns, bool asc_order, bool alpha);

  Client& sort(const String& key, int32 offset, int32 count, const Array<String>& get_patterns, bool asc_order, bool alpha, const ReplyCallback& reply_cb);
  Future<Reply> sort(const String& key, int32 offset, int32 count, const Array<String>& get_patterns, bool asc_order, bool alpha);

  Client& sort(const String& key, const String& by_pattern, const Array<String>& get_patterns, bool asc_order, bool alpha, const ReplyCallback& reply_cb);
  Future<Reply> sort(const String& key, const String& by_pattern, const Array<String>& get_patterns, bool asc_order, bool alpha);

  Client& sort(const String& key, const Array<String>& get_patterns, bool asc_order, bool alpha, const String& store_dest, const ReplyCallback& reply_cb);
  Future<Reply> sort(const String& key, const Array<String>& get_patterns, bool asc_order, bool alpha, const String& store_dest);

  Client& sort(const String& key, int32 offset, int32 count, const Array<String>& get_patterns, bool asc_order, bool alpha, const String& store_dest, const ReplyCallback& reply_cb);
  Future<Reply> sort(const String& key, int32 offset, int32 count, const Array<String>& get_patterns, bool asc_order, bool alpha, const String& store_dest);

  Client& sort(const String& key, const String& by_pattern, const Array<String>& get_patterns, bool asc_order, bool alpha, const String& store_dest, const ReplyCallback& reply_cb);
  Future<Reply> sort(const String& key, const String& by_pattern, const Array<String>& get_patterns, bool asc_order, bool alpha, const String& store_dest);

  Client& sort(const String& key, const String& by_pattern, int32 offset, int32 count, const Array<String>& get_patterns, bool asc_order, bool alpha, const ReplyCallback& reply_cb);
  Future<Reply> sort(const String& key, const String& by_pattern, int32 offset, int32 count, const Array<String>& get_patterns, bool asc_order, bool alpha);

  Client& sort(const String& key, const String& by_pattern, int32 offset, int32 count, const Array<String>& get_patterns, bool asc_order, bool alpha, const String& store_dest, const ReplyCallback& reply_cb);
  Future<Reply> sort(const String& key, const String& by_pattern, int32 offset, int32 count, const Array<String>& get_patterns, bool asc_order, bool alpha, const String& store_dest);

  Client& spop(const String& key, const ReplyCallback& reply_cb);
  Future<Reply> spop(const String& key);

  Client& spop(const String& key, int count, const ReplyCallback& reply_cb);
  Future<Reply> spop(const String& key, int count);

  Client& srandmember(const String& key, const ReplyCallback& reply_cb);
  Future<Reply> srandmember(const String& key);

  Client& srandmember(const String& key, int count, const ReplyCallback& reply_cb);
  Future<Reply> srandmember(const String& key, int count);

  Client& srem(const String& key, const Array<String>& members, const ReplyCallback& reply_cb);
  Future<Reply> srem(const String& key, const Array<String>& members);

  Client& sscan(const String& key, int32 cursor, const ReplyCallback& reply_cb);
  Future<Reply> sscan(const String& key, int32 cursor);

  Client& sscan(const String& key, int32 cursor, const String& pattern, const ReplyCallback& reply_cb);
  Future<Reply> sscan(const String& key, int32 cursor, const String& pattern);

  Client& sscan(const String& key, int32 cursor, int32 count, const ReplyCallback& reply_cb);
  Future<Reply> sscan(const String& key, int32 cursor, int32 count);

  Client& sscan(const String& key, int32 cursor, const String& pattern, int32 count, const ReplyCallback& reply_cb);
  Future<Reply> sscan(const String& key, int32 cursor, const String& pattern, int32 count);

  Client& strlen(const String& key, const ReplyCallback& reply_cb);
  Future<Reply> strlen(const String& key);

  Client& sunion(const Array<String>& keys, const ReplyCallback& reply_cb);
  Future<Reply> sunion(const Array<String>& keys);

  Client& sunionstore(const String& destination, const Array<String>& keys, const ReplyCallback& reply_cb);
  Future<Reply> sunionstore(const String& dst, const Array<String>& keys);

  Client& sync(const ReplyCallback& reply_cb);
  Future<Reply> sync();

  Client& time(const ReplyCallback& reply_cb);
  Future<Reply> time();

  Client& ttl(const String& key, const ReplyCallback& reply_cb);
  Future<Reply> ttl(const String& key);

  Client& type(const String& key, const ReplyCallback& reply_cb);
  Future<Reply> type(const String& key);

  Client& unwatch(const ReplyCallback& reply_cb);
  Future<Reply> unwatch();

  Client& wait(int numslaves, int timeout, const ReplyCallback& reply_cb);
  Future<Reply> wait(int numslaves, int timeout);

  Client& watch(const Array<String>& keys, const ReplyCallback& reply_cb);
  Future<Reply> watch(const Array<String>& keys);

  Client& zadd(const String& key, const Array<String>& options, const tl::multimap<String, String>& score_members, const ReplyCallback& reply_cb);
  Future<Reply> zadd(const String& key, const Array<String>& options, const tl::multimap<String, String>& score_members);

  Client& zcard(const String& key, const ReplyCallback& reply_cb);
  Future<Reply> zcard(const String& key);

  Client& zcount(const String& key, int min, int max, const ReplyCallback& reply_cb);
  Future<Reply> zcount(const String& key, int min, int max);

  Client& zcount(const String& key, double min, double max, const ReplyCallback& reply_cb);
  Future<Reply> zcount(const String& key, double min, double max);

  Client& zcount(const String& key, const String& min, const String& max, const ReplyCallback& reply_cb);
  Future<Reply> zcount(const String& key, const String& min, const String& max);

  Client& zincrby(const String& key, int incr, const String& member, const ReplyCallback& reply_cb);
  Future<Reply> zincrby(const String& key, int incr, const String& member);

  Client& zincrby(const String& key, double incr, const String& member, const ReplyCallback& reply_cb);
  Future<Reply> zincrby(const String& key, double incr, const String& member);

  Client& zincrby(const String& key, const String& incr, const String& member, const ReplyCallback& reply_cb);
  Future<Reply> zincrby(const String& key, const String& incr, const String& member);

  Client& zinterstore(const String& destination, int32 numkeys, const Array<String>& keys, const Array<int32> weights, AggregateMethod method, const ReplyCallback& reply_cb);
  Future<Reply> zinterstore(const String& destination, int32 numkeys, const Array<String>& keys, const Array<int32> weights, AggregateMethod method);

  Client& zlexcount(const String& key, int min, int max, const ReplyCallback& reply_cb);
  Future<Reply> zlexcount(const String& key, int min, int max);

  Client& zlexcount(const String& key, double min, double max, const ReplyCallback& reply_cb);
  Future<Reply> zlexcount(const String& key, double min, double max);

  Client& zlexcount(const String& key, const String& min, const String& max, const ReplyCallback& reply_cb);
  Future<Reply> zlexcount(const String& key, const String& min, const String& max);

  Client& zrange(const String& key, int start, int stop, const ReplyCallback& reply_cb);
  Client& zrange(const String& key, int start, int stop, bool withscores, const ReplyCallback& reply_cb);
  Future<Reply> zrange(const String& key, int start, int stop, bool withscores = false);

  Client& zrange(const String& key, double start, double stop, const ReplyCallback& reply_cb);
  Client& zrange(const String& key, double start, double stop, bool withscores, const ReplyCallback& reply_cb);
  Future<Reply> zrange(const String& key, double start, double stop, bool withscores = false);

  Client& zrange(const String& key, const String& start, const String& stop, const ReplyCallback& reply_cb);
  Client& zrange(const String& key, const String& start, const String& stop, bool withscores, const ReplyCallback& reply_cb);
  Future<Reply> zrange(const String& key, const String& start, const String& stop, bool withscores = false);

  Client& zrangebylex(const String& key, int min, int max, const ReplyCallback& reply_cb);
  Client& zrangebylex(const String& key, int min, int max, bool withscores, const ReplyCallback& reply_cb);
  Future<Reply> zrangebylex(const String& key, int min, int max, bool withscores = false);

  Client& zrangebylex(const String& key, double min, double max, const ReplyCallback& reply_cb);
  Client& zrangebylex(const String& key, double min, double max, bool withscores, const ReplyCallback& reply_cb);
  Future<Reply> zrangebylex(const String& key, double min, double max, bool withscores = false);

  Client& zrangebylex(const String& key, const String& min, const String& max, const ReplyCallback& reply_cb);
  Client& zrangebylex(const String& key, const String& min, const String& max, bool withscores, const ReplyCallback& reply_cb);
  Future<Reply> zrangebylex(const String& key, const String& min, const String& max, bool withscores = false);

  Client& zrangebylex(const String& key, int min, int max, int32 offset, int32 count, const ReplyCallback& reply_cb);
  Client& zrangebylex(const String& key, int min, int max, int32 offset, int32 count, bool withscores, const ReplyCallback& reply_cb);
  Future<Reply> zrangebylex(const String& key, int min, int max, int32 offset, int32 count, bool withscores = false);

  Client& zrangebylex(const String& key, double min, double max, int32 offset, int32 count, const ReplyCallback& reply_cb);
  Client& zrangebylex(const String& key, double min, double max, int32 offset, int32 count, bool withscores, const ReplyCallback& reply_cb);
  Future<Reply> zrangebylex(const String& key, double min, double max, int32 offset, int32 count, bool withscores = false);

  Client& zrangebylex(const String& key, const String& min, const String& max, int32 offset, int32 count, const ReplyCallback& reply_cb);
  Client& zrangebylex(const String& key, const String& min, const String& max, int32 offset, int32 count, bool withscores, const ReplyCallback& reply_cb);
  Future<Reply> zrangebylex(const String& key, const String& min, const String& max, int32 offset, int32 count, bool withscores = false);

  Client& zrangebyscore(const String& key, int min, int max, const ReplyCallback& reply_cb);
  Client& zrangebyscore(const String& key, int min, int max, bool withscores, const ReplyCallback& reply_cb);
  Future<Reply> zrangebyscore(const String& key, int min, int max, bool withscores = false);

  Client& zrangebyscore(const String& key, double min, double max, const ReplyCallback& reply_cb);
  Client& zrangebyscore(const String& key, double min, double max, bool withscores, const ReplyCallback& reply_cb);
  Future<Reply> zrangebyscore(const String& key, double min, double max, bool withscores = false);

  Client& zrangebyscore(const String& key, const String& min, const String& max, const ReplyCallback& reply_cb);
  Client& zrangebyscore(const String& key, const String& min, const String& max, bool withscores, const ReplyCallback& reply_cb);
  Future<Reply> zrangebyscore(const String& key, const String& min, const String& max, bool withscores = false);

  Client& zrangebyscore(const String& key, int min, int max, int32 offset, int32 count, const ReplyCallback& reply_cb);
  Client& zrangebyscore(const String& key, int min, int max, int32 offset, int32 count, bool withscores, const ReplyCallback& reply_cb);
  Future<Reply> zrangebyscore(const String& key, int min, int max, int32 offset, int32 count, bool withscores = false);

  Client& zrangebyscore(const String& key, double min, double max, int32 offset, int32 count, const ReplyCallback& reply_cb);
  Client& zrangebyscore(const String& key, double min, double max, int32 offset, int32 count, bool withscores, const ReplyCallback& reply_cb);
  Future<Reply> zrangebyscore(const String& key, double min, double max, int32 offset, int32 count, bool withscores = false);

  Client& zrangebyscore(const String& key, const String& min, const String& max, int32 offset, int32 count, const ReplyCallback& reply_cb);
  Client& zrangebyscore(const String& key, const String& min, const String& max, int32 offset, int32 count, bool withscores, const ReplyCallback& reply_cb);
  Future<Reply> zrangebyscore(const String& key, const String& min, const String& max, int32 offset, int32 count, bool withscores = false);

  Client& zrank(const String& key, const String& member, const ReplyCallback& reply_cb);
  Future<Reply> zrank(const String& key, const String& member);

  Client& zrem(const String& key, const Array<String>& members, const ReplyCallback& reply_cb);
  Future<Reply> zrem(const String& key, const Array<String>& members);

  Client& zremrangebylex(const String& key, int min, int max, const ReplyCallback& reply_cb);
  Future<Reply> zremrangebylex(const String& key, int min, int max);

  Client& zremrangebylex(const String& key, double min, double max, const ReplyCallback& reply_cb);
  Future<Reply> zremrangebylex(const String& key, double min, double max);

  Client& zremrangebylex(const String& key, const String& min, const String& max, const ReplyCallback& reply_cb);
  Future<Reply> zremrangebylex(const String& key, const String& min, const String& max);

  Client& zremrangebyrank(const String& key, int start, int stop, const ReplyCallback& reply_cb);
  Future<Reply> zremrangebyrank(const String& key, int start, int stop);

  Client& zremrangebyrank(const String& key, double start, double stop, const ReplyCallback& reply_cb);
  Future<Reply> zremrangebyrank(const String& key, double start, double stop);

  Client& zremrangebyrank(const String& key, const String& start, const String& stop, const ReplyCallback& reply_cb);
  Future<Reply> zremrangebyrank(const String& key, const String& start, const String& stop);

  Client& zremrangebyscore(const String& key, int min, int max, const ReplyCallback& reply_cb);
  Future<Reply> zremrangebyscore(const String& key, int min, int max);

  Client& zremrangebyscore(const String& key, double min, double max, const ReplyCallback& reply_cb);
  Future<Reply> zremrangebyscore(const String& key, double min, double max);

  Client& zremrangebyscore(const String& key, const String& min, const String& max, const ReplyCallback& reply_cb);
  Future<Reply> zremrangebyscore(const String& key, const String& min, const String& max);

  Client& zrevrange(const String& key, int start, int stop, const ReplyCallback& reply_cb);
  Client& zrevrange(const String& key, int start, int stop, bool withscores, const ReplyCallback& reply_cb);
  Future<Reply> zrevrange(const String& key, int start, int stop, bool withscores = false);

  Client& zrevrange(const String& key, double start, double stop, const ReplyCallback& reply_cb);
  Client& zrevrange(const String& key, double start, double stop, bool withscores, const ReplyCallback& reply_cb);
  Future<Reply> zrevrange(const String& key, double start, double stop, bool withscores = false);

  Client& zrevrange(const String& key, const String& start, const String& stop, const ReplyCallback& reply_cb);
  Client& zrevrange(const String& key, const String& start, const String& stop, bool withscores, const ReplyCallback& reply_cb);
  Future<Reply> zrevrange(const String& key, const String& start, const String& stop, bool withscores = false);

  Client& zrevrangebylex(const String& key, int max, int min, const ReplyCallback& reply_cb);
  Client& zrevrangebylex(const String& key, int max, int min, bool withscores, const ReplyCallback& reply_cb);
  Future<Reply> zrevrangebylex(const String& key, int max, int min, bool withscores = false);

  Client& zrevrangebylex(const String& key, double max, double min, const ReplyCallback& reply_cb);
  Client& zrevrangebylex(const String& key, double max, double min, bool withscores, const ReplyCallback& reply_cb);
  Future<Reply> zrevrangebylex(const String& key, double max, double min, bool withscores = false);

  Client& zrevrangebylex(const String& key, const String& max, const String& min, const ReplyCallback& reply_cb);
  Client& zrevrangebylex(const String& key, const String& max, const String& min, bool withscores, const ReplyCallback& reply_cb);
  Future<Reply> zrevrangebylex(const String& key, const String& max, const String& min, bool withscores = false);

  Client& zrevrangebylex(const String& key, int max, int min, int32 offset, int32 count, const ReplyCallback& reply_cb);
  Client& zrevrangebylex(const String& key, int max, int min, int32 offset, int32 count, bool withscores, const ReplyCallback& reply_cb);
  Future<Reply> zrevrangebylex(const String& key, int max, int min, int32 offset, int32 count, bool withscores = false);

  Client& zrevrangebylex(const String& key, double max, double min, int32 offset, int32 count, const ReplyCallback& reply_cb);
  Client& zrevrangebylex(const String& key, double max, double min, int32 offset, int32 count, bool withscores, const ReplyCallback& reply_cb);
  Future<Reply> zrevrangebylex(const String& key, double max, double min, int32 offset, int32 count, bool withscores = false);

  Client& zrevrangebylex(const String& key, const String& max, const String& min, int32 offset, int32 count, const ReplyCallback& reply_cb);
  Client& zrevrangebylex(const String& key, const String& max, const String& min, int32 offset, int32 count, bool withscores, const ReplyCallback& reply_cb);
  Future<Reply> zrevrangebylex(const String& key, const String& max, const String& min, int32 offset, int32 count, bool withscores = false);

  Client& zrevrangebyscore(const String& key, int max, int min, const ReplyCallback& reply_cb);
  Client& zrevrangebyscore(const String& key, int max, int min, bool withscores, const ReplyCallback& reply_cb);
  Future<Reply> zrevrangebyscore(const String& key, int max, int min, bool withscores = false);

  Client& zrevrangebyscore(const String& key, double max, double min, const ReplyCallback& reply_cb);
  Client& zrevrangebyscore(const String& key, double max, double min, bool withscores, const ReplyCallback& reply_cb);
  Future<Reply> zrevrangebyscore(const String& key, double max, double min, bool withscores = false);

  Client& zrevrangebyscore(const String& key, const String& max, const String& min, const ReplyCallback& reply_cb);
  Client& zrevrangebyscore(const String& key, const String& max, const String& min, bool withscores, const ReplyCallback& reply_cb);
  Future<Reply> zrevrangebyscore(const String& key, const String& max, const String& min, bool withscores = false);

  Client& zrevrangebyscore(const String& key, int max, int min, int32 offset, int32 count, const ReplyCallback& reply_cb);
  Client& zrevrangebyscore(const String& key, int max, int min, int32 offset, int32 count, bool withscores, const ReplyCallback& reply_cb);
  Future<Reply> zrevrangebyscore(const String& key, int max, int min, int32 offset, int32 count, bool withscores = false);

  Client& zrevrangebyscore(const String& key, double max, double min, int32 offset, int32 count, const ReplyCallback& reply_cb);
  Client& zrevrangebyscore(const String& key, double max, double min, int32 offset, int32 count, bool withscores, const ReplyCallback& reply_cb);
  Future<Reply> zrevrangebyscore(const String& key, double max, double min, int32 offset, int32 count, bool withscores = false);

  Client& zrevrangebyscore(const String& key, const String& max, const String& min, int32 offset, int32 count, const ReplyCallback& reply_cb);
  Client& zrevrangebyscore(const String& key, const String& max, const String& min, int32 offset, int32 count, bool withscores, const ReplyCallback& reply_cb);
  Future<Reply> zrevrangebyscore(const String& key, const String& max, const String& min, int32 offset, int32 count, bool withscores = false);

  Client& zrevrank(const String& key, const String& member, const ReplyCallback& reply_cb);
  Future<Reply> zrevrank(const String& key, const String& member);

  Client& zscan(const String& key, int32 cursor, const ReplyCallback& reply_cb);
  Future<Reply> zscan(const String& key, int32 cursor);

  Client& zscan(const String& key, int32 cursor, const String& pattern, const ReplyCallback& reply_cb);
  Future<Reply> zscan(const String& key, int32 cursor, const String& pattern);

  Client& zscan(const String& key, int32 cursor, int32 count, const ReplyCallback& reply_cb);
  Future<Reply> zscan(const String& key, int32 cursor, int32 count);

  Client& zscan(const String& key, int32 cursor, const String& pattern, int32 count, const ReplyCallback& reply_cb);
  Future<Reply> zscan(const String& key, int32 cursor, const String& pattern, int32 count);

  Client& zscore(const String& key, const String& member, const ReplyCallback& reply_cb);
  Future<Reply> zscore(const String& key, const String& member);

  Client& zunionstore(const String& destination, int32 numkeys, const Array<String>& keys, const Array<int32> weights, AggregateMethod method, const ReplyCallback& reply_cb);
  Future<Reply> zunionstore(const String& destination, int32 numkeys, const Array<String>& keys, const Array<int32> weights, AggregateMethod method);

 private:
  /// Client kill impl
  template <typename T>
  typename EnableIf<IsSame<T, ClientType>::Value>::Type
  client_kill_unpack_arg(Array<String>& redis_cmd, ReplyCallback&, ClientType type);

  template <typename T>
  typename EnableIf<IsSame<T, bool>::Value>::Type
  client_kill_unpack_arg(Array<String>& redis_cmd, ReplyCallback&, bool skip);

  template <typename T>
  typename EnableIf<IsIntegral<T>::Value>::Type
  client_kill_unpack_arg(Array<String>& redis_cmd, ReplyCallback&, uint64 id);

  template <typename T>
  typename EnableIf<IsClass<T>::Value>::Type
  client_kill_unpack_arg(Array<String>&, ReplyCallback& reply_cb, const T& cb);

  template <typename T, typename... Ts>
  void client_kill_impl(Array<String>& redis_cmd, ReplyCallback& reply, const T& arg, const Ts&... args);

  template <typename T>
  void client_kill_impl(Array<String>& redis_cmd, ReplyCallback& reply, const T& arg);

 private:
  /// sort impl
  Client& sort(const String& key, const String& by_pattern, bool limit, int32 offset, int32 count, const Array<String>& get_patterns, bool asc_order, bool alpha, const String& store_dest, const ReplyCallback& reply_cb);

  /// zrevrangebyscore impl
  Client& zrevrangebyscore(const String& key, const String& max, const String& min, bool limit, int32 offset, int32 count, bool withscores, const ReplyCallback& reply_cb);

  /// zrangebyscore impl
  Client& zrangebyscore(const String& key, const String& min, const String& max, bool limit, int32 offset, int32 count, bool withscores, const ReplyCallback& reply_cb);

  /// zrevrangebylex impl
  Client& zrevrangebylex(const String& key, const String& max, const String& min, bool limit, int32 offset, int32 count, bool withscores, const ReplyCallback& reply_cb);

  /// zrangebylex impl
  Client& zrangebylex(const String& key, const String& min, const String& max, bool limit, int32 offset, int32 count, bool withscores, const ReplyCallback& reply_cb);

 private:
  /// redis connection receive handler, triggered whenever a reply has been read by the redis connection
  /// \param conn redis_connection instance
  /// \param reply parsed reply
  void OnConnectionReceive(Connection& conn, Reply& reply);

  /// redis_connection disconnection handler, triggered whenever a disconnection occurred
  /// \param conn redis_connection instance
  void OnConnectionDisconnected(Connection& conn);

  /// reset the queue of pending callbacks
  void ClearCallbacks();

  /// try to Commit the pending pipelined
  /// if Client is disconnected, will throw an exception and clear all pending callbacks (call ClearCallbacks())
  void TryCommit();

  /// Execute a command on the Client and tie the callback to a Future
  Future<Reply> Execute(const Function<Client& (const ReplyCallback&)>& f);

 private:
  /// struct to store commands information (command to be sent and callback to be called)
  struct CommandRequest
  {
    Array<String> command;
    ReplyCallback callback;
  };

 private:
  /// server we are connected to
  String redis_server_;
  /// port we are connected to
  int32 redis_port_ = 0;
  /// master name (if we are using Sentinel) we are connected to
  String master_name_;
  /// password used to authenticate
  String password_;
  /// selected redis db
  int database_index_ = 0;

  /// tcp Client for redis connection
  Connection conn_;

  /// redis Sentinel
  Sentinel sentinel_;

  /// max time to connect
  uint32 connect_timeout_msecs_ = 0;
  /// max number of reconnection attempts
  int32 max_reconnects_ = 0;
  /// current number of attempts to Reconnect
  int32 current_reconnect_attempts_ = 0;
  /// time between two reconnection attempts
  uint32 reconnect_interval_msecs_ = 0;

  /// reconnection status
  tl::atomic_bool reconnecting_;
  /// to force cancel reconnection
  tl::atomic_bool cancel_;

  /// sent commands waiting to be executed
  tl::queue<CommandRequest> commands_;

  /// user defined connect status callback
  ConnectCallback connect_callback_;

  ///  callbacks thread safety
  FastMutex callbacks_mutex_;

  /// condvar for callbacks updates
  tl::condition_variable sync_cv_;

  /// number of callbacks currently being running
  tl::atomic<unsigned int> callbacks_running_;
}; // namespace cpp_redis



//
// inlines
//

template <typename T>
typename EnableIf<IsSame<T, Client::ClientType>::Value>::Type
Client::client_kill_unpack_arg(Array<String>& redis_cmd, ReplyCallback&, ClientType type)
{
  redis_cmd.Add("TYPE");
  String type_string;

  switch (type) {
  case ClientType::normal: type_string = "normal"; break;
  case ClientType::master: type_string = "master"; break;
  case ClientType::pubsub: type_string = "pubsub"; break;
  case ClientType::slave: type_string = "slave"; break;
  }

  redis_cmd.Add(type_string);
}

template <typename T>
typename EnableIf<IsSame<T, bool>::Value>::Type
Client::client_kill_unpack_arg(Array<String>& redis_cmd, ReplyCallback&, bool skip)
{
  redis_cmd.Add("SKIPME");
  redis_cmd.Add(skip ? "yes" : "no");
}

template <typename T>
typename EnableIf<IsIntegral<T>::Value>::Type
Client::client_kill_unpack_arg(Array<String>& redis_cmd, ReplyCallback&, uint64 id)
{
  redis_cmd.Add("ID");
  redis_cmd.Add(fun::ToString(id));
}

template <typename T>
typename EnableIf<IsClass<T>::Value>::Type
Client::client_kill_unpack_arg(Array<String>&, ReplyCallback& reply_cb, const T& cb)
{
  reply_cb = cb;
}

template <typename T, typename... Ts>
void Client::client_kill_impl(Array<String>& redis_cmd, ReplyCallback& reply, const T& arg, const Ts&... args)
{
  static_assert(!IsClass<T>::Value, "Reply callback should be in the end of the argument list");
  client_kill_unpack_arg<T>(redis_cmd, reply, arg);
  client_kill_impl(redis_cmd, reply, args...);
}

template <typename T>
void Client::client_kill_impl(Array<String>& redis_cmd, ReplyCallback& reply, const T& arg)
{
  client_kill_unpack_arg<T>(redis_cmd, reply, arg);
}

template <typename T, typename... Ts>
inline Client& Client::client_kill(const T& arg, const Ts&... args)
{
  //TODO
  //static_assert(helpers::is_different_types<T, Ts...>::Value, "Should only have one distinct value per filter type");
  //static_assert(!(IsClass<T>::Value && IsSame<T, typename helpers::back<T, Ts...>::Type>::Value), "Should have at least one filter");

  Array<String> redis_cmd({"CLIENT", "KILL"});
  ReplyCallback reply_cb = nullptr;
  client_kill_impl<T, Ts...>(redis_cmd, reply_cb, arg, args...);

  return Send(redis_cmd, reply_cb);
}

template <typename T, typename... Ts>
inline Client& Client::client_kill(const String& host, int port, const T& arg, const Ts&... args)
{
  //TODO
  //static_assert(helpers::is_different_types<T, Ts...>::Value, "Should only have one distinct value per filter type");
  Array<String> redis_cmd({"CLIENT", "KILL"});

  //! If we have other type than lambda, then it's a filter
  if (!IsClass<T>::Value)
  {
    redis_cmd.Add("ADDR");
  }

  redis_cmd.Add(host + ":" + fun::ToString(port));
  ReplyCallback reply_cb = nullptr;
  client_kill_impl<T, Ts...>(redis_cmd, reply_cb, arg, args...);

  return Send(redis_cmd, reply_cb);
}

inline Client& Client::client_kill(const String& host, int port)
{
  return client_kill(host, port, ReplyCallback(nullptr));
}

template <typename... Ts>
inline Client& Client::client_kill(const char* host, int port, const Ts&... args)
{
  return client_kill(String(host), port, args...);
}

template <typename T, typename... Ts>
Future<Reply> Client::client_kill_future(const T arg, const Ts... args)
{
  //! gcc 4.8 doesn't handle variadic template capture arguments (appears in 4.9)
  //! so std::bind should capture all arguments because of the compiler.
  return Execute(std::bind([this](T arg, Ts... args, const ReplyCallback& cb) -> Client& {
    return client_kill(arg, args..., cb);
  },
    arg, args..., std::placeholders::_1));
}

} // namespace redis
} // namespace fun
