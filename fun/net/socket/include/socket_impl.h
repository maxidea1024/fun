#pragma once

#include "fun/base/base.h"
#include "fun/base/ref_counted_object.h"
#include "fun/base/timespan.h"

namespace fun {
namespace net {

class InetAddress;

/**
 * This class encapsulates the Berkeley sockets API.
 *
 * Subclasses implement specific socket types like
 * stream or datagram sockets.
 *
 * You should not create any instances of this class.
 */
class FUN_NETSOCKET_API SocketImpl : public RefCountedObject {
 public:
  enum SelectMode {
    SELECT_READ  = 1,
    SELECT_WRITE = 2,
    SELECT_ERROR = 4
  };

  SocketImpl(const SocketImpl&) = delete;
  SocketImpl& operator = (const SocketImpl&) = delete;

  /**
   * Get the next completed connection from the
   * socket's completed connection queue.
   *
   * If the queue is empty, waits until a connection
   * request completes.
   *
   * Returns a new TCP socket for the connection
   * with the client.
   *
   * The client socket's address is returned in clientAddr.
   */
  virtual SocketImpl* AcceptConnection(InetAddress& client_addr);

  /**
   * Initializes the socket and establishes a connection to
   * the TCP server at the given address.
   *
   * Can also be used for UDP sockets. In this case, no
   * connection is established. Instead, incoming and outgoing
   * packets are restricted to the specified address.
   */
  virtual void Connect(const InetAddress& address);

  /**
   * Initializes the socket, sets the socket timeout and
   * establishes a connection to the TCP server at the given address.
   */
  virtual void Connect(const InetAddress& address, const Timespan& timeout);

  /**
   * Initializes the socket and establishes a connection to
   * the TCP server at the given address. Prior to opening the
   * connection the socket is set to nonblocking mode.
   */
  virtual void ConnectNB(const InetAddress& address);

  /**
   * Bind a local address to the socket.
   *
   * This is usually only done when establishing a server
   * socket. TCP clients should not bind a socket to a
   * specific address.
   *
   * If reuse_addr is true, sets the SO_REUSEADDR
   * socket option.
   */
  virtual void Bind(const InetAddress& address, bool reuse_addr = false);
  virtual void Bind(const InetAddress& address, bool reuse_addr, bool reuse_port);

  /**
   * Bind a local IPv6 address to the socket.
   *
   * This is usually only done when establishing a server
   * socket. TCP clients should not bind a socket to a
   * specific address.
   *
   * If reuse_addr is true, sets the SO_REUSEADDR
   * socket option.
   *
   * The given address must be an IPv6 address. The
   * IPPROTO_IPV6/IPV6_V6ONLY option is set on the socket
   * according to the ipv6_only parameter.
   *
   * If the library has not been built with IPv6 support,
   * a NotImplementedException will be thrown.
   */
  virtual void Bind6(const InetAddress& address, bool reuse_addr = false, bool ipv6_only = false);
  virtual void Bind6(const InetAddress& address, bool reuse_addr, bool reuse_port, bool ipv6_only);

  /**
   * Puts the socket into listening state.
   *
   * The socket becomes a passive socket that
   * can accept incoming connection requests.
   *
   * The backlog argument specifies the maximum
   * number of connections that can be queued
   * for this socket.
   */
  virtual void Listen(int32 backlog = 64);

  /**
   * Close the socket.
   */
  virtual void Close();

  /**
   * Shuts down the receiving part of the socket connection.
   */
  virtual void ShutdownReceive();

  /**
   * Shuts down the sending part of the socket connection.
   */
  virtual void ShutdownSend();

  /**
   * Shuts down both the receiving and the sending part
   * of the socket connection.
   */
  virtual void Shutdown();

  /**
   * Sends the contents of the given data-buffer through
   * the socket.
   *
   * Returns the number of bytes sent, which may be
   * less than the number of bytes specified.
   *
   * Certain socket implementations may also return a negative
   * value denoting a certain condition.
   */
  virtual int32 SendBytes(const void* data, int32 length, int32 flags = 0);

  /**
   * Receives data from the socket and stores it
   * in buffer. Up to Length bytes are received.
   *
   * Returns the number of bytes received.
   *
   * Certain socket implementations may also return a negative
   * value denoting a certain condition.
   */
  virtual int32 ReceiveBytes(void* buffer, int32 length, int32 flags = 0);

  /**
   * Sends the contents of the given data-buffer through
   * the socket to the given address.
   *
   * Returns the number of bytes sent, which may be
   * less than the number of bytes specified.
   */
  virtual int32 SendTo(const void* data, int32 length, const InetAddress& address, int32 flags = 0);

  /**
   * Receives data from the socket and stores it
   * in buffer. Up to Length bytes are received.
   * Stores the address of the sender in address.
   *
   * Returns the number of bytes received.
   */
  virtual int32 ReceiveFrom(void* buffer, int32 length, InetAddress& address, int32 flags = 0);

  /**
   * Sends one byte of urgent data through the socket.
   *
   * The data is sent with the MSG_OOB flag.
   *
   * The preferred way for a socket to receive urgent data
   * is by enabling the SO_OOBINLINE option.
   */
  virtual void SendUrgent(uint8 data);

  /**
   * Returns the number of bytes available that can be read
   * without causing the socket to block.
   */
  virtual int32 Available();

  /**
   * Determines the status of the socket, using a
   * call to select().
   *
   * The mode argument is constructed by combining the values
   * of the SelectMode enumeration.
   *
   * Returns true if the next operation corresponding to
   * mode will not block, false otherwise.
   */
  virtual bool Poll(const Timespan& timeout, int32 mode);

  /**
   * Sets the size of the send buffer.
   */
  virtual void SetSendBufferSize(int32 size);

  /**
   * Returns the size of the send buffer.
   *
   * The returned value may be different than the
   * value previously set with SetSendBufferSize(),
   * as the system is free to adjust the value.
   */
  virtual int32 GetSendBufferSize();

  /**
   * Sets the size of the receive buffer.
   */
  virtual void SetReceiveBufferSize(int32 size);

  /**
   * Returns the size of the receive buffer.
   *
   * The returned value may be different than the
   * value previously set with SetReceiveBufferSize(),
   * as the system is free to adjust the value.
   */
  virtual int32 GetReceiveBufferSize();

  /**
   * Sets the send timeout for the socket.
   */
  virtual void SetSendTimeout(const Timespan& timeout);

  /**
   * Returns the send timeout for the socket.
   *
   * The returned timeout may be different than the
   * timeout previously set with SetSendTimeout(),
   * as the system is free to adjust the value.
   */
  virtual Timespan GetSendTimeout();

  /**
   * Sets the send timeout for the socket.
   *
   * On systems that do not support SO_RCVTIMEO, a
   * workaround using poll() is provided.
   */
  virtual void SetReceiveTimeout(const Timespan& timeout);

  /**
   * Returns the receive timeout for the socket.
   *
   * The returned timeout may be different than the
   * timeout previously set with SetReceiveTimeout(),
   * as the system is free to adjust the value.
   */
  virtual Timespan GetReceiveTimeout();

  /**
   * Returns the IP address and port number of the socket.
   */
  virtual InetAddress GetSocketAddress();

  /**
   * Returns the IP address and port number of the peer socket.
   */
  virtual InetAddress GetPeerAddress();

  /**
   * Sets the socket option specified by level and option
   * to the given integer value.
   */
  void SetOption(int32 level, int32 option, int32 value);

  /**
   * Sets the socket option specified by level and option
   * to the given integer value.
   */
  void SetOption(int32 level, int32 option, uint32 value);

  /**
   * Sets the socket option specified by level and option
   * to the given integer value.
   */
  void SetOption(int32 level, int32 option, uint8 value);

  /**
   * Sets the socket option specified by level and option
   * to the given time value.
   */
  void SetOption(int32 level, int32 option, const Timespan& value);

  /**
   * Sets the socket option specified by level and option
   * to the given time value.
   */
  void SetOption(int32 level, int32 option, const IpAddress& value);

  /**
   * Sets the socket option specified by level and option
   * to the given time value.
   */
  virtual void SetRawOption(int32 level, int32 option, const void* value, fun_socklen_t length);

  /**
   * Returns the value of the socket option
   * specified by level and option.
   */
  void GetOption(int32 level, int32 option, int32& value);

  /**
   * Returns the value of the socket option
   * specified by level and option.
   */
  void GetOption(int32 level, int32 option, uint32& value);

  /**
   * Returns the value of the socket option
   * specified by level and option.
   */
  void GetOption(int32 level, int32 option, uint8& value);

  /**
   * Returns the value of the socket option
   * specified by level and option.
   */
  void GetOption(int32 level, int32 option, Timespan& value);

  /**
   * Returns the value of the socket option
   * specified by level and option.
   */
  void GetOption(int32 level, int32 option, IpAddress& value);

  /**
   * Returns the value of the socket option
   * specified by level and option.
   */
  virtual void GetRawOption(int32 level, int32 option, void* value, fun_socklen_t& length);

  /**
   * Sets the value of the SO_LINGER socket option.
   */
  void SetLinger(bool on, int32 seconds);

  /**
   * Returns the value of the SO_LINGER socket option.
   */
  void GetLinger(bool& on, int32& seconds);

  /**
  Sets the value of the TCP_NODELAY socket option.
  */
  void SetNoDelay(bool flag);

  /**
   * Returns the value of the TCP_NODELAY socket option.
   */
  bool GetNoDelay();

  /**
   * Sets the value of the SO_KEEPALIVE socket option.
   */
  void SetKeepAlive(bool flag);

  /**
   * Returns the value of the SO_KEEPALIVE socket option.
   */
  bool GetKeepAlive();

  /**
   * Sets the value of the SO_REUSEADDR socket option.
   */
  void SetReuseAddress(bool flag);

  /**
   * Returns the value of the SO_REUSEADDR socket option.
   */
  bool GetReuseAddress();

  /**
   * Sets the value of the SO_REUSEPORT socket option.
   * Does nothing if the socket implementation does not
   * support SO_REUSEPORT.
   */
  void SetReusePort(bool flag);

  /**
   * Returns the value of the SO_REUSEPORT socket option.
   *
   * Returns false if the socket implementation does not
   * support SO_REUSEPORT.
   */
  bool GetReusePort();

  /**
   * Sets the value of the SO_OOBINLINE socket option.
   */
  void SetOOBInline(bool flag);

  /**
   * Returns the value of the SO_OOBINLINE socket option.
   */
  bool GetOOBInline();

  /**
   * Sets the value of the SO_BROADCAST socket option.
   */
  void SetBroadcast(bool flag);

  /**
   * Returns the value of the SO_BROADCAST socket option.
   */
  bool GetBroadcast();

  /**
   * Sets the socket in blocking mode if flag is true,
   * disables blocking mode if flag is false.
   */
  virtual void SetBlocking(bool flag);

  /**
   * Returns the blocking mode of the socket.
   * This method will only work if the blocking modes of
   * the socket are changed via the SetBlocking method!
   */
  virtual bool GetBlocking() const;

  /**
   * Returns true if the socket's connection is secure
   * (using SSL or TLS).
   */
  virtual bool Secure() const;

  /**
   * Returns the value of the SO_ERROR socket option.
   */
  int32 GetSocketError();

  /**
   * Returns the socket descriptor for the
   * underlying native socket.
   */
  SOCKET GetSocketHandle() const;

  /**
   * A wrapper for the ioctl system call.
   */
  void ioctl(fun_ioctl_request_t request, int32& arg);

  /**
   * A wrapper for the ioctl system call.
   */
  void ioctl(fun_ioctl_request_t request, void* arg);

#if FUN_PLATFORM_UNIX_FAMILY
  /**
   * A wrapper for the fcntl system call.
   */
  int32 fcntl(fun_fcntl_request_t request);

  /**
   * A wrapper for the fcntl system call.
   */
  int32 fcntl(fun_fcntl_request_t request, long arg);
#endif

  /**
   * Returns true if the underlying socket is initialized.
   */
  bool IsInitialized() const;

 protected:
  /**
   * Creates a SocketImpl.
   */
  SocketImpl();

  /**
   * Creates a SocketImpl using the given native socket.
   */
  SocketImpl(SOCKET socket_handle);

  /**
   * Destroys the SocketImpl.
   * Closes the socket if it is still open.
   */
  virtual ~SocketImpl();

  /**
   * Creates the underlying native socket.
   *
   * Subclasses must implement this method so
   * that it calls InitSocket() with the
   * appropriate arguments.
   *
   * The default implementation creates a
   * stream socket.
   */
  virtual void Init();

  /**
   * Creates the underlying native socket.
   *
   * The first argument, af, specifies the address family
   * used by the socket, which should be either AF_INET or
   * AF_INET6.
   *
   * The second argument, type, specifies the type of the
   * socket, which can be one of SOCK_STREAM, SOCK_DGRAM
   * or SOCK_RAW.
   *
   * The third argument, proto, is normally set to 0,
   * except for raw sockets.
   */
  void InitSocket(int32 af, int32 type, int32 proto = 0);

  /**
   * Allows subclasses to set the socket manually, if no valid socket is set yet.
   */
  void Reset(SOCKET new_fd = INVALID_SOCKET);

  /**
   * Returns the last error code.
   */
  static int32 GetLastError();

  /**
   * Throws an appropriate exception for the last error.
   */
  static void ThrowError();

  /**
   * Throws an appropriate exception for the last error.
   */
  static void ThrowError(const String& arg);

  /**
   * Throws an appropriate exception for the given error code.
   */
  static void ThrowError(int32 code);

  /**
   * Throws an appropriate exception for the given error code.
   */
  static void ThrowError(int32 code, const String& arg);

 private:
  SOCKET fd_;
  Timespan recv_timeout_;
  Timespan send_timeout_;
  bool blocking_;
  bool broken_timeout_;

  friend class Socket;
  friend class SecureSocketImpl;
};


//
// inlines
//

inline SOCKET SocketImpl::GetSocketHandle() const {
  return fd_;
}

inline bool SocketImpl::IsInitialized() const {
  return fd_ != INVALID_SOCKET;
}

inline int32 SocketImpl::GetLastError() {
#if FUN_PLATFORM_WINDOWS_FAMILY
  return WSAGetLastError();
#else
  return errno;
#endif
}

inline bool SocketImpl::GetBlocking() const {
  return blocking_;
}

} // namespace net
} // namespace fun
