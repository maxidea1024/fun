#pragma once

#include "fun/net/host_entry.h"
#include "fun/net/ip_address.h"
#include "fun/net/net.h"
#include "fun/net/socket_defs.h"

namespace fun {
namespace net {

/**
 * This class provides an interface to the domain name service.
 *
 * An internal DNS cache is used to speed up name lookups.
 */
class FUN_NET_API Dns {
 public:
  enum HintFlag {
    DNS_HINT_NONE = 0,
#if FUN_PLATFORM_HAVE_ADDRINFO
    /** Socket address will be used in bind() call */
    DNS_HINT_AI_PASSIVE = AI_PASSIVE,
    /** Return canonical name in first ai_canonname */
    DNS_HINT_AI_CANONNAME = AI_CANONNAME,
    /** Nodename must be a numeric address string */
    DNS_HINT_AI_NUMERICHOST = AI_NUMERICHOST,
    /** Servicename must be a numeric port number */
    DNS_HINT_AI_NUMERICSERV = AI_NUMERICSERV,
    /** Query both IP6 and IP4 with AI_V4MAPPED */
    DNS_HINT_AI_ALL = AI_ALL,
    /** Resolution only if global address configured */
    DNS_HINT_AI_ADDRCONFIG = AI_ADDRCONFIG,
    /** On v6 failure, query v4 and convert to V4MAPPED format */
    DNS_HINT_AI_V4MAPPED = AI_V4MAPPED
#endif
  };

  /**
   * Returns a HostEntry object containing the DNS information
   * for the host with the given name. HintFlag argument is only
   * used on platforms that have getaddrinfo().
   *
   * Throws a HostNotFoundException if a host with the given
   * name cannot be found.
   *
   * Throws a NoAddressFoundException if no address can be
   * found for the hostname.
   *
   * Throws a DnsException in case of a general DNS error.
   *
   * Throws an IoException in case of any other error.
   */
  static HostEntry HostByName(const String& HotName,
                              uint32 int_flags =
#ifdef FUN_PLATFORM_HAVE_ADDRINFO
                                  DNS_HINT_AI_CANONNAME | DNS_HINT_AI_ADDRCONFIG
#else
                                  DNS_HINT_NONE
#endif
  );

  /**
   * Returns a HostEntry object containing the DNS information
   * for the host with the given IP address. HintFlag argument is only
   * used on platforms that have getaddrinfo().
   *
   * Throws a HostNotFoundException if a host with the given
   * name cannot be found.
   *
   * Throws a DnsException in case of a general DNS error.
   *
   * Throws an IoException in case of any other error.
   */
  static HostEntry HostByAddress(const IpAddress& address,
                                 uint32 int_flags =
#ifdef FUN_PLATFORM_HAVE_ADDRINFO
                                     DNS_HINT_AI_CANONNAME |
                                     DNS_HINT_AI_ADDRCONFIG
#else
                                     DNS_HINT_NONE
#endif
  );

  /**
   * Returns a HostEntry object containing the DNS information
   * for the host with the given IP address or host name.
   *
   * Throws a HostNotFoundException if a host with the given
   * name cannot be found.
   *
   * Throws a NoAddressFoundException if no address can be
   * found for the hostname.
   *
   * Throws a DnsException in case of a general DNS error.
   *
   * Throws an IoException in case of any other error.
   */
  static HostEntry Resolve(const String& address);

  /**
   * Convenience method that calls Resolve(address) and returns
   * the first address from the HostInfo.
   */
  static IpAddress ResolveOne(const String& address);

  /**
   * Returns a HostEntry object containing the DNS information
   * for this host.
   *
   * Throws a HostNotFoundException if DNS information
   * for this host cannot be found.
   *
   * Throws a NoAddressFoundException if no address can be
   * found for this host.
   *
   * Throws a DnsException in case of a general DNS error.
   *
   * Throws an IoException in case of any other error.
   */
  static const HostEntry& ThisHost();

  /**
   * Returns the host name of this host.
   */
  static const String& HostName();

 protected:
  /**
   * Returns the code of the last error.
   */
  static int32 LastError();

  /**
   * Throws an exception according to the error code.
   */
  static void HandleError(int32 code, const String& arg);

  /**
   * Throws an exception according to the getaddrinfo() error code.
   */
  static void AddrInfoError(int32 code, const String& arg);
};

}  // namespace net
}  // namespace fun
