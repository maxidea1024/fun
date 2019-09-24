#pragma once

#include "fun/base/base.h"

#if FUN_PLATFORM_WINDOWS_FAMILY
#if defined(_WIN32_WCE)
#include "fun/base/process_wince.h"
#else
#include "fun/base/process_win32.h"
#endif
#elif FUN_PLATFORM != FUN_PLATFORM_VXWORKS
#include "fun/base/process_vx.h"
#elif FUN_PLATFORM_UNIX_FAMILY
#include "fun/base/process_unix.h"
#endif

namespace fun {

class Pipe;

class FUN_BASE_API ProcessHandle {
 public:
  typedef ProcessImpl::PIDImpl PID;

  ProcessHandle(const ProcessHandle& rhs);
  ~ProcessHandle();
  ProcessHandle& operator=(const ProcessHandle& rhs);

  PID GetId() const;

  int Wait() const;

 protected:
  ProcessHandle(ProcessHandleImpl* impl);

 private:
  ProcessHandle();

  ProcessHandleImpl* impl_;

  friend class Process;
};

class FUN_BASE_API Process : public ProcessImpl {
 public:
  typedef PIDImpl PID;
  typedef ArgsImpl Args;
  typedef EnvImpl Env;

  static PID CurrentPid();

  static void GetTimes(long& user_time, long& kernel_time);

  static ProcessHandle Launch(const String& command, const Args& args);

  static ProcessHandle Launch(const String& command, const Args& args,
                              const String& initial_directory);

  static ProcessHandle Launch(const String& command, const Args& args,
                              Pipe* in_pipe, Pipe* out_pipe, Pipe* err_pipe);

  static ProcessHandle Launch(const String& command, const Args& args,
                              const String& initial_directory, Pipe* in_pipe,
                              Pipe* out_pipe, Pipe* err_pipe);

  static ProcessHandle Launch(const String& command, const Args& args,
                              Pipe* in_pipe, Pipe* out_pipe, Pipe* err_pipe,
                              const Env& env);

  static ProcessHandle Launch(const String& command, const Args& args,
                              const String& initial_directory, Pipe* in_pipe,
                              Pipe* out_pipe, Pipe* err_pipe, const Env& env);

  static int Wait(const ProcessHandle& handle);

  static bool IsRunning(const ProcessHandle& handle);

  static bool IsRunning(PID pid);

  static void Kill(ProcessHandle& handle);

  static void Kill(PID pid);

  static void RequestTermination(PID pid);
};

//
// inlines
//

FUN_ALWAYS_INLINE Process::PID Process::CurrentPid() {
  return ProcessImpl::CurrentPidImpl();
}

FUN_ALWAYS_INLINE void Process::GetTimes(long& user_time, long& kernel_time) {
  ProcessImpl::GetTimesImpl(user_time, kernel_time);
}

}  // namespace fun
