#pragma once

namespace fun {
namespace net {

class INetCoreCallbacks;
class SendFragRefs;
class ReceivedMessage;
class VizAgent;

class SendOption
{
 public:
  MessageReliability reliability;
  int32 unrealiable_s2c_routed_multicast_max_count;
  double unreliable_s2c_routed_multicast_max_ping;
  int32 max_direct_broadcast_count;
  uint64 unique_id;
  MessagePriority priority;
  EncryptionMode encryption_mode;
  CompressionMode compression_mode;
  bool enable_p2p_jit_trigger;
  bool bounce;
  bool allow_relayed_send;
  int32 ttl;
  double force_relay_threshold_ratio;
  bool conditional_fragging;
  bool engine_only_specific;

  inline SendOption()
    : reliability(MessageReliability::Last)
    , unrealiable_s2c_routed_multicast_max_count(0)
    , unreliable_s2c_routed_multicast_max_ping(NetConfig::unreliable_s2c_routed_multicast_max_ping_default)
    , max_direct_broadcast_count(0)
    , unique_id(0)
    , priority(MessagePriority::Last)
    , encryption_mode(EncryptionMode::Last)
    , enable_p2p_jit_trigger(true)
    , bounce(true)
    , allow_relayed_send(true)
    , ttl(-1)
    , force_relay_threshold_ratio(0)
    , engine_only_specific(false)
    , conditional_fragging(NetConfig::conditional_fragging_by_default)
    , compression_mode(CompressionMode::None)
  {
  }

  inline SendOption(const RpcCallOption& rpc_call_opt)
    : Reliability(rpc_call_opt.reliability)
    , unrealiable_s2c_routed_multicast_max_count(rpc_call_opt.unrealiable_s2c_routed_multicast_max_count)
    , unreliable_s2c_routed_multicast_max_ping(rpc_call_opt.unreliable_s2c_routed_multicast_max_ping)
    , max_direct_broadcast_count(rpc_call_opt.max_direct_p2p_multicast_count)
    , unique_id(rpc_call_opt.unique_id)
    , priority(rpc_call_opt.priority)
    , bounce(rpc_call_opt.bounce)
    , enable_p2p_jit_trigger(rpc_call_opt.enable_p2p_jit_trigger)
    , allow_relayed_send(rpc_call_opt.allow_relayed_send)
    , encryption_mode(rpc_call_opt.encryption_mode)
    , ttl(-1)
    , force_relay_threshold_ratio(rpc_call_opt.force_relay_threshold_ratio)
    , engine_only_specific(rpc_call_opt.engine_only_specific)
    , conditional_fragging(NetConfig::conditional_fragging_by_default)
    , compression_mode(rpc_call_opt.compression_mode)
  {
  }
};


class NetCoreImpl : public RpcHost
{
 protected:
  //@todo SendFragRefs에서 bytes로 변환하는 부분을 제거한다면 도움이 될듯 한데..
  //WSASend 부분에서 방법이 있을듯 싶음..

  bool Send_SecureLayer(const SendFragRefs& payload,
                        const SendOption& send_opt,
                        const HostId* sendto_list,
                        int32 sendto_count);

  bool Send_CompressLayer(const SendFragRefs& payload,
                          const SendOption& send_opt,
                          const HostId* sendto_list,
                          int32 sendto_count);

  virtual bool Send_BroadcastLayer( const SendFragRefs& payload,
                                    const SendOption& send_opt,
                                    const HostId* sendto_list,
                                    int32 sendto_count) = 0;

  virtual void EnqueueError(SharedPtr<ResultInfo> result_info) = 0;
  virtual void EnqueueWarning(SharedPtr<ResultInfo> result_info) = 0;

  virtual bool AsyncCallbackMayOccur() = 0;


  //
  // Synchronization
  //

 public:
  virtual CCriticalSection2& GetMutex() = 0;

  inline void CheckCriticalSectionDeadLock(const char* where)
  {
    if (NetConfig::deadlock_checking_enabled) {
      CheckCriticalSectionDeadLock_INTERNAL(where);
    }
  }

  virtual void CheckCriticalSectionDeadLock_INTERNAL(const char* where) = 0;


  //
  // Proxy / Stub
  //

  Array<RpcProxy*> proxies_nolock_;
  Array<RpcStub*> stubs_nolock_;

  void AttachProxy(RpcProxy* proxy);
  void AttachStub(RpcStub* stub);

  void DetachProxy(RpcProxy* proxy);
  void DetachStub(RpcStub* stub);

  virtual INetCoreCallbacks* GetCallbacks_NOLOCK() = 0;

  void CleanupEveryProxyAndStub();


  //
  // Encryption / Decryption
  //

  virtual bool NextEncryptCount(HostId remote, CryptoCountType& out_count) = 0;
  virtual void PrevEncryptCount(HostId remote) = 0;
  virtual bool GetExpectedDecryptCount(HostId remote, CryptoCountType& out_count) = 0;
  virtual bool NextDecryptCount(HostId remote) = 0;

  virtual SessionKey* GetCryptSessionKey(HostId remote, String& out_error) = 0;

  /**
   * 암호화해제.  출력물은 CMessageIn임.
   */
  bool DecryptMessage(MessageType msg_type, ReceivedMessage& received_msg, MessageIn& decrypted_output);


  //
  // Compression / Decompression
  //

  /**
   * 압축해제.  출력물은 CMessageIn임.
   */
  bool DecompressMessage(ReceivedMessage& received_msg, MessageIn& decompressed_output);


  //
  // Messages
  //

  /**
   * 주고받을 수 있는 RPC 혹은 사용자 정의 메시지의 최대 길이입니다.
   */
  virtual int32 GetMessageMaxLength() = 0;

  /**
   * 메시지를 모두 읽었는지 여부를 체크함.
   */
  void PostCheckReadMessage(IMessageIn& msg, RpcId rpc_id, const char* rpc_name);

  void ShowError_NOLOCK(SharedPtr<ResultInfo> result_info);
  void ShowNotImplementedRpcWarning(RpcId rpc_id, const char* rpc_name);

  //TODO 이게 꼭 필요한 물건인가??
  void CheckDefaultTimeoutTimeValidation(double timeout_sec);

  bool SendFreeform(const HostId* sendto_list,
                    int32 sendto_count,
                    const RpcCallOption& rpc_call_opt,
                    const uint8* payload,
                    int32 payload_length);


  //
  // Ctor / Dtor
  //

  NetCoreImpl();
  virtual ~NetCoreImpl();


  //
  // VizAgent
  //

  //TODO
  //SharedPtr<VizAgent> viz_agent_;
};

//TODO 그냥 제거하는쪽으로 처리하자...
//@todo 한곳으로 모으는게 좋을듯 싶은데.. 아니면, 글로벌 로컬라이징 기능을 지원하는게 좋을듯 함!
//extern const char* DuplicatedRpcIdErrorText;
//extern const char* BadRpcIdErrorText;
//extern const char* AsyncCallbackMayOccurErrorText;

} // namespace net
} // namespace fun
