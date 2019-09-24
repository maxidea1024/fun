#include "fun/net/rpc/rpc_channel.h"

namespace fun {
namespace net {
namespace rpc {

RpcChannel::RpcChannel()
  : codec_(std::bind(&RpcChannel::OnRpcMessage, this, _1,_2,_3))
  , services_(nullptr)
  , conn_() {
}

RpcChannel::RpcChannel(const TcpConnectionPtr& conn)
  : codec_(std::bind(&RpcChannel::OnRpcMessage, this, _1,_2,_3))
  , services_(nullptr)
  , conn_(conn) {
}

RpcChannel::~RpcChannel() {}

void RpcChannel::CallMethod(const google::protobuf::MethodDescription* method,
                            const google::protobuf::Message& request,
                            const google::protobuf::Message* response,
                            const ClientDoneCallback& done) {
  RpcMessage message;
  message.set_type(REQUEST);
  int64 id = next_id_.IncrementAndGet();
  message.set_service(method->service()->full_name());
  message.set_method(method->name());
  message.set_request(request.SerializeAsString());

  OutstandingCall outstanding = { response, done };
  {
    ScopedLock guard(mutex_);
    outstandings_.Add(id, outstanding);
  }

  codec_.Send(conn_, message);
}

void RpcChannel::OnDisconnect() {
  //TODO???
}

void RpcChannel::OnMessage( const TcpConnectionPtr& conn,
                            Buffer* buf,
                            const Timestamp& received_time) {
  codec_.OnMessage(conn, buf, received_time);
}

void RpcChannel::OnRpcMessage(const TcpConnectionPtr& conn,
                              const RpcMessagePtr& message_ptr,
                              const Timestamp& received_time) {
  RpcMessage& message = *message_ptr;

  if (message.type() == RESPONSE) {
    int64 id = message.id();
    fun_check(message.has_response());

    // corresponding outstanding.
    OutstandingCall outstanding = { nullptr, nullptr };
    bool found = false;

    {
      ScopedLock guard(mutex_);
      auto it = outstandings_.find(id);
      if (it != outstandings_.end()) {
        outstanding = it->second;
        outstandings_.erase(it);
        found = true;
      } else {
        // not found
        //TODO error handling..
      }
    }

    if (!found) {
      LOG_ERROR << "Unknown RESPONSE";
    }

    if (outstanding.response) {
      google::protobuf::MessagePtr response(outstanding.response->New());
      response->ParseFromString(message.response());
      if (outstanding.done) {
        outstanding.done(response);
      }
    }
    else {
      LOG_ERROR << "No response prototype";
    }
  } else if (message.type() == REQUEST) {
    CallServiceMethod(message);
  } else if (message.type() == ERROR) {
    //TODO???
  }
}

void RpcChannel::CallServiceMethod(const RpcMessage& message) {
  if (services_) {
    auto it = services_->find(message.service());
    if (it != services_->end()) {
      Service* service = it->second;
      fun_check_ptr(service);

      const google::protobuf::ServiceDescriptor* desc = service->GetDescriptor();
      const google::protobuf::MethodDescription* method = desc->FindMethodByName(message.method());

      if (method) {
        google::protobuf::MessagePtr request(service->GetRequestPrototype(method).New());
        request->ParseFromString(message.request());
        int64 id = message.id();
        const google::protobuf::Message* response_prototype = &service->GetResponsePrototype(method);
        service->CallMethod(method, request, response_prototype,
                std::bind(&RpcChannel::DoneCallback, this, response_prototype, _1, id));
      }
      else {
        //TODO???
      }
    } else {
      //TODO???
    }
  } else {
    //TODO???
  }
}

void RpcChannel::DoneCallback(const google::protobuf::Message* response_prototype,
                              const google::protobuf::Message* response,
                              int64 id) {
  RpcMessage message;
  message.set_type(RESPONSE);
  message.set_id(id);
  message.set_response(response->SerializeAsString());
  codec_.Send(conn_, message);
}

} // namespace rpc
} // namespace net
} // namespace fun
