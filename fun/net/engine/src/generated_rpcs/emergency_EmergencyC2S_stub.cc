﻿/**
 * Auto-generated by IDL Compiler (2.1.3)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 */
#include "emergency_EmergencyC2S_stub.h"
#include "emergency.h" // _Args, _PArgs

namespace fun {
namespace EmergencyC2S
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
      case (fun::uint32)RpcIds::EmergencyLogData_Begin:
      {
          // Declare arguments.
          fun::DateTime logon_time = fun::DateTime::None;
          fun::int32 connect_count = 0;
          fun::int32 remote_peer_count = 0;
          fun::int32 direct_p2p_enable_peer_count = 0;
          fun::String nat_device_name;
          fun::HostId peer_id = fun::HostId_None;
          fun::int32 Iopendingcount = 0;
          fun::int32 total_tcp_issued_send_bytes_ = 0;

          // Read arguments.
          if (RpcHint__.result_code == 0)
          {
            DO_CHECKED__(fun::FlexFormat::ReadDateTime(ImmutableMessage__, logon_time));
            DO_CHECKED__(fun::FlexFormat::ReadSInt32(ImmutableMessage__, connect_count));
            DO_CHECKED__(fun::FlexFormat::ReadSInt32(ImmutableMessage__, remote_peer_count));
            DO_CHECKED__(fun::FlexFormat::ReadSInt32(ImmutableMessage__, direct_p2p_enable_peer_count));
            DO_CHECKED__(fun::FlexFormat::ReadString(ImmutableMessage__, nat_device_name));
            DO_CHECKED__(EngineTypes_UserTypeHandlers::Read(ImmutableMessage__, peer_id));
            DO_CHECKED__(fun::FlexFormat::ReadSInt32(ImmutableMessage__, Iopendingcount));
            DO_CHECKED__(fun::FlexFormat::ReadSInt32(ImmutableMessage__, total_tcp_issued_send_bytes_));
          }

          Core->PostCheckReadMessage(ImmutableMessage__, RpcIds::EmergencyLogData_Begin, RpcNames::EmergencyLogData_Begin());

          // NotifyCallFromStub
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            fun::String ArgsStr__ = TEXT("{");
            ArgsStr__ += TEXT("\"logon_time\":") + fun::ToString(logon_time);
            ArgsStr__ += TEXT(",\"connect_count\":") + fun::ToString(connect_count);
            ArgsStr__ += TEXT(",\"remote_peer_count\":") + fun::ToString(remote_peer_count);
            ArgsStr__ += TEXT(",\"direct_p2p_enable_peer_count\":") + fun::ToString(direct_p2p_enable_peer_count);
            ArgsStr__ += TEXT(",\"nat_device_name\":") + fun::ToString(nat_device_name);
            ArgsStr__ += TEXT(",\"peer_id\":") + fun::ToString(peer_id);
            ArgsStr__ += TEXT(",\"Iopendingcount\":") + fun::ToString(Iopendingcount);
            ArgsStr__ += TEXT(",\"total_tcp_issued_send_bytes_\":") + fun::ToString(total_tcp_issued_send_bytes_);
            ArgsStr__ += TEXT("}");

            Core->NotifyCallFromStub(RpcIds::EmergencyLogData_Begin, RpcNames::EmergencyLogData_Begin(), ArgsStr__);
          }

          // Before RPC invocation.
          fun::uint32 InvocationTime__ = 0;
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            Core->BeforeRpcInvocation(fun::BeforeRpcSummary(RpcIds::EmergencyLogData_Begin, RpcNames::EmergencyLogData_Begin(), RemoteId__, host_tag));
            InvocationTime__ = fun::Clock::Milliseconds();
          }

          bool bIsImplemented__ = false;
          // First, try function object.
          if ((bool)OnEmergencyLogData_Begin)
          {
            bIsImplemented__ = OnEmergencyLogData_Begin(RemoteId__, RpcHint__, logon_time, connect_count, remote_peer_count, direct_p2p_enable_peer_count, nat_device_name, peer_id, Iopendingcount, total_tcp_issued_send_bytes_);
          }
          // Second, call derived stub function.
          else
          {
            bIsImplemented__ = EmergencyLogData_Begin(RemoteId__, RpcHint__, logon_time, connect_count, remote_peer_count, direct_p2p_enable_peer_count, nat_device_name, peer_id, Iopendingcount, total_tcp_issued_send_bytes_);
          }

          if (!bIsImplemented__)
          {
            Core->ShowNotImplementedRpcWarning(RpcIds::EmergencyLogData_Begin, RpcNames::EmergencyLogData_Begin());
          }

          // After RPC invocation.
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            Core->AfterRpcInvocation(fun::AfterRpcSummary(RpcIds::EmergencyLogData_Begin, RpcNames::EmergencyLogData_Begin(), RemoteId__, host_tag, fun::Clock::Milliseconds() - InvocationTime__));
          }

          return true;
      }

      case (fun::uint32)RpcIds::EmergencyLogData_Error:
      {
          // Declare arguments.
          fun::int32 msg_size_error_count = 0;
          fun::int32 net_reset_error_count = 0;
          fun::int32 conn_reset_error_count = 0;
          fun::int32 last_error_completion_length = 0;

          // Read arguments.
          if (RpcHint__.result_code == 0)
          {
            DO_CHECKED__(fun::FlexFormat::ReadSInt32(ImmutableMessage__, msg_size_error_count));
            DO_CHECKED__(fun::FlexFormat::ReadSInt32(ImmutableMessage__, net_reset_error_count));
            DO_CHECKED__(fun::FlexFormat::ReadSInt32(ImmutableMessage__, conn_reset_error_count));
            DO_CHECKED__(fun::FlexFormat::ReadSInt32(ImmutableMessage__, last_error_completion_length));
          }

          Core->PostCheckReadMessage(ImmutableMessage__, RpcIds::EmergencyLogData_Error, RpcNames::EmergencyLogData_Error());

          // NotifyCallFromStub
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            fun::String ArgsStr__ = TEXT("{");
            ArgsStr__ += TEXT("\"msg_size_error_count\":") + fun::ToString(msg_size_error_count);
            ArgsStr__ += TEXT(",\"net_reset_error_count\":") + fun::ToString(net_reset_error_count);
            ArgsStr__ += TEXT(",\"conn_reset_error_count\":") + fun::ToString(conn_reset_error_count);
            ArgsStr__ += TEXT(",\"last_error_completion_length\":") + fun::ToString(last_error_completion_length);
            ArgsStr__ += TEXT("}");

            Core->NotifyCallFromStub(RpcIds::EmergencyLogData_Error, RpcNames::EmergencyLogData_Error(), ArgsStr__);
          }

          // Before RPC invocation.
          fun::uint32 InvocationTime__ = 0;
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            Core->BeforeRpcInvocation(fun::BeforeRpcSummary(RpcIds::EmergencyLogData_Error, RpcNames::EmergencyLogData_Error(), RemoteId__, host_tag));
            InvocationTime__ = fun::Clock::Milliseconds();
          }

          bool bIsImplemented__ = false;
          // First, try function object.
          if ((bool)OnEmergencyLogData_Error)
          {
            bIsImplemented__ = OnEmergencyLogData_Error(RemoteId__, RpcHint__, msg_size_error_count, net_reset_error_count, conn_reset_error_count, last_error_completion_length);
          }
          // Second, call derived stub function.
          else
          {
            bIsImplemented__ = EmergencyLogData_Error(RemoteId__, RpcHint__, msg_size_error_count, net_reset_error_count, conn_reset_error_count, last_error_completion_length);
          }

          if (!bIsImplemented__)
          {
            Core->ShowNotImplementedRpcWarning(RpcIds::EmergencyLogData_Error, RpcNames::EmergencyLogData_Error());
          }

          // After RPC invocation.
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            Core->AfterRpcInvocation(fun::AfterRpcSummary(RpcIds::EmergencyLogData_Error, RpcNames::EmergencyLogData_Error(), RemoteId__, host_tag, fun::Clock::Milliseconds() - InvocationTime__));
          }

          return true;
      }

      case (fun::uint32)RpcIds::EmergencyLogData_Stats:
      {
          // Declare arguments.
          fun::int64 total_tcp_recv_bytes = 0;
          fun::int64 total_tcp_send_bytes = 0;
          fun::int64 total_udp_send_count = 0;
          fun::int64 total_udp_send_bytes = 0;
          fun::int64 total_udp_recv_count = 0;
          fun::int64 total_udp_recv_bytes = 0;

          // Read arguments.
          if (RpcHint__.result_code == 0)
          {
            DO_CHECKED__(fun::FlexFormat::ReadSInt64(ImmutableMessage__, total_tcp_recv_bytes));
            DO_CHECKED__(fun::FlexFormat::ReadSInt64(ImmutableMessage__, total_tcp_send_bytes));
            DO_CHECKED__(fun::FlexFormat::ReadSInt64(ImmutableMessage__, total_udp_send_count));
            DO_CHECKED__(fun::FlexFormat::ReadSInt64(ImmutableMessage__, total_udp_send_bytes));
            DO_CHECKED__(fun::FlexFormat::ReadSInt64(ImmutableMessage__, total_udp_recv_count));
            DO_CHECKED__(fun::FlexFormat::ReadSInt64(ImmutableMessage__, total_udp_recv_bytes));
          }

          Core->PostCheckReadMessage(ImmutableMessage__, RpcIds::EmergencyLogData_Stats, RpcNames::EmergencyLogData_Stats());

          // NotifyCallFromStub
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            fun::String ArgsStr__ = TEXT("{");
            ArgsStr__ += TEXT("\"total_tcp_recv_bytes\":") + fun::ToString(total_tcp_recv_bytes);
            ArgsStr__ += TEXT(",\"total_tcp_send_bytes\":") + fun::ToString(total_tcp_send_bytes);
            ArgsStr__ += TEXT(",\"total_udp_send_count\":") + fun::ToString(total_udp_send_count);
            ArgsStr__ += TEXT(",\"total_udp_send_bytes\":") + fun::ToString(total_udp_send_bytes);
            ArgsStr__ += TEXT(",\"total_udp_recv_count\":") + fun::ToString(total_udp_recv_count);
            ArgsStr__ += TEXT(",\"total_udp_recv_bytes\":") + fun::ToString(total_udp_recv_bytes);
            ArgsStr__ += TEXT("}");

            Core->NotifyCallFromStub(RpcIds::EmergencyLogData_Stats, RpcNames::EmergencyLogData_Stats(), ArgsStr__);
          }

          // Before RPC invocation.
          fun::uint32 InvocationTime__ = 0;
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            Core->BeforeRpcInvocation(fun::BeforeRpcSummary(RpcIds::EmergencyLogData_Stats, RpcNames::EmergencyLogData_Stats(), RemoteId__, host_tag));
            InvocationTime__ = fun::Clock::Milliseconds();
          }

          bool bIsImplemented__ = false;
          // First, try function object.
          if ((bool)OnEmergencyLogData_Stats)
          {
            bIsImplemented__ = OnEmergencyLogData_Stats(RemoteId__, RpcHint__, total_tcp_recv_bytes, total_tcp_send_bytes, total_udp_send_count, total_udp_send_bytes, total_udp_recv_count, total_udp_recv_bytes);
          }
          // Second, call derived stub function.
          else
          {
            bIsImplemented__ = EmergencyLogData_Stats(RemoteId__, RpcHint__, total_tcp_recv_bytes, total_tcp_send_bytes, total_udp_send_count, total_udp_send_bytes, total_udp_recv_count, total_udp_recv_bytes);
          }

          if (!bIsImplemented__)
          {
            Core->ShowNotImplementedRpcWarning(RpcIds::EmergencyLogData_Stats, RpcNames::EmergencyLogData_Stats());
          }

          // After RPC invocation.
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            Core->AfterRpcInvocation(fun::AfterRpcSummary(RpcIds::EmergencyLogData_Stats, RpcNames::EmergencyLogData_Stats(), RemoteId__, host_tag, fun::Clock::Milliseconds() - InvocationTime__));
          }

          return true;
      }

      case (fun::uint32)RpcIds::EmergencyLogData_OSVersion:
      {
          // Declare arguments.
          fun::uint32 os_major_version = 0;
          fun::uint32 os_minor_version = 0;
          fun::uint8 product_type = 0;
          fun::uint16 processor_architecture = 0;

          // Read arguments.
          if (RpcHint__.result_code == 0)
          {
            DO_CHECKED__(fun::FlexFormat::ReadUInt32(ImmutableMessage__, os_major_version));
            DO_CHECKED__(fun::FlexFormat::ReadUInt32(ImmutableMessage__, os_minor_version));
            DO_CHECKED__(fun::FlexFormat::ReadFixed8(ImmutableMessage__, product_type));
            DO_CHECKED__(fun::FlexFormat::ReadFixed16(ImmutableMessage__, processor_architecture));
          }

          Core->PostCheckReadMessage(ImmutableMessage__, RpcIds::EmergencyLogData_OSVersion, RpcNames::EmergencyLogData_OSVersion());

          // NotifyCallFromStub
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            fun::String ArgsStr__ = TEXT("{");
            ArgsStr__ += TEXT("\"os_major_version\":") + fun::ToString(os_major_version);
            ArgsStr__ += TEXT(",\"os_minor_version\":") + fun::ToString(os_minor_version);
            ArgsStr__ += TEXT(",\"product_type\":") + fun::ToString(product_type);
            ArgsStr__ += TEXT(",\"processor_architecture\":") + fun::ToString(processor_architecture);
            ArgsStr__ += TEXT("}");

            Core->NotifyCallFromStub(RpcIds::EmergencyLogData_OSVersion, RpcNames::EmergencyLogData_OSVersion(), ArgsStr__);
          }

          // Before RPC invocation.
          fun::uint32 InvocationTime__ = 0;
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            Core->BeforeRpcInvocation(fun::BeforeRpcSummary(RpcIds::EmergencyLogData_OSVersion, RpcNames::EmergencyLogData_OSVersion(), RemoteId__, host_tag));
            InvocationTime__ = fun::Clock::Milliseconds();
          }

          bool bIsImplemented__ = false;
          // First, try function object.
          if ((bool)OnEmergencyLogData_OSVersion)
          {
            bIsImplemented__ = OnEmergencyLogData_OSVersion(RemoteId__, RpcHint__, os_major_version, os_minor_version, product_type, processor_architecture);
          }
          // Second, call derived stub function.
          else
          {
            bIsImplemented__ = EmergencyLogData_OSVersion(RemoteId__, RpcHint__, os_major_version, os_minor_version, product_type, processor_architecture);
          }

          if (!bIsImplemented__)
          {
            Core->ShowNotImplementedRpcWarning(RpcIds::EmergencyLogData_OSVersion, RpcNames::EmergencyLogData_OSVersion());
          }

          // After RPC invocation.
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            Core->AfterRpcInvocation(fun::AfterRpcSummary(RpcIds::EmergencyLogData_OSVersion, RpcNames::EmergencyLogData_OSVersion(), RemoteId__, host_tag, fun::Clock::Milliseconds() - InvocationTime__));
          }

          return true;
      }

      case (fun::uint32)RpcIds::EmergencyLogData_LogEvent:
      {
          // Declare arguments.
          fun::LogCategory Category = (fun::LogCategory)0;
          fun::DateTime added_time = fun::DateTime::None;
          fun::String text;

          // Read arguments.
          if (RpcHint__.result_code == 0)
          {
            DO_CHECKED__(EngineTypes_UserTypeHandlers::Read(ImmutableMessage__, Category));
            DO_CHECKED__(fun::FlexFormat::ReadDateTime(ImmutableMessage__, added_time));
            DO_CHECKED__(fun::FlexFormat::ReadString(ImmutableMessage__, text));
          }

          Core->PostCheckReadMessage(ImmutableMessage__, RpcIds::EmergencyLogData_LogEvent, RpcNames::EmergencyLogData_LogEvent());

          // NotifyCallFromStub
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            fun::String ArgsStr__ = TEXT("{");
            ArgsStr__ += TEXT("\"Category\":") + fun::ToString(Category);
            ArgsStr__ += TEXT(",\"added_time\":") + fun::ToString(added_time);
            ArgsStr__ += TEXT(",\"text\":") + fun::ToString(text);
            ArgsStr__ += TEXT("}");

            Core->NotifyCallFromStub(RpcIds::EmergencyLogData_LogEvent, RpcNames::EmergencyLogData_LogEvent(), ArgsStr__);
          }

          // Before RPC invocation.
          fun::uint32 InvocationTime__ = 0;
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            Core->BeforeRpcInvocation(fun::BeforeRpcSummary(RpcIds::EmergencyLogData_LogEvent, RpcNames::EmergencyLogData_LogEvent(), RemoteId__, host_tag));
            InvocationTime__ = fun::Clock::Milliseconds();
          }

          bool bIsImplemented__ = false;
          // First, try function object.
          if ((bool)OnEmergencyLogData_LogEvent)
          {
            bIsImplemented__ = OnEmergencyLogData_LogEvent(RemoteId__, RpcHint__, Category, added_time, text);
          }
          // Second, call derived stub function.
          else
          {
            bIsImplemented__ = EmergencyLogData_LogEvent(RemoteId__, RpcHint__, Category, added_time, text);
          }

          if (!bIsImplemented__)
          {
            Core->ShowNotImplementedRpcWarning(RpcIds::EmergencyLogData_LogEvent, RpcNames::EmergencyLogData_LogEvent());
          }

          // After RPC invocation.
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            Core->AfterRpcInvocation(fun::AfterRpcSummary(RpcIds::EmergencyLogData_LogEvent, RpcNames::EmergencyLogData_LogEvent(), RemoteId__, host_tag, fun::Clock::Milliseconds() - InvocationTime__));
          }

          return true;
      }

      case (fun::uint32)RpcIds::EmergencyLogData_End:
      {
          // Declare arguments.
          fun::int32 server_udp_addr_count = 0;
          fun::int32 remote_udp_addr_count = 0;

          // Read arguments.
          if (RpcHint__.result_code == 0)
          {
            DO_CHECKED__(fun::FlexFormat::ReadSInt32(ImmutableMessage__, server_udp_addr_count));
            DO_CHECKED__(fun::FlexFormat::ReadSInt32(ImmutableMessage__, remote_udp_addr_count));
          }

          Core->PostCheckReadMessage(ImmutableMessage__, RpcIds::EmergencyLogData_End, RpcNames::EmergencyLogData_End());

          // NotifyCallFromStub
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            fun::String ArgsStr__ = TEXT("{");
            ArgsStr__ += TEXT("\"server_udp_addr_count\":") + fun::ToString(server_udp_addr_count);
            ArgsStr__ += TEXT(",\"remote_udp_addr_count\":") + fun::ToString(remote_udp_addr_count);
            ArgsStr__ += TEXT("}");

            Core->NotifyCallFromStub(RpcIds::EmergencyLogData_End, RpcNames::EmergencyLogData_End(), ArgsStr__);
          }

          // Before RPC invocation.
          fun::uint32 InvocationTime__ = 0;
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            Core->BeforeRpcInvocation(fun::BeforeRpcSummary(RpcIds::EmergencyLogData_End, RpcNames::EmergencyLogData_End(), RemoteId__, host_tag));
            InvocationTime__ = fun::Clock::Milliseconds();
          }

          bool bIsImplemented__ = false;
          // First, try function object.
          if ((bool)OnEmergencyLogData_End)
          {
            bIsImplemented__ = OnEmergencyLogData_End(RemoteId__, RpcHint__, server_udp_addr_count, remote_udp_addr_count);
          }
          // Second, call derived stub function.
          else
          {
            bIsImplemented__ = EmergencyLogData_End(RemoteId__, RpcHint__, server_udp_addr_count, remote_udp_addr_count);
          }

          if (!bIsImplemented__)
          {
            Core->ShowNotImplementedRpcWarning(RpcIds::EmergencyLogData_End, RpcNames::EmergencyLogData_End());
          }

          // After RPC invocation.
          if (notify_call_from_stub_enabled_ && !engine_specific_only_)
          {
            Core->AfterRpcInvocation(fun::AfterRpcSummary(RpcIds::EmergencyLogData_End, RpcNames::EmergencyLogData_End(), RemoteId__, host_tag, fun::Clock::Milliseconds() - InvocationTime__));
          }

          return true;
      }
    }

    Failure__: ImmutableMessage__.SetPosition(SavedReadPosition__); return false;
    #undef DO_CHECKED__
  }
} // end of namespace EmergencyC2S

} // end of namespace fun
