#pragma once

#include "fun/base/windows_less.h"
#include "fun/framework/framework.h"

#define FUN_LPQUERY_SERVICE_CONFIG LPQUERY_SERVICE_CONFIGW

namespace fun {
namespace framework {

/**
 * This class provides an object-oriented interface to
 * the Windows Service Control Manager for registering,
 * unregistering, configuring, starting and stopping
 * services.
 *
 * This class is only available on Windows platforms.
 */
class FUN_FRAMEWORK_API WinService {
 public:
  enum Startup { SVC_AUTO_START, SVC_MANUAL_START, SVC_DISABLED };

  /**
   * Creates the WinService, using the given service name.
   */
  WinService(const String& name);

  /**
   * Destroys the WinService.
   */
  ~WinService();

  // Disable default constructor and copy.
  WinService() = delete;
  WinService(const WinService&) = delete;
  WinService& operator=(const WinService&) = delete;

  /**
   * Returns the service name.
   */
  const String& GetName() const;

  /**
   * Returns the service's display name.
   */
  String GetDisplayName() const;

  /**
   * Returns the path to the service executable.
   *
   * Throws a NotFoundException if the service has not been registered.
   */
  String GetPath() const;

  /**
   * Creates a Windows service with the executable specified by path
   * and the given display_name.
   *
   * Throws a ExistsException if the service has already been registered.
   */
  void RegisterService(const String& path, const String& display_name);

  /**
   * Creates a Windows service with the executable specified by path
   * and the given display_name. The service name is used as display name.
   *
   * Throws a ExistsException if the service has already been registered.
   */
  void RegisterService(const String& path);

  /**
   * Deletes the Windows service.
   *
   * Throws a NotFoundException if the service has not been registered.
   */
  void UnregisterService();

  /**
   * Returns true if the service has been registered with
   * the Service Control Manager.
   */
  bool IsRegistered() const;

  /**
   * Returns true if the service is currently running.
   */
  bool IsRunning() const;

  /**
   * Starts the service.
   * Does nothing if the service is already running.
   *
   * Throws a NotFoundException if the service has not been registered.
   */
  void Start();

  /**
   * Stops the service.
   * Does nothing if the service is not running.
   *
   * Throws a NotFoundException if the service has not been registered.
   */
  void Stop();

  /**
   * Sets the startup mode for the service.
   */
  void SetStartup(Startup startup);

  /**
   * Returns the startup mode for the service.
   */
  Startup GetStartup() const;

  /**
   * Sets the service description in the registry.
   */
  void SetDescription(const String& description);

  /**
   * Returns the service description from the registry.
   */
  String GetDescription() const;

  static const int32 STARTUP_TIMEOUT;

 protected:
  static const String REGISTRY_KEY;
  static const String REGISTRY_DESCRIPTION;

 private:
  void Open() const;
  bool TryOpen() const;
  void Close() const;
  FUN_LPQUERY_SERVICE_CONFIG GetConfig() const;

  String name_;
  SC_HANDLE scm_handle_;
  mutable SC_HANDLE svc_handle_;
};

}  // namespace framework
}  // namespace fun
