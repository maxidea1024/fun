#include "fun/base/monitored_process.h"

#if FUN_WITH_MONITORED_PROCESS

namespace fun {

MonitoredProcess::MonitoredProcess( const String& url,
                                    const String& params,
                                    bool hidden,
                                    bool create_pipes)
  : cancelling_(false),
    start_time_(DateTime::Null),
    end_time_(DateTime::Null),
    hidden_(hidden),
    kill_tree_(false),
    params_(params),
    create_pipes_(create_pipes),
    read_pipe_(nullptr),
    write_pipe_(nullptr),
    return_code_(0),
    thread_(nullptr),
    url_(url),
    sleep_interval_(0.0f) {}

MonitoredProcess::~MonitoredProcess() {
  if (IsRunning()) {
    Cancel(true);
    thread_->WaitForCompletion();
    delete thread_;
  }
}

Timespan MonitoredProcess::GetDuration() const {
  return IsRunning() ? (DateTime::UtcNow() - start_time_) : (end_time_ - start_time_);
}

bool MonitoredProcess::IsRunning() const {
  return !!thread_;
}

bool MonitoredProcess::Launch() {
  // fail if already running.
  if (IsRunning()) {
    return false;
  }

  if (create_pipes_ && !CPlatformProcess::CreatePipe(read_pipe_, write_pipe_)) {
    return false;
  }

  process_handle_ = CPlatformProcess::CreateNewProcess(*url_, *params_, false, hidden_, hidden_, nullptr, 0, *CPaths::RootDir(), write_pipe_);
  if (!process_handle_.IsValid()) {
    return false;
  }

  static int32 MonitoredProcessIndex = 0;
  const String MonitoredProcessName = String::Format(TEXT("MonitoredProcess %d"), MonitoredProcessIndex);
  MonitoredProcessIndex++;

  thread_ = RunnableThread::Create(this, *MonitoredProcessName, 128 * 1024, ThreadPriority::AboveNormal);
  return true;
}

void MonitoredProcess::ProcessOutput(const String& output) {
  TArray<String> log_lines;
  output.SplitLines(log_lines);

  for (int32 i = 0; i < log_lines.Count(); i++) {
    output_event_.ExecuteIfBound(log_lines[i]);
  }
}

void MonitoredProcess::Run() {
  // monitor the process
  start_time_ = DateTime::UtcNow(); {
    do {
      CPlatformProcess::Sleep(sleep_interval_);

      // launched된 프로세스에 pipe로 연결후 출력을 받아서 자체적으로 출력함.
      ProcessOutput(CPlatformProcess::ReadPipe(read_pipe_));

      if (cancelling_) {
        CPlatformProcess::TerminateProcess(process_handle_, kill_tree_);
        canceled_event_.ExecuteIfBound();
        thread_ = nullptr;
        return 0;
      }
    }
    while (CPlatformProcess::IsProcessRunning(process_handle_));
  }
  end_time_ = DateTime::UtcNow();

  // close output pipes
  CPlatformProcess::ClosePipe(read_pipe_, write_pipe_);
  read_pipe_ = nullptr;
  write_pipe_ = nullptr;

  // get completion status
  if (!CPlatformProcess::GetProcessReturnCode(process_handle_, &return_code_)) {
    return_code_ = -1;
  }

  completed_event_.ExecuteIfBound(return_code_);
  thread_ = nullptr;
}

} // namespace fun

#endif // FUN_WITH_MONITORED_PROCESS
