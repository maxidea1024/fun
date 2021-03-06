﻿/**
 * Auto-generated by IDL Compiler (2.1.3)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 */
#include "viz_VizS2C_stub.h"
#include "viz.h" // _Args, _PArgs

namespace fun {
namespace VizS2C {

bool Stub::ProcessReceivedMessage(fun::ReceivedMessage& received_msg, void* host_tag)
{
  #define DO_CHECKED__(Expr) { if (!(Expr)) goto Failure__; }

  const fun::HostId rpc_recvfrom_11 = received_msg.remote_id;
  fun::IMessageIn& rpc_msg_11 = received_msg.unsafe_message;
  const fun::int32 SavedReadPosition__ = rpc_msg_11.GetPosition();

  fun::RpcHint rpc_hint_11;
  rpc_hint_11.relayed = received_msg.relayed;
  rpc_hint_11.host_tag = host_tag;

  fun::RpcId rpc_id_11;
  fun::RpcHeader rpc_header_11;
  DO_CHECKED__(fun::LiteFormat::Read(rpc_msg_11, rpc_id_11));
  DO_CHECKED__(rpc_header_11.Read(rpc_msg_11));

  rpc_hint_11.result_code = rpc_header_11.result_code;
  rpc_hint_11.error_message = rpc_header_11.error_message;

  switch ((fun::uint32)rpc_id_11) {
    case (fun::uint32)RpcIds::NotifyLoginOk: {
        // No arguments.

        core_->PostCheckReadMessage(rpc_msg_11, RpcIds::NotifyLoginOk, RpcNames::NotifyLoginOk());

        // NotifyCallFromStub
        if (notify_call_from_stub_enabled_ && !engine_specific_only_) {
          core_->NotifyCallFromStub(RpcIds::NotifyLoginOk, RpcNames::NotifyLoginOk(), TEXT("{}"));
        }

        // Before RPC invocation.
        fun::uint32 InvocationTime__ = 0;
        if (notify_call_from_stub_enabled_ && !engine_specific_only_) {
          core_->BeforeRpcInvocation(fun::BeforeRpcSummary(RpcIds::NotifyLoginOk, RpcNames::NotifyLoginOk(), rpc_recvfrom_11, host_tag));
          InvocationTime__ = fun::Clock::Milliseconds();
        }

        bool bIsImplemented__ = false;
        // First, try function object.
        if ((bool)OnNotifyLoginOk) {
          bIsImplemented__ = OnNotifyLoginOk(rpc_recvfrom_11, rpc_hint_11);
        }
        // Second, call derived stub function.
        else {
          bIsImplemented__ = NotifyLoginOk(rpc_recvfrom_11, rpc_hint_11);
        }

        if (!bIsImplemented__) {
          core_->ShowNotImplementedRpcWarning(RpcIds::NotifyLoginOk, RpcNames::NotifyLoginOk());
        }

        // After RPC invocation.
        if (notify_call_from_stub_enabled_ && !engine_specific_only_) {
          core_->AfterRpcInvocation(fun::AfterRpcSummary(RpcIds::NotifyLoginOk, RpcNames::NotifyLoginOk(), rpc_recvfrom_11, host_tag, fun::Clock::Milliseconds() - InvocationTime__));
        }

        return true;
    }

    case (fun::uint32)RpcIds::NotifyLoginFailed: {
        // Declare arguments.
        fun::ResultCode reason = (fun::ResultCode)0;

        // Read arguments.
        if (rpc_hint_11.result_code == 0) {
          DO_CHECKED__(EngineTypes_UserTypeHandlers::Read(rpc_msg_11, reason));
        }

        core_->PostCheckReadMessage(rpc_msg_11, RpcIds::NotifyLoginFailed, RpcNames::NotifyLoginFailed());

        // NotifyCallFromStub
        if (notify_call_from_stub_enabled_ && !engine_specific_only_) {
          fun::String ArgsStr__ = TEXT("{");
          ArgsStr__ += TEXT("\"reason\":") + fun::ToString(reason);
          ArgsStr__ += TEXT("}");

          core_->NotifyCallFromStub(RpcIds::NotifyLoginFailed, RpcNames::NotifyLoginFailed(), ArgsStr__);
        }

        // Before RPC invocation.
        fun::uint32 InvocationTime__ = 0;
        if (notify_call_from_stub_enabled_ && !engine_specific_only_) {
          core_->BeforeRpcInvocation(fun::BeforeRpcSummary(RpcIds::NotifyLoginFailed, RpcNames::NotifyLoginFailed(), rpc_recvfrom_11, host_tag));
          InvocationTime__ = fun::Clock::Milliseconds();
        }

        bool bIsImplemented__ = false;
        // First, try function object.
        if ((bool)OnNotifyLoginFailed) {
          bIsImplemented__ = OnNotifyLoginFailed(rpc_recvfrom_11, rpc_hint_11, reason);
        }
        // Second, call derived stub function.
        else {
          bIsImplemented__ = NotifyLoginFailed(rpc_recvfrom_11, rpc_hint_11, reason);
        }

        if (!bIsImplemented__) {
          core_->ShowNotImplementedRpcWarning(RpcIds::NotifyLoginFailed, RpcNames::NotifyLoginFailed());
        }

        // After RPC invocation.
        if (notify_call_from_stub_enabled_ && !engine_specific_only_) {
          core_->AfterRpcInvocation(fun::AfterRpcSummary(RpcIds::NotifyLoginFailed, RpcNames::NotifyLoginFailed(), rpc_recvfrom_11, host_tag, fun::Clock::Milliseconds() - InvocationTime__));
        }

        return true;
    }
  }

  Failure__: rpc_msg_11.SetPosition(SavedReadPosition__); return false;
  #undef DO_CHECKED__
}

} // end of namespace VizS2C
} // end of namespace fun
