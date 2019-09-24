#pragma once

#include "socket.h"
#include "fifo_buffer.h"

namespace fun {
namespace net {

class StreamSocketImpl;

/**
 * This class provides an interface to a TCP stream socket.
 */
class FUN_NETSOCKET_API StreamSocket : public Socket {
 public:
  /**
   * Creates an unconnected stream socket.
   * 
   * Before sending or receiving data, the socket
   * must be connected with a call to Connect().
   */
  StreamSocket();

  /**
   * Creates a stream socket and connects it to
   * the socket specified by address.
   */
  explicit StreamSocket(const InetAddress& address);

  /**
   * Creates the StreamSocket with the SocketImpl
   * from another socket. The SocketImpl must be
   * a StreamSocketImpl, otherwise an InvalidArgumentException
   * will be thrown.
   */
  StreamSocket(const Socket& socket);

  /**
   * Destroys the StreamSocket.
   */
  virtual ~StreamSocket();

  /**
   * Assignment operator.
   * 
   * Releases the socket's SocketImpl and
   * attaches the SocketImpl from the other socket and
   * increments the reference count of the SocketImpl.
   */
  StreamSocket& operator = (const Socket& socket);

  /**
   * Initializes the socket and establishes a connection to
   * the TCP server at the given address.
   * 
   * Can also be used for UDP sockets. In this case, no
   * connection is established. Instead, incoming and outgoing
   * packets are restricted to the specified address.
   */
  void Connect(const InetAddress& address);

  /**
   * Initializes the socket, sets the socket timeout and
   * establishes a connection to the TCP server at the given address.
   */
  void Connect(const InetAddress& address, const Timespan& timeout);

  /**
   * Initializes the socket and establishes a connection to
   * the TCP server at the given address. Prior to opening the
   * connection the socket is set to nonblocking mode.
   */
  void ConnectNB(const InetAddress& address);

  /**
   * Shuts down the receiving part of the socket connection.
   */
  void ShutdownReceive();

  /**
   * Shuts down the sending part of the socket connection.
   */
  void ShutdownSend();

  /**
   * Shuts down both the receiving and the sending part
   * of the socket connection.
   */
  void Shutdown();

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
  int32 SendBytes(const void* data, int32 length, int32 flags = 0);

  /**
   * Sends the contents of the given buffer through
   * the socket. FIFOBuffer has writable/readable transition
   * notifications which may be enabled to notify the caller when
   * the buffer transitions between empty, partially full and
   * full states.
   * 
   * Returns the number of bytes sent, which may be
   * less than the number of bytes specified.
   * 
   * Certain socket implementations may also return a negative
   * value denoting a certain condition.
   */
  int32 SendBytes(FIFOBuffer& buffer);

  /**
   * Receives data from the socket and stores it
   * in buffer. Up to length bytes are received.
   * 
   * Returns the number of bytes received.
   * A return value of 0 means a graceful shutdown
   * of the connection from the peer.
   * 
   * Throws a TimeoutException if a receive timeout has
   * been set and nothing is received within that interval.
   * Throws a NetException (or a subclass) in case of other errors.
   */
  int32 ReceiveBytes(void* buffer, int32 length, int32 flags = 0);

  /**
   * Receives data from the socket and stores it
   * in buffer. Up to length bytes are received. FIFOBuffer has
   * writable/readable transition notifications which may be enabled
   * to notify the caller when the buffer transitions between empty,
   * partially full and full states.
   * 
   * Returns the number of bytes received.
   * A return value of 0 means a graceful shutdown
   * of the connection from the peer.
   * 
   * Throws a TimeoutException if a receive timeout has
   * been set and nothing is received within that interval.
   * Throws a NetException (or a subclass) in case of other errors.
   */
  int32 ReceiveBytes(FIFOBuffer& buffer);

  /**
   * Sends one byte of urgent data through the socket.
   * 
   * The data is sent with the MSG_OOB flag.
   * 
   * The preferred way for a socket to receive urgent data
   * is by enabling the SO_OOBINLINE Option.
   */
  void SendUrgent(uint8 data);

  /**
   * Creates the socket and attaches the given SocketImpl.
   * The socket takes ownership of the SocketImpl.
   * 
   * The SocketImpl must be a StreamSocketImpl, otherwise
   * an InvalidArgumentException will be thrown.
   */
  StreamSocket(SocketImpl* impl);

 private:
  enum { BUFFER_SIZE = 1024 };

  friend class ServerSocket;
  //friend class SocketIOS;
};

} // namespace net
} // namespace fun
