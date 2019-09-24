﻿/**
 * Auto-generated by IDL Compiler (2.1.3)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 */
#include "dump_DumpC2S_stub.h"
#include "dump.h" // _Args, _PArgs

namespace fun {
namespace DumpC2S
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
      case (fun::uint32)RpcIds::Start:
      {
          // No arguments.

          Core->PostCheckReadMessage(ImmutableMessage__, RpcIds::Start, RpcNames::Start());

          // NotifyCallFromStub
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            Core->NotifyCallFromStub(RpcIds::Start, RpcNames::Start(), TEXT("{}"));
          }

          // Before RPC invocation.
          fun::uint32 InvocationTime__ = 0;
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            Core->BeforeRpcInvocation(fun::BeforeRpcSummary(RpcIds::Start, RpcNames::Start(), RemoteId__, host_tag));
            InvocationTime__ = fun::Clock::Milliseconds();
          }

          bool bIsImplemented__ = false;
          // First, try function object.
          if ((bool)OnStart)
          {
            bIsImplemented__ = OnStart(RemoteId__, RpcHint__);
          }
          // Second, call derived stub function.
          else
          {
            bIsImplemented__ = Start(RemoteId__, RpcHint__);
          }

          if (!bIsImplemented__)
          {
            Core->ShowNotImplementedRpcWarning(RpcIds::Start, RpcNames::Start());
          }

          // After RPC invocation.
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            Core->AfterRpcInvocation(fun::AfterRpcSummary(RpcIds::Start, RpcNames::Start(), RemoteId__, host_tag, fun::Clock::Milliseconds() - InvocationTime__));
          }

          return true;
      }

      case (fun::uint32)RpcIds::Chunk:
      {
          // Declare arguments.
          fun::ByteArray Chunk;

          // Read arguments.
          if (RpcHint__.result_code == 0)
          {
            DO_CHECKED__(fun::FlexFormat::ReadBytes(ImmutableMessage__, Chunk));
          }

          Core->PostCheckReadMessage(ImmutableMessage__, RpcIds::Chunk, RpcNames::Chunk());

          // NotifyCallFromStub
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            fun::String ArgsStr__ = TEXT("{");
            ArgsStr__ += TEXT("\"Chunk\":") + fun::ToString(Chunk);
            ArgsStr__ += TEXT("}");

            Core->NotifyCallFromStub(RpcIds::Chunk, RpcNames::Chunk(), ArgsStr__);
          }

          // Before RPC invocation.
          fun::uint32 InvocationTime__ = 0;
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            Core->BeforeRpcInvocation(fun::BeforeRpcSummary(RpcIds::Chunk, RpcNames::Chunk(), RemoteId__, host_tag));
            InvocationTime__ = fun::Clock::Milliseconds();
          }

          bool bIsImplemented__ = false;
          // First, try function object.
          if ((bool)OnChunk)
          {
            bIsImplemented__ = OnChunk(RemoteId__, RpcHint__, Chunk);
          }
          // Second, call derived stub function.
          else
          {
            bIsImplemented__ = Chunk(RemoteId__, RpcHint__, Chunk);
          }

          if (!bIsImplemented__)
          {
            Core->ShowNotImplementedRpcWarning(RpcIds::Chunk, RpcNames::Chunk());
          }

          // After RPC invocation.
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            Core->AfterRpcInvocation(fun::AfterRpcSummary(RpcIds::Chunk, RpcNames::Chunk(), RemoteId__, host_tag, fun::Clock::Milliseconds() - InvocationTime__));
          }

          return true;
      }

      case (fun::uint32)RpcIds::End:
      {
          // No arguments.

          Core->PostCheckReadMessage(ImmutableMessage__, RpcIds::End, RpcNames::End());

          // NotifyCallFromStub
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            Core->NotifyCallFromStub(RpcIds::End, RpcNames::End(), TEXT("{}"));
          }

          // Before RPC invocation.
          fun::uint32 InvocationTime__ = 0;
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            Core->BeforeRpcInvocation(fun::BeforeRpcSummary(RpcIds::End, RpcNames::End(), RemoteId__, host_tag));
            InvocationTime__ = fun::Clock::Milliseconds();
          }

          bool bIsImplemented__ = false;
          // First, try function object.
          if ((bool)OnEnd)
          {
            bIsImplemented__ = OnEnd(RemoteId__, RpcHint__);
          }
          // Second, call derived stub function.
          else
          {
            bIsImplemented__ = End(RemoteId__, RpcHint__);
          }

          if (!bIsImplemented__)
          {
            Core->ShowNotImplementedRpcWarning(RpcIds::End, RpcNames::End());
          }

          // After RPC invocation.
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            Core->AfterRpcInvocation(fun::AfterRpcSummary(RpcIds::End, RpcNames::End(), RemoteId__, host_tag, fun::Clock::Milliseconds() - InvocationTime__));
          }

          return true;
      }
    }

    Failure__: ImmutableMessage__.SetPosition(SavedReadPosition__); return false;
    #undef DO_CHECKED__
  }
} // end of namespace DumpC2S

} // end of namespace fun