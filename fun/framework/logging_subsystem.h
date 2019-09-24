#pragma once

#include "fun/framework/framework.h"
#include "fun/framework/subsystem.h"

namespace fun {
namespace framework {

/**
 * The LoggingSubsystem class initializes the logging
 * framework using the LoggingConfigurator.
 *
 * It also sets the Application's logger to
 * the logger specified by the "application.logger"
 * property, or to "Application" if the property
 * is not specified.
 */
class FUN_FRAMEWORK_API LoggingSubsystem : public Subsystem {
 public:
  LoggingSubsystem();

  // Subsystem interface
  const char* GetName() const override;

 protected:
  void Initialize(Application& app) override;
  void Uninitialize() override;

  ~LoggingSubsystem();
};

}  // namespace framework
}  // namespace fun
