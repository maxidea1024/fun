//@deprecated
//없애도록 하되, 대체가 가능한 부분이 있다면 찾아서 대체하도록 하자.
#include "SysUtil.h"
#include "fun/net/net.h"

#include <tlhelp32.h>

namespace fun {

//#define INFO_BUFFER_SIZE 1024
//
// int32 CWindowsMisc::NumberOfCoresIncludingHyperthreads()
// int32 GetNoofProcessors()
//{
//	SYSTEM_INFO SI;
//	GetSystemInfo(&SI);
//	return (int32)SI.dwNumberOfProcessors;
//}
//
// size_t GetOccupiedPhysicalMemory()
//{
//	MEMORYSTATUS MS;
//	GlobalMemoryStatus(&MS);
//	return MS.dwTotalPhys - MS.dwAvailPhys;
//}
//
// size_t GetTotalPhysicalMemory()
//{
//	MEMORYSTATUS MS;
//	GlobalMemoryStatus(&MS);
//	return MS.dwTotalPhys;
//}
//
// String GetComputerName()
//{
//	char Name[1024];
//	DWORD NameLen = COUNTOF(Name);
//	if (::GetComputerNameW(Name, &NameLen))
//	{
//		return Name;
//	}
//	else
//	{
//		return TEXT("");
//	}
//}
//
// void SetCurrentDirectoryWhereProgramExists()
//{
//	char ModuleFilename[_MAX_PATH];
//	GetModuleFileName(nullptr, ModuleFilename, _MAX_PATH);
//
//	char Path[_MAX_PATH];
//	char Drive[_MAX_PATH];
//	char Dir[_MAX_PATH];
//	char CName[_MAX_PATH];
//	char Ext[_MAX_PATH];
//#if (_MSC_VER >= 1400)
//	_tsplitpath_s(ModuleFilename, Drive, _MAX_PATH, Dir, _MAX_PATH, CName,
//_MAX_PATH, Ext, _MAX_PATH); 	_tmakepath_s(Path, _MAX_PATH, Drive, Dir,
//TEXT(""), TEXT("")); #else 	_tsplitpath(ModuleFilename, Drive, Dir, CName,
//Ext); 	_tmakepath(Path, Drive, Dir, TEXT(""), TEXT("")); #endif
//
//	SetCurrentDirectory(Path);
//}
//
// bool EnableLowFragmentationHeap()
//{
//	static FUN_ALIGNED_VOLATILE LONG Did = 0;
//
//	if (::InterlockedCompareExchange(&Did, 1, 0) == 0)
//	{
//		ULONG HeapFragValue = 2;
//
//		if (Kernel32Api::Get().HeapSetInformation != nullptr)
//		{
//			Kernel32Api::Get().HeapSetInformation(GetProcessHeap(),
//				HeapCompatibilityInformation,
//				&HeapFragValue,
//				sizeof(HeapFragValue));
//		}
//	}
//	return true;
//}
//
// bool IsIocpSupported()
//{
//	return Kernel32Api::Get().CreateIoCompletionPort != nullptr;
//}
//
// bool IsServiceMode()
//{
//	// 서비스 모드로 실행중인지 확인하기
//	char UserName[INFO_BUFFER_SIZE];
//	DWORD Size = INFO_BUFFER_SIZE;
//	if (::GetUserNameA(UserName, &Size) == FALSE)
//	{
//		return false;
//	}
//
//	if (strcmp(UserName, "SYSTEM") == 0 ||
//		strcmp(UserName, "NETWORK SERVICE") == 0 ||
//		strcmp(UserName, "LOCAL SERVICE") == 0)
//	{
//		return true;
//	}
//
//	return false;
//}

bool IsGqcsExSupported() {
  return Kernel32Api::Get().GetQueuedCompletionStatusEx != nullptr;
}

// DWORD CountSetBits(ULONG_PTR BitMask)
//{
//	DWORD LSHIFT = sizeof(ULONG_PTR) * 8 - 1;
//	DWORD BitSetCount = 0;
//	ULONG_PTR BitTest = (ULONG_PTR)1 << LSHIFT;
//	DWORD i;
//
//	for (i = 0; i <= LSHIFT; ++i)
//	{
//		BitSetCount += ((BitMask & BitTest) ? 1 : 0);
//		BitTest >>= 1;
//	}
//
//	return BitSetCount;
//}
//
// bool IsHyperThreading()
//{
//	if (Kernel32Api::Get().GetLogicalProcessInformation == nullptr)
//	{
//		return false;
//	}
//
//	static FUN_ALIGNED_VOLATILE DWORD ProcessorCoreCount = 0;
//	static FUN_ALIGNED_VOLATILE DWORD LogicalProcessorCount = 0;
//
//	if (ProcessorCoreCount != 0 && LogicalProcessorCount != 0)
//	{
//		return (ProcessorCoreCount != LogicalProcessorCount);
//	}
//
//	ProcessorCoreCount = 0;
//	LogicalProcessorCount = 0;
//
//	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION Buffer = nullptr;
//	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION Ptr = nullptr;
//	DWORD ReturnLength = 0;
//	DWORD ByteOffset = 0;
//
//	DWORD rc = Kernel32Api::Get().GetLogicalProcessInformation(Buffer,
//&ReturnLength); 	if (FALSE == rc)
//	{
//		if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
//		{
//			if (Buffer != nullptr)
//			{
//				free(Buffer);
//			}
//
//			Buffer =
//(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)::malloc(ReturnLength); 			if (Buffer ==
//nullptr)
//			{
//				return false;
//			}
//
//			rc = Kernel32Api::Get().GetLogicalProcessInformation(Buffer,
//&ReturnLength); 			if (FALSE == rc)
//			{
//				free(Buffer);
//				return false;
//			}
//		}
//		else
//		{
//			//실패라면 hyper-threading 없다고 가정.
//			return false;
//		}
//	}
//
//	Ptr = Buffer;
//	while (ByteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <=
//ReturnLength)
//	{
//		switch (Ptr->Relationship)
//		{
//		//주석 되어 있는것들은 필요하면 사용하도록 하자.
//		//case RelationNumaNode:
//		//	// Non-NUMA systems report a single record of this type.
//		//	numaNodeCount++;
//		//	break;
//
//		case RelationProcessorCore:
//			ProcessorCoreCount++;
//
//			// A hyperthreaded core supplies more than one logical
//processor. 			LogicalProcessorCount += CountSetBits(Ptr->ProcessorMask); 			break;
//
//		//case RelationCache:
//		//	// Cache data is in Ptr->Cache, one CACHE_DESCRIPTOR
//structure for each cache.
//		//	Cache = &Ptr->Cache;
//		//	if (Cache->Level == 1)
//		//	{
//		//		ProcessorL1CacheCount++;
//		//	}
//		//	else if (Cache->Level == 2)
//		//	{
//		//		ProcessorL2CacheCount++;
//		//	}
//		//	else if (Cache->Level == 3)
//		//	{
//		//		ProcessorL3CacheCount++;
//		//	}
//		//	break;
//
//		//case RelationProcessorPackage:
//		//	// Logical processors share a physical package.
//		//	processorPackageCount++;
//		//	break;
//
//		default:
//			break;
//		}
//		ByteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
//		Ptr++;
//	}
//	::free(Buffer);
//
//	return (ProcessorCoreCount != LogicalProcessorCount);
//}
//
// String GetFullPath(const char* fileName)
//{
//	//	CSingleLock Lock(&m_mutex, 1);
//
//	//@todo CPath of MFC
//	fun_check(0);
//	return String();
//
//	//CPath path(fileName);
//	//if (path.IsRelative())
//	//{
//	//	path.Combine(GetCurrentDirectory(), fileName);
//	//}
//	//return String(path);
//}
//
// String GetCurrentDirectory()
//{
//	char CWD[_MAX_PATH];
//	::GetCurrentDirectory(_MAX_PATH, CWD);
//	return CWD;
//}
//
// BOOL CreateDirectory(LPCTSTR Path)
//{
//	String FullPath = GetFullPath(Path);
//	return ::CreateDirectory(FullPath, 0);
//}
//
// BOOL CreateFullDirectory(const char* lpPathName, String& OutCreatedDirectory)
//{
//	String refinedPath;
//	refinedPath = GetFullPath(lpPathName);
//
//	OutCreatedDirectory = refinedPath;
//
//	int curPos = 0;
//
//	String incPath;
//	String resToken = refinedPath.Tokenize(TEXT("\\"), curPos);
//	while (resToken != TEXT(""))
//	{
//		incPath += resToken;
//		incPath += TEXT("\\");
//
//		if (!CreateDirectory(incPath) && GetLastError() !=
//ERROR_FILE_EXISTS && GetLastError() != ERROR_ALREADY_EXISTS)
//		{
//			return FALSE;
//		}
//
//		resToken = refinedPath.Tokenize(TEXT("\\"), curPos);
//	};
//
//	return TRUE;
//}
//
Kernel32Api::Kernel32Api() {
  kernel32_dll_handle_ = LoadLibraryA("kernel32.dll");
  if (kernel32_dll_handle_ == nullptr) {
    throw Exception(TEXT("Cannot find kernel32.dll handle!"));
  }

#pragma warning(push)
#pragma warning(disable : 4191)
  CreateIoCompletionPort = (CreateIoCompletionPortProc)GetProcAddress(
      kernel32_dll_handle_, "CreateIoCompletionPort");
  // HeapSetInformation =
  // (HeapSetInformationProc)GetProcAddress(kernel32_dll_handle_,
  // "HeapSetInformation"); QueueUserWorkItem =
  // (QueueUserWorkItemProc)GetProcAddress(kernel32_dll_handle_,
  // "QueueUserWorkItem");
  GetQueuedCompletionStatus = (GetQueuedCompletionStatusProc)GetProcAddress(
      kernel32_dll_handle_, "GetQueuedCompletionStatus");
  GetQueuedCompletionStatusEx = (GetQueuedCompletionStatusExProc)GetProcAddress(
      kernel32_dll_handle_, "GetQueuedCompletionStatusEx");
  AddVectoredExceptionHandler = (AddVectoredExceptionHandlerProc)GetProcAddress(
      kernel32_dll_handle_, "AddVectoredExceptionHandler");

  // CreateTimerQueue =
  // (CreateTimerQueueProc)GetProcAddress(kernel32_dll_handle_,
  // "CreateTimerQueue"); CreateTimerQueueTimer =
  // (CreateTimerQueueTimerProc)GetProcAddress(kernel32_dll_handle_,
  // "CreateTimerQueueTimer"); DeleteTimerQueueTimer =
  // (DeleteTimerQueueTimerProc)GetProcAddress(kernel32_dll_handle_,
  // "DeleteTimerQueueTimer"); DeleteTimerQueueEx =
  // (DeleteTimerQueueExProc)GetProcAddress(kernel32_dll_handle_,
  // "DeleteTimerQueueEx");
  InitializeCriticalSectionEx = (InitializeCriticalSectionExProc)GetProcAddress(
      kernel32_dll_handle_, "InitializeCriticalSectionEx");
  // GetLogicalProcessInformation =
  // (GetLogicalProcessorInformation)GetProcAddress(kernel32_dll_handle_,
  // "GetLogicalProcessorInformation");
#pragma warning(pop)
}

Kernel32Api& Kernel32Api::Get() {
  static Kernel32Api instance;
  // 여러스레드에서 동시다발적으로 ctor가 실행돼도 문제는 없음. 파괴자 루틴도
  // 없겠다, 따라서 그냥 이렇게 고고고.
  return instance;
}

// CLocale::CLocale()
//{
//	LanguageId = 0;
//
//	char Nation[7];
//	if (0 != GetLocaleInfoA(LOCALE_SYSTEM_DEFAULT, LOCALE_ICOUNTRY, Nation,
//7))
//	{
//		LanguageId = atoi(Nation);
//	}
//
//	/*		switch (LanguageId)
//	{
//	case 82:        // KOR(대한민국)
//	case 1:        // USA(미국)
//	case 7:        // RUS(러시아)
//	case 81:        // 일본
//	case 86:        // 중국
//	break;
//	}*/
//}
//
//
// CLocale& CLocale::Get()
//{
//	return __super::Get();
//}

// SystemInfo::CSystemInfo()
//
//	UnsafeMemory::Memzero(&Info);
//	::GetSystemInfo(&Info);
//
//
//
// SystemInfo& CSystemInfo::Get()
//
//	return __super::Get();
//

// FunNet 사용자가 실수한 경우에만 보여주는 에러다.
// void ShowUserMisuseError(const char* text)
//{
//	if (NetConfig::UserMisuseErrorReaction == EErrorReaction::MessageBox)
//	{
//		::OutputDebugString(text);
//
//		// 대화 상자를 표시한다.
//		::MessageBox(nullptr, text, TEXT("FunNet"), MB_OK|MB_ICONHAND);
//	}
//	else if (NetConfig::UserMisuseErrorReaction ==
//EErrorReaction::DebugOutput)
//	{
//		// DebugOutput으로 출력한다.
//		::OutputDebugString(text);
//	}
//	else
//	{
//		// 크래시를 유발한다.
//		//  아래 것은 덤프가 안잡힐때가 많다. 그러므로 크래시 유발은
//이코드로 하자. 		int32* X = nullptr; 		*X = 100;
//	}
//}

//
// String GetProcessName()
//{
//	static char ModuleFilename[_MAX_PATH] = {0};
//	if (ModuleFilename[0] == '\0')
//	{
//		GetModuleFileName(nullptr, ModuleFilename,
//COUNTOF(ModuleFilename));
//	}
//	return ModuleFilename;
//}
//
// int32 GetTotalThreadCount()
//{
//	//
//http://weseetips.com/2008/05/02/how-to-iterate-all-running-process-in-your-system/
//에서 발췌 	HANDLE hProcessSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL,
//0);
//
//	// Initialize the process entry structure.
//	PROCESSENTRY32 ProcessEntry = { 0 };
//	ProcessEntry.dwSize = sizeof(ProcessEntry);
//
//	// Get the first process info.
//	BOOL Return = FALSE;
//	Return = Process32First(hProcessSnapShot, &ProcessEntry);
//
//	// Getting process info failed.
//	if (!Return)
//	{
//		return -1;
//	}
//
//	DWORD PID = GetCurrentProcessId();
//	do
//	{
//		/*// print the process details.
//		cout << L"Process EXE File:" << ProcessEntry.szExeFile
//		<< endl;
//		cout << L"Process ID:" << ProcessEntry.th32ProcessID
//		<< endl;
//		cout << L"Process References:" << ProcessEntry.cntUsage
//		<< endl;
//		cout << L"Process Thread Count:" << ProcessEntry.cntThreads
//		<< endl;*/
//
//
//		// check the PROCESSENTRY32 for other members_.
//
//		if (ProcessEntry.th32ProcessID == PID)
//		{
//			return ProcessEntry.cntThreads;
//		}
//	}
//	while (Process32Next(hProcessSnapShot, &ProcessEntry));
//
//	// Close the handle
//	CloseHandle(hProcessSnapShot);
//
//	return -1;
//}
//
// uint32 GetSysTicks()
//{
//	return ::GetTickCount();
//}
//
// String GetSystemErrorMessage(int32 Error)
//{
//	String result;
//	GetSystemErrorMessage(result.GetBuffer(2048), 2048, Error);
//	result.ReleaseBuffer();
//	return result;
//}
//
//@maxidea: todo: platform issue
// const char* GetSystemErrorMessage(char* OutBuffer, int32 BufferCount, int32
// Error)
//{
//	fun_check(OutBuffer && BufferCount);
//	*OutBuffer = TEXT('\0');
//	if (Error == 0)
//	{
//		Error = GetLastError();
//	}
//
//	//FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, Error,
//MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), OutBuffer, BufferCount, nullptr);
//	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, Error,
//MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), OutBuffer, BufferCount, nullptr);
//	char* Found = _tcschr(OutBuffer, TEXT('\r'));
//	if (Found != nullptr)
//	{
//		*Found = TEXT('\0');
//	}
//	Found = _tcschr(OutBuffer, TEXT('\n'));
//	if (Found != nullptr)
//	{
//		*Found = TEXT('\0');
//	}
//	return OutBuffer;
//}

}  // namespace fun
