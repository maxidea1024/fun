#pragma once

namespace fun {
namespace net {

extern const Uuid EMERGENCY_PROTCOL_VERSION;

class IEmergencyLogServerDelegate
{
public:
  virtual void OnStartServer(StartServerArgs& MutableParams) = 0;

  virtual bool ShouldStop() = 0;

  virtual CCriticalSection2& GetMutex() = 0;

  virtual void OnServerStartComplete(const ResultInfo* result_info) = 0;

  virtual void OnTick() {}

  virtual ~IEmergencyLogServerDelegate() {}
};


class EmergencyLogServer
{
public:
  NETENGINE_API static EmergencyLogServer* New(IEmergencyLogServerDelegate* Delegate);

  virtual void Serve() = 0;

  virtual ~EmergencyLogServer() {}
};

} // namespace net
} // namespace fun
