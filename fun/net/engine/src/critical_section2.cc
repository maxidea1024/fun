//@deprecated 플랫폼 중립적으로 재작업해야함.
#include "fun/net/net.h"
#include <assert.h>

namespace fun {
namespace net {

enum { ValidKeyValue = 0x75067506 };

#ifndef CRITICAL_SECTION_NO_DEBUG_INFO
#define CRITICAL_SECTION_NO_DEBUG_INFO  0x01000000
#endif

CCriticalSection2::CCriticalSection2(bool bFasterForWindows6) {
  bNeverCallDtor = false;
  ValidKey = ValidKeyValue;

  // 디버그 빌드에서는 faster critsec은 사용하지 않음.
#ifdef _DEBUG
  bFasterForWindows6 = false;
#endif

  if (bFasterForWindows6) { // CSingleton에서는 이걸 접근할 때 false이므로 아래 Kernel32Api 싱글톤을 또 접근하는 우가 없음
    if (Kernel32Api::Get().InitializeCriticalSectionEx != nullptr) { // 반드시 이걸 나중에 체크해야 한다. Instance를 접근하는 순간 critsec을 접근하므로!
      if (!Kernel32Api::Get().InitializeCriticalSectionEx(&CS, 0, bFasterForWindows6 ? CRITICAL_SECTION_NO_DEBUG_INFO : 0)) {
        throw Exception(TEXT("InitializeCriticalSectionEx failure."), __FUNCTION__);
      }
      else {
        return;
      }
    }
  }

  InitializeCriticalSection(&CS);
}

CCriticalSection2::~CCriticalSection2() {
  if (bNeverCallDtor) {
    return;
  }

  if (IsCriticalSectionLocked(CS)) {
    //throw Exception(TEXT("critical-section is still in use. destruction may cause problems."));
    fun_check(0);
  }
  DeleteCriticalSection(&CS);

  ValidKey = 0;

#ifdef _DEBUG
  UnsafeMemory::Memset(&CS, 0x00, sizeof(CS)); // 이렇게 지워버려야 디버깅중 무효화된 객체인지 체크가 가능하다.
#endif
}

bool CCriticalSection2::IsLockedByCurrentThread() {
  return IsCriticalSectionLockedByCurrentThread(*this);
}

void CCriticalSection2::Lock() {
#ifdef USE_DETECTING_LOCK
  Lock_DetectLongLockingRoutine();
#else
  if (ValidKey != ValidKeyValue) {
    //LOG(LogNetEngine, Warning, TEXT("cannot enter critical-section which has been already destroyed! NOTE: this may be solved by deleting NetClient instance before your WinMain() finishes working."));
    assert(0);
  }

  EnterCriticalSection(&CS);
#endif
}

void CCriticalSection2::UnsafeLock()
{
  if (ValidKey == ValidKeyValue) {
    EnterCriticalSection(&CS);
  }
  else {
    // ignore unsafely
  }
}

bool CCriticalSection2::TryLock()
{
  if (ValidKey != ValidKeyValue) {
    //LOG(LogNetEngine, Warning, TEXT("lock invalid critical-section is prohibited! NOTE: this may be solved by manually destroying FunNet related objects if they are defined as global objects."));
    assert(0);
  }
  return TryEnterCriticalSection(&CS) ? true : false;
}

void CCriticalSection2::Unlock()
{
#ifdef USE_DETECTING_LOCK
  Unlock_DetectLongLockingRoutine();
#else

  // unlock 단계에서는 체크하지 말자. safe lock, unsafe lock 중 어느걸 했는지 모르므로,
  // lock에서 엄한 사태인데 unlock이 무슨 의미리요.
  // 게다가 파괴자에서 throw? 이거 짜증남 따라서 즐. 특히 BST 케이스에서 이게 문제됐다!

  //if (ValidKey != ValidKeyValue) {
  //    throw Exception(TEXT("unlock invalid critical-section is prohibited."));
  //}
  //else
  {
    LeaveCriticalSection(&CS);
  }
#endif
}

void CCriticalSection2::Lock_DetectLongPreviousLockingThread()
{
  DWORD t0 = GetTickCount();
  HANDLE PrevThread = CS.OwningThread;
  EnterCriticalSection(&CS);
  DWORD t1 = GetTickCount();
  if (t1 - t0 > 1)
  {
    //printf("허거덕! 소요시간=%d밀리초,예전 스레드=%d\n", t1 - t0, (int)PrevThread);
    fun_check(0);
  }
}

__declspec(thread) DWORD CriticalSection_FirstLockTime;

void CCriticalSection2::Lock_DetectLongLockingRoutine() {
  bool bFirstLock = false;
  if (IsLockedByCurrentThread() == false) {
    bFirstLock = true;
  }

  EnterCriticalSection(&CS);

  if (bFirstLock) {
    CriticalSection_FirstLockTime = GetTickCount();
  }
}

void CCriticalSection2::Unlock_DetectLongLockingRoutine() {
  LeaveCriticalSection(&CS);

  if (IsLockedByCurrentThread() == false) {
    DWORD elapsed_time = GetTickCount() - CriticalSection_FirstLockTime;
    if (elapsed_time > 20) {
      //printf("허거덕! 소요시간=%d밀리초\n", elapsed_time);
      DebugBreak();
    }
  }
}

//void CCriticalSection2::SetName(const string& Name)
//{
//  Name = name;
//}

bool CCriticalSection2::IsValid() {
  return ValidKeyValue == ValidKey;
}

#pragma warning(disable:4312)

// Critical Section을 이 함수를 호출하는 스레드가 Lock했는가 검사한다.
// for x86 NT/2000 only
bool IsCriticalSectionLockedByCurrentThread(const CRITICAL_SECTION& CS) {
  //fun_check(IsIocpSupported()); // for NT/2000 only
  return CS.OwningThread == (HANDLE)GetCurrentThreadId();
}

bool IsCriticalSectionLocked(const CRITICAL_SECTION& CS) {
  //fun_check(IsIocpSupported()); 막은 이유: 이 함수가 CKernel32API singleton을 접근하니까.
  return CS.OwningThread != nullptr && CS.OwningThread != INVALID_HANDLE_VALUE;
}

uint32 GetAppropriateSpinCount() {
  // 테스트 해보니, 하이퍼 스레딩시에도 0을 리턴해야
  // Cpu사용률이 줄어 든다. 5배정도 차이.
  // hyperthread를 쓸 경우 spin count를 설정하면 오히려 해악 (http://software.intel.com/en-us/articles/managing-lock-contention-large-and-small-critical-sections/)
  // TODO: hyperthread 여부를 감지하도록 수정하자. 이 함수는 자주 콜 됨을 감안해서.
  //if (IsHyperThreading())
  //  return 1000;
  //return 8000;

  //hyperthreading가 아니더라도, 0을 쓰는게 안정적이다.
  return 0;
}

void CCriticalSection2::AssertIsLockedByCurrentThread() {
  if (!IsLockedByCurrentThread()) {
    DebugBreak();
  }
}

void CCriticalSection2::AssertIsNotLockedByCurrentThread() {
  if (IsLockedByCurrentThread()) {
    DebugBreak();
  }
}

} // namespace net
} // namespace fun
