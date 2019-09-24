#pragma once

namespace fun {
namespace net {

class InternalSocket;
class LanClientImpl;

class LanListener_C : public Runnable {
 public:
  LanListener_C(LanClientImpl* owner);
  ~LanListener_C();

  void StartListening();
  void StopListening();
  bool IsListening() const;

 private:
  bool AcceptNewSocket(InternalSocket* new_socket,
                       const InetAddress& remote_addr);
  InternalSocket* NewAcceptPendedSocket();

  void CatchThreadUnexpectedExit(const char* where, const char* reason);

  friend class LanClientImpl;

  FUN_ALIGNED_VOLATILE bool should_stop_;
  LanClientImpl* owner_;
  UniquePtr<Thread> thread_;

  // Runnable interface
  void Run() override;
};

}  // namespace net
}  // namespace fun
