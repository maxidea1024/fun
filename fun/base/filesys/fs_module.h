#pragma once

#include "fun/base/base.h"
#include "fun/module/module_interface.h"

namespace fun {

/**
 * Platform File Module Interface
 */
class IPlatformFileSystemModule : public IModuleInterface {
 public:
  /**
   * Creates a platform file system instance.
   *
   * @return Platform file instance.
   */
  virtual IPlatformFS* GetPlatformFileSystem() = 0;
};

} // namespace fun
