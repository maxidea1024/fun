//TODO 코드 정리..
#pragma once

#include <winsvc.h>

#if FUN_PLATFORM_WINDOWS_FAMILY

namespace fun {
namespace net {

class INTServiceCallbacks
{
 public:
  virtual ~INTServiceCallbacks() {}

  virtual void Log(int32 type, const char* text) = 0;

  virtual void Run() = 0;

  virtual void Stop() = 0;

  virtual void Pause() = 0;

  virtual void Continue() = 0;
};

class NtServiceStartParameter
{
 public:
  INTServiceCallbacks* service_event;

  String service_name;

  inline NtServiceStartParameter()
    : service_event(nullptr)
    , service_name("FunCompile Service")
  {}
};

class NtService : protected Singleton<NtService>
{
 public:
  NETENGINE_API NtService();
  NETENGINE_API ~NtService();

  NETENGINE_API static void WinMain(int32 argc, char* argv[], char* envp[], const NtServiceStartParameter& args);
  NETENGINE_API const char* GetName();
  NETENGINE_API bool IsStartedBySCM() const;
  NETENGINE_API bool FindArg(const char* name);
  NETENGINE_API String CreateArg();
  NETENGINE_API BOOL IsInstalled();
  NETENGINE_API void Log(int32 type, const char* fmt, ...);
  NETENGINE_API void FrequentWarning(const char* text);
  NETENGINE_API void FrequentWarningWithCallStack(const char* text);

  // Utility
  NETENGINE_API static bool PumpMessages(uint32 timeout_msec);

 private:
  struct NtServiceInternal* internal_;

  static void WINAPI ServiceMainProc(DWORD argc, char** argv);
  void SetServiceStatus(DWORD state);
  void Run();
  void ServiceMain(int32 argc, char* argv[]);
  void Handler(DWORD opcode);
  static void WINAPI HandlerProc(DWORD opcode);
  void WinMain_ActualService();
  void WinMain_Console();
  BOOL Install();
  BOOL Uninstall();
  void WinMain_INTERNAL(const NtServiceStartParameter& args);
};

} // namespace net
} // namespace fun

#endif // FUN_PLATFORM_WINDOWS_FAMILY
