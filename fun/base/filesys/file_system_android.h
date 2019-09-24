﻿#pragma once

#include <jni.h>
#include "fun/base/base.h"
#include "fun/base/filesys/file_system_base.h"

namespace fun {

/**
 * Android File I/O implementation with additional utilities to deal
 * with Java side access.
 */
class FUN_BASE_API IAndroidPlatformFS : public IPhysicalFileSystem {
 public:
  static IAndroidPlatformFS& GetPhysicalFileSystem();

  /**
   * Get the android.content.res.AssetManager that Java code
   * should use to open APK assets.
   */
  virtual jobject GetAssetManager() = 0;

  // Get detailed information for a file that
  // we can hand to other Android media classes for access.

  /**
   * Is file embedded as an asset in the APK?
   */
  virtual bool IsAsset(const char* filename) = 0;

  /**
   * Offset within file or asset where its data starts.
   * Note, offsets for assets is relative to complete APK file
   * and matches what is returned by AssetFileDescriptor.getStartOffset().
   */
  virtual int64 FileStartOffset(const char* filename) = 0;

  /**
   * Get the root, i.e. underlying, path for the file. This
   * can be any of: a resolved file path, an OBB path, an
   * asset path.
   */
  virtual String FileRootPath(const char* filename) = 0;
};

}  // namespace fun
