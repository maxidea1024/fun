#pragma once

#include "fun/base/base.h"
#include "fun/base/timestamp.h"
#include "fun/base/timespan.h"
#include "fun/base/delegate/delegate.h"
#include "fun/base/process.h"
#include "fun/base/runnable.h"
#include "fun/base/thread.h"
#include "fun/base/container/queue.h"

#define FUN_WITH_INTERACTIVE_PROCESS  0

#if FUN_WITH_INTERACTIVE_PROCESS

namespace fun {

//TODO
//DECLARE_LOG_CATEGORY_EXTERN(LogInteractiveProcess, Info, All);

/**
 * Declares a delegate that is executed when a interactive process completed.
 * 
 * The first parameter is the process return code.
 */
FUN_DECLARE_DELEGATE_PARAMS2(OnInteractiveProcessCompleted, int32, bool);

/**
 * Declares a delegate that is executed when a interactive process produces output.
 * 
 * The first parameter is the produced output.
 */
FUN_DECLARE_DELEGATE_PARAMS1(OnInteractiveProcessOutput, const String&);


/**
 * Implements an external process that can be interacted.
 */
class FUN_BASE_API InteractiveProcess : public Runnable {
 public:
  /**
   * Creates a new interactive process.
   * 
   * \param url - The url_ of the executable to launch.
   * \param params - The command line parameters.
   * \param hidden - Whether the window of the process should be hidden.
   */
  InteractiveProcess(const String& url, const String& params, bool hidden, bool long_time = false);

  /**
   * Destructor.
   */
  ~InteractiveProcess();

  /**
   * Gets the duration of time that the task has been running.
   * 
   * \return Time duration.
   */
  Timespan GetDuration() const;

  /**
   * Checks whether the process is still running.
   * 
   * \return true if the process is running, false otherwise.
   */
  bool IsRunning() const {
    return !!thread_;
  }

  /**
   * Launches the process
   * 
   * \return True if succeed
   */
  bool Launch();

  /**
   * Returns a delegate that is executed when the process has been canceled.
   * 
   * \return The delegate.
   */
  SimpleDelegate& OnCanceled() {
    return canceled_event_;
  }

  /**
   * Returns a delegate that is executed when the interactive process completed.
   * Delegate won't be executed if process terminated without user wanting
   * 
   * \return The delegate.
   */
  OnInteractiveProcessCompleted& OnCompleted() {
    return completed_event_;
  }

  /**
   * Returns a delegate that is executed when a interactive process produces output.
   * 
   * \return The delegate.
   */
  OnInteractiveProcessOutput& OnOutput() {
    return output_event_;
  }

  /**
   * Sends the message when process is ready
   * 
   * \param message to be sent
   */
  void SendWhenReady(const String& message);

  /**
   * Returns the return code from the exited process
   * 
   * \return Process return code
   */
  int32 GetReturnCode() const {
    return return_code_;
  }

  /**
   * Cancels the process.
   * 
   * \param kill_tree - Whether to kill the entire process tree when canceling this process.
   */
  void Cancel(bool kill_tree = false) {
    cancelling_ = true;
    kill_tree_ = kill_tree;
  }

  // Runnable interface
  //bool InitRunnable() override { return true; }
  void Run() override;
  //void StopRunnable() override { Cancel(); }
  //void ExitRunnable() override {}

 protected:
  /**
   * Processes the given output string.
   * 
   * \param output - The output string to process.
   */
  void ProcessOutput(const String& output);

  /**Takes the first message to be sent from messages_to_process_, if there is one, and sends it to process */
  void SendMessageToProcessif();

 private:
  /** Whether the process is being canceled. */
  bool cancelling_ : 1;

  /** Whether the window of the process should be hidden. */
  bool hidden_ : 1;

  /** Whether to kill the entire process tree when cancelling this process. */
  bool kill_tree_ : 1;

  /** How many milliseconds should the process sleep */
  float sleep_time_;

  /** Holds the url of the executable to launch. */
  String url_;

  /** Holds the command line parameters. */
  String params_;

  /** Holds the handle to the process. */
  ProcessHandle process_handle_;

  /** Holds the read pipe of parent process. */
  void* read_pipe_parent_;
  /** Holds the write pipe of parent process. */
  void* write_pipe_parent_;
  /** Holds the read pipe of child process. Should not be used except for testing */
  void* read_pipe_child_;
  /** Holds the write pipe of child process. Should not be used except for testing */
  void* write_pipe_child_;

  /** Holds the thread object. */
  RunnableThread* thread_;

  /** Holds the name of thread */
  String thread_name_;

  /** Holds the return code. */
  int32 return_code_;

  /** Holds the time at which the process started. */
  DateTime start_time_;

  /** Holds the time at which the process ended. */
  DateTime end_time_;

  /** Holds messages to be written to pipe when ready */
  Queue<String> messages_to_process_;

  /** Holds a delegate that is executed when the process has been canceled. */
  SimpleDelegate canceled_event_;;

  /** Holds a delegate that is executed when a interactive process completed. */
  OnInteractiveProcessCompleted completed_event_;

  /** Holds a delegate that is executed when a interactive process produces output. */
  OnInteractiveProcessOutput output_event_;
};

} // namespace fun

#endif // FUN_WITH_INTERACTIVE_PROCESS
