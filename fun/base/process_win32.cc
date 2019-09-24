#include "fun/base/process_win32.h"
#include "fun/base/container/array.h"
#include "fun/base/exception.h"
#include "fun/base/file.h"
#include "fun/base/named_event.h"
#include "fun/base/path.h"
#include "fun/base/pipe.h"
#include "fun/base/str.h"

namespace fun {

//
// ProcessHandleImpl
//

ProcessHandleImpl::ProcessHandleImpl(HANDLE process_handle, uint32 pid)
    : process_handle_(process_handle), pid_(pid) {}

ProcessHandleImpl::~ProcessHandleImpl() { CloseHandle(); }

void ProcessHandleImpl::CloseHandle() {
  if (process_handle_) {
    ::CloseHandle(process_handle_);
    process_handle_ = NULL;
  }
}

uint32 ProcessHandleImpl::GetId() const { return pid_; }

HANDLE ProcessHandleImpl::GetHandle() const { return process_handle_; }

int ProcessHandleImpl::Wait() const {
  DWORD rc = WaitForSingleObject(process_handle_, INFINITE);
  if (rc != WAIT_OBJECT_0) {
    throw SystemException("Wait failed for process", String::FromNumber(pid_));
  }

  DWORD exit_code;
  if (GetExitCodeProcess(process_handle_, &exit_code) == 0) {
    throw SystemException("Cannot get exit code for process",
                          String::FromNumber(pid_));
  }

  return exit_code;
}

//
// ProcessImpl
//

ProcessImpl::PIDImpl ProcessImpl::CurrentPidImpl() {
  return GetCurrentProcessId();
}

void ProcessImpl::GetTimesImpl(long& user_time, long& kernel_time) {
  FILETIME ft_creation;
  FILETIME ft_exit;
  FILETIME ft_kernel;
  FILETIME ft_user;

  if (GetProcessTimes(GetCurrentProcess(), &ft_creation, &ft_exit, &ft_kernel,
                      &ft_user) != 0) {
    ULARGE_INTEGER time;
    time.LowPart = ft_kernel.dwLowDateTime;
    time.HighPart = ft_kernel.dwHighDateTime;
    kernel_time = long(time.QuadPart / 10000000L);
    time.LowPart = ft_user.dwLowDateTime;
    time.HighPart = ft_user.dwHighDateTime;
    user_time = long(time.QuadPart / 10000000L);
  } else {
    user_time = kernel_time = -1;
  }
}

namespace {

bool ArgNeedsEscaping(const String& arg) {
  bool contains_quotable_char = (arg.IndexOfAny(" \t\n\v\"") != INVALID_INDEX);
  // Assume args that start and end with quotes are already quoted and do not
  // require further quoting. There is probably code out there written before
  // Launch() escaped the arguments that does its own escaping of arguments.
  // This ensures we do not interfere with those arguments.
  bool already_quoted =
      arg.Len() > 1 && '\"' == arg[0] && '\"' == arg[arg.Len() - 1];
  return contains_quotable_char && !already_quoted;
}

// Based on code from
// https://blogs.msdn.microsoft.com/twistylittlepassagesallalike/2011/04/23/everyone-quotes-command-line-arguments-the-wrong-way/
String EscapeArg(const String& arg) {
  if (ArgNeedsEscaping(arg)) {
    String quoted_arg("\"");
    for (String::const_iterator it = arg.begin();; ++it) {
      unsigned blackslash_count = 0;
      while (it != arg.end() && '\\' == *it) {
        ++it;
        ++blackslash_count;
      }

      if (it == arg.end()) {
        quoted_arg.Append(2 * blackslash_count, '\\');
        break;
      } else if ('"' == *it) {
        quoted_arg.Append(2 * blackslash_count + 1, '\\');
        quoted_arg.push_back('"');
      } else {
        quoted_arg.Append(blackslash_count, '\\');
        quoted_arg.push_back(*it);
      }
    }
    quoted_arg.push_back('"');
    return quoted_arg;
  } else {
    return arg;
  }
}

}  // namespace

ProcessHandleImpl* ProcessImpl::LaunchImpl(const String& command,
                                           const ArgsImpl& args,
                                           const String& initial_directory,
                                           Pipe* in_pipe, Pipe* out_pipe,
                                           Pipe* err_pipe, const EnvImpl& env) {
  String commandline = EscapeArg(command);
  for (const auto& arg : args) {
    commandline.Append(" ");
    commandline.Append(EscapeArg(arg));
  }

  UString ucommandline = UString::FromUtf8(commandline);

  const wchar_t* application_name = nullptr;
  UString uapplication_name;
  if (command.Len() > MAX_PATH) {
    Path p(command);
    if (p.IsAbsolute()) {
      uapplication_name = UString::FromUtf8(command);
      if (p.GetExtension().IsEmpty()) {
        uapplication_name += UTEXT(".EXE");
      }
      application_name = uapplication_name.c_str();
    }
  }

  STARTUPINFOW startup_info;
  GetStartupInfoW(&startup_info);  // take defaults from current process
  startup_info.cb = sizeof(STARTUPINFOW);
  startup_info.lpReserved = NULL;
  startup_info.lpDesktop = NULL;
  startup_info.lpTitle = NULL;
  startup_info.dwFlags = STARTF_FORCEOFFFEEDBACK;
  startup_info.cbReserved2 = 0;
  startup_info.lpReserved2 = NULL;

  HANDLE process_handle = GetCurrentProcess();
  bool must_inherit_handles = false;
  if (in_pipe) {
    DuplicateHandle(process_handle, in_pipe->ReadHandle(), process_handle,
                    &startup_info.hStdInput, 0, TRUE, DUPLICATE_SAME_ACCESS);
    must_inherit_handles = true;
    in_pipe->Close(Pipe::CLOSE_READ);
  } else if (GetStdHandle(STD_INPUT_HANDLE)) {
    DuplicateHandle(process_handle, GetStdHandle(STD_INPUT_HANDLE),
                    process_handle, &startup_info.hStdInput, 0, TRUE,
                    DUPLICATE_SAME_ACCESS);
    must_inherit_handles = true;
  } else {
    startup_info.hStdInput = 0;
  }

  // out_pipe may be the same as err_pipe, so we duplicate first and close
  // later.
  if (out_pipe) {
    DuplicateHandle(process_handle, out_pipe->WriteHandle(), process_handle,
                    &startup_info.hStdOutput, 0, TRUE, DUPLICATE_SAME_ACCESS);
    must_inherit_handles = true;
  } else if (GetStdHandle(STD_OUTPUT_HANDLE)) {
    DuplicateHandle(process_handle, GetStdHandle(STD_OUTPUT_HANDLE),
                    process_handle, &startup_info.hStdOutput, 0, TRUE,
                    DUPLICATE_SAME_ACCESS);
    must_inherit_handles = true;
  } else {
    startup_info.hStdOutput = 0;
  }

  if (err_pipe) {
    DuplicateHandle(process_handle, err_pipe->WriteHandle(), process_handle,
                    &startup_info.hStdError, 0, TRUE, DUPLICATE_SAME_ACCESS);
    must_inherit_handles = true;
  } else if (GetStdHandle(STD_ERROR_HANDLE)) {
    DuplicateHandle(process_handle, GetStdHandle(STD_ERROR_HANDLE),
                    process_handle, &startup_info.hStdError, 0, TRUE,
                    DUPLICATE_SAME_ACCESS);
    must_inherit_handles = true;
  } else {
    startup_info.hStdError = 0;
  }

  if (out_pipe) {
    out_pipe->Close(Pipe::CLOSE_WRITE);
  }

  if (err_pipe) {
    err_pipe->Close(Pipe::CLOSE_WRITE);
  }

  if (must_inherit_handles) {
    startup_info.dwFlags |= STARTF_USESTDHANDLES;
  }

  UString uinitial_directory = UString::FromUtf8(initial_directory);
  const wchar_t* working_directory =
      uinitial_directory.IsEmpty() ? 0 : uinitial_directory.c_str();

  const char* env_ptr = nullptr;
  Array<char> env_chars;
  if (!env.IsEmpty()) {
    env_chars = GetEnvironmentVariablesBuffer(env);
    env_ptr = &env_chars[0];
  }

  PROCESS_INFORMATION process_info;
  DWORD creation_flags = GetConsoleWindow() ? 0 : CREATE_NO_WINDOW;
  BOOL rc = CreateProcessW(
      application_name, const_cast<wchar_t*>(ucommandline.c_str()),
      NULL,  // processAttributes
      NULL,  // threadAttributes
      must_inherit_handles, creation_flags, (LPVOID)env_ptr, working_directory,
      &startup_info, &process_info);

  if (startup_info.hStdInput) {
    CloseHandle(startup_info.hStdInput);
  }

  if (startup_info.hStdOutput) {
    CloseHandle(startup_info.hStdOutput);
  }

  if (startup_info.hStdError) {
    CloseHandle(startup_info.hStdError);
  }

  if (rc) {
    CloseHandle(process_info.hThread);
    return new ProcessHandleImpl(process_info.hProcess,
                                 process_info.dwProcessId);
  } else {
    throw SystemException("Cannot Launch process", command);
  }
}

void ProcessImpl::KillImpl(ProcessHandleImpl& handle) {
  if (handle.GetHandle()) {
    // TODO handle.process()에서 process라는 이름을 변경해주어야할듯...
    if (TerminateProcess(handle.GetHandle(), 0) == 0) {
      handle.CloseHandle();
      throw SystemException("cannot kill process");
    }

    handle.CloseHandle();
  }
}

void ProcessImpl::KillImpl(PIDImpl pid) {
  HANDLE process_handle = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
  if (process_handle) {
    if (TerminateProcess(process_handle, 0) == 0) {
      CloseHandle(process_handle);
      throw SystemException("cannot kill process");
    }

    CloseHandle(process_handle);
  } else {
    switch (GetLastError()) {
      case ERROR_ACCESS_DENIED:
        throw NoPermissionException("cannot kill process");
      case ERROR_INVALID_PARAMETER:
        throw NotFoundException("cannot kill process");
      default:
        throw SystemException("cannot kill process");
    }
  }
}

bool ProcessImpl::IsRunningImpl(const ProcessHandleImpl& handle) {
  bool result = true;
  DWORD exit_code;
  BOOL rc = GetExitCodeProcess(handle.GetHandle(), &exit_code);
  if (!rc || exit_code != STILL_ACTIVE) {
    result = false;
  }

  return result;
}

bool ProcessImpl::IsRunningImpl(PIDImpl pid) {
  HANDLE process_handle = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
  bool result = true;
  DWORD exit_code;
  BOOL rc = GetExitCodeProcess(process_handle, &exit_code);
  if (!rc || exit_code != STILL_ACTIVE) {
    result = false;
  }

  return result;
}

void ProcessImpl::RequestTerminationImpl(PIDImpl pid) {
  NamedEvent ev(TerminationEventName(pid));
  ev.Set();
}

String ProcessImpl::TerminationEventName(PIDImpl pid) {
  // TODO 이름에 대한 정밀분석...
  String ev_name("TTSTRM");
  // NumberFormatter::AppendHex(ev_name, pid, 8);
  ev_name += String::Format("{0:08x}", pid);
  return ev_name;
}

}  // namespace fun
