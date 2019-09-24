#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * This class provides access to environment variables
 * and some general system information.
 */
class FUN_BASE_API Environment {
 public:
  /**
   * Ethernet address. (MAC address)
   */
  typedef uint8 NodeId[6];


  /**
   * Returns the value of the environment variable
   * with the given name. Throws a NotFoundException
   * if the variable does not exist.
   */
  static String Get(const String& name);

  /**
   * Returns the value of the environment variable
   * with the given name. If the environment variable
   * is undefined, returns default_value instead.
   */
  static String Get(const String& name, const String& default_value);

  /**
   * Returns true if an environment variable
   * with the given name is defined.
   */
  static bool Has(const String& name);

  /**
   * Sets the environment variable with the given name
   * to the given value.
   */
  static void Set(const String& name, const String& value);

  /**
   * Returns the operating system name.
   * */
  static String GetOsName();

  /**
   * Returns the operating system name in a
   * "user-friendly" way.
   *
   * Currently this is only implemented for
   * Windows. There it will return names like
   * "Windows XP" or "Windows 7/Server 2008 SP2".
   * On other platforms, returns the same as
   * GetOsName().
   */
  static String GetOsDisplayName();

  /**
   * Returns the operating system version.
   */
  static String GetOsVersion();

  /**
   * Returns the operating system architecture.
   */
  static String GetOsArchitecture();

  /**
   * Returns the node (or host) name.
   */
  static String GetNodeName();

  /**
   * Returns the Ethernet address of the first Ethernet
   * adapter found on the system.
   *
   * Throws a SystemException if no Ethernet adapter is available.
   */
  static String GetNodeId();

  /**
   * Returns the Ethernet address (format "xx:xx:xx:xx:xx:xx")
   * of the first Ethernet adapter found on the system.
   *
   * Throws a SystemException if no Ethernet adapter is available.
   */
  static void GetNodeId(NodeId& out_id);

  /**
   * Returns the number of processors installed in the system.
   *
   * If the number of processors cannot be determined, returns 1.
   */
  static int32 GetProcessorCount();

  /**
   * Returns the FUN C++ Libraries version as a hexadecimal
   * number in format 0xAABBCCDD, where
   *    - AA is the major version number,
   *    - BB is the minor version number,
   *    - CC is the revision number, and
   *    - DD is the patch level number.
   *
   * Some patch level ranges have special meanings:
   *    - Dx mark development releases,
   *    - Ax mark alpha releases, and
   *    - Bx mark beta releases.
   */
  static uint32 GetLibraryVersion();

  /**
   * Return the operating system as defined
   * in the include fun/base/platform.h (FUN_PLATFORM)
   */
  static int32 GetOs();

  /**
   * Return the underlying cpu that runs this operating system
   * as defined in fun/base/platform (FUN_ARCH)
   */
  static int32 GetArch();

  /**
   * Return true if the operating system belongs to the Linux family
   */
  static bool IsUnix();

  /**
   * Return true if the operating system belongs to the Windows family
   */
  static bool IsWindows();
};

} // namespace fun
