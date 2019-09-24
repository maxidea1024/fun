#pragma once

namespace fun {
namespace net {

class InternalSocket;
class LanServerImpl;
class LanClient_S;

class LanListener_S : public Runnable {
 public:
  LanListener_S(LanServerImpl* owner);
  virtual ~LanListener_S();

  void StartListening();
  void StopListening();
  bool IsListening() const;

 private:
  bool AcceptNewSocket(InternalSocket* new_socket,
                       const InetAddress& remote_addr);
  InternalSocket* NewAcceptPendedSocket();

  void CatchThreadUnexpectedExit(const char* where, const char* reason);
  void CatchThreadExceptionAndPurgeClient(LanClient_S* client,
                                          const char* where,
                                          const char* reason);

  LanServerImpl* owner_;
  FUN_ALIGNED_VOLATILE bool should_stop_;
  UniquePtr<Thread> thread_;

  // Runnable interface
  void Run() override;
};

}  // namespace net
}  // namespace fun
