#pragma once

#include "fun/base/container/array.h"
#include "fun/net/ip_address.h"
#include "fun/net/net.h"
#include "fun/net/socket_defs.h"

namespace fun {
namespace net {

/**
 * This class stores information about a host such as host name, alias names
 * and a list of IP addresses.
 */
class FUN_NET_API HostEntry {
 public:
  typedef Array<String> AliasList;
  typedef Array<IpAddress> AddressList;

  /**
   * Creates an empty HostEntry.
   */
  HostEntry();

  /**
   * Creates the HostEntry from the data in a hostent structure.
   */
  HostEntry(struct hostent* entry);

  /**
   * Creates the HostEntry from the data in an addrinfo structure.
   */
  HostEntry(struct addrinfo* addr_info);

#if FUN_PLATFORM_VXWORKS
  HostEntry(const String& name, const IpAddress& addr);
#endif

  /**
   * Creates the HostEntry by copying another one.
   */
  HostEntry(const HostEntry& rhs);

  /**
   * Assigns another HostEntry.
   */
  HostEntry& operator=(const HostEntry& rhs);

  /**
   * Swaps the HostEntry with another one.
   */
  void Swap(HostEntry& other);

  /**
   * Destroys the HostEntry.
   */
  ~HostEntry();

  /**
   * Returns the canonical host name.
   */
  const String& GetName() const;

  /**
   * Returns a vector containing alias names for the host name.
   */
  const AliasList& GetAliases() const;

  /**
   * Returns a vector containing the IPAddresses for the host.
   */
  const AddressList& GetAddresses() const;

 private:
  String host_name_;
  AliasList host_aliases_;
  AddressList host_addresses_;
};

//
// inlines
//

FUN_ALWAYS_INLINE const String& HostEntry::GetName() const {
  return host_name_;
}

FUN_ALWAYS_INLINE const HostEntry::AliasList& HostEntry::GetAliases() const {
  return host_aliases_;
}

FUN_ALWAYS_INLINE const HostEntry::AddressList& HostEntry::GetAddresses()
    const {
  return host_addresses_;
}

}  // namespace net
}  // namespace fun

// TODO 이걸 안해도 되도록 하자.
namespace fun {

FUN_ALWAYS_INLINE void Swap(net::HostEntry& x, net::HostEntry& y) { x.Swap(y); }

}  // namespace fun
