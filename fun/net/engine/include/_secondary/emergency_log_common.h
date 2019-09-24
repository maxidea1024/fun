//@deprecated
 //TODO 제거하는 쪽으로 생각해보도록 하자.

#pragma once

namespace fun {
namespace net {

extern const GUID EMERGENCY_PROTCOL_VERSION;

class IEmergencyLogServerDelegate
{
 public:
  virtual ~IEmergencyLogServerDelegate() {}

  virtual void OnStartServer(StartServerArgs& args) = 0;

  virtual bool ShouldStop() = 0;

  virtual CCriticalSection2* GetMutex() = 0;

  virtual void OnServerStartComplete(const ResultInfo* result_info) = 0;

  virtual void OnTick() {}
};

class EmergencyLogServer
{
 public:
  NETENGINE_API static EmergencyLogServer* New(IEmergencyLogServerDelegate* delegate);

  virtual ~EmergencyLogServer() {}

  virtual void RunMainLoop() = 0;
};

} // namespace net
} // namespace fun
