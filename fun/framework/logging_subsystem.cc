#include "fun/framework/logging_subsystem.h"

#include "fun/base/logging/logger.h"
#include "fun/framework/application.h"
#include "fun/framework/logging_configurator.h"

using fun::Logger;

namespace fun {
namespace framework {

LoggingSubsystem::LoggingSubsystem() {}

LoggingSubsystem::~LoggingSubsystem() {}

const char* LoggingSubsystem::GetName() const { return "Logging Subsystem"; }

void LoggingSubsystem::Initialize(Application& app) {
  LoggingConfigurator configurator;
  configurator.Configure(app.GetConfigPtr());
  String logger =
      app.GetConfig().GetString("application.logger", "Application");
  app.SetLogger(Logger::Get(logger));
}

void LoggingSubsystem::Uninitialize() {}

}  // namespace framework
}  // namespace fun
