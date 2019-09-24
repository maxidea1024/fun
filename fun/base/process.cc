#include "fun/base/process.h"
#include "fun/base/container/array.h"
#include "fun/base/environment.h"
#include "fun/base/string/string.h"

#ifdef _MSC_VER
// Array::Resize에서 int32가 아닌 size_t를 인자로 받는 형태로 바꾸는게 좋을듯...
#pragma warning(disable : 4267)  // truncation size_t -> int32
#endif

namespace fun {
namespace {

// 이 함수는 각 플랫폼별 process 처리하는 코드들이 공유함.
// 별도로 정리가 필요해보임.
Array<char> GetEnvironmentVariablesBuffer(const fun::Process::Env& env) {
  Array<char> env_buf;
  std::size_t pos = 0;

  for (const auto& pair : env) {
    std::size_t env_len = pair.key.Len() + pair.value.Len() + 1;

    env_buf.Resize(pos + env_len + 1);
    UnsafeMemory::Memcpy(&env_buf[pos], pair.key.ConstData(), pair.key.Len());
    pos += pair.key.Len();
    env_buf[pos] = '=';
    ++pos;
    UnsafeMemory::Memcpy(&env_buf[pos], pair.value.ConstData(),
                         pair.value.Len());
    pos += pair.value.Len();
    env_buf[pos] = '\0';
    ++pos;
  }

  env_buf.Resize(pos + 1);
  env_buf[pos] = '\0';

  return env_buf;
}

}  // namespace
}  // namespace fun

#if FUN_PLATFORM_WINDOWS_FAMILY
#if defined(_WIN32_WCE)
#include "fun/base/process_wince.cc"
#else
#include "fun/base/process_win32.cc"
#endif
#elif FUN_PLATFORM != FUN_PLATFORM_VXWORKS
#include "fun/base/process_vx.cc"
#elif FUN_PLATFORM_UNIX_FAMILY
#include "fun/base/process_unix.cc"
#endif

namespace fun {

//
// ProcessHandle
//

ProcessHandle::ProcessHandle(const ProcessHandle& rhs) : impl_(rhs.impl_) {
  impl_->AddRef();
}

ProcessHandle::~ProcessHandle() { impl_->Release(); }

// TODO 참조 카운터 관계를 확인해야함.
//최초 0으로 하는지 1으로 하는지에 따라서 다르므로...
ProcessHandle::ProcessHandle(ProcessHandleImpl* impl) : impl_(impl) {
  fun_check_ptr(impl_);
}

ProcessHandle& ProcessHandle::operator=(const ProcessHandle& rhs) {
  if (FUN_LIKELY(&rhs != this)) {
    impl_->Release();
    impl_ = rhs.impl_;
    impl_->AddRef();
  }

  return *this;
}

ProcessHandle::PID ProcessHandle::GetId() const { return impl_->GetId(); }

int ProcessHandle::Wait() const { return impl_->Wait(); }

//
// Process
//

ProcessHandle Process::Launch(const String& command, const Args& args) {
  String initial_directory;
  Env env;
  return ProcessHandle(
      LaunchImpl(command, args, initial_directory, 0, 0, 0, env));
}

ProcessHandle Process::Launch(const String& command, const Args& args,
                              const String& initial_directory) {
  Env env;
  return ProcessHandle(
      LaunchImpl(command, args, initial_directory, 0, 0, 0, env));
}

ProcessHandle Process::Launch(const String& command, const Args& args,
                              Pipe* in_pipe, Pipe* out_pipe, Pipe* err_pipe) {
  fun_check(in_pipe == nullptr || (in_pipe != out_pipe && in_pipe != err_pipe));
  String initial_directory;
  Env env;
  return ProcessHandle(LaunchImpl(command, args, initial_directory, in_pipe,
                                  out_pipe, err_pipe, env));
}

ProcessHandle Process::Launch(const String& command, const Args& args,
                              const String& initial_directory, Pipe* in_pipe,
                              Pipe* out_pipe, Pipe* err_pipe) {
  fun_check(in_pipe == nullptr || (in_pipe != out_pipe && in_pipe != err_pipe));
  Env env;
  return ProcessHandle(LaunchImpl(command, args, initial_directory, in_pipe,
                                  out_pipe, err_pipe, env));
}

ProcessHandle Process::Launch(const String& command, const Args& args,
                              Pipe* in_pipe, Pipe* out_pipe, Pipe* err_pipe,
                              const Env& env) {
  fun_check(in_pipe == nullptr || (in_pipe != out_pipe && in_pipe != err_pipe));
  String initial_directory;
  return ProcessHandle(LaunchImpl(command, args, initial_directory, in_pipe,
                                  out_pipe, err_pipe, env));
}

ProcessHandle Process::Launch(const String& command, const Args& args,
                              const String& initial_directory, Pipe* in_pipe,
                              Pipe* out_pipe, Pipe* err_pipe, const Env& env) {
  fun_check(in_pipe == nullptr || (in_pipe != out_pipe && in_pipe != err_pipe));
  return ProcessHandle(LaunchImpl(command, args, initial_directory, in_pipe,
                                  out_pipe, err_pipe, env));
}

int Process::Wait(const ProcessHandle& handle) { return handle.Wait(); }

void Process::Kill(ProcessHandle& handle) { KillImpl(*handle.impl_); }

void Process::Kill(PID pid) { KillImpl(pid); }

bool Process::IsRunning(const ProcessHandle& handle) {
  return IsRunningImpl(*handle.impl_);
}

bool Process::IsRunning(PID pid) { return IsRunningImpl(pid); }

void Process::RequestTermination(PID pid) { RequestTerminationImpl(pid); }

}  // namespace fun
