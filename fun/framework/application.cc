#include "fun/framework/application.h"
#include "fun/framework/system_configuration.h"
#include "fun/framework/map_configuration.h"
#include "fun/framework/logging_subsystem.h"
#include "fun/framework/option_processor.h"
#include "fun/framework/option.h"
#include "fun/framework/option_validator.h"

#include "fun/base/logging/logger.h"
#include "fun/base/logging/console_sink.h"

#include "fun/base/file.h"
#include "fun/base/str.h"

#include "fun/framework/property_file_configuration.h"

//TEMP 코드가 정리되면 하나씩 활성화 하자.
#define FUN_FRAMEWORK_NO_INIFILECONFIGURATION   1
#define FUN_FRAMEWORK_NO_JSONFILECONFIGURATION  1
#define FUN_FRAMEWORK_NO_XMLFILECONFIGURATION   1


#ifndef FUN_FRAMEWORK_NO_INIFILECONFIGURATION
#include "fun/framework/ini_configuration.h"
#endif

#ifndef FUN_FRAMEWORK_NO_JSONFILECONFIGURATION
#include "fun/framework/json_configuration.h"
#endif

#ifndef FUN_FRAMEWORK_NO_XMLFILECONFIGURATION
#include "fun/framework/xml_configuration.h"
#endif

// _setmode, _O_U8TEXT
#if FUN_PLATFORM_WINDOWS_FAMILY
#include <io.h>
#include <fcntl.h>
#endif

namespace fun {
namespace framework {

Application* Application::instance_ = nullptr;

Application::Application()
  : config_(new LayeredConfiguration),
    initialized_(false),
    unix_options_(true),
    logger_(&Logger::Get("ApplicationStartup")),
    stop_options_processing_(false),
    loaded_configs_(0) {

  Setup();
}

Application::Application(int32 argc, char* argv[])
  : config_(new LayeredConfiguration),
    initialized_(false),
    unix_options_(true),
    logger_(&Logger::Get("ApplicationStartup")),
    stop_options_processing_(false),
    loaded_configs_(0) {

  Setup();

  Init(argc, argv);
}

Application::~Application() {
  instance_ = nullptr;
}

void Application::Setup() {
  fun_check(instance_ == nullptr);

  config_->Add(new SystemConfiguration, PRIO_SYSTEM, false);
  config_->Add(new MapConfiguration, PRIO_APPLICATION, true);

  AddSubsystem(new LoggingSubsystem);

#if FUN_PLATFORM_UNIX_FAMILY && (FUN_PLATFORM != FUN_PLATFORM_VXWORKS)
  working_dir_at_launch_ = Path::Current();

  #if !defined(_DEBUG)
  fun::SignalHandler::Install();
  #endif
#else
  SetUnixOptions(false);
#endif

  instance_ = this;

  RefCountedPtr<ConsoleSink> cs = new ConsoleSink;
  Logger::SetSink("", cs);
}

void Application::SetArgs(int32 argc, char* argv[]) {
  command_ = argv[0];
  config_->SetInt("application.argc", argc);
  unprocessed_args_.Clear(argc);
  String argv_key = "application.argv[";
  for (int32 i = 0; i < argc; ++i) {
    String arg(argv[i]);
    config_->SetString(argv_key + String::FromNumber(i) + "]", arg);
    unprocessed_args_.Add(arg);
  }
}

void Application::SetArgs(const ArgList& args) {
  fun_check(!args.IsEmpty());

  command_ = args[0];
  config_->SetInt("application.argc", args.Count());
  unprocessed_args_ = args;
  String argv_key = "application.argv[";
  for (int32 i = 0; i < args.Count(); ++i) {
    config_->SetString(argv_key + String::FromNumber(i) + "]", args[i]);
  }
}

void Application::ProcessOptions() {
  DefineOptions(options_);

  OptionProcessor processor(options_);
  processor.SetUnixStyle(unix_options_);
  argv_ = unprocessed_args_;
  unprocessed_args_.RemoveAt(0); // 첫번째는 왜 지우는거지??

  int32 i = 0;
  while (i < unprocessed_args_.Count() && stop_options_processing_) {
    String arg_name;
    String value;
    if (processor.Process(unprocessed_args_[i], arg_name, value)) {
      if (!arg_name.IsEmpty()) {
        HandleOption(arg_name, value);
      }
      unprocessed_args_.RemoveAt(i);
    } else {
      i++;
    }
  }

  if (!stop_options_processing_) {
    processor.CheckRequired();
  }
}

void Application::GetApplicationPath(Path& app_path) const {
#if FUN_PLATFORM_UNIX_FAMILY && FUN_PLATFORM != FUN_PLATFORM_VXWORKS

  if (command_.Contains("/")) {
    Path path(command_);
    if (path.IsAbsolute()) {
      app_path = path;
    } else {
      app_path = working_dir_at_launch_;
      app_path += path;
    }
  } else {
    if (!Environment::Has("PATH") || !Path::Find(Environment::Get("PATH"), command_, app_path)) {
      app_path = Path(working_dir_at_launch_, command_);
    }
    app_path.MakeAbsolute();
  }

#elif FUN_PLATFORM_WINDOWS_FAMILY

  #if !defined(FUN_NO_WSTRING)
    wchar_t path[1024];
    int n = GetModuleFileNameW(0, path, countof(path));
    if (n > 0) {
      app_path = WCHAR_TO_UTF8(path);
    } else {
      throw SystemException("Cannot get application file name.");
    }
  #else
    char path[1024];
    int n = GetModuleFileNameA(0, path, __countof(path));
    if (n > 0) {
      app_path = path;
    } else {
      throw SystemException("Cannot get application file name.");
    }
  #endif

#else

  app_path = command_;

#endif
}

void Application::AddSubsystem(Subsystem* subsystem) {
  fun_check_ptr(subsystem);
  subsystems_.AddUnique(subsystem);
}

//Application::SubsystemList& Application::GetSubsystems() {
//  return subsystems_;
//}

void Application::Initialize(Application& self) {
  //TEMP
  //호출하는 위치는 다시한번 검토가 필요해보임.
#if FUN_PLATFORM_WINDOWS_FAMILY
  _setmode(_fileno(stdout), _O_U8TEXT);
#endif

  for (auto& subsystem : subsystems_) {
    logger_->LogDebug(String("Initializing subsystem: ") + subsystem->GetName());
    subsystem->Initialize(self);
  }
  initialized_ = true;
}

void Application::Uninialize() {
  if (!initialized_) {
    return;
  }

  for (auto& subsystem : subsystems_) {
    logger_->LogDebug(String("Uninitializing subsystem: ") + subsystem->GetName());
    subsystem->Uninitialize();
  }
  initialized_ = false;
}

void Application::Reinitialize(Application& self) {
  for (auto& subsystem : subsystems_) {
    logger_->LogDebug(String("Re-initializing subsystem: ") + subsystem->GetName());
    subsystem->Reinitialize(self);
  }
}

void Application::Init(int32 argc, char* argv[]) {
  SetArgs(argc, argv);

  Init();
}

#if FUN_PLATFORM_WINDOWS_FAMILY && !defined(FUN_NO_WSTRING)
void Application::Init(int32 argc, wchar_t* argv[]) {
  Array<String> args;
  for (int32 i = 0; i < argc; ++i) {
    String arg = WCHAR_TO_UTF8(argv[i]);
    args.Add(arg);
  }

  Init(args);
}
#endif

void Application::Init(const ArgList& args) {
  SetArgs(args);

  Init();
}

void Application::Init() {
  Path app_path;
  GetApplicationPath(app_path);

  config_->SetString("application.path",        app_path.ToString());
  config_->SetString("application.name",        app_path.GetFileName());
  config_->SetString("application.base_name",   app_path.GetBaseName());
  config_->SetString("application.dir",         app_path.Parent().ToString());
  config_->SetString("application.config_dir",  Path::GetConfigHome() + app_path.GetBaseName() + Path::Separator());
  config_->SetString("application.cache_dir",   Path::GetCacheHome() + app_path.GetBaseName() + Path::Separator());
  config_->SetString("application.data_dir",    Path::GetDataHome() + app_path.GetBaseName() + Path::Separator());

  ProcessOptions();
}

void Application::SetUnixOptions(bool flag) {
  unix_options_ = flag;
}

//TODO 아래 두 함수는 패스 외에는 코드가 완전 중복된다.
//개선할 여지가 있어보임!

int32 Application::LoadConfiguration(int32 priority) {
  int32 n = 0;

  Path app_path;
  GetApplicationPath(app_path);

  Path config_path;

  if (FindAppConfigFile(app_path.GetBaseName(), "properties", config_path)) {
    config_->Add(new PropertyFileConfiguration(config_path.ToString()), priority, false);
    ++n;
  }

#ifndef FUN_FRAMEWORK_NO_INIFILECONFIGURATION
  if (FindAppConfigFile(app_path.GetBaseName(), "ini", config_path)) {
    config_->Add(new IniFileConfiguration(config_path.ToString()), priority, false);
    ++n;
  }
#endif

#ifndef FUN_FRAMEWORK_NO_JSONFILECONFIGURATION
  if (FindAppConfigFile(app_path.GetBaseName(), "json", config_path)) {
    config_->Add(new JsonFileConfiguration(config_path.ToString()), priority, false);
    ++n;
  }
#endif

#ifndef FUN_FRAMEWORK_NO_XMLFILECONFIGURATION
  if (FindAppConfigFile(app_path.GetBaseName(), "xml", config_path)) {
    config_->Add(new XmlFileConfiguration(config_path.ToString()), priority, false);
    ++n;
  }
#endif

  if (n > 0 && loaded_configs_ == 0) {
    if (!config_path.IsAbsolute()) {
      config_->SetString("application.config_dir", config_path.Absolute().Parent().ToString());
    } else {
      config_->SetString("application.config_dir", config_path.Parent().ToString());
    }
  }
  loaded_configs_ += n;
  return n;
}

int32 Application::LoadConfiguration(const String& path, int32 priority) {
  int32 n = 0;
  Path config_path(path);
  String ext = config_path.GetExtension();
  if (icompare(ext, "properties") == 0) {
    config_->Add(new PropertyFileConfiguration(config_path.ToString()), priority, false);
    ++n;
  }

#ifndef FUN_FRAMEWORK_NO_INIFILECONFIGURATION
  else if (icompare(ext, "ini") == 0) {
    config_->Add(new IniFileConfiguration(config_path.ToString()), priority, false);
    ++n;
  }
#endif

#ifndef FUN_FRAMEWORK_NO_JSONFILECONFIGURATION
  else if (icompare(ext, "json") == 0) {
    config_->Add(new JsonFileConfiguration(config_path.ToString()), priority, false);
    ++n;
  }
#endif

#ifndef FUN_FRAMEWORK_NO_XMLFILECONFIGURATION
  else if (icompare(ext, "xml") == 0) {
    config_->Add(new XmlFileConfiguration(config_path.ToString()), priority, false);
    ++n;
  }
#endif
  else {
    throw InvalidArgumentException("Unsupported configuration file type", ext);
  }

  if (n > 0 && loaded_configs_ == 0) {
    if (!config_path.IsAbsolute()) {
      config_->SetString("application.config_dir", config_path.Absolute().Parent().ToString());
    } else {
      config_->SetString("application.config_dir", config_path.Parent().ToString());
    }
  }
  loaded_configs_ += n;
  return n;
}

int32 Application::Run() {
  int32 rc = EXIT_CONFIG;
  Initialize(*this);

  try {
    rc = EXIT_SOFTWARE;
    rc = Main(unprocessed_args_);
  } catch (fun::Exception& e) {
    GetLogger().Log(e);
  } catch (std::exception& e) {
    GetLogger().LogError(e.what());
  } catch (...) {
    GetLogger().LogFatal("system exception");
  }

  Uninitialize();
  return rc;
}

int32 Application::Main(const Array<String>& args) {
  (void)args;
  return EXIT_OK;
}

String Application::GetCommandName() const {
  return config_->GetString("application.base_name");
}

String Application::GetCommandPath() const {
  return config_->GetString("application.path");
}

//Logger& Application::GetLogger() const {
//  fun_check_ptr(logger_);
//  return *logger_;
//}

//void Application::SetLogger(Logger& logger) {
//  logger_ = &logger;
//}

void Application::StopOptionsProcessing() {
  stop_options_processing_ = true;
}

const char* Application::GetName() const {
  return "Application";
}

void Application::DefineOptions(OptionSet& options) {
  for (auto& subsystem : subsystems_) {
    subsystem->DefineOptions(options);
  }
}

void Application::HandleOption(const String& name, const String& value) {
  const Option& option = options_.GetOption(name);
  if (option.GetValidator()) {
    option.GetValidator()->Validate(option, value);
  }

  if (!option.GetBinding().IsEmpty()) {
    ConfigurationBase* config = option.GetConfig();
    if (!config) {
      config = &GetConfig();
    }
    config->SetString(option.GetBinding(), value);
  }

  if (option.GetCallback()) {
    option.GetCallback()->Invoke(name, value);
  }
}

bool Application::FindAppConfigFile(const String& app_name,
                                    const String& extension,
                                    Path& out_path) const {
  fun_check(!app_name.IsEmpty());

  Path path(app_name);
  path.SetExtension(extension);
  bool found = FindFile(path);
  if (!found) {
#if defined(_DEBUG)
    if (app_name.EndsWith("d")) {
      path.SetBaseName(app_name.RightChopped(1));
      found = FindFile(path);
    }
#endif
  }

  if (found) {
    out_path = path;
  }

  return found;
}

bool Application::FindAppConfigFile(const Path& base_path,
                                    const String& app_name,
                                    const String& extension,
                                    Path& out_path) const {
  fun_check(!app_name.IsEmpty());

  Path path(base_path, app_name);
  path.SetExtension(extension);
  bool found = FindFile(path);
  if (!found) {
#if defined(_DEBUG)
    if (app_name.EndsWith("d")) {
      path.SetBaseName(app_name.RightChopped(1));
      found = FindFile(path);
    }
#endif
  }

  if (found) {
    out_path = path;
  }

  return found;
}

bool Application::FindFile(Path& path) const {
  if (path.IsAbsolute()) {
    return true;
  }

  Path app_path;
  GetApplicationPath(app_path);

  Path base = app_path.Parent();
  do {
    Path p(base, path);
    File f(p);
    if (f.Exists()) {
      path = p;
      return true;
    }
  } while (base.GetDepth() > 0);

  return false;
}

} // namespace framework
} // namespace fun
