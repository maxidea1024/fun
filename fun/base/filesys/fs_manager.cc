#include "CorePrivatePCH.h"
#include "HAL/IPlatformFSCachedWrapper.h"
#include "HAL/IPlatformFSLogWrapper.h"
#include "HAL/IPlatformFSModule.h"
#include "HAL/IPlatformFSOpenLogWrapper.h"
#include "HAL/IPlatformFSProfilerWrapper.h"
#include "HAL/PlatformFSManager.h"
#include "Modules/ModuleManager.h"

namespace fun {

PlatformFSManager::PlatformFSManager() : topmost_platform_fs_(nullptr) {}

IPlatformFS& PlatformFSManager::GetPlatformFS() {
  if (topmost_platform_fs_ == nullptr) {
    topmost_platform_fs_ = &IPlatformFS::GetPlatformPhysical();
  }
  return *topmost_platform_fs_;
}

void PlatformFSManager::SetPlatformFS(IPlatformFS& new_topmost_platform_fs) {
  topmost_platform_fs_ = &new_topmost_platform_fs;
  topmost_platform_fs_->InitializeAfterSetActive();
}

IPlatformFS* PlatformFSManager::FindPlatformFS(const char* name) {
  fun_check_ptr(topmost_platform_fs_);
  for (IPlatformFS* chain = topmost_platform_fs_; chain;
       chain = chain->GetLowerLevel()) {
    if (CharTraits::Stricmp(chain->GetName(), name) == 0) {
      return chain;
    }
  }
  return nullptr;
}

IPlatformFS* PlatformFSManager::GetPlatformFS(const char* name) {
  IPlatformFS* platform_fs = nullptr;

  // Check Core platform files (Profile, Log) by name.
  if (CharTraits::Strcmp(LoggedPlatformFS::GetTypeName(), name) == 0) {
    static ScopedPtr<IPlatformFS> AutoDestroySingleton(new LoggedPlatformFS());
    platform_fs = AutoDestroySingleton.GetOwnedPointer();
  }
#if !FUN_BUILD_SHIPPING
  else if (CharTraits::Strcmp(TypedProfiledPlatformFileSystem<
                                  ProfiledFSStatsFileDetailed>::GetTypeName(),
                              name) == 0) {
    static ScopedPtr<IPlatformFS> AutoDestroySingleton(
        new TypedProfiledPlatformFileSystem<ProfiledFSStatsFileDetailed>());
    platform_fs = AutoDestroySingleton.GetOwnedPointer();
  } else if (CharTraits::Strcmp(TypedProfiledPlatformFileSystem<
                                    ProfiledFSStatsFileSimple>::GetTypeName(),
                                name) == 0) {
    static ScopedPtr<IPlatformFS> AutoDestroySingleton(
        new TypedProfiledPlatformFileSystem<ProfiledFSStatsFileSimple>());
    platform_fs = AutoDestroySingleton.GetOwnedPointer();
  } else if (CharTraits::Strcmp(PlatformFSReadStats::GetTypeName(), name) ==
             0) {
    static ScopedPtr<IPlatformFS> AutoDestroySingleton(
        new PlatformFSReadStats());
    platform_fs = AutoDestroySingleton.GetOwnedPointer();
  } else if (CharTraits::Strcmp(PlatformFSOpenLog::GetTypeName(), name) == 0) {
    static ScopedPtr<IPlatformFS> AutoDestroySingleton(new PlatformFSOpenLog());
    platform_fs = AutoDestroySingleton.GetOwnedPointer();
  }
#endif
  else if (CharTraits::Strcmp(CachedReadPlatformFS::GetTypeName(), name) == 0) {
    static ScopedPtr<IPlatformFS> AutoDestroySingleton(
        new CachedReadPlatformFS());
    platform_fs = AutoDestroySingleton.GetOwnedPointer();
  } else {
    // Try to load a module containing the platform file.
    if (IPlatformFSModule* platform_fs_module =
            ModuleManager::LoadModulePtr<IPlatformFSModule>(name)) {
      // TODO: Attempt to create platform file
      platform_fs = platform_fs_module->GetPlatformFS();
    }
  }

  return platform_fs;
}

PlatformFSManager& PlatformFSManager::Get() {
  static PlatformFSManager singleton;
  return singleton;
}

}  // namespace fun
