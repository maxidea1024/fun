#include "fun/net/dns.h"

namespace fun {
namespace net {

// warning
// Windows�� ��� WSAStartup() �Լ��� �̸� ȣ��Ǿ�� �Ʒ��� �Լ����� �������Ѵ�.

HostEntry Dns::HostByName(const String& host_name, uint32 int_flags) {
  //@maxidea: todo: thread-safe issue?

#if FUN_PLATFORM_HAVE_ADDRINFO
  struct addrinfo* ai;
  struct addrinfo hints;
  UnsafeMemory::Memzero(&hints, sizeof(hints));
  hints.ai_flags = int_flags;
  const int rc = getaddrinfo(host_name.c_str(), nullptr, &hints, &ai);
  if (rc == 0) {
    HostEntry result(ai);
    freeaddrinfo(ai);
    return result;
  } else {
    AddrInfoError(rc, host_name);
  }
#elif FUN_PLATFORM_VXWORKS
  int addr = HostGetByName(const_cast<char*>(host_name.c_str()));
  if (addr != ERROR) {
    return HostEntry(host_name, IpAddress(&addr, sizeof(addr)));
  }
#else
  if (struct hostent* he = gethostbyname(host_name.c_str()) {
    return HostEntry(he);
  }
#endif

  HandleError(LastError(), host_name); // will throw an appropriate exception
  return HostEntry(); // relax compiler.
}

HostEntry Dns::HostByAddress(const IpAddress& address, uint32 int_flags) {
#if FUN_PLATFORM_HAVE_ADDRINFO
  InetAddress sa(address, 0);

  sockaddr_in6 native_addr;
  sa.ToNative(native_addr);

  char host_name[1024];
  int rc = getnameinfo((const sockaddr*)&native_addr,
                        sizeof(native_addr),
                        host_name,
                        sizeof(host_name),
                        nullptr,
                        0,
                        NI_NAMEREQD);
  if (rc == 0) {
    struct addrinfo* ai;
    struct addrinfo hints;
    UnsafeMemory::Memset(&hints, 0x00, sizeof(hints));
    hints.ai_flags = int_flags;
    rc = getaddrinfo(host_name, nullptr, &hints, &ai);
    if (rc == 0) {
      HostEntry result(ai);
      freeaddrinfo(ai);
      return result;
    } else {
      AddrInfoError(rc, address.ToString());
    }
  } else {
    AddrInfoError(rc, address.ToString());
  }
#else
  //TODO
  //af�� �����Ϸ���? ipv4 mapped�δ� �ָ��ѵ�??
  if (struct hostent* he = gethostbyaddr(reinterpret_cast<const char*>(address.addr()),
                                        address.Length(),
                                        address.af())) {
    return HostEntry(he);
  }
#endif

  const int32 err = LastError();
  HandleError(err, address.ToString()); // will throw an appropriate exception
  return HostEntry(); // relax compiler.
}

HostEntry Dns::Resolve(const String& address) {
  IpAddress ip;
  if (IpAddress::TryParse(address, ip)) {
    return HostByAddress(ip);
  } else {
    return HostByName(address);
  }
}

IpAddress Dns::ResolveOne(const String& address) {
  const HostEntry& entry = Resolve(address);
  if (!entry.Addresses().IsEmpty()) {
    return entry.Addresses()[0];
  } else {
    throw NoAddressFoundException(address);
  }
}

const HostEntry& Dns::ThisHost() {
  static HostEntry cached_this_host_entry = HostByName(host_name());
  return cached_this_host_entry;
}

const String& Dns::HostName() {
  static String cached_host_name;

  if (cached_host_name.Len() == 0) {
    char buffer[256];
    const int rc = gethostname(buffer, sizeof(buffer));
    if (rc == 0) {
      cached_host_name = String(buffer);
    } else {
#if FUN_PLATFORM_WINDOWS_FAMILY
      int err = WSAGetLastError();
      throw NetException(String::Format("cannot get host name(error=%d)", err));
#else
      throw NetException(StringLiteral("cannot get host name"));
#endif
    }
  }

  return cached_host_name;
}

int Dns::LastError() {
#if FUN_PLATFORM_WINDOWS_FAMILY
  return GetLastError();
#elif FUN_PLATFORM_VXWORKS
  return errno;
#else
  return h_errno;
#endif
}

void Dns::HandleError(int code, const String& arg) {
  switch (code) {
    case FUN_ESYSNOTREADY:
      throw NetException(StringLiteral("net subsystem not ready"));
    case FUN_ENOTINIT:
      throw NetException(StringLiteral("net subsystem not initialized"));
    case FUN_HOST_NOT_FOUND:
      throw HostNotFoundException(arg);
    case FUN_TRY_AGAIN:
      throw DnsException(StringLiteral("temporary DNS error while resolving"), arg);
    case FUN_NO_RECOVERY:
      throw DnsException(StringLiteral("non recoverable DNS error while resolving"), arg);
    case FUN_NO_DATA:
      throw NoAddressFoundException(arg);
    default:
      throw IoException(String::FromNumber(code));
  }
}

void Dns::AddrInfoError(int code, const String& args) {
#if FUN_PLATFORM_HAVE_IPv6 || FUN_PLATFORM_HAVE_ADDRINFO
  switch (code) {
    case EAI_AGAIN:
      throw DnsException(StringLiteral("Temporary DNS error while resolving"), args);

    case EAI_FAIL:
      throw DnsException(StringLiteral("Non recoverable DNS error while resolving"), args);

  #if !FUN_PLATFORM_WINDOWS_FAMILY // EAI_NODATA and EAI_NONAME have the same value
  #if defined(EAI_NODATA) // deprecated in favor of EAI_NONAME on FreeBSD
    case EAI_NODATA:
      throw NoAddressFoundException(args);
  #endif
  #endif

    case EAI_NONAME:
      throw HostNotFoundException(args);

  #if defined(EAI_SYSTEM)
    case EAI_SYSTEM:
      HandleError(LastError(), args);
      break;
  #endif

  #if FUN_PLATFORM_WINDOWS_FAMILY
    case WSANO_DATA: // may happen on XP
      throw HostNotFoundException(args);
  #endif

    default:
      throw DnsException(StringLiteral("EAI"), String::FromNumber(code));
  }
#endif // FUN_PLATFORM_HAVE_IPv6 || FUN_PLATFORM_HAVE_ADDRINFO
}

} // namespace net
} // namespace fun
