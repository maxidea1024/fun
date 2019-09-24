#include "CorePrivatePCH.h"

IFileSystem& IFileSystem::GetPhysicalFileSystem() {
  static AppleFileSystem singleton;
  return singleton;
}
