﻿/**
 * Auto-generated by IDL Compiler (2.1.3)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 */
#pragma once

#include "FUN.h"
#include "lan_types.h"

#define IDL_BEGIN_GLOBAL_NAMESPACE }
#define IDL_END_GLOBAL_NAMESPACE namespace fun {

namespace fun {

//==============================================================================
// Service LanC2C
//==============================================================================

namespace LanC2C
{
} // end of namespace LanC2C


//==============================================================================
// Service LanC2S
//==============================================================================

namespace LanC2S
{
  struct CLanC2S_P2PGroup_MemberJoin_Ack_Args
  {
    // arguments.
    fun::HostId group_id;
    fun::HostId added_member_id;
    fun::uint32 event_id;

    // Construction
    inline CLanC2S_P2PGroup_MemberJoin_Ack_Args() {}

    // Reader
    bool Read(fun::IMessageIn& Input__)
    {
      #define DO_CHECKED__(Expr) { if (!(Expr)) return false; }
      DO_CHECKED__(EngineTypes_UserTypeHandlers::Read(Input__, group_id));
      DO_CHECKED__(EngineTypes_UserTypeHandlers::Read(Input__, added_member_id));
      DO_CHECKED__(fun::FlexFormat::ReadUInt32(Input__, event_id));
      return true;
      #undef DO_CHECKED__
    }

    // ToString(Json style for diagnostics)
    inline fun::String ToString() const
    {
      fun::String result = TEXT("{");
      result << TEXT("\"group_id\":") << fun::ToString(group_id);
      result << TEXT("\",added_member_id\":") << fun::ToString(added_member_id);
      result << TEXT("\",event_id\":") << fun::ToString(event_id);
      result << TEXT("}");
      return result;
    }
  };

  struct CLanC2S_P2PGroup_MemberJoin_Ack_PArgs
  {
    // arguments.
    const fun::HostId* group_id;
    const fun::HostId* added_member_id;
    const fun::uint32* event_id;

    // Construction
    inline CLanC2S_P2PGroup_MemberJoin_Ack_PArgs(const fun::HostId* GroupId_, const fun::HostId* AddedMemberId_, const fun::uint32* EventId_)
      : group_id(GroupId_)
      , added_member_id(AddedMemberId_)
      , event_id(EventId_)
    {}

    // Writer
    inline void Write(fun::IMessageOut& Output__)
    {
      EngineTypes_UserTypeHandlers::Write(Output__, *group_id);
      EngineTypes_UserTypeHandlers::Write(Output__, *added_member_id);
      fun::FlexFormat::WriteUInt32(Output__, *event_id);
    }

    // ToString(Json style for diagnostics)
    inline fun::String ToString() const
    {
      fun::String result = TEXT("{");
      result << TEXT("\"group_id\":") << fun::ToString(*group_id);
      result << TEXT("\",added_member_id\":") << fun::ToString(*added_member_id);
      result << TEXT("\",event_id\":") << fun::ToString(*event_id);
      result << TEXT("}");
      return result;
    }
  };

  struct CLanC2S_ReportP2PPeerPing_Args
  {
    // arguments.
    fun::HostId peer_id;
    fun::uint32 recent_ping;

    // Construction
    inline CLanC2S_ReportP2PPeerPing_Args() {}

    // Reader
    bool Read(fun::IMessageIn& Input__)
    {
      #define DO_CHECKED__(Expr) { if (!(Expr)) return false; }
      DO_CHECKED__(EngineTypes_UserTypeHandlers::Read(Input__, peer_id));
      DO_CHECKED__(fun::FlexFormat::ReadUInt32(Input__, recent_ping));
      return true;
      #undef DO_CHECKED__
    }

    // ToString(Json style for diagnostics)
    inline fun::String ToString() const
    {
      fun::String result = TEXT("{");
      result << TEXT("\"peer_id\":") << fun::ToString(peer_id);
      result << TEXT("\",recent_ping\":") << fun::ToString(recent_ping);
      result << TEXT("}");
      return result;
    }
  };

  struct CLanC2S_ReportP2PPeerPing_PArgs
  {
    // arguments.
    const fun::HostId* peer_id;
    const fun::uint32* recent_ping;

    // Construction
    inline CLanC2S_ReportP2PPeerPing_PArgs(const fun::HostId* PeerId_, const fun::uint32* RecentPing_)
      : peer_id(PeerId_)
      , recent_ping(RecentPing_)
    {}

    // Writer
    inline void Write(fun::IMessageOut& Output__)
    {
      EngineTypes_UserTypeHandlers::Write(Output__, *peer_id);
      fun::FlexFormat::WriteUInt32(Output__, *recent_ping);
    }

    // ToString(Json style for diagnostics)
    inline fun::String ToString() const
    {
      fun::String result = TEXT("{");
      result << TEXT("\"peer_id\":") << fun::ToString(*peer_id);
      result << TEXT("\",recent_ping\":") << fun::ToString(*recent_ping);
      result << TEXT("}");
      return result;
    }
  };

  struct CLanC2S_ShutdownTcp_Args
  {
    // arguments.
    fun::ByteArray comment;

    // Construction
    inline CLanC2S_ShutdownTcp_Args() {}

    // Reader
    bool Read(fun::IMessageIn& Input__)
    {
      #define DO_CHECKED__(Expr) { if (!(Expr)) return false; }
      DO_CHECKED__(fun::FlexFormat::ReadBytes(Input__, comment));
      return true;
      #undef DO_CHECKED__
    }

    // ToString(Json style for diagnostics)
    inline fun::String ToString() const
    {
      fun::String result = TEXT("{");
      result << TEXT("\"comment\":") << fun::ToString(comment);
      result << TEXT("}");
      return result;
    }
  };

  struct CLanC2S_ShutdownTcp_PArgs
  {
    // arguments.
    const fun::ByteArray* comment;

    // Construction
    inline CLanC2S_ShutdownTcp_PArgs(const fun::ByteArray* Comment_)
      : comment(Comment_)
    {}

    // Writer
    inline void Write(fun::IMessageOut& Output__)
    {
      fun::FlexFormat::WriteBytes(Output__, *comment);
    }

    // ToString(Json style for diagnostics)
    inline fun::String ToString() const
    {
      fun::String result = TEXT("{");
      result << TEXT("\"comment\":") << fun::ToString(*comment);
      result << TEXT("}");
      return result;
    }
  };
} // end of namespace LanC2S


//==============================================================================
// Service LanS2C
//==============================================================================

namespace LanS2C
{
  struct CLanS2C_P2PGroup_MemberJoin_Args
  {
    // arguments.
    fun::HostId group_id;
    fun::HostId member_id;
    fun::ByteArray custom_field;
    fun::uint32 event_id;
    fun::ByteArray P2PAESSessionKey;
    fun::ByteArray P2PRC4SessionKey;
    fun::Uuid ConnectionTag;

    // Construction
    inline CLanS2C_P2PGroup_MemberJoin_Args() {}

    // Reader
    bool Read(fun::IMessageIn& Input__)
    {
      #define DO_CHECKED__(Expr) { if (!(Expr)) return false; }
      DO_CHECKED__(EngineTypes_UserTypeHandlers::Read(Input__, group_id));
      DO_CHECKED__(EngineTypes_UserTypeHandlers::Read(Input__, member_id));
      DO_CHECKED__(fun::FlexFormat::ReadBytes(Input__, custom_field));
      DO_CHECKED__(fun::FlexFormat::ReadUInt32(Input__, event_id));
      DO_CHECKED__(fun::FlexFormat::ReadBytes(Input__, P2PAESSessionKey));
      DO_CHECKED__(fun::FlexFormat::ReadBytes(Input__, P2PRC4SessionKey));
      DO_CHECKED__(fun::FlexFormat::ReadGuid(Input__, ConnectionTag));
      return true;
      #undef DO_CHECKED__
    }

    // ToString(Json style for diagnostics)
    inline fun::String ToString() const
    {
      fun::String result = TEXT("{");
      result << TEXT("\"group_id\":") << fun::ToString(group_id);
      result << TEXT("\",member_id\":") << fun::ToString(member_id);
      result << TEXT("\",custom_field\":") << fun::ToString(custom_field);
      result << TEXT("\",event_id\":") << fun::ToString(event_id);
      result << TEXT("\",P2PAESSessionKey\":") << fun::ToString(P2PAESSessionKey);
      result << TEXT("\",P2PRC4SessionKey\":") << fun::ToString(P2PRC4SessionKey);
      result << TEXT("\",ConnectionTag\":") << fun::ToString(ConnectionTag);
      result << TEXT("}");
      return result;
    }
  };

  struct CLanS2C_P2PGroup_MemberJoin_PArgs
  {
    // arguments.
    const fun::HostId* group_id;
    const fun::HostId* member_id;
    const fun::ByteArray* custom_field;
    const fun::uint32* event_id;
    const fun::ByteArray* P2PAESSessionKey;
    const fun::ByteArray* P2PRC4SessionKey;
    const fun::Uuid* ConnectionTag;

    // Construction
    inline CLanS2C_P2PGroup_MemberJoin_PArgs(const fun::HostId* GroupId_, const fun::HostId* MemberId_, const fun::ByteArray* CustomField_, const fun::uint32* EventId_, const fun::ByteArray* P2PAESSessionKey_, const fun::ByteArray* P2PRC4SessionKey_, const fun::Uuid* ConnectionTag_)
      : group_id(GroupId_)
      , member_id(MemberId_)
      , custom_field(CustomField_)
      , event_id(EventId_)
      , P2PAESSessionKey(P2PAESSessionKey_)
      , P2PRC4SessionKey(P2PRC4SessionKey_)
      , ConnectionTag(ConnectionTag_)
    {}

    // Writer
    inline void Write(fun::IMessageOut& Output__)
    {
      EngineTypes_UserTypeHandlers::Write(Output__, *group_id);
      EngineTypes_UserTypeHandlers::Write(Output__, *member_id);
      fun::FlexFormat::WriteBytes(Output__, *custom_field);
      fun::FlexFormat::WriteUInt32(Output__, *event_id);
      fun::FlexFormat::WriteBytes(Output__, *P2PAESSessionKey);
      fun::FlexFormat::WriteBytes(Output__, *P2PRC4SessionKey);
      fun::FlexFormat::WriteGuid(Output__, *ConnectionTag);
    }

    // ToString(Json style for diagnostics)
    inline fun::String ToString() const
    {
      fun::String result = TEXT("{");
      result << TEXT("\"group_id\":") << fun::ToString(*group_id);
      result << TEXT("\",member_id\":") << fun::ToString(*member_id);
      result << TEXT("\",custom_field\":") << fun::ToString(*custom_field);
      result << TEXT("\",event_id\":") << fun::ToString(*event_id);
      result << TEXT("\",P2PAESSessionKey\":") << fun::ToString(*P2PAESSessionKey);
      result << TEXT("\",P2PRC4SessionKey\":") << fun::ToString(*P2PRC4SessionKey);
      result << TEXT("\",ConnectionTag\":") << fun::ToString(*ConnectionTag);
      result << TEXT("}");
      return result;
    }
  };

  struct CLanS2C_P2PGroup_MemberJoin_Unencrypted_Args
  {
    // arguments.
    fun::HostId group_id;
    fun::HostId member_id;
    fun::ByteArray custom_field;
    fun::uint32 event_id;
    fun::Uuid ConnectionTag;

    // Construction
    inline CLanS2C_P2PGroup_MemberJoin_Unencrypted_Args() {}

    // Reader
    bool Read(fun::IMessageIn& Input__)
    {
      #define DO_CHECKED__(Expr) { if (!(Expr)) return false; }
      DO_CHECKED__(EngineTypes_UserTypeHandlers::Read(Input__, group_id));
      DO_CHECKED__(EngineTypes_UserTypeHandlers::Read(Input__, member_id));
      DO_CHECKED__(fun::FlexFormat::ReadBytes(Input__, custom_field));
      DO_CHECKED__(fun::FlexFormat::ReadUInt32(Input__, event_id));
      DO_CHECKED__(fun::FlexFormat::ReadGuid(Input__, ConnectionTag));
      return true;
      #undef DO_CHECKED__
    }

    // ToString(Json style for diagnostics)
    inline fun::String ToString() const
    {
      fun::String result = TEXT("{");
      result << TEXT("\"group_id\":") << fun::ToString(group_id);
      result << TEXT("\",member_id\":") << fun::ToString(member_id);
      result << TEXT("\",custom_field\":") << fun::ToString(custom_field);
      result << TEXT("\",event_id\":") << fun::ToString(event_id);
      result << TEXT("\",ConnectionTag\":") << fun::ToString(ConnectionTag);
      result << TEXT("}");
      return result;
    }
  };

  struct CLanS2C_P2PGroup_MemberJoin_Unencrypted_PArgs
  {
    // arguments.
    const fun::HostId* group_id;
    const fun::HostId* member_id;
    const fun::ByteArray* custom_field;
    const fun::uint32* event_id;
    const fun::Uuid* ConnectionTag;

    // Construction
    inline CLanS2C_P2PGroup_MemberJoin_Unencrypted_PArgs(const fun::HostId* GroupId_, const fun::HostId* MemberId_, const fun::ByteArray* CustomField_, const fun::uint32* EventId_, const fun::Uuid* ConnectionTag_)
      : group_id(GroupId_)
      , member_id(MemberId_)
      , custom_field(CustomField_)
      , event_id(EventId_)
      , ConnectionTag(ConnectionTag_)
    {}

    // Writer
    inline void Write(fun::IMessageOut& Output__)
    {
      EngineTypes_UserTypeHandlers::Write(Output__, *group_id);
      EngineTypes_UserTypeHandlers::Write(Output__, *member_id);
      fun::FlexFormat::WriteBytes(Output__, *custom_field);
      fun::FlexFormat::WriteUInt32(Output__, *event_id);
      fun::FlexFormat::WriteGuid(Output__, *ConnectionTag);
    }

    // ToString(Json style for diagnostics)
    inline fun::String ToString() const
    {
      fun::String result = TEXT("{");
      result << TEXT("\"group_id\":") << fun::ToString(*group_id);
      result << TEXT("\",member_id\":") << fun::ToString(*member_id);
      result << TEXT("\",custom_field\":") << fun::ToString(*custom_field);
      result << TEXT("\",event_id\":") << fun::ToString(*event_id);
      result << TEXT("\",ConnectionTag\":") << fun::ToString(*ConnectionTag);
      result << TEXT("}");
      return result;
    }
  };

  struct CLanS2C_P2PGroup_MemberLeave_Args
  {
    // arguments.
    fun::HostId member_id;
    fun::HostId group_id;

    // Construction
    inline CLanS2C_P2PGroup_MemberLeave_Args() {}

    // Reader
    bool Read(fun::IMessageIn& Input__)
    {
      #define DO_CHECKED__(Expr) { if (!(Expr)) return false; }
      DO_CHECKED__(EngineTypes_UserTypeHandlers::Read(Input__, member_id));
      DO_CHECKED__(EngineTypes_UserTypeHandlers::Read(Input__, group_id));
      return true;
      #undef DO_CHECKED__
    }

    // ToString(Json style for diagnostics)
    inline fun::String ToString() const
    {
      fun::String result = TEXT("{");
      result << TEXT("\"member_id\":") << fun::ToString(member_id);
      result << TEXT("\",group_id\":") << fun::ToString(group_id);
      result << TEXT("}");
      return result;
    }
  };

  struct CLanS2C_P2PGroup_MemberLeave_PArgs
  {
    // arguments.
    const fun::HostId* member_id;
    const fun::HostId* group_id;

    // Construction
    inline CLanS2C_P2PGroup_MemberLeave_PArgs(const fun::HostId* MemberId_, const fun::HostId* GroupId_)
      : member_id(MemberId_)
      , group_id(GroupId_)
    {}

    // Writer
    inline void Write(fun::IMessageOut& Output__)
    {
      EngineTypes_UserTypeHandlers::Write(Output__, *member_id);
      EngineTypes_UserTypeHandlers::Write(Output__, *group_id);
    }

    // ToString(Json style for diagnostics)
    inline fun::String ToString() const
    {
      fun::String result = TEXT("{");
      result << TEXT("\"member_id\":") << fun::ToString(*member_id);
      result << TEXT("\",group_id\":") << fun::ToString(*group_id);
      result << TEXT("}");
      return result;
    }
  };

  struct CLanS2C_P2PConnectStart_Args
  {
    // arguments.
    fun::HostId peer_id;
    fun::InetAddress external_addr;

    // Construction
    inline CLanS2C_P2PConnectStart_Args() {}

    // Reader
    bool Read(fun::IMessageIn& Input__)
    {
      #define DO_CHECKED__(Expr) { if (!(Expr)) return false; }
      DO_CHECKED__(EngineTypes_UserTypeHandlers::Read(Input__, peer_id));
      DO_CHECKED__(EngineTypes_UserTypeHandlers::Read(Input__, external_addr));
      return true;
      #undef DO_CHECKED__
    }

    // ToString(Json style for diagnostics)
    inline fun::String ToString() const
    {
      fun::String result = TEXT("{");
      result << TEXT("\"peer_id\":") << fun::ToString(peer_id);
      result << TEXT("\",external_addr\":") << fun::ToString(external_addr);
      result << TEXT("}");
      return result;
    }
  };

  struct CLanS2C_P2PConnectStart_PArgs
  {
    // arguments.
    const fun::HostId* peer_id;
    const fun::InetAddress* external_addr;

    // Construction
    inline CLanS2C_P2PConnectStart_PArgs(const fun::HostId* PeerId_, const fun::InetAddress* ExternalAddr_)
      : peer_id(PeerId_)
      , external_addr(ExternalAddr_)
    {}

    // Writer
    inline void Write(fun::IMessageOut& Output__)
    {
      EngineTypes_UserTypeHandlers::Write(Output__, *peer_id);
      EngineTypes_UserTypeHandlers::Write(Output__, *external_addr);
    }

    // ToString(Json style for diagnostics)
    inline fun::String ToString() const
    {
      fun::String result = TEXT("{");
      result << TEXT("\"peer_id\":") << fun::ToString(*peer_id);
      result << TEXT("\",external_addr\":") << fun::ToString(*external_addr);
      result << TEXT("}");
      return result;
    }
  };

  struct CLanS2C_GroupP2PConnectionComplete_Args
  {
    // arguments.
    fun::HostId group_id;

    // Construction
    inline CLanS2C_GroupP2PConnectionComplete_Args() {}

    // Reader
    bool Read(fun::IMessageIn& Input__)
    {
      #define DO_CHECKED__(Expr) { if (!(Expr)) return false; }
      DO_CHECKED__(EngineTypes_UserTypeHandlers::Read(Input__, group_id));
      return true;
      #undef DO_CHECKED__
    }

    // ToString(Json style for diagnostics)
    inline fun::String ToString() const
    {
      fun::String result = TEXT("{");
      result << TEXT("\"group_id\":") << fun::ToString(group_id);
      result << TEXT("}");
      return result;
    }
  };

  struct CLanS2C_GroupP2PConnectionComplete_PArgs
  {
    // arguments.
    const fun::HostId* group_id;

    // Construction
    inline CLanS2C_GroupP2PConnectionComplete_PArgs(const fun::HostId* GroupId_)
      : group_id(GroupId_)
    {}

    // Writer
    inline void Write(fun::IMessageOut& Output__)
    {
      EngineTypes_UserTypeHandlers::Write(Output__, *group_id);
    }

    // ToString(Json style for diagnostics)
    inline fun::String ToString() const
    {
      fun::String result = TEXT("{");
      result << TEXT("\"group_id\":") << fun::ToString(*group_id);
      result << TEXT("}");
      return result;
    }
  };
} // end of namespace LanS2C


} // end of namespace fun

#undef IDL_BEGIN_GLOBAL_NAMESPACE
#undef IDL_END_GLOBAL_NAMESPACE