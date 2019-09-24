#pragma once

#include "fun/framework/framework.h"
#include "fun/framework/application.h"
#include "fun/base/event.h"

#if FUN_PLATFORM_WINDOWS_FAMILY
#include "fun/base/named_event.h"
#endif

namespace fun {
namespace framework {

/**
 * A subclass of the Application class that is used
 * for implementing server applications.
 *
 * A ServerApplication allows for the application
 * to run as a Windows service or as a Unix daemon
 * without the need to add extra code.
 *
 * For a ServerApplication to work both from the command line
 * and as a daemon or service, a few rules must be met:
 *   - Subsystems must be registered in the constructor.
 *   - All non-trivial initializations must be made in the
 *     Initialize() method.
 *   - At the end of the Main() method, WaitForTerminationRequest()
 *     should be called.
 *   - New threads must only be created in Initialize() or Main() or
 *     methods called from there, but not in the application class'
 *     constructor or in the constructor of instance variables.
 *     The reason for this is that fork() will be called in order to
 *     create the daemon process, and threads created prior to calling
 *     fork() won't be taken over to the daemon process.
 *   - The Main(argc, argv) function must look as follows:
 *
 *   int main(int argc, char** argv) {
 *     MyServerApplication app;
 *     return app.Run(argc, argv);
 *   }
 *
 * The FUN_SERVER_MAIN macro can be used to implement Main(argc, argv).
 * FUN_SERVER_MAIN supports Unicode command line arguments.
 *
 * On Windows platforms, an application built on top of the
 * ServerApplication class can be run both from the command line
 * or as a service.
 *
 * To run an application as a Windows service, it must be registered
 * with the Windows Service Control Manager (SCM). To do this, the application
 * can be started from the command line, with the /RegisterService option
 * specified. This causes the application to register itself with the
 * SCM, and then exit. Similarly, an application registered as a service can
 * be unregistered, by specifying the /unregisterService option.
 * The file name of the application executable (excluding the .exe suffix)
 * is used as the service name. Additionally, a more user-friendly name can be
 * specified, using the /displayName option (e.g., /displayName="Demo Service")
 * and a service description can be added with the /description option.
 * The startup mode (automatic or manual) for the service can be specified
 * with the /startup option.
 *
 * An application can determine whether it is running as a service by checking
 * for the "application.run_as_service" configuration property.
 *
 *   if (GetConfig().GetBool("application.run_as_service", false)) {
 *     // do service specific things
 *   }
 *
 * Note that the working directory for an application running as a service
 * is the Windows system directory (e.g., C:\Windows\system32). Take this
 * into account when working with relative filesystem paths. Also, services
 * run under a different user account, so an application that works when
 * started from the command line may fail to run as a service if it depends
 * on a certain environment (e.g., the PATH environment variable).
 *
 * An application registered as a Windows service can be started
 * with the NET START <name> command and stopped with the NET STOP <name>
 * command. Alternatively, the Services MMC applet can be used.
 *
 * On Unix platforms, an application built on top of the ServerApplication
 * class can be optionally run as a daemon by giving the --daemon
 * command line option. A daemon, when launched, immediately
 * forks off a background process that does the actual work. After launching
 * the background process, the foreground process exits.
 *
 * After the initialization is complete, but before entering the Main() method,
 * the current working directory for the daemon process is changed to the root
 * directory ("/"), as it is common practice for daemon processes. Therefore, be
 * careful when working with files, as relative paths may not point to where
 * you expect them point to.
 *
 * An application can determine whether it is running as a daemon by checking
 * for the "application.runAsDaemon" configuration property.
 *
 *   if (GetConfig().GetBool("application.run_as_daemon", false)) {
 *     // do daemon specific things
 *   }
 *
 * When running as a daemon, specifying the --pidfile option (e.g.,
 * --pidfile=/var/run/sample.pid) may be useful to record the process ID of
 * the daemon in a file. The PID file will be removed when the daemon process
 * terminates (but not, if it crashes).
 */
class FUN_FRAMEWORK_API ServerApplication : public Application {
 public:
  /**
   * Creates the ServerApplication.
   */
  ServerApplication();

  /**
   * Destroys the ServerApplication.
   */
  ~ServerApplication();

  /**
   * Returns true if the application runs from the command line.
   * Returns false if the application runs as a Unix daemon
   * or Windows service.
   */
  bool IsInteractive() const;

  /**
   * Runs the application by performing additional initializations
   * and calling the Main() method.
   */
  int32 Run(int32 argc, char** argv);

  /**
   * Runs the application by performing additional initializations
   * and calling the Main() method.
   */
  int32 Run(const Array<String>& args);

#if !defined(FUN_NO_WSTRING)
  /**
   * Runs the application by performing additional initializations
   * and calling the Main() method.
   *
   * This Windows-specific version of init is used for passing
   * Unicode command line arguments from wmain().
   */
  int32 Run(int32 argc, wchar_t** argv);
#endif

  /**
   * Sends a friendly termination request to the application.
   * If the application's main thread is waiting in
   * WaitForTerminationRequest(), this method will return
   * and the application can shut down.
   */
  static void Terminate();

 protected:
  int32 Run();

  virtual void WaitForTerminationRequest();

#if !defined(_WIN32_WCE)
  void DefineOptions(OptionSet& options);
#endif

#if FUN_PLATFORM_WINDOWS_FAMILY && !defined(_WIN32_WCE)
  /**
   * Registers the ServerApplication to receive SERVICE_CONTROL_DEVICEEVENT
   * events via HandleDeviceEvent().
   */
  static HDEVNOTIFY RegisterServiceDeviceNotification(LPVOID filter, DWORD flags);

  /**
   * Handles the SERVICE_CONTROL_DEVICEEVENT event. The default
   * implementation does nothing and returns ERROR_CALL_NOT_IMPLEMENTED.
   */
  virtual DWORD HandleDeviceEvent(DWORD event_type, LPVOID event_data);
#endif

 private:
#if FUN_PLATFORM == FUN_PLATFORM_VXWORKS

  static fun::Event terminate_;

#elif FUN_PLATFORM_UNIX_FAMILY

  void HandleDaemon(const String& name, const String& value);
  void HandleUMask(const String& name, const String& value);
  void HandlePidFile(const String& name, const String& value);
  bool IsDaemon(int32 argc, char** argv);
  void BeDaemon();

#if FUN_PLATFORM == FUN_PLATFORM_ANDROID || defined(__NACL__) || defined(__EMSCRIPTEN__)
  static Event terminate_;
#endif

#elif FUN_PLATFORM_WINDOWS_FAMILY

#if !defined(_WIN32_WCE)
  enum Action {
    SRV_RUN,
    SRV_REGISTER,
    SRV_UNREGISTER
  };
  static BOOL __stdcall ConsoleCtrlHandler(DWORD ctrlType);
  static DWORD __stdcall ServiceControlHandler( DWORD control,
                                                DWORD event_type,
                                                LPVOID event_data,
                                                LPVOID context);

#if !defined(FUN_NO_WSTRING)
  static void __stdcall ServiceMain(DWORD argc, LPWSTR* argv);
#endif

  bool HasConsole();
  bool IsService();
  void BeService();
  void RegisterService();
  void UnregisterService();
  void HandleRegisterService(const String& name, const String& value);
  void HandleUnregisterService(const String& name, const String& value);
  void HandleDisplayName(const String& name, const String& value);
  void HandleDescription(const String& name, const String& value);
  void HandleStartup(const String& name, const String& value);

  Action action_;
  String display_name_;
  String description_;
  String startup_;

  static Event terminated_;
  static SERVICE_STATUS service_status_;
  static SERVICE_STATUS_HANDLE service_status_handle_;

#endif // _WIN32_WCE

  static fun::NamedEvent terminate_;

#endif // defined(FUN_PLATFORM_WINDOWS_FAMILY)
};

} // namespace framework
} // namespace fun


//
// Macro to implement main()
//

#if FUN_PLATFORM_WINDOWS_FAMILY && !defined(FUN_NO_WSTRING)

  #define FUN_SERVER_MAIN(App) \
    int32 wmain(int32 argc, wchar_t** argv) { \
      try { \
        App app; \
        return app.Run(argc, argv); \
      } catch (fun::Exception& e) { \
        std::cerr << e.GetDisplayText().c_str() << std::endl; \
        return fun::framework::Application::EXIT_SOFTWARE; \
      } \
    } \
    FUN_WMAIN_WRAPPER()

#elif defined(FUN_PLATFORM_VXWORKS)

  #define FUN_SERVER_MAIN(App) \
    int32 RedSrvMain(const char* app_name, ...) { \
      fun::Array<String> args; \
      args.Add(String(app_name)); \
      va_list vargs; \
      va_start(vargs, app_name); \
      const char* arg = va_arg(vargs, const char*); \
      while (arg) { \
        args.Add(String(arg)); \
        arg = va_arg(vargs, const char*); \
      } \
      va_end(vargs); \
      try { \
        App app; \
        return app.Run(args); \
      } catch (fun::Exception& e) { \
        std::cerr << e.GetDisplayText().c_str() << std::endl; \
        return fun::framework::Application::EXIT_SOFTWARE; \
      } \
    }

#else

  #define FUN_SERVER_MAIN(App) \
    int32 main(int32 argc, char** argv) { \
      try { \
        App app; \
        return app.Run(argc, argv); \
      } catch (fun::Exception& e) { \
        std::cerr << e.GetDisplayText().c_str() << std::endl; \
        return fun::framework::Application::EXIT_SOFTWARE; \
      } \
    }

#endif
