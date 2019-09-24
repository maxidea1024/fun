// Copyright 2011, Shuo Chen.  All rights reserved.
// http://code.google.com/p/red/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#pragma once

#include <red/net/Callbacks.h>

#include <google/protobuf/message.h>

#include <map>

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

typedef fun::SharedPtr<google::protobuf::Message> MessagePtr;

class ProtobufDispatcherLite : Noncopyable {
 public:
  typedef Function<void(const fun::net::TcpConnectionPtr&, const MessagePtr&,
                        const fun::Timestamp&)>
      ProtobufMessageCallback;

  // ProtobufDispatcher()
  //   : default_cb_(discardProtobufMessage)
  // {
  // }

  explicit ProtobufDispatcherLite(const ProtobufMessageCallback& default_cb)
      : default_cb_(default_cb) {}

  void OnProtobufMessage(const fun::net::TcpConnectionPtr& conn,
                         const MessagePtr& message,
                         const fun::Timestamp& received_time) const {
    CallbackMap::const_iterator it = callbacks_.find(message->GetDescriptor());
    if (it != callbacks_.end()) {
      it->second(conn, message, received_time);
    } else {
      default_cb_(conn, message, received_time);
    }
  }

  void RegisterMessageCallback(const google::protobuf::Descriptor* desc,
                               const ProtobufMessageCallback& cb) {
    callbacks_[desc] = cb;
  }

 private:
  // static void discardProtobufMessage(const fun::net::TcpConnectionPtr&,
  //                                    const MessagePtr&,
  //                                    fun::Timestamp);

  typedef std::map<const google::protobuf::Descriptor*, ProtobufMessageCallback>
      CallbackMap;
  CallbackMap callbacks_;
  ProtobufMessageCallback default_cb_;
};
