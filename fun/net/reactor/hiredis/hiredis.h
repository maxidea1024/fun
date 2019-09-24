#pragma once

namespace hiredis {

class Hiredis : public boost::enable_shared_from_this<Hiredis>,
                boost::noncopyable {
 public:
  typedef boost::function<void(Hiredis*, int)> ConnectCallback;
  typedef boost::function<void(Hiredis*, int)> DisconnectCallback;
  typedef boost::function<void(Hiredis*, redisReply*)> CommandCallback;

  Hiredis(fun::net::EventLoop* loop, const fun::net::InetAddress& serverAddr);
  ~Hiredis();

  const fun::net::InetAddress& serverAddress() const {
      return server_addr_;
  }
  // redisAsyncContext* context() { return context_; }
  bool connected() const;
  const char* errstr() const;

  void setConnectCallback(const ConnectCallback& cb) {
    connectCb_ = cb;
  }

  void setDisconnectCallback(const DisconnectCallback& cb) { disconnectCb_ = cb; }

  void connect();
  void disconnect();  // FIXME: implement this with redisAsyncDisconnect

  int command(const CommandCallback& cb, fun::StringArg cmd, ...);

  int ping();

 private:
  void handleRead(fun::Timestamp receiveTime);
  void handleWrite();

  int fd() const;
  void logConnection(bool up) const;
  void setChannel();
  void RemoveChannel();

  void connectCallback(int status);
  void disconnectCallback(int status);
  void commandCallback(redisReply* reply, CommandCallback* privdata);

  static Hiredis* getHiredis(const redisAsyncContext* ac);

  static void connectCallback(const redisAsyncContext* ac, int status);
  static void disconnectCallback(const redisAsyncContext* ac, int status);
  // command callback
  static void commandCallback(redisAsyncContext* ac, void*, void*);

  static void addRead(void* privdata);
  static void delRead(void* privdata);
  static void addWrite(void* privdata);
  static void delWrite(void* privdata);
  static void cleanup(void* privdata);

  void pingCallback(Hiredis* me, redisReply* reply);

 private:
  fun::net::EventLoop* loop_;
  const fun::net::InetAddress server_addr_;
  redisAsyncContext* context_;
  fun::SharedPtr<fun::net::Channel> channel_;
  ConnectCallback connectCb_;
  DisconnectCallback disconnectCb_;
};

}
