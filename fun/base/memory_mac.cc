#include "CorePrivatePCH.h"
#include "MallocTBB.h"
#include "MallocAnsi.h"
#include "MallocBinned2.h"

#include <sys/param.h>
#include <sys/mount.h>

void MacMemory::Init() {
  CGenericPlatformMemory::Init();

  const CPlatformMemoryConstants& memory_constants = PlatformMemory::GetConstants();
  LOG(LogInit, Info, "Memory total: Physical=%.1fGB (%dGB approx) Pagefile=%.1fGB Virtual=%.1fGB",
        float(memory_constants.TotalPhysical/1024.0/1024.0/1024.0),
        memory_constants.TotalPhysicalGB,
        float((memory_constants.TotalVirtual-memory_constants.TotalPhysical)/1024.0/1024.0/1024.0),
        float(memory_constants.TotalVirtual/1024.0/1024.0/1024.0));
}

MemoryAllocator* MacMemory::BaseAllocator() {
  bool is_mavericks = false;

  char OSRelease[PATH_MAX] = {};
  size_t OSReleaseBufferSize = PATH_MAX;
  if (sysctlbyname("kern.osrelease", OSRelease, &OSReleaseBufferSize, nullptr, 0) == 0) {
    int32 OSVersionMajor = 0;
    if (sscanf(OSRelease, "%d", &OSVersionMajor) == 1) {
      is_mavericks = OSVersionMajor <= 13;
    }
  }

  if (getenv("Fun_FORCE_MALLOC_ANSI") != nullptr || is_mavericks) {
    return new CMallocAnsi();
  }
  else {
#if FUN_FORCE_ANSI_ALLOCATOR || IS_PROGRAM
    return new CMallocAnsi();
#elif (WITH_EDITORONLY_DATA || IS_PROGRAM) && TBB_ALLOCATOR_ALLOWED
    return new FMallocTBB();
#else
    return new CMallocBinned2((uint32)(GetConstants().page_size & uint32_MAX), 0x100000000);
#endif
  }
}

CPlatformMemoryStats MacMemory::GetStats() {
  const CPlatformMemoryConstants& memory_constants = PlatformMemory::GetConstants();

  CPlatformMemoryStats memory_stats;

  // Gather platform memory stats.
  vm_statistics stats;
  mach_msg_type_number_t StatsSize = sizeof(stats);
  host_statistics(mach_host_self(), HOST_VM_INFO, (host_info_t)&stats, &StatsSize);
  uint64_t free_mem = stats.free_count * memory_constants.page_size;
  memory_stats.AvailablePhysical = free_mem;

  // Get swap file info
  xsw_usage swap_usage;
  size_t size = sizeof(swap_usage);
  sysctlbyname("vm.swapusage", &swap_usage, &size, nullptr, 0);
  memory_stats.AvailableVirtual = free_mem + swap_usage.xsu_avail;

  // Just get memory information for the process and report the working set instead
  mach_task_basic_info_data_t task_info;
  mach_msg_type_number_t TaskInfoCount = MACH_TASK_BASIC_INFO_COUNT;
  task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&task_info, &TaskInfoCount);
  memory_stats.UsedPhysical = task_info.resident_size;
  if (memory_stats.UsedPhysical > memory_stats.PeakUsedPhysical) {
    memory_stats.PeakUsedPhysical = memory_stats.UsedPhysical;
  }
  memory_stats.UsedVirtual = task_info.virtual_size;
  if (memory_stats.UsedVirtual > memory_stats.PeakUsedVirtual) {
    memory_stats.PeakUsedVirtual = memory_stats.UsedVirtual;
  }

  return memory_stats;
}

const CPlatformMemoryConstants& MacMemory::GetConstants() {
  static CPlatformMemoryConstants memory_constants;

  if (memory_constants.TotalPhysical == 0) {
    // Gather platform memory constants.

    // Get page size.
    vm_size_t page_size;
    host_page_size(mach_host_self(), &page_size);

    // Get swap file info
    xsw_usage swap_usage;
    size_t size = sizeof(swap_usage);
    sysctlbyname("vm.swapusage", &swap_usage, &size, nullptr, 0);

    // Get memory.
    int64 AvailablePhysical = 0;
    int mib[] = {CTL_HW, HW_MEMSIZE};
    size_t Length = sizeof(int64);
    sysctl(mib, 2, &AvailablePhysical, &Length, nullptr, 0);

    memory_constants.TotalPhysical = AvailablePhysical;
    memory_constants.TotalVirtual = AvailablePhysical + swap_usage.xsu_total;
    memory_constants.page_size = (uint32)page_size;

    memory_constants.TotalPhysicalGB = (memory_constants.TotalPhysical + 1024 * 1024 * 1024 - 1) / 1024 / 1024 / 1024;
  }

  return memory_constants;
}

bool MacMemory::PageProtect(void* const ptr,
                            const size_t size,
                            const bool can_read,
                            const bool can_write) {
  int32 protect_mode;
  if (can_read && can_write) {
    protect_mode = PROT_READ | PROT_WRITE;
  } else if (can_read) {
    protect_mode = PROT_READ;
  } else if (can_write) {
    protect_mode = PROT_WRITE;
  } else {
    protect_mode = PROT_NONE;
  }
  return mprotect(ptr, size, static_cast<int32>(protect_mode)) == 0;
}

void* MacMemory::BinnedAllocFromOS(size_t size) {
  return mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
}

void MacMemory::BinnedFreeToOS(void* ptr, size_t size) {
  if (munmap(ptr, size) != 0) {
    const int err = errno;
    LOG(LogHAL, Fatal, "munmap(addr=%p, len=%llu) failed with errno = %d (%s)", ptr, size,
        err, StringCast<TCHAR>(strerror(err)).Get());
  }
}
