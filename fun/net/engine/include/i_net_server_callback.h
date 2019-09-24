#pragma once

#include "INetCoreCallbacks.h"
#include "fun/net/net.h"

namespace fun {
namespace net {

class UserWorkerThreadCallbackContext;

class INetServerCallbacks : public INetCoreCallbacks {
 public:
  virtual bool OnConnectionRequest(const InetAddress& client_addr,
                                   const ByteArray& user_data_from_client,
                                   ByteArray& reply) {
    return true;
  }

  virtual void OnClientJoined(const NetClientInfo* client_info) = 0;

  virtual void OnClientLeft(const NetClientInfo* client_info,
                            const ResultInfo* result_info,
                            const ByteArray& comment) = 0;

  virtual void OnP2PGroupJoinMemberAckComplete(HostId group_id,
                                               HostId member_id,
                                               ResultCode result_code) = 0;

  virtual void OnP2PGroupRemoved(HostId group_id) {}

  virtual void OnClientHackSuspected(HostId client_id, HackType hack_type) {}

  virtual void OnUserWorkerThreadBegin() = 0;
  virtual void OnUserWorkerThreadEnd() = 0;

  virtual void OnUserWorkerThreadCallbackBegin(
      UserWorkerThreadCallbackContext* context) {}  //딱히 쓸모가 없어보임...
  virtual void OnUserWorkerThreadCallbackEnd(
      UserWorkerThreadCallbackContext* context) {}  //딱히 쓸모가 없어보임...

  virtual void OnTick(void* context) {}
};

class UserWorkerThreadCallbackContext {
 public:
};

}  // namespace net
}  // namespace fun
