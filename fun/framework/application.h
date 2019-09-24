#pragma once

#include "fun/framework/framework.h"
#include "fun/framework/subsystem.h"
#include "fun/framework/layered_configuration.h"
#include "fun/framework/option_set.h"
#include "fun/base/timestamp.h"
#include "fun/base/timespan.h"
#include "fun/base/ref_counted.h"
#include "fun/base/ref_counted_ptr.h"
#include "fun/base/logging/logger.h"
#include "fun/base/path.h"

namespace fun {
namespace framework {

class OptionSet;

/**
 * The Application class implements the main subsystem
 * in a process. The application class is responsible for
 * initializing all its subsystems.
 *
 * Subclasses can and should override the following virtual methods:
 *   - Initialize() (the one-argument, protected variant)
 *   - Uninitialize()
 *   - Reinitialize()
 *   - DefineOptions()
 *   - HandleOption()
 *   - Main()
 *
 * The application's main logic should be implemented in
 * the Main() method.
 *
 * There may be at most one instance of the Application class
 * in a process.
 *
 * The Application class maintains a LayeredConfiguration (available
 * via the GetConfig() member function) consisting of:
 *   - a MapConfiguration (priority -100) storing application-specific
 *     properties, as well as properties from bound command line arguments.
 *   - a SystemConfiguration (priority 100)
 *   - the configurations loaded with LoadConfiguration().
 *
 * The Application class sets a few default properties in
 * its configuration. These are:
 *   - application.path: the absolute path to application executable
 *   - application.name: the file name of the application executable
 *   - application.base_name: the file name (excluding extension) of the application executable
 *   - application.dir: the path to the directory where the application executable resides
 *   - application.config_dir: the path to the directory where user specific configuration files of the application should be stored.
 *   - application.cache_dir: the path to the directory where user specific non-essential data files of the application should be stored.
 *   - application.data_dir: the path to the directory where user specific data files of the application should be stored.
 *   - application.temp_dir: the path to the directory where user specific temporary files and other file objects of the application should be stored.
 *
 * If LoadConfiguration() has never been called, application.config_dir will be equal to application.dir.
 *
 * The FUN_APP_MAIN macro can be used to implement Main(argc, argv).
 * FUN_APP_MAIN supports Unicode command line arguments.
 */
class FUN_FRAMEWORK_API Application : public Subsystem {
 public:
  using ArgList = Array<String>;
  using SubsystemPtr = RefCountedPtr<Subsystem>;
  using SubsystemList = Array<SubsystemPtr>;

  /**
   * Commonly used exit status codes.
   * Based on the definitions in the 4.3BSD <sysexits.h> header file.
   */
  enum ExitCode {
    EXIT_OK          = 0,  /// successful termination
    EXIT_USAGE       = 64, /// command line usage error
    EXIT_DATAERR     = 65, /// data format error
    EXIT_NOINPUT     = 66, /// cannot open input
    EXIT_NOUSER      = 67, /// addressee unknown
    EXIT_NOHOST      = 68, /// host name unknown
    EXIT_UNAVAILABLE = 69, /// service unavailable
    EXIT_SOFTWARE    = 70, /// internal software error
    EXIT_OSERR       = 71, /// system error (e.g., can't fork)
    EXIT_OSFILE      = 72, /// critical OS file missing
    EXIT_CANTCREAT   = 73, /// can't create (user) output file
    EXIT_IOERR       = 74, /// input/output error
    EXIT_TEMPFAIL    = 75, /// temp failure; user is invited to retry
    EXIT_PROTOCOL    = 76, /// remote error in protocol
    EXIT_NOPERM      = 77, /// permission denied
    EXIT_CONFIG      = 78  /// configuration error
  };

  enum ConfigPriority {
    PRIO_APPLICATION = -100,
    PRIO_DEFAULT = 0,
    PRIO_SYSTEM = 100
  };

 public:
  /** Creates the Application. */
  Application();

  /** Creates the Application and calls Init(argc, argv). */
  Application(int32 argc, char* argv[]);

  /**
   * Adds a new subsystem to the application. The
   * application immediately takes ownership of it, so that a
   * call in the form
   *     Application::instance().AddSubsystem(new MySubsystem);
   * is okay.
   */
  void AddSubsystem(Subsystem* subsystem);

  /**
   * Processes the application's command line arguments
   * and sets the application's properties (e.g.,
   * "application.path", "application.name", etc.).
   *
   * Note that as of release 1.3.7, Init() no longer
   * calls Initialize(). This is now called from Run().
   */
  void Init(int32 argc, char* argv[]);

#if FUN_PLATFORM_WINDOWS_FAMILY && !defined(FUN_NO_WSTRING)
  /**
   * Processes the application's command line arguments
   * and sets the application's properties (e.g.,
   * "application.path", "application.name", etc.).
   *
   * Note that as of release 1.3.7, Init() no longer
   * calls Initialize(). This is now called from Run().
   *
   * This Windows-specific version of Init is used for passing
   * Unicode command line arguments from wmain().
   */
  void Init(int32 argc, wchar_t* argv[]);
#endif

  /**
   * Processes the application's command line arguments
   * and sets the application's properties (e.g.,
   * "application.path", "application.name", etc.).
   *
   * Note that as of release 1.3.7, Init() no longer
   * calls Initialize(). This is now called from Run().
   */
  void Init(const ArgList& args);

  /**
   * Returns true if the application is in initialized state
   * (that means, has been initialized but not yet Uninitialized).
   */
  bool IsInitialized() const;

  /**
   * Specify whether command line option handling is Unix-style
   * (flag == true; default) or Windows/OpenVMS-style (flag == false).
   *
   * This member function should be called from the constructor of
   * a subclass to be effective.
   */
  void SetUnixOptions(bool flag);

  /**
   * Loads configuration information from a default location.
   *
   * The configuration(s) will be added to the application's
   * LayeredConfiguration with the given priority.
   *
   * The configuration file(s) must be located in the same directory
   * as the executable or a parent directory of it, and must have the
   * same base name as the executable, with one of the following extensions:
   * .properties, .ini or .xml.
   *
   * The .properties file, if it exists, is loaded first, followed
   * by the .ini file and the .xml file.
   *
   * If the application is built in debug mode (the _DEBUG preprocessor
   * macro is defined) and the base name of the application executable
   * ends with a 'd', a config file without the 'd' ending its base name is
   * also found.
   *
   * Example: Given the application "SampleAppd.exe", built in debug mode.
   * Then LoadConfiguration() will automatically Find a configuration file
   * named "SampleApp.properties" if it exists and if "SampleAppd.properties"
   * cannot be found.
   *
   * Returns the number of configuration files loaded, which may be zero.
   *
   * This method must not be called before Init(argc, argv)
   * has been called.
   */
  int32 LoadConfiguration(int32 priority = PRIO_DEFAULT);

  /**
   * Loads configuration information from the file specified by
   * the given path. The file type is determined by the file
   * extension. The following extensions are supported:
   *   - .properties - properties file (PropertyFileConfiguration)
   *   - .ini        - initialization file (IniFileConfiguration)
   *   - .xml        - XML file (XmlFileConfiguration)
   *   - .json       - JSON file (JsonFileConfiguration)
   *
   * Extensions are not case sensitive.
   *
   * The configuration will be added to the application's
   * LayeredConfiguration with the given priority.
   */
  int32 LoadConfiguration(const String& path, int32 priority = PRIO_DEFAULT);

  /**
   * Returns a reference to the subsystem of the class
   * given as template argument.
   *
   * Throws a NotFoundException if such a subsystem has
   * not been registered.
   */
  template <typename T>
  T& GetSubsystem() const;

  /** Returns a reference to the subsystem list */
  SubsystemList& GetSubsystems();

  /**
   * Runs the application by performing additional (un)initializations
   * and calling the Main() method.
   *
   * First calls Initialize(), then calls Main(), and
   * finally calls Uninitialize(). The latter will be called
   * even if Main() throws an exception. If Initialize() throws
   * an exception, Main() will not be called and the exception
   * will be propagated to the caller. If Uninitialize() throws
   * an exception, the exception will be propagated to the caller.
   */
  virtual int32 Run();

  /** Returns the command name used to invoke the application. */
  String GetCommandName() const;
  /** Returns the full command path used to invoke the application. */
  String GetCommandPath() const;
  /** Returns the application's configuration reference. */
  LayeredConfiguration& GetConfig() const;
  /** Returns the application's configuration smart pointer. */
  LayeredConfiguration::Ptr GetConfigPtr() const;

  /**
   * Returns the application's logger.
   *
   * Before the logging subsystem has been initialized, the
   * application's logger is "ApplicationStartup", which is
   * connected to a ConsoleSink.
   *
   * After the logging subsystem has been initialized, which
   * usually happens as the first action in Application::Initialize(),
   * the application's logger is the one specified by the
   * "application.logger" configuration property. If that property
   * is not specified, the logger is "Application".
   */
  Logger& GetLogger() const;

  /**
   * Returns reference to vector of the application's arguments as
   * specified on the command line. If user overrides the
   * Application::Main(const ArgList&) function, it will receive
   * only the command line parameters that were not processed in
   * Application::ProcessOptons(). This function returns the
   * full set of command line parameters as received in
   * main(argc, argv*).
   */
  const ArgList& GetArgv() const;

  /** Returns the application's option set. */
  const OptionSet& GetOptions() const;

  /** Returns a reference to the Application singleton. */
  static Application& Get();

  /** Returns the application start time (UTC). */
  const Timestamp& GetStartTime() const;

  /** Returns the application uptime. */
  Timespan GetUptime() const;

  /**
   * If called from an option callback, stops all further
   * options processing.
   *
   * If called, the following options on the command line
   * will not be processed, and required options will not
   * be checked.
   *
   * This is useful, for example, if an option for displaying
   * help information has been encountered and no other things
   * besides displaying help shall be done.
   */
  void StopOptionsProcessing();

  /** Gets the application name. */
  const char* GetName() const;

 protected:
  /**
   * Initializes the application and all registered subsystems.
   * Subsystems are always initialized in the exact same order
   * in which they have been registered.
   *
   * Overriding implementations must call the base class implementation.
   */
  void Initialize(Application& self);

  /**
   * Uninitializes the application and all registered subsystems.
   * Subsystems are always uninitialized in reverse order in which
   * they have been initialized.
   *
   * Overriding implementations must call the base class implementation.
   */
  void Uninialize();

  /**
   * Re-nitializes the application and all registered subsystems.
   * Subsystems are always reinitialized in the exact same order
   * in which they have been registered.
   *
   * Overriding implementations must call the base class implementation.
   */
  void Reinitialize(Application& self);

  /**
   * Called before command line processing begins.
   * If a subclass wants to support command line arguments,
   * it must override this method.
   * The default implementation does not define any options itself,
   * but calls DefineOptions() on all registered subsystems.
   *
   * Overriding implementations should call the base class implementation.
   */
  virtual void DefineOptions(OptionSet& options);

  /**
   * Called when the option with the given name is encountered
   * during command line arguments processing.
   *
   * The default implementation does option validation, bindings
   * and callback handling.
   *
   * Overriding implementations must call the base class implementation.
   */
  virtual void HandleOption(const String& name, const String& value);

  /**
   * Sets the logger used by the application.
   */
  void SetLogger(Logger& logger);

  /**
   * The application's main logic.
   *
   * Unprocessed command line arguments are passed in args.
   * Note that all original command line arguments are available
   * via the properties application.argc and application.argv[<n>].
   *
   * Returns an exit code which should be one of the values
   * from the ExitCode enumeration.
   */
  virtual int32 Main(const Array<String>& args);

  /**
   * Searches for the file in path in the application directory.
   *
   * If path is absolute, the method immediately returns true and
   * leaves path unchanged.
   *
   * If path is relative, searches for the file in the application
   * directory and in all subsequent parent directories.
   * Returns true and stores the absolute path to the file in
   * path if the file could be found. Returns false and leaves path
   * unchanged otherwise.
   */
  bool FindFile(Path& path) const; // reference 타입임에 주의!!

  /**
   * Common initialization code.
   */
  void Init();

  /**
   * Destroys the Application and deletes all registered subsystems.
   */
  ~Application();

 private:
  void Setup();
  void SetArgs(int32 argc, char* argv[]);
  void SetArgs(const ArgList& args);
  void GetApplicationPath(Path& app_path) const;
  void ProcessOptions();

  bool FindAppConfigFile( const String& app_name,
                          const String& extension,
                          Path& path) const;

  bool FindAppConfigFile( const Path& base_path,
                          const String& app_name,
                          const String& extension,
                          Path& path) const;

  using ConfigPtr = LayeredConfiguration::Ptr;
  using LoggerPtr = Logger::Ptr;

  ConfigPtr config_;
  SubsystemList subsystems_;
  bool initialized_;
  String command_;
  ArgList argv_;
  ArgList unprocessed_args_;
  OptionSet options_;
  bool unix_options_;
  Logger* logger_;
  Timestamp start_time_;
  bool stop_options_processing_;
  int32 loaded_configs_;

#if FUN_PLATFORM_UNIX_FAMILY
  String working_dir_at_launch_;
#endif

  static Application* instance_;

  friend class LoggingSubsystem;

  Application(const Application&) = delete;
  Application& operator = (const Application&) = delete;
};


//
// inlines
//

template <typename T>
FUN_ALWAYS_INLINE T& Application::GetSubsystem() const {
  for (auto& it : subsystems_) {
    //TODO dynmaic cast를 통해서만 가능한가??
    const Subsystem* subsystem = it.Get();
    const T* casted = dynamic_cast<T*>(subsystem);
    if (casted) {
      return *const_cast<T*>(casted);
    }
  }

  throw NotFoundException("The subsystem has not been registered", typeid(T).name());
}

FUN_ALWAYS_INLINE Application::SubsystemList& Application::GetSubsystems() {
  return subsystems_;
}

bool Application::IsInitialized() const {
  return initialized_;
}

FUN_ALWAYS_INLINE LayeredConfiguration& Application::GetConfig() const {
  fun_check(config_.IsValid());
  return *static_cast<LayeredConfiguration*>(config_.Get());
}

FUN_ALWAYS_INLINE LayeredConfiguration::Ptr Application::GetConfigPtr() const {
  return config_;
}

FUN_ALWAYS_INLINE Logger& Application::GetLogger() const {
  fun_check_ptr(logger_);
  return *logger_;
}

FUN_ALWAYS_INLINE Application& Application::Get() {
  fun_check_ptr(instance_);
  return *instance_;
}

FUN_ALWAYS_INLINE const Application::ArgList& Application::GetArgv() const {
  return argv_;
}

FUN_ALWAYS_INLINE const OptionSet& Application::GetOptions() const {
  return options_;
}

FUN_ALWAYS_INLINE const Timestamp& Application::GetStartTime() const {
  return start_time_;
}

FUN_ALWAYS_INLINE Timespan Application::GetUptime() const {
  Timestamp now;
  return now - start_time_;
}

} // namespace framework
} // namespace fun


//
// Macro to implement main()
//

#if FUN_PLATFORM_WINDOWS_FAMILY && !defined(FUN_NO_WSTRING)

  #define FUN_APP_MAIN(App) \
    int wmain(int argc, wchar_t** argv) { \
      fun::RefCountedPtr<App> app = new App; \
      try { \
        app->Init(argc, argv); \
      } catch (fun::Exception& e) { \
        app->GetLogger().Log(e); \
        return fun::framework::Application::EXIT_CONFIG;\
      } \
      \
      return app->Run(); \
    } \
    FUN_WMAIN_WRAPPER()

#elif defined(FUN_PLATFORM_VXWORKS)

  #define FUN_APP_MAIN(App) \
    int RedAppMain(const char* app_name, ...) { \
      fun::Array<String> args; \
      args.Add(String(app_name)); \
      \
      va_list vargs; \
      va_start(vargs, app_name); \
      const char* arg = va_arg(vargs, const char*); \
      while (arg) { \
        args.Add(String(arg)); \
        arg = va_arg(vargs, const char*); \
      } \
      va_end(vargs); \
      \
      fun::RefCountedPtr<App> app = new App; \
      try { \
        app->Init(args); \
      } catch (fun::Exception& e) { \
        app->GetLogger().Log(e); \
        return fun::framework::Application::EXIT_CONFIG; \
      } \
      \
      return app->Run(); \
    }

#else

  #define FUN_APP_MAIN(App) \
    int main(int argc, char** argv) { \
      fun::RefCountedPtr<App> app = new App; \
      try { \
        app->Init(argc, argv); \
      } catch (fun::Exception& e) { \
        app->GetLogger().Log(e); \
        return fun::framework::Application::EXIT_CONFIG; \
      } \
      \
      return app->Run(); \
    }

#endif
