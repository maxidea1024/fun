#pragma once

#include "fun/base/base.h"

namespace fun {

namespace LogLevel {
  /** Log level type */
  typedef uint8 Type;

  /**
   * Logging severity level.
   */
  enum {
    /**
     * A fatal error. The application will most likely terminate.
     * This is the highest level.
     */
    Fatal = 1,

    /**
     * A critical error. The application might not be able to
     * continue running successfully.
     */
    Critical,

    /**
     * An error. An operation did not complete successfully,
     * but the application as a whole is not affected.
     */
    Error,

    /**
     * A warning. An operation completed with an unexpected result.
     */
    Warning,

    /**
     * A notice, which is an information with just a higher level.
     */
    Notice,

    /**
     * An informational message, usually denoting
     * the successful completion of an operation.
     */
    Information,

    /**
     * A debugging message.
     */
    Debug,

    /**
     * A tracing message. This is the lowest level.
     */
    Trace
  };
} // namespace LogLevel

} // namespace fun
