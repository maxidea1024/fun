//<todo> 플랫폼 와이드하게 윈도우즈 전용 함수를 별도로 랩핑하는게 좋을듯 함..
#include "NetEnginePrivate.h"
#include "Net/Engine/Apps/AgentConnector.h"

#include "GeneratedRPCs/Agent_AgentC2S_proxy.cc"
#include "GeneratedRPCs/Agent_AgentS2C_stub.cc"

#include <psapi.h>
#pragma comment(lib, "psapi.lib")

namespace fun {
namespace net {

AgentConnector* AgentConnector::New(IAgentConnectorDelegate* delegate)
{
  return new AgentConnectorImpl(delegate);
}

AgentConnectorImpl::AgentConnectorImpl(IAgentConnectorDelegate* delegate)
{
  delegate_ = delegate;

  client_ = nullptr;

  delay_time_about_send_agent_status_ = 1000;
  prev_kernel_time_ = 0;
  prev_user_time_ = 0;
  prev_instrumented_time_ = 0;
}

AgentConnectorImpl::~AgentConnectorImpl()
{
}

void AgentConnectorImpl::OnJoinedToServer(const ResultInfo* result_info, const ByteArray& reply_from_server)
{
  if (!result_info->IsOk()) {
    OutputDebugString(*String::Format("OnJoinedToServer is failed. result_info=%s\n", *result_info->ToString()));
  }

  proxy_.RequestCredential(HostId_Server, RpcCallOption::StrongSecureReliable, cookie_);
}

void AgentConnectorImpl::OnLeftFromServer(const ResultInfo* result_info)
{
  if (!ResultInfo->IsOk()) {
    OutputDebugString(*String::Format("OnLeftFromServer is failed. result_info=%s\n", *result_info->ToString()));
  }
}

bool AgentConnectorImpl::Start()
{
  LanConnectionArgs args;

  // Agent가 CreateProcess할때 넣은 환경변수를 구하기
  char port_value[128];
  char cookie_value[128];
  if (GetEnvironmentVariableA("agent_port", port_value, 128) == 0 ||
      GetEnvironmentVariableA("agent_cookie", cookie_value, 128) == 0) {
      OutputDebugString(*String::Format("GetEnvironmentVariable is failed. GetLastError=%u\n", GetLastError()));
    return false;
  }

  cookie_ = atoi(cookie_value);
  args.server_ip = "127.0.0.1"; // local by default.
  args.server_port = atoi(port_value);

  client_ = LanClient::New();

  client_->SetCallbacks(this);
  client_->AttachProxy(&proxy_);
  client_->AttachStub(this);

  last_try_connect_time_ = GetTickCount();

  SharedPtr<ResultInfo> out_error;
  if (!client_->Connect(args, out_error)) {
    OutputDebugString("connect is failed.\n");
    return false;
  }

  last_send_status_time_ = GetTickCount();
  return true;
}

bool AgentConnectorImpl::SendReportStatus(const ReportStatus& status)
{
  if (client_ == nullptr) {
    return false;
  }

  // 문자열이 비어서는 안된다.
  if (status.StatusText.IsEmpty()) {
    delegate_->OnWarning(ResultInfo::From(ResultCode::Unexpected, HostId_None, "Agent SendReportStatus에 빈 문자열이 들어왔습니다."));

    //<todo>
    //이것좀 단순화 시킬수는 없으려나??
    //인자를 너무 많이 넣어야한다..
    proxy_.ReportStatusBegin(HostId_Server, RpcCallOption::Reliable, (uint8)ReportStatus::StatusType_Warning, "Status에 Status정보를 넣으세요");
    proxy_.ReportStatusEnd(HostId_Server, RpcCallOption::Reliable);
    return false;
  }

  proxy_.ReportStatusBegin(HostId_Server, RpcCallOption::Reliable, (uint8)status.StatusType, status.StatusText);

  for (auto& pair : status.list) {
    if (pair.key.IsEmpty() || pair.value.IsEmpty()) {
      delegate_->OnWarning(ResultInfo::From(ResultCode::Unexpected, HostId_None, "Agent SendReportStatus에 빈 문자열이 들어왔습니다."));
      proxy_.ReportStatusValue(HostId_Server, RpcCallOption::Reliable, TEXT("OnWarning"), "Agent SendReportStatus에 빈 문자열이 들어왔습니다.");
    }
    else {
      proxy_.ReportStatusValue(HostId_Server, RpcCallOption::Reliable, pair.first, pair.second);
    }
  }

  proxy_.ReportStatusEnd(HostId_Server, RpcCallOption::Reliable);

  return true;
}

bool AgentConnectorImpl::EventLog(ReportStatus::EStatusType log_type, const char* text)
{
  if (client_ == nullptr || text[0] == 0) {
    return false;
  }

  proxy_.ReportStatusBegin(HostId_Server, RpcCallOption::Reliable, (uint8)log_type, text);
  proxy_.ReportStatusEnd(HostId_Server, RpcCallOption::Reliable);
  return true;
}

DEFRPC_AgentS2C_NotifyCredential(AgentConnectorImpl)
{
  if (bAuthentication == true) {
    delegate_->OnAuthentication(ResultInfo::From(ResultCode::Ok));
  }
  else {
    delegate_->OnAuthentication(ResultInfo::From(ResultCode::Unexpected));
  }

  return true;
}

DEFRPC_AgentS2C_RequestServerAppStop(AgentConnectorImpl)
{
  OutputDebugString("RequestServerAppStop\n");

  delegate_->OnStopCommand();
  return true;
}

void AgentConnectorImpl::Tick()
{
  if (client_ == nullptr) {
    return;
  }

  if (client_->GetServerConnectionState() != ConnectionState::Connected) {
    if (client_->GetServerConnectionState() != ConnectionState::Connecting &&
        (GetTickCount() - last_try_connect_time_) > 1000) {
      // Try re-connect
      Start();

      last_try_connect_time_ = GetTickCount();
    }
    return;
  }

  if ((GetTickCount() - last_send_status_time_) > delay_time_about_send_agent_status_) {
    // CPU 타임 알아오기
    float cpu_user_time;
    float cpu_kernel_time;
    GetCpuTime(cpu_user_time, cpu_kernel_time);

    PROCESS_MEMORY_COUNTERS_EX mem_info;
    UnsafeMemory::Memzero(&mem_info);
    mem_info.cb = sizeof(PROCESS_MEMORY_COUNTERS_EX);

    GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&mem_info, sizeof(PROCESS_MEMORY_COUNTERS_EX));
    DWORD mem_size = (DWORD)mem_info.WorkingSetSize;

    proxy_.ReportServerAppState(HostId_Server, RpcCallOption::Reliable, cpu_user_time, cpu_kernel_time, mem_size);

    last_send_status_time_ = GetTickCount();
  }
}

void AgentConnectorImpl::SetDelayTimeAboutSendAgentStatus(uint32 delay_msec)
{
  delay_time_about_send_agent_status_ = delay_msec;
}

void AgentConnectorImpl::GetCpuTime(float& out_user_time, float& out_kernel_time)
{
  const double current_time = GetPreciseCurrentTime();

  FILETIME creation_time, exit_time, kernel_time, user_time;
  ::GetProcessTimes(GetCurrentProcess(), &creation_time, &exit_time, &kernel_time, &user_time);

  uint64 user_time2 = *(uint64*)&user_time;
  uint64 kernel_time2 = *(uint64*)&kernel_time;

  // For first visit
  if (prev_user_time_ == 0) {
    prev_user_time_ = user_time2;
  }

  if (prev_kernel_time_ == 0) {
    prev_kernel_time_ = kernel_time2;
  }

  if (prev_instrumented_time_ == 0) {
    prev_instrumented_time_ = current_time;
  }

  const uint64 user_time_diff = user_time2 - prev_user_time_;
  const uint64 kernel_time_diff = kernel_time2 - prev_kernel_time_;
  const double time_diff = current_time - prev_instrumented_time_;

  prev_user_time_ = user_time2;
  prev_kernel_time_ = kernel_time2;
  prev_instrumented_time_ = current_time;

  if (time_diff > 0) {
    out_user_time = (float)((double)user_time_diff / 100000 / time_diff / GetNoofProcessors());
    out_kernel_time = (float)((double)kernel_time_diff / 100000 / time_diff / GetNoofProcessors());
  }
  else {
    out_user_time = 0;
    out_kernel_time = 0;
  }
}

} // namespace net
} // namespace fun
