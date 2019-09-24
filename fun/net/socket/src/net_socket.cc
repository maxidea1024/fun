#include "CorePrivatePCH.h"
#include "Net/socket/NetSocket.h"

namespace fun {
namespace net {

//TODO XBOX에서도 소켓 서브시스템 초기화를 해야함.
//TODO IpAddress/CIPEndPoint를 static으로 잡을 경우, 호출순서에 문제가 있음. 이를 동적으로 해결해야함.

#if FUN_PLATFORM_WINDOWS_FAMILY
class StaticTimeWinsockInitializer {
 public:
  StaticTimeWinsockInitializer() {
    WORD version = MAKEWORD(2, 2);
    WSADATA data;
    if (WSAStartup(version, &data) != 0) {
      fprintf(stderr, "WSAStartup() failure\n");
      exit(-1);
    }
  }

  ~StaticTimeWinsockInitializer() {
    WSACleanup();
  }
};

static StaticTimeWinsockInitializer static_ws_initializer;

#endif //FUN_PLATFORM_WINDOWS_FAMILY

} // namespace net
} // namespace fun
