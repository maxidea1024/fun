﻿/**
 * Auto-generated by IDL Compiler (2.1.3)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 */
#pragma once

#include "FUN.h"
#include "agent_defs.h"
#include "agent_types.h"

namespace fun {
namespace AgentC2S
{
  struct Stub : public fun::RpcStub
  {
    fun::int32 GetDeclaredRpcCount() const override { return AgentC2S::NumRpcFunctions; }
    const fun::RpcId* GetDeclaredRpcIds() const override { return AgentC2S::RpcIds::RpcIdList(); }
    const char** GetDeclaredRpcNames() const override { return AgentC2S::RpcNames::RpcNameList(); }

    // RequestCredential
    virtual bool RequestCredential(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::int32 Cookie) { return false; }
    #define DECLARE_RPCSTUB_AgentC2S_RequestCredential bool RequestCredential(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::int32 Cookie) override;
    #define IMPLEMENT_RPCSTUB_AgentC2S_RequestCredential(TClass) bool TClass::RequestCredential(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::int32 Cookie)
    typedef fun::Function<bool(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::int32 /* Cookie */)> RequestCredentialFunctionType;
    RequestCredentialFunctionType OnRequestCredential;

    // ReportStatusBegin
    virtual bool ReportStatusBegin(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::uint8 Type, const fun::String& text) { return false; }
    #define DECLARE_RPCSTUB_AgentC2S_ReportStatusBegin bool ReportStatusBegin(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::uint8 Type, const fun::String& text) override;
    #define IMPLEMENT_RPCSTUB_AgentC2S_ReportStatusBegin(TClass) bool TClass::ReportStatusBegin(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::uint8 Type, const fun::String& text)
    typedef fun::Function<bool(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::uint8 /* Type */, const fun::String& /* text */)> ReportStatusBeginFunctionType;
    ReportStatusBeginFunctionType OnReportStatusBegin;

    // ReportStatusValue
    virtual bool ReportStatusValue(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::String& Key, const fun::String& Value) { return false; }
    #define DECLARE_RPCSTUB_AgentC2S_ReportStatusValue bool ReportStatusValue(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::String& Key, const fun::String& Value) override;
    #define IMPLEMENT_RPCSTUB_AgentC2S_ReportStatusValue(TClass) bool TClass::ReportStatusValue(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::String& Key, const fun::String& Value)
    typedef fun::Function<bool(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::String& /* Key */, const fun::String& /* Value */)> ReportStatusValueFunctionType;
    ReportStatusValueFunctionType OnReportStatusValue;

    // ReportStatusEnd
    virtual bool ReportStatusEnd(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint) { return false; }
    #define DECLARE_RPCSTUB_AgentC2S_ReportStatusEnd bool ReportStatusEnd(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint) override;
    #define IMPLEMENT_RPCSTUB_AgentC2S_ReportStatusEnd(TClass) bool TClass::ReportStatusEnd(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint)
    typedef fun::Function<bool(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint)> ReportStatusEndFunctionType;
    ReportStatusEndFunctionType OnReportStatusEnd;

    // ReportServerAppState
    virtual bool ReportServerAppState(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const float CpuUserTime, const float CpuKerenlTime, const fun::int32 MemorySize) { return false; }
    #define DECLARE_RPCSTUB_AgentC2S_ReportServerAppState bool ReportServerAppState(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const float CpuUserTime, const float CpuKerenlTime, const fun::int32 MemorySize) override;
    #define IMPLEMENT_RPCSTUB_AgentC2S_ReportServerAppState(TClass) bool TClass::ReportServerAppState(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const float CpuUserTime, const float CpuKerenlTime, const fun::int32 MemorySize)
    typedef fun::Function<bool(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const float /* CpuUserTime */, const float /* CpuKerenlTime */, const fun::int32 /* MemorySize */)> ReportServerAppStateFunctionType;
    ReportServerAppStateFunctionType OnReportServerAppState;

    // EventLog
    virtual bool EventLog(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::LogCategory& Category, const fun::String& text) { return false; }
    #define DECLARE_RPCSTUB_AgentC2S_EventLog bool EventLog(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::LogCategory& Category, const fun::String& text) override;
    #define IMPLEMENT_RPCSTUB_AgentC2S_EventLog(TClass) bool TClass::EventLog(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::LogCategory& Category, const fun::String& text)
    typedef fun::Function<bool(const fun::HostId rpc_recvfrom, const fun::RpcHint& RpcHint, const fun::LogCategory& /* Category */, const fun::String& /* text */)> EventLogFunctionType;
    EventLogFunctionType OnEventLog;

    // RpcStub interface
    bool ProcessReceivedMessage(fun::ReceivedMessage& received_msg, void* host_tag) override;
  };

} // end of namespace AgentC2S

} // end of namespace fun
