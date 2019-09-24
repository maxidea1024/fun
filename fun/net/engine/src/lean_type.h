// TODO TypeId로 대체하도록 하자.

#pragma once

namespace fun {

/**
 * dynamic_cast를 피하기 위해서 사용함.
 */
enum class LeanType {
  None,

  TcpTransport_C,
  UdpSocket_C,

  RemoteClient_S,
  UdpSocket_S,

  NetServer,

  // 이하는 Lan Client/Server에서 사용함.
  LanClient_S,
  LanRemotePeer_C,
  AcceptedInfo,
  LanServer
};

}  // namespace fun
