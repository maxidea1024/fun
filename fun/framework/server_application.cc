#include "fun/framework/server_application.h"
#include "fun/framework/option.h"
#include "fun/framework/option_exception.h"
#include "fun/framework/option_set.h"
// TODO
//#include "fun/base/file_stream.h"
#include "fun/base/exception.h"

#if FUN_PLATFORM != FUN_PLATFORM_VXWORKS
#include "fun/base/named_event.h"
#include "fun/base/process.h"
#endif  //#if FUN_PLATFORM != FUN_PLATFORM_VXWORKS

#include "fun/base/logging/logger.h"
#include "fun/base/str.h"

#if FUN_PLATFORM_UNIX_FAMILY && (FUN_PLATFORM != FUN_PLATFORM_VXWORKS)
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>
#include "fun/base/temporary_file.h"
#elif FUN_PLATFORM_WINDOWS_FAMILY
#if !defined(_WIN32_WCE)
#include "fun/framework/win_registry_key.h"
#include "fun/framework/win_service.h"
#endif
#include <cstring>
#include "fun/base/windows_less.h"
#endif

//#include "fun/base/unicode_converter.h"

namespace fun {
namespace framework {

#if FUN_PLATFORM_WINDOWS_FAMILY
NamedEvent ServerApplication::terminate_(
    ProcessImpl::TerminationEventName(Process::CurrentPid()));
#if !defined(_WIN32_WCE)
Event ServerApplication::terminated_;
SERVICE_STATUS ServerApplication::service_status_;
SERVICE_STATUS_HANDLE ServerApplication::service_status_handle_ = 0;
#endif
#endif

#if FUN_PLATFORM == FUN_PLATFORM_VXWORKS ||                      \
    FUN_PLATFORM == FUN_PLATFORM_ANDROID || defined(__NACL__) || \
    defined(__EMSCRIPTEN__)
Event ServerApplication::terminate_;
#endif

ServerApplication::ServerApplication() {
#if FUN_PLATFORM_WINDOWS_FAMILY
#if !defined(_WIN32_WCE)
  action_ = SRV_RUN;
  UnsafeMemory::Memset(&service_status_, 0, sizeof(service_status_));
#endif
#endif
}

ServerApplication::~ServerApplication() {}

bool ServerApplication::IsInteractive() const {
  bool run_in_background =
      GetConfig().GetBool("application.run_as_daemon", false) ||
      GetConfig().GetBool("application.run_as_service", false);
  return !run_in_background;
}

int32 ServerApplication::Run() { return Application::Run(); }

void ServerApplication::Terminate() {
#if FUN_PLATFORM_WINDOWS_FAMILY
  terminate_.Set();
#elif FUN_PLATFORM != FUN_PLATFORM_VXWORKS ||                    \
    FUN_PLATFORM == FUN_PLATFORM_ANDROID || defined(__NACL__) || \
    defined(__EMSCRIPTEN__)
  terminate_.Set();
#else
  Process::RequestTermination(Process::CurrentPid());
#endif
}

#if FUN_PLATFORM_WINDOWS_FAMILY
#if !defined(_WIN32_WCE)

//
// Windows specific code
//

BOOL ServerApplication::ConsoleCtrlHandler(DWORD ctrl_type) {
  switch (ctrl_type) {
    case CTRL_C_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_BREAK_EVENT:
      Terminate();
      return terminated_.TryWait(10000) ? TRUE : FALSE;
    default:
      return FALSE;
  }
}

HDEVNOTIFY
ServerApplication::RegisterServiceDeviceNotification(LPVOID filter,
                                                     DWORD flags) {
  return RegisterDeviceNotification(service_status_handle_, filter, flags);
}

DWORD ServerApplication::HandleDeviceEvent(DWORD /*event_type*/,
                                           LPVOID /*event_data*/) {
  return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD ServerApplication::ServiceControlHandler(DWORD control, DWORD event_type,
                                               LPVOID event_data,
                                               LPVOID context) {
  DWORD result = NO_ERROR;
  ServerApplication* this_ptr = reinterpret_cast<ServerApplication*>(context);

  switch (control) {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
      Terminate();
      service_status_.dwCurrentState = SERVICE_STOP_PENDING;
      break;
    case SERVICE_CONTROL_INTERROGATE:
      break;
    case SERVICE_CONTROL_DEVICEEVENT:
      if (this_ptr) {
        result = this_ptr->HandleDeviceEvent(event_type, event_data);
      }
      break;
  }
  SetServiceStatus(service_status_handle_, &service_status_);
  return result;
}

#if !defined(FUN_NO_WSTRING)
void ServerApplication::ServiceMain(DWORD argc, LPWSTR* argv)
#endif
{
  ServerApplication& app = static_cast<ServerApplication&>(Application::Get());

  app.GetConfig().SetBool("application.run_as_service", true);

#if !defined(FUN_NO_WSTRING)
  service_status_handle_ =
      RegisterServiceCtrlHandlerExW(L"", ServiceControlHandler, &app);
#endif
  if (!service_status_handle_) {
    throw SystemException("cannot register service control handler");
  }

  service_status_.dwServiceType = SERVICE_WIN32;
  service_status_.dwCurrentState = SERVICE_START_PENDING;
  service_status_.dwControlsAccepted =
      SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
  service_status_.dwWin32ExitCode = 0;
  service_status_.dwServiceSpecificExitCode = 0;
  service_status_.dwCheckPoint = 0;
  service_status_.dwWaitHint = 0;
  SetServiceStatus(service_status_handle_, &service_status_);

  try {
#if !defined(FUN_NO_WSTRING)
    Array<String> args;
    for (DWORD i = 0; i < argc; ++i) {
      String arg = WCHAR_TO_UTF8(argv[i]);
      args.Add(arg);
    }
    app.Init(args);
#endif
    service_status_.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(service_status_handle_, &service_status_);
    int32 rc = app.Run();
    service_status_.dwWin32ExitCode = rc ? ERROR_SERVICE_SPECIFIC_ERROR : 0;
    service_status_.dwServiceSpecificExitCode = rc;
  } catch (Exception& e) {
    app.GetLogger().Log(e);
    service_status_.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
    service_status_.dwServiceSpecificExitCode = EXIT_CONFIG;
  } catch (...) {
    app.GetLogger().LogError("fatal error - aborting");
    service_status_.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
    service_status_.dwServiceSpecificExitCode = EXIT_SOFTWARE;
  }
  service_status_.dwCurrentState = SERVICE_STOPPED;
  SetServiceStatus(service_status_handle_, &service_status_);
}

void ServerApplication::WaitForTerminationRequest() {
  SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
  terminate_.Wait();
  terminated_.Set();
}

int32 ServerApplication::Run(int32 argc, char** argv) {
  if (!HasConsole() && IsService()) {
    return 0;
  }

  int32 rc = EXIT_OK;
  try {
    Init(argc, argv);

    switch (action_) {
      case SRV_REGISTER:
        RegisterService();
        rc = EXIT_OK;
        break;
      case SRV_UNREGISTER:
        UnregisterService();
        rc = EXIT_OK;
        break;
      default:
        rc = Run();
    }
  } catch (Exception& e) {
    GetLogger().Log(e);
    rc = EXIT_SOFTWARE;
  }
  return rc;
}

int32 ServerApplication::Run(const Array<String>& args) {
  if (!HasConsole() && IsService()) {
    return 0;
  }

  int32 rc = EXIT_OK;
  try {
    Init(args);

    switch (action_) {
      case SRV_REGISTER:
        RegisterService();
        rc = EXIT_OK;
        break;
      case SRV_UNREGISTER:
        UnregisterService();
        rc = EXIT_OK;
        break;
      default:
        rc = Run();
    }
  } catch (Exception& e) {
    GetLogger().Log(e);
    rc = EXIT_SOFTWARE;
  }
  return rc;
}

#if !defined(FUN_NO_WSTRING)
int32 ServerApplication::Run(int32 argc, wchar_t** argv) {
  if (!HasConsole() && IsService()) {
    return 0;
  }

  int32 rc = EXIT_OK;
  try {
    Init(argc, argv);

    switch (action_) {
      case SRV_REGISTER:
        RegisterService();
        rc = EXIT_OK;
        break;
      case SRV_UNREGISTER:
        UnregisterService();
        rc = EXIT_OK;
        break;
      default:
        rc = Run();
    }
  } catch (Exception& e) {
    GetLogger().Log(e);
    rc = EXIT_SOFTWARE;
  }
  return rc;
}
#endif

bool ServerApplication::IsService() {
#if !defined(FUN_NO_WSTRING)
  wchar_t service_name_buf[2] = L"";

  SERVICE_TABLE_ENTRYW sdt[2];
  sdt[0].lpServiceName = service_name_buf;
  sdt[0].lpServiceProc = ServiceMain;
  sdt[1].lpServiceName = NULL;
  sdt[1].lpServiceProc = NULL;
  return StartServiceCtrlDispatcherW(sdt) != 0;
#endif
}

bool ServerApplication::HasConsole() {
  HANDLE hstdout = GetStdHandle(STD_OUTPUT_HANDLE);
  return hstdout != INVALID_HANDLE_VALUE && hstdout != NULL;
}

void ServerApplication::RegisterService() {
  String name = GetConfig().GetString("application.base_name");
  String path = GetConfig().GetString("application.path");

  WinService service(name);
  if (display_name_.IsEmpty()) {
    service.RegisterService(path);
  } else {
    service.RegisterService(path, display_name_);
  }

  if (startup_ == "auto") {
    service.SetStartup(WinService::SVC_AUTO_START);
  } else if (startup_ == "manual") {
    service.SetStartup(WinService::SVC_MANUAL_START);
  }

  if (!description_.IsEmpty()) {
    service.SetDescription(description_);
  }

  GetLogger().LogInformation(
      "The application has been successfully registered as a service.");
}

void ServerApplication::UnregisterService() {
  String name = GetConfig().GetString("application.baseName");

  WinService service(name);
  service.UnregisterService();

  GetLogger().LogInformation("The service has been successfully unregistered.");
}

void ServerApplication::DefineOptions(OptionSet& options) {
  Application::DefineOptions(options);

  options.AddOption(
      Option("RegisterService", "", "Register the application as a service.")
          .Required(false)
          .Repeatable(false)
          .Callback(OptionCallback<ServerApplication>(
              this, &ServerApplication::HandleRegisterService)));

  options.AddOption(
      Option("UnregisterService", "",
             "Unregister the application as a service.")
          .Required(false)
          .Repeatable(false)
          .Callback(OptionCallback<ServerApplication>(
              this, &ServerApplication::HandleUnregisterService)));

  options.AddOption(Option("displayName", "",
                           "Specify a display name for the service (only with "
                           "/RegisterService).")
                        .Required(false)
                        .Repeatable(false)
                        .Argument("name")
                        .Callback(OptionCallback<ServerApplication>(
                            this, &ServerApplication::HandleDisplayName)));

  options.AddOption(
      Option(
          "description", "",
          "Specify a description for the service (only with /RegisterService).")
          .Required(false)
          .Repeatable(false)
          .Argument("text")
          .Callback(OptionCallback<ServerApplication>(
              this, &ServerApplication::HandleDescription)));

  options.AddOption(Option("startup", "",
                           "Specify the startup mode for the service (only "
                           "with /RegisterService).")
                        .Required(false)
                        .Repeatable(false)
                        .Argument("automatic|manual")
                        .Callback(OptionCallback<ServerApplication>(
                            this, &ServerApplication::HandleStartup)));
}

void ServerApplication::HandleRegisterService(const String& name,
                                              const String& value) {
  action_ = SRV_REGISTER;
}

void ServerApplication::HandleUnregisterService(const String& name,
                                                const String& value) {
  action_ = SRV_UNREGISTER;
}

void ServerApplication::HandleDisplayName(const String& name,
                                          const String& value) {
  display_name_ = value;
}

void ServerApplication::HandleDescription(const String& name,
                                          const String& value) {
  description_ = value;
}

void ServerApplication::HandleStartup(const String& name, const String& value) {
  if (icompare(value, 4, String("auto")) == 0) {
    startup_ = "auto";
  } else if (icompare(value, String("manual")) == 0) {
    startup_ = "manual";
  } else {
    throw InvalidArgumentException(
        "Argument to startup option must be 'auto[matic]' or 'manual'");
  }
}

#else  // _WIN32_WCE

void ServerApplication::WaitForTerminationRequest() { terminate_.Wait(); }

int32 ServerApplication::Run(int32 argc, char** argv) {
  try {
    Init(argc, argv);
  } catch (Exception& e) {
    GetLogger().Log(e);
    return EXIT_CONFIG;
  }

  return Run();
}

int32 ServerApplication::Run(const Array<String>& args) {
  try {
    Init(args);
  } catch (Exception& e) {
    GetLogger().Log(e);
    return EXIT_CONFIG;
  }

  return Run();
}

#if !defined(FUN_NO_WSTRING)
int32 ServerApplication::Run(int32 argc, wchar_t** argv) {
  try {
    Init(argc, argv);
  } catch (Exception& e) {
    GetLogger().Log(e);
    return EXIT_CONFIG;
  }

  return Run();
}
#endif

#endif  // _WIN32_WCE

#elif FUN_PLATFORM != FUN_PLATFORM_VXWORKS

//
// VxWorks specific code
//

void ServerApplication::WaitForTerminationRequest() { terminate_.Wait(); }

int32 ServerApplication::Run(int32 argc, char** argv) {
  try {
    Init(argc, argv);
  } catch (Exception& e) {
    GetLogger().Log(e);
    return EXIT_CONFIG;
  }

  return Run();
}

int32 ServerApplication::Run(const Array<String>& args) {
  try {
    Init(args);
  } catch (Exception& e) {
    GetLogger().Log(e);
    return EXIT_CONFIG;
  }
  return Run();
}

void ServerApplication::DefineOptions(OptionSet& options) {
  Application::DefineOptions(options);
}

#elif FUN_PLATFORM_UNIX_FAMILY

//
// Unix specific code
//

void ServerApplication::WaitForTerminationRequest() {
#if FUN_PLATFORM != FUN_PLATFORM_ANDROID && !defined(__NACL__) && \
    !defined(__EMSCRIPTEN__)
  sigset_t sset;
  sigemptyset(&sset);
  if (!std::getenv("FUN_ENABLE_DEBUGGER")) {
    sigaddset(&sset, SIGINT);
  }

  sigaddset(&sset, SIGQUIT);
  sigaddset(&sset, SIGTERM);
  sigprocmask(SIG_BLOCK, &sset, NULL);
  int32 sig;
  sigwait(&sset, &sig);
#else  // FUN_PLATFORM != FUN_PLATFORM_ANDROID || __NACL__ || __EMSCRIPTEN__
  terminate_.Wait();
#endif
}

int32 ServerApplication::Run(int32 argc, char** argv) {
  bool run_as_daemon = IsDaemon(argc, argv);
  if (run_as_daemon) {
    BeDaemon();
  }

  try {
    Init(argc, argv);

    if (run_as_daemon) {
      int32 rc = chdir("/");
      if (rc != 0) {
        return EXIT_OSERR;
      }
    }
  } catch (Exception& e) {
    GetLogger().Log(e);
    return EXIT_CONFIG;
  }

  return Run();
}

int32 ServerApplication::Run(const Array<String>& args) {
  bool run_as_daemon = false;
  for (const auto& arg : args) {
    if (arg == "--daemon") {
      run_as_daemon = true;
      break;
    }
  }

  if (run_as_daemon) {
    BeDaemon();
  }

  try {
    Init(args);

    if (run_as_daemon) {
      int32 rc = chdir("/");
      if (rc != 0) {
        return EXIT_OSERR;
      }
    }
  } catch (Exception& e) {
    GetLogger().Log(e);
    return EXIT_CONFIG;
  }

  return Run();
}

bool ServerApplication::IsDaemon(int32 argc, char** argv) {
  String option("--daemon");
  for (int32 i = 1; i < argc; ++i) {
    if (option == argv[i]) {
      return true;
    }
  }

  return false;
}

void ServerApplication::BeDaemon() {
#if !defined(FUN_NO_FORK_EXEC)
  pid_t pid;
  if ((pid = fork()) < 0) {
    throw SystemException("cannot fork daemon process");
  } else if (pid != 0) {
    exit(0);
  }

  setsid();
  umask(027);

  // attach stdin, stdout, stderr to /dev/null
  // instead of just closing them. This avoids
  // issues with third party/legacy code writing
  // stuff to stdout/stderr.
  FILE* fin = freopen("/dev/null", "r+", stdin);
  if (!fin) {
    throw OpenFileException("Cannot attach stdin to /dev/null");
  }

  FILE* fout = freopen("/dev/null", "r+", stdout);
  if (!fout) {
    throw OpenFileException("Cannot attach stdout to /dev/null");
  }

  FILE* ferr = freopen("/dev/null", "r+", stderr);
  if (!ferr) {
    throw OpenFileException("Cannot attach stderr to /dev/null");
  }
#else
  throw NotImplementedException("platform does not allow fork/exec");
#endif
}

void ServerApplication::DefineOptions(OptionSet& options) {
  Application::DefineOptions(options);

  options.AddOption(Option("daemon", "", "Run application as a daemon.")
                        .Required(false)
                        .Repeatable(false)
                        .Callback(OptionCallback<ServerApplication>(
                            this, &ServerApplication::HandleDaemon)));

  options.AddOption(
      Option("umask", "", "Set the daemon's umask (octal, e.g. 027).")
          .Required(false)
          .Repeatable(false)
          .Argument("mask")
          .Callback(OptionCallback<ServerApplication>(
              this, &ServerApplication::HandleUMask)));

  options.AddOption(
      Option("pidfile", "",
             "Write the process ID of the application to given file.")
          .Required(false)
          .Repeatable(false)
          .Argument("path")
          .Callback(OptionCallback<ServerApplication>(
              this, &ServerApplication::HandlePidFile)));
}

void ServerApplication::HandleDaemon(const String& name, const String&) {
  GetConfig().SetBool("application.runAsDaemon", true);
}

void ServerApplication::HandleUMask(const String& name, const String& value) {
  int32 mask = 0;
  for (const auto& ch : value) {
    mask *= 8;

    if (ch >= '0' && ch <= '7') {
      mask += ch - '0';
    } else {
      throw InvalidArgumentException("umask contains non-octal characters",
                                     value);
    }
  }

  umask(mask);
}

void ServerApplication::HandlePidFile(const String& name, const String& value) {
  FileOutputStream ostr(value);
  if (ostr.good()) {
    ostr << Process::CurrentPid() << std::endl;
  } else {
    throw CreateFileException("Cannot write PID to file", value);
  }

  TemporaryFile::RegisterForDeletion(value);
}

#endif

}  // namespace framework
}  // namespace fun
