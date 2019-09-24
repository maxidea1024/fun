#pragma once

#include "fun/base/base.h"
#include "fun/base/ref_counted.h"
//#include <vector>
//#include <map>
#include "fun/base/windows_less.h"
#include "fun/base/container/array.h"
#include "fun/base/container/map.h"

namespace fun {

class Pipe;

class FUN_BASE_API ProcessHandleImpl : public RefCountedObject {
 public:
  ProcessHandleImpl(HANDLE process_handle, uint32 pid);
  ~ProcessHandleImpl();

  uint32 GetId() const;
  HANDLE GetHandle() const;
  int Wait() const;
  void CloseHandle();

  ProcessHandleImpl(const ProcessHandleImpl&) = delete;
  ProcessHandleImpl& operator = (const ProcessHandleImpl&) = delete;

 private:
  HANDLE process_handle_;
  uint32 pid_;
};

class FUN_BASE_API ProcessImpl {
 public:
  typedef uint32 PIDImpl;
  typedef Array<String> ArgsImpl;
  typedef Map<String, String> EnvImpl;

  static PIDImpl CurrentPidImpl();
  static void GetTimesImpl(long& user_time, long& kernel_time);
  static ProcessHandleImpl* LaunchImpl( const String& command,
                                        const ArgsImpl& args,
                                        const String& initial_directory,
                                        Pipe* in_pipe,
                                        Pipe* out_pipe,
                                        Pipe* err_pipe,
                                        const EnvImpl& env);
  static void KillImpl(ProcessHandleImpl& handle);
  static void KillImpl(PIDImpl pid);
  static bool IsRunningImpl(const ProcessHandleImpl& handle);
  static bool IsRunningImpl(PIDImpl pid);
  static void RequestTerminationImpl(PIDImpl pid);
  static String TerminationEventName(PIDImpl pid);
};

} // namespace fun
