#include "fun/net/host_entry.h"

#include <algorithm>  //std::stable_sort

namespace fun {
namespace net {

//#undef FUN_SOCKETADDRESS_PREFER_IPv4
//#define FUN_SOCKETADDRESS_PREFER_IPv4  0

namespace {

struct AFLT {
  bool operator()(const IpAddress& a1, const IpAddress& a2) {
    // return a1.af() < a2.af();
    const int32 v1 = (int32)a1.IsIPv4MappedToIPv6();
    const int32 v2 = (int32)a2.IsIPv4MappedToIPv6();
    return v1 < v2;
  }
};

}  // namespace

HostEntry::HostEntry() : host_name_(), host_aliases_(), host_addresses_() {}

HostEntry::HostEntry(struct hostent* entry) {
  host_name = entry->h_name;

  if (char** alias = entry->h_aliases) {
    while (*alias) {
      host_aliases_.Add(*alias);
      ++alias;
    }
  }

  if (char** address = entry->h_addr_list) {
    while (*address) {
      // host_addresses_.Add(IpAddress(*address, entry->h_length));
      host_addresses_.Add(IpAddress(*address));
      ++address;
    }

    if (host_addresses_.Count() > 1) {
// TODO 내부 sort 함수로 대체하자.
#if FUN_SOCKETADDRESS_PREFER_IPv4
      // if we get both IPv4 and IPv6 addresses, prefer IPv4
      IpAddress* ptr = &host_addresses_[0];
      IpAddress* end = ptr + host_addresses_.Count();
      std::stable_sort(ptr, end, AFLT());
#endif
    }
  }
}

HostEntry::HostEntry(struct addrinfo* addr_info) {
  for (struct addrinfo* it = addr_info; it; it = it->ai_next) {
    if (it->ai_canonname) {
      host_name = it->ai_canonname;
    }

    if (it->ai_addrlen > 0 && it->ai_addr) {
      switch (it->ai_addr->sa_family) {
        case AF_INET:
          // host_addresses_.Add(IpAddress(&reinterpret_cast<struct
          // sockaddr_in*>(it->ai_addr)->sin_addr, sizeof(in_addr)));
          host_addresses_.Add(IpAddress(
              reinterpret_cast<struct sockaddr_in*>(it->ai_addr)->sin_addr));
          break;

        case AF_INET6:
          // host_addresses_.Add(IpAddress(&reinterpret_cast<struct
          // sockaddr_in6*>(it->ai_addr)->sin6_addr, sizeof(in6_addr),
          // reinterpret_cast<struct
          // sockaddr_in6*>(it->ai_addr)->sin6_scope_id));
          host_addresses_.Add(IpAddress(
              reinterpret_cast<struct sockaddr_in6*>(it->ai_addr)->sin6_addr,
              reinterpret_cast<struct sockaddr_in6*>(it->ai_addr)
                  ->sin6_scope_id));
          break;

        default:
          break;
      }
    }
  }

  if (host_addresses_.Count() > 1) {
// TODO 내부 sort 함수로 대체하자.
#if FUN_SOCKETADDRESS_PREFER_IPv4
    // if we get both IPv4 and IPv6 addresses, prefer IPv4
    IpAddress* ptr = &host_addresses_[0];
    IpAddress* end = ptr + host_addresses_.Count();
    std::stable_sort(ptr, end, AFLT());
#endif
  }
}

#if FUN_PLATFORM_VXWORKS
HostEntry::HostEntry(const String& name, const IpAddress& addr)
    : host_name(name) {
  host_addresses_.Add(addr);
}
#endif

HostEntry::HostEntry(const HostEntry& rhs)
    : host_name(rhs.host_name),
      host_aliases_(rhs.host_aliases_),
      host_addresses_(rhs.host_addresses_) {}

HostEntry& HostEntry::operator=(const HostEntry& rhs) {
  if (FUN_LIKELY(&rhs != this)) {
    host_name = rhs.host_name;
    host_aliases_ = rhs.host_aliases_;
    host_addresses_ = rhs.host_addresses_;
  }

  return *this;
}

void HostEntry::Swap(HostEntry& other) {
  fun::Swap(host_name, other.host_name);
  fun::Swap(host_aliases_, other.host_aliases_);
  fun::Swap(host_addresses_, other.host_addresses_);
}

HostEntry::~HostEntry() {}

}  // namespace net
}  // namespace fun
