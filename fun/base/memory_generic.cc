#include "CorePrivatePCH.h"
#include "Containers/Ticker.h"
#include "Async/Async.h"
#include "HAL/MallocAnsi.h"
#include "Platform/Generic/GenericPlatformMemoryPoolStats.h"
#include "HAL/MemoryMisc.h"

namespace fun {

DEFINE_STAT(MCR_Physical);
DEFINE_STAT(MCR_GPU);
DEFINE_STAT(MCR_TexturePool);

DEFINE_STAT(STAT_TotalPhysical);
DEFINE_STAT(STAT_TotalVirtual);
DEFINE_STAT(STAT_PageSize);
DEFINE_STAT(STAT_TotalPhysicalGB);

DEFINE_STAT(STAT_AvailablePhysical);
DEFINE_STAT(STAT_AvailableVirtual);
DEFINE_STAT(STAT_UsedPhysical);
DEFINE_STAT(STAT_PeakUsedPhysical);
DEFINE_STAT(STAT_UsedVirtual);
DEFINE_STAT(STAT_PeakUsedVirtual);


/** Helper class used to update platform memory stats. */
struct CGenericStatsUpdater
{
  /** Called once per second, enqueues stats update. */
  static bool EnqueueUpdateStats(float /*InDeltaTime*/)
  {
    AsyncTask(ENamedThreads::AnyThread, []()
    {
      DoUpdateStats();
    });
    return true; // Tick again
  }

  /** Gathers and sets all platform memory statistics into the corresponding stats. */
  static void DoUpdateStats()
  {
    // This is slow, so do it on the task graph.
    CPlatformMemoryStats MemoryStats = PlatformMemory::GetStats();
    SET_MEMORY_STAT(STAT_TotalPhysical, MemoryStats.TotalPhysical);
    SET_MEMORY_STAT(STAT_TotalVirtual, MemoryStats.TotalVirtual);
    SET_MEMORY_STAT(STAT_PageSize, MemoryStats.PageSize);
    SET_MEMORY_STAT(STAT_TotalPhysicalGB, MemoryStats.TotalPhysicalGB);

    SET_MEMORY_STAT(STAT_AvailablePhysical, MemoryStats.AvailablePhysical);
    SET_MEMORY_STAT(STAT_AvailableVirtual, MemoryStats.AvailableVirtual);
    SET_MEMORY_STAT(STAT_UsedPhysical, MemoryStats.UsedPhysical);
    SET_MEMORY_STAT(STAT_PeakUsedPhysical, MemoryStats.PeakUsedPhysical);
    SET_MEMORY_STAT(STAT_UsedVirtual, MemoryStats.UsedVirtual);
    SET_MEMORY_STAT(STAT_PeakUsedVirtual, MemoryStats.PeakUsedVirtual);

    // Platform specific stats.
    PlatformMemory::InternalUpdateStats(MemoryStats);
  }
};


CGenericPlatformMemoryStats::CGenericPlatformMemoryStats()
  : CGenericPlatformMemoryConstants(PlatformMemory::GetConstants())
  , AvailablePhysical(0)
  , AvailableVirtual(0)
  , UsedPhysical(0)
  , PeakUsedPhysical(0)
  , UsedVirtual(0)
  , PeakUsedVirtual(0)
{}


bool CGenericPlatformMemory::bIsOOM = false;
uint64 CGenericPlatformMemory::OOMAllocationSize = 0;
uint32 CGenericPlatformMemory::OOMAllocationAlignment = 0;


/**
 * Value determined by series of tests on Fortnite with limited process memory.
 * 26MB sufficed to report all test crashes, using 32MB to have some slack.
 * If this pool is too large, use the following values to determine proper size:
 * 2MB pool allowed to report 78% of crashes.
 * 6MB pool allowed to report 90% of crashes.
 */
uint32 CGenericPlatformMemory::BackupOOMMemoryPoolSize = 32 * 1024 * 1024;
void* CGenericPlatformMemory::BackupOOMMemoryPool = nullptr;


void CGenericPlatformMemory::SetupMemoryPools()
{
  SET_MEMORY_STAT(MCR_Physical, 0); // "unlimited" physical memory, we still need to make this call to set the short name, etc
  SET_MEMORY_STAT(MCR_GPU, 0); // "unlimited" GPU memory, we still need to make this call to set the short name, etc
  SET_MEMORY_STAT(MCR_TexturePool, 0); // "unlimited" Texture memory, we still need to make this call to set the short name, etc

  BackupOOMMemoryPool = PlatformMemory::BinnedAllocFromOS(BackupOOMMemoryPoolSize);
}


void CGenericPlatformMemory::Init()
{
  if (PlatformMemory::SupportBackupMemoryPool())
  {
    SetupMemoryPools();
  }

#if STATS
  // Stats are updated only once per second.
  const float PollingInterval = 1.0f;
  CTicker::GetCoreTicker().AddTicker(CTickerDelegate::CreateStatic(&CGenericStatsUpdater::EnqueueUpdateStats), PollingInterval);

  // Update for the first time.
  CGenericStatsUpdater::DoUpdateStats();
#endif //STATS
}


void CGenericPlatformMemory::OnOutOfMemory(uint64 Size, uint32 Alignment)
{
  // Update memory stats before we enter the crash handler.

  OOMAllocationSize = Size;
  OOMAllocationAlignment = Alignment;

  bIsOOM = true;
  CPlatformMemoryStats PlatformMemoryStats = PlatformMemory::GetStats();
  if (BackupOOMMemoryPool)
  {
    PlatformMemory::BinnedFreeToOS(BackupOOMMemoryPool, BackupOOMMemoryPoolSize);
    LOG(LogMemory, Warning, TEXT("Freeing %d bytes from backup pool to handle out of memory."), BackupOOMMemoryPoolSize);
  }
  LOG(LogMemory, Warning, TEXT("MemoryStats:")\
    TEXT("\n\tAvailablePhysical %llu")\
    TEXT("\n\tAvailableVirtual %llu")\
    TEXT("\n\tUsedPhysical %llu")\
    TEXT("\n\tPeakUsedPhysical %llu")\
    TEXT("\n\tUsedVirtual %llu")\
    TEXT("\n\tPeakUsedVirtual %llu"),
    (uint64)PlatformMemoryStats.AvailablePhysical,
    (uint64)PlatformMemoryStats.AvailableVirtual,
    (uint64)PlatformMemoryStats.UsedPhysical,
    (uint64)PlatformMemoryStats.PeakUsedPhysical,
    (uint64)PlatformMemoryStats.UsedVirtual,
    (uint64)PlatformMemoryStats.PeakUsedVirtual);
  LOG(LogMemory, Fatal, TEXT("Ran out of memory allocating %llu bytes with alignment %u"), Size, Alignment);
}


MemoryAllocator* CGenericPlatformMemory::BaseAllocator()
{
  return new CMallocAnsi();
}


CPlatformMemoryStats CGenericPlatformMemory::GetStats()
{
  LOG(LogMemory, Warning, TEXT("CGenericPlatformMemory::GetStats not implemented on this platform"));
  return CPlatformMemoryStats();
}


void CGenericPlatformMemory::GetStatsForMallocProfiler(CGenericMemoryStats& OutStats)
{
#if STATS
  CPlatformMemoryStats Stats = PlatformMemory::GetStats();

  // Base common stats for all platforms.
  OutStats.Add(GET_STATDESCRIPTION(STAT_TotalPhysical), Stats.TotalPhysical);
  OutStats.Add(GET_STATDESCRIPTION(STAT_TotalVirtual), Stats.TotalVirtual);
  OutStats.Add(GET_STATDESCRIPTION(STAT_PageSize), Stats.PageSize);
  OutStats.Add(GET_STATDESCRIPTION(STAT_TotalPhysicalGB), (size_t)Stats.TotalPhysicalGB);
  OutStats.Add(GET_STATDESCRIPTION(STAT_AvailablePhysical), Stats.AvailablePhysical);
  OutStats.Add(GET_STATDESCRIPTION(STAT_AvailableVirtual), Stats.AvailableVirtual);
  OutStats.Add(GET_STATDESCRIPTION(STAT_UsedPhysical), Stats.UsedPhysical);
  OutStats.Add(GET_STATDESCRIPTION(STAT_PeakUsedPhysical), Stats.PeakUsedPhysical);
  OutStats.Add(GET_STATDESCRIPTION(STAT_UsedVirtual), Stats.UsedVirtual);
  OutStats.Add(GET_STATDESCRIPTION(STAT_PeakUsedVirtual), Stats.PeakUsedVirtual);
#endif //STATS
}

const CPlatformMemoryConstants& CGenericPlatformMemory::GetConstants()
{
  LOG(LogMemory, Warning, TEXT("CGenericPlatformMemory::GetConstants not implemented on this platform"));
  static CPlatformMemoryConstants MemoryConstants;
  return MemoryConstants;
}

uint32 CGenericPlatformMemory::GetPhysicalGBRam()
{
  return PlatformMemory::GetConstants().TotalPhysicalGB;
}

void* CGenericPlatformMemory::BinnedAllocFromOS(size_t Size)
{
  LOG(LogMemory, Error, TEXT("CGenericPlatformMemory::BinnedAllocFromOS not implemented on this platform"));
  return nullptr;
}

void CGenericPlatformMemory::BinnedFreeToOS(void* Ptr, size_t Size)
{
  LOG(LogMemory, Error, TEXT("CGenericPlatformMemory::BinnedFreeToOS not implemented on this platform"));
}

void CGenericPlatformMemory::DumpStats(Printer& Ar)
{
  const float InvMB = 1.0f / 1024.0f / 1024.0f;
  CPlatformMemoryStats MemoryStats = PlatformMemory::GetStats();
#if !NO_LOGGING
  const CName CategoryName(LogMemory.GetCategoryName());
#else
  const CName CategoryName(TEXT("LogMemory"));
#endif
  Ar.CategorizedPrintf(CategoryName, ELogVerbosity::Info, TEXT("Platform Memory Stats for %s"), ANSI_TO_TCHAR(CPlatformProperties::PlatformName()));
  Ar.CategorizedPrintf(CategoryName, ELogVerbosity::Info, TEXT("Process Physical Memory: %.2f MB used, %.2f MB peak"), MemoryStats.UsedPhysical*InvMB, MemoryStats.PeakUsedPhysical*InvMB);
  Ar.CategorizedPrintf(CategoryName, ELogVerbosity::Info, TEXT("Process Virtual Memory: %.2f MB used, %.2f MB peak"), MemoryStats.UsedVirtual*InvMB, MemoryStats.PeakUsedVirtual*InvMB);

  Ar.CategorizedPrintf(CategoryName, ELogVerbosity::Info, TEXT("Physical Memory: %.2f MB used, %.2f MB total"), (MemoryStats.TotalPhysical - MemoryStats.AvailablePhysical)*InvMB, MemoryStats.TotalPhysical*InvMB);
  Ar.CategorizedPrintf(CategoryName, ELogVerbosity::Info, TEXT("Virtual Memory: %.2f MB used, %.2f MB total"), (MemoryStats.TotalVirtual - MemoryStats.AvailableVirtual)*InvMB, MemoryStats.TotalVirtual*InvMB);
}

void CGenericPlatformMemory::DumpPlatformAndAllocatorStats(class Printer& Ar)
{
  PlatformMemory::DumpStats(Ar);
  GMalloc->DumpAllocatorStats(Ar);
}

void CGenericPlatformMemory::MemswapImpl(void* RESTRICT ptr1, void* RESTRICT ptr2, size_t Size)
{
  union PtrUnion
  {
    void* PtrVoid;
    uint8* Ptr8;
    uint16* Ptr16;
    uint32* Ptr32;
    uint64* Ptr64;
    UPTRINT PtrUint;
  };

  if (!Size)
  {
    return;
  }

  PtrUnion Union1 = { ptr1 };
  PtrUnion Union2 = { ptr2 };

  if (Union1.PtrUint & 1)
  {
    Valswap(*Union1.Ptr8++, *Union2.Ptr8++);
    Size -= 1;
    if (!Size)
    {
      return;
    }
  }

  if (Union1.PtrUint & 2)
  {
    Valswap(*Union1.Ptr16++, *Union2.Ptr16++);
    Size -= 2;
    if (!Size)
    {
      return;
    }
  }

  if (Union1.PtrUint & 4)
  {
    Valswap(*Union1.Ptr32++, *Union2.Ptr32++);
    Size -= 4;
    if (!Size)
    {
      return;
    }
  }

  const uint32 CommonAlignment = MathBase::Min(MathBase::CountTrailingZeros(Union1.PtrUint - Union2.PtrUint), 3u);
  switch (CommonAlignment) {
  default:
    for (; Size >= 8; Size -= 8)
    {
      Valswap(*Union1.Ptr64++, *Union2.Ptr64++);
    }
    break;

  case 2:
    for (; Size >= 4; Size -= 4)
    {
      Valswap(*Union1.Ptr32++, *Union2.Ptr32++);
    }
    break;

  case 1:
    for (; Size >= 2; Size -= 2)
    {
      Valswap(*Union1.Ptr16++, *Union2.Ptr16++);
    }
    break;

  case 0:
    for (; Size >= 1; Size -= 1)
    {
      Valswap(*Union1.Ptr8++, *Union2.Ptr8++);
    }
    break;
  }
}


CGenericPlatformMemory::CSharedMemoryRegion::CSharedMemoryRegion(const String& InName, uint32 InAccessMode, void* InAddress, size_t InSize)
  : AccessMode(InAccessMode)
  , Address(InAddress)
  , Size(InSize)
{
  CCharTraits::Strcpy(Name, sizeof(Name) - 1, *InName);
}

CGenericPlatformMemory::CSharedMemoryRegion* CGenericPlatformMemory::MapNamedSharedMemoryRegion(const String& Name, bool bCreate, uint32 AccessMode, size_t Size)
{
  LOG(LogHAL, Error, TEXT("CGenericPlatformMemory::MapNamedSharedMemoryRegion not implemented on this platform"));
  return nullptr;
}

bool CGenericPlatformMemory::UnmapNamedSharedMemoryRegion(CSharedMemoryRegion* MemoryRegion)
{
  LOG(LogHAL, Error, TEXT("CGenericPlatformMemory::UnmapNamedSharedMemoryRegion not implemented on this platform"));
  return false;
}

void CGenericPlatformMemory::InternalUpdateStats(const CPlatformMemoryStats& MemoryStats)
{
  // Generic method is empty. Implement at platform level.
}

} // namespace fun
