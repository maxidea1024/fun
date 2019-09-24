#include <DbgHelp.h>
#include <Shlwapi.h>
#include <TlHelp32.h>
#include <psapi.h>
#include "CorePrivatePCH.h"
#include "Platform/Windows/AllowWindowsTypes.h"
#include "Platform/Windows/HideWindowsTypes.h"
#include "Platform/Windows/WindowsStackWalk.h"

#include "Modules/ModuleManager.h"

//@todo 임시로 직접 지정해줌.
#pragma comment(lib, "Dbghelp.lib")
//#pragma comment(lib, "NtosKrnl.lib") //windows10?

namespace fun {

/** Whether appInitStackWalking() has been called successfully or not. */
static bool GStackWalkingInitialized = false;
static bool GNeedToRefreshSymbols = false;

static const TCHAR* CrashReporterSettings =
    TEXT("/Script/FunEd.CrashReporterSettings");

// NOTE: Make sure to enable Stack Frame pointers: bOmitFramePointers = false,
// or /Oy- If GStackWalkingInitialized is true, traces will work anyway but will
// be much slower.
#define USE_FAST_STACKTRACE 0

typedef bool(WINAPI* TFEnumProcesses)(uint32* lpidProcess, uint32 cb,
                                      uint32* cbNeeded);
typedef bool(WINAPI* TFEnumProcessModules)(HANDLE hProcess, HMODULE* lphModule,
                                           uint32 cb, LPDWORD lpcbNeeded);
typedef uint32(WINAPI* TFGetModuleBaseName)(HANDLE hProcess, HMODULE hModule,
                                            LPWSTR lpBaseName, uint32 nSize);
typedef uint32(WINAPI* TFGetModuleFileNameEx)(HANDLE hProcess, HMODULE hModule,
                                              LPWSTR lpFilename, uint32 nSize);
typedef bool(WINAPI* TFGetModuleInformation)(HANDLE hProcess, HMODULE hModule,
                                             LPMODULEINFO lpmodinfo, uint32 cb);

static TFEnumProcesses FEnumProcesses;
static TFEnumProcessModules FEnumProcessModules;
static TFGetModuleBaseName FGetModuleBaseName;
static TFGetModuleFileNameEx FGetModuleFileNameEx;
static TFGetModuleInformation FGetModuleInformation;

// Helper function performing the actual stack walk. This code relies on the
// symbols being loaded for best results walking the stack albeit at a
// significant performance penalty.
//
// This helper function is designed to be called within a structured exception
// handler.
//
// @param backtrace - Array to write backtrace to
// @param max_depth - Maximum depth to walk - needs to be less than or equal to
// array size
// @param context - Thread context information
//
// @return EXCEPTION_EXECUTE_HANDLER
static int32 CaptureStackTraceHelper(uint64* backtrace, uint32 max_depth,
                                     CONTEXT* context) {
  STACKFRAME64 stack_frame64;
  HANDLE process_handle;
  HANDLE thread_handle;
  unsigned long last_error;
  bool stackwalk_succeded = true;
  uint32 current_depth = 0;
  uint32 machine_type = IMAGE_FILE_MACHINE_I386;
  CONTEXT context_copy = *context;

#if !PLATFORM_SEH_EXCEPTIONS_DISABLED
  __try
#endif
  {
    // Get context, process and thread information.
    process_handle = GetCurrentProcess();
    thread_handle = GetCurrentThread();

    // Zero out stack frame.
    UnsafeMemory::Memzero(stack_frame64);

    // Initialize the STACKFRAME structure.
    stack_frame64.AddrPC.Mode = AddrModeFlat;
    stack_frame64.AddrStack.Mode = AddrModeFlat;
    stack_frame64.AddrFrame.Mode = AddrModeFlat;
#if FUN_64_BIT
    stack_frame64.AddrPC.Offset = context->Rip;
    stack_frame64.AddrStack.Offset = context->Rsp;
    stack_frame64.AddrFrame.Offset = context->Rbp;
    machine_type = IMAGE_FILE_MACHINE_AMD64;
#else   // FUN_64_BIT
    stack_frame64.AddrPC.Offset = context->Eip;
    stack_frame64.AddrStack.Offset = context->Esp;
    stack_frame64.AddrFrame.Offset = context->Ebp;
#endif  // FUN_64_BIT

    // Walk the stack one frame at a time.
    while (stackwalk_succeded && (current_depth < max_depth)) {
      stackwalk_succeded =
          !!StackWalk64(machine_type, process_handle, thread_handle,
                        &stack_frame64, &context_copy, nullptr,
                        SymFunctionTableAccess64, SymGetModuleBase64, nullptr);

      backtrace[current_depth++] = stack_frame64.AddrPC.Offset;

      if (!stackwalk_succeded) {
        // StackWalk failed! give up.
        last_error = GetLastError();
        break;
      }

      // Stop if the frame pointer is NULL.
      // Note that the thread's PC 'stack_frame64.AddrPC.Offset' COULD be 0 in
      // case something calls a nullptr.
      if (stack_frame64.AddrFrame.Offset == 0) {
        break;
      }
    }
  }
#if !PLATFORM_SEH_EXCEPTIONS_DISABLED
  __except (EXCEPTION_EXECUTE_HANDLER) {
    // We need to catch any exceptions within this function so they don't get
    // sent to the engine's error handler, hence causing an infinite loop.
    return EXCEPTION_EXECUTE_HANDLER;
  }
#endif

  // NULL out remaining entries.
  for (; current_depth < max_depth; current_depth++) {
    backtrace[current_depth] = 0;
  }

  return EXCEPTION_EXECUTE_HANDLER;
}

PRAGMA_DISABLE_OPTIMIZATION  // Work around "flow in or out of inline asm code
                             // suppresses global optimization" warning C4740.

#if USE_FAST_STACKTRACE
                                 NTSYSAPI uint16 NTAPI
                                 RtlCaptureStackBackTrace(
                                     __in uint32 FramesToSkip,
                                     __in uint32 FramesToCapture,
                                     __out_ecount(FramesToCapture)
                                         PVOID* backtrace,
                                     __out_opt PDWORD BackTraceHash);

/** Maximum callstack depth that is supported by the current OS. */
static ULONG GMaxCallstackDepth = 62;

/** Whether DetermineMaxCallstackDepth() has been called or not. */
static bool GMaxCallstackDepthInitialized = false;

/** Maximum callstack depth we support, no matter what OS we're running on. */
#define MAX_CALLSTACK_DEPTH 128

/** Checks the current OS version and sets up the GMaxCallstackDepth variable.
 */
void DetermineMaxCallstackDepth() {
  // Check that we're running on Vista or newer (version 6.0+).
  if (CWindowsMisc::VerifyWindowsVersion(6, 0)) {
    GMaxCallstackDepth = MAX_CALLSTACK_DEPTH;
  } else {
    GMaxCallstackDepth = MathBase::Min<ULONG>(62, MAX_CALLSTACK_DEPTH);
  }
  GMaxCallstackDepthInitialized = true;
}

#endif

void StackwalkWin::StackwalkAndDump(ANSICHAR* HumanReadableString,
                                    size_t HumanReadableStringSize,
                                    int32 IgnoreCount, void* context) {
  InitStackWalking();

  CGenericPlatformStackWalk::StackwalkAndDump(
      HumanReadableString, HumanReadableStringSize, IgnoreCount, context);
}

void StackwalkWin::ThreadStackwalkAndDump(ANSICHAR* HumanReadableString,
                                          size_t HumanReadableStringSize,
                                          int32 IgnoreCount, uint32 thread_id) {
  InitStackWalking();

  const HANDLE thread_handle =
      OpenThread(THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_TERMINATE |
                     THREAD_SUSPEND_RESUME,
                 false, thread_id);
  if (thread_handle) {
    // Suspend the thread before grabbing its context (possible fix for
    // incomplete callstacks)
    SuspendThread(thread_handle);

    // Give task scheduler some time to actually suspend the thread
    CPlatformProcess::SleepNoStats(0.01f);

    CONTEXT thread_context;
    thread_context.ContextFlags = CONTEXT_CONTROL;
    if (GetThreadContext(thread_handle, &thread_context)) {
      CGenericPlatformStackWalk::StackwalkAndDump(HumanReadableString,
                                                  HumanReadableStringSize,
                                                  IgnoreCount, &thread_context);
    }

    ResumeThread(thread_handle);
  }
}

// @TODO yrx 2014-09-05 Switch to TArray<uint64,TFixedAllocator<100>>>
/**
Capture a stack backtrace and optionally use the passed in exception pointers.

@param backtrace - [out] Pointer to array to take backtrace
@param max_depth - Entries in backtrace array
@param context - Optional thread context information
*/
void StackwalkWin::CaptureStackBackTrace(uint64* backtrace, uint32 max_depth,
                                         void* context) {
  // Make sure we have place to store the information before we go through the
  // process of raising an exception and handling it.
  if (backtrace == nullptr || max_depth == 0) {
    return;
  }

  if (context) {
    CaptureStackTraceHelper(backtrace, max_depth, (CONTEXT*)context);
  } else {
#if USE_FAST_STACKTRACE
    // NOTE: Make sure to enable Stack Frame pointers: bOmitFramePointers =
    // false, or /Oy- If GStackWalkingInitialized is true, traces will work
    // anyway but will be much slower.
    if (GStackWalkingInitialized) {
      CONTEXT helper_context;
      RtlCaptureContext(&helper_context);

      // Capture the back trace.
      CaptureStackTraceHelper(backtrace, max_depth, &helper_context);
    } else {
      if (!GMaxCallstackDepthInitialized) {
        DetermineMaxCallstackDepth();
      }
      PVOID WinBackTrace[MAX_CALLSTACK_DEPTH];
      uint16 NumFrames = RtlCaptureStackBackTrace(
          0, MathBase::Min<ULONG>(GMaxCallstackDepth, max_depth), WinBackTrace,
          nullptr);
      for (uint16 FrameIndex = 0; FrameIndex < NumFrames; ++FrameIndex) {
        backtrace[FrameIndex] = (uint64)WinBackTrace[FrameIndex];
      }

      while (NumFrames < max_depth) {
        backtrace[NumFrames++] = 0;
      }
    }
#elif FUN_64_BIT
    // Raise an exception so CaptureStackBackTraceHelper has access to context
    // record.
    __try {
      RaiseException(
          0,  // Application-defined exception code.
          0,  // Zero indicates continuable exception.
          0,  // Number of arguments in args array (ignored if args is NULL)
          nullptr);  // Array of arguments
    }
    // Capture the back trace.
    __except (CaptureStackTraceHelper(
        backtrace, max_depth, (GetExceptionInformation())->ContextRecord)) {
    }
#elif 1
    // Use a bit of inline assembly to capture the information relevant to stack
    // walking which is basically EIP and EBP.
    CONTEXT helper_context;
    memset(&helper_context, 0, sizeof(CONTEXT));
    helper_context.ContextFlags = CONTEXT_FULL;

    // Use a fake function call to pop the return address and retrieve EIP.
    __asm {
      call FakeFunctionCall
    FakeFunctionCall:
      pop eax
      mov helper_context.Eip, eax
      mov helper_context.Ebp, ebp
      mov helper_context.Esp, esp
    }

    // Capture the back trace.
    CaptureStackTraceHelper(backtrace, max_depth, &helper_context);
#else
    CONTEXT helper_context;
    // RtlCaptureContext relies on EBP being untouched so if the below crashes
    // it is because frame pointer omission is enabled. It is implied by /Ox or
    // /O2 and needs to be manually disabled via /Oy-
    RtlCaptureContext(helper_context);

    // Capture the back trace.
    CaptureStackTraceHelper(backtrace, max_depth, &helper_context);
#endif
  }
}

PRAGMA_ENABLE_OPTIMIZATION

void StackwalkWin::ProgramCounterToSymbolInfo(
    uint64 ProgramCounter, CProgramCounterSymbolInfo& OutSymbolInfo) {
  // Initialize stack walking as it loads up symbol information which we
  // require.
  InitStackWalking();

  // Set the program counter.
  OutSymbolInfo.ProgramCounter = ProgramCounter;

  uint32 last_error = 0;
  HANDLE process_handle = GetCurrentProcess();

  // Initialize symbol.
  ANSICHAR SymbolBuffer[sizeof(IMAGEHLP_SYMBOL64) +
                        CProgramCounterSymbolInfo::MAX_NAME_LENGHT] = {0};
  IMAGEHLP_SYMBOL64* Symbol = (IMAGEHLP_SYMBOL64*)SymbolBuffer;
  Symbol->SizeOfStruct = sizeof(SymbolBuffer);
  Symbol->MaxNameLength = CProgramCounterSymbolInfo::MAX_NAME_LENGHT;

  // Get function name.
  if (SymGetSymFromAddr64(process_handle, ProgramCounter, nullptr, Symbol)) {
    // Skip any funky chars in the beginning of a function name.
    int32 Offset = 0;
    while (Symbol->Name[Offset] < 32 || Symbol->Name[Offset] > 127) {
      Offset++;
    }

    // Write out function name.
    CStringTraitsA::Strncpy(OutSymbolInfo.FunctionName, Symbol->Name + Offset,
                            CProgramCounterSymbolInfo::MAX_NAME_LENGHT);
    CStringTraitsA::Strncat(OutSymbolInfo.FunctionName, "()",
                            CProgramCounterSymbolInfo::MAX_NAME_LENGHT);
  } else {
    // No symbol found for this address.
    last_error = GetLastError();
  }

  // Get filename and line number.
  IMAGEHLP_LINE64 ImageHelpLine = {0};
  ImageHelpLine.SizeOfStruct = sizeof(ImageHelpLine);
  if (SymGetLineFromAddr64(process_handle, ProgramCounter,
                           (::DWORD*)&OutSymbolInfo.SymbolDisplacement,
                           &ImageHelpLine)) {
    CStringTraitsA::Strncpy(OutSymbolInfo.Filename, ImageHelpLine.FileName,
                            CProgramCounterSymbolInfo::MAX_NAME_LENGHT);
    OutSymbolInfo.LineNumber = ImageHelpLine.LineNumber;
  } else {
    last_error = GetLastError();
  }

  // Get module name.
  IMAGEHLP_MODULE64 ImageHelpModule = {0};
  ImageHelpModule.SizeOfStruct = sizeof(ImageHelpModule);
  if (SymGetModuleInfo64(process_handle, ProgramCounter, &ImageHelpModule)) {
    // Write out module information.
    CStringTraitsA::Strncpy(OutSymbolInfo.ModuleName, ImageHelpModule.ImageName,
                            CProgramCounterSymbolInfo::MAX_NAME_LENGHT);
  } else {
    last_error = GetLastError();
  }
}

// Get process module handle NULL-terminated list.
// On error this method returns NULL.
//
// IMPORTANT: Returned value must be deallocated by UnsafeMemory::Free().
static HMODULE* GetProcessModules(HANDLE process_handle) {
  const int32 NumModules = StackwalkWin::GetProcessModuleCount();
  // Allocate start size (last element reserved for NULL value)
  uint32 ResultBytes = NumModules * sizeof(HMODULE);
  HMODULE* ResultData =
      (HMODULE*)UnsafeMemory::Malloc(ResultBytes + sizeof(HMODULE));

  uint32 BytesRequired = 0;
  if (!FEnumProcessModules(process_handle, ResultData, ResultBytes,
                           (::DWORD*)&BytesRequired)) {
    UnsafeMemory::Free(ResultData);
    // Can't get process module list
    return nullptr;
  }

  if (BytesRequired <= ResultBytes) {
    // Add end module list marker
    ResultData[BytesRequired / sizeof(HMODULE)] = nullptr;
    return ResultData;
  }

  // No enough memory?
  return nullptr;
}

// Upload locally built symbols to network symbol storage.
//
// Use case:
//   Game designers use game from source (without prebuild game .dll-files).
//   In this case all game .dll-files are compiled locally.
//   For post-mortem debug programmers need .dll and .pdb files from designers.
bool StackwalkWin::UploadLocalSymbols() {
  InitStackWalking();

  // Upload locally compiled files to symbol storage.
  String SymbolStorage;
  if (!g_config->GetString(CrashReporterSettings, TEXT("UploadSymbolsPath"),
                           SymbolStorage, GEditorPerProjectIni) ||
      SymbolStorage.IsEmpty()) {
    // Nothing to do.
    return true;
  }
  // Prepare String
  SymbolStorage.Replace(TEXT("/"), TEXT("\\"), CaseSensitivity::CaseSensitive);
  SymbolStorage = TEXT("SRV*") + SymbolStorage;

  int32 ErrorCode = 0;
  HANDLE process_handle = GetCurrentProcess();

  // Enumerate process modules.
  HMODULE* ModuleHandlePointer = GetProcessModules(process_handle);
  if (!ModuleHandlePointer) {
    ErrorCode = GetLastError();
    return false;
  }

#if FUN_WITH_EDITOR
  // Get FUN Engine Editor directory for detecting non-game editor binaries.
  String EnginePath = CPaths::ConvertRelativePathToFull(CPaths::EngineDir());
  CPaths::MakePlatformFilename(EnginePath);
#endif

  // Upload all locally built modules.
  for (int32 ModuleIndex = 0; ModuleHandlePointer[ModuleIndex]; ModuleIndex++) {
    WCHAR ImageName[MAX_PATH] = {0};
    FGetModuleFileNameEx(process_handle, ModuleHandlePointer[ModuleIndex],
                         ImageName, MAX_PATH);

#if FUN_WITH_EDITOR
    WCHAR RelativePath[MAX_PATH];
    // Skip binaries inside FUN Engine Editor directory (non-game editor
    // binaries)
    if (PathRelativePathTo(RelativePath, *EnginePath, FILE_ATTRIBUTE_DIRECTORY,
                           ImageName, 0) &&
        CCharTraits::Strncmp(RelativePath, TEXT("..\\"), 3)) {
      continue;
    }
#endif

    WCHAR DebugName[MAX_PATH];
    CCharTraits::Strcpy(DebugName, ImageName);

    if (PathRenameExtensionW(DebugName, L".pdb")) {
      // Upload only if found .pdb file
      if (PathFileExistsW(DebugName)) {
        // Upload original file
        fun_log(LogWindows, Trace, TEXT("Uploading to symbol storage: %s"),
                ImageName);
        if (!SymSrvStoreFileW(process_handle, *SymbolStorage, ImageName,
                              SYMSTOREOPT_PASS_IF_EXISTS)) {
          fun_log(LogWindows, Warning,
                  TEXT("Uploading to symbol storage failed: %s. Error: %d"),
                  ImageName, GetLastError());
        }

        // Upload debug symbols
        fun_log(LogWindows, Trace, TEXT("Uploading to symbol storage: %s"),
                DebugName);
        if (!SymSrvStoreFileW(process_handle, *SymbolStorage, DebugName,
                              SYMSTOREOPT_PASS_IF_EXISTS)) {
          fun_log(LogWindows, Warning,
                  TEXT("Uploading to symbol storage failed: %s. Error: %d"),
                  DebugName, GetLastError());
        }
      }
    }
  }
  return true;
}

// Loads modules for current process.
static void LoadProcessModules(const String& RemoteStorage) {
  int32 ErrorCode = 0;
  HANDLE process_handle = GetCurrentProcess();

  // Enumerate process modules.
  HMODULE* ModuleHandlePointer = GetProcessModules(process_handle);
  if (!ModuleHandlePointer) {
    ErrorCode = GetLastError();
    return;
  }

  // Load the modules.
  for (int32 ModuleIndex = 0; ModuleHandlePointer[ModuleIndex]; ModuleIndex++) {
    MODULEINFO ModuleInfo = {0};
    WCHAR ModuleName[CProgramCounterSymbolInfo::MAX_NAME_LENGHT] = {0};
    WCHAR ImageName[CProgramCounterSymbolInfo::MAX_NAME_LENGHT] = {0};
#if FUN_64_BIT
    static_assert(sizeof(MODULEINFO) == 24,
                  "Broken alignment for 64bit Windows include.");
#else
    static_assert(sizeof(MODULEINFO) == 12,
                  "Broken alignment for 32bit Windows include.");
#endif
    FGetModuleInformation(process_handle, ModuleHandlePointer[ModuleIndex],
                          &ModuleInfo, sizeof(ModuleInfo));
    FGetModuleFileNameEx(process_handle, ModuleHandlePointer[ModuleIndex],
                         ImageName, CProgramCounterSymbolInfo::MAX_NAME_LENGHT);
    FGetModuleBaseName(process_handle, ModuleHandlePointer[ModuleIndex],
                       ModuleName, CProgramCounterSymbolInfo::MAX_NAME_LENGHT);

    // Set the search path to find PDBs in the same folder as the DLL.
    WCHAR SearchPath[MAX_PATH] = {0};
    WCHAR* FileName = nullptr;
    const auto Result =
        GetFullPathNameW(ImageName, MAX_PATH, SearchPath, &FileName);

    String SearchPathList;
    if (Result != 0 && Result < MAX_PATH) {
      *FileName = 0;
      SearchPathList = SearchPath;
    }
    if (!RemoteStorage.IsEmpty()) {
      if (!SearchPathList.IsEmpty()) {
        SearchPathList.Append(';');
      }
      SearchPathList.Append(RemoteStorage);
    }

    SymSetSearchPathW(process_handle, *SearchPathList);

    // Load module.
    const DWORD64 BaseAddress =
        SymLoadModuleExW(process_handle, ModuleHandlePointer[ModuleIndex],
                         ImageName, ModuleName, (DWORD64)ModuleInfo.lpBaseOfDll,
                         (uint32)ModuleInfo.SizeOfImage, nullptr, 0);
    if (!BaseAddress) {
      ErrorCode = GetLastError();
      fun_log(LogWindows, Warning, TEXT("SymLoadModuleExW. Error: %d"),
              GetLastError());
    }
  }

  // Free the module handle pointer allocated in case the static array was
  // insufficient.
  UnsafeMemory::Free(ModuleHandlePointer);
}

int32 StackwalkWin::GetProcessModuleCount() {
  CPlatformStackWalk::InitStackWalking();

  HANDLE process_handle = GetCurrentProcess();
  uint32 BytesRequired = 0;

  // Enumerate process modules.
  bool bEnumProcessModulesSucceeded =
      FEnumProcessModules(process_handle, nullptr, 0, (::DWORD*)&BytesRequired);
  if (!bEnumProcessModulesSucceeded) {
    return 0;
  }

  // Find out how many modules we need to load modules for.
  const int32 ModuleCount = BytesRequired / sizeof(HMODULE);
  return ModuleCount;
}

int32 StackwalkWin::GetProcessModuleSignatures(
    CStackWalkModuleInfo* ModuleSignatures, const int32 ModuleSignaturesSize) {
  CPlatformStackWalk::InitStackWalking();

  HANDLE process_handle = GetCurrentProcess();

  // Enumerate process modules.
  HMODULE* ModuleHandlePointer = GetProcessModules(process_handle);
  if (!ModuleHandlePointer) {
    return 0;
  }

  // Find out how many modules we need to load modules for.
  IMAGEHLP_MODULEW64 Img = {0};
  Img.SizeOfStruct = sizeof(Img);

  int32 SignatureIndex = 0;

  // Load the modules.
  for (int32 ModuleIndex = 0; ModuleHandlePointer[ModuleIndex] &&
                              SignatureIndex < ModuleSignaturesSize;
       ModuleIndex++) {
    MODULEINFO ModuleInfo = {0};
    WCHAR ModuleName[MAX_PATH] = {0};
    WCHAR ImageName[MAX_PATH] = {0};
#if FUN_64_BIT
    static_assert(sizeof(MODULEINFO) == 24,
                  "Broken alignment for 64bit Windows include.");
#else
    static_assert(sizeof(MODULEINFO) == 12,
                  "Broken alignment for 32bit Windows include.");
#endif
    FGetModuleInformation(process_handle, ModuleHandlePointer[ModuleIndex],
                          &ModuleInfo, sizeof(ModuleInfo));
    FGetModuleFileNameEx(process_handle, ModuleHandlePointer[ModuleIndex],
                         ImageName, MAX_PATH);
    FGetModuleBaseName(process_handle, ModuleHandlePointer[ModuleIndex],
                       ModuleName, MAX_PATH);

    // Load module.
    if (SymGetModuleInfoW64(process_handle, (DWORD64)ModuleInfo.lpBaseOfDll,
                            &Img)) {
      CStackWalkModuleInfo Info = {0};
      Info.BaseOfImage = Img.BaseOfImage;
      CCharTraits::Strcpy(Info.ImageName, Img.ImageName);
      Info.ImageSize = Img.ImageSize;
      CCharTraits::Strcpy(Info.LoadedImageName, Img.LoadedImageName);
      CCharTraits::Strcpy(Info.ModuleName, Img.ModuleName);
      Info.PdbAge = Img.PdbAge;
      Info.PdbSig = Img.PdbSig;
      UnsafeMemory::Memcpy(&Info.PdbSig70, &Img.PdbSig70, sizeof(GUID));
      Info.TimeDateStamp = Img.TimeDateStamp;

      ModuleSignatures[SignatureIndex] = Info;
      ++SignatureIndex;
    }
  }

  // Free the module handle pointer allocated in case the static array was
  // insufficient.
  UnsafeMemory::Free(ModuleHandlePointer);

  return SignatureIndex;
}

// Callback from the modules system that the loaded modules have changed and we
// need to reload symbols.
static void OnModulesChanged(CName ModuleThatChanged,
                             EModuleChangeReason ReasonForChange) {
  GNeedToRefreshSymbols = true;
}

String StackwalkWin::GetDownstreamStorage() {
  String DownstreamStorage;
  if (g_config->GetString(CrashReporterSettings, TEXT("DownstreamStorage"),
                          DownstreamStorage, GEditorPerProjectIni) &&
      !DownstreamStorage.IsEmpty()) {
    DownstreamStorage =
        CPaths::ConvertRelativePathToFull(CPaths::RootDir(), DownstreamStorage);
  } else {
    DownstreamStorage = CPaths::ConvertRelativePathToFull(
        CPaths::EngineIntermediateDir(), TEXT("Symbols"));
  }
  CPaths::MakePlatformFilename(DownstreamStorage);
  return DownstreamStorage;
}

// Create path symbol path.
// Reference:
// https://msdn.microsoft.com/en-us/library/ms681416%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396
static String GetRemoteStorage(const String& DownstreamStorage) {
  TArray<String> RemoteStorage;
  g_config->GetArray(CrashReporterSettings, TEXT("RemoteStorage"),
                     RemoteStorage, GEditorPerProjectIni);
  if (RemoteStorage.Num() > 0) {
    String SymbolStorage;
    for (int StorageIndex = 0; StorageIndex < RemoteStorage.Num();
         ++StorageIndex) {
      if (StorageIndex > 0) {
        SymbolStorage.Append(';');
      }
      SymbolStorage.Append(TEXT("SRV*"));
      SymbolStorage.Append(DownstreamStorage);
      SymbolStorage.Append('*');
      SymbolStorage.Append(RemoteStorage[StorageIndex]);
    }
    return SymbolStorage;
  } else {
    return String();
  }
}

bool StackwalkWin::InitStackWalking() {
  // DbgHelp functions are not thread safe, but this function can potentially be
  // called from different threads in our engine, so we take a critical section
  static CCriticalSection CriticalSection;
  CScopedLock Lock(CriticalSection);

  // Only initialize once.
  if (!GStackWalkingInitialized) {
    void* DllHandle = CPlatformProcess::GetDllHandle(TEXT("PSAPI.DLL"));
    if (DllHandle == nullptr) {
      return false;
    }

    // Load dynamically linked PSAPI routines.
    FEnumProcesses = (TFEnumProcesses)CPlatformProcess::GetDllExport(
        DllHandle, TEXT("EnumProcesses"));
    FEnumProcessModules = (TFEnumProcessModules)CPlatformProcess::GetDllExport(
        DllHandle, TEXT("EnumProcessModules"));
    FGetModuleFileNameEx =
        (TFGetModuleFileNameEx)CPlatformProcess::GetDllExport(
            DllHandle, TEXT("GetModuleFileNameExW"));
    FGetModuleBaseName = (TFGetModuleBaseName)CPlatformProcess::GetDllExport(
        DllHandle, TEXT("GetModuleBaseNameW"));
    FGetModuleInformation =
        (TFGetModuleInformation)CPlatformProcess::GetDllExport(
            DllHandle, TEXT("GetModuleInformation"));

    // Abort if we can't look up the functions.
    if (!FEnumProcesses || !FEnumProcessModules || !FGetModuleFileNameEx ||
        !FGetModuleBaseName || !FGetModuleInformation) {
      return false;
    }

    // Set up the symbol engine.
    uint32 SymOpts = SymGetOptions();

    SymOpts |= SYMOPT_LOAD_LINES;
    SymOpts |= SYMOPT_FAIL_CRITICAL_ERRORS;
    SymOpts |= SYMOPT_DEFERRED_LOADS;
    SymOpts |= SYMOPT_EXACT_SYMBOLS;

    // This option allows for undecorated names to be handled by the symbol
    // engine.
    SymOpts |= SYMOPT_UNDNAME;

    // Disable by default as it can be very spammy/slow.  Turn it on if you are
    // debugging symbol look-up!
    //      SymOpts |= SYMOPT_DEBUG;

    // Not sure these are important or desirable
    //      SymOpts |= SYMOPT_ALLOW_ABSOLUTE_SYMBOLS;
    //      SymOpts |= SYMOPT_CASE_INSENSITIVE;

    SymSetOptions(SymOpts);

    // Initialize the symbol engine.
    const String RemoteStorage = GetRemoteStorage(GetDownstreamStorage());
    SymInitializeW(GetCurrentProcess(),
                   RemoteStorage.IsEmpty() ? nullptr : *RemoteStorage, true);

    GNeedToRefreshSymbols = false;
    GStackWalkingInitialized = true;
  }
#if WINVER > 0x502
  else if (GNeedToRefreshSymbols) {
    // Refresh and reload symbols
    SymRefreshModuleList(GetCurrentProcess());
    GNeedToRefreshSymbols = false;
  }
#endif

  return GStackWalkingInitialized;
}

void StackwalkWin::RegisterOnModulesChanged() {
  // Register for callback so we can reload symbols when new modules are loaded
  CModuleManager::Get().OnModulesChanged().AddStatic(&OnModulesChanged);
}

}  // namespace fun
