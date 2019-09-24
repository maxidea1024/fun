#pragma once

#include "fun/redis/redis.h"
#include "fun/redis/connection.h"
#include <mutex> //std::mutex

namespace fun {
namespace redis {

class FUN_REDIS_API Subscriber {
 public:
  Subscriber();
  Subscriber(const SharedPtr<TcpClient>& tcp_client);
  ~Subscriber();

  Subscriber(const Subscriber&) = delete;
  Subscriber& operator = (const Subscriber&) = delete;

 public:
  typedef TFunction<void(Subscriber&)> ConnectedCallback;
  typedef TFunction<void(Subscriber&)> DisconnectedCallback;
  typedef TFunction<void(Reply&)> ReplyCallback;
  typedef TFunction<void(const String&,const String&)> SubscribeCallback;
  typedef TFunction<void(int64)> AcknowledgementCallback;

  void BlockedConnect(const String& host = "localhost", int32 port = 6379, const DisconnectedCallback& disconnected_cb = nullptr);
  void AsyncConnect(const String& host = "localhost", int32 port = 6379, const ConnectedCallback& connected_cb = nullptr, const DisconnectedCallback& disconnected_cb = nullptr);

  void Disconnect(bool wait_for_removal = false);

  bool IsConnected() const;
  bool IsConnecting() const;
  bool IsDisconnected() const;

  Subscriber& Auth(const String& password, const ReplyCallback& reply_cb = nullptr);

  Subscriber& Subscribe(const String& channel, const SubscribeCallback& subscribe_cb, const AcknowledgementCallback& acknowledgment_cb = nullptr);
  Subscriber& PSubscribe(const String& pattern, const SubscribeCallback& subscribe_cb, const AcknowledgementCallback& acknowledgment_cb = nullptr);
  Subscriber& Unsubscribe(const String& channel);
  Subscriber& PUnsubscribe(const String& pattern);

  void SetAutoCommit(bool auto_commit);
  bool GetAutoCommit() const;

  Subscriber& Commit();

 private:
  struct CallbackHolder {
    SubscribeCallback subscribe_cb;
    AcknowledgementCallback acknowledgment_cb;
  };

 private:
  void OnConnected(Connection& conn);
  void OnDisconnected(Connection& conn);
  void OnReplyReceivedFromConnection(Connection& conn, Reply& reply);

  void OnAcknowledgementReply(const TArray<Reply>& reply);
  void OnSubscribeReply(const TArray<Reply>& reply);
  void OnPSubscribeReply(const TArray<Reply>& reply);

  void CallOnAcknowledgementHandler(const String& channel,
                                    const UnorderedMap<String,CallbackHolder>& channels,
                                    std::mutex& channels_mutex,
                                    int64 channel_count);

 private:
  Connection conn_;

  UnorderedMap<String, CallbackHolder> subscribed_channels_;
  UnorderedMap<String, CallbackHolder> p_subscribed_channels_;

  ConnectedCallback connected_cb;
  DisconnectedCallback disconnected_cb;

  std::mutex subscribed_channels_mutex_;
  std::mutex p_subscribed_channels_mutex_;

  ReplyCallback auth_reply_cb_;

  bool auto_commit;
  Connection& Send(const TArray<String>& redis_cmd);
};

} // namespace redis
} // namespace fun
