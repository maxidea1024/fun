#pragma once

namespace fun {
namespace net {

class SocketImpl;

/**
 * Socket is the common base class for
 * StreamSocket, ServerSocket, CDatagramSocket and other
 * socket classes.
 * 
 * it provides operations common to all socket types.
 */
class FUN_NETSOCKET_API Socket {
 public:
  /**
   * The mode argument to Poll() and Select().
   */
  enum SelectMode {
    SELECT_READ  = 1,
    SELECT_WRITE = 2,
    SELECT_ERROR = 4
  };

  typedef Array<Socket> SocketList;

  /**
   * Creates an uninitialized socket.
   */
  Socket();

  /**
  Copy constructor.

  Attaches the SocketImpl from the other socket and
  increments the reference count of the SocketImpl.
  */
  Socket(const Socket& rhs);

  /**
  Assignment operator.

  Releases the socket's SocketImpl and
  attaches the SocketImpl from the other socket and
  increments the reference count of the SocketImpl.
  */
  Socket& operator = (const Socket& rhs);

  /**
  Destroys the Socket and releases the
  SocketImpl.
  */
  virtual ~Socket();

  /**
  Returns true if both sockets share the same
  SocketImpl, false otherwise.
  */
  bool operator == (const Socket& rhs) const;

  /**
  Returns false if both sockets share the same
  SocketImpl, true otherwise.
  */
  bool operator != (const Socket& rhs) const;

  /**
  Compares the SocketImpl pointers.
  */
  bool operator <  (const Socket& rhs) const;

  /**
  Compares the SocketImpl pointers.
  */
  bool operator <= (const Socket& rhs) const;

  /**
  Compares the SocketImpl pointers.
  */
  bool operator >  (const Socket& rhs) const;

  /**
  Compares the SocketImpl pointers.
  */
  bool operator >= (const Socket& rhs) const;

  /**
  Closes the socket.
  */
  void Close();

  /**
  Determines the status of one or more sockets,
  using a call to select().

  read_list contains the list of sockets which should be
  checked for readability.

  write_list contains the list of sockets which should be
  checked for writeability.

  ExceptList contains a list of sockets which should be
  checked for a pending error.

  Returns the number of sockets ready.

  After return,
    * read_list contains those sockets ready for reading,
    * write_list contains those sockets ready for writing,
    * error_list contains those sockets with a pending error.

  If the total number of sockets passed in read_list, write_list and
  error_list is zero, select() will return immediately and the
  return value will be 0.

  If one of the sockets passed to select() is closed while
  select() runs, select will return immediately. However,
  the closed socket will not be included in any list.
  In this case, the return value may be greater than the sum
  of all sockets in all list.
  */
  static int32 Select(SocketList& read_list,
                      SocketList& write_list,
                      SocketList& error_list,
                      const Timespan& timeout);

  /**
  Determines the status of the socket, using a
  call to Poll() or Select().

  The mode argument is constructed by combining the values
  of the SelectMode enumeration.

  Returns true if the next operation corresponding to
  mode will not block, false otherwise.
  */
  bool Poll(const Timespan& timeout, int32 mode) const;

  /**
  Returns the number of bytes Available that can be read
  without causing the socket to block.
  */
  int32 Available() const;

  /**
  Sets the Size of the send buffer.
  */
  void SetSendBufferSize(int32 Size);

  /**
  Returns the Size of the send buffer.

  The returned value may be different than the
  value previously set with SetSendBufferSize(),
  as the system is free to adjust the value.
  */
  int32 GetSendBufferSize() const;

  /**
  Sets the Size of the receive buffer.
  */
  void SetReceiveBufferSize(int32 Size);

  /**
  Returns the Size of the receive buffer.

  The returned value may be different than the
  value previously set with SetReceiveBufferSize(),
  as the system is free to adjust the value.
  */
  int32 GetReceiveBufferSize() const;

  /**
  Sets the send timeout for the socket.
  */
  void SetSendTimeout(const Timespan& timeout);

  /**
  Returns the send timeout for the socket.

  The returned timeout may be different than the
  timeout previously set with SetSendTimeout(),
  as the system is free to adjust the value.
  */
  Timespan GetSendTimeout() const;

  /**
  Sets the send timeout for the socket.

  On systems that do not support SO_RCVTIMEO, a
  workaround using Poll() is provided.
  */
  void SetReceiveTimeout(const Timespan& timeout);

  /**
  Returns the receive timeout for the socket.

  The returned timeout may be different than the
  timeout previously set with GetReceiveTimeout(),
  as the system is free to adjust the value.
  */
  Timespan GetReceiveTimeout() const;

  /**
  Sets the socket option specified by level and option
  to the given integer value.
  */
  void SetOption(int32 level, int32 option, int32 value);

  /**
  Sets the socket option specified by level and option
  to the given integer value.
  */
  void SetOption(int32 level, int32 option, uint32 value);

  /**
  Sets the socket option specified by level and option
  to the given integer value.
  */
  void SetOption(int32 level, int32 option, uint8 value);

  /**
  Sets the socket option specified by level and option
  to the given time value.
  */
  void SetOption(int32 level, int32 option, const Timespan& value);

  /**
  Sets the socket option specified by level and option
  to the given time value.
  */
  void SetOption(int32 level, int32 option, const IpAddress& value);

  /**
  Returns the value of the socket option
  specified by level and option.
  */
  void GetOption(int32 level, int32 option, int32& value) const;

  /**
  Returns the value of the socket option
  specified by level and option.
  */
  void GetOption(int32 level, int32 option, uint32& value) const;

  /**
  Returns the value of the socket option
  specified by level and option.
  */
  void GetOption(int32 level, int32 option, uint8& value) const;

  /**
  Returns the value of the socket option
  specified by level and option.
  */
  void GetOption(int32 level, int32 option, Timespan& value) const;

  /**
  Returns the value of the socket option
  specified by level and option.
  */
  void GetOption(int32 level, int32 option, IpAddress& value) const;

  /**
  Sets the value of the SO_LINGER socket option.
  */
  void SetLinger(bool On, int32 Seconds);

  /**
  Returns the value of the SO_LINGER socket option.
  */
  void GetLinger(bool& On, int32& Seconds) const;

  /**
  Sets the value of the TCP_NODELAY socket option.
  */
  void SetNoDelay(bool flag);

  /**
  Returns the value of the TCP_NODELAY socket option.
  */
  bool GetNoDelay() const;

  /**
  Sets the value of the SO_KEEPALIVE socket option.
  */
  void SetKeepAlive(bool flag);

  /**
  Returns the value of the SO_KEEPALIVE socket option.
  */
  bool GetKeepAlive() const;

  /**
  Sets the value of the SO_REUSEADDR socket option.
  */
  void SetReuseAddress(bool flag);

  /**
  Returns the value of the SO_REUSEADDR socket option.
  */
  bool GetReuseAddress() const;

  /**
  Sets the value of the SO_REUSEPORT socket option.
  Does nothing if the socket implementation does not
  support SO_REUSEPORT.
  */
  void SetReusePort(bool flag);

  /**
  Returns the value of the SO_REUSEPORT socket option.

  Returns false if the socket implementation does not
  support SO_REUSEPORT.
  */
  bool GetReusePort() const;

  /**
  Sets the value of the SO_OOBINLINE socket option.
  */
  void SetOOBInline(bool flag);

  /**
  Returns the value of the SO_OOBINLINE socket option.
  */
  bool GetOOBInline() const;

  /**
  Sets the socket in blocking mode if flag is true,
  disables blocking mode if flag is false.
  */
  void SetBlocking(bool flag);

  /**
  Returns the blocking mode of the socket.
  This method will only work if the blocking modes of
  the socket are changed via the SetBlocking method!
  */
  bool GetBlocking() const;

  /**
  Returns the IP address and port number of the socket.
  */
  InetAddress GetSocketAddress() const;

  /**
  Returns the IP address and port number of the peer socket.
  */
  InetAddress GetPeerAddress() const;

  /**
  Returns the SocketImpl for this socket.
  */
  SocketImpl* GetImpl() const;

  /**
  Returns true if the socket's connection is secure
  (using SSL or TLS).
  */
  bool Secure() const;

  /**
  Returns true if the system supports IPv4.
  */
  static bool SupportsIPv4();

  /**
  Returns true if the system supports IPv6.
  */
  static bool SupportsIPv6();

  /**
   * Creates the underlying system socket for the given
   * address family.
   * 
   * Normally, this method should not be called directly, as
   * socket creation will be handled automatically. There are
   * a few situations where calling this method after creation
   * of the Socket object makes sense. One example is setting
   * a socket option before calling Bind() On a ServerSocket.
   */
  void Init();

 protected:
  /**
   * Creates the Socket and attaches the given SocketImpl.
   * The socket takes ownership of the SocketImpl.
   */
  Socket(SocketImpl* impl);

  /**
   * Returns the socket descriptor for this socket.
   */
  fun_socket_t GetSocketHandle() const;

 private:
  friend class SocketIoService;

#if PLATFORM_HAVE_FD_POLL
  /**
   * Utility functor used to compare socket file descriptors.
   * Used in Poll() member function.
   */
  class FDCompare {
   public:
    FDCompare(int32 fd) : fd_(fd) {}

    inline bool operator()(const Socket& rhs) const {
      return rhs.GetSocketHandle() == fd_;
    }

   private:
    FDCompare();

    int32 fd_;
  };
#endif

  SocketImpl* impl_;
};

} // namespace net
} // namespace fun
