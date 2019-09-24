#pragma once

#include "server_socket.h"
#include "tcp_client.h"

namespace fun {
namespace net {

/**
 */
class FUN_NETSOCKET_API TcpServer {
 public:
  typedef Function<bool(const SharedPtr<TcpClient>&)> NewConnectionCallback;

  TcpServer();
  ~TcpServer();

  TcpServer(const TcpServer&) = delete;
  TcpServer& operator=(const TcpServer&) = delete;

  bool operator==(const TcpServer& rhs) const;
  bool operator!=(const TcpServer& rhs) const;

  /**
   * Start the tcp_server at the given host and port.
   */
  void Start(const String& addr, uint32 port,
             const NewConnectionCallback& cb = nullptr);

  /**
   * Disconnect the tcp_server if it was currently running.
   */
  void Stop(bool wait_for_removal = false,
            bool recursive_wait_for_removal = true);

  /**
   * Returns whether the server is currently running or not
   */
  bool IsRunning() const;

  /**
   * io service read callback
   */
  void OnReadAvailable(SOCKET fd);
  /**
   * client disconnected
   */
  void OnClientDisconnected(const SharedPtr<TcpClient>& client);

  // TODO 외부에서 락을 할 수 있어야할듯 싶은데??
  const Array<SharedPtr<TcpClient>>& GetClients() const;

 private:
  /** IO service. */
  SharedPtr<SocketIoService> io_service_;

  /** Server socket. */
  ServerSocket socket_;

  /** Whether the server is currently running or not */
  ThreadSafeBool is_running_;

  /** Client list. */
  Array<SharedPtr<TcpClient>> clients_;

  /** Mutex for client list. */
  Mutex clients_mutex_;

  /** New connection callback */
  NewConnectionCallback new_connection_cb_;
};

}  // namespace net
}  // namespace fun
