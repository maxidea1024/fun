#pragma once

#include "fun/base/base.h"

namespace fun {

class LogSink;

/**
 * Any object that is capable to taking commands.
 */
class FUN_BASE_API Exec {
 public:
  virtual ~Exec() {}

  /**
   * TODO
   */
  virtual bool Execute(class RuntimeEnv* env, const char* cmd, LogSink& sink) = 0;
};

} // namespace fun
