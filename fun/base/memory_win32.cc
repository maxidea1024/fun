#include "CorePrivatePCH.h"

#include "HAL/MallocTBB.h"
#include "HAL/MallocAnsi.h"
#include "Platform/Generic/GenericPlatformMemoryPoolStats.h"
#include "HAL/MemoryMisc.h"


//TODO 검증을 위해서 임시로...
#undef FUN_FORCE_ANSI_ALLOCATOR
#define FUN_FORCE_ANSI_ALLOCATOR       1
#define ENABLE_WIN_ALLOC_TRACKING  1


#if ENABLE_WIN_ALLOC_TRACKING
#include <crtdbg.h>
#endif

#if !FUN_FORCE_ANSI_ALLOCATOR
#include "HAL/MallocBinnedRedirect.h"
#endif

#include "Platform/Windows/AllowWindowsTypes.h"
#include <Psapi.h>
#pragma comment(lib, "psapi.lib")

namespace fun {

DECLARE_MEMORY_STAT(TEXT("Windows Specific Memory Stat"), STAT_WindowsSpecificMemoryStat, STATGROUP_MemoryPlatform);


#if ENABLE_WIN_ALLOC_TRACKING
// This allows tracking of allocations that don't happen within the engine's wrappers.
// You will probably want to set conditional breakpoints here to capture specific allocations
// which aren't related to static initialization, they will happen on the CRT anyway.
static int WindowsAllocHook(int nAllocType, void* pvData, size_t nSize, int nBlockUse, long lRequest, const unsigned char* szFileName, int nLine)
{
  return true;
}
#endif //ENABLE_WIN_ALLOC_TRACKING

#include "Platform/Generic/GenericPlatformMemoryPoolStats.h"

void CWindowsMemory::Init()
{
  CGenericPlatformMemory::Init();

#if PLATFORM_32BITS
  const int64 GB(1024*1024*1024);
  SET_MEMORY_STAT(MCR_Physical, 2*GB); //2Gb of physical memory on win32
#endif

  const CPlatformMemoryConstants& memory_constants = PlatformMemory::GetConstants();
#if PLATFORM_32BITS
  LOG(LogMemory, Info, TEXT("Memory total: Physical=%.1fGB (%dGB approx) Virtual=%.1fGB"),
    float(memory_constants.TotalPhysical/1024.0/1024.0/1024.0),
    memory_constants.TotalPhysicalGB,
    float(memory_constants.TotalVirtual/1024.0/1024.0/1024.0));
#else
  // Logging virtual memory size for 64bits is pointless.
  LOG(LogMemory, Info, TEXT("Memory total: Physical=%.1fGB (%dGB approx)"),
    float(memory_constants.TotalPhysical/1024.0/1024.0/1024.0),
    memory_constants.TotalPhysicalGB);
#endif //PLATFORM_32BITS

  DumpStats(*GLog);
}


MemoryAllocator* CWindowsMemory::BaseAllocator()
{
#if ENABLE_WIN_ALLOC_TRACKING
  // This allows tracking of allocations that don't happen within the engine's wrappers.
  // This actually won't be compiled unless bDebugBuildsActuallyUseDebugCRT is set in the
  // build configuration for UBT.
  _CrtSetAllocHook(WindowsAllocHook);
#endif

#if FUN_FORCE_ANSI_ALLOCATOR
  return new CMallocAnsi();
#elif (WITH_EDITORONLY_DATA || IS_PROGRAM) && (PLATFORM_SUPPORTS_TBB && TBB_ALLOCATOR_ALLOWED)
  return new CMallocTBB();
#else
  return new CMallocBinnedRedirect((uint32)(GetConstants().PageSize & uint32_MAX), (uint64)uint32_MAX + 1);
#endif
}


CPlatformMemoryStats CWindowsMemory::GetStats()
{
  // GlobalMemoryStatusEx
  // MEMORYSTATUSEX
  //  ullTotalPhys
  //  ullAvailPhys
  //  ullTotalVirtual
  //  ullAvailVirtual
  //
  // GetProcessMemoryInfo
  // PROCESS_MEMORY_COUNTERS
  //  WorkingSetSize
  //  UsedVirtual
  //  PeakUsedVirtual
  //
  // GetPerformanceInfo
  //  PPERFORMANCE_INFORMATION
  //  PageSize
  //
  // This method is slow, do not call it too often.
  // #TODO Should be executed only on the background thread.

  CPlatformMemoryStats MemoryStats;

  // Gather platform memory stats.
  MEMORYSTATUSEX mse;
  UnsafeMemory::Memzero(&mse, sizeof(mse));
  mse.dwLength = sizeof(mse);
  GlobalMemoryStatusEx(&mse);

  PROCESS_MEMORY_COUNTERS pmc;
  UnsafeMemory::Memzero(&pmc, sizeof(pmc));
  GetProcessMemoryInfo(::GetCurrentProcess(), &pmc, sizeof(pmc));

  MemoryStats.AvailablePhysical = mse.ullAvailPhys;
  MemoryStats.AvailableVirtual = mse.ullAvailVirtual;

  MemoryStats.UsedPhysical = pmc.WorkingSetSize;
  MemoryStats.PeakUsedPhysical = pmc.PeakWorkingSetSize;
  MemoryStats.UsedVirtual = pmc.PagefileUsage;
  MemoryStats.PeakUsedVirtual = pmc.PeakPagefileUsage;

  return MemoryStats;
}

void CWindowsMemory::GetStatsForMallocProfiler(CGenericMemoryStats& OutStats)
{
#if STATS
  CGenericPlatformMemory::GetStatsForMallocProfiler(OutStats);

  CPlatformMemoryStats Stats = GetStats();

  // Windows specific stats.
  OutStats.Add(GET_STATDESCRIPTION(STAT_WindowsSpecificMemoryStat), Stats.WindowsSpecificMemoryStat);
#endif
}

const CPlatformMemoryConstants& CWindowsMemory::GetConstants()
{
  static CPlatformMemoryConstants memory_constants;

  if (memory_constants.TotalPhysical == 0) {
    // Gather platform memory constants.
    MEMORYSTATUSEX mse;
    PlatformMemory::Memzero(&mse, sizeof(mse));
    mse.dwLength = sizeof(mse);
    ::GlobalMemoryStatusEx(&mse);

    SYSTEM_INFO sys_info;
    PlatformMemory::Memzero(&sys_info, sizeof(sys_info));
    ::GetSystemInfo(&sys_info);

    memory_constants.TotalPhysical = mse.ullTotalPhys;
    memory_constants.TotalVirtual = mse.ullTotalVirtual;
    memory_constants.PageSize = sys_info.dwAllocationGranularity;  // Use this so we get larger 64KiB pages, instead of 4KiB

    memory_constants.TotalPhysicalGB = (memory_constants.TotalPhysical + 1024 * 1024 * 1024 - 1) / 1024 / 1024 / 1024;
  }

  return memory_constants;
}

bool CWindowsMemory::PageProtect(void* const ptr, const size_t size, const bool can_read, const bool can_write)
{
  DWORD flOldProtect;
  uint32 protect_mode = 0;
  if (can_read && can_write) {
    protect_mode = PAGE_READWRITE;
  }
  else if (can_write) {
    protect_mode = PAGE_READWRITE;
  }
  else if (can_read) {
    protect_mode = PAGE_READONLY;
  }
  else {
    protect_mode = PAGE_NOACCESS;
  }
  return VirtualProtect(ptr, size, protect_mode, &flOldProtect) != 0;
}

void* CWindowsMemory::BinnedAllocFromOS(size_t size)
{
  return VirtualAlloc(nullptr, size, MEM_COMMIT, PAGE_READWRITE);
}

void CWindowsMemory::BinnedFreeToOS(void* ptr, size_t size)
{
  CA_SUPPRESS(6001)
  fun_verify(VirtualFree(ptr, 0, MEM_RELEASE) != 0);
}

PlatformMemory::CSharedMemoryRegion*
CWindowsMemory::MapNamedSharedMemoryRegion(const String& InName, bool bCreate, uint32 access_mode, size_t size)
{
  String Name(TEXT("Global\\"));
  Name += InName;

  DWORD OpenMappingAccess = FILE_MAP_READ;
  CHECK(access_mode != 0);
  if (access_mode == PlatformMemory::ESharedMemoryAccess::Write) {
    OpenMappingAccess = FILE_MAP_WRITE;
  }
  else if (access_mode == (PlatformMemory::ESharedMemoryAccess::Write | PlatformMemory::ESharedMemoryAccess::Read)) {
    OpenMappingAccess = FILE_MAP_ALL_ACCESS;
  }

  HANDLE Mapping = nullptr;
  if (bCreate) {
    DWORD CreateMappingAccess = PAGE_READONLY;
    CHECK(access_mode != 0);
    if (access_mode == PlatformMemory::ESharedMemoryAccess::Write) {
      CreateMappingAccess = PAGE_WRITECOPY;
    }
    else if (access_mode == (PlatformMemory::ESharedMemoryAccess::Write | PlatformMemory::ESharedMemoryAccess::Read)) {
      CreateMappingAccess = PAGE_READWRITE;
    }

#if PLATFORM_64BITS
    const DWORD MaxSizeHigh = (size >> 32);
    DWORD MaxSizeLow = size & 0xFFFFFFFF;
#else
    const DWORD MaxSizeHigh = 0;
    DWORD MaxSizeLow = size;
#endif

    Mapping = CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, CreateMappingAccess, MaxSizeHigh, MaxSizeLow, *Name);

    if (Mapping == nullptr) {
      DWORD err = GetLastError();
      LOG(LogHAL, Warning, TEXT("CreateFileMapping(file=INVALID_HANDLE_VALUE, security=NULL, protect=0x%x, MaxSizeHigh=%d, MaxSizeLow=%d, name='%s') failed with GetLastError() = %d"), CreateMappingAccess, MaxSizeHigh, MaxSizeLow, *Name, err);
    }
  }
  else {
    Mapping = OpenFileMapping(OpenMappingAccess, FALSE, *Name);

    if (Mapping == nullptr) {
      DWORD err = GetLastError();
      LOG(LogHAL, Warning, TEXT("OpenFileMapping(access=0x%x, inherit=false, name='%s') failed with GetLastError() = %d"), OpenMappingAccess, *Name, err);
    }
  }

  if (Mapping == nullptr) {
    return nullptr;
  }

  void* ptr = MapViewOfFile(Mapping, OpenMappingAccess, 0, 0, size);
  if (ptr == nullptr) {
    DWORD err = GetLastError();
    LOG(LogHAL, Warning, TEXT("MapViewOfFile(mapping=0x%x, access=0x%x, OffsetHigh=0, OffsetLow=0, NumBytes=%u) failed with GetLastError() = %d"), Mapping, OpenMappingAccess, size, err);
    CloseHandle(Mapping);
    return nullptr;
  }

  return new CWindowsSharedMemoryRegion(Name, access_mode, ptr, size, Mapping);
}

bool CWindowsMemory::UnmapNamedSharedMemoryRegion(CSharedMemoryRegion* MemoryRegion)
{
  bool bAllSucceeded = true;

  if (MemoryRegion != nullptr) {
    auto WindowsRegion = static_cast<CWindowsSharedMemoryRegion*>(MemoryRegion);

    if (!UnmapViewOfFile(WindowsRegion->GetAddress())) {
      bAllSucceeded = false;

      int err = GetLastError();
      LOG(LogHAL, Warning, TEXT("UnmapViewOfFile(address=%p) failed with GetLastError() = %d"), WindowsRegion->GetAddress(), err);
    }

    if (!CloseHandle(WindowsRegion->GetMapping())) {
      bAllSucceeded = false;

      int err = GetLastError();
      LOG(LogHAL, Warning, TEXT("CloseHandle(handle=0x%x) failed with GetLastError() = %d"), WindowsRegion->GetMapping(), err);
    }

    // delete the region
    delete WindowsRegion;
  }

  return bAllSucceeded;
}

void CWindowsMemory::InternalUpdateStats(const CPlatformMemoryStats& MemoryStats)
{
  // Windows specific stats.
  SET_MEMORY_STAT(STAT_WindowsSpecificMemoryStat, MemoryStats.WindowsSpecificMemoryStat);
}

#include "Platform/Windows/HideWindowsTypes.h"

} // namespace fun
