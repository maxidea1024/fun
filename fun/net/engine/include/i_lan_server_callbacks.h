#pragma once

#include "fun/net/net.h"
#include "fun/net/i_net_core_callbacks.h"

namespace fun {
namespace net {

class NetClientInfo;
class ByteArray;
class InetAddress;
class UserWorkerThreadCallbackContext;

class ILanServerCallbacks : public INetCoreCallbacks {
 public:
  virtual bool OnConnectionRequest( const InetAddress& client_addr,
                                    const ByteArray& user_data_from_client,
                                    ByteArray& reply) {
    return true;
  }

  virtual void OnClientJoined(const NetClientInfo* client_info) = 0;

  virtual void OnClientLeft(const NetClientInfo* client_info,
                            const ResultInfo* result_info,
                            const ByteArray& comment) = 0;

  virtual void OnP2PGroupJoinMemberAckComplete( HostId group_id,
                                                HostId member_id,
                                                ResultCode result) = 0;

  virtual void OnP2PConnectionEstablished(HostId member_id, HostId remote_id) = 0;

  virtual void OnP2PDisconnected( HostId member_id,
                                  HostId remote_id,
                                  ResultCode result) = 0;

  virtual void OnGroupP2PConnectionComplete(HostId group_id) = 0;

  virtual void OnP2PGroupRemoved(HostId group_id) {}

  virtual void OnClientHackSuspected(HostId client_id, HackType hack_type) {}

  virtual void OnUserWorkerThreadBegin() = 0;//딱히 쓸모가 없어보임...
  virtual void OnUserWorkerThreadEnd() = 0;//딱히 쓸모가 없어보임...

  virtual void OnUserWorkerThreadCallbackBegin(UserWorkerThreadCallbackContext* context) {}//딱히 쓸모가 없어보임...
  virtual void OnUserWorkerThreadCallbackEnd(UserWorkerThreadCallbackContext* context) {}//딱히 쓸모가 없어보임...

  virtual void OnTick(void* context) {}
};

} // namespace net
} // namespace fun
