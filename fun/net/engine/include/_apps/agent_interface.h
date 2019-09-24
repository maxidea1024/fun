#pragma once

namespace fun {
namespace net {

class ReportStatus
{
public:
  enum class EType {
    OK,
    Warning,
    Error
  };

  EStatusType Type;
  String text;

  typedef Map<String, String> CKeyValueList;
  CKeyValueList KVs;
};


class IAgentConnectorDelegate
{
public:
  virtual void OnAuthentication(const ResultInfo* result_info) = 0;

  virtual void OnStopCommand() = 0;

  virtual void OnReportStatsCommand() = 0;

  virtual void OnWarning(const ResultInfo* result_info) = 0;

  virtual ~IAgentConnectorDelegate() {}
};


class AgentConnector
{
public:
  NETENGINE_API static AgentConnector* New(IAgentConnectorDelegate* Delegate);

public:
  virtual bool Start() = 0;

  virtual bool SendReportStatus(const ReportStatus& Status) = 0;

  virtual bool EventLog(ReportStatus::EType Type, const char* text) = 0;

  virtual void Tick() = 0;

  virtual void SetDelayTimeAboutSendAgentStatus(uint32 DelayInMSec) = 0;

  virtual ~AgentConnector() {}
};

} // namespace net
} // namespace fun
