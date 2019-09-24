﻿/**
 * Auto-generated by IDL Compiler (1.0.2)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 */
#include "dump_C2S_proxy.h"
#include "dump.h" // _Args, _PArgs

namespace fun { namespace Dump {
namespace C2S
{
  bool Proxy::Start(const fun::HostId* RpcRemoteIds, const fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& RpcCallOption)
  {
    fun::MessageOut RpcMessage__;

    // Write RPC function id.
    fun::LiteFormat::Write(RpcMessage__, RpcIds::Start);

    // Write RPC header.
    fun::RpcHeader::WriteOk(RpcMessage__);

    // No RPC function arguments.

    if (notify_send_by_proxy_enabled_ && !engine_specific_only_)
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, RpcCallOption, RpcMessage__, RpcNames::Start(), RpcIds::Start, TEXT("{}"));
    }
    else
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, RpcCallOption, RpcMessage__, RpcNames::Start(), RpcIds::Start, nullptr);
    }
  }

  bool Proxy::Chunk(const fun::HostId* RpcRemoteIds, const fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& RpcCallOption, const fun::ByteArray& Chunk)
  {
    fun::MessageOut RpcMessage__;

    // Write RPC function id.
    fun::LiteFormat::Write(RpcMessage__, RpcIds::Chunk);

    // Write RPC header.
    fun::RpcHeader::WriteOk(RpcMessage__);

    // Write RPC function arguments.
    fun::FlexFormat::WriteBytes(RpcMessage__, Chunk);

    if (notify_send_by_proxy_enabled_ && !engine_specific_only_)
    {
      fun::String ArgsStr__ = TEXT("{");
      ArgsStr__ << TEXT("\"Chunk\":") << fun::ToString(Chunk);
      ArgsStr__ << TEXT("}");

      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, RpcCallOption, RpcMessage__, RpcNames::Chunk(), RpcIds::Chunk, *ArgsStr__);
    }
    else
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, RpcCallOption, RpcMessage__, RpcNames::Chunk(), RpcIds::Chunk, nullptr);
    }
  }

  bool Proxy::End(const fun::HostId* RpcRemoteIds, const fun::int32 RpcRemoteIdCount, const fun::RpcCallOption& RpcCallOption)
  {
    fun::MessageOut RpcMessage__;

    // Write RPC function id.
    fun::LiteFormat::Write(RpcMessage__, RpcIds::End);

    // Write RPC header.
    fun::RpcHeader::WriteOk(RpcMessage__);

    // No RPC function arguments.

    if (notify_send_by_proxy_enabled_ && !engine_specific_only_)
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, RpcCallOption, RpcMessage__, RpcNames::End(), RpcIds::End, TEXT("{}"));
    }
    else
    {
      return RpcCall(RpcRemoteIds, RpcRemoteIdCount, RpcCallOption, RpcMessage__, RpcNames::End(), RpcIds::End, nullptr);
    }
  }
} // end of namespace C2S

}} // end of namespace fun::Dump
