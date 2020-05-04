#include "fun/redis/subscriber.h"

/*

자동 재접속시, 이전에 subscribied 된 내용을 있었다면, 다시 적용해주어야함.
이때 auth 절차도 있었다면 이또한 재인증을 해주어야함.

*/

namespace fun {
namespace redis {

Subscriber::Subscriber()
    : conn_(),
      auth_reply_cb_(nullptr),
      connected_cb_(nullptr),
      disconnected_cb_(nullptr),
      auto_commit_(true) {}

Subscriber::Subscriber(const SharedPtr<TcpClient>& tcp_client)
    : conn_(tcp_client),
      auth_reply_cb_(nullptr),
      connected_cb_(nullptr),
      disconnected_cb_(nullptr),
      auto_commit_(true) {}

Subscriber::~Subscriber() { conn_.Disconnect(true); }

void Subscriber::ConnectSync(const String& host, int32 port,
                             const DisconnectedCallback& disconnected_cb) {
  auto disconnected_cb2 = [this](Connection& conn) { OnDisconnected(conn); };
  auto reply_cb2 = [this](Connection& conn, Reply& reply) {
    OnReplyReceivedFromConnection(conn, reply);
  };

  conn_.ConnectSync(host, port, disconnected_cb2, reply_cb2);

  connected_cb_ = nullptr;
  disconnected_cb_ = disconnected_cb;

  LOG(LogRedis, Info, TEXT("Redis::Subscriber::ConnectSync(): connected."));
}

void Subscriber::ConnectAsync(const String& host, int32 port,
                              const ConnectedCallbackType& connected_cb,
                              const DisconnectedCallback& disconnected_cb) {
  auto connected_cb2 = [this](Connection& conn) { OnConnected(conn); };
  auto disconnected_cb2 = [this](Connection& conn) { OnDisconnected(conn); };
  auto reply_cb2 = [this](Connection& conn, Reply& reply) {
    OnReplyReceivedFromConnection(conn, reply);
  };

  conn_.ConnectAsync(host, port, connected_cb2, disconnected_cb2, reply_cb2);

  connected_cb_ = connected_cb;
  disconnected_cb_ = disconnected_cb;

  LOG(LogRedis, Info, TEXT("Redis::Subscriber::ConnectAsync(): connecting."));
}

void Subscriber::Disconnect(bool wait_for_removal) {
  LOG(LogRedis, Info,
      TEXT("Redis::Subscriber::Disconnect(): attempts to disconnect."));
  conn_.Disconnect(wait_for_removal);
  LOG(LogRedis, Info, TEXT("Redis::Subscriber::Disconnect(): disconnected."));
}

bool Subscriber::IsConnected() const { return conn_.IsConnected(); }

bool Subscriber::IsConnecting() const { return conn_.IsConnecting(); }

bool Subscriber::IsDisconnected() const { return conn_.IsDisconnected(); }

Subscriber& Subscriber::Auth(const String& password,
                             const ReplyCallback& reply_cb) {
  LOG(LogRedis, Info,
      TEXT("Redis::Subscriber::Auth(): attempts to authenticate."));
  Send({"AUTH", password});
  auth_reply_cb_ = reply_cb;
  LOG(LogRedis, Info, TEXT("Redis::Subscriber::Auth(): AUTH command sent."));
  return *this;
}

Subscriber& Subscriber::Subscribe(
    const String& channel, const SubscribeCallback& subscribe_cb,
    const AcknowledgementCallback& acknowledgment_cb) {
  std::lock_guard<std::mutex> lock(subscribed_channels_mutex_);
  LOG(LogRedis, Info,
      TEXT("Redis::Subscriber::Subscribe(): attemps to subscribe to channel: "
           "%s"),
      *String(channel));
  // subscribed_channels_[channel] = {subscribe_cb, acknowledgment_cb};
  // //작동안함!
  subscribed_channels_.Add(channel, {subscribe_cb, acknowledgment_cb});
  Send({"SUBSCRIBE", channel});
  LOG(LogRedis, Info,
      TEXT("Redis::Subscriber::Subscribe(): subscribed to channel: %s"),
      *String(channel));
  return *this;
}

Subscriber& Subscriber::PSubscribe(
    const String& pattern, const SubscribeCallback& subscribe_cb,
    const AcknowledgementCallback& acknowledgment_cb) {
  std::lock_guard<std::mutex> lock(p_subscribed_channels_mutex_);
  LOG(LogRedis, Info,
      TEXT("Redis::Subscriber::PSubscribe(): attemps to psubscribe to channel: "
           "%s"),
      *String(pattern));
  // p_subscribed_channels_[pattern] = {subscribe_cb, acknowledgment_cb};
  // //작동안함!
  p_subscribed_channels_.Add(pattern, {subscribe_cb, acknowledgment_cb});
  Send({"PSUBSCRIBE", pattern});
  LOG(LogRedis, Info,
      TEXT("Redis::Subscriber::PSubscribe(): psubscribed to channel: %s"),
      *String(pattern));
  return *this;
}

Subscriber& Subscriber::Unsubscribe(const String& channel) {
  std::lock_guard<std::mutex> lock(subscribed_channels_mutex_);
  LOG(LogRedis, Info,
      TEXT("Redis::Subscriber::Unsubscribe(): attemps to unsubscribe to "
           "channel: %s"),
      *String(channel));
  auto it = subscribed_channels_.CreateKeyIterator(channel);
  if (!it) {
    // TODO 예외를 던지는게 좋으려나??
    LOG(LogRedis, Info,
        TEXT(
            "Redis::Subscriber::Unsubscribe(): was not subscribed to channel."),
        *String(channel));
    return *this;
  }

  Send({"UNSUBSCRIBE", channel});
  it.RemoveCurrent();
  LOG(LogRedis, Info,
      TEXT("Redis::Subscriber::Unsubscribe(): unsubscribed from channel: %s"),
      *String(channel));
  return *this;
}

Subscriber& Subscriber::PUnsubscribe(const String& pattern) {
  std::lock_guard<std::mutex> lock(p_subscribed_channels_mutex_);
  LOG(LogRedis, Info,
      TEXT("Redis::Subscriber::PUnsubscribe(): attemps to punsubscribe to "
           "channel: %s"),
      *String(pattern));
  auto it = p_subscribed_channels_.CreateKeyIterator(pattern);
  if (!it) {
    // TODO 예외를 던지는게 좋으려나??
    LOG(LogRedis, Info,
        TEXT("Redis::Subscriber::PUnsubscribe(): was not psubscribed to "
             "channel: %s"),
        *String(pattern));
    return *this;
  }

  Send({"PUNSUBSCRIBE", pattern});
  it.RemoveCurrent();
  LOG(LogRedis, Info,
      TEXT("Redis::Subscriber::PUnsubscribe(): punsubscribed from channel: %s"),
      *String(pattern));
  return *this;
}

void Subscriber::SetAutoCommit(bool auto_commit) {
  if (auto_commit != auto_commit_) {
    auto_commit_ = auto_commit;

    if (auto_commit_) {
      Commit();
    }
  }
}

bool Subscriber::GetAutoCommit() const { return auto_commit_; }

Subscriber& Subscriber::Commit() {
  try {
    LOG(LogRedis, Info,
        TEXT("Redis::Subscriber::Commit(): attempts to send pipelined "
             "commands."));
    conn_.Commit();
    LOG(LogRedis, Info,
        TEXT("Redis::Subscriber::Commit(): sent pipelined commands."));
  } catch (Exception& e) {
    LOG(LogRedis, Info,
        TEXT(
            "Redis::Subscriber::Commit(): could not send pipelined commands."));
    e.Rethrow();
  }

  return *this;
}

Connection& Subscriber::Send(const TArray<String>& redis_cmd) {
  conn_.Send(redis_cmd);

  if (auto_commit_) {
    Commit();
  }

  return conn_;
}

void Subscriber::OnConnected(Connection& conn) {
  if (connected_cb_) {
    connected_cb_(*this);
  }
}

void Subscriber::OnDisconnected(Connection& conn) {
  if (disconnected_cb_) {
    disconnected_cb_(*this);
  }
}

void Subscriber::OnReplyReceivedFromConnection(Connection& conn, Reply& reply) {
  if (!reply.IsArray()) {
    if (auth_reply_cb_) {
      LOG(LogRedis, Info,
          TEXT("Redis::Subscriber::OnReplyReceivedFromConnection(): executes "
               "auth callback."));

      auth_reply_cb_(reply);
      auth_reply_cb_ = nullptr;
    }

    return;
  }

  auto& array = reply.AsArray();

  // ArrayNum=3 and array[2].IsString()   ->  SUBSCRIBE
  // ArrayNum=3 and array[2].IsInteger()  ->  AKNOWLEDGEMENT
  // ArrayNum=4                           ->  PSUBSCRIBE
  // otherwise                            ->  Unexpected reply

  if (array.Count() == 3 && array[2].IsInteger()) {
    OnAcknowledgementReply(array);
  } else if (array.Count() == 3 && array[2].IsString()) {
    OnSubscribeReply(array);
  } else if (array.Count() == 4) {
    OnPSubscribeReply(array);
  } else {
    // Unexpected reply
    // TODO error handling
  }
}

void Subscriber::OnAcknowledgementReply(const TArray<Reply>& reply) {
  fun_check(reply.Count() == 3);

  const auto& title = reply[0];
  const auto& channel = reply[1];
  const auto& channel_count = reply[2];

  if (!title.IsString() || !channel.IsString() || !channel_count.IsInteger()) {
    return;
  }

  if (title.AsString() == "subscribe") {
    CallOnAcknowledgementHandler(channel.AsString(), subscribed_channels_,
                                 subscribed_channels_mutex_,
                                 channel_count.AsInteger());
  } else if (title.AsString() == "psubscribe") {
    CallOnAcknowledgementHandler(channel.AsString(), p_subscribed_channels_,
                                 p_subscribed_channels_mutex_,
                                 channel_count.AsInteger());
  }
}

void Subscriber::OnSubscribeReply(const TArray<Reply>& reply) {
  fun_check(reply.Count() == 3);

  const auto& title = reply[0];
  const auto& channel = reply[1];
  const auto& message = reply[2];

  if (!title.IsString() || !channel.IsString() || !message.IsString()) {
    return;
  }

  if (title.AsString() != "message") {
    return;
  }

  std::lock_guard<std::mutex> lock(subscribed_channels_mutex_);
  auto it = subscribed_channels_.CreateKeyIterator(channel.AsString());
  if (it) {
    LOG(LogRedis, Info,
        TEXT("Redis::Subscriber::OnSubscribeReply(): executes subscribe "
             "callback for channel: %s"),
        *String(channel.AsString()));
    it.Value().subscribe_cb(channel.AsString(), message.AsString());
  }
}

void Subscriber::OnPSubscribeReply(const TArray<Reply>& reply) {
  fun_check(reply.Count() == 4);

  const auto& title = reply[0];
  const auto& pchannel = reply[1];
  const auto& channel = reply[2];
  const auto& message = reply[3];

  // validation reply types.
  if (!title.IsString() || !pchannel.IsString() || !channel.IsString() ||
      !message.IsString()) {
    return;
  }

  if (title.AsString() != "pmessage") {
    return;
  }

  std::lock_guard<std::mutex> lock(p_subscribed_channels_mutex_);
  auto it = p_subscribed_channels_.CreateKeyIterator(pchannel.AsString());
  if (it) {
    LOG(LogRedis, Info,
        TEXT("Redis::Subscriber::OnPSubscribeReply(): executes psubscribe "
             "callback for channel: %s"),
        *String(pchannel.AsString()));
    it.Value().subscribe_cb(channel.AsString(), message.AsString());
  }
}

void Subscriber::CallOnAcknowledgementHandler(
    const String& channel, const TMap<String, CallbackHolder>& channels,
    std::mutex& channels_mutex, int64 channel_count) {
  std::lock_guard<std::mutex> lock(channels_mutex);
  auto it = channels.CreateConstKeyIterator(channel);
  if (it && it.Value().acknowledgment_cb) {
    LOG(LogRedis, Info,
        TEXT("Redis::Subscriber::CallOnAcknowledgementHandler(): executes "
             "acknowledgement callback for channel: %s"),
        *String(channel));
    it.Value().acknowledgment_cb(channel_count);
  }
}

}  // namespace redis
}  // namespace fun
