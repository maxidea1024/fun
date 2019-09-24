//TODO 코드정리
#include "NetEnginePrivate.h"

#include "NTService.h"
#include "IntraTracer.h"

#include <io.h>
#include <fcntl.h>

namespace fun {
namespace net {

struct NtServiceInternal
{
  Array<String> args;
  Array<String> ArgsFromScm;
  Array<String> EnvParams;

  bool bActualService;
  char Name[1000];

  DWORD LastFrequentWarning;

  SERVICE_STATUS_HANDLE ServiceStatusHandle;
  SERVICE_STATUS Status;
  INTServiceCallbacks* ServiceEvent;
  DWORD ThreadId;

  NtServiceInternal()
  {
    // 타 싱글톤과의 파괴 순서를 지키기 위해
    Tracer::Get();

    ServiceEvent = nullptr;
    LastFrequentWarning = 0;
  }
};

// 외부 DLL을 쓰는 경우 R6002 floating point not loaded라는 에러가 발생하는 경우가 있다.
// 그것을 예방하고자, 무조건 아래와 같은 루틴을 넣도록 한다.
void EnforceLoadFloatingPointModule()
{
  char* s = "1.2";
  float a = 1.2f;

#if (_MSC_VER >= 1400)
  sscanf_s(s, "%f", &a);
#else
  sscanf(s, "%f", &a);
#endif
}

NtService::NtService()
{
  Internal = new NtServiceInternal;
}

NtService::~NtService()
{
  delete Internal;
}

void NtService::WinMain(int argc, char* argv[], char* envp[], const NtServiceStartParameter& args)
{
  NtService& Core = NtService::Get();

  for (int32 i = 0; i < argc; ++i)
  {
    Core.Internal->args.Add(argv[i]);
  }
  //Internal->m_envp = envp;

  Core.WinMain_INTERNAL(args);
}

void NtService::WinMain_ActualService()
{
  SERVICE_TABLE_ENTRY st[] =
  {
    { Internal->Name, ServiceMainProc },
    { nullptr, nullptr }
  };

  if (!::StartServiceCtrlDispatcher(st)) {
    DWORD LastError = GetLastError();

    String text;

    // ERROR_FAILED_SERVICE_CONTROLLER_CONNECT 1063 이 에러는 콘솔 응용프로그램에서 서비스 모드를 실행하였을 때 나는 에러이다.
    if (LastError == 1063L) {
      text.Format(TEXT("콘솔 응용프로그램에서는 Service를 실행할 수 없습니다.콘솔에서 실행하려면 -console이 추가 파라메터로 붙어야 합니다. Service를 실행하시려면 제어판의 서비스에서 시작을 누르십시오. StartServiceCtrlDispatcher Error=%d"), LastError);
      Log(EVENTLOG_INFORMATION_TYPE, text);
    }
    else {
      text.Format(TEXT("StartServiceCtrlDispatcher Error=%d nettention Support로 문의하세요"), LastError);
      Log(EVENTLOG_INFORMATION_TYPE, text);

      // 1063이 아닐 때에는 콘솔 응용프로그램이 아니기 때문에 printf를 쓸 필요가 없다.
    }
  }
}

void NtService::WinMain_Console()
{
  Run();
}

void WINAPI NtService::ServiceMainProc(DWORD argc, char** argv)
{
  EnforceLoadFloatingPointModule();

  NtService::Get().ServiceMain(argc, argv);
}

// 이 함수는 이 프로그램이 서비스 매니저에 의해 실행될 때 호출된다. 즉 시작 버튼을 누를떄 호출된다.
// (서비스 모드에서는 main()이 호출되지 않는다.)
// \argc 서비스 시작 메시지. .exe를 실행할 떄의 파라메터가 아니다.
// \argv 서비스 시작 메시지. .exe를 실행할 떄의 파라메터가 아니다.
void NtService::ServiceMain(int32 argc, char* argv[])
{
#ifdef UNICODE
  _setmode(_fileno(stdout), _O_U16TEXT);
  _setmode(_fileno(stderr), _O_U16TEXT);
#else
  // UTF8
  _setmode(_fileno(stdout), _O_U8TEXT);
  _setmode(_fileno(stderr), _O_U8TEXT);
#endif

  for (int32 i = 0; i < argc; ++i) {
    Internal->ArgsFromScm.Add(argv[i]);
  }

  // Register the control request handler
  Internal->Status.dwCurrentState = SERVICE_START_PENDING;
  Internal->ServiceStatusHandle = RegisterServiceCtrlHandler(Internal->Name, HandlerProc);
  if (Internal->ServiceStatusHandle == nullptr) {
    Log(EVENTLOG_INFORMATION_TYPE, TEXT("Handler not installed"));
    return;
  }

  SetServiceStatus(SERVICE_START_PENDING);

  Internal->Status.dwWin32ExitCode = S_OK;
  Internal->Status.dwCheckPoint = 0;
  Internal->Status.dwWaitHint = 0;

  // when the run function returns, the service has stopped.
  Run();

  SetServiceStatus(SERVICE_STOPPED);

  Log(EVENTLOG_INFORMATION_TYPE, TEXT("serivce has stopped."));
}

void NtService::Handler(DWORD Opcode)
{
  switch (Opcode) {
  case SERVICE_CONTROL_STOP:
  case SERVICE_CONTROL_SHUTDOWN:
    Log(EVENTLOG_INFORMATION_TYPE, TEXT("requesting stopping service..."));
    if (Internal->ServiceEvent != nullptr) {
      Internal->ServiceEvent->Stop();
    }
    SetServiceStatus(SERVICE_STOP_PENDING);
    ::PostThreadMessage(Internal->ThreadId, WM_QUIT, 0, 0);
    break;

  case SERVICE_CONTROL_PAUSE:
    SetServiceStatus(SERVICE_PAUSE_PENDING);
    if (Internal->ServiceEvent != nullptr) {
      Internal->ServiceEvent->Pause();
    }
    SetServiceStatus(SERVICE_PAUSED);
    break;

  case SERVICE_CONTROL_CONTINUE:
    SetServiceStatus(SERVICE_CONTINUE_PENDING);
    if (Internal->ServiceEvent != nullptr) {
      Internal->ServiceEvent->Continue();
    }
    SetServiceStatus(SERVICE_RUNNING);
    break;

  case SERVICE_CONTROL_INTERROGATE:
    break;

  default:
    Log(EVENTLOG_INFORMATION_TYPE, TEXT("invalid service request"));
    break;
  }
}

void WINAPI NtService::HandlerProc(DWORD Opcode)
{
  NtService::Get().Handler(Opcode);
}

void NtService::SetServiceStatus(DWORD State)
{
  if (Internal->bActualService) {
    Internal->Status.dwCurrentState = State;

    ::SetServiceStatus(Internal->ServiceStatusHandle, &Internal->Status);
  }
}

bool NtService::FindArg(const char* Name)
{
  for (int32 i = 0; i < Internal->args.Count(); ++i) {
    if (_tcsicmp(Name, Internal->args[i]) == 0) {
      return true;
    }
  }

  return false;
}

String NtService::CreateArg()
{
  String Ret;

  for (int32 i = 0; i < Internal->args.Count(); ++i) {
    if (_tcsicmp(TEXT("-AR"), Internal->args[i]) == 0 ||
        _tcsicmp(TEXT("-console"), Internal->args[i]) == 0 ||
        _tcsicmp(TEXT("-install"), Internal->args[i]) == 0 ||
        _tcsicmp(TEXT("-uninstall"), Internal->args[i]) == 0) {
      continue;
    }

    Ret += Internal->args[i];
    Ret += TEXT(" ");
  }

  return Ret;
}

void NtService::Log(int32 Type, const char* Fmt, ...)
{
  if (Internal->ServiceEvent != nullptr) {
    String text;
    va_list ArgList;
    va_start(ArgList, Fmt);
    text.FormatV(Fmt, ArgList);
    va_end(ArgList);

    Internal->ServiceEvent->Log(Type, text);
  }
}

void NtService::Run()
{
  Internal->ThreadId = GetCurrentThreadId();

  SetServiceStatus(SERVICE_RUNNING);

  Log(EVENTLOG_INFORMATION_TYPE, TEXT("The service has started. PID=%d"), GetCurrentProcessId());

  SetThreadName(-1, TEXT("FunNet Service main"));

  if (Internal->ServiceEvent != nullptr) {
    Internal->ServiceEvent->Run();
  }
}

BOOL NtService::Install()
{
  // 기 동명 서비스를 제거.
  Uninstall();

  SC_HANDLE SCMHandle = ::OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
  if (SCMHandle == nullptr) {
    // 유니코드를 wprintf하여 한글이 출력되기 위해서는 _wsetlocale(LC_ALL, L"korean"); 되어 있어야 한다.
    _tprintf(TEXT("%s: service manager를 열 수 없습니다.  관리자 모드 실행이 아닐 수 있습니다.\n"), (const char*)Internal->Name);
    return FALSE;
  }

  // Get the executable file path
  char FilePath[_MAX_PATH];
  ::GetModuleFileName(nullptr, FilePath, _MAX_PATH);

  String BinaryPathName = String(FilePath) + TEXT(" ") + CreateArg();

  SC_HANDLE ServiceHandle = ::CreateService(
      SCMHandle, Internal->Name, Internal->Name,
      SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS ,
      SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
      BinaryPathName, nullptr, nullptr, nullptr, nullptr, nullptr
    );

  if (ServiceHandle == nullptr) {
    ::CloseServiceHandle(SCMHandle);

    String text = String::Format(TEXT("오류: NT 서비스 등록 불가능. GetLastError=%d"), GetLastError());
    _tprintf(TEXT("%s: %s"), (const char*)Internal->Name, (const char*)text);
    return FALSE;
  }

  // 서비스 실패(예: 크래쉬)시 자동 재시작을 하게 한다.
  if (FindArg(TEXT("-AR"))) {
    SC_ACTION Action;
    Action.type = SC_ACTION_RESTART;
    Action.Delay = 60000;

    SERVICE_FAILURE_ACTIONS FailureActions;
    UnsafeMemory::Memzero(&FailureActions);
    FailureActions.dwResetPeriod = 60 * 60 * 24;
    FailureActions.cActions = 1;
    FailureActions.lpsaActions = &Action;

    if (!ChangeServiceConfig2(ServiceHandle, SERVICE_CONFIG_FAILURE_ACTIONS, &FailureActions)) {
      _tprintf(TEXT("%s: 서비스 등록은 성공했지만 오류시 재시작 기능을 켜지 못했습니다.\n"), (const char*)Internal->Name);
    }
  }

  ::CloseServiceHandle(ServiceHandle);
  ::CloseServiceHandle(SCMHandle);

  return TRUE;
}

BOOL NtService::IsInstalled()
{
  BOOL bResult = FALSE;

  SC_HANDLE SCMHandle = ::OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
  if (SCMHandle != nullptr) {
    SC_HANDLE ServiceHandle = ::OpenService(SCMHandle, Internal->Name, SERVICE_QUERY_CONFIG);
    if (ServiceHandle != nullptr) {
      bResult = TRUE;

      ::CloseServiceHandle(ServiceHandle);
    }

    ::CloseServiceHandle(SCMHandle);
  }

  return bResult;
}

BOOL NtService::Uninstall()
{
  if (!IsInstalled()) {
    return TRUE;
  }

  SC_HANDLE SCMHandle = ::OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
  if (SCMHandle == nullptr) {
    _tprintf(TEXT("%s: Couldn't open service manager"), (const char*)Internal->Name);
    return FALSE;
  }

  SC_HANDLE ServiceHandle = ::OpenService(SCMHandle, Internal->Name, SERVICE_STOP | DELETE);
  if (ServiceHandle == nullptr) {
    ::CloseServiceHandle(SCMHandle);
    _tprintf(TEXT("%s: Couldn't open service"), (const char*)Internal->Name);
    return FALSE;
  }

  SERVICE_STATUS Status;
  ::ControlService(ServiceHandle, SERVICE_CONTROL_STOP, &Status);

  BOOL bDelete = ::DeleteService(ServiceHandle);
  ::CloseServiceHandle(ServiceHandle);
  ::CloseServiceHandle(SCMHandle);

  if (bDelete) {
    return TRUE;
  }

  _tprintf(TEXT("%s: Service could not be deleted"), (const char*)Internal->Name);
  return FALSE;
}

// FrequentWarningWithCallStack과 달리 콜 스택을 남기지 않는다.
void NtService::FrequentWarning(const char* text)
{
  DWORD current_time = GetTickCount();

  if (!Internal->LastFrequentWarning || current_time - Internal->LastFrequentWarning > 500) {
    Internal->LastFrequentWarning = current_time;

    if (Internal->ServiceEvent != nullptr) {
      Internal->ServiceEvent->Log(EVENTLOG_WARNING_TYPE, String::Format(TEXT("%s\n(짧은 시간동안 연속 발생 경고는 생략)"), text));
    }
  }
}

// 자주 발생할 수 있는 경고를 출력한다. 이 경고는 너무 많이 발생해서 서버가 오히려 과부하가 걸리는걸 막는 기능이 있다.
// 막는 방법은, 잦은 메시지는 버린다.
void NtService::FrequentWarningWithCallStack(const char* text)
{
  DWORD current_time = GetTickCount();

  if (!Internal->LastFrequentWarning || current_time - Internal->LastFrequentWarning > 500) {
    Internal->LastFrequentWarning = current_time;

    //StringA dmp;
    //g_CallStackDumper.GetDumped_ForGameServer(dmp);
    //String ee;
    //ee.Format(L"%s\n%s\n(짧은 시간동안 연속 발생 경고는 생략)", text, CA2W(dmp));

    if (Internal->ServiceEvent != nullptr) {
      Internal->ServiceEvent->Log(EVENTLOG_WARNING_TYPE, String::Format(TEXT("%s\n(짧은 시간동안 연속 발생 경고는 생략)"), text));
    }
    //FUN_TRACE("%s\n", CT2A(ee));
  }
}

//  int NtService::GetArgc()
//  {
//    return Internal->args.Count();
//  }

bool NtService::IsStartedBySCM() const
{
  return Internal->bActualService;
}

const char* NtService::GetName()
{
  return Internal->Name;
}

//  WIDECHAR** NtService::GetArgvFromSCM()
//  {
//    return Internal->ArgsvFromSCM;
//  }
//
//  int NtService::GetArgcFromSCM()
//  {
//    return Internal->m_argcFromSCM;
//  }
//
//  WIDECHAR** NtService::GetEnvp()
//  {
//    return Internal->m_envp;
//  }
//
//  WIDECHAR** NtService::GetArgv()
//  {
//    return Internal->m_argv;
//  }
//

void NtService::WinMain_INTERNAL(const NtServiceStartParameter& args)
{
  Internal->ServiceEvent = args.ServiceEvent;

#if (_MSC_VER >= 1400)
  _tcscpy_s(Internal->Name, COUNTOF(Internal->Name) - 1, args.ServiceName);
#else
  _tcscpy(Internal->Name, args.GetServiceName());
#endif

  // set up the initial service status
  Internal->ServiceStatusHandle = nullptr;
  Internal->Status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
  Internal->Status.dwCurrentState = SERVICE_STOPPED;
  Internal->Status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PAUSE_CONTINUE;
  Internal->Status.dwWin32ExitCode = 0;
  Internal->Status.dwServiceSpecificExitCode = 0;
  Internal->Status.dwCheckPoint = 0;
  Internal->Status.dwWaitHint = 0;

  if (FindArg(TEXT("-install"))) {
    if (!Install()) {
      _tprintf(TEXT("서비스 %s 등록 실패\n"), (const char*)Internal->Name);
    }

    return;
  }
  else if (FindArg(TEXT("-uninstall"))) {
    if (!Uninstall()) {
      _tprintf(TEXT("서비스 %s 등록 해제 실패\n"), (const char*)Internal->Name);
    }

    return;
  }

  Internal->bActualService = !FindArg(TEXT("-console"));

  if (Internal->bActualService == false) {
    WinMain_Console();
  }
  else {
    _tprintf(TEXT("NT Service mode로 실행을 시작합니다. 만약 콘솔에서 실행하는 것이면 -console 실행 파라메터를 첨부하십시오."));

    WinMain_ActualService();
  }
}


// Utility..

bool NtService::PumpMessages(uint32 timeout_msec)
{
  MSG msg;

  // 최대 일정 짧은 시간동안, 콘솔 입력, 윈도 메시지 수신, 메인 스레드 종료 중 하나를 기다린다.
  // Waiting one of console input, receiving windows message, close main thread during short period of time.
  ::MsgWaitForMultipleObjects(0, 0, TRUE, timeout_msec, QS_ALLEVENTS);

  if (::PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE)) {
    if (!::GetMessage(&msg, nullptr, 0, 0)) {
      return false;
    }

    ::TranslateMessage(&msg);
    ::DispatchMessage(&msg);
  }

  return true;
}

} // namespace net
} // namespace fun
