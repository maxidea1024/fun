#pragma once

namespace fun {
namespace net {

class NetServerImpl;
class InternalSocket;

class NetListener_S : public Runnable {
 public:
  NetListener_S(NetServerImpl* owner);
  virtual ~NetListener_S();

  /**
   * Start listening.
   */
  void StartListening();

  /**
   * Stop listening.
   */
  void StopListening();

  /**
   * Returns true if listening.
   */
  bool IsListening() const;

 private:
  bool AcceptNewSocket(InternalSocket* new_socket, const InetAddress& remote_addr);
  InternalSocket* NewAcceptPendedSocket();

  void CatchThreadUnexpectedExit(const char* where, const char* reason);

  NetServerImpl* owner;
  FUN_ALIGNED_VOLATILE bool should_stop_;
  UniquePtr<RunnableThread> thread_;

  // Runnable interface
  void Run() override;
};

} // namespace net
} // namespace fun
