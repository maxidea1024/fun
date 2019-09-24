//TODO 코드정리 혹은 deprecation
#include "NetEnginePrivate.h"

#include "MiniDumper.h"

#pragma comment(lib, "DbgHelp.lib")

namespace fun {
namespace net {

MiniDumper::MiniDumper()
{
  hit_count_ = 0;
  delegate_ = nullptr;
}

MiniDumper::~MiniDumper()
{
}

// 직접 호출하지 말것
// 프로그램 오류 발생시 호출되는 함수
LONG MiniDumper::TopLevelFilter(_EXCEPTION_POINTERS* pExceptionInfo)
{
  CScopedLock2 guard(Get().filter_working_mutex_);

  // 만약 아래 구문도 실패할 경우를 고려, 문제 재발시 무시하도록 한다.
  LONG a = ::InterlockedIncrement(&Get().hit_count_);
  if (a > 1) {
    return EXCEPTION_CONTINUE_SEARCH;
  }

  MiniDumper* d = &Get();
  IMiniDumpDelegate* dg = d->delegate_;
  // IMiniDumpDelegate 가 유효하지 않으면 debug assert fail을 발생한다.
  fun_check(dg != nullptr);
  dg->GetDumpFilePath(Get().dump_file_path_);

  // stack overflow exception의 경우 새로운 스레드를 생성해서 덤프를 작성한다.
  if (pExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW) {
    _MINIDUMP_EXCEPTION_INFORMATION ex_info;

    ex_info.ThreadId = ::GetCurrentThreadId();
    ex_info.ExceptionPointers = pExceptionInfo;
    ex_info.ClientPointers = FALSE;

    DWORD ThreadId;
    HANDLE ThreadHandle = ::CreateThread(nullptr, 0, StaticCreateDumpStackOverflow, &ex_info, 0, &ThreadId);

    // 생성 스레드가 덤프 끝날때까지 대기한다.
    WaitForSingleObject(ThreadHandle, INFINITE);

    CloseHandle(ThreadHandle);
  }
  else {
    // create the file
    HANDLE file_handle = ::CreateFile(Get().dump_file_path_, GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file_handle != INVALID_HANDLE_VALUE) {
      _MINIDUMP_EXCEPTION_INFORMATION ex_info;

      ex_info.ThreadId = ::GetCurrentThreadId();
      ex_info.ExceptionPointers = pExceptionInfo;
      ex_info.ClientPointers = FALSE;

      // 덤프 파일을 일단 쌓기 시작하면 굳이 오류 창을 표시하게 할 필요가 없다. 따라서 막아버린다.
      SetErrorMode(SEM_NOGPFAULTERRORBOX);

      // write the dump
      BOOL ok = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file_handle, Get().delegate_->GetMiniDumpType(), &ex_info, nullptr, nullptr);

      ::CloseHandle(file_handle);

      if (ok) {
        Get().delegate_->AfterWriteDumpDone();
      }
    }
    else {
      // why?
      _tprintf(TEXT("MiniDumper::TopLevelFilter:  could not open file to writing. filename='%s'\n"), Get().dump_file_path_);
    }

  }
  //return EXCEPTION_CONTINUE_SEARCH;
  return EXCEPTION_CONTINUE_EXECUTION; // 예전에는 EXCEPTION_EXECUTE_HANDLER
}

DWORD __stdcall MiniDumper::StaticCreateDumpStackOverflow(void* ctx)
{
  SetThreadName(GetCurrentThreadId(), TEXT("CreateDump StackOverflow"));

  _MINIDUMP_EXCEPTION_INFORMATION* ex_info = (_MINIDUMP_EXCEPTION_INFORMATION*)ctx;

  // create the file
  HANDLE file_handle = ::CreateFile(Get().dump_file_path_, GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file_handle != INVALID_HANDLE_VALUE) {
    // 덤프 파일을 일단 쌓기 시작하면 굳이 오류 창을 표시하게 할 필요가 없다. 따라서 막아버린다.
    SetErrorMode(SEM_NOGPFAULTERRORBOX);

    // write the dump
    BOOL ok = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file_handle, Get().delegate_->GetMiniDumpType(), ex_info, nullptr, nullptr);

    ::CloseHandle(file_handle);

    if (ok) {
      Get().delegate_->AfterWriteDumpDone();
    }
  }

  return 333;
}

void MiniDumper::DumpWithFlags(MINIDUMP_TYPE mini_dump_type)
{
  CScopedLock2 Lock(filter_working_mutex_);

  delegate_->GetDumpFilePath(dump_file_path_);

  // create the file
  HANDLE file_handle = ::CreateFile(dump_file_path_, GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file_handle != INVALID_HANDLE_VALUE) {
    // 덤프 파일을 일단 쌓기 시작하면 굳이 오류 창을 표시하게 할 필요가 없다. 따라서 막아버린다.
    SetErrorMode(SEM_NOGPFAULTERRORBOX);

    // write the dump
    BOOL ok = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file_handle, mini_dump_type, nullptr, nullptr, nullptr);

    ::CloseHandle(file_handle);
  }
}

void MiniDumper::SetUnhandledExceptionHandler()
{
  void* a = ::SetUnhandledExceptionFilter(TopLevelFilter);
  //void* a = Kernel32Api::Get().AddVectoredExceptionHandler(0, TopLevelFilter);
}

/** 유저 호출에 의해 미니 덤프 파일을 생성한다. 단, 메모리 전체 덤프를 하므로 용량이 큰 파일이 생성된다. */
void MiniDumper::ManualFullDump()
{
  DumpWithFlags(MiniDumpWithFullMemory);
}

/** 유저 호출에 의해 미니 덤프 파일을 생성한다. */
void MiniDumper::ManualMiniDump()
{
  DumpWithFlags(SmallMiniDumpType);
}

/** 초기화한다. 거의 프로그램 실행 초기에 이게 1회 호출되어야 미니 덤프가 작동한다. */
void MiniDumper::SetDelegate(IMiniDumpDelegate* delegate)
{
  delegate_ = delegate;
}

static int32 GetCurrentThreadExecutionPoint(uint32 Code, _EXCEPTION_POINTERS* ex_ptr, HANDLE file_handle)
{
  _MINIDUMP_EXCEPTION_INFORMATION ex_info;
  ex_info.ThreadId = ::GetCurrentThreadId();
  ex_info.ClientPointers = FALSE;

  ex_info.ExceptionPointers = ex_ptr;

  // write the dump
  BOOL ok = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file_handle, MiniDumpNormal, &ex_info, nullptr, nullptr);

  return EXCEPTION_EXECUTE_HANDLER;
}

void MiniDumper::WriteDumpFromHere(const char* Filename, bool bFullDump)
{
  // 덤프 파일을 일단 쌓기 시작하면 굳이 오류 창을 표시하게 할 필요가 없다. 따라서 막아버린다.
  SetErrorMode(SEM_NOGPFAULTERRORBOX);

  // create the file
  HANDLE file_handle = ::CreateFile(Filename, GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

  if (file_handle != INVALID_HANDLE_VALUE) {
    // 현재 스레드의 콜스택 수집
    __try {
      char* X = nullptr;
      *X = 1; //일부러
    }
    __except(GetCurrentThreadExecutionPoint(GetExceptionCode(), GetExceptionInformation(), file_handle)) {
    }

    ::CloseHandle(file_handle);
  }
}

//  void MiniDumper::WriteDumpAtExceptionThrower(const char* fileName, bool fullDump)
//  {
//    // 덤프 파일을 일단 쌓기 시작하면 굳이 오류 창을 표시하게 할 필요가 없다. 따라서 막아버린다.
//    SetErrorMode(SEM_NOGPFAULTERRORBOX);
//
//    // create the file
//    HANDLE file_handle = ::CreateFile(fileName, GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS,
//      FILE_ATTRIBUTE_NORMAL, nullptr);
//
//    if (file_handle != INVALID_HANDLE_VALUE) {
//      _MINIDUMP_EXCEPTION_INFORMATION ex_info;
//      ex_info.ThreadId = ::GetCurrentThreadId();
//      ex_info.ClientPointers = nullptr;
//
//      ex_info.ExceptionPointers = GetExceptionInformation();
//
//      // write the dump
//      BOOL ok = MiniDumpWriteDump(GetCurrentProcess(),
//        GetCurrentProcessId(), file_handle,
//        MiniDumpNormal,
//        &ex_info, nullptr, nullptr);
//
//      ::CloseHandle(file_handle);
//    }
//
//  }


LONG CALLBACK ExceptionLogger::ExceptionLoggerProc(PEXCEPTION_POINTERS pExceptionInfo)
{
#ifdef _DEBUG
  // 디버깅을 하고자 할 때는 non-debugger로 실행 후 아래 라인에서 멈추면 debugger attach를 하면 땡.
  //::MessageBox(nullptr, TEXT("XXX"), TEXT("XXX"), MB_OK);
#endif

  // 예외가 발생한 순간의 호출 스택을 담은 DMP 파일을 생성한다.
  CScopedLock2 Lock(ExceptionLogger::Get().CS);

  // 가장 마지막에 덤프한 이후 너무 짧은 시간 내면 덤프를 무시한다. 이게 없으면 지나치게 많은 예외
  // 발생시 정상적인 실행이 불가능할 정도로 느려진다.
  DWORD CurrTick = GetTickCount();
  if (CurrTick - ExceptionLogger::Get().last_logged_tick_ >= 10000) {
    ExceptionLogger::Get().last_logged_tick_ = CurrTick;

    ExceptionLogger::Get().dump_serial_++;

    char DmpFilePath[9999];
#if (_MSC_VER >= 1400)
    swprintf_s(DmpFilePath, 9998, TEXT("%s\\DUMP%d.DMP"), (const char*)ExceptionLogger::Get().directory_, ExceptionLogger::Get().dump_serial_);
#else
    swprintf(DmpFilePath, TEXT("%s\\DUMP%d.DMP"), (const char*)ExceptionLogger::Get().directory_, ExceptionLogger::Get().dump_serial_);
#endif
    // create the file
    HANDLE file_handle = ::CreateFile(DmpFilePath, GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file_handle != INVALID_HANDLE_VALUE) {
      _MINIDUMP_EXCEPTION_INFORMATION ex_info;

      ex_info.ThreadId = ::GetCurrentThreadId();
      ex_info.ExceptionPointers = pExceptionInfo;
      ex_info.ClientPointers = FALSE;

      // 덤프 파일을 일단 쌓기 시작하면 굳이 오류 창을 표시하게 할 필요가 없다. 따라서 막아버린다.
      SetErrorMode(SEM_NOGPFAULTERRORBOX);

      // write the dump
      BOOL ok = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), file_handle, MiniDumpNormal, &ex_info, nullptr, nullptr);

      ::CloseHandle(file_handle);
    }
  }
  return EXCEPTION_CONTINUE_SEARCH;
}

void ExceptionLogger::Init(IExceptionLoggerDelegate* delegate)
{
  static const char* ErrorText = TEXT("This feature requires Windows XP or 2003 Server!");

  if (Kernel32Api::Get().AddVectoredExceptionHandler == nullptr) {
    LOG(LogNetEngine, Error, ErrorText);
    throw ErrorText;
  }

  // 덤프 파일이 쌓일 폴더를 생성한다.
  delegate_ = delegate;
  dump_serial_ = 0;
  last_logged_tick_ = 0;

  CreateFullDirectory(delegate_->GetDumpDirectory(), directory_);

  if (directory_.GetLength() > 0 && directory_[directory_.GetLength()-1] == TEXT('\\')) {
    directory_ = directory_.Left(directory_.GetLength()-1);
  }

  // 이 기능은 Windows 2003 또는 Windows XP 이상에서만 작동한다.
  Kernel32Api::Get().AddVectoredExceptionHandler(1, ExceptionLoggerProc);
}

} // namespace net
} // namespace fun
