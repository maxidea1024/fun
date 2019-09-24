//@deprecated  일단은 유지하자...
#pragma once

namespace fun {
namespace net {

//int32 GetNoofProcessors();
//size_t GetOccupiedPhysicalMemory();
//size_t GetTotalPhysicalMemory();
//String GetComputerName();
//bool EnableLowFragmentationHeap();
//void SetCurrentDirectoryWhereProgramExists();
//bool IsIocpSupported();
//bool IsServiceMode();

bool IsGqcsExSupported();

//bool IsHyperThreading();
//DWORD CountSetBits(ULONG_PTR BitMask);

typedef WINBASEAPI
  /*__out_opt*/
  HANDLE
  (WINAPI* CreateIoCompletionPortProc)(
  /*__in*/     HANDLE FileHandle,
  /*__in_opt*/ HANDLE ExistingCompletionPort,
  /*__in*/     ULONG_PTR CompletionKey,
  /*__in*/     DWORD NumberOfConcurrentThreads
  );

//typedef WINBASEAPI
//  BOOL
//  (WINAPI* HeapSetInformationProc) (
//  /*__in_opt*/ HANDLE HeapHandle,
//  /*__in*/ HEAP_INFORMATION_CLASS HeapInformationClass,
//  /*__in_bcount_opt HeapInformationLength*/ PVOID HeapInformation,
//  /*__in*/ size_t HeapInformationLength
//  );

//typedef WINBASEAPI
//  BOOL
//  (WINAPI* QueueUserWorkItemProc) (
//  /*__in*/     LPTHREAD_START_ROUTINE Function,
//  /*__in_opt*/ PVOID context,
//  /*__in*/     ULONG Flags
//  );

typedef WINBASEAPI
  BOOL
  (WINAPI* GetQueuedCompletionStatusProc) (
  /*__in*/  HANDLE completion_port_,
  /*__out*/ LPDWORD lpNumberOfBytesTransferred,
  /*__out*/ PULONG_PTR lpCompletionKey,
  /*__out*/ LPOVERLAPPED *lpOverlapped,
  /*__in*/  DWORD dwMilliseconds
  );

struct FUN_OVERLAPPED_ENTRY
{
  ULONG_PTR lpCompletionKey;
  LPOVERLAPPED lpOverlapped;
  ULONG_PTR Internal;
  DWORD dwNumberOfBytesTransferred;
};

typedef WINBASEAPI
  BOOL
  (WINAPI* GetQueuedCompletionStatusExProc) (
  /*__in  */HANDLE completion_port_,
  /*__out_ecount_part(ulCount, *ulNumEntriesRemoved)*/ FUN_OVERLAPPED_ENTRY* lpCompletionPortEntries,
  /*__in  */ULONG ulCount,
  /*__out */PULONG ulNumEntriesRemoved,
  /*__in  */DWORD dwMilliseconds,
  /*__in  */BOOL fAlertable
  );

typedef WINBASEAPI
  PVOID
  (WINAPI* AddVectoredExceptionHandlerProc) (
  /*__in*/          ULONG FirstHandler,
  /*__in*/          PVECTORED_EXCEPTION_HANDLER VectoredHandler
  );

//typedef WINBASEAPI
//  BOOL
//  (WINAPI* GetLogicalProcessorInformation)(
//  /*__out */  PSYSTEM_LOGICAL_PROCESSOR_INFORMATION Buffer,
//  /*))inout */ PDWORD ReturnLength
//  );

//String GetFullPath(const char* FileName);
//BOOL CreateFullDirectory(const char* PathName, String& OutCreatedDirectory);
//String GetCurrentDirectory();

//typedef VOID (NTAPI * WaitOrTimerCallbackProc) (PVOID lpParameter, BOOLEAN bTimerOrWaitFired);

//typedef WINBASEAPI
//  HANDLE
//  (WINAPI* CreateTimerQueueProc) (
//  VOID
//  );

//typedef WINBASEAPI
//  BOOL
//  (WINAPI* CreateTimerQueueTimerProc) (
//  /*__deref_out*/ PHANDLE phNewTimer,
//  /*__in_opt   */ HANDLE TimerQueue,
//  /*__in       */ WaitOrTimerCallbackProc Callback,
//  /*__in_opt   */ PVOID Parameter,
//  /*__in       */ DWORD DueTime,
//  /*__in       */ DWORD Period,
//  /*__in       */ ULONG Flags
//  ) ;


//typedef WINBASEAPI
//  BOOL
//  (WINAPI* DeleteTimerQueueTimerProc) (
//  /*__in_opt*/ HANDLE TimerQueue,
//  /*__in    */ HANDLE Timer,
//  /*__in_opt*/ HANDLE CompletionEvent
//  );

//typedef WINBASEAPI
//  BOOL
//  (WINAPI* DeleteTimerQueueExProc) (
//  /*__in    */ HANDLE TimerQueue,
//  /*__in_opt*/ HANDLE CompletionEvent
//  );

typedef WINBASEAPI
  BOOL
  (WINAPI* InitializeCriticalSectionExProc) (
  /*__out*/ LPCRITICAL_SECTION lpCriticalSection,
  /*__in*/  DWORD dwSpinCount,
  /*__in  */DWORD Flags
  );



struct Kernel32Api
{
  Kernel32Api();

  static Kernel32Api& Get();

  CreateIoCompletionPortProc CreateIoCompletionPort;
  //HeapSetInformationProc HeapSetInformation;
  //QueueUserWorkItemProc QueueUserWorkItem;
  GetQueuedCompletionStatusProc GetQueuedCompletionStatus;
  GetQueuedCompletionStatusExProc GetQueuedCompletionStatusEx;
  AddVectoredExceptionHandlerProc AddVectoredExceptionHandler;

  //CreateTimerQueueProc CreateTimerQueue;
  //CreateTimerQueueTimerProc CreateTimerQueueTimer;
  //DeleteTimerQueueTimerProc DeleteTimerQueueTimer;
  //DeleteTimerQueueExProc DeleteTimerQueueEx;
  InitializeCriticalSectionExProc InitializeCriticalSectionEx;
  //GetLogicalProcessorInformation GetLogicalProcessInformation;

  HINSTANCE kernel32_dll_handle_;
};

//struct CLocale : protected Singleton<CLocale>
//{
//  int32 LanguageId;
//
//  CLocale();
//
//  static CLocale& Get();
//};
//
//struct CSystemInfo : protected Singleton<CSystemInfo>
//{
//  SYSTEM_INFO Info;
//
//  CSystemInfo();
//
//  inline uint32 GetMemoryPageSize() const { return Info.dwPageSize; }
//
//  static CSystemInfo& Get();
//};

//void ShowUserMisuseError(const char* text);
//String GetProcessName();
//int32 GetTotalThreadCount();
//uint32 GetSysTicks();
//String GetSystemErrorMessage(int32 Error = 0);
//const char* GetSystemErrorMessage(char* OutBuffer, int32 BufferCount, int32 Error);

} // namespace net
} // namespace fun
