﻿/**
 * Auto-generated by IDL Compiler (2.1.3)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 */
#include "lan_LanC2C_stub.h"
#include "lan.h" // _Args, _PArgs

namespace fun {
namespace LanC2C
{
  bool Stub::ProcessReceivedMessage(fun::ReceivedMessage& received_msg, void* host_tag)
  {
    #define DO_CHECKED__(Expr) { if (!(Expr)) goto Failure__; }

    const fun::HostId RemoteId__ = received_msg.remote_id;
    fun::IMessageIn& ImmutableMessage__ = received_msg.unsafe_message;
    const fun::int32 SavedReadPosition__ = ImmutableMessage__.GetPosition();

    fun::RpcHint RpcHint__;
    RpcHint__.relayed = received_msg.relayed;
    RpcHint__.host_tag = host_tag;

    fun::RpcId RpcId__;
    fun::RpcHeader RpcHeader__;
    DO_CHECKED__(fun::LiteFormat::Read(ImmutableMessage__, RpcId__));
    DO_CHECKED__(RpcHeader__.Read(ImmutableMessage__));

    RpcHint__.result_code = RpcHeader__.result_code;
    RpcHint__.error_message = RpcHeader__.error_message;

    switch ((fun::uint32)RpcId__)
    {
      case (fun::uint32)RpcIds::Foo:
      {
          // No arguments.

          Core->PostCheckReadMessage(ImmutableMessage__, RpcIds::Foo, RpcNames::Foo());

          // NotifyCallFromStub
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            Core->NotifyCallFromStub(RpcIds::Foo, RpcNames::Foo(), TEXT("{}"));
          }

          // Before RPC invocation.
          fun::uint32 InvocationTime__ = 0;
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            Core->BeforeRpcInvocation(fun::BeforeRpcSummary(RpcIds::Foo, RpcNames::Foo(), RemoteId__, host_tag));
            InvocationTime__ = fun::Clock::Milliseconds();
          }

          bool bIsImplemented__ = false;
          // First, try function object.
          if ((bool)OnFoo)
          {
            bIsImplemented__ = OnFoo(RemoteId__, RpcHint__);
          }
          // Second, call derived stub function.
          else
          {
            bIsImplemented__ = Foo(RemoteId__, RpcHint__);
          }

          if (!bIsImplemented__)
          {
            Core->ShowNotImplementedRpcWarning(RpcIds::Foo, RpcNames::Foo());
          }

          // After RPC invocation.
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            Core->AfterRpcInvocation(fun::AfterRpcSummary(RpcIds::Foo, RpcNames::Foo(), RemoteId__, host_tag, fun::Clock::Milliseconds() - InvocationTime__));
          }

          return true;
      }
    }

    Failure__: ImmutableMessage__.SetPosition(SavedReadPosition__); return false;
    #undef DO_CHECKED__
  }
} // end of namespace LanC2C

} // end of namespace fun