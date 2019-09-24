#pragma once

#include "fun/base/base.h"

namespace fun {

class IPlatformFS;

/**
 * Platform file system chain manager.
 */
class FUN_BASE_API LayeredPlatformFS {
 public:
  /**
   * Constructor.
   */
  LayeredPlatformFS();

  /**
   * Gets the currently used platform file system.
   *
   * @return Reference to the currently used platform file system.
   */
  IPlatformFS& GetPlatformFS();

  /**
   * Sets the current platform file system.
   *
   * @param new_topmost_platform_fs - Platform file system to be used.
   */
  void SetPlatformFS(IPlatformFS& new_topmost_platform_fs);

  /**
   * Finds a platform file in the chain of active platform file systems.
   *
   * @param name - name of the platform file system.
   *
   * @return Pointer to the active platform file system or nullptr if the
   * platform file system was not found.
   */
  IPlatformFS* FindPlatformFS(const char* name);

  /**
   * Creates a new platform file system instance.
   *
   * @param name - name of the platform file system to create.
   *
   * @return Platform file instance of the platform file system type was found,
   * nullptr otherwise.
   */
  IPlatformFS* GetPlatformFS(const char* name);

  /**
   * Gets LayeredPlatformFS Singleton.
   */
  static LayeredPlatformFS& Get();

 private:
  /** Currently used platform file system. */
  IPlatformFS* topmost_platform_fs_;
};

}  // namespace fun
