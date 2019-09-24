#pragma once

#include "fun/base/base.h"
#include "fun/base/event.h"
#include "fun/base/mutex.h"
#include "fun/base/optional.h"
#include "fun/base/ref_counted.h"

#include <unistd.h>
//#include <vector>
//#include <map>

#include "fun/base/container/array.h"
#include "fun/base/container/map.h"

namespace fun {

class Pipe;

class FUN_BASE_API ProcessHandleImpl : public RefCountedObject {
 public:
  ProcessHandleImpl(pid_t pid);
  ~ProcessHandleImpl();

  pid_t GetId() const;
  int Wait() const;
  int Wait(int options) const;

 private:
  const pid_t pid_;
  mutable FastMutex mutex_;
  mutable Event event_;
  mutable Optional<int> status_;
};

class FUN_BASE_API ProcessImpl {
 public:
  typedef pid_t PIDImpl;
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

 private:
  static ProcessHandleImpl* LaunchByForkExecImpl( const String& command,
                                                  const ArgsImpl& args,
                                                  const String& initial_directory,
                                                  Pipe* in_pipe,
                                                  Pipe* out_pipe,
                                                  Pipe* err_pipe,
                                                  const EnvImpl& env);
};

} // namespace fun
