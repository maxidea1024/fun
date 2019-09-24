#include "CorePrivatePCH.h"
#include "fun/net/net.h"

namespace fun {
namespace net {

RpcProxy::RpcProxy()
  : signature_(1),
    core_(nullptr),
    engine_specific_only_(false),

  // 이걸 켜는 순간 성능저하가 매우 심각 할수 있으므로,
  // 필요한 경우에만 활성화함.
    notify_send_by_proxy_enabled_(false) {}

RpcProxy::~RpcProxy() {
  if (core_) {
    LOG(LogNetEngine, Warning, "RPC proxy which is still in use by FunNet core cannot be destroyed! destroy NetClient or NetServer instance first.");
  }
}

bool RpcProxy::RpcCall( const HostId* rpc_sendto_list,
                        int32 rpc_sendto_count,
                        const RpcCallOption& rpc_call_opt,
                        const MessageOut& message,
                        const char* rpc_name,
                        RpcId rpc_id,
                        const char* arguments) {
  if (core_ == nullptr) {
    LOG(LogNetEngine, Warning, "FunNet RPC proxy is not attached yet!");
    return false;
  }

  rpc_call_opt.AssureValidation();

  MessageOut header;

  // 메시지 타입을 기록합니다.
  LiteFormat::Write(header, MessageType::RPC);

  //note: RPC header는 각각의 poroxy에서 기록됩니다.
  // RPC header.
  //RpcHeader::WriteOk(header);

  SendFragRefs frag_refs;
  frag_refs.Add(header); // header

  //Message의 처음 부분은 다음과 같아야함.
  //  rpc_id
  //  result_code
  //  error_message
  // 생성된 Proxy코드에서 기계적으로 넣어주무로, 별도로 신경쓸 필요는 없음.
  frag_refs.Add(message); // message


  //debugging
#if 0
  //----------------------------------------------------------------------
  ByteArray HexDump;
  ByteArray::BytesToDebuggableString(HexDump, frag_refs.ToBytes(), false, "  << ");

  //이렇게만 하기에는 NetServer/LanServer/NetClient/LanClient가 있네그려...
  //LanServer/LanClient는 제거하는게 여러모로 좋을듯도...

  //const char* sender = core_->IsServer() ? "NetServer" : "NetClient";

  const ByteArray Dump = frag_refs.ToBytes().ToHex();
  LOG(LogNetEngine,Info,"RpcCall: NAME=%s, LEN=%d\n%s", rpc_name, frag_refs.GetTotalLength(), *String(HexDump));
  //----------------------------------------------------------------------
#endif

  const bool result = core_->Send(frag_refs, SendOption(rpc_call_opt), rpc_sendto_list, rpc_sendto_count);

  // Call back the event that called RPC.
  if (arguments && (notify_send_by_proxy_enabled_ && !engine_specific_only_)) {
    //@todo 사용자 레벨에서 요구할 경우에만 처리하는게 좋을듯 싶음..
    MessageSummary summary;
    summary.payload_length = frag_refs.GetTotalLength();
    summary.rpc_id = rpc_id;
    summary.rpc_name = rpc_name;
    summary.encryption_mode = rpc_call_opt.encryption_mode;
    summary.compression_mode = rpc_call_opt.compression_mode;
    summary.arguments = arguments;

    for (int32 i = 0; i < rpc_sendto_count; ++i) {
      core_->NotifySendByProxy(rpc_sendto_list[i], summary, rpc_call_opt);
    }

    core_->Viz_NotifySendByProxy(rpc_sendto_list, rpc_sendto_count, summary, rpc_call_opt);
  }

  return result;
}

bool RpcProxy::RpcFailure(const HostId* rpc_sendto_list,
                          int32 rpc_sendto_count,
                          const RpcCallOption& rpc_call_opt,
                          RpcId rpc_id,
                          int32 result_code,
                          const char* error_message) {
  if (core_ == nullptr) {
    LOG(LogNetEngine, Warning, "FunNet RPC proxy is not attached yet!");
    return false;

  rpc_call_opt.AssureValidation();

  MessageOut header;
  LiteFormat::Write(header, MessageType::RPC);  // message-type
  LiteFormat::Write(header, rpc_id);            // stub id
  RpcHeader::Write(header, result_code, error_message ? error_message : ""); // header

  const bool result = core_->Send(SendFragRefs(header), SendOption(rpc_call_opt), rpc_sendto_list, rpc_sendto_count);

  // Call back the event that called RPC.
  if (notify_send_by_proxy_enabled_ && !engine_specific_only_) {
    //@todo 사용자 레벨에서 요구할 경우에만 처리하는게 좋을듯 싶음..
    MessageSummary summary;
    summary.payload_length = 0;
    summary.rpc_id = (RpcId)65535; //TODO 따로 정의해 주는게 좋을듯함..
    summary.rpc_name = "RpcFailure";
    summary.encryption_mode = rpc_call_opt.encryption_mode;
    summary.compression_mode = rpc_call_opt.compression_mode;
    summary.arguments = "{}";

    for (int32 i = 0; i < rpc_sendto_count; ++i) {
      core_->NotifySendByProxy(rpc_sendto_list[i], summary, rpc_call_opt);
    }

    core_->Viz_NotifySendByProxy(rpc_sendto_list, rpc_sendto_count, summary, rpc_call_opt);
  }

  return result;
}

} // namespace net
} // namespace fun
