#pragma once

#include "fun/net/tcp_connection.h"
#include <map>

/**
one FastCgiCodec per TcpConnection
both lighttpd and nginx do not implement multiplexing,
so there is no concurrent requests of one connection.
*/
class FastCgiCodec : Noncopyable {
 public:
  typedef std::map<String, String> ParamMap;

  typedef Function<void (const fun::net::TcpConnectionPtr& conn,
                                ParamMap&,
                                fun::net::Buffer*)> Callback;

  explicit FastCgiCodec(const Callback& cb)
    : cb_(cb)
    , got_request_(false)
    , keep_conn_(false) {
  }

  void OnMessage(const fun::net::TcpConnectionPtr& conn,
                 fun::net::Buffer* buf,
                 const fun::Timestamp& received_time) {
    ParseRequest(buf);
    if (got_request_) {
      cb_(conn, params_, &stdin_);
      stdin_.DrainAll();
      params_stream_.DrainAll();
      params_.clear();
      got_request_ = false;
      if (!keep_conn_)
      {
        conn->Shutdown();
      }
    }
  }

  static void Respond(fun::net::Buffer* response);

 private:
  struct RecordHeader;
  bool ParseRequest(fun::net::Buffer* buf);
  bool OnBeginRequest(const RecordHeader& header, const fun::net::Buffer* buf);
  void OnStdin(const char* content, uint16_t length);
  bool OnParams(const char* content, uint16_t length);
  bool ParseAllParams();
  uint32_t ReadLength();

  static void EndStdout(fun::net::Buffer* buf);
  static void EndRequest(fun::net::Buffer* buf);

  Callback cb_;
  bool got_request_;
  bool keep_conn_;
  fun::net::Buffer stdin_;
  fun::net::Buffer params_stream_;
  ParamMap params_;

  const static unsigned kRecordHeader;
};
