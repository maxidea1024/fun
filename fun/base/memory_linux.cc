#include "CorePrivatePCH.h"
#include "MallocAnsi.h"
#include "MallocJemalloc.h"
#include "MallocBinned2.h"
#include <sys/sysinfo.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <unistd.h> // sysconf

namespace fun {

void LinuxMemory::Init() {
  CGenericPlatformMemory::Init();

  const CPlatformMemoryConstants& MemoryConstants = PlatformMemory::GetConstants();
  LOG(LogInit, Info, " - Physical RAM available (not considering process quota): %d GB (%lu MB, %lu KB, %lu bytes)",
    MemoryConstants.TotalPhysicalGB,
    MemoryConstants.TotalPhysical / (1024ULL * 1024ULL),
    MemoryConstants.TotalPhysical / 1024ULL,
    MemoryConstants.TotalPhysical);
}

MemoryAllocator* LinuxMemory::BaseAllocator() {
  // This function gets executed very early, way before main() (because global constructors will allocate memory).
  // This makes it ideal, if unobvious, place for a root privilege CHECK.
  if (geteuid() == 0) {
    fprintf(stderr, "Refusing to run with the root privileges.\n");
    CPlatformMisc::RequestExit(true);
    // unreachable
    return nullptr;
  }

  enum class EAllocatorToUse {
    Ansi,
    Jemalloc,
    Binned
  }
  allocator_to_use = EAllocatorToUse::Binned;

  // Prefer jemalloc for the editor and programs as it saved ~20% RES usage in my (RCL) tests.
  // Leave binned as the default for games and servers to keep runtime behavior consistent across platforms.
  if (PLATFORM_SUPPORTS_JEMALLOC && (FUN_EDITOR != 0 || IS_PROGRAM != 0)) {
    allocator_to_use = EAllocatorToUse::Jemalloc;
  }

  if (FUN_FORCE_ANSI_ALLOCATOR) {
    allocator_to_use = EAllocatorToUse::Ansi;
  }
  else {
    // Allow overriding on the command line.
    // We get here before main due to global ctors, so need to do some hackery to get command line args
    if (FILE* CmdLineFile = fopen("/proc/self/cmdline", "r")) {
      char * Arg = nullptr;
      size_t Size = 0;
      while (getdelim(&Arg, &Size, 0, CmdLineFile) != -1) {
#if PLATFORM_SUPPORTS_JEMALLOC
        if (CStringTraitsA::Stricmp(Arg, "-jemalloc") == 0) {
          allocator_to_use = EAllocatorToUse::Jemalloc;
          break;
        }
#endif
        if (CStringTraitsA::Stricmp(Arg, "-ansimalloc") == 0) {
          allocator_to_use = EAllocatorToUse::Ansi;
          break;
        }

        if (CStringTraitsA::Stricmp(Arg, "-binnedmalloc") == 0) {
          allocator_to_use = EAllocatorToUse::Jemalloc;
          break;
        }
      }
      free(Arg);
      fclose(CmdLineFile);
    }
  }

  MemoryAllocator* Allocator = nullptr;

  switch (allocator_to_use) {
    case EAllocatorToUse::Ansi:
      Allocator = new CMallocAnsi();
      break;

#if PLATFORM_SUPPORTS_JEMALLOC
    case EAllocatorToUse::Jemalloc:
      Allocator = new CMallocJemalloc();
      break;
#endif

    default: // intentional fall-through
    case EAllocatorToUse::Binned:
      Allocator = new CMallocBinned2(PlatformMemory::GetConstants().PageSize & uint32_MAX, 0x100000000);
      break;
  }

  printf("Using %ls.\n", Allocator != nullptr ? Allocator->GetDescriptiveName() : "nullptr allocator! We will probably crash right away");

  return Allocator;
}

bool LinuxMemory::PageProtect(void* const ptr, const size_t Size, const bool can_read, const bool can_write) {
  int32 protect_mode;
  if (can_read && can_write) {
    protect_mode = PROT_READ | PROT_WRITE;
  }
  else if (can_read) {
    protect_mode = PROT_READ;
  }
  else if (can_write) {
    protect_mode = PROT_WRITE;
  }
  else {
    protect_mode = PROT_NONE;
  }
  return mprotect(ptr, Size, protect_mode) == 0;
}

void* LinuxMemory::BinnedAllocFromOS(size_t Size) {
  return mmap(nullptr, Size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
}

void LinuxMemory::BinnedFreeToOS(void* ptr, size_t Size) {
  if (munmap(ptr, Size) != 0) {
    const int err = errno;
    LOG(LogHAL, Fatal, "munmap(addr=%p, len=%llu) failed with errno = %d (%s)", ptr, Size, err, StringCast<TCHAR>(strerror(err)).Get());
  }
}

namespace LinuxMemory {
  /**
  @brief Returns value in bytes from a status line
  @param line in format "Blah:  10000 kB" - needs to be writable as it will modify it
  @return value in bytes (10240000, i.e. 10000 * 1024 for the above example)
  */
  uint64 GetBytesFromStatusLine(char* line)
  {
    fun_check_ptr(line);
    int Len = strlen(line);

    // Len should be long enough to hold at least " kB\n"
    const int kSuffixLength = 4;    // " kB\n"
    if (Len <= kSuffixLength) {
      return 0;
    }

    // let's CHECK that this is indeed "kB"
    char * Suffix = &line[Len - kSuffixLength];
    if (strcmp(Suffix, " kB\n") != 0) {
      // Linux the kernel changed the format, huh?
      return 0;
    }

    // kill the kB
    *Suffix = 0;

    // find the beginning of the number
    for (const char* NumberBegin = Suffix; NumberBegin >= line; --NumberBegin) {
      if (*NumberBegin == ' ') {
        return static_cast<uint64>(atol(NumberBegin + 1)) * 1024ULL;
      }
    }

    // we were unable to find whitespace in front of the number
    return 0;
  }
}

CPlatformMemoryStats LinuxMemory::GetStats() {
  CPlatformMemoryStats MemoryStats;   // will init from constants

  // open to all kind of overflows, thanks to Linux approach of exposing system stats via /proc and lack of proper C API
  // And no, sysinfo() isn't useful for this (cannot get the same value for MemAvailable through it for example).

  if (FILE* FileGlobalMemStats = fopen("/proc/meminfo", "r")) {
    int FieldsSetSuccessfully = 0;
    size_t MemFree = 0, Cached = 0;
    do {
      char LineBuffer[256] = {0};
      char* line = fgets(LineBuffer, countof(LineBuffer), FileGlobalMemStats);
      if (line == nullptr) {
        break;  // eof or an error
      }

      // if we have MemAvailable, favor that (see http://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/commit/?id=34e431b0ae398fc54ea69ff85ec700722c9da773)
      if (strstr(line, "MemAvailable:") == line) {
        MemoryStats.AvailablePhysical = LinuxMemory::GetBytesFromStatusLine(line);
        ++FieldsSetSuccessfully;
      }
      else if (strstr(line, "SwapFree:") == line) {
        MemoryStats.AvailableVirtual = LinuxMemory::GetBytesFromStatusLine(line);
        ++FieldsSetSuccessfully;
      }
      else if (strstr(line, "MemFree:") == line) {
        MemFree = LinuxMemory::GetBytesFromStatusLine(line);
        ++FieldsSetSuccessfully;
      }
      else if (strstr(line, "Cached:") == line) {
        Cached = LinuxMemory::GetBytesFromStatusLine(line);
        ++FieldsSetSuccessfully;
      }
    }
    while (FieldsSetSuccessfully < 4);

    // if we didn't have MemAvailable (kernels < 3.14 or CentOS 6.x), use free + cached as a (bad) approximation
    if (MemoryStats.AvailablePhysical == 0) {
      MemoryStats.AvailablePhysical = MathBase::Min(MemFree + Cached, MemoryStats.TotalPhysical);
    }

    fclose(FileGlobalMemStats);
  }

  // again /proc "API" :/
  if (FILE* ProcMemStats = fopen("/proc/self/status", "r")) {
    int FieldsSetSuccessfully = 0;
    do {
      char LineBuffer[256] = {0};
      char* line = fgets(LineBuffer, countof(LineBuffer), ProcMemStats);
      if (line == nullptr) {
        break;  // eof or an error
      }

      if (strstr(line, "VmPeak:") == line) {
        MemoryStats.PeakUsedVirtual = LinuxMemory::GetBytesFromStatusLine(line);
        ++FieldsSetSuccessfully;
      }
      else if (strstr(line, "VmSize:") == line) {
        MemoryStats.UsedVirtual = LinuxMemory::GetBytesFromStatusLine(line);
        ++FieldsSetSuccessfully;
      }
      else if (strstr(line, "VmHWM:") == line) {
        MemoryStats.PeakUsedPhysical = LinuxMemory::GetBytesFromStatusLine(line);
        ++FieldsSetSuccessfully;
      }
      else if (strstr(line, "VmRSS:") == line) {
        MemoryStats.UsedPhysical = LinuxMemory::GetBytesFromStatusLine(line);
        ++FieldsSetSuccessfully;
      }
    }
    while (FieldsSetSuccessfully < 4);

    fclose(ProcMemStats);
  }

  // sanitize stats as sometimes peak < used for some reason
  MemoryStats.PeakUsedVirtual = MathBase::Max(MemoryStats.PeakUsedVirtual, MemoryStats.UsedVirtual);
  MemoryStats.PeakUsedPhysical = MathBase::Max(MemoryStats.PeakUsedPhysical, MemoryStats.UsedPhysical);

  return MemoryStats;
}

const CPlatformMemoryConstants& LinuxMemory::GetConstants() {
  static CPlatformMemoryConstants MemoryConstants;

  if (MemoryConstants.TotalPhysical == 0) {
    // Gather platform memory stats.
    struct sysinfo SysInfo;
    unsigned long long MaxPhysicalRAMBytes = 0;
    unsigned long long MaxVirtualRAMBytes = 0;

    if (0 == sysinfo(&SysInfo)) {
      MaxPhysicalRAMBytes = static_cast< unsigned long long >(SysInfo.mem_unit) * static_cast< unsigned long long >(SysInfo.totalram);
      MaxVirtualRAMBytes = static_cast< unsigned long long >(SysInfo.mem_unit) * static_cast< unsigned long long >(SysInfo.totalswap);
    }

    MemoryConstants.TotalPhysical = MaxPhysicalRAMBytes;
    MemoryConstants.TotalVirtual = MaxVirtualRAMBytes;
    MemoryConstants.TotalPhysicalGB = (MemoryConstants.TotalPhysical + 1024 * 1024 * 1024 - 1) / 1024 / 1024 / 1024;
    MemoryConstants.PageSize = sysconf(_SC_PAGESIZE);
  }

  return MemoryConstants;
}

PlatformMemory::CSharedMemoryRegion*
LinuxMemory::MapNamedSharedMemoryRegion(const string& InName, bool bCreate, uint32 AccessMode, size_t Size) {
  // expecting platform-independent name, so convert it to match platform requirements
  string Name("/");
  Name += InName;
  CTCHARToUTF8 NameUTF8(*Name);

  // correct size to match platform constraints
  CPlatformMemoryConstants MemConstants = PlatformMemory::GetConstants();
  CHECK(MemConstants.PageSize > 0);   // also relying on it being power of two, which should be true in foreseeable future
  if (Size & (MemConstants.PageSize - 1))
  {
    Size = Size & ~(MemConstants.PageSize - 1);
    Size += MemConstants.PageSize;
  }

  int ShmOpenFlags = bCreate ? O_CREAT : 0;
  // note that you cannot combine O_RDONLY and O_WRONLY to get O_RDWR
  CHECK(AccessMode != 0);
  if (AccessMode == PlatformMemory::ESharedMemoryAccess::Read) {
    ShmOpenFlags |= O_RDONLY;
  }
  else if (AccessMode == PlatformMemory::ESharedMemoryAccess::Write) {
    ShmOpenFlags |= O_WRONLY;
  }
  else if (AccessMode == (PlatformMemory::ESharedMemoryAccess::Write | PlatformMemory::ESharedMemoryAccess::Read)) {
    ShmOpenFlags |= O_RDWR;
  }

  int ShmOpenMode = (S_IRUSR | S_IWUSR) | (S_IRGRP | S_IWGRP) | (S_IROTH | S_IWOTH);  // 0666

  // open the object
  int SharedMemoryFd = shm_open(NameUTF8.Get(), ShmOpenFlags, ShmOpenMode);
  if (SharedMemoryFd == -1) {
    int err = errno;
    LOG(LogHAL, Warning, "shm_open(name='%s', flags=0x%x, mode=0x%x) failed with errno = %d (%s)", *Name, ShmOpenFlags, ShmOpenMode, err, StringCast<TCHAR>(strerror(err)).Get());
    return nullptr;
  }

  // truncate if creating (note that we may still don't have rights to do so)
  if (bCreate) {
    int Res = ftruncate(SharedMemoryFd, Size);
    if (Res != 0) {
      int err = errno;
      LOG(LogHAL, Warning, "ftruncate(fd=%d, size=%d) failed with errno = %d (%s)", SharedMemoryFd, Size, err, StringCast<TCHAR>(strerror(err)).Get());
      shm_unlink(NameUTF8.Get());
      return nullptr;
    }
  }

  // map
  int MmapProtFlags = 0;
  if (AccessMode & PlatformMemory::ESharedMemoryAccess::Read) {
    MmapProtFlags |= PROT_READ;
  }

  if (AccessMode & PlatformMemory::ESharedMemoryAccess::Write) {
    MmapProtFlags |= PROT_WRITE;
  }

  void *ptr = mmap(nullptr, Size, MmapProtFlags, MAP_SHARED, SharedMemoryFd, 0);
  if (ptr == MAP_FAILED) {
    int err = errno;
    LOG(LogHAL, Warning, "mmap(addr=nullptr, length=%d, prot=0x%x, flags=MAP_SHARED, fd=%d, 0) failed with errno = %d (%s)", Size, MmapProtFlags, SharedMemoryFd, err, StringCast<TCHAR>(strerror(err)).Get());

    if (bCreate) {
      shm_unlink(NameUTF8.Get());
    }
    return nullptr;
  }

  return new CLinuxSharedMemoryRegion(Name, AccessMode, ptr, Size, SharedMemoryFd, bCreate);
}

bool LinuxMemory::UnmapNamedSharedMemoryRegion(CSharedMemoryRegion * MemoryRegion) {
  bool bAllSucceeded = true;

  if (MemoryRegion) {
    CLinuxSharedMemoryRegion * LinuxRegion = static_cast< CLinuxSharedMemoryRegion* >(MemoryRegion);

    if (munmap(LinuxRegion->GetAddress(), LinuxRegion->GetSize()) == -1) {
      bAllSucceeded = false;

      int err = errno;
      LOG(LogHAL, Warning, "munmap(addr=%p, len=%d) failed with errno = %d (%s)", LinuxRegion->GetAddress(), LinuxRegion->GetSize(), err, StringCast<TCHAR>(strerror(err)).Get());
    }

    if (close(LinuxRegion->GetFileDescriptor()) == -1) {
      bAllSucceeded = false;

      int err = errno;
      LOG(LogHAL, Warning, "close(fd=%d) failed with errno = %d (%s)", LinuxRegion->GetFileDescriptor(), err, StringCast<TCHAR>(strerror(err)).Get());
    }

    if (LinuxRegion->NeedsToUnlinkRegion()) {
      CTCHARToUTF8 NameUTF8(LinuxRegion->GetName());
      if (shm_unlink(NameUTF8.Get()) == -1) {
        bAllSucceeded = false;

        int err = errno;
        LOG(LogHAL, Warning, "shm_unlink(name='%s') failed with errno = %d (%s)", LinuxRegion->GetName(), err, StringCast<TCHAR>(strerror(err)).Get());
      }
    }

    // delete the region
    delete LinuxRegion;
  }

  return bAllSucceeded;
}

} // namespace fun
