#include "CorePrivatePCH.h"
#include "MallocBinned2.h"
#include "MallocAnsi.h"
#include "unistd.h"
#include <jni.h>

#define JNI_CURRENT_VERSION  JNI_VERSION_1_6
extern JavaVM* GJavaVM;

static int64 GetNativeHeapAllocatedSize() {
  int64 AllocatedSize = 0;
#if 0 // TODO: this works but sometimes crashes?
  JNIEnv* Env = nullptr;
  GJavaVM->GetEnv((void **)&Env, JNI_CURRENT_VERSION);
  jint AttachThreadResult = GJavaVM->AttachCurrentThread(&Env, nullptr);

  if (AttachThreadResult != JNI_ERR)
  {
    jclass Class = Env->FindClass("android/os/Debug");
    if (Class != nullptr)
    {
      jmethodID MethodID = Env->GetStaticMethodID(Class, "getNativeHeapAllocatedSize", "()J");
      if (MethodID)
      {
        AllocatedSize = Env->CallStaticLongMethod(Class, MethodID);
      }
    }
  }
#endif
  return AllocatedSize;
}

void CAndroidMemory::Init() {
  CGenericPlatformMemory::Init();

  const CPlatformMemoryConstants& MemoryConstants = PlatformMemory::GetConstants();
  CPlatformMemoryStats MemoryStats = GetStats();
  LOG(LogInit, Info, TEXT("Memory total: Physical=%.2fMB (%dGB approx) Available=%.2fMB PageSize=%.1fKB"),
    float(MemoryConstants.TotalPhysical/1024.0/1024.0),
    MemoryConstants.TotalPhysicalGB,
    float(MemoryStats.AvailablePhysical/1024.0/1024.0),
    float(MemoryConstants.PageSize/1024.0)
    );
}

CPlatformMemoryStats CAndroidMemory::GetStats() {
  const CPlatformMemoryConstants& MemoryConstants = PlatformMemory::GetConstants();

  CPlatformMemoryStats MemoryStats;

  //int32 NumAvailPhysPages = sysconf(_SC_AVPHYS_PAGES);
  //MemoryStats.AvailablePhysical = NumAvailPhysPages * MemoryConstants.PageSize;

  MemoryStats.AvailablePhysical = MemoryConstants.TotalPhysical - GetNativeHeapAllocatedSize();
  MemoryStats.AvailableVirtual = 0;
  MemoryStats.UsedPhysical = 0;
  MemoryStats.UsedVirtual = 0;

  return MemoryStats;
}

const CPlatformMemoryConstants& CAndroidMemory::GetConstants() {
  static CPlatformMemoryConstants MemoryConstants;

  if (MemoryConstants.TotalPhysical == 0) {
    int32 NumPhysPages = sysconf(_SC_PHYS_PAGES);
    int32 PageSize = sysconf(_SC_PAGESIZE);

    MemoryConstants.TotalPhysical = NumPhysPages * PageSize;
    MemoryConstants.TotalVirtual = 0;
    MemoryConstants.PageSize = (uint32)PageSize;

    MemoryConstants.TotalPhysicalGB = (MemoryConstants.TotalPhysical + 1024 * 1024 * 1024 - 1) / 1024 / 1024 / 1024;
  }

  return MemoryConstants;
}

MemoryAllocator* CAndroidMemory::BaseAllocator() {
  const CPlatformMemoryConstants& MemoryConstants = PlatformMemory::GetConstants();
  // 1 << MathBase::CeilLogTwo(MemoryConstants.TotalPhysical) should really be MathBase::RoundUpToPowerOfTwo,
  // but that overflows to 0 when MemoryConstants.TotalPhysical is close to 4GB, since CeilLogTwo returns 32
  // this then causes the MemoryLimit to be 0 and crashing the app
  uint64 MemoryLimit = MathBase::Min<uint64>(uint64(1) << MathBase::CeilLogTwo(MemoryConstants.TotalPhysical), 0x100000000);

  //return new CMallocAnsi();
  return new CMallocBinned2(MemoryConstants.PageSize, MemoryLimit);
}

void* CAndroidMemory::BinnedAllocFromOS(size_t Size) {
  // valloc was deprecated, this is a functional equivalent, for SDK 21
  const CPlatformMemoryConstants& MemoryConstants = PlatformMemory::GetConstants();
  return memalign(MemoryConstants.PageSize, Size);
}

void CAndroidMemory::BinnedFreeToOS(void* ptr) {
  free(ptr);
}
