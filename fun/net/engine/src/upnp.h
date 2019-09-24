// TODO simple-socket 적용. Http::CSession도 검토.
#pragma once

#include "fun/net/net.h"

namespace fun {
namespace net {

class NetClientManager;
class UPnPTask;
class HttpSession;

typedef SharedPtr<UPnPTask> UPnPTaskPtr;

enum class SsdpState {
  None,
  Requesting,
  ResponseWaiting,
  Finished,
};

enum class HttpState {
  Init,
  Connecting,
  Requesting,
  ReponseWaiting,
  ParsingHttpResponse,
  Disposing,
  Disposed,
};

typedef SharedPtr<class DiscoveringNatDevice> DiscoveringNatDevicePtr;

class SsdpContext : public IInternalSocketDelegate {
 public:
  /** 로컬 주소입니다. */
  String local_addr_;

  /** 통신을 위해 사용되는 소켓 객체입니다. */
  InternalSocketPtr socket_;

  /** 현재 상태 값입니다. */
  SsdpState state_;

  /** 요청 횟수입니다. */
  int32 request_count_;

  /** 응답을 받기 위한 버퍼입니다. */
  ByteArray response_buffer_;

  /** Send issue 여부를 나타내는 플래그입니다. */
  FUN_ALIGNED_VOLATILE bool send_issued_;

  /** Receive issue 여부를 나타내는 플래그입니다. */
  FUN_ALIGNED_VOLATILE bool recv_issued_;

  /** 응답을 기다리기 시작한 시각입니다. */
  double response_wait_start_time_;

 private:
  virtual void OnSocketWarning(InternalSocket* socket, const String& text) {}

 public:
  /** 멤버 변수를 초기화합니다. */
  SsdpContext();

  /** 안전하게 종료가 가능한지 알아봅니다. */
  bool IsSafeDisposeGuaranteed() const;
};

typedef SharedPtr<SsdpContext> SsdpContextPtr;

class DiscoveredNatDevice;
typedef SharedPtr<DiscoveredNatDevice> DiscoveredNatDevicePtr;

class UPnP {
  friend class HttpSession;

 public:
  void Heartbeat();

  static ByteArray GetPortMappingRandomDesc();

 private:
  void InitSsdp();
  void ConditionalStopAllDiscoveryWorks();
  void Heartbeat_Http();
  void Heartbeat_Ssdp(SsdpContext* ssdp);
  void Heartbeat_Discovering(DiscoveringNatDevice* device);
  void Heartbeat_Discovered(DiscoveredNatDevice* device);
  void Heartbeat_Task(UPnPTask* task);

  void Heartbeat_Task_InitState(UPnPTask* task);
  void Heartbeat_Task_Connecting(UPnPTask* task);
  void Heartbeat_Task_Requesting(UPnPTask* task);

  ByteArray GetXmlNodeValue(const ByteArray& xml_doc,
                            const ByteArray& node_name) const;
  ByteArray GetXmlNodeValueAfter(const ByteArray& xml_doc,
                                 const ByteArray& node_name,
                                 const ByteArray& find_after) const;
  NetClientManager* manager_;

  CCriticalSection2 mutex_;

  int32 heartbeat_count_;

  typedef Map<ByteArray, DiscoveringNatDevicePtr> DiscoveringNatDevices;
  DiscoveringNatDevices discovering_nat_devices_;

  typedef List<DiscoveredNatDevicePtr> DiscoveredNatDevices;
  DiscoveredNatDevices discovered_nat_devices_;

  typedef Array<SsdpContextPtr> SsdpContextes;
  SsdpContextes ssdp_contextes_;

  double start_time_;

  // DeletePortMapping 명령이 실행되는 Task가 몇 개인지
  int32 delete_port_mapping_task_count_;

  virtual void OnSocketWarning(InternalSocket* socket, const String& msg) {}

  void Heartbeat_Ssdp_NoneCase(SsdpContext* ssdp);

  void Ssdp_IssueRequest(SsdpContext* ssdp);
  void Heartbeat_Ssdp_RequestingCase(SsdpContext* ssdp);

  void Ssdp_IssueResponseWait(SsdpContext* ssdp);
  void Heartbeat_Ssdp_ResponseWaitingCase(SsdpContext* ssdp);

 public:
  UPnP(NetClientManager* maneger);
  ~UPnP();

  void IssueTermination();
  void WaitUntilNoIssueGuaranteed();
  String GetNatDeviceName() const;

  void AddTcpPortMapping(const InetAddress& lan_addr,
                         const InetAddress& wan_addr, bool is_tcp);
  void DeleteTcpPortMapping(const InetAddress& lan_addr,
                            const InetAddress& wan_addr, bool is_tcp);

  void Heartbeat_Task_ResponseWaiting(UPnPTask* task);
  void Heartbeat_Task_ParsingHttpResponse(UPnPTask* task);

  DiscoveredNatDevice* GetCommandAvailableNatDevice() const;
};

class HttpSession {
 public:
  UPnP* owner_upnp_;
  InternalSocketPtr socket;
  ByteArray http_request_;

  FUN_ALIGNED_VOLATILE bool send_issued_;
  FUN_ALIGNED_VOLATILE bool recv_issued_;
  int32 http_request_issue_offset_;
  int32 http_response_issue_offset_;
  Array<uint8> http_response_buffer_;
  ByteArray http_response_;

 public:
  HttpSession()
      : owner_upnp_(nullptr),
        send_issued_(false),
        recv_issued_(false),
        http_response_issue_offset_(0),
        http_request_issue_offset_(0) {}

  bool IssueConnect(const String& host, int32 port);
  bool IssueRequestSend();
  bool IsSafeDisposeGuaranteed() const;
};

class DiscoveringNatDevice : public IInternalSocketDelegate,
                             public HttpSession {
 public:
  /** NAT device name. */
  ByteArray name_;

  /** URI */
  Uri uri_;

 private:
  /** 현재 상태 값입니다. */
  HttpState state_;

  /** 현재 상태로 바뀐 이후 Turn이 변경된 횟수입니다. (즉, 호출 횟수입니다.) */
  int32 state_turn_counter_;

 public:
  HttpState GetState() const { return state_; }
  void SetState(HttpState new_state);
  int32 GetCurrentStateTurnCounter() const { return state_turn_counter_; }
  void IncreaseCurrentStateTurnCounter() { state_turn_counter_++; }

  bool TakeResponse(const ByteArray& response);

  // IInternalSocketDelegate interface
  void OnSocketWarning(InternalSocket* socket, const String& msg) override {}

  DiscoveringNatDevice();
  ~DiscoveringNatDevice();
};

/**
NAT device 찾기가 완전히 끝난 객체. CDiscoveringNatDevice는 찾기 진행 중인 것과
상반된다.
*/
class DiscoveredNatDevice {
 public:
  /** 포트매핑 명령 수행 등을 하는 작업 큐 */
  List<UPnPTaskPtr> tasks_;

  /** 발견된 device의 이름. LAN 내 같은 모델의 공유기가 있을 수 있으므로 key로
   * 잡으면 곤란. */
  String device_name_;

  /** 공유기에 명령을 던질 때 공유기 측에서 받아 처리할 웹 페이지 주소 */
  ByteArray control_url_;

  /** 발견된 공유기의 연결 주소 */
  Uri uri_;

 public:
  bool IsSafeDisposeGuaranteed() const;
  void IssueTermination();
};

enum class UPnPTaskType {
  AddPortMapping,
  DeletePortMapping,
};

/**
upnp 사용 가능한 공유기를 찾았으면 거기에 명령을 던질 수 있다.
이것은 공유기에 명령 수행중인 상태 객체이다.
소켓 핸들과 io 진행 상태 값을 가짐.
*/
class UPnPTask : public IInternalSocketDelegate, public HttpSession {
 private:
  // IInternalSocketDelegate interface
  void OnSocketWarning(InternalSocket* socket, const String& msg) override {}

 public:
  HttpState state_;
  UPnPTaskType type_;

  // 이 명령 처리 대상의 공유기.
  DiscoveredNatDevice* owner_device_;

  // AddPortMapping 전용
  bool is_tcp_;

  // Add, DeletePortMapping 전용
  InetAddress lan_addr_;
  InetAddress wan_addr_;

  void BuildUPnPRequestText();

 private:
  void BuildUPnPRequestText_AddPortMapping();
  void BuildUPnPRequestText_DeletePortMapping();
};

class UPnpTcpPortMappingState {
 public:
  InetAddress lan_addr;
  InetAddress wan_addr;
};

}  // namespace net
}  // namespace fun
