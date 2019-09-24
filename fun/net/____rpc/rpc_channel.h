#pragma once

#include "fun/net/rpc/rpc.h"
#include "fun/base/noncopyable.h"
#include "fun/base/function.h"


namespace google {
namespace protobuf {

class Descriptor;
class ServiceDescriptor;
class MethodDescriptor;
class Message;

typedef fun::SharedPtr<Message> MessagePtr;

} // namespace protobuf
} // namespace google


namespace fun {
namespace net {
namespace rpc {


class RpcController;
class Service;


/**
Abstract interface for an RPC channel.  An RpcChannel represents a
communication line to a Service which can be used to call that Service's
methods.  The Service may be running on another machine.  Normally, you
should not call an RpcChannel directly, but instead construct a stub Service
wrapping it.  Example:
FIXME: update here
  RpcChannel* channel = new MyRpcChannel("remotehost.example.com:1234");
  MyService* service = new MyService::Stub(channel);
  service->MyMethod(request, &response, callback);
*/
class RpcChannel : Noncopyable {
 public:
  typedef Map<String,Service*> ServiceMap;

  RpcChannel();
  explicit RpcChannel(const TcpConnectionPtr& conn);
  ~RpcChannel();

  void SetConnection(const TcpConnectionPtr& conn) {
    conn_ = conn;
  }

  const SerivceMap* GetSerivces() const {
    return serivces_;
  }

  typedef Function<void (const google::protobuf::MessagePtr&)> ClientDoneCallback;

  void CallMethod(const google::protobuf::MethodDescription* method,
                  const google::protobuf::Message& request,
                  const google::protobuf::Message* response,
                  const ClientDoneCallback& done);

  template <typename Output>
  static void DowncastCall( const Function<void (const SharedPtr<Output>&)>& done,
                            const google::protobuf::MessagePtr& output) {
    done(google::protobuf::down_pointer_cast<Output>(output));
  }

  template <typename Output>
  void CallMethod(const google::protobuf::MethodDescriptor* method,
                  const google::protobuf::Message& request,
                  const Output* response,
                  const Function<void (const SharedPtr<Output>&)>& done) {
    CallMethod(method, request, response, std::bind(&DowncastCall<Output>, done, _1)));
  }

  void OnDisconnect();

  void OnMessage( const TcpConnectionPtr& conn,
                  Buffer* buf,
                  const Timestamp& received_time);

 private:
  void OnRpcMessage(const TcpConnectionPtr& conn,
                    const RpcMessagePtr& message,
                    const Timestamp& received_time);

  void CallServiceMethod(const RpcMessage& message);
  void DoneCallback(const google::protobuf::Message* response_prototype,
                    const google::protobuf::Message* response,
                    int64 id);

  struct OutstandingCall {
    const google::protobuf::Message* response;
    ClientDoneCallback done;
  };

  RpcCodec codec_;
  TcpConnectionPtr conn_;
  AtomicInt64 id_;

  FastMutex mutex_;
  Map<int64,OutstandingCall> outstandings_;

  const ServiceMap* services_;
};

typedef SharedPtr<RpcChannel> RpcChannelPtr;

} // namespace rpc
} // namespace net
} // namespace fun
