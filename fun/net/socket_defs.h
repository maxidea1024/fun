#pragma once

#include "fun/net/net.h"

namespace fun {
namespace net {

#define FUN_ENOERR  0

#if FUN_PLATFORM_WINDOWS_FAMILY

  #include <winsock2.h>
  #include <ws2tcpip.h>

  //TODO 이걸 여기에 포함시켜야할까?? 실질적으로는 InternalSocket에서만 사용되는데...
  #include <mswsock.h>

  #define FUN_INVALID_SOCKET  INVALID_SOCKET
  #define fun_socket_t        SOCKET
  #define fun_socklen_t       int
  #define fun_ioctl_request_t int
  #define fun_closesocket(s)  closesocket(s)
  #define FUN_EINTR           WSAEINTR
  #define FUN_EACCES          WSAEACCES
  #define FUN_EFAULT          WSAEFAULT
  #define FUN_EINVAL          WSAEINVAL
  #define FUN_EMFILE          WSAEMFILE
  #define FUN_EAGAIN          WSAEWOULDBLOCK
  #define FUN_EWOULDBLOCK     WSAEWOULDBLOCK
  #define FUN_EINPROGRESS     WSAEINPROGRESS
  #define FUN_EALREADY        WSAEALREADY
  #define FUN_ENOTSOCK        WSAENOTSOCK
  #define FUN_EDESTADDRREQ    WSAEDESTADDRREQ
  #define FUN_EMSGSIZE        WSAEMSGSIZE
  #define FUN_EPROTOTYPE      WSAEPROTOTYPE
  #define FUN_ENOPROTOOPT     WSAENOPROTOOPT
  #define FUN_EPROTONOSUPPORT WSAEPROTONOSUPPORT
  #define FUN_ESOCKTNOSUPPORT WSAESOCKTNOSUPPORT
  #define FUN_ENOTSUP         WSAEOPNOTSUPP
  #define FUN_EPFNOSUPPORT    WSAEPFNOSUPPORT
  #define FUN_EAFNOSUPPORT    WSAEAFNOSUPPORT
  #define FUN_EADDRINUSE      WSAEADDRINUSE
  #define FUN_EADDRNOTAVAIL   WSAEADDRNOTAVAIL
  #define FUN_ENETDOWN        WSAENETDOWN
  #define FUN_ENETUNREACH     WSAENETUNREACH
  #define FUN_ENETRESET       WSAENETRESET
  #define FUN_ECONNABORTED    WSAECONNABORTED
  #define FUN_ECONNRESET      WSAECONNRESET
  #define FUN_ENOBUFS         WSAENOBUFS
  #define FUN_EISCONN         WSAEISCONN
  #define FUN_ENOTCONN        WSAENOTCONN
  #define FUN_ESHUTDOWN       WSAESHUTDOWN
  #define FUN_ETIMEDOUT       WSAETIMEDOUT
  #define FUN_ECONNREFUSED    WSAECONNREFUSED
  #define FUN_EHOSTDOWN       WSAEHOSTDOWN
  #define FUN_EHOSTUNREACH    WSAEHOSTUNREACH
  #define FUN_ESYSNOTREADY    WSASYSNOTREADY
  #define FUN_ENOTINIT        WSANOTINITIALISED
  #define FUN_HOST_NOT_FOUND  WSAHOST_NOT_FOUND
  #define FUN_TRY_AGAIN       WSATRY_AGAIN
  #define FUN_NO_RECOVERY     WSANO_RECOVERY
  #define FUN_NO_DATA         WSANO_DATA
  #ifndef ADDRESS_FAMILY
  #define ADDRESS_FAMILY      USHORT
  #endif

#elif FUN_PLATFORM == FUN_PLATFORM_VXWORKS

  #include <hostLib.h>
  #include <ifLib.h>
  #include <inetLib.h>
  #include <ioLib.h>
  #include <resolvLib.h>
  #include <types.h>
  #include <socket.h>
  #include <netinet/tcp.h>

  #define FUN_INVALID_SOCKET  -1
  #define fun_socket_t        int
  #define fun_socklen_t       int
  #define fun_ioctl_request_t int
  #define fun_closesocket(s)  ::close(s)
  #define FUN_EINTR           EINTR
  #define FUN_EACCES          EACCES
  #define FUN_EFAULT          EFAULT
  #define FUN_EINVAL          EINVAL
  #define FUN_EMFILE          EMFILE
  #define FUN_EAGAIN          EAGAIN
  #define FUN_EWOULDBLOCK     EWOULDBLOCK
  #define FUN_EINPROGRESS     EINPROGRESS
  #define FUN_EALREADY        EALREADY
  #define FUN_ENOTSOCK        ENOTSOCK
  #define FUN_EDESTADDRREQ    EDESTADDRREQ
  #define FUN_EMSGSIZE        EMSGSIZE
  #define FUN_EPROTOTYPE      EPROTOTYPE
  #define FUN_ENOPROTOOPT     ENOPROTOOPT
  #define FUN_EPROTONOSUPPORT EPROTONOSUPPORT
  #define FUN_ESOCKTNOSUPPORT ESOCKTNOSUPPORT
  #define FUN_ENOTSUP         ENOTSUP
  #define FUN_EPFNOSUPPORT    EPFNOSUPPORT
  #define FUN_EAFNOSUPPORT    EAFNOSUPPORT
  #define FUN_EADDRINUSE      EADDRINUSE
  #define FUN_EADDRNOTAVAIL   EADDRNOTAVAIL
  #define FUN_ENETDOWN        ENETDOWN
  #define FUN_ENETUNREACH     ENETUNREACH
  #define FUN_ENETRESET       ENETRESET
  #define FUN_ECONNABORTED    ECONNABORTED
  #define FUN_ECONNRESET      ECONNRESET
  #define FUN_ENOBUFS         ENOBUFS
  #define FUN_EISCONN         EISCONN
  #define FUN_ENOTCONN        ENOTCONN
  #define FUN_ESHUTDOWN       ESHUTDOWN
  #define FUN_ETIMEDOUT       ETIMEDOUT
  #define FUN_ECONNREFUSED    ECONNREFUSED
  #define FUN_EHOSTDOWN       EHOSTDOWN
  #define FUN_EHOSTUNREACH    EHOSTUNREACH
  #define FUN_ESYSNOTREADY    -4
  #define FUN_ENOTINIT        -5
  #define FUN_HOST_NOT_FOUND  HOST_NOT_FOUND
  #define FUN_TRY_AGAIN       TRY_AGAIN
  #define FUN_NO_RECOVERY     NO_RECOVERY
  #define FUN_NO_DATA         NO_DATA

#elif FUN_PLATFORM_UNIX_FAMILY || PLATFORM_VMS_FAMILY

  #include <unistd.h>
  #include <errno.h>
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <sys/un.h>
  #include <fcntl.h>

  #if FUN_PLATFORM != FUN_PLATFORM_HPUX
  #include <sys/select.h>
  #endif

  #include <sys/ioctl.h>

  #if FUN_PLATFORM_VMS_FAMILY
  #include <inet.h>
  #else
  #include <arpa/inet.h>
  #endif

  #include <netinet/in.h>
  #include <netinet/tcp.h>
  #include <netdb.h>
  #if FUN_PLATFORM_UNIX_FAMILY
    #if FUN_PLATFORM == FUN_PLATFORM_LINUX
      // Net/src/NetworkInterface.cpp changed #include <linux/if.h> to #include <net/if.h>
      // no more conflict, can use #include <net/if.h> here
      #include <net/if.h>
    #elif FUN_PLATFORM == FUN_PLATFORM_HPUX
      extern "C" {
        #include <net/if.h>
      }
    #else
      #include <net/if.h>
    #endif
  #endif
  #if FUN_PLATFORM == FUN_PLATFORM_SOLARIS || FUN_PLATFORM == FUN_PLATFORM_MAC_OS_X
    #include <sys/sockio.h>
    #include <sys/filio.h>
  #endif
  #define FUN_INVALID_SOCKET    -1
    #define fun_socket_t        int
    #define fun_socklen_t       socklen_t
  #define fun_fcntl_request_t  int
  #if FUN_PLATFORM_BSD_FAMILY
    #define fun_ioctl_request_t unsigned long
  #else
    #define fun_ioctl_request_t int
  #endif
  #define fun_closesocket(s)  ::close(s)

  #define FUN_EINTR           EINTR
  #define FUN_EACCES          EACCES
  #define FUN_EFAULT          EFAULT
  #define FUN_EINVAL          EINVAL
  #define FUN_EMFILE          EMFILE
  #define FUN_EAGAIN          EAGAIN
  #define FUN_EWOULDBLOCK     EWOULDBLOCK
  #define FUN_EINPROGRESS     EINPROGRESS
  #define FUN_EALREADY        EALREADY
  #define FUN_ENOTSOCK        ENOTSOCK
  #define FUN_EDESTADDRREQ    EDESTADDRREQ
  #define FUN_EMSGSIZE        EMSGSIZE
  #define FUN_EPROTOTYPE      EPROTOTYPE
  #define FUN_ENOPROTOOPT     ENOPROTOOPT
  #define FUN_EPROTONOSUPPORT EPROTONOSUPPORT
  #if defined(ESOCKTNOSUPPORT)
  #define FUN_ESOCKTNOSUPPORT ESOCKTNOSUPPORT
  #else
  #define FUN_ESOCKTNOSUPPORT -1
  #endif
  #define FUN_ENOTSUP         ENOTSUP
  #define FUN_EPFNOSUPPORT    EPFNOSUPPORT
  #define FUN_EAFNOSUPPORT    EAFNOSUPPORT
  #define FUN_EADDRINUSE      EADDRINUSE
  #define FUN_EADDRNOTAVAIL   EADDRNOTAVAIL
  #define FUN_ENETDOWN        ENETDOWN
  #define FUN_ENETUNREACH     ENETUNREACH
  #define FUN_ENETRESET       ENETRESET
  #define FUN_ECONNABORTED    ECONNABORTED
  #define FUN_ECONNRESET      ECONNRESET
  #define FUN_ENOBUFS         ENOBUFS
  #define FUN_EISCONN         EISCONN
  #define FUN_ENOTCONN        ENOTCONN
  #if defined(ESHUTDOWN)
  #define FUN_ESHUTDOWN       ESHUTDOWN
  #else
  #define FUN_ESHUTDOWN   -2
  #endif
  #define FUN_ETIMEDOUT       ETIMEDOUT
  #define FUN_ECONNREFUSED    ECONNREFUSED
  #if defined(EHOSTDOWN)
  #define FUN_EHOSTDOWN       EHOSTDOWN
  #else
  #define FUN_EHOSTDOWN       -3
  #endif
  #define FUN_EHOSTUNREACH    EHOSTUNREACH
  #define FUN_ESYSNOTREADY    -4
  #define FUN_ENOTINIT        -5
  #define FUN_HOST_NOT_FOUND  HOST_NOT_FOUND
  #define FUN_TRY_AGAIN       TRY_AGAIN
  #define FUN_NO_RECOVERY     NO_RECOVERY
  #define FUN_NO_DATA         NO_DATA
#endif

#if FUN_PLATFORM_BSD_FAMILY || (FUN_PLATFORM == FUN_PLATFORM_TRU64) || (FUN_PLATFORM == FUN_PLATFORM_AIX) || (FUN_PLATFORM == FUN_PLATFORM_IRIX) || (FUN_PLATFORM == FUN_PLATFORM_QNX) || (FUN_PLATFORM == FUN_PLATFORM_VXWORKS)
  #define FUN_HAVE_SALEN 1
#else
  #define FUN_HAVE_SALEN 0
#endif

#if FUN_PLATFORM != FUN_PLATFORM_VXWORKS && !defined(FUN_NET_NO_ADDRINFO)
  #define FUN_PLATFORM_HAVE_ADDRINFO 1
#endif

#if (FUN_PLATFORM == FUN_PLATFORM_HPUX) || (FUN_PLATFORM == FUN_PLATFORM_SOLARIS) || (FUN_PLATFORM == FUN_PLATFORM_WINDOWS_CE) || (FUN_PLATFORM == FUN_PLATFORM_CYGWIN)
  #define FUN_BROKEN_TIMEOUTS 1
#else
  #define FUN_BROKEN_TIMEOUTS 0
#endif

#define FUN_PLATFORM_HAVE_IPv6  1
#define FUN_SOCKETADDRESS_PREFER_IPv4  1 // IPv4, IPv6 주소 여러개중에 IPv4 주소를 선호할지 여부.

#if FUN_PLATFORM_HAVE_ADDRINFO
  #ifndef AI_PASSIVE
    #define AI_PASSIVE 0
  #endif
  #ifndef AI_CANONNAME
    #define AI_CANONNAME 0
  #endif
  #ifndef AI_NUMERICHOST
    #define AI_NUMERICHOST 0
  #endif
  #ifndef AI_NUMERICSERV
    #define AI_NUMERICSERV 0
  #endif
  #ifndef AI_ALL
    #define AI_ALL 0
  #endif
  #ifndef AI_ADDRCONFIG
    #define AI_ADDRCONFIG 0
  #endif
  #ifndef AI_V4MAPPED
    #define AI_V4MAPPED 0
  #endif
#endif

#if FUN_HAVE_SALEN
  #define fun_set_sa_len(sa, len)  (sa)->sa_len = (len)
  #define fun_set_sin_len(sa)  (sa)->sin_len = sizeof(struct sockaddr_in)
  #if FUN_PLATFORM_HAVE_IPv6
    #define fun_set_sin6_len(sa)  (sa)->sin6_len = sizeof(struct sockaddr_in6)
  #endif
  #if FUN_PLATFORM_UNIX_FAMILY
    #define fun_set_sun_len(sa, len)  (sa)->sun_len = (len)
  #endif
#else
  #define fun_set_sa_len(sa, len)   (void)0
  #define fun_set_sin_len(sa)       (void)0
  #define fun_set_sin6_len(sa)      (void)0
  #define fun_set_sun_len(sa, len)  (void)0
#endif

#ifndef INADDR_NONE
#define INADDR_NONE  0xFFFFFFFF
#endif

#ifndef INADDR_ANY
#define INADDR_ANY  0x00000000
#endif

#ifndef INADDR_BROADCAST
#define INADDR_BROADCAST  0xFFFFFFFF
#endif

#ifndef INADDR_LOOPBACK
#define INADDR_LOOPBACK  0x7F000001
#endif

#ifndef INADDR_UNSPEC_GROUP
#define INADDR_UNSPEC_GROUP  0xE0000000
#endif

#ifndef INADDR_ALLHOSTS_GROUP
#define INADDR_ALLHOSTS_GROUP  0xE0000001
#endif

#ifndef INADDR_ALLRTRS_GROUP
#define INADDR_ALLRTRS_GROUP  0xE0000002
#endif

#ifndef INADDR_MAX_LOCAL_GROUP
#define INADDR_MAX_LOCAL_GROUP  0xE00000FF
#endif

#if !defined(s6_addr16)
  #if FUN_PLATFORM_WINDOWS_FAMILY
    #define s6_addr16  u.Word
  #else
    #define s6_addr16  __u6_addr.__u6_addr16
  #endif
#endif

#if !defined(s6_addr32)
  #if FUN_PLATFORM_UNIX_FAMILY
    #if FUN_PLATFORM_SOLARIS
      #define s6_addr32  _S6_un._S6_u32
    #else
      #define s6_addr32  __u6_addr.__u6_addr32
    #endif
  #endif
#endif


enum class AddressFamily {
  /**
   * IPv4 address family.
   */
  IPv4,

  /**
   * IPv6 address family.
   */
  IPv6,

#if FUN_PLATFORM_UNIX_FAMILY
  /**
   * UNIX domain socket address family. Available on UNIX/POSIX platforms only.
   */
  UNIX_LOCAL,
#endif
};

//TODO 제거하거나 다른 방법으로 하자.
FUN_ALWAYS_INLINE TextStream& operator << (TextStream& stream, AddressFamily value) {
  switch (value) {
    case AddressFamily::IPv4: stream << "IPv4"; break;
    case AddressFamily::IPv6: stream << "IPv6"; break;
    default: stream << "<undefined>";
  }
  return stream;
}

} // namespace net
} // namespace fun
