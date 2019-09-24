//@todo IDLC로 다 빼버리자!

#include "fun/net/net.h"

namespace fun {
//namespace net {

#define VALUE2STRING(Enum, X)     case Enum::X: stream << #X; return stream;
#define RETURN_UNEXPECTED_VALUE   stream << "<undefined>"; return stream;

/*
TextStream& operator << (TextStream& stream, const HostIdArray& value) {
  stream << "[";
  for (int32 i = 0; i < value.Count(); ++i) {
    if (i != 0) stream << ",";
    stream << fun::ToString(value[i]);
  }
  stream << "]";
  return stream;
}
*/

TextStream& operator << (TextStream& stream, const HostId value) {
  return stream << String::FromNumber((uint32)value);
}

TextStream& operator << (TextStream& stream, const FrameNumber value) {
  return stream << String::FromNumber((uint32)value);
}

TextStream& operator << (TextStream& stream, const RpcId value) {
  return stream << String::FromNumber((uint32)value);
}

TextStream& operator << (TextStream& stream, const MessagePriority value) {
  switch (value) {
    VALUE2STRING(MessagePriority,Ring0)
    VALUE2STRING(MessagePriority,Ring1)
    VALUE2STRING(MessagePriority,High)
    VALUE2STRING(MessagePriority,Medium)
    VALUE2STRING(MessagePriority,Low)
    VALUE2STRING(MessagePriority,Ring99)
    VALUE2STRING(MessagePriority,Last)
  }
  RETURN_UNEXPECTED_VALUE;
}

TextStream& operator << (TextStream& stream, const MessageReliability value) {
  switch (value) {
    VALUE2STRING(MessageReliability,Unreliable)
    VALUE2STRING(MessageReliability,Reliable)
    VALUE2STRING(MessageReliability,Last)
  }
  RETURN_UNEXPECTED_VALUE;
}

TextStream& operator << (TextStream& stream, const ResultCode value) {
  switch (value) {
    VALUE2STRING(ResultCode,Ok)
    VALUE2STRING(ResultCode,Unexpected)
    VALUE2STRING(ResultCode,AlreadyConnected)
    VALUE2STRING(ResultCode,TCPConnectFailure)
    VALUE2STRING(ResultCode,InvalidSessionKey)
    VALUE2STRING(ResultCode,EncryptFail)
    VALUE2STRING(ResultCode,DecryptFail)
    VALUE2STRING(ResultCode,ConnectServerTimeout)
    VALUE2STRING(ResultCode,ProtocolVersionMismatch)
    VALUE2STRING(ResultCode,NotifyServerDeniedConnection)
    VALUE2STRING(ResultCode,ConnectServerSuccessful)
    VALUE2STRING(ResultCode,DisconnectFromRemote)
    VALUE2STRING(ResultCode,DisconnectFromLocal)
    VALUE2STRING(ResultCode,DangerousArgumentWarning)
    VALUE2STRING(ResultCode,UnknownEndPoint)
    VALUE2STRING(ResultCode,ServerNotReady)
    VALUE2STRING(ResultCode,ServerPortListenFailure)
    VALUE2STRING(ResultCode,AlreadyExists)
    VALUE2STRING(ResultCode,PermissionDenied)
    VALUE2STRING(ResultCode,BadSessionUuid)
    VALUE2STRING(ResultCode,InvalidCredential)
    VALUE2STRING(ResultCode,InvalidHeroName)
    VALUE2STRING(ResultCode,LoadDataPreceded)
    VALUE2STRING(ResultCode,AdjustedGamerIDNotFilled)
    VALUE2STRING(ResultCode,NoHero)
    VALUE2STRING(ResultCode,UnitTestFailed)
    VALUE2STRING(ResultCode,P2PUdpFailed)
    VALUE2STRING(ResultCode,RUdpFailed)
    VALUE2STRING(ResultCode,ServerUdpFailed)
    VALUE2STRING(ResultCode,NoP2PGroupRelation)
    VALUE2STRING(ResultCode,UserRequested)
    VALUE2STRING(ResultCode,InvalidPacketFormat)
    VALUE2STRING(ResultCode,TooLargeMessageDetected)
    VALUE2STRING(ResultCode,CannotEncryptUnreliableMessage)
    VALUE2STRING(ResultCode,ValueNotExist)
    VALUE2STRING(ResultCode,TimeOut)
    VALUE2STRING(ResultCode,LoadedDataNotFound)
    VALUE2STRING(ResultCode,SendQueueIsHeavy)
    VALUE2STRING(ResultCode,TooSlowHeartbeatWarning)
    VALUE2STRING(ResultCode,CompressFail)
    VALUE2STRING(ResultCode,LocalSocketCreationFailed)
    VALUE2STRING(ResultCode,NoneAvailableInPortPool)
    VALUE2STRING(ResultCode,InvalidPortPool)
  }
  RETURN_UNEXPECTED_VALUE;
}

TextStream& operator << (TextStream& stream, const LogCategory value) {
  switch (value) {
    VALUE2STRING(LogCategory,System)
    VALUE2STRING(LogCategory,TCP)
    VALUE2STRING(LogCategory,UDP)
    VALUE2STRING(LogCategory,P2P)
    VALUE2STRING(LogCategory,SP2P)
    VALUE2STRING(LogCategory,PP2P)
    VALUE2STRING(LogCategory,LP2P)
  }
  RETURN_UNEXPECTED_VALUE;
}

TextStream& operator << (TextStream& stream, const HackType value) {
  switch (value) {
    VALUE2STRING(HackType,None)
    VALUE2STRING(HackType,SpeedHack)
    VALUE2STRING(HackType,PacketRig)
  }
  RETURN_UNEXPECTED_VALUE;
}

TextStream& operator << (TextStream& stream, const ConnectionState value) {
  switch (value) {
    VALUE2STRING(ConnectionState,Disconnected)
    VALUE2STRING(ConnectionState,Connecting)
    VALUE2STRING(ConnectionState,Connected)
    VALUE2STRING(ConnectionState,Disconnecting)
  };
  RETURN_UNEXPECTED_VALUE;
}

TextStream& operator << (TextStream& stream, const FallbackMethod value) {
  switch (value) {
    VALUE2STRING(FallbackMethod,None)
    VALUE2STRING(FallbackMethod,PeersUdpToTcp)
    VALUE2STRING(FallbackMethod,ServerUdpToTcp)
    VALUE2STRING(FallbackMethod,CloseUdpSocket)
  }
  RETURN_UNEXPECTED_VALUE;
}

TextStream& operator << (TextStream& stream, const DirectP2PStartCondition value) {
  switch (value) {
    VALUE2STRING(DirectP2PStartCondition,Jit)
    VALUE2STRING(DirectP2PStartCondition,Always)
    VALUE2STRING(DirectP2PStartCondition,Last)
  }
  RETURN_UNEXPECTED_VALUE;
}

TextStream& operator << (TextStream& stream, const StrongEncryptionLevel value) {
  switch (value) {
    VALUE2STRING(StrongEncryptionLevel,None)
    VALUE2STRING(StrongEncryptionLevel,Low)
    VALUE2STRING(StrongEncryptionLevel,Middle)
    VALUE2STRING(StrongEncryptionLevel,High)
  }
  RETURN_UNEXPECTED_VALUE;
}

TextStream& operator << (TextStream& stream, const WeakEncryptionLevel value) {
  switch (value) {
    VALUE2STRING(WeakEncryptionLevel,None)
    VALUE2STRING(WeakEncryptionLevel,Low)
    VALUE2STRING(WeakEncryptionLevel,Middle)
    VALUE2STRING(WeakEncryptionLevel,High)
  }
  RETURN_UNEXPECTED_VALUE;
}

TextStream& operator << (TextStream& stream, const MessageType value) {
  switch (value) {
    VALUE2STRING(MessageType,None)
    VALUE2STRING(MessageType,RPC)
    VALUE2STRING(MessageType,FreeformMessage)
    VALUE2STRING(MessageType,ConnectServerTimedout)
    VALUE2STRING(MessageType,NotifyServerConnectionHint)
    VALUE2STRING(MessageType,NotifyCSEncryptedSessionKey)
    VALUE2STRING(MessageType,NotifyCSSessionKeySuccess)
    VALUE2STRING(MessageType,NotifyServerConnectionRequestData)
    VALUE2STRING(MessageType,NotifyProtocolVersionMismatch)
    VALUE2STRING(MessageType,NotifyServerDeniedConnection)
    VALUE2STRING(MessageType,NotifyServerConnectSuccess)
    VALUE2STRING(MessageType,RequestStartServerHolepunch)
    VALUE2STRING(MessageType,ServerHolepunch)
    VALUE2STRING(MessageType,ServerHolepunchAck)
    VALUE2STRING(MessageType,NotifyHolepunchSuccess)
    VALUE2STRING(MessageType,NotifyClientServerUdpMatched)
    VALUE2STRING(MessageType,PeerUdp_ServerHolepunch)
    VALUE2STRING(MessageType,PeerUdp_ServerHolepunchAck)
    VALUE2STRING(MessageType,PeerUdp_NotifyHolepunchSuccess)
    VALUE2STRING(MessageType,RUdp_Frame)
    VALUE2STRING(MessageType,ReliableRelay1)
    VALUE2STRING(MessageType,UnreliableRelay1)
    VALUE2STRING(MessageType,UnreliableRelay1_RelayDestListCompressed)
    VALUE2STRING(MessageType,LingerDataFrame1)
    VALUE2STRING(MessageType,ReliableRelay2)
    VALUE2STRING(MessageType,UnreliableRelay2)
    VALUE2STRING(MessageType,LingerDataFrame2)
    VALUE2STRING(MessageType,RequestServerTimeAndKeepAlive)
    VALUE2STRING(MessageType,SpeedHackDetectorPing)
    VALUE2STRING(MessageType,ReplyServerTime)
    VALUE2STRING(MessageType,ArbitaryTouch)
    VALUE2STRING(MessageType,PeerUdp_PeerHolepunch)
    VALUE2STRING(MessageType,PeerUdp_PeerHolepunchAck)
    VALUE2STRING(MessageType,P2PIndirectServerTimeAndPing)
    VALUE2STRING(MessageType,P2PIndirectServerTimeAndPong)
    VALUE2STRING(MessageType,S2CRoutedMulticast1)
    VALUE2STRING(MessageType,S2CRoutedMulticast2)
    VALUE2STRING(MessageType,Encrypted_Reliable)
    VALUE2STRING(MessageType,Encrypted_Unreliable)
    VALUE2STRING(MessageType,Compressed)
    VALUE2STRING(MessageType,RequestReceiveSpeedAtReceiverSide_NoRelay)
    VALUE2STRING(MessageType,ReplyReceiveSpeedAtReceiverSide_NoRelay)
    VALUE2STRING(MessageType,NotifyConnectionPeerRequestData)
    VALUE2STRING(MessageType,NotifyCSP2PDisconnected)
    VALUE2STRING(MessageType,NotifyConnectPeerRequestDataSucess)
    VALUE2STRING(MessageType,NotifyCSConnectionPeerSuccess)
    VALUE2STRING(MessageType,Ignore)
    VALUE2STRING(MessageType,RequestServerConnectionHint)
  }
  RETURN_UNEXPECTED_VALUE;
}

TextStream& operator << (TextStream& stream, const LocalEventType value) {
  switch (value) {
    VALUE2STRING(LocalEventType,None)
    VALUE2STRING(LocalEventType,ConnectServerFail)
    VALUE2STRING(LocalEventType,ClientServerDisconnect)
    VALUE2STRING(LocalEventType,ClientJoinCandidate)
    VALUE2STRING(LocalEventType,ClientJoinApproved)
    VALUE2STRING(LocalEventType,ClientLeaveAfterDispose)
    VALUE2STRING(LocalEventType,P2PAddMemberAckComplete)
    VALUE2STRING(LocalEventType,P2PAddMember)
    VALUE2STRING(LocalEventType,P2PLeaveMember)
    VALUE2STRING(LocalEventType,DirectP2PEnabled)
    VALUE2STRING(LocalEventType,RelayP2PEnabled)
    VALUE2STRING(LocalEventType,GroupP2PEnabled)
    VALUE2STRING(LocalEventType,ServerUdpChanged)
    VALUE2STRING(LocalEventType,SynchronizeServerTime)
    VALUE2STRING(LocalEventType,HackSuspected)
    VALUE2STRING(LocalEventType,TcpListenFail)
    VALUE2STRING(LocalEventType,P2PGroupRemoved)
    VALUE2STRING(LocalEventType,P2PDisconnected)
    //VALUE2STRING(LocalEventType,UnitTestFail)
    VALUE2STRING(LocalEventType,Error)
    VALUE2STRING(LocalEventType,Warning)
  }
  RETURN_UNEXPECTED_VALUE;
}

TextStream& operator << (TextStream& stream, const EncryptionMode value) {
  switch (value) {
    VALUE2STRING(EncryptionMode,None)
    VALUE2STRING(EncryptionMode,Strong)
    VALUE2STRING(EncryptionMode,Weak)
    VALUE2STRING(EncryptionMode,Last)
  }
  RETURN_UNEXPECTED_VALUE;
}

TextStream& operator << (TextStream& stream, const CompressionMode value) {
  switch (value) {
    VALUE2STRING(CompressionMode,None)
    VALUE2STRING(CompressionMode,Zip)
    VALUE2STRING(CompressionMode,Lzf)
    VALUE2STRING(CompressionMode,Snappy)
    VALUE2STRING(CompressionMode,Last)
  }
  RETURN_UNEXPECTED_VALUE;
}

TextStream& operator << (TextStream& stream, const RUdpFrameType value) {
  switch (value) {
    VALUE2STRING(RUdpFrameType,None)
    VALUE2STRING(RUdpFrameType,Data)
    VALUE2STRING(RUdpFrameType,Ack)
  }
  RETURN_UNEXPECTED_VALUE;
}

TextStream& operator << (TextStream& stream, const SocketErrorCode value) {
  switch (value) {
    VALUE2STRING(SocketErrorCode,Ok)
    VALUE2STRING(SocketErrorCode,Error)
    VALUE2STRING(SocketErrorCode,Timeout)
    VALUE2STRING(SocketErrorCode,ConnectionRefused)
    VALUE2STRING(SocketErrorCode,ConnectResetByRemote)
    VALUE2STRING(SocketErrorCode,NotSocket)
    VALUE2STRING(SocketErrorCode,ShutdownByRemote)
    VALUE2STRING(SocketErrorCode,WouldBlock)
    VALUE2STRING(SocketErrorCode,IoPending)
    VALUE2STRING(SocketErrorCode,AccessError)
    VALUE2STRING(SocketErrorCode,OperationAborted)
    VALUE2STRING(SocketErrorCode,InvalidArgument)
  }
  RETURN_UNEXPECTED_VALUE;
}

TextStream& operator << (TextStream& stream, const ShutdownFlag value) {
  switch (value) {
    VALUE2STRING(ShutdownFlag,Send)
    VALUE2STRING(ShutdownFlag,Receive)
    VALUE2STRING(ShutdownFlag,Both)
  }
  RETURN_UNEXPECTED_VALUE;
}

TextStream& operator << (TextStream& stream, const SocketType value) {
  switch (value) {
  VALUE2STRING(SocketType,Tcp)
  VALUE2STRING(SocketType,Udp)
  VALUE2STRING(SocketType,Raw)
  }
  RETURN_UNEXPECTED_VALUE;
}

TextStream& operator << (TextStream& stream, const RuntimePlatform value) {
  switch (value) {
    VALUE2STRING(RuntimePlatform,Unknown)

    VALUE2STRING(RuntimePlatform,OSXEditor)
    VALUE2STRING(RuntimePlatform,OSX)
    VALUE2STRING(RuntimePlatform,Windows)
    VALUE2STRING(RuntimePlatform,OSXWeb)
    VALUE2STRING(RuntimePlatform,OSXDashboard)
    VALUE2STRING(RuntimePlatform,WindowsWeb)
    VALUE2STRING(RuntimePlatform,WindowsEditor)
    VALUE2STRING(RuntimePlatform,IPhone)
    VALUE2STRING(RuntimePlatform,PS3)
    VALUE2STRING(RuntimePlatform,XBOX360)
    VALUE2STRING(RuntimePlatform,Android)
    VALUE2STRING(RuntimePlatform,NaCl)
    VALUE2STRING(RuntimePlatform,Linux)
    VALUE2STRING(RuntimePlatform,Flash)
    VALUE2STRING(RuntimePlatform,LinuxEditor)
    VALUE2STRING(RuntimePlatform,WebGL)
    VALUE2STRING(RuntimePlatform,WSAX86)
    VALUE2STRING(RuntimePlatform,WSAX64)
    VALUE2STRING(RuntimePlatform,WSAARM)
    VALUE2STRING(RuntimePlatform,WP8)
    VALUE2STRING(RuntimePlatform,BlackBerry)
    VALUE2STRING(RuntimePlatform,Tizen)
    VALUE2STRING(RuntimePlatform,PSP2)
    VALUE2STRING(RuntimePlatform,PS4)
    VALUE2STRING(RuntimePlatform,PSM)
    VALUE2STRING(RuntimePlatform,XboxOne)
    VALUE2STRING(RuntimePlatform,SamsungTV)
    VALUE2STRING(RuntimePlatform,WiiU)
    VALUE2STRING(RuntimePlatform,tvOS)
    VALUE2STRING(RuntimePlatform,Switch)
  }
  RETURN_UNEXPECTED_VALUE;
}

#undef RETURN_UNEXPECTED_VALUE
#undef VALUE2STRING

//} // namespace net
} // namespace fun
