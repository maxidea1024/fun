#pragma once

#include "item.h"

#include "fun/base/logging.h"
#include "fun/net/tcp_connection.h"

#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/tokenizer.hpp>

using String;

class MemcacheServer;

class Session : Noncopyable, public EnableSharedFromThis<Session> {
 public:
  Session(MemcacheServer* owner, const fun::net::TcpConnectionPtr& conn)
      : owner_(owner),
        conn_(conn),
        state_(kNewCommand),
        protocol_(kAscii)  // FIXME
        ,
        noreply_(false),
        policy_(Item::kInvalid),
        bytes_to_discard_(0),
        needle_(Item::makeItem(kLongestKey, 0, 0, 2, 0)),
        bytes_read_(0),
        requests_processed_(0) {
    conn_->SetMessageCallback(
        boost::bind(&Session::OnMessage, this, _1, _2, _3));
  }

  ~Session() {
    LOG_INFO << "requests processed: " << requests_processed_
             << " input buffer size: "
             << conn_->GetInputBuffer()->internalCapacity()
             << " output buffer size: "
             << conn_->GetOutputBuffer()->internalCapacity();
  }

 private:
  enum State {
    kNewCommand,
    kReceiveValue,
    kDiscardValue,
  };

  enum Protocol {
    kAscii,
    kBinary,
    kAuto,
  };

  void OnMessage(const fun::net::TcpConnectionPtr& conn, fun::net::Buffer* buf,
                 const fun::Timestamp&);
  void OnWriteComplete(const fun::net::TcpConnectionPtr& conn);
  void receiveValue(fun::net::Buffer* buf);
  void discardValue(fun::net::Buffer* buf);
  // TODO: highWaterMark
  // TODO: OnWriteComplete

  // returns true if finished a request
  bool ProcessRequest(fun::StringPiece request);
  void resetRequest();
  void reply(fun::StringPiece msg);

  struct SpaceSeparator {
    void Reset() {}
    template <typename InputIterator, typename Token>
    bool operator()(InputIterator& next, InputIterator end, Token& tok);
  };

  typedef boost::tokenizer<SpaceSeparator, const char*, fun::StringPiece>
      Tokenizer;
  struct Reader;
  bool doUpdate(Tokenizer::iterator& beg, Tokenizer::iterator end);
  void doDelete(Tokenizer::iterator& beg, Tokenizer::iterator end);

  MemcacheServer* owner_;
  fun::net::TcpConnectionPtr conn_;
  State state_;
  Protocol protocol_;

  // current request
  String command_;
  bool noreply_;
  Item::UpdatePolicy policy_;
  ItemPtr curr_item_;
  size_t bytes_to_discard_;
  // cached
  ItemPtr needle_;
  fun::net::Buffer output_buf_;

  // per session stats
  size_t bytes_read_;
  size_t requests_processed_;

  static String kLongestKey;
};

typedef fun::SharedPtr<Session> SessionPtr;
