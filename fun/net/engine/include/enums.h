#pragma once

#include "fun/net/net.h"

namespace fun {
namespace net {

/**
 * 메시지 전송 우선순위들입니다.
 */
enum class MessagePriority {
  Ring0 = 0,
  Ring1 = 1,
  High = 2,
  Medium = 3,
  Low = 4,
  Ring99 = 5,
  Last = 6,

  Holepunch = Ring99,
};

FUN_NETX_API TextStream& operator << (TextStream& stream, const MessagePriority value);


/**
 * 메시지 신뢰성 타입들입니다.
 */
enum class MessageReliability {
  Unreliable = 0,
  Reliable = 1,
  Last = 2,
};

FUN_NETX_API TextStream& operator << (TextStream& stream, const MessageReliability value);


/**
 * 결과 코드들입니다.
 */
enum class ResultCode {
  Ok = 0,
  Unexpected = 1,
  AlreadyConnected = 2,
  TCPConnectFailure = 3,
  InvalidSessionKey = 4,
  EncryptFail = 5,
  DecryptFail = 6,
  ConnectServerTimeout = 7,
  ProtocolVersionMismatch = 8,
  NotifyServerDeniedConnection = 9,
  ConnectServerSuccessful = 10,
  DisconnectFromRemote = 11,
  DisconnectFromLocal = 12,
  DangerousArgumentWarning = 13,
  UnknownEndPoint = 14,
  ServerNotReady = 15,
  ServerPortListenFailure = 16,
  AlreadyExists = 17,
  PermissionDenied = 18,
  BadSessionUuid = 19,
  InvalidCredential = 20,
  InvalidHeroName = 21,
  LoadDataPreceded = 22,
  AdjustedGamerIDNotFilled = 23,
  NoHero = 24,
  UnitTestFailed = 25,
  P2PUdpFailed = 26,
  RUdpFailed = 27,
  ServerUdpFailed = 28,
  NoP2PGroupRelation = 29,
  UserRequested = 30,
  InvalidPacketFormat = 31,
  TooLargeMessageDetected = 32,
  CannotEncryptUnreliableMessage = 33,
  ValueNotExist = 34,
  TimeOut = 35,
  LoadedDataNotFound = 36,
  SendQueueIsHeavy = 37,
  TooSlowHeartbeatWarning = 38,
  CompressFail = 39,
  LocalSocketCreationFailed = 40,
  NoneAvailableInPortPool = 41,
  InvalidPortPool = 42,
};

FUN_NETX_API TextStream& operator << (TextStream& stream, const ResultCode value);


/**
 * 호스트 아이디입니다.
 */
enum HostId {
  HostId_None = 0,
  HostId_Server = 1,
  HostId_Last = 2,
};

FUN_NETX_API TextStream& operator << (TextStream& stream, const HostId value);


/**
 * RPC 함수 ID입니다.
 */
enum RpcId {
  RpcId_None = 0,
};

FUN_NETX_API TextStream& operator << (TextStream& stream, const RpcId value);


/**
 * 로그 카테고리들입니다.
 */
enum class LogCategory {
  System = 0,
  TCP = 1,
  UDP = 2,
  P2P = 3,

  /** 서버와의 P2P */
  SP2P = 4,

  /** 피어와의 P2P */
  PP2P = 5,

  /** 랜서버끼리의 P2P */
  LP2P = 6,
};

FUN_NETX_API TextStream& operator << (TextStream& stream, const LogCategory value);


/**
 * 검출된 해킹 유형들입니다.
 */
enum class HackType {
  None = 0,

  /** 스피드핵. */
  SpeedHack = 1,

  /** 패킷 변조. */
  PacketRig = 2,
};

FUN_NETX_API TextStream& operator << (TextStream& stream, const HackType value);


/**
 * @warning 순서를 변경하면 절대 안됨!
 */
enum class ConnectionState {
  Connecting = 0,
  //EarlyConnected = 1, (단순히 통신만 연결된 상태)
  Connected = 2,
  Disconnecting = 3,
  Disconnected = 4,
};

FUN_NETX_API TextStream& operator << (TextStream& stream, const ConnectionState value);


/**
 * 후퇴모드
 */
enum class FallbackMethod {
  None = 0,
  PeersUdpToTcp = 1,
  ServerUdpToTcp = 2,
  CloseUdpSocket = 3,
};

FUN_NETX_API TextStream& operator << (TextStream& stream, const FallbackMethod value);


/**
 * P2P 시작 모드
 * TODO 구태여 enum으로 존재할 필요는 없을듯 싶음...
 */
enum class DirectP2PStartCondition {
  Jit = 0,
  Always = 1,
  Last = 2,
};

FUN_NETX_API TextStream& operator << (TextStream& stream, const DirectP2PStartCondition value);


/**
 *강한 암호화 레벨입니다.
 */
enum class StrongEncryptionLevel {
  None = 0,
  Low = 128,
  Middle = 192,
  High = 256,
};

FUN_NETX_API TextStream& operator << (TextStream& stream, const StrongEncryptionLevel value);


/**
 * 약한 암호화 레벨입니다.
 */
enum class WeakEncryptionLevel {
  None = 0,
  Low = 512,
  Middle = 1024,
  High = 2048,
};

FUN_NETX_API TextStream& operator << (TextStream& stream, const WeakEncryptionLevel value);


/**
 * 암호화 모드입니다.
 */
enum class EncryptionMode {
  /** Encryption을 수행하지 않음. */
  None = 0,

  /** RSA를 통해서 강력한 암호화를 수행함. */
  Strong = 1,

  /** RC4를 통해서 약한 암호화를 수행함. */
  Weak = 2,

  /** Lame. 제일 약한 암호화 방식. (쉽게 노출가능하지만, 속도는 빠름.) */
  //Lame = 3,

  Last = 3,
};

FUN_NETX_API TextStream& operator << (TextStream& stream, const EncryptionMode value);


/**
 * 압축모드입니다.
 */
enum class CompressionMode {
  /** 압축을 수행하지 않음. */
  None = 0,

  /** ZLIB를 통한 압축을 수행함. */
  Zip = 1,

  /** lzf 압축을 수행함. */
  Lzf = 2,

  /** LZ4 압축을 수행함. */
  LZ4 = 3,

  /** GOOGLE SNAPPY를 통한 압축을 수행함. */
  Snappy = 4,

  Last = 5,
};

FUN_NETX_API TextStream& operator << (TextStream& stream, const CompressionMode value);


/**
 * TODO private이면 안되려나??
 */
enum FrameNumber {
  //Invalid = 0
};

FUN_NETX_API TextStream& operator << (TextStream& stream, const FrameNumber value);


/*
//@todo 멀티 플랫폼 지원을 위해서는 일부 수정이 필요해보임.
//혹은 Translation을 해주는 함수를 통해서 처리하는게 바람직해 보임.
//소켓함수들 및 심볼을 사용하기 위해서, 상단에 헤더가 노출되는게 바람직하지 않음.

TODO : 조금더 자세히 처리해주는게 좋을듯...
*/
enum class SocketErrorCode {
  Ok = 0,
  Error = SOCKET_ERROR,
  Timeout = WSAETIMEDOUT,
  ConnectionRefused = WSAECONNREFUSED,
  ConnectResetByRemote = WSAECONNRESET,
  NotSocket = WSAENOTSOCK,
  ShutdownByRemote = WSAEDISCON, // FD_CLOSE or FD_SEND에서의 이벤트
  WouldBlock = WSAEWOULDBLOCK,
  IoPending = WSA_IO_PENDING,
  AccessError = WSAEACCES,
  OperationAborted = ERROR_OPERATION_ABORTED,
  InvalidArgument = WSAEINVAL,
};

FUN_NETX_API TextStream& operator << (TextStream& stream, const SocketErrorCode value);


/**
 * @todo 멀티 플랫폼지원
 */
enum class ShutdownFlag {
  Send = SD_SEND,
  Receive = SD_RECEIVE,
  Both = SD_BOTH,
};

FUN_NETX_API TextStream& operator << (TextStream& stream, const ShutdownFlag value);


enum class SocketType {
  /** TCP socket. */
  Tcp,

  /** UDP socket. */
  Udp,

  /** RAW socket. */
  Raw,
};

FUN_NETX_API TextStream& operator << (TextStream& stream, const SocketType value);


enum class RuntimePlatform {
  Unknown = 999,

  OSXEditor = 0,
  OSX = 1,
  Windows = 2,
  OSXWeb = 3,
  OSXDashboard = 4,
  WindowsWeb = 5,
  WindowsEditor = 7,
  IPhone = 8,
  PS3 = 9,
  XBOX360 = 10,
  Android = 11,
  NaCl = 12,
  Linux = 13,
  Flash = 15,
  LinuxEditor = 16,
  WebGL = 17,
  WSAX86 = 18,
  WSAX64 = 19,
  WSAARM = 20,
  WP8 = 21,
  BlackBerry = 22,
  Tizen = 23,
  PSP2 = 24,
  PS4 = 25,
  PSM = 26,
  XboxOne = 27,
  SamsungTV = 28,
  //29?
  WiiU = 30,
  tvOS = 31,
  Switch = 32,
};

FUN_NETX_API TextStream& operator << (TextStream& stream, const RuntimePlatform value);

} // namespace net
} // namespace fun
