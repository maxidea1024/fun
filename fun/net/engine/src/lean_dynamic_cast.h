#pragma once

namespace fun {

class ICompletionContext;

class ISendDest_S;
class UdpSocket_S;
class RemoteClient_S;
class TcpTransport_C;

class LanClient_S;
class LanRemotePeer_C;
class AcceptedInfo;

UdpSocket_S* LeanDynamicCast(ICompletionContext* X);
RemoteClient_S* LeanDynamicCast(ISendDest_S* X);
RemoteClient_S* LeanDynamicCast2(ICompletionContext* X);
LanClient_S* LeanDynamicCastForLanClient(ISendDest_S* X);
LanClient_S* LeanDynamicCast2ForLanClient(ICompletionContext* X);

TcpTransport_C* LeanDynamicCastTcpTransport_C(ICompletionContext* X);
LanRemotePeer_C* LeanDynamicCastRemotePeer_C(ICompletionContext* X);

AcceptedInfo* LeanDynamicCastAcceptedInfo(ICompletionContext* X);

}  // namespace fun
