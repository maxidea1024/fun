//TODO 코드정리
#pragma once

//@note VS2015에서 경고가 나오므로, 임시로 꺼줌.
#if defined(_MSC_VER) && _MSC_VER >= 1900
#pragma warning(push)
#pragma warning(disable : 4091) //warning C4091 : 'typedef ' : 변수를 선언하지 않으면 '' 왼쪽은 무시됩니다.
#endif

#include <dbghelp.h>

#if defined(_MSC_VER) && _MSC_VER >= 1900
#pragma warning(pop)
#endif

FUN_BEGIN_NAMESPACE

class IMiniDumpDelegate;

class MiniDumper : protected Singleton<MiniDumper>
{
 public:
  NETENGINE_API MiniDumper();
  NETENGINE_API ~MiniDumper();

  NETENGINE_API void SetUnhandledExceptionHandler();
  NETENGINE_API void ManualFullDump();
  NETENGINE_API void ManualMiniDump();

  NETENGINE_API void SetDelegate(IMiniDumpDelegate* delegate);

  NETENGINE_API static MiniDumper& Get()
  {
    return Singleton<MiniDumper>::Get();
  }

  NETENGINE_API static void WriteDumpFromHere(const char* filename, bool full_dump = false);
  //NETENGINE_API static void WriteDumpAtExceptionThrower(const char* filename, bool full_dump = false);

 private:
  CCriticalSection2 filter_working_mutex_;
  FUN_ALIGNED_VOLATILE LONG hit_count_;

  static LONG WINAPI TopLevelFilter(struct _EXCEPTION_POINTERS* exception_info);

  IMiniDumpDelegate* delegate_;

  static DWORD __stdcall StaticCreateDumpStackOverflow(void* context);

 protected:
  char dump_file_path_[2048];

  void DumpWithFlags(MINIDUMP_TYPE minidump_flags);
};

class IMiniDumpDelegate
{
 public:
  virtual ~IMiniDumpDelegate() {}

  virtual void GetDumpFilePath(char* output) = 0;

  virtual void AfterWriteDumpDone() = 0;

  virtual MINIDUMP_TYPE GetMiniDumpType() = 0;
};

#define SmallMiniDumpType  MiniDumpNormal // MiniDumpWithDataSegs가 가장 쓸만하다곤 하지만 6MB나 되니까 유저들이 잘 안보낸다.

class IExceptionLoggerDelegate
{
 public:
  virtual ~IExceptionLoggerDelegate() {}

  virtual String GetDumpDirectory() = 0;
};

class ExceptionLogger : protected Singleton<ExceptionLogger>
{
 public:
  void Init(IExceptionLoggerDelegate* delegate);

  NETENGINE_API static ExceptionLogger& Get()
  {
    return Singleton<ExceptionLogger>::Get();
  }

 private:
  /** Critical section. */
  CCriticalSection2 mutex_;

  /** Directory. */
  String directory_;

  /** Seriali number for dump. */
  uint32 dump_serial_;

  /** Tick value at last logged. */
  uint32 last_logged_tick_;

  /** Delegate. */
  IExceptionLoggerDelegate* delegate_;

  static LONG CALLBACK ExceptionLoggerProc(PEXCEPTION_POINTERS exception_info);
};

} // namespace net
} // namespace fun
