﻿/**
 * Auto-generated by IDL Compiler (1.0.2)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 */
#include "emergency_S2C_stub.h"
#include "emergency.h" // _Args, _PArgs

namespace fun { namespace Emergency {
namespace S2C
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
      case (fun::uint32)RpcIds::EmergencyLogData_AckComplete:
      {
          // No arguments.

          Core->PostCheckReadMessage(ImmutableMessage__, RpcIds::EmergencyLogData_AckComplete, RpcNames::EmergencyLogData_AckComplete());

          // NotifyCallFromStub
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            Core->NotifyCallFromStub(RpcIds::EmergencyLogData_AckComplete, RpcNames::EmergencyLogData_AckComplete(), TEXT("{}"));
          }

          // Before RPC invocation.
          fun::uint32 InvocationTime__ = 0;
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            Core->BeforeRpcInvocation(fun::BeforeRpcSummary(RpcIds::EmergencyLogData_AckComplete, RpcNames::EmergencyLogData_AckComplete(), RemoteId__, host_tag));
            InvocationTime__ = fun::Clock::Milliseconds();
          }

          bool bIsImplemented__ = false;
          // First, try function object.
          if ((bool)OnEmergencyLogData_AckComplete)
          {
            bIsImplemented__ = OnEmergencyLogData_AckComplete(RemoteId__, RpcHint__);
          }
          // Second, call derived stub function.
          else
          {
            bIsImplemented__ = EmergencyLogData_AckComplete(RemoteId__, RpcHint__);
          }

          if (!bIsImplemented__)
          {
            Core->ShowNotImplementedRpcWarning(RpcIds::EmergencyLogData_AckComplete, RpcNames::EmergencyLogData_AckComplete());
          }

          // After RPC invocation.
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            Core->AfterRpcInvocation(fun::AfterRpcSummary(RpcIds::EmergencyLogData_AckComplete, RpcNames::EmergencyLogData_AckComplete(), RemoteId__, host_tag, fun::Clock::Milliseconds() - InvocationTime__));
          }

          return true;
      }
    }

    Failure__: ImmutableMessage__.SetPosition(SavedReadPosition__); return false;
    #undef DO_CHECKED__
  }
} // end of namespace S2C

}} // end of namespace fun::Emergency
