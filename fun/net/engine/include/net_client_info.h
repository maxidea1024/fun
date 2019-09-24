#pragma once

#include "fun/net/net.h"

namespace fun {
namespace net {

typedef SharedPtr<class NetClientInfo> NetClientInfoPtr;

/**
 * 접속한 클라이언트의 정보를 담은 구조체입니다.
 */
class NetClientInfo {
 public:
  /**
   * 서버에서 인식한 클라이언트의 TCP socket 주소입니다.
   * 즉, 공인 인터넷 주소(public ip) 입니다.
   * 보안을 위해서, 타 클라이언트는 조회할 수 없습니다.
   * 서버에서만 조회할 수 있습니다.
   */
  InetAddress tcp_addr_from_server;

  /**
   * 서버에서 인식한 클라이언트의 UDP socket 주소입니다.
   * 즉, 홀펀칭이 완료된 후에 얻어진 주소입니다.
   * 만약, 홀펀칭이 이루어지지 않을 경우 이 값은 InetAddress::None입니다.
   */
  InetAddress udp_addr_from_server;

  /**
   * 클라이언트의 내부 UDP socket 주소입니다.
   */
  InetAddress udp_addr_internal;

  /**
   * 서버에서 할당된 클라이언트의 ID 입니다.
   */
  HostId host_id;

  /**
   * 릴레이를 통한 P2P중인지 여부를 나타냅니다.
   */
  bool relayed_p2p;

  /**
   * 클라이언트가 참여한 P2P 그룹목록입니다.
   */
  HostIdSet joined_p2p_groups;

  /**
   * 클라이언트가 NAT(공유기) 뒤에 위치하고 있는지 여부를 나타냅니다.
   */
  bool behind_nat;

  /**
   * 클라이언트가 서버와 UDP 통신이 이루어지고 있는지 여부를 나타냅니다.
   */
  bool real_udp_enabled;

  /**
   * 클라이언트에서 사용중인 NAT(공유기) 장치 이름입니다.
   * 장치를 인식하기 전까지는 빈문자열이며, 인식에 실패했을 경우에는
   * 계속 빈문자열입니다.
   */
  String nat_device_name;

  /**
   * 최근에 측정된 초단위 평균 Ping 타임입니다.
   */
  double recent_ping;

  /**
   * 이 클라이언트로 전송대기중인 데이터 총량(바이트단위)입니다.
   * 단, 릴레이되는 메시지는 포함하지 않습니다.
   */
  int32 send_queued_amount_in_byte;

  /**
   * 클라이언트에 지정한 태그 주소입니다.
   * 이 값은 서버내부에서만 사용되며, 네트워크 너머로 동기화되지 않습니다.
   */
  void* host_tag;

  /**
   * 클라이언트의 최근 frame rate 값입니다.
   */
  double recent_frame_rate;

  /**
   * 클라이언트가 서버에게 UDP packet 전송을 시도한 총 횟수입니다.
   */
  int32 to_server_send_udp_message_attempt_count;

  /**
   * 클라이언트가 서버에게 UDP packet 전송이 성공한 총 횟수입니다.
   */
  int32 to_server_send_udp_message_success_count;

  /**
   * 클라이언트가 현재 사용하고 있는 HostId의 재사용 횟수입니다.
   * (구지 필요 없어보임..)
   */
  int32 host_id_recycle_count;

  FUN_NETX_API NetClientInfo();

  FUN_NETX_API String ToString(bool at_server) const;
};

} // namespace net
} // namespace fun
