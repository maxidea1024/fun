//@deprecated 대체할 방법을 찾아보도록 하자.
#include "CorePrivatePCH.h"
#include "fun/net/net.h"

#include "RemoteClient.h"
#include "LanRemoteClient.h"
#include "NetClient.h"
#include "LanClient.h"

namespace fun {
namespace net {

UdpSocket_S* LeanDynamicCast(ICompletionContext* x) {
  if (x && x->GetLeanType() == LeanType::UdpSocket_S) {
    return static_cast<UdpSocket_S*>(x);
  } else {
    return nullptr;
  }
}

RemoteClient_S* LeanDynamicCast(ISendDest_S* x) {
  if (x && x->GetLeanType() == LeanType::RemoteClient_S) {
    return static_cast<RemoteClient_S*>(x);
  } else {
    return nullptr;
  }
}

RemoteClient_S* LeanDynamicCast2(ICompletionContext* x) {
  if (x && x->GetLeanType() == LeanType::RemoteClient_S) {
    return static_cast<RemoteClient_S*>(x);
  } else {
    return nullptr;
  }
}

LanClient_S* LeanDynamicCastForLanClient(ISendDest_S* x) {
  if (x && x->GetLeanType() == LeanType::LanClient_S) {
    return static_cast<LanClient_S*>(x);
  } else {
    return nullptr;
  }
}

LanClient_S* LeanDynamicCast2ForLanClient(ICompletionContext* x) {
  if (x && x->GetLeanType() == LeanType::LanClient_S) {
    return static_cast<LanClient_S*>(x);
  } else {
    return nullptr;
  }
}

TcpTransport_C* LeanDynamicCastTcpTransport_C(ICompletionContext* x) {
  if (x && x->GetLeanType() == LeanType::TcpTransport_C) {
    return static_cast<TcpTransport_C*>(x);
  } else {
    return nullptr;
  }
}

LanRemotePeer_C* LeanDynamicCastRemotePeer_C(ICompletionContext* x) {
  if (x && x->GetLeanType() == LeanType::LanRemotePeer_C) {
    return static_cast<LanRemotePeer_C*>(x);
  } else {
    return nullptr;
  }
}

AcceptedInfo* LeanDynamicCastAcceptedInfo(ICompletionContext* x) {
  if (x && x->GetLeanType() == LeanType::AcceptedInfo) {
    return static_cast<AcceptedInfo*>(x);
  } else {
    return nullptr;
  }
}

} // namespace net
} // namespace fun
