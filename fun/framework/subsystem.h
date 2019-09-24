#pragma once

#include "fun/framework/framework.h"
#include "fun/base/ref_counted.h"

namespace fun {
namespace framework {

class Application;
class OptionSet;

/**
 * Subsystems extend an application in a modular way.
 *
 * The Subsystem class provides a common interface
 * for subsystems so that subsystems can be automatically
 * initialized at startup and uninitialized at shutdown.
 *
 * Subsystems should also support dynamic reconfiguration,
 * so that they can be reconfigured anytime during the
 * life of a running application.
 *
 * The degree to which dynamic reconfiguration is supported
 * is up to the actual subsystem implementation. It can
 * range from ignoring the reconfiguration request (not
 * recommended), to changing certain settings that affect
 * the performance, to a complete reinitialization.
 */
class FUN_FRAMEWORK_API Subsystem : public RefCountedObject {
 public:
  using Ptr = RefCountedPtr<Subsystem>;

  /** Creates the Subsystem. */
  Subsystem();

  /**
   * Returns the name of the subsystem.
   * Must be implemented by subclasses.
   */
  virtual const char* GetName() const = 0;

 protected:
  friend class Application;

  /** Destroys the Subsystem. */
  virtual ~Subsystem();

  /** Initializes the subsystem. */
  virtual void Initialize(Application& app) = 0;

  /** Uninitializes the subsystem. */
  virtual void Uninitialize() = 0;

  /**
   * Re-initializes the subsystem.
   *
   * The default implementation just calls Uninitialize() followed by Initialize().
   * Actual implementations might want to use a less radical and possibly more performant
   * approach.
   */
  virtual void Reinitialize(Application& app);

  /**
   * Called before the Application's command line processing begins.
   * If a subsystem wants to support command line arguments,
   * it must override this method.
   * The default implementation does not define any options.
   *
   * To effectively handle options, a subsystem should either bind
   * the option to a configuration property or specify a callback
   * to handle the option.
   */
  virtual void DefineOptions(OptionSet& options);
};

} // namespace framework
} // namespace fun
