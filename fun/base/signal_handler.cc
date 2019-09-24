#include "fun/base/signal_handler.h"

#if FUN_PLATFORM_UNIX_FAMILY && FUN_PLATFORM != FUN_PLATFORM_VXWORKS

#include "fun/base/thread.h"
//#include "fun/base/number_formatter.h"
#include <signal.h>
#include <cstdlib>
#include "fun/base/exception.h"

namespace fun {

SignalHandler::JumpBufferList SignalHandler::jump_buffer_list_;

SignalHandler::SignalHandler() {
  JumpBufferList& list = GetJumpBufferList();
  JumpBuffer buf;
  list.push_back(buf);
}

SignalHandler::~SignalHandler() { GetJumpBufferList().pop_back(); }

sigjmp_buf& SignalHandler::GetJumpBuffer() {
  return GetJumpBufferList().back().buf;
}

void SignalHandler::ThrowSignalException(int sig) {
  switch (sig) {
    case SIGILL:
      throw SignalException("Illegal instruction");
    case SIGBUS:
      throw SignalException("Bus error");
    case SIGSEGV:
      throw SignalException("Segmentation violation");
    case SIGSYS:
      throw SignalException("Invalid system call");
    default:
      throw SignalException(NumberFormatter::FormatHex(sig));
  }
}

void SignalHandler::Install() {
#ifndef FUN_NO_SIGNAL_HANDLER
  struct sigaction sa;
  sa.sa_handler = HandleSignal;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGILL, &sa, 0);
  sigaction(SIGBUS, &sa, 0);
  sigaction(SIGSEGV, &sa, 0);
  sigaction(SIGSYS, &sa, 0);
#endif
}

void SignalHandler::HandleSignal(int sig) {
  JumpBufferList& jb = GetJumpBufferList();
  if (!jb.IsEmpty()) {
    siglongjmp(jb.back().buf, sig);
  }

  // Abort if no jump buffer registered
  std::abort();
}

SignalHandler::JumpBufferList& SignalHandler::GetJumpBufferList() {
  ThreadImpl* thread = ThreadImpl::CurrentImpl();
  if (thread) {
    return thread->jump_buffer_list_;
  } else {
    return jump_buffer_list_;
  }
}

}  // namespace fun

#endif  // FUN_PLATFORM_UNIX_FAMILY
