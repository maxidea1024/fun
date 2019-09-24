#include "Hiredis.h"

#include "fun/base/logging.h"
#include "fun/net/channel.h"
#include <red/net/EventLoop.h>
#include <red/net/SocketsOps.h>

#include <hiredis/async.h>

using namespace fun;
using namespace fun::net;
using namespace hiredis;

static void dummy(const fun::SharedPtr<Channel>&) {
}

Hiredis::Hiredis(EventLoop* loop, const InetAddress& serverAddr)
  : loop_(loop)
  , server_addr_(serverAddr)
  , context_(NULL) {}

Hiredis::~Hiredis() {
  LOG_DEBUG << this;
  assert(!channel_ || channel_->isNoneEvent());
  ::redisAsyncFree(context_);
}

bool Hiredis::connected() const {
  return channel_ && context_ && (context_->c.flags & REDIS_CONNECTED);
}

const char* Hiredis::errstr() const {
  assert(context_ != NULL);
  return context_->errstr;
}

void Hiredis::connect() {
  assert(!context_);

  context_ = ::redisAsyncConnect(server_addr_.toIp().c_str(), server_addr_.toPort());

  context_->ev.addRead = addRead;
  context_->ev.delRead = delRead;
  context_->ev.addWrite = addWrite;
  context_->ev.delWrite = delWrite;
  context_->ev.cleanup = cleanup;
  context_->ev.data = this;

  setChannel();

  assert(context_->onConnect == NULL);
  assert(context_->onDisconnect == NULL);
  ::redisAsyncSetConnectCallback(context_, connectCallback);
  ::redisAsyncSetDisconnectCallback(context_, disconnectCallback);
}

void Hiredis::disconnect() {
  if (connected()) {
    LOG_DEBUG << this;
    ::redisAsyncDisconnect(context_);
  }
}

int Hiredis::fd() const {
  assert(context_);
  return context_->c.fd;
}

void Hiredis::setChannel() {
  LOG_DEBUG << this;
  assert(!channel_);
  channel_.Reset(new Channel(loop_, fd()));
  channel_->setReadCallback(boost::bind(&Hiredis::handleRead, this, _1));
  channel_->setWriteCallback(boost::bind(&Hiredis::handleWrite, this));
}

void Hiredis::RemoveChannel() {
  LOG_DEBUG << this;
  channel_->DisableAll();
  channel_->Remove();
  loop_->QueueInLoop(boost::bind(dummy, channel_));
  channel_.Reset();
}

void Hiredis::handleRead(fun::Timestamp receiveTime) {
  LOG_TRACE << "receiveTime = " << receiveTime.toString();
  ::redisAsyncHandleRead(context_);
}

void Hiredis::handleWrite() {
  if (!(context_->c.flags & REDIS_CONNECTED)) {
    RemoveChannel();
  }
  ::redisAsyncHandleWrite(context_);
}

/* static */ Hiredis* Hiredis::getHiredis(const redisAsyncContext* ac) {
  Hiredis* hiredis = static_cast<Hiredis*>(ac->ev.data);
  assert(hiredis->context_ == ac);
  return hiredis;
}

void Hiredis::logConnection(bool up) const {
  InetAddress localAddr(sockets::getLocalAddr(fd()));
  InetAddress peerAddr(sockets::getPeerAddr(fd()));

  LOG_INFO << localAddr.toIpPort() << " -> "
           << peerAddr.toIpPort() << " is "
           << (up ? "UP" : "DOWN");
}

/* static */ void Hiredis::connectCallback(const redisAsyncContext* ac, int status) {
  LOG_TRACE;
  getHiredis(ac)->connectCallback(status);
}

void Hiredis::connectCallback(int status) {
  if (status != REDIS_OK) {
    LOG_ERROR << context_->errstr << " failed to connect to " << server_addr_.toIpPort();
  } else {
    logConnection(true);
    setChannel();
  }

  if (connectCb_) {
    connectCb_(this, status);
  }
}

/* static */ void Hiredis::disconnectCallback(const redisAsyncContext* ac, int status) {
  LOG_TRACE;
  getHiredis(ac)->disconnectCallback(status);
}

void Hiredis::disconnectCallback(int status) {
  logConnection(false);
  RemoveChannel();

  if (disconnectCb_) {
    disconnectCb_(this, status);
  }
}

void Hiredis::addRead(void* privdata) {
  LOG_TRACE;
  Hiredis* hiredis = static_cast<Hiredis*>(privdata);
  hiredis->channel_->enableReading();
}

void Hiredis::delRead(void* privdata) {
  LOG_TRACE;
  Hiredis* hiredis = static_cast<Hiredis*>(privdata);
  hiredis->channel_->disableReading();
}

void Hiredis::addWrite(void* privdata) {
  LOG_TRACE;
  Hiredis* hiredis = static_cast<Hiredis*>(privdata);
  hiredis->channel_->enableWriting();
}

void Hiredis::delWrite(void* privdata) {
  LOG_TRACE;
  Hiredis* hiredis = static_cast<Hiredis*>(privdata);
  hiredis->channel_->disableWriting();
}

void Hiredis::cleanup(void* privdata) {
  Hiredis* hiredis = static_cast<Hiredis*>(privdata);
  LOG_DEBUG << hiredis;
}

int Hiredis::command(const CommandCallback& cb, fun::StringArg cmd, ...) {
  if (!connected()) return REDIS_ERR;

  LOG_TRACE;
  CommandCallback* p = new CommandCallback(cb);
  va_list args;
  va_start(args, cmd);
  int ret = ::redisvAsyncCommand(context_, commandCallback, p, cmd.c_str(), args);
  va_end(args);
  return ret;
}

/* static */ void Hiredis::commandCallback(redisAsyncContext* ac, void* r, void* privdata) {
  redisReply* reply = static_cast<redisReply*>(r);
  CommandCallback* cb = static_cast<CommandCallback*>(privdata);
  getHiredis(ac)->commandCallback(reply, cb);
}

void Hiredis::commandCallback(redisReply* reply, CommandCallback* cb) {
  (*cb)(this, reply);
  delete cb;
}

int Hiredis::ping() {
  return command(boost::bind(&Hiredis::pingCallback, this, _1, _2), "PING");
}

void Hiredis::pingCallback(Hiredis* me, redisReply* reply) {
  assert(this == me);
  LOG_DEBUG << reply->str;
}
