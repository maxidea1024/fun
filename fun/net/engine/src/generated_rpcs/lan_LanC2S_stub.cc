﻿/**
 * Auto-generated by IDL Compiler (2.1.3)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 */
#include "lan_LanC2S_stub.h"
#include "lan.h" // _Args, _PArgs

namespace fun {
namespace LanC2S
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
      case (fun::uint32)RpcIds::ReliablePing:
      {
          // No arguments.

          Core->PostCheckReadMessage(ImmutableMessage__, RpcIds::ReliablePing, RpcNames::ReliablePing());

          // NotifyCallFromStub
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            Core->NotifyCallFromStub(RpcIds::ReliablePing, RpcNames::ReliablePing(), TEXT("{}"));
          }

          // Before RPC invocation.
          fun::uint32 InvocationTime__ = 0;
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            Core->BeforeRpcInvocation(fun::BeforeRpcSummary(RpcIds::ReliablePing, RpcNames::ReliablePing(), RemoteId__, host_tag));
            InvocationTime__ = fun::Clock::Milliseconds();
          }

          bool bIsImplemented__ = false;
          // First, try function object.
          if ((bool)OnReliablePing)
          {
            bIsImplemented__ = OnReliablePing(RemoteId__, RpcHint__);
          }
          // Second, call derived stub function.
          else
          {
            bIsImplemented__ = ReliablePing(RemoteId__, RpcHint__);
          }

          if (!bIsImplemented__)
          {
            Core->ShowNotImplementedRpcWarning(RpcIds::ReliablePing, RpcNames::ReliablePing());
          }

          // After RPC invocation.
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            Core->AfterRpcInvocation(fun::AfterRpcSummary(RpcIds::ReliablePing, RpcNames::ReliablePing(), RemoteId__, host_tag, fun::Clock::Milliseconds() - InvocationTime__));
          }

          return true;
      }

      case (fun::uint32)RpcIds::P2PGroup_MemberJoin_Ack:
      {
          // Declare arguments.
          fun::HostId group_id = fun::HostId_None;
          fun::HostId added_member_id = fun::HostId_None;
          fun::uint32 event_id = 0;

          // Read arguments.
          if (RpcHint__.result_code == 0)
          {
            DO_CHECKED__(EngineTypes_UserTypeHandlers::Read(ImmutableMessage__, group_id));
            DO_CHECKED__(EngineTypes_UserTypeHandlers::Read(ImmutableMessage__, added_member_id));
            DO_CHECKED__(fun::FlexFormat::ReadUInt32(ImmutableMessage__, event_id));
          }

          Core->PostCheckReadMessage(ImmutableMessage__, RpcIds::P2PGroup_MemberJoin_Ack, RpcNames::P2PGroup_MemberJoin_Ack());

          // NotifyCallFromStub
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            fun::String ArgsStr__ = TEXT("{");
            ArgsStr__ += TEXT("\"group_id\":") + fun::ToString(group_id);
            ArgsStr__ += TEXT(",\"added_member_id\":") + fun::ToString(added_member_id);
            ArgsStr__ += TEXT(",\"event_id\":") + fun::ToString(event_id);
            ArgsStr__ += TEXT("}");

            Core->NotifyCallFromStub(RpcIds::P2PGroup_MemberJoin_Ack, RpcNames::P2PGroup_MemberJoin_Ack(), ArgsStr__);
          }

          // Before RPC invocation.
          fun::uint32 InvocationTime__ = 0;
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            Core->BeforeRpcInvocation(fun::BeforeRpcSummary(RpcIds::P2PGroup_MemberJoin_Ack, RpcNames::P2PGroup_MemberJoin_Ack(), RemoteId__, host_tag));
            InvocationTime__ = fun::Clock::Milliseconds();
          }

          bool bIsImplemented__ = false;
          // First, try function object.
          if ((bool)OnP2PGroup_MemberJoin_Ack)
          {
            bIsImplemented__ = OnP2PGroup_MemberJoin_Ack(RemoteId__, RpcHint__, group_id, added_member_id, event_id);
          }
          // Second, call derived stub function.
          else
          {
            bIsImplemented__ = P2PGroup_MemberJoin_Ack(RemoteId__, RpcHint__, group_id, added_member_id, event_id);
          }

          if (!bIsImplemented__)
          {
            Core->ShowNotImplementedRpcWarning(RpcIds::P2PGroup_MemberJoin_Ack, RpcNames::P2PGroup_MemberJoin_Ack());
          }

          // After RPC invocation.
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            Core->AfterRpcInvocation(fun::AfterRpcSummary(RpcIds::P2PGroup_MemberJoin_Ack, RpcNames::P2PGroup_MemberJoin_Ack(), RemoteId__, host_tag, fun::Clock::Milliseconds() - InvocationTime__));
          }

          return true;
      }

      case (fun::uint32)RpcIds::ReportP2PPeerPing:
      {
          // Declare arguments.
          fun::HostId peer_id = fun::HostId_None;
          fun::uint32 recent_ping = 0;

          // Read arguments.
          if (RpcHint__.result_code == 0)
          {
            DO_CHECKED__(EngineTypes_UserTypeHandlers::Read(ImmutableMessage__, peer_id));
            DO_CHECKED__(fun::FlexFormat::ReadUInt32(ImmutableMessage__, recent_ping));
          }

          Core->PostCheckReadMessage(ImmutableMessage__, RpcIds::ReportP2PPeerPing, RpcNames::ReportP2PPeerPing());

          // NotifyCallFromStub
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            fun::String ArgsStr__ = TEXT("{");
            ArgsStr__ += TEXT("\"peer_id\":") + fun::ToString(peer_id);
            ArgsStr__ += TEXT(",\"recent_ping\":") + fun::ToString(recent_ping);
            ArgsStr__ += TEXT("}");

            Core->NotifyCallFromStub(RpcIds::ReportP2PPeerPing, RpcNames::ReportP2PPeerPing(), ArgsStr__);
          }

          // Before RPC invocation.
          fun::uint32 InvocationTime__ = 0;
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            Core->BeforeRpcInvocation(fun::BeforeRpcSummary(RpcIds::ReportP2PPeerPing, RpcNames::ReportP2PPeerPing(), RemoteId__, host_tag));
            InvocationTime__ = fun::Clock::Milliseconds();
          }

          bool bIsImplemented__ = false;
          // First, try function object.
          if ((bool)OnReportP2PPeerPing)
          {
            bIsImplemented__ = OnReportP2PPeerPing(RemoteId__, RpcHint__, peer_id, recent_ping);
          }
          // Second, call derived stub function.
          else
          {
            bIsImplemented__ = ReportP2PPeerPing(RemoteId__, RpcHint__, peer_id, recent_ping);
          }

          if (!bIsImplemented__)
          {
            Core->ShowNotImplementedRpcWarning(RpcIds::ReportP2PPeerPing, RpcNames::ReportP2PPeerPing());
          }

          // After RPC invocation.
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            Core->AfterRpcInvocation(fun::AfterRpcSummary(RpcIds::ReportP2PPeerPing, RpcNames::ReportP2PPeerPing(), RemoteId__, host_tag, fun::Clock::Milliseconds() - InvocationTime__));
          }

          return true;
      }

      case (fun::uint32)RpcIds::ShutdownTcp:
      {
          // Declare arguments.
          fun::ByteArray comment;

          // Read arguments.
          if (RpcHint__.result_code == 0)
          {
            DO_CHECKED__(fun::FlexFormat::ReadBytes(ImmutableMessage__, comment));
          }

          Core->PostCheckReadMessage(ImmutableMessage__, RpcIds::ShutdownTcp, RpcNames::ShutdownTcp());

          // NotifyCallFromStub
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            fun::String ArgsStr__ = TEXT("{");
            ArgsStr__ += TEXT("\"comment\":") + fun::ToString(comment);
            ArgsStr__ += TEXT("}");

            Core->NotifyCallFromStub(RpcIds::ShutdownTcp, RpcNames::ShutdownTcp(), ArgsStr__);
          }

          // Before RPC invocation.
          fun::uint32 InvocationTime__ = 0;
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            Core->BeforeRpcInvocation(fun::BeforeRpcSummary(RpcIds::ShutdownTcp, RpcNames::ShutdownTcp(), RemoteId__, host_tag));
            InvocationTime__ = fun::Clock::Milliseconds();
          }

          bool bIsImplemented__ = false;
          // First, try function object.
          if ((bool)OnShutdownTcp)
          {
            bIsImplemented__ = OnShutdownTcp(RemoteId__, RpcHint__, comment);
          }
          // Second, call derived stub function.
          else
          {
            bIsImplemented__ = ShutdownTcp(RemoteId__, RpcHint__, comment);
          }

          if (!bIsImplemented__)
          {
            Core->ShowNotImplementedRpcWarning(RpcIds::ShutdownTcp, RpcNames::ShutdownTcp());
          }

          // After RPC invocation.
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            Core->AfterRpcInvocation(fun::AfterRpcSummary(RpcIds::ShutdownTcp, RpcNames::ShutdownTcp(), RemoteId__, host_tag, fun::Clock::Milliseconds() - InvocationTime__));
          }

          return true;
      }

      case (fun::uint32)RpcIds::ShutdownTcpHandshake:
      {
          // No arguments.

          Core->PostCheckReadMessage(ImmutableMessage__, RpcIds::ShutdownTcpHandshake, RpcNames::ShutdownTcpHandshake());

          // NotifyCallFromStub
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            Core->NotifyCallFromStub(RpcIds::ShutdownTcpHandshake, RpcNames::ShutdownTcpHandshake(), TEXT("{}"));
          }

          // Before RPC invocation.
          fun::uint32 InvocationTime__ = 0;
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            Core->BeforeRpcInvocation(fun::BeforeRpcSummary(RpcIds::ShutdownTcpHandshake, RpcNames::ShutdownTcpHandshake(), RemoteId__, host_tag));
            InvocationTime__ = fun::Clock::Milliseconds();
          }

          bool bIsImplemented__ = false;
          // First, try function object.
          if ((bool)OnShutdownTcpHandshake)
          {
            bIsImplemented__ = OnShutdownTcpHandshake(RemoteId__, RpcHint__);
          }
          // Second, call derived stub function.
          else
          {
            bIsImplemented__ = ShutdownTcpHandshake(RemoteId__, RpcHint__);
          }

          if (!bIsImplemented__)
          {
            Core->ShowNotImplementedRpcWarning(RpcIds::ShutdownTcpHandshake, RpcNames::ShutdownTcpHandshake());
          }

          // After RPC invocation.
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            Core->AfterRpcInvocation(fun::AfterRpcSummary(RpcIds::ShutdownTcpHandshake, RpcNames::ShutdownTcpHandshake(), RemoteId__, host_tag, fun::Clock::Milliseconds() - InvocationTime__));
          }

          return true;
      }
    }

    Failure__: ImmutableMessage__.SetPosition(SavedReadPosition__); return false;
    #undef DO_CHECKED__
  }
} // end of namespace LanC2S

} // end of namespace fun
