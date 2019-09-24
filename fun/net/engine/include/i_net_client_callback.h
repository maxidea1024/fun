#pragma once

#include "INetCoreCallbacks.h"
#include "fun/net/net.h"

namespace fun {
namespace net {

class ResultInfo;
class ByteArray;

/**
 * TODO
 */
class INetClientCallbacks : public INetCoreCallbacks {
 public:
  INetClientCallbacks()
      : bHolsterMoreCallback_FORONETHREADEDMODEL(false),
        bPostponeThisCallback_FORONETHREADEDMODEL(false) {}

  /**
   * 클라가 서버에 정상적으로 접속을 한 직후 호출되는 콜백입니다.
   */
  virtual void OnJoinedToServer(const ResultInfo* result_info,
                                const ByteArray& reply_from_server) = 0;

  /**
   * 클라가 서버와의 접속이 끊어진 직후 호출되는 콜백입니다.
   */
  virtual void OnLeftFromServer(const ResultInfo* result_info) = 0;

  /**
   * 새로운 멤버가 P2P 그룹에 진입했을 경우에 호출되는 콜백입니다.
   */
  virtual void OnP2PMemberJoined(HostId member_id, HostId group_id,
                                 int32 member_count,
                                 const ByteArray& custom_field) = 0;

  /**
   * 멤버가 P2P 그룹에서 나갔을 경우에 호출되는 콜백입니다.
   */
  virtual void OnP2PMemberLeft(HostId member_id, HostId group_id,
                               int32 member_count) = 0;

  /**
   * Peer P2P의 상태가 변경 되었을때 호출되는 콜백입니다.
   */
  virtual void OnP2PStateChanged(HostId remote_id, ResultCode reason) = 0;

  /**
   * Server P2P의 상태가 변경 되었을때 호출되는 콜백입니다.
   */
  virtual void OnServerUdpStateChanged(ResultCode reason) {}

  /**
   * 서버와의 시간을 동기화할 수 있을때 호출되는 콜백입니다.
   */
  virtual void OnSynchronizeServerTime() = 0;

  // TODO 딱히 쓸모 없어보임...
  bool bHolsterMoreCallback_FORONETHREADEDMODEL;
  bool bPostponeThisCallback_FORONETHREADEDMODEL;

  FUN_NETX_API void HolsterMoreCallbackUntilNextTick();
  FUN_NETX_API void PostponeThisCallback();
};

}  // namespace net
}  // namespace fun
