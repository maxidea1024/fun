#pragma once

#include "fun/net/net.h"

namespace fun {
namespace net {

class ReceivedMessage;
class RpcHost;
class BeforeRpcSummary;
class AfterRpcSummary;

/**
TODO
*/
class FUN_NETX_API RpcStub {
 public:
  RpcHost* core_;

  /**
   * 엔진 전용임을 나타내는 값으로 엔진 개발자가 아니라면,
   * 이 값은 항상 false(기본값)이어야 합니다.
   */
  bool engine_specific_only_;

  bool notify_call_from_stub_enabled_;

  bool stub_profiling_enabled_;

  virtual const char* GetRpcStubName() const = 0;
  virtual int32 GetDeclaredRpcCount() const = 0;
  virtual const RpcId* GetDeclaredRpcIds() const = 0;
  virtual const char** GetDeclaredRpcNames() const = 0;

  virtual bool ProcessReceivedMessage(ReceivedMessage& received_msg,
                                      void* host_tag) = 0;
  void ShowUnknownHostIdWarning(HostId remote_id);

  RpcStub();
  virtual ~RpcStub();

  // TODO 이것도 제거하는게 좋을듯 싶다...
  //실제로 그다지 필요해보이지가 않음.
  //효용성에 대해서는 좀더 고민해보도록 하자.
  bool bHolsterMoreCallback_FORONETHREADEDMODEL;
  bool bPostponeThisCallback_FORONETHREADEDMODEL;

  void HolsterMoreCallbackUntilNextTick();
  void PostponeThisCallback();
};

}  // namespace net
}  // namespace fun
