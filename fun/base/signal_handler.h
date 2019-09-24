#pragma once

#include "fun/base/base.h"

#if FUN_PLATFORM_UNIX_FAMILY && FUN_PLATFORM != FUN_PLATFORM_VXWORKS

#include "fun/base/container/array.h"
#include <setjmp.h>

namespace fun {

/**
 * This helper class simplifies the handling of POSIX signals.
 *
 * The class provides a signal handler (installed with
 * InstallHandlers()) that translates certain POSIX
 * signals (SIGILL, SIGBUS, SIGSEGV, SIGSYS) into
 * C++ exceptions.
 *
 * Internally, a stack of sigjmp_buf structs is maintained for
 * each thread. The constructor pushes a new sigjmp_buf onto
 * the current thread's stack. The destructor pops the sigjmp_buf
 * from the stack.
 *
 * The fun_throw_on_signal macro creates an instance of SignalHandler
 * on the stack, which results in a new sigjmp_buf being created.
 * The sigjmp_buf is then set-up with sigsetjmp().
 *
 * The HandleSignal() method, which is invoked when a signal arrives,
 * checks if a sigjmp_buf is available for the current thread.
 * If so, siglongjmp() is used to jump out of the signal handler.
 *
 * Typical usage is as follows:
 *
 *   try {
 *     fun_throw_on_signal;
 *      ...
 *   } catch (fun::SignalException&) {
 *     ...
 *   }
 *
 * The best way to deal with a SignalException is to log as much context
 * information as possible, to aid in debugging, and then to exit.
 *
 * The SignalHandler can be disabled globally by compiling TTS and client
 * code with the FUN_NO_SIGNAL_HANDLER macro defined.
 */
class FUN_BASE_API SignalHandler {
 public:
  /**
   * Creates the SignalHandler.
   */
  SignalHandler();

  /**
   * Destroys the SignalHandler.
   */
  ~SignalHandler();

  /**
   * Returns the top-most sigjmp_buf for the current thread.
   */
  sigjmp_buf& GetJumpBuffer();

  /**
   * Throws a SignalException with a textual description
   * of the given signal as argument.
   */
  static void ThrowSignalException(int sig);

  /**
   * Installs signal handlers for SIGILL, SIGBUS, SIGSEGV
   * and SIGSYS.
   */
  static void Install();

 protected:
  /**
   * The actual signal handler.
   */
  static void HandleSignal(int sig);

  /**
   * sigjmp_buf cannot be used to instantiate a std::vector,
   * so we provide a wrapper struct.
   */
  struct JumpBuffer {
    sigjmp_buf buf;
  };
  typedef Array<JumpBuffer> JumpBufferList;

  /**
   * Returns the JumpBufferList for the current thread.
   */
  static JumpBufferList& GetJumpBufferList();

 private:
  static JumpBufferList jump_buffer_list_;

  friend class ThreadImpl;
};


#ifndef FUN_NO_SIGNAL_HANDLER
#define fun_throw_on_signal \
  fun::SignalHandler _fun_signal_handler; \
  int _fun_signal = sigsetjmp(_fun_signal_handler.GetJumpBuffer(), 1); \
  if (_fun_signal) _fun_signal_handler.ThrowSignalException(_fun_signal);
#else
#define fun_throw_on_signal
#endif

} // namespace fun

#endif // FUN_PLATFORM_UNIX_FAMILY
