#include "fun/net/net.h"
#include "Misc/ConfigCache.h"

namespace fun {

//
// NetConnectionArgs
//

NetConnectionArgs::NetConnectionArgs() {
  TunedNetworkerSendInterval_TEST = 0;
  server_port = 0;
}

void NetConnectionArgs::LoadFromINI(const String& filename, const String& section_name) {
  //g_config->GetValue(Section, TEXT("TunedNetworkerSendInterval_TEST"), TunedNetworkerSendInterval_TEST, filename);
  //g_config->GetValue(Section, TEXT("server_port"), server_port, filename);
  //g_config->GetValue(Section, TEXT("UseTimerType"), UseTimerType, filename);
}

void NetConnectionArgs::SaveToINI(const String& filename, const String& section_name) {
  //g_config->SetValue(Section, TEXT("TunedNetworkerSendInterval_TEST"), TunedNetworkerSendInterval_TEST, filename);
  //g_config->SetValue(Section, TEXT("server_port"), server_port, filename);
  //g_config->SetValue(Section, TEXT("UseTimerType"), UseTimerType, filename);
  //g_config->Flush(false, filename);
}


//
// LanConnectionArgs
//

LanConnectionArgs::LanConnectionArgs() {
  server_port = 0;
  thread_count = 0;
  network_thread_count = 0;
  timer_callback_interval = 0;
  timer_callback_context = nullptr;

  // 비워 두면 INADDR_ANY가 되어서 자동으로 할당 받음.
  // 다만, 명시적으로 지정해야 정상 동작하는 경우도 있을 수 있으므로,
  // 바인딩 실패시 이부분을 확인해 보도록 하자.
  local_nic_addr_ = TEXT("");

  p2p_listening_port = 0; // Free port

  external_net_worker_thread_pool = nullptr;
  external_user_worker_thread_pool = nullptr;
}

void LanConnectionArgs::LoadFromINI(const String& filename, const String& section_name) {
  //g_config->GetValue(section_name, TEXT("server_port"), server_port, filename);
  //g_config->GetValue(section_name, TEXT("thread_count"), thread_count, filename);
  //g_config->GetValue(section_name, TEXT("network_thread_count"), network_thread_count, filename);
  //g_config->GetValue(section_name, TEXT("timer_callback_interval"), timer_callback_interval, filename);
  //g_config->GetValue(section_name, TEXT("local_nic_addr_"), local_nic_addr_, filename);
  //g_config->GetValue(section_name, TEXT("p2p_listening_port"), p2p_listening_port, filename);
  //g_config->GetValue(section_name, TEXT("UseTimerType"), UseTimerType, filename);
}

void LanConnectionArgs::SaveToINI(const String& filename, const String& section_name) {
  //g_config->SetValue(section_name, TEXT("server_port"), server_port, filename);
  //g_config->SetValue(section_name, TEXT("thread_count"), thread_count, filename);
  //g_config->SetValue(section_name, TEXT("network_thread_count"), network_thread_count, filename);
  //g_config->SetValue(section_name, TEXT("timer_callback_interval"), timer_callback_interval, filename);
  //g_config->SetValue(section_name, TEXT("local_nic_addr_"), local_nic_addr_, filename);
  //g_config->SetValue(section_name, TEXT("p2p_listening_port"), p2p_listening_port, filename);
  //g_config->SetValue(section_name, TEXT("UseTimerType"), UseTimerType, filename);
  //g_config->Flush(false, filename);
}


//
// StartServerArgs
//

StartServerArgs::StartServerArgs() {
  strong_encrypted_message_key_length = (int32)StrongEncryptionLevel::Low; // default strong encrypt level low
  weak_encrypted_message_key_length = (int32)WeakEncryptionLevel::Low; // default weak encrypt level low

  bEnableIocp = true;
  //tcp_port = 0;
  thread_count = 0;
  udp_assign_mode = ServerUdpAssignMode::PerClient;
  network_thread_count = 0;
  server_as_p2p_group_member_allowed = false;

  //p2p_encrypted_messaging_enabled = false;
  p2p_encrypted_messaging_enabled = true;

  upnp_detect_nat_device = NetConfig::upnp_detect_nat_device_by_default;
  upnp_tcp_addr_port_mapping = NetConfig::upnp_tcp_addr_port_mapping_by_default;
  using_over_block_icmp_environment = false;
  timer_callback_interval = 0;
  timer_callback_context = nullptr;
  bEnableNagleAlgorithm = false; //@maxidea: true일 경우, 경고가 뜨게 되어 있어서 일단 꺼둠.  차후에 코드 리뷰해야함.
  HostIdGenerationPolicy = HostIdGenerationPolicy::NoRecycle;
  ping_test_enabled = false;
  client_emergency_log_max_line_count = 0;
  ignore_failed_bind_port = false;
  TunedNetworkerSendInterval_TEST = 0;
  failed_bind_ports.Clear();
  pre_create_p2p_group_count = 0;
  pre_created_p2p_group_start_host_id = HostId_None;

  external_net_worker_thread_pool = nullptr;
  external_user_worker_thread_pool = nullptr;
}

void StartServerArgs::LoadFromINI(const String& filename, const String& section_name) {
  //CScopedConfigAccess ScopedConfigAccess(filename, Section);
  //g_config->GetValue(Section, TEXT("server_addr_at_client"), server_addr_at_client, filename);
  //g_config->GetValue(Section, TEXT("local_nic_addr_"), local_nic_addr_, filename);
  //g_config->GetValue(Section, TEXT("tcp_ports"), tcp_ports, filename);
  //g_config->GetValue(Section, TEXT("protocol_version"), protocol_version, filename);
  //g_config->GetValue(Section, TEXT("udp_ports"), udp_ports, filename);
  //g_config->GetValue(Section, TEXT("udp_assign_mode"), udp_assign_mode, filename);
  //g_config->GetValue(Section, TEXT("thread_count"), thread_count, filename);
  //g_config->GetValue(Section, TEXT("network_thread_count"), network_thread_count, filename);
  //g_config->GetValue(Section, TEXT("strong_encrypted_message_key_length"), strong_encrypted_message_key_length, filename);
  //g_config->GetValue(Section, TEXT("weak_encrypted_message_key_length"), weak_encrypted_message_key_length, filename);
  //g_config->GetValue(Section, TEXT("p2p_encrypted_messaging_enabled"), p2p_encrypted_messaging_enabled, filename);
  //g_config->GetValue(Section, TEXT("server_as_p2p_group_member_allowed"), server_as_p2p_group_member_allowed, filename);
  //g_config->GetValue(Section, TEXT("bEnableIocp"), bEnableIocp, filename);
  //g_config->GetValue(Section, TEXT("upnp_detect_nat_device"), upnp_detect_nat_device, filename);
  //g_config->GetValue(Section, TEXT("upnp_tcp_addr_port_mapping"), upnp_tcp_addr_port_mapping, filename);
  //g_config->GetValue(Section, TEXT("using_over_block_icmp_environment"), using_over_block_icmp_environment, filename);
  //g_config->GetValue(Section, TEXT("timer_callback_interval"), timer_callback_interval, filename);
  //g_config->GetValue(Section, TEXT("bEnableNagleAlgorithm"), bEnableNagleAlgorithm, filename);
  //g_config->GetValue(Section, TEXT("HostIdGenerationPolicy"), HostIdGenerationPolicy, filename);
  //g_config->GetValue(Section, TEXT("client_emergency_log_max_line_count"), client_emergency_log_max_line_count, filename);
  //g_config->GetValue(Section, TEXT("pre_created_p2p_group_start_host_id"), pre_created_p2p_group_start_host_id, filename);
  //g_config->GetValue(Section, TEXT("pre_create_p2p_group_count"), pre_create_p2p_group_count, filename);
  //g_config->GetValue(Section, TEXT("pre_create_p2p_group_option"), pre_create_p2p_group_option, filename);
  //g_config->GetValue(Section, TEXT("ping_test_enabled"), ping_test_enabled, filename);
  //g_config->GetValue(Section, TEXT("ignore_failed_bind_port"), ignore_failed_bind_port, filename);
  //g_config->GetValue(Section, TEXT("UseTimerType"), UseTimerType, filename);
}

void StartServerArgs::SaveToINI(const String& filename, const String& section_name) {
  //CScopedConfigAccess ScopedConfigAccess(filename, Section);
  //g_config->SetValue(Section, TEXT("server_addr_at_client"), server_addr_at_client, filename);
  //g_config->SetValue(Section, TEXT("local_nic_addr_"), local_nic_addr_, filename);
  //g_config->SetValue(Section, TEXT("tcp_ports"), tcp_ports, filename);
  //g_config->SetValue(Section, TEXT("protocol_version"), protocol_version, filename);
  //g_config->SetValue(Section, TEXT("udp_ports"), udp_ports, filename);
  //g_config->SetValue(Section, TEXT("udp_assign_mode"), udp_assign_mode, filename);
  //g_config->SetValue(Section, TEXT("thread_count"), thread_count, filename);
  //g_config->SetValue(Section, TEXT("network_thread_count"), network_thread_count, filename);
  //g_config->SetValue(Section, TEXT("strong_encrypted_message_key_length"), strong_encrypted_message_key_length, filename);
  //g_config->SetValue(Section, TEXT("weak_encrypted_message_key_length"), weak_encrypted_message_key_length, filename);
  //g_config->SetValue(Section, TEXT("p2p_encrypted_messaging_enabled"), p2p_encrypted_messaging_enabled, filename);
  //g_config->SetValue(Section, TEXT("server_as_p2p_group_member_allowed"), server_as_p2p_group_member_allowed, filename);
  //g_config->SetValue(Section, TEXT("bEnableIocp"), bEnableIocp, filename);
  //g_config->SetValue(Section, TEXT("upnp_detect_nat_device"), upnp_detect_nat_device, filename);
  //g_config->SetValue(Section, TEXT("upnp_tcp_addr_port_mapping"), upnp_tcp_addr_port_mapping, filename);
  //g_config->SetValue(Section, TEXT("using_over_block_icmp_environment"), using_over_block_icmp_environment, filename);
  //g_config->SetValue(Section, TEXT("timer_callback_interval"), timer_callback_interval, filename);
  //g_config->SetValue(Section, TEXT("bEnableNagleAlgorithm"), bEnableNagleAlgorithm, filename);
  //g_config->SetValue(Section, TEXT("HostIdGenerationPolicy"), HostIdGenerationPolicy, filename);
  //g_config->SetValue(Section, TEXT("client_emergency_log_max_line_count"), client_emergency_log_max_line_count, filename);
  //g_config->SetValue(Section, TEXT("pre_created_p2p_group_start_host_id"), pre_created_p2p_group_start_host_id, filename);
  //g_config->SetValue(Section, TEXT("pre_create_p2p_group_count"), pre_create_p2p_group_count, filename);
  //g_config->SetValue(Section, TEXT("pre_create_p2p_group_option"), pre_create_p2p_group_option, filename);
  //g_config->SetValue(Section, TEXT("ping_test_enabled"), ping_test_enabled, filename);
  //g_config->SetValue(Section, TEXT("ignore_failed_bind_port"), ignore_failed_bind_port, filename);
  //g_config->SetValue(Section, TEXT("UseTimerType"), UseTimerType, filename);
  //g_config->Flush(false, filename);
}


//
// LanStartServerArgs
//

LanStartServerArgs::LanStartServerArgs() {
  strong_encrypted_message_key_length = 128;
  weak_encrypted_message_key_length = 0;
  tcp_port = 0;
  thread_count = 0;
  network_thread_count = 0;
  server_as_p2p_group_member_allowed = false;
  p2p_encrypted_messaging_enabled = false;
  look_ahead_p2p_send_enabled = true;
  timer_callback_interval = 0;
  timer_callback_context = nullptr;
  bEnableNagleAlgorithm = false;
  HostIdGenerationPolicy = HostIdGenerationPolicy::NoRecycle;
  external_net_worker_thread_pool = nullptr;
  external_user_worker_thread_pool = nullptr;
}

void LanStartServerArgs::LoadFromINI(const String& filename, const String& section_name) {
  //g_config->GetValue(Section, TEXT("strong_encrypted_message_key_length"), strong_encrypted_message_key_length, filename);
  //g_config->GetValue(Section, TEXT("weak_encrypted_message_key_length"), weak_encrypted_message_key_length, filename);
  //g_config->GetValue(Section, TEXT("tcp_port"), tcp_port, filename);
  //g_config->GetValue(Section, TEXT("thread_count"), thread_count, filename);
  //g_config->GetValue(Section, TEXT("network_thread_count"), network_thread_count, filename);
  //g_config->GetValue(Section, TEXT("server_as_p2p_group_member_allowed"), server_as_p2p_group_member_allowed, filename);
  //g_config->GetValue(Section, TEXT("p2p_encrypted_messaging_enabled"), p2p_encrypted_messaging_enabled, filename);
  //g_config->GetValue(Section, TEXT("look_ahead_p2p_send_enabled"), look_ahead_p2p_send_enabled, filename);
  //g_config->GetValue(Section, TEXT("timer_callback_interval"), timer_callback_interval, filename);
  //g_config->GetValue(Section, TEXT("bEnableNagleAlgorithm"), bEnableNagleAlgorithm, filename);
  //g_config->GetValue(Section, TEXT("HostIdGenerationPolicy"), HostIdGenerationPolicy, filename);
  //g_config->GetValue(Section, TEXT("UseTimerType"), UseTimerType, filename);
}

void LanStartServerArgs::SaveToINI(const String& filename, const String& section_name) {
  //g_config->SetValue(Section, TEXT("strong_encrypted_message_key_length"), strong_encrypted_message_key_length, filename);
  //g_config->SetValue(Section, TEXT("weak_encrypted_message_key_length"), weak_encrypted_message_key_length, filename);
  //g_config->SetValue(Section, TEXT("tcp_port"), tcp_port, filename);
  //g_config->SetValue(Section, TEXT("thread_count"), thread_count, filename);
  //g_config->SetValue(Section, TEXT("network_thread_count"), network_thread_count, filename);
  //g_config->SetValue(Section, TEXT("server_as_p2p_group_member_allowed"), server_as_p2p_group_member_allowed, filename);
  //g_config->SetValue(Section, TEXT("p2p_encrypted_messaging_enabled"), p2p_encrypted_messaging_enabled, filename);
  //g_config->SetValue(Section, TEXT("look_ahead_p2p_send_enabled"), look_ahead_p2p_send_enabled, filename);
  //g_config->SetValue(Section, TEXT("timer_callback_interval"), timer_callback_interval, filename);
  //g_config->SetValue(Section, TEXT("bEnableNagleAlgorithm"), bEnableNagleAlgorithm, filename);
  //g_config->SetValue(Section, TEXT("HostIdGenerationPolicy"), HostIdGenerationPolicy, filename);
  //g_config->SetValue(Section, TEXT("UseTimerType"), UseTimerType, filename);
  //g_config->Flush(false, filename);
}

} // namespace fun
