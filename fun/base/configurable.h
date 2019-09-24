#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * TODO
 */
class FUN_BASE_API Configurable {
 public:
  /**
   * Creates the Configurable
   */
  Configurable();

  /**
   * Virtual dummy destructor.
   */
  virtual ~Configurable() {}

  /**
   * Sets the property with the given name to the given value.
   * If a property with the given name is not supported, a
   * PropertyNotSupportedException is thrown.
   */
  virtual void SetProperty(const String& name, const String& value) = 0;

  /**
   * Returns the value of the property with the given name.
   * If a proprty with the given name is not supported, a
   * PropertyNotSupportedException is thrown.
   */
  virtual String GetProperty(const String& name) const = 0;
};

} // namespace fun
