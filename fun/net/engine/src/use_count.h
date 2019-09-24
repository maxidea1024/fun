#pragma once

#include "fun/net/net.h"

namespace fun {
namespace net {

class InetAddress;
class SendFragRefs;
class UdpSendOption;

class IHostObject {
 public:
  virtual ~IHostObject() {}

  virtual SocketErrorCode IssueSend(double absolute_time) = 0;
  virtual CCriticalSection2& GetSendMutex() = 0;
  virtual CCriticalSection2& GetMutex() = 0;
  virtual void Decrease() = 0;
  virtual void OnIssueSendFail(const char* where, SocketErrorCode socket_error) = 0;
  virtual void SendWhenReady(HostId sender_id,
                            const InetAddress& sender_addr,
                            HostId dest_id,
                            const SendFragRefs& data_to_send,
                            const UdpSendOption& send_opt) = 0;
};

/**
 * UDP socket, remote 객체들 등은 NO Lock(main) 상태에서 NO dispose 보장해야 한다.
 */
class UseCount {
 public:
  UseCount();
  virtual ~UseCount();

  long GetUseCount();
  void IncreaseUseCount();
  void DecreaseUseCount();
  void AssertIsZeroUseCount();
  virtual bool IsLockedByCurrentThread() = 0;

 private:
  FUN_ALIGNED_VOLATILE int32 in_use_count_;
  //AtomicCounter in_use_count_;
};

class ScopedUseCounter {
 public:
  ScopedUseCounter(UseCount& use_count);
  ~ScopedUseCounter();

 private:
  UseCount* use_count_;
};

} // namespace net
} // namespace fun
