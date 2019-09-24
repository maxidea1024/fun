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

#ifndef NDEBUG
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_base_of.hpp>
#endif

typedef fun::SharedPtr<google::protobuf::Message> MessagePtr;

class Callback : Noncopyable {
 public:
  virtual ~Callback(){};

  virtual void OnMessage(const fun::net::TcpConnectionPtr&,
                         const MessagePtr& message,
                         const fun::Timestamp&) const = 0;
};

template <typename T>
class CallbackT : public Callback {
#ifndef NDEBUG
  BOOST_STATIC_ASSERT((boost::is_base_of<google::protobuf::Message, T>::value));
#endif
 public:
  typedef Function<void(const fun::net::TcpConnectionPtr&,
                        const fun::SharedPtr<T>& message,
                        const fun::Timestamp&)>
      ProtobufMessageTCallback;

  CallbackT(const ProtobufMessageTCallback& cb) : callback_(cb) {}

  virtual void OnMessage(const fun::net::TcpConnectionPtr& conn,
                         const MessagePtr& message,
                         const fun::Timestamp& received_time) const {
    fun::SharedPtr<T> concrete = fun::down_pointer_cast<T>(message);
    fun_check(concrete != NULL);
    callback_(conn, concrete, received_time);
  }

 private:
  ProtobufMessageTCallback callback_;
};

class ProtobufDispatcher {
 public:
  typedef Function<void(const fun::net::TcpConnectionPtr&,
                        const MessagePtr& message, const fun::Timestamp&)>
      ProtobufMessageCallback;

  explicit ProtobufDispatcher(const ProtobufMessageCallback& default_cb)
      : default_cb_(default_cb) {}

  void OnProtobufMessage(const fun::net::TcpConnectionPtr& conn,
                         const MessagePtr& message,
                         const fun::Timestamp& received_time) const {
    CallbackMap::const_iterator it = callbacks_.find(message->GetDescriptor());
    if (it != callbacks_.end()) {
      it->second->OnMessage(conn, message, received_time);
    } else {
      default_cb_(conn, message, received_time);
    }
  }

  template <typename T>
  void RegisterMessageCallback(
      const typename CallbackT<T>::ProtobufMessageTCallback& cb) {
    fun::SharedPtr<CallbackT<T> > pd(new CallbackT<T>(cb));
    callbacks_[T::descriptor()] = pd;
  }

 private:
  typedef std::map<const google::protobuf::Descriptor*,
                   fun::SharedPtr<Callback> >
      CallbackMap;

  CallbackMap callbacks_;
  ProtobufMessageCallback default_cb_;
};
