//TODO 코드 정리
//CClientConnectArgs로 변경하도록 하자.

#pragma once

#include "fun/net/net.h"

namespace fun {
namespace net {

class ThreadPool2;

class NetConnectionArgs {
 public:
  String server_ip;
  int32 server_port;
  Array<int32> local_udp_port_pool;
  Uuid protocol_version;
  ByteArray user_data;
  //TODO 제거하던지 이름을 바꾸던지 하자.
  double TunedNetworkerSendInterval_TEST;

  FUN_NETX_API NetConnectionArgs();

  FUN_NETX_API void LoadFromINI(const String& filename, const String& section_name);
  FUN_NETX_API void SaveToINI(const String& filename, const String& section_name);
};


enum class ServerUdpAssignMode {
  None = 0,
  PerClient = 1,
  Static = 2,
};


//TODO 이건 실상 필요가 없을듯 싶다.
enum class HostIdGenerationPolicy {
  NoRecycle = 0,
  Recycle = 1,
  Assign = 2,
};


class P2PGroupOption {
 public:
  bool direct_p2p_enabled;

  FUN_NETX_API P2PGroupOption();

  static FUN_NETX_API P2PGroupOption Default;
};


class StartServerArgs {
 public:
  /**
   * 클라이언트가 인식할 서버주소.
   *
   * IP 또는 hostname을 넣어주면 되는데, 평상시에는 비워두어도 됨.
   * 다만, L4 장비 뒤에 놓인 서버일 경우에 주소인식을 제대로 못할 수 있으므로, 직접 지정해주어야함.
   */
  String server_addr_at_client;

  /**
   * 서버의 리스닝 바인딩을 위한 주소.
   *
   * 비워두어도 된다. 다만, NIC가 두개 이상일 경우에는 첫번째 주소로 바인딩 되는데, 두번째 NIC의 주소를 사용해야한다던지
   * 하는 경우에는 직접 지정해야 원하는 NIC로 접속을 받아낼 수 있다.
   */
  String local_nic_addr;


  //@maxidea: deprecated.
  //int32 tcp_port;

  //@maxidea: *NEW*
  /**
   * Listening socket을 여러개 만들 수 있다.
   * 목록을 채우지 않으면 0이 하나 들어가게 된다. 0을 지정하면 TCP 리스닝 포트 한개가
   * 자동 할당 된다.  자동 할당이 유용한 경우는, 클라이언트가 서버에 접속하기 전에 자동
   * 할당된 포트 번호를 받은 경우, 예를들어 스트레스 테스트를 위해 불특정 다수의 서버를
   * 띄워야 하는 경우가 되겠다.
   * 이때 클라이언트에게 자동할당된 포트 주소를 알려주어야 한다.
   * 자동 할당된 포트 번호는 GetServerAddress()로 얻을 수 있다.
   */
  Array<int32> tcp_ports; //*NEW*
  Array<int32> failed_bind_tcp_ports; //*NEW*

  Uuid protocol_version;

  Array<int32> udp_ports;
  ServerUdpAssignMode udp_assign_mode;

  int32 thread_count;
  int32 network_thread_count;

  int32 strong_encrypted_message_key_length;
  int32 weak_encrypted_message_key_length;
  bool p2p_encrypted_messaging_enabled;

  bool server_as_p2p_group_member_allowed;

  //TODO 제거하도록 하자.
  bool bEnableIocp;

  bool upnp_detect_nat_device;
  bool upnp_tcp_addr_port_mapping;

  bool using_over_block_icmp_environment;

  uint32 timer_callback_interval;
  void* timer_callback_context;

  bool bEnableNagleAlgorithm;

  HostIdGenerationPolicy host_id_generation_policy;

  int32 client_emergency_log_max_line_count;

  HostId pre_created_p2p_group_start_host_id;
  int32 pre_create_p2p_group_count;
  P2PGroupOption pre_create_p2p_group_option;

  bool ping_test_enabled;

  /**
   * True이면, UdpPorts에 이미 사용중인 Port가 있더라도, 실패하지 않고 건너띄면서 다음 포트를 Bind함.
   * 즉, 개별 Udp Port binding에 실패 했다고 해도 나머지 포트들에 대해서 binding을 지속한다.
   */
  bool ignore_failed_bind_port;
  Array<int32> failed_bind_ports;

  ThreadPool2* external_user_worker_thread_pool;
  ThreadPool2* external_net_worker_thread_pool;

  /**
   * 송신주기를 임의로 변경하고 싶을 경우 사용한다. 기본은 0.03초로 설정되어 있는데,
   * 이 주기를 변경하고 싶을 경우에 아래의 값을 변경하도록 한다.
   */
  //TODO 제거하던지 이름을 변경하도록 하자.
  double TunedNetworkerSendInterval_TEST;

 public:
  FUN_NETX_API StartServerArgs();

  FUN_NETX_API void LoadFromINI(const String& filename, const String& section_name);
  FUN_NETX_API void SaveToINI(const String& filename, const String& section_name);
};


class LanConnectionArgs {
 public:
  String server_ip;
  int32 server_port;

  Uuid protocol_version;

  ByteArray user_data;

  String local_nic_addr;
  int32 p2p_listening_port;

  int32 thread_count;
  int32 network_thread_count;

  uint32 timer_callback_interval;
  void* timer_callback_context;

  ThreadPool2* external_userworker_thread_pool;
  ThreadPool2* external_networker_thread_pool;

  FUN_NETX_API LanConnectionArgs();

  FUN_NETX_API void LoadFromINI(const String& filename, const String& section_name);
  FUN_NETX_API void SaveToINI(const String& filename, const String& section_name);
};


class LanStartServerArgs {
 public:
  String server_addr_at_client;
  String local_nic_addr;
  int32 tcp_port;

  Uuid protocol_version;

  int32 thread_count;
  int32 network_thread_count;

  int32 strong_encrypted_message_key_length;
  int32 weak_encrypted_message_key_length;
  bool p2p_encrypted_messaging_enabled;

  bool server_as_p2p_group_member_allowed;

  bool look_ahead_p2p_send_enabled;

  uint32 timer_callback_interval;
  void* timer_callback_context;

  bool bEnableNagleAlgorithm;

  HostIdGenerationPolicy host_id_generation_policy;

  ThreadPool2* external_user_worker_thread_pool;
  ThreadPool2* external_net_worker_thread_pool;

  FUN_NETX_API LanStartServerArgs();

  FUN_NETX_API void LoadFromINI(const String& filename, const String& section_name);
  FUN_NETX_API void SaveToINI(const String& filename, const String& section_name);
};

} // namespace net
} // namespace fun
