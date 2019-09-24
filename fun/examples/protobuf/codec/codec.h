// Copyright 2011, Shuo Chen.  All rights reserved.
// http://code.google.com/p/red/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#pragma once

#include <red/net/Buffer.h>
#include "fun/net/tcp_connection.h"

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include <google/protobuf/message.h>

// struct ProtobufTransportFormat __attribute__ ((__packed__))
// {
//   int32_t  len;
//   int32_t  nameLen;
//   char     typeName[nameLen];
//   char     protobufData[len-nameLen-8];
//   int32_t  checkSum; // adler32 of nameLen, typeName and protobufData
// }

typedef fun::SharedPtr<google::protobuf::Message> MessagePtr;

//
// FIXME: merge with RpcCodec
//
class ProtobufCodec : Noncopyable
{
 public:

  enum ErrorCode
  {
    kNoError = 0,
    kInvalidLength,
    kCheckSumError,
    kInvalidNameLen,
    kUnknownMessageType,
    kParseError,
  };

  typedef Function<void (const fun::net::TcpConnectionPtr&,
                            const MessagePtr&,
                            const fun::Timestamp&)> ProtobufMessageCallback;

  typedef Function<void (const fun::net::TcpConnectionPtr&,
                                fun::net::Buffer*,
                                const fun::Timestamp&,
                                ErrorCode)> ErrorCallback;

  explicit ProtobufCodec(const ProtobufMessageCallback& message_cb)
    : message_cb_(message_cb),
      error_cb_(DefaultErrorCallback)
  {
  }

  ProtobufCodec(const ProtobufMessageCallback& message_cb, const ErrorCallback& error_cb)
    : message_cb_(message_cb),
      error_cb_(error_cb)
  {
  }

  void OnMessage(const fun::net::TcpConnectionPtr& conn,
                 fun::net::Buffer* buf,
                 const fun::Timestamp& received_time);

  void Send(const fun::net::TcpConnectionPtr& conn,
            const google::protobuf::Message& message)
  {
    // FIXME: serialize to TcpConnection::GetOutputBuffer()
    fun::net::Buffer buf;
    FillEmptyBuffer(&buf, message);
    conn->Send(&buf);
  }

  static const String& ErrorCodeToString(ErrorCode error_code);

  static void FillEmptyBuffer(fun::net::Buffer* buf,
                              const google::protobuf::Message& message);

  static google::protobuf::Message* CreateMessage(const String& type_name);

  static MessagePtr Parse(const char* buf, int len, ErrorCode* error_code);

 private:
  static void DefaultErrorCallback(const fun::net::TcpConnectionPtr&,
                                   fun::net::Buffer*,
                                   const fun::Timestamp&,
                                   ErrorCode);

  ProtobufMessageCallback message_cb_;
  ErrorCallback error_cb_;

  const static int kHeaderLen = sizeof(int32_t);
  const static int kMinMessageLen = 2*kHeaderLen + 2; // nameLen + typeName + checkSum
  const static int kMaxMessageLen = 64*1024*1024; // same as codec_stream.h kDefaultTotalBytesLimit
};
