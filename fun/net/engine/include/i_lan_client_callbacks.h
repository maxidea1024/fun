#pragma once

#include "fun/net/net.h"
#include "INetCoreCallbacks.h"

namespace fun {
namespace net {

class ResultInfo;
class ByteArray;
class UserWorkerThreadCallbackContext;

/**
 * 주의:
 * LanClient는 멀티스레드 형태로 동작하므로, mutex 객체로
 * 데이터를 보호 해야함.
 */
class ILanClientCallbacks : public INetCoreCallbacks {
 public:
  bool bHolsterMoreCallback_FORONETHREADEDMODEL;
  bool bPostponeThisCallback_FORONETHREADEDMODEL;

  /**
   * Default constructor.
   */
  ILanClientCallbacks()
    : bHolsterMoreCallback_FORONETHREADEDMODEL(false)
    , bPostponeThisCallback_FORONETHREADEDMODEL(false) {
  }

  /**
   * Called when this client is connected to server.
   */
  virtual void OnJoinedToServer(const ResultInfo* result_info,
                                const ByteArray& reply_from_server) = 0;

  /**
   * Called when owner client is disconnected from server.
   */
  virtual void OnLeftFromServer(const ResultInfo* result_info) = 0;

  /**
   * Called when a P2P member joined into P2P group.
   */
  virtual void OnP2PMemberJoined( HostId member_id,
                                  HostId group_id,
                                  int32 member_count,
                                  const ByteArray& custom_field) = 0;

  /**
   * Called when P2P connection is established.
   */
  virtual void OnP2PConnectionEstablished(HostId remote_id) = 0;

  /**
   * Called when a peer is disconnected from P2P group.
   */
  virtual void OnP2PDisconnected(HostId remote_id, ResultCode result_code) = 0;

  /**
   * Called when P2P group's all connections is completed.
   */
  virtual void OnGroupP2PConnectionComplete(HostId group_id) = 0;

  /**
   * P2P 멤버가 해당 P2P 그룹에서 나갔음을 알릴때 호출됨.
   */
  virtual void OnP2PMemberLeft( HostId member_id,
                                HostId group_id,
                                int32 member_count) = 0;

  virtual void OnSynchronizeServerTime() = 0;

  virtual void OnUserWorkerThreadBegin() = 0; //딱히 쓸모가 없어보임...
  virtual void OnUserWorkerThreadEnd() = 0; //딱히 쓸모가 없어보임...

  virtual void OnUserWorkerThreadCallbackBegin(UserWorkerThreadCallbackContext* context) {} //딱히 쓸모가 없어보임...
  virtual void OnUserWorkerThreadCallbackEnd(UserWorkerThreadCallbackContext* context) {} //딱히 쓸모가 없어보임...

  virtual void OnTick(void* context) {}

  void PostponeThisCallback();
};

} // namespace net
} // namespace fun
