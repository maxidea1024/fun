#pragma once

#include "fun/base/base.h"
#include "fun/base/delegate/delegate.h"
#include "fun/base/process.h"
#include "fun/base/runnable.h"
#include "fun/base/thread.h"

#define FUN_WITH_MONITORED_PROCESS 0

#if FUN_WITH_MONITORED_PROCESS

namespace fun {

/**
 * Declares a delegate that is executed when a monitored process completed.
 *
 * The first parameter is the process return code.
 */
FUN_DECLARE_DELEGATE_PARAMS1(OnMonitoredProcessCompleted, int32)

/**
 * Declares a delegate that is executed when a monitored process produces
 * output.
 *
 * The first parameter is the produced output.
 */
FUN_DECLARE_DELEGATE_PARAMS1(OnMonitoredProcessOutput, String)

/**
 * Implements an external process that can be monitored.
 */
class MonitoredProcess : public Runnable {
 public:
  MonitoredProcess(const String& url, const String& params, bool hidden,
                   bool create_pipes = true);
  ~MonitoredProcess();

  void Cancel(bool kill_tree = false) {
    cancelling_ = true;
    kill_tree_ = kill_tree;
  }

  Timespan GetDuration() const;

  bool IsRunning() const;

  bool Launch();

  void SetSleepInterval(float interval) { sleep_interval_ = interval; }

  SimpleDelegate& OnCanceled() { return canceled_event_; }

  OnMonitoredProcessCompleted& OnCompleted() { return completed_event_; }

  OnMonitoredProcessOutput& OnOutput() { return output_event_; }

  int32 GetReturnCode() const { return return_code_; }

 public:
  // Runnable interface.
  virtual bool InitRunnable() override { return true; }
  virtual void Run() override;
  virtual void StopRunnable() override { Cancel(); }
  virtual void ExitRunnable() override {}

 protected:
  void ProcessOutput(const String& output);

 private:
  bool cancelling_;
  DateTime start_time_;
  DateTime end_time_;
  bool hidden_;
  bool kill_tree_;
  String params_;
  ProcessHandle process_handle_;
  bool create_pipes_;
  void* read_pipe_;
  void* write_pipe_;
  int32 return_code_;
  RunnableThread* thread_;
  String url_;
  float sleep_interval_;

  /** Holds a delegate that is executed when the process has been canceled. */
  SimpleDelegate canceled_event_;

  /** Holds a delegate that is executed when a monitored process completed. */
  OnMonitoredProcessCompleted completed_event_;

  /** Holds a delegate that is executed when a monitored process produces
   * output. */
  OnMonitoredProcessOutput output_event_;
};

}  // namespace fun

#endif  // FUN_WITH_MONITORED_PROCESS
