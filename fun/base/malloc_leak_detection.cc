#include "fun/base/malloc_leak_detection.h"

#if FUN_MALLOC_LEAKDETECTION

namespace fun {

//TODO
/*
AutoConsoleCommand InvalidateCachedShaders(
  "MallocLeak",
  "Usage:\n"
  "'MallocLeak Start'  Begins accumulating unique callstacks\n"
  "'MallocLeak Stop'  Dumps outstanding unique callstacks and stops accumulation.  Also clears data.\n"
  "'MallocLeak Clear'  Clears all accumulated data.  Does not change start/stop state.\n"
  "'MallocLeak Dump size' Dumps oustanding unique callstacks with optional size filter in bytes.\n",
  ConsoleCommandWithArgsDelegate::CreateStatic(MemoryAllocatorLeakDetection::HandleMallocLeakCommand),
  ECVF_Default
  );
*/

MemoryAllocatorLeakDetection::MemoryAllocatorLeakDetection()
  : capture_allocs_(false), dump_outstanding_allocs_(false) {}

MemoryAllocatorLeakDetection& MemoryAllocatorLeakDetection::Get() {
  static MemoryAllocatorLeakDetection inst;
  return inst;
}

MemoryAllocatorLeakDetection::~MemoryAllocatorLeakDetection() {}

void MemoryAllocatorLeakDetection::HandleMallocLeakCommandInternal(const Array<String>& args) {
  if (args.Count() >= 1) {
    if (args[0].Compare("Start", CaseSensitivity::IgnoreCase) == 0) {
      fun_log(LogConsoleResponse, Info, "Starting allocation tracking.");
      SetAllocationCollection(true);
    } else if (args[0].Compare("Stop", CaseSensitivity::IgnoreCase) == 0) {
      fun_log(LogConsoleResponse, Info, "Stopping allocation tracking and clearing data.");
      SetAllocationCollection(false);
      DumpOpenCallstacks();
      ClearData();
    } else if (args[0].Compare("Clear", CaseSensitivity::IgnoreCase) == 0) {
      fun_log(LogConsoleResponse, Info, "Clearing tracking data.");
      ClearData();
    } else if (args[0].Compare("Dump", CaseSensitivity::IgnoreCase) == 0) {
      uint32 filter_size = 0;
      if (args.Count() >= 2) {
        filter_size = CharTraitsA::Atoi(*args[1]);
      }
      fun_log(LogConsoleResponse, Info, "Dumping unique calltacks with %i bytes or more oustanding.", filter_size);
      DumpOpenCallstacks(filter_size);
    }
  }
}

void MemoryAllocatorLeakDetection::HandleMallocLeakCommand(const Array<String>& args) {
  Get().HandleMallocLeakCommandInternal(args);
}

void MemoryAllocatorLeakDetection::AddCallstack(CallstackTrack& callstack) {
  ScopedLock<FastMutex> pointers_guard(allocated_pointers_mutex_);
  uint32 callstack_hash = Crc::Crc32(callstack.callstack, sizeof(callstack.callstack), 0);
  CallstackTrack& unique_callstack = unique_callstacks_.FindOrAdd(callstack_hash);
  //if we had a hash collision bail and lose the data rather than corrupting existing data.
  if (unique_callstack.count > 0 && unique_callstack != callstack) {
    fun_check_msg(false, "callstack hash collision.  Throwing away new stack.");
    return;
  }

  if (unique_callstack.count == 0) {
    unique_callstack = callstack;
  } else {
    unique_callstack.size += callstack.size;
  }

  unique_callstack.count++;
}

void MemoryAllocatorLeakDetection::RemoveCallstack(CallstackTrack& callstack) {
  ScopedLock<FastMutex> pointers_guard(allocated_pointers_mutex_);
  uint32 callstack_hash = Crc::Crc32(callstack.callstack, sizeof(callstack.callstack), 0);
  CallstackTrack* unique_callstack = unique_callstacks_.Find(callstack_hash);
  if (unique_callstack) {
    unique_callstack->count--;
    unique_callstack->size -= callstack.size;
    if (unique_callstack->count == 0) {
      unique_callstack = nullptr;
      unique_callstacks_.Remove(callstack_hash);
    }
  }
}

void MemoryAllocatorLeakDetection::SetAllocationCollection(bool enable) {
  ScopedLock<FastMutex> pointers_guard(allocated_pointers_mutex_);
  capture_allocs_ = enable;
}

void MemoryAllocatorLeakDetection::DumpOpenCallstacks(uint32 filter_size) {
  //could be called when OOM so avoid fun_log functions.
  ScopedLock<FastMutex> pointers_guard(allocated_pointers_mutex_);
  CPlatformMisc::LowLevelOutputDebugStringf("Dumping out of %i possible open callstacks filtered with more than %u bytes on frame: %i\n", unique_callstacks_.Count(), filter_size, (int32)g_frame_counter);
  const int32 MAX_CALLSTACK_LINE_CHARS = 2048;
  char callstack_string[MAX_CALLSTACK_LINE_CHARS];
  char callstack_string_wide[MAX_CALLSTACK_LINE_CHARS];
  UnsafeMemory::Memzero(callstack_string);
  for (const auto& pair : unique_callstacks_) {
    const CallstackTrack& callstack = pair.value;
    if (callstack.size >= filter_size) {
      CPlatformMisc::LowLevelOutputDebugStringf("alloc_size: %i, Num: %i, FirstFrameEverAllocated: %i\n", callstack.size, callstack.Count, callstack.frame_number);
      for (int32 i = 0; i < CallstackTrack::Depth; ++i) {
        CPlatformStackWalk::ProgramCounterToHumanReadableString(i, callstack.CallStack[i], callstack_string, MAX_CALLSTACK_LINE_CHARS);
        //convert ansi -> tchar without mallocs in case we are in OOM handler.
        for (int32 cur_char = 0; cur_char < MAX_CALLSTACK_LINE_CHARS; ++cur_char) {
          callstack_string_wide[cur_char] = callstack_string[cur_char];
        }
        CPlatformMisc::LowLevelOutputDebugStringf("%s\n", callstack_string_wide);
        UnsafeMemory::Memzero(callstack_string);
      }
    }
  }
}

void MemoryAllocatorLeakDetection::ClearData() {
  ScopedLock<FastMutex> pointers_guard(allocated_pointers_mutex_);
  open_pointers_.Clear();
  unique_callstacks_.Clear();
}

bool MemoryAllocatorLeakDetection::Exec(RuntimeEnv* env, const char* cmd, Printer& ar) {
  return false;
}

void MemoryAllocatorLeakDetection::Malloc(void* ptr, size_t size) {
  if (ptr) {
    if (capture_allocs_) {
      ScopedLock<FastMutex> pointers_guard(allocated_pointers_mutex_);
      if (!recursive_) {
        recursive_ = true;
        CallstackTrack callstack;
        CPlatformStackWalk::CaptureStackBackTrace(callstack.callstack, CallstackTrack::Depth);
        //TODO client에서나 의미 있는 값인데... 서버에서는 어떤식으로 처리하는게 바람직할까?
        //없애는게 좋을듯...??
        callstack.frame_number = g_frame_counter;
        callstack.size = size;
        AddCallstack(callstack);
        open_pointers_.Add(ptr, callstack);
        recursive_ = false;
      }
    }
  }
}

void MemoryAllocatorLeakDetection::Realloc(void* old_ptr, void* new_ptr, size_t size) {
  if (capture_allocs_) {
    ScopedLock<FastMutex> pointers_guard(allocated_pointers_mutex_);
    Free(old_ptr);
    Malloc(new_ptr, size);
  }
}

void MemoryAllocatorLeakDetection::Free(void* ptr) {
  if (ptr) {
    if (capture_allocs_) {
      ScopedLock<FastMutex> pointers_guard(allocated_pointers_mutex_);
      if (!recursive_) {
        recursive_ = true;
        if (CallstackTrack* callstack = open_pointers_.Find(ptr)) {
          RemoveCallstack(*callstack);
        }
        open_pointers_.Remove(ptr);
        recursive_ = false;
      }
    }
  }
}

MemoryAllocatorLeakDetectionProxy::MemoryAllocatorLeakDetectionProxy(MemoryAllocator* malloc)
  : inner_malloc_(malloc),
    verify_(MemoryAllocatorLeakDetection::Get()) {}

} // namespace fun

#endif //FUN_MALLOC_LEAKDETECTION
