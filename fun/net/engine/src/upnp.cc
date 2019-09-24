//@deprecated simple-socket을 사용해서 간단히 만들도록 하자. 아니면, Http::CSession을 사용해도 좋을듯..
//TODO simplesocket으로 변경하도록 하자.

/*

사용 프로토콜

  TCP/IP, HTTP, HTTPU, HTTPMU, SSDP, GENA, SOAP, XML


네트워킹 단계

  1. Addressing
  2. Discovery
  3. Description
  4. Control
  5. Eventing
  6. Presentation

1. Addressing


*/


//정리가 잘되어있는 사이트
// https://www.joinc.co.kr/w/Site/IOT/Discovery

#include "NetEnginePrivate.h"

#include "UPnP.h"
#include "Networker_C.h" // NetClientManager

namespace fun {
namespace net {

static const int32 HTTP_RESPONSE_BUFFER_LENGTH = 32768;

static const InetAddress SSDP_REQUEST_TO = InetAddress(String("239.255.255.250"), 1900);

static const ByteArray SSDP_REQUEST =
    ByteArray(
      "M-SEARCH * HTTP/1.1\r\n"
      "Host: 239.255.255.250:1900\r\n"
      "Man: \"ssdp:discover\"\r\n"
      "ST: upnp:rootdevice\r\n"
      "MX: 3\r\n"
      "\r\n");


//
// UPnP
//

UPnP::UPnP(NetClientManager* manager) {
  manager_ = manager;
  delete_port_mapping_task_count_ = 0;
  heartbeat_count_ = 0;
  start_time_ = manager_->GetCachedAbsoluteTime();
}

void UPnP::Heartbeat() {
  CScopedLock2 guard(mutex_); // 한 개의 스레드에서만 이것을 처리해야 하므로

  if (heartbeat_count_ == 0) {
    InitSsdp();
  }

  heartbeat_count_++;

  for (auto context : ssdp_contextes_) {
    Heartbeat_Ssdp(context.Get());
  }

  Heartbeat_Http();

  ConditionalStopAllDiscoveryWorks();
}

void UPnP::Heartbeat_Ssdp_NoneCase(SsdpContext* ssdp) {
  // 소켓 생성 및 SSDP broadcast를 한다.

  //TODO 이것이 되려면 IPv4 소켓에 브로드 캐스팅 해야함!
  ssdp->socket_.Reset(new InternalSocket(SocketType::Udp, ssdp));

  // turn-on broadcasting option
  if (ssdp->socket_->EnableBroadcastOption(true) != SocketErrorCode::Ok) {
    TRACE_SOURCE_LOCATION();
    ssdp->socket_.Reset();
    ssdp->state_ = SsdpState::Finished;
    return;
  }

  // Local address에 바인딩해야한다면...
  if (!ssdp->local_addr_.IsEmpty()) {
    if (!ssdp->socket_->Bind(*ssdp->local_addr_, 0)) {
      TRACE_SOURCE_LOCATION();
      ssdp->socket_.Reset();
      ssdp->state_ = SsdpState::Finished;
      return;
    }
  } else {
    if (!ssdp->socket_->Bind()) {
      TRACE_SOURCE_LOCATION();
      ssdp->socket_.Reset();
      ssdp->state_ = SsdpState::Finished;
      return;
    }
  }

  // 소켓을 넌블럭킹 모드로 바꾼 후 패킷을 쏜다.
  ssdp->socket_->SetBlockingMode(false);

  Ssdp_IssueRequest(ssdp);
}

void UPnP::Heartbeat_Ssdp_RequestingCase(SsdpContext* ssdp) {
  while (true) {
    //TODO Polling으로 대체하는게 바람직해 보임...

    OverlappedResult overlapped_result;
    if (!ssdp->socket_->GetSendOverlappedResult(false, overlapped_result)) {
      // Still pended..
      return;
    }

    mutex_.AssertIsLockedByCurrentThread();

    ssdp->send_issued_ = false;

    if (overlapped_result.socket_error != SocketErrorCode::Ok || overlapped_result.completed_length == 0) {
      // 송신이 중도 잘못됐음을 의미한다. 따라서 에러 처리한다.
      TRACE_SOURCE_LOCATION();
      ssdp->state_ = SsdpState::Finished;
      ssdp->socket_.Reset();
      return;
    }

    // UDP 이므로, 3번 정도 보내둔다. (혹시나 해서...)
    ssdp->request_count++;
    if (ssdp->request_count < 3) {
      Ssdp_IssueRequest(ssdp);
    } else {
      if (ssdp->response_wait_start_time_ == 0) {
        ssdp->response_wait_start_time_ = manager_->GetCachedAbsoluteTime();
      }

      Ssdp_IssueResponseWait(ssdp);
      return;
    }
  }
}

void UPnP::Heartbeat_Ssdp_ResponseWaitingCase(SsdpContext* ssdp) {
  while (true) {
    //TODO Polling으로 하는게 바람직해 보임...

    OverlappedResult overlapped_result;
    if (!ssdp->socket_->GetRecvOverlappedResult(false, overlapped_result)) {
      // Still pended..
      return;
    }

    ssdp->recv_issued_ = false;

    if (overlapped_result.socket_error != SocketErrorCode::Ok || overlapped_result.completed_length == 0) {
      // 송신이 중도 잘못됐음을 의미한다. 따라서 에러 처리한다.
      TRACE_SOURCE_LOCATION();
      ssdp->state_ = SsdpState::Finished;
      ssdp->socket_.Reset();
      return;
    }

    // 수신 완료. (ByteArray 자체가 대부분의 경우 nul-term을 보장하므로, 끝에 nul을 추가로 붙여줄필요 없음.)
    ByteArray reponse(overlapped_result.completed_length, NoInit);
    UnsafeMemory::Memcpy(reponse.MutableData(), ssdp->socket_->GetRecvBufferPtr(), overlapped_result.completed_length);

    // 여러개의 NAT일 수 있으므로 여러번 받도록 한다.
    DiscoveringNatDevicePtr new_nat(new DiscoveringNatDevice);
    new_nat->owner_upnp_ = this;

    if (new_nat->TakeResponse(reponse)) {
      if (discovering_nat_devices_.Count() < 20) { // THIS IS NEEDED
        const ByteArray Host = new_nat->uri_.GetHost();
        if (!discovering_nat_devices_.Contains(Host)) { // 같은 이름의 디바이스가 이미 등록되어 있으면 제외
          discovering_nat_devices_.Add(Host, new_nat);
        }
      }
    }

    const double time_diff = manager_->GetCachedAbsoluteTime() - ssdp->response_wait_start_time_;
    if (time_diff > 3) {
      // no more issue. 바로 종료해도 OK.
      TRACE_SOURCE_LOCATION();
      ssdp->socket_->CloseSocketHandleOnly();
      ssdp->state_ = SsdpState::Finished;
    } else {
      // 또 다른 Response가 올 수 있다. 수신을 다시 건다.
      Ssdp_IssueResponseWait(ssdp);
    }
  }
}

String UPnP::GetNatDeviceName() const {
  CScopedLock2 guard(const_cast<CCriticalSection2&>(mutex_));

  if (!discovered_nat_devices_.IsEmpty()) {
    return discovered_nat_devices_.Front()->device_name_;
  } else {
    return String();
  }
}

void UPnP::Ssdp_IssueRequest(SsdpContext* ssdp) {
  mutex_.AssertIsLockedByCurrentThread();

  ssdp->send_issued_ = true;

  const SocketErrorCode socket_error = ssdp->socket_->IssueSendTo((const uint8*)SSDP_REQUEST.ConstData(), SSDP_REQUEST.Len() + 1, SSDP_REQUEST_TO);
  if (socket_error != SocketErrorCode::Ok && socket_error != SocketErrorCode::IoPending) {
    ssdp->send_issued_ = false;
    ssdp->socket_.Reset();
    ssdp->state_ = SsdpState::Finished;

    char error_msg[2048];
    CPlatformMisc::GetSystemErrorMessage(error_msg, countof(error_msg), (int32)socket_error);
    TRACE_SOURCE_LOCATION_MSG(*String::Format("sendto failed: %s (socker_error: %d)", error_msg, (int32)socket_error));
  } else { // OK
    ssdp->state_ = SsdpState::Requesting;
  }
}

void UPnP::Ssdp_IssueResponseWait(SsdpContext* ssdp) {
  // UDP는 다음 송신을 걸 이유가 없다. 바로 완료이렸다.
  // 이제 수신 대기를 시작한다.

  ssdp->response_buffer_.ResizeUninitialized(8192);
  UnsafeMemory::Memset(ssdp->response_buffer_.MutableData(), 0x00, ssdp->response_buffer_.Len());

  ssdp->recv_issued_ = true;

  if (ssdp->socket_->IssueRecvFrom(ssdp->response_buffer_.Len() - 1) == SocketErrorCode::Ok) {
    ssdp->state_ = SsdpState::ResponseWaiting;
  } else {
    ssdp->recv_issued_ = false;
    ssdp->socket_.Reset();
    ssdp->state_ = SsdpState::Finished;
  }
}

void UPnP::Heartbeat_Ssdp(SsdpContext* ssdp) {
  switch (ssdp->state_) {
    case SsdpState::None:
      Heartbeat_Ssdp_NoneCase(ssdp);
      break;

    case SsdpState::Requesting:
      Heartbeat_Ssdp_RequestingCase(ssdp);
      break;

    case SsdpState::ResponseWaiting:
      Heartbeat_Ssdp_ResponseWaitingCase(ssdp);
      break;

    case SsdpState::Finished:
      // Do nothing, already mission is completed!
      break;
  }
}

void UPnP::Heartbeat_Http() {
  CScopedLock2 guard(mutex_);

  for (auto it = discovering_nat_devices_.CreateIterator(); it; ++it) {
    auto device = it->value;

    Heartbeat_Discovering(device.Get());

    if (device->GetState() == HttpState::Disposed) {
      it.RemoveCurrent();
    }
  }

  // 각 찾은 공유기들이 갖고 있는 Task들을 수행한다.
  // 가령 포트 매핑 명령을 비동기로 수행하는 Task들.
  for (auto device : discovered_nat_devices_) {
    Heartbeat_Discovered(device.Get());
  }
}

void UPnP::Heartbeat_Discovering(DiscoveringNatDevice* device) {
  switch (device->GetState()) {
    case HttpState::Connecting: {
        if (device->GetCurrentStateTurnCounter() == 0) { // first chance in this state
          device->socket_.Reset(new InternalSocket(SocketType::Tcp, device));
          device->socket_->Bind();
          device->socket_->SetBlockingMode(false);

          //const ByteArray URI = device->uri_.GetHost() + "/" + device->uri_.GetPath();
          //const ByteArray URI = ByteArray::Format("%s:%d",
          //        *device->uri_.GetHost(),
          //        device->uri_.GetPort());
          if (!device->IssueConnect(String(device->uri_.GetHost()), device->uri_.GetPort())) {
            device->SetState(HttpState::ParsingHttpResponse);
            return;
          }
        }

        InternalSocketSelectContext select_context;
        select_context.AddWriteWaiter(*device->socket_);
        select_context.AddExceptionWaiter(*device->socket_);

        // 기다리지 않고 폴링한다. 어차피 이 함수는 타이머에 의해 일정 시간마다 호출되니까.
        select_context.Wait(0); // no-wait

        SocketErrorCode socket_error;
        if (select_context.GetConnectResult(*device->socket_, socket_error)) {
          if (socket_error == SocketErrorCode::Ok) { // 정상적으로 접속이 이루어졌으므로, 요청을 보내도록 합니다.
            device->SetState(HttpState::Requesting);
            return;
          } else {
            // nonblock socket의 connect에서는 blocked socket에서는 없던
            // '아직 연결 결과가 안나왔는데도 연결 실패라고 오는' 경우가
            // 종종 있다. 이런 것도 이렇게 막아야 한다.
            if (!device->IssueConnect(String(device->uri_.GetHost()), device->uri_.GetPort())) {
              // 이미 completion된 상태이므로 안전.
              device->SetState(HttpState::ParsingHttpResponse);
              return;
            }
          }
        } else {
          // Still pended...
        }
      }
      break;

    case HttpState::Requesting: {
        if (device->GetCurrentStateTurnCounter() == 0) { // first chance in this state
          device->socket_->SetBlockingMode(true);

          ByteArray path = device->uri_.GetPath();
          ByteArray host = device->uri_.GetHost();

          //TODO 버그인지는 모르겠으나, Uri::GetPath()에서 반환된 경로명에 앞에가 '/'로 시작하지 않아서, 요청이 안되는 현상이 있음.
          if (!path.StartsWith('/')) {
            path.Prepend('/');
          }

          device->http_request_ = ByteArray::Format(
                        "GET %s HTTP/1.1\r\n\r\n"
                        "Host: %s:%d\r\n\r\n"
                        "Connection: keep-alive\r\n\r\n",
                        *path,
                        *host,
                        device->uri_.GetPort());

          if (!device->IssueRequestSend()) {
            device->SetState(HttpState::ParsingHttpResponse);
            return;
          }
        }

        OverlappedResult overlapped_result;
        if (device->socket_->GetSendOverlappedResult(0, overlapped_result)) {
          mutex_.AssertIsLockedByCurrentThread();

          device->send_issued_ = false;

          if (overlapped_result.completed_length < 0) {
            device->SetState(HttpState::ParsingHttpResponse);
            return;
          } else {
            device->http_request_issue_offset_ += overlapped_result.completed_length;

            const int32 length = device->http_request_.Len() - device->http_request_issue_offset_ + 1;
            if (length <= 0) {
              device->SetState(HttpState::ReponseWaiting);
              return;
            } else {
              device->IssueRequestSend();
            }
          }
        }
      }
      break;

    case HttpState::ReponseWaiting: {
        mutex_.AssertIsLockedByCurrentThread();

        if (device->GetCurrentStateTurnCounter() == 0) { // first chance in this state
          device->recv_issued_ = true;

          if (device->socket_->IssueRecv(HTTP_RESPONSE_BUFFER_LENGTH) != SocketErrorCode::Ok) {
            device->recv_issued_ = false;
            device->SetState(HttpState::Disposing);
            return;
          }
        }

        OverlappedResult overlapped_result;
        if (device->socket_->GetRecvOverlappedResult(0, overlapped_result)) {
          device->recv_issued_ = false;

          if (overlapped_result.completed_length <= 0) {
            device->SetState(HttpState::ParsingHttpResponse);
            return;
          } else {
            device->http_response_buffer_.Append(device->socket_->GetRecvBufferPtr(), overlapped_result.completed_length);
            device->http_response_issue_offset_ += overlapped_result.completed_length;

            const int32 length = HTTP_RESPONSE_BUFFER_LENGTH - device->http_response_issue_offset_;
            if (length <= 0) {
              device->SetState(HttpState::ParsingHttpResponse);
              return;
            } else {
              device->recv_issued_ = true;

              if (device->socket_->IssueRecv(length) != SocketErrorCode::Ok) {
                device->recv_issued_ = false;
                device->SetState(HttpState::Disposing);
                return;
              }
            }
          }
        }
      }
      break;

    // Parse HTTP reponse
    case HttpState::ParsingHttpResponse:
      if (!device->http_response_buffer_.IsEmpty()) {
        device->http_response_ = ByteArray((const char*)device->http_response_buffer_.ConstData(), device->http_response_buffer_.Count());
        device->http_response_.TrimToNulTerminator();

        auto sock_name = device->socket_->GetSockName();

        ByteArray nat_device_name;
        ByteArray control_url;

        // Parse HTTP reponse

        //FriendlyName + '##' + ModelDescription 형태로 되어 있는데, 그냥 FriendlyName만 있어도 좋지 않을까 싶은데...
        //nat_device_name = GetXmlNodeValue(device->http_response_, "FRIENDLYNAME") + ByteArray("##") + GetXmlNodeValue(device->http_response_, "MODELDESCRIPTION");
        nat_device_name = GetXmlNodeValue(device->http_response_, "FRIENDLYNAME");
        control_url = GetXmlNodeValueAfter(device->http_response_, "CONTROLURL", "WANIPCONNECTION");

        if (!nat_device_name.IsEmpty()) {
          LOG(LogNetEngine,Info,"FOUND device.  name: %s, control_url: %s", *String(nat_device_name), *String(ControlUrl));
        }

        if (nat_device_name.Len() > 2) {
          // NAT device를 완전히 찾았다. 새 항목을 추가하자.
          DiscoveredNatDevicePtr new_nat(new DiscoveredNatDevice);
          new_nat->uri_ = device->uri_; // copy
          new_nat->device_name_ = nat_device_name;
          new_nat->control_url_ = control_url;
          discovered_nat_devices_.Append(new_nat);
        } else {
          // 못찾았음...
        }
      }

      device->SetState(HttpState::Disposing);
      return;

    case HttpState::Disposing: {
        // Goto disposed state if no issue is guaranteed
        mutex_.AssertIsLockedByCurrentThread();

        if (device->GetCurrentStateTurnCounter() == 0) { // first chance in this state
          if (device->socket_) {
            device->socket_->CloseSocketHandleOnly();
          }
        }

        mutex_.AssertIsLockedByCurrentThread();

        OverlappedResult overlapped_result;
        if (device->socket_->GetRecvOverlappedResult(0, overlapped_result)) {
          device->recv_issued_ = false;
        }

        if (device->socket_->GetSendOverlappedResult(0, overlapped_result)) {
          device->send_issued_ = false;
        }

        if (!device->recv_issued_ && !device->send_issued_) {
          device->SetState(HttpState::Disposed);
          return;
        }
      }
      break;
  }

  device->IncreaseCurrentStateTurnCounter();
}

ByteArray UPnP::GetXmlNodeValue(const ByteArray& xml_doc,
                                const ByteArray& node_name) const {
  // Find by this capitalized text then trim with original one
  ByteArray xml_doc_cap = xml_doc; xml_doc_cap.MakeUpper();
  ByteArray node_name_cap = node_name; node_name_cap.MakeUpper();

  const ByteArray node_tag1_cap = ByteArray::Format("<%s>", *node_name_cap);

  int32 value_index1 = xml_doc_cap.IndexOf(node_tag1_cap);
  if (value_index1 >= 0) {
    value_index1 += node_tag1_cap.Len();
  }

  const ByteArray node_tag2_cap = ByteArray::Format("</%s>", *node_name_cap);
  const int32 value_index2 = xml_doc_cap.IndexOf(node_tag2_cap);

  if (value_index1 >= 0 && value_index2 >= 0 && value_index2 > value_index1) {
    return (xml_doc.Mid(value_index1, value_index2 - value_index1));
  } else {
    return ByteArray();
  }
}

ByteArray UPnP::GetXmlNodeValueAfter( const ByteArray& xml_doc,
                                      const ByteArray& node_name,
                                      const ByteArray& find_after) const {
  // Find by this capitalized text then trim with original one
  ByteArray xml_doc_cap = xml_doc; xml_doc_cap.MakeUpper();
  ByteArray node_name_cap = node_name; node_name_cap.MakeUpper();
  ByteArray find_after_cap = find_after; find_after_cap.MakeUpper();

  const int32 value_index0 = xml_doc_cap.IndexOf(find_after_cap);
  if (value_index0 < 0) {
    return "";
  }

  ByteArray xml_doc2_cap = xml_doc_cap.Right(xml_doc_cap.Len() - value_index0);
  ByteArray xml_doc2 = xml_doc.Right(xml_doc.Len() - value_index0);

  ByteArray node_tag1_cap;
  node_tag1_cap = ByteArray::Format("<%s>", *node_name_cap);

  int32 value_index1 = xml_doc2_cap.IndexOf(node_tag1_cap);
  if (value_index1 >= 0) {
    value_index1 += node_tag1_cap.Len();
  }

  ByteArray node_tag2_cap;
  node_tag2_cap = ByteArray::Format("</%s>", *node_name_cap);
  const int32 value_index2 = xml_doc2_cap.IndexOf(node_tag2_cap);

  if (value_index1 >= 0 && value_index2 >= 0 && value_index2 > value_index1) {
    return xml_doc2.Mid(value_index1, value_index2 - value_index1);
  } else {
    return ByteArray();
  }
}

void UPnP::IssueTermination() {
  CScopedLock2 guard(mutex_);

  // DeletePortMapping 명령은 웬만하면 반드시 모두 실행된 후에 게임 클라 프로세스가 종료되어야 한다.
  // 이를 위패 DeletePortMapping Task가 남아 있는 한 issue termination을 유보해야 한다.
  // 그러나 마냥 유보할 수는 없으므로 일정 시간 기다리도록 하자.
  int32 count = delete_port_mapping_task_count_;
  guard.Unlock();

  if (count > 0) {
    CPlatformProcess::Sleep(0.001f);
  }

  // 이제 실제로 종료하는 과정을 수행
  guard.guard();

  for (auto context : ssdp_contextes_) {
    if (context->socket_) {
      context->socket_->CloseSocketHandleOnly();
    }
  }

  // upnp Task도 모두 종료 지시해야.
  for (auto device : discovered_nat_devices_) {
    device->IssueTermination();
  }
}

void UPnP::WaitUntilNoIssueGuaranteed() {
  while (true) {
    CScopedLock2 guard(mutex_);

    //TODO
    // 프로세스 종료중이면 아무것도 하지 않는다.
    //if (CThread::bDllProcessDetached_INTERNAL) {
    //  return;
    //}

    if (GIsRequestingExit) {
      return;
    }

    OverlappedResult overlapped_result;
    for (auto it = ssdp_contextes_.CreateConstIterator(); it; ++it) {
      auto context = *it;

      if (context->socket_) {
        if (context->socket_->GetRecvOverlappedResult(false, overlapped_result)) {
          context->recv_issued_ = false;
        }

        if (context->socket_->GetSendOverlappedResult(false, overlapped_result)) {
          context->send_issued_ = false;
        }
      }

      if (!context->IsSafeDisposeGuaranteed()) {
        goto WaitingMore;
      }
    }

    mutex_.AssertIsLockedByCurrentThread();

    // Discovering NAT devices에 대한 마무리 처리
    for (auto it = discovering_nat_devices_.CreateIterator(); it; ++it) {
      auto device = it->value;

      if (!device->IsSafeDisposeGuaranteed()) {
        goto WaitingMore;
      }
    }

    // Discovered NAT devices에 대한 마무리 처리
    for (auto it = discovered_nat_devices_.CreateIterator(); it; ++it) {
      auto device = *it;

      if (!device->IsSafeDisposeGuaranteed()) {
        goto WaitingMore;
      }
    }

    return;

WaitingMore:
    guard.Unlock();

    CPlatformProcess::Sleep(0.01f);
  }
}

void UPnP::ConditionalStopAllDiscoveryWorks() {
  CScopedLock2 guard(mutex_);

  // Stop all if SSDP works for a very long time
  if ((manager_->GetCachedAbsoluteTime() - start_time_) > 60 * 10) {
    for (auto context : ssdp_contextes_) {
      if (context->socket_) {
        context->socket_->CloseSocketHandleOnly();
      }
    }

    for (auto& pair : discovering_nat_devices_) {
      pair.value->SetState(HttpState::Disposing);
    }
  }
}

void UPnP::InitSsdp() {
  CScopedLock2 guard(mutex_);

  const auto& local_addr_list = NetUtil::LocalAddresses();
  for (const auto& local_addr : local_addr_list) {
    //TODO 일단은, IPv4 주소만 다루도록 하자.
    //if (local_addr.Family() == InetAddress::IPv4)
    if (local_addr.IsIPv4MappedToIPv6()) {
      SsdpContextPtr context(new SsdpContext);
      context->local_addr_ = local_addr.ToString();
      ssdp_contextes_.Add(context);
    }
  }
}

void UPnP::AddTcpPortMapping( const InetAddress& lan_addr,
                              const InetAddress& wan_addr,
                              bool is_tcp) {
  CScopedLock2 guard(mutex_);

  if (auto device = GetCommandAvailableNatDevice()) {
    // 공유기의 task queue에, TCP연결-송신-수신-완료 과정을 수행할 첫 issue를 건다.
    // 건 이슈는 netClient manager worker thread에 의해서 issue connect가 될 것이다.
    // 이를 위한 heartbeat도 이미 작동하고 있다.
    UPnPTaskPtr task(new UPnPTask);
    task->owner_upnp_ = this;
    task->owner_device_ = device;
    task->type_ = UPnPTaskType::AddPortMapping;
    task->is_tcp_ = is_tcp;
    task->lan_addr_ = lan_addr;
    task->wan_addr_ = wan_addr;
    task->state_ = HttpState::Init;
    device->tasks_.Append(task);

    LOG(LogNetEngine,Warning,"AddTcpPortMapping.  lan_address: %s, wan_address: %s", *lan_addr.ToString(), *wan_addr.ToString());
  }
}

void UPnP::Heartbeat_Discovered(DiscoveredNatDevice* device) {
  for (auto it = device->tasks_.CreateIterator(); it;) {
    auto task = *it;

    Heartbeat_Task(task.Get());

    bool is_removed = false;
    if (task->state_ == HttpState::Disposed) {
      if (task->Type == UPnPTaskType::DeletePortMapping) {
        --delete_port_mapping_task_count_;
        device->tasks_.Remove(it);
        is_removed = true;
      }
    }

    if (!is_removed) {
      ++it;
    }
  }
}

void UPnP::Heartbeat_Task(UPnPTask* task) {
  switch (task->state_) {
    case HttpState::Init:
      Heartbeat_Task_InitState(task);
      break;

    case HttpState::Connecting:
      Heartbeat_Task_Connecting(task);
      break;

    case HttpState::Requesting:
      Heartbeat_Task_Requesting(task);
      break;

    case HttpState::ReponseWaiting:
      Heartbeat_Task_ResponseWaiting(task);
      break;

    case HttpState::ParsingHttpResponse:
      Heartbeat_Task_ParsingHttpResponse(task);
      break;

    case HttpState::Disposing:
      task->state_ = HttpState::Disposed;
      break;
  }
}

void UPnP::Heartbeat_Task_InitState(UPnPTask* task) {
  // Create a tcp socket object.
  task->socket_.Reset(new InternalSocket(SocketType::Tcp, task));

  // Bind to local address.
  if (!task->socket_->Bind()) {
    task->state_ = HttpState::Disposing;
    return;
  }

  // non-blocking 모드로 접속을 시도합니다.
  task->socket_->SetBlockingMode(false);
  if (!task->IssueConnect(String(task->owner_device_->uri_.GetHost()), task->owner_device_->uri_.GetPort())) {
    task->state_ = HttpState::Disposing;
    return;
  }

  task->state_ = HttpState::Connecting;
}

void UPnP::Heartbeat_Task_Connecting(UPnPTask* task) {
  InternalSocketSelectContext select_context;
  select_context.AddWriteWaiter(*task->socket_);
  select_context.AddExceptionWaiter(*task->socket_);

  // 기다리지 않고 폴링한다. 어차피 이 함수는 타이머에 의해 일정 시간마다 호출되니까.
  select_context.Wait(0); // no-wait

  SocketErrorCode socket_error;
  if (select_context.GetConnectResult(*task->socket_, socket_error)) {
    if (socket_error == SocketErrorCode::Ok) {
      // 연결에 성공했으므로, 비동기 모드를 해제하고,
      // AddPortMapping SOAP 명령을 보내도록 합니다.

      // Switch to blocking mode.
      task->socket_->SetBlockingMode(true);

      // Build SOAP command string.
      task->BuildUPnPRequestText();

      // Send request to device.
      if (!task->IssueRequestSend()) {
        task->state_ = HttpState::Disposing;
      } else {
        task->state_ = HttpState::Requesting;
      }
    } else {
      // nonblock socket의 connect에서는 blocked socket에서는 없던
      // '아직 연결 결과가 안나왔는데도 연결 실패라고 오는' 경우가
      // 종종 있다. 이런 것도 이렇게 막아야 한다.
      if (!task->IssueConnect(String(task->owner_device_->uri_.GetHost()), task->owner_device_->uri_.GetPort())) {
        // 이미 completion된 상태이므로 안전.
        task->state_ = HttpState::Disposing;
      }
    }
  } else {
    // pended...
  }
}

void UPnP::Heartbeat_Task_Requesting(UPnPTask* task) {
  OverlappedResult overlapped_result;
  if (task->socket_->GetSendOverlappedResult(false, overlapped_result)) {
    fun_check(mutex_.IsLockedByCurrentThread());

    task->send_issued_ = false;

    if (overlapped_result.completed_length < 0) { // error?
      task->state_ = HttpState::Disposing;
    } else {
      task->http_request_issue_offset_ += overlapped_result.completed_length;

      const int32 length = task->http_request_.Len() - task->http_request_issue_offset_ + 1;
      if (length <= 0) {
        task->recv_issued_ = true;

        if (task->socket_->IssueRecv(HTTP_RESPONSE_BUFFER_LENGTH) != SocketErrorCode::Ok) {
          task->recv_issued_ = false;

          task->state_ = HttpState::Disposing;
        } else {
          task->state_ = HttpState::ReponseWaiting;
        }
      } else {
        task->IssueRequestSend();
      }
    }
  }
}

void UPnP::Heartbeat_Task_ResponseWaiting(UPnPTask* task) {
  OverlappedResult overlapped_result;
  if (task->socket_->GetRecvOverlappedResult(false, overlapped_result)) {
    task->recv_issued_ = false;

    if (overlapped_result.completed_length <= 0) { // error?
      task->state_ = HttpState::ParsingHttpResponse;
    } else {
      task->http_response_buffer_.Append(task->socket_->GetRecvBufferPtr(), overlapped_result.completed_length);
      task->http_response_issue_offset_ += overlapped_result.completed_length;

      const int32 length = HTTP_RESPONSE_BUFFER_LENGTH - task->http_response_issue_offset_;
      if (length <= 0) { // done?
        task->state_ = HttpState::ParsingHttpResponse;
      } else {
        task->recv_issued_ = true;

        if (task->socket_->IssueRecv(length) != SocketErrorCode::Ok) {
          task->state_ = HttpState::ParsingHttpResponse;
        }
      }
    }
  }
}

void UPnP::Heartbeat_Task_ParsingHttpResponse(UPnPTask* task) {
  task->http_response_ = ByteArray((const char*)task->http_response_buffer_.GetData(), task->http_response_buffer_.Count());
  task->state_ = HttpState::Disposing;
}

ByteArray UPnP::GetPortMappingRandomDesc() {
  // 문자열이 너무 길게되면 일부 공유기에서 'bad request' 오류가 나오게 되므로 가능한 짧게 유지해야합니다.
  return ByteArray::Format("FUN_%d", Clock::Cycles());
}

void UPnP::DeleteTcpPortMapping(const InetAddress& lan_addr,
                                const InetAddress& wan_addr,
                                bool is_tcp) {
  CScopedLock2 guard(mutex_);

  // 두 개 이상의 upnp 공유기가 감지되면 하지 말자.
  // TODO: default gateway를 찾은 후 하는 것이 정석.
  if (discovered_nat_devices_.Count() != 1) {
    return;
  }

  auto device = discovered_nat_devices_.Front();

  // 공유기의 task queue에, TCP연결-송신-수신-완료 과정을 수행할 첫 issue를 건다.
  // 건 이슈는 netClient manager worker thread에 의해서 issue connect가 될 것이다.
  // 이를 위한 heartbeat도 이미 작동하고 있다.
  UPnPTaskPtr task(new UPnPTask);
  task->owner_upnp_ = this;
  task->owner_device_ = device.Get();
  task->type_ = UPnPTaskType::DeletePortMapping;
  task->is_tcp_ = is_tcp;
  task->lan_addr_ = lan_addr;
  task->wan_addr_ = wan_addr;
  task->state_ = HttpState::Init;
  device->tasks_.Append(task);

  delete_port_mapping_task_count_++;
}

DiscoveredNatDevice* UPnP::GetCommandAvailableNatDevice() const {
  // 두 개 이상의 upnp 공유기가 감지되면 하지 말자.
  // TODO: default gateway를 찾은 후 하는 것이 정석.
  if (discovered_nat_devices_.Count() != 1) {
    return nullptr;
  }

  return discovered_nat_devices_.Front().Get();
}

UPnP::~UPnP() {
  CScopedLock2 guard(mutex_);

  discovering_nat_devices_.Clear();
  discovered_nat_devices_.Clear();
  ssdp_contextes_.Clear();
}


//
// DiscoveringNatDevice
//

bool DiscoveringNatDevice::TakeResponse(const ByteArray& response) {
  ByteArray xml_uri;

  const int32 begin = response.IndexOf("http");
  const int32 end = response.IndexOf(".xml");
  if (begin < end && begin >= 0) {
    xml_uri = response.Mid(begin, end - begin + 4);
  }

  // Resolve taken uri.
  uri_.Resolve(xml_uri);

  return true;
}

DiscoveringNatDevice::~DiscoveringNatDevice() {
  //fun_check(socket == nullptr);
}

DiscoveringNatDevice::DiscoveringNatDevice()
  : state_(HttpState::Connecting), state_turn_counter_(0) {
}

void DiscoveringNatDevice::SetState(HttpState new_state) {
  if (new_state > state_) {
    state_ = new_state;
    state_turn_counter_ = 0;
  }
}


//
// DiscoveredNatDevice
//

bool DiscoveredNatDevice::IsSafeDisposeGuaranteed() const {
  for (auto task : tasks_) {
    if (!task->IsSafeDisposeGuaranteed()) {
      return false;
    }
  }

  return true;
}

void DiscoveredNatDevice::IssueTermination() {
  for (auto task : tasks_) {
    if (task->socket_) {
      task->socket_->CloseSocketHandleOnly();

      if (IsSafeDisposeGuaranteed()) {
        task->state_ = HttpState::Disposing;
      }
    }
  }
}


//
// HttpSession
//

bool HttpSession::IssueConnect(const String& host, int32 port) {
  const SocketErrorCode socket_error = socket_->Connect(host, port);
  return socket_error == SocketErrorCode::Ok || socket_error == SocketErrorCode::WouldBlock;
}

bool HttpSession::IssueRequestSend() {
  fun_check(owner_upnp_->mutex_.IsLockedByCurrentThread());

  send_issued_ = true;

  const int32 offset = http_request_issue_offset_;
  const int32 length = http_request_.Len() - offset + 1;
  if (socket_->IssueSend((const uint8*)*http_request_ + offset, length) != SocketErrorCode::Ok) {
    send_issued_ = false;
    return false;
  } else {
    return true;
  }
}

bool HttpSession::IsSafeDisposeGuaranteed() const {
  if (!socket_.IsValid()) {
    return true;
  }

  if (!socket_->IsClosed()) {
    return false;
  }

  //TODO bSendIssued가 true로 지속적으로 바뀌고 있음...
  return !send_issued_ && !recv_issued_;
}


//
// SsdpContext
//

SsdpContext::SsdpContext()
  : send_issued_(false),
    recv_issued_(false),
    request_count_(0),
    response_wait_start_time_(0),
    state_(SsdpState::None) {
}

bool SsdpContext::IsSafeDisposeGuaranteed() const {
  if (!socket_.IsValid()) {
    return true;
  }

  if (!socket_->IsClosed()) {
    return false;
  }

  return !send_issued_ && !recv_issued_;
}


//
// UPnPTask
//

void UPnPTask::BuildUPnPRequestText() {
  switch (Type) {
    case UPnPTaskType::AddPortMapping:
      BuildUPnPRequestText_AddPortMapping();
      break;

    case UPnPTaskType::DeletePortMapping:
      BuildUPnPRequestText_DeletePortMapping();
      break;

    default:
      fun_check(0);
      state_ = HttpState::Disposing;
      break;
  }
}

void UPnPTask::BuildUPnPRequestText_AddPortMapping() {
  auto device = owner_device_;

  ByteArray soap_cmd_token[20];
  soap_cmd_token[0] =
    "<?xml version=\"1.0\"?>\r\n"
    "<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/Soap/envelope/\" "
    "SOAP-ENV:encodingStyle=\"http://schemas.xmlsoap.org/Soap/encoding/\"><SOAP-ENV:Body>"
    "<m:AddPortMapping xmlns:m=\"urn:schemas-upnp-org:service:WANIPConnection:1\">"
    "<NewRemoteHost xmlns:dt=\"urn:schemas-microsoft-com:datatypes\" dt:dt=\"string\">"
    "</NewRemoteHost><NewExternalPort xmlns:dt=\"urn:schemas-microsoft-com:datatypes\" dt:dt=\"ui2\">";
  soap_cmd_token[1] = ByteArray::FromNumber(wan_addr.GetPort());
  soap_cmd_token[2] =
    "</NewExternalPort><NewProtocol xmlns:dt=\"urn:schemas-microsoft-com:datatypes\" dt:dt=\"string\">";
  soap_cmd_token[3] = is_tcp ? "TCP" : "UDP";
  soap_cmd_token[4] = "</NewProtocol><NewInternalPort xmlns:dt=\"urn:schemas-microsoft-com:datatypes\" dt:dt=\"ui2\">";
  soap_cmd_token[5] = ByteArray::FromNumber(lan_addr.GetPort());
  soap_cmd_token[6] = "</NewInternalPort><NewInternalClient xmlns:dt=\"urn:schemas-microsoft-com:datatypes\" dt:dt=\"string\">";
  soap_cmd_token[7] = lan_addr.GetHost().ToString();
  soap_cmd_token[8] =
    "</NewInternalClient>"
    "<NewEnabled xmlns:dt=\"urn:schemas-microsoft-com:datatypes\" dt:dt=\"boolean\">1</NewEnabled>"
    "<NewPortMappingDescription xmlns:dt=\"urn:schemas-microsoft-com:datatypes\" dt:dt=\"string\">";
  soap_cmd_token[9] = UPnP::GetPortMappingRandomDesc();
  soap_cmd_token[10] = "</NewPortMappingDescription><NewLeaseDuration xmlns:dt=\"urn:schemas-microsoft-com:datatypes\" dt:dt=\"ui4\">0</NewLeaseDuration></m:AddPortMapping></SOAP-ENV:Body></SOAP-ENV:Envelope>";

  ByteArray soap_cmd;
  for (int32 i = 0; i < 20; ++i) {
    soap_cmd += soap_cmd_token[i];
  }

  ByteArray header = ByteArray::Format(
      "POST %s HTTP/1.1\r\n"
      "Cache-Control: no-cache\r\n"
      "Connection: Close\r\n"
      "Pragma: no-cache\r\n"
      "Content-Type: text/xml; charset=\"utf-8\"\r\n"
      "User-Agent: Microsoft-Windows/6.1 UPnP/1.0\r\n"
      "SOAPAction: \"urn:schemas-upnp-org:service:WANIPConnection:1#AddPortMapping\"\r\n"
      "Content-length: %d\r\n"
      "Host: %s:%d\r\n\r\n",
      *device->control_url_,
      soap_cmd.Len(),
      *device->uri_.GetHost(),
      device->uri_.GetPort());

  http_request_ = header + soap_cmd;
}

void UPnPTask::BuildUPnPRequestText_DeletePortMapping() {
  //코드가 살짝 지저분함. 차후에 수정하도록 하자.
  auto device = owner_device_;

  ByteArray soap_cmd_token[20];
  soap_cmd_token[0] =
    "<?xml version=\"1.0\"?>"
    "<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/Soap/envelope/\" "
    "SOAP-ENV:encodingStyle=\"http://schemas.xmlsoap.org/Soap/encoding/\">"
    "<SOAP-ENV:Body><m:DeletePortMapping xmlns:m=\"urn:schemas-upnp-org:service:WANIPConnection:1\">"
    "<NewRemoteHost xmlns:dt=\"urn:schemas-microsoft-com:datatypes\" dt:dt=\"string\"></NewRemoteHost>"
    "<NewExternalPort xmlns:dt=\"urn:schemas-microsoft-com:datatypes\" dt:dt=\"ui2\">";

  soap_cmd_token[1] = ByteArray::FromNumber(wan_addr.GetPort());
  soap_cmd_token[2] = "</NewExternalPort><NewProtocol xmlns:dt=\"urn:schemas-microsoft-com:datatypes\" dt:dt=\"string\">";
  soap_cmd_token[3] = is_tcp ? "TCP" : "UDP";
  soap_cmd_token[4] = "</NewProtocol></m:DeletePortMapping></SOAP-ENV:Body></SOAP-ENV:Envelope>";

  ByteArray soap_cmd;
  for (int32 i = 0; i < 20; ++i) {
    soap_cmd += soap_cmd_token[i];
  }

  ByteArray header = ByteArray::Format(
      "POST %s HTTP/1.1\r\n"
      "Cache-Control: no-cache\r\n"
      "Connection: Close\r\n"
      "Pragma: no-cache\r\n"
      "Content-Type: text/xml; charset=\"utf-8\"\r\n"
      "User-Agent: Microsoft-Windows/6.1 UPnP/1.0\r\n"
      "SOAPAction: \"urn:schemas-upnp-org:service:WANIPConnection:1#DeletePortMapping\"\r\n"
      "Content-length: %d\r\n"
      "Host: %s:%d\r\n\r\n",
      *device->control_url_,
      soap_cmd.Len(),
      *device->uri_.GetHost(),
      device->uri_.GetPort());

  http_request_ = header + soap_cmd;
}

} // namespace net
} // namespace fun
