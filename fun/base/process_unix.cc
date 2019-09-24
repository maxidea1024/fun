#include "fun/base/process_unix.h"
#include "fun/base/exception.h"
#include "fun/base/pipe.h"
#include "fun/base/thread.h"

#include <limits>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/wait.h>

#if defined(__QNX__)
#include <process.h>
#include <spawn.h>
#include <cstring>
#endif

namespace fun {

//
// ProcessHandleImpl
//

ProcessHandleImpl::ProcessHandleImpl(pid_t pid)
  : pid_(pid),
    mutex_(),
    event_(EventResetType::Manual),
    status_() {}

ProcessHandleImpl::~ProcessHandleImpl() {}

pid_t ProcessHandleImpl::GetId() const {
  return pid_;
}

int ProcessHandleImpl::Wait() const {
  if (Wait(0) != pid_) {
    throw SystemException("Cannot wait for process", String::FromNumber(pid_));
  }

  const int status = status_.Value();
  if (WIFEXITED(status)) {
    return WEXITSTATUS(status);
  }
  if (WIFSIGNALED(status)) {
    return -WTERMSIG(status);
  }

  // This line should never be reached.
  return std::numeric_limits<int>::max();
}

int ProcessHandleImpl::Wait(int options) const {
  {
    ScopedLock<FastMutex> guard(mutex_);
    if (status_.IsSpecified()) {
      return pid_;
    }
  }

  int status;
  int rc;
  do {
    rc = waitpid(pid_, &status, options);
  } while (rc < 0 && errno == EINTR);

  if (rc == pid_) {
    ScopedLock<FastMutex> guard(mutex_);
    status_ = status;
    event_.Set();
  } else if (rc < 0 && errno == ECHILD) {
    // Looks like another thread was lucky and it should update the status for us shortly
    event_.Wait();

    ScopedLock<FastMutex> guard(mutex_);
    if (status_.IsSpecified()) {
      rc = pid_;
    }
  }

  return rc;
}


//
// ProcessImpl
//

ProcessImpl::PIDImpl ProcessImpl::CurrentPidImpl() {
  return getpid();
}

void ProcessImpl::GetTimesImpl(long& user_time, long& kernel_time) {
  struct rusage usage;
  getrusage(RUSAGE_SELF, &usage);
  user_time = usage.ru_utime.tv_sec;
  kernel_time = usage.ru_stime.tv_sec;
}

ProcessHandleImpl* ProcessImpl::LaunchImpl( const String& command,
                                            const ArgsImpl& args,
                                            const String& initial_directory,
                                            Pipe* in_pipe,
                                            Pipe* out_pipe,
                                            Pipe* err_pipe,
                                            const EnvImpl& env) {
#if defined(__QNX__)
  if (initial_directory.IsEmpty()) {
    /// use QNX's spawn system call which is more efficient than fork/exec.
    char** argv = new char*[args.Count() + 2];
    int i = 0;
    argv[i++] = const_cast<char*>(command.c_str());
    for (const auto& arg : args) {
      argv[i++] = const_cast<char*>(arg.c_str());
    }
    argv[i] = nullptr;
    struct inheritance inherit;
    UnsafeMemory::Memset(&inherit, 0, sizeof(inherit));
    inherit.flags = SPAWN_ALIGN_DEFAULT | SPAWN_CHECK_SCRIPT | SPAWN_SEARCH_PATH;
    int fd_map[3];
    fd_map[0] = in_pipe  ? in_pipe->ReadHandle()   : 0;
    fd_map[1] = out_pipe ? out_pipe->WriteHandle() : 1;
    fd_map[2] = err_pipe ? err_pipe->WriteHandle() : 2;

    char** env_ptr = nullptr;
    Array<char> env_chars;
    Array<char*> env_ptrs;
    if (!env.IsEmpty()) {
      env_chars = GetEnvironmentVariablesBuffer(env);
      env_ptrs.Reserve(env.Count() + 1);
      char* p = &env_chars[0];
      while (*p) {
        env_ptrs.Add(p);
        while (*p) { ++p; }
        ++p;
      }
      env_ptrs.Add(0);
      env_ptr = &env_ptrs[0];
    }

    int pid = spawn(command.c_str(), 3, fd_map, &inherit, argv, env_ptr);
    delete[] argv;
    if (pid == -1) {
      throw SystemException("cannot spawn", command);
    }

    if (in_pipe) {
      in_pipe->Close(Pipe::CLOSE_READ);
    }

    if (out_pipe) {
      out_pipe->Close(Pipe::CLOSE_WRITE);
    }

    if (err_pipe) {
      err_pipe->Close(Pipe::CLOSE_WRITE);
    }

    return new ProcessHandleImpl(pid);
  } else {
    return LaunchByForkExecImpl(command, args, initial_directory, in_pipe, out_pipe, err_pipe, env);
  }
#else
  return LaunchByForkExecImpl(command, args, initial_directory, in_pipe, out_pipe, err_pipe, env);
#endif
}

ProcessHandleImpl* ProcessImpl::LaunchByForkExecImpl( const String& command,
                                                      const ArgsImpl& args,
                                                      const String& initial_directory,
                                                      Pipe* in_pipe,
                                                      Pipe* out_pipe,
                                                      Pipe* err_pipe,
                                                      const EnvImpl& env) {
#if !defined(FUN_NO_FORK_EXEC)
  // We must not allocated memory after fork(),
  // therefore allocate all required buffers first.
  Array<char> env_chars = GetEnvironmentVariablesBuffer(env);
  Array<char*> argv(args.Count() + 2);
  int i = 0;
  argv[i++] = const_cast<char*>(command.c_str());
  for (const auto& arg : args) {
    argv[i++] = const_cast<char*>(arg.c_str());
  }
  argv[i] = nullptr;

  const char* initial_directory_ptr = initial_directory.IsEmpty() ? 0 : initial_directory.c_str();

  int pid = fork();
  if (pid < 0) {
    throw SystemException("Cannot fork process for", command);
  } else if (pid == 0) {
    if (initial_directory_ptr) {
      if (chdir(initial_directory_ptr) != 0) {
        _exit(72);
      }
    }

    // set environment variables
    char* p = &env_chars[0];
    while (*p) {
      putenv(p);
      while (*p) ++p;
      ++p;
    }

    // setup redirection
    if (in_pipe) {
      dup2(in_pipe->ReadHandle(), STDIN_FILENO);
      in_pipe->Close(Pipe::CLOSE_BOTH);
    }

    // out_pipe and err_pipe may be the same, so we dup first and close later
    if (out_pipe) {
      dup2(out_pipe->WriteHandle(), STDOUT_FILENO);
    }

    if (err_pipe) {
      dup2(err_pipe->WriteHandle(), STDERR_FILENO);
    }

    if (out_pipe) {
      out_pipe->Close(Pipe::CLOSE_BOTH);
    }

    if (err_pipe) {
      err_pipe->Close(Pipe::CLOSE_BOTH);
    }

    // close all open file descriptors other than stdin, stdout, stderr
    for (int fd = 3; fd < sysconf(_SC_OPEN_MAX); ++fd) {
      close(fd);
    }

    execvp(argv[0], &argv[0]);
    _exit(72);
  }

  if (in_pipe) {
    in_pipe->Close(Pipe::CLOSE_READ);
  }

  if (out_pipe) {
    out_pipe->Close(Pipe::CLOSE_WRITE);
  }

  if (err_pipe) {
    err_pipe->Close(Pipe::CLOSE_WRITE);
  }

  return new ProcessHandleImpl(pid);
#else
  throw NotImplementedException("platform does not allow fork/exec");
#endif
}

void ProcessImpl::KillImpl(ProcessHandleImpl& handle) {
  KillImpl(handle.GetId());
}

void ProcessImpl::KillImpl(PIDImpl pid) {
  if (kill(pid, SIGKILL) != 0) {
    switch (errno) {
      case ESRCH:
        throw NotFoundException("cannot kill process");
      case EPERM:
        throw NoPermissionException("cannot kill process");
      default:
        throw SystemException("cannot kill process");
    }
  }
}

bool ProcessImpl::IsRunningImpl(const ProcessHandleImpl& handle) {
  return handle.Wait(WNOHANG) == 0;
}

bool ProcessImpl::IsRunningImpl(PIDImpl pid) {
  if (kill(pid, 0) == 0) {
    return true;
  } else {
    return false;
  }
}

void ProcessImpl::RequestTerminationImpl(PIDImpl pid) {
  if (kill(pid, SIGINT) != 0) {
    switch (errno) {
      case ESRCH:
        throw NotFoundException("cannot terminate process");
      case EPERM:
        throw NoPermissionException("cannot terminate process");
      default:
        throw SystemException("cannot terminate process");
    }
  }
}

} // namespace fun
