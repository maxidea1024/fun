#pragma once

namespace fun {
namespace net {

/**
 * EmergencyLogServer로 보낼 data
 */
class EmergencyLogData
{
 public:
  DateTime logon_time; // 로그를 기록한 시간

  // netClientstat에서 얻은값
  uint64 total_tcp_recv_bytes;
  uint64 total_tcp_send_bytes;
  uint64 total_udp_send_count;
  uint64 total_udp_send_bytes;
  uint64 total_udp_recv_count;
  uint64 total_udp_recv_bytes;

  HostId host_id;
  uint64 total_tcp_issued_send_bytes_; // tcp Issue를 건 총 Send Byte양
  int32 io_pending_count;
  int32 connect_count; // NetClient 객체가 있는동안 NetServer와 연결한 횟수
  int32 remote_peer_count; // 연결된 remotePeer 갯수
  int32 direct_p2p_enable_peer_count; // direct로 연결된 remotePeer 갯수
  String nat_device_name; // NAT 장치 이름
  int32 msg_size_error_count; // WSAEMSGSIZE 에러난 횟수
  int32 net_reset_error_count; // WSAENETRESET(10052) 에러난 횟수
  int32 conn_reset_error_count; // WSAECONNRESET(10054) 에러난 횟수

  // os version을 알아내기 위한 변수들
  int32 os_major_version;
  int32 os_minor_version;
  int32 product_type;
  int32 processor_architecture;

  int32 server_udp_addr_count; // 최근의 server udp addr 갯수
  int32 remote_udp_addr_count; // 최근의 remote udp addr 갯수

  int32 last_error_completion_length; // completion data length < 0 일때 GetLastError값

  struct Log {
    DateTime added_time;
    LogCategory category;
    String text;
  };
  typedef List<Log> EmergencyLogList;
  EmergencyLogList log_hist;

  String server_ip;
  int32 server_port;

public:
  EmergencyLogData();

  void CopyTo(EmergencyLogData& to) const;
};


//
// inlines
//

inline EmergencyLogData::EmergencyLogData()
{
  total_tcp_recv_bytes = 0;
  total_tcp_send_bytes = 0;
  total_udp_send_count = 0;
  total_udp_send_bytes = 0;
  total_udp_recv_count = 0;
  total_udp_recv_bytes = 0;
  connect_count = 0;
  remote_peer_count = 0;
  direct_p2p_enable_peer_count = 0;
  msg_size_error_count = 0;
  net_reset_error_count = 0;
  conn_reset_error_count = 0;
  os_major_version = 0;
  os_minor_version = 0;
  product_type = 0;
  processor_architecture = 0;

  server_udp_addr_count = 0;
  remote_udp_addr_count = 0;
  last_error_completion_length = 0;
}

inline void EmergencyLogData::CopyTo(EmergencyLogData& to) const
{
  to.logon_time = logon_time;

  to.connect_count = connect_count;

  to.direct_p2p_enable_peer_count = direct_p2p_enable_peer_count;

  to.conn_reset_error_count = conn_reset_error_count;
  to.last_error_completion_length = last_error_completion_length;
  to.msg_size_error_count = msg_size_error_count;

  to.nat_device_name = nat_device_name;

  to.net_reset_error_count = net_reset_error_count;

  to.os_major_version = os_major_version;
  to.os_minor_version = os_minor_version;

  to.processor_architecture = processor_architecture;

  to.product_type = product_type;

  to.remote_peer_count = remote_peer_count;

  to.remote_udp_addr_count = remote_udp_addr_count;
  to.server_udp_addr_count = server_udp_addr_count;

  to.total_tcp_recv_bytes = total_tcp_recv_bytes;
  to.total_tcp_send_bytes = total_tcp_send_bytes;

  to.total_udp_recv_bytes = total_udp_recv_bytes;
  to.total_udp_recv_count = total_udp_recv_count;
  to.total_udp_send_bytes = total_udp_send_bytes;
  to.total_udp_send_count = total_udp_send_count;

  to.log_list.Clear();
  for (const auto& log : log_list) {
    to.log_list.Add(log);
  }
}

} // namespace net
} // namespace fun
