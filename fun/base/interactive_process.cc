#include "fun/base/interactive_process.h"

#if FUN_WITH_INTERACTIVE_PROCESS

namespace fun {

// TODO
// DEFINE_LOG_CATEGORY(LogInteractiveProcess);

static inline bool CreatePipeWrite(void*& read_pipe, void*& write_pipe) {
#if FUN_PLATFORM_WINDOWS_FAMILY
  SECURITY_ATTRIBUTES Attr = {sizeof(SECURITY_ATTRIBUTES), nullptr, true};

  if (!::CreatePipe(&read_pipe, &write_pipe, &Attr, 0)) {
    return false;
  }

  if (!::SetHandleInformation(write_pipe, HANDLE_FLAG_INHERIT, 0)) {
    return false;
  }

  return true;
#else
  return CPlatformProcess::CreatePipe(read_pipe, write_pipe);
#endif  // FUN_PLATFORM_WINDOWS_FAMILY
}

InteractiveProcess::InteractiveProcess(const String& url, const String& params,
                                       bool hidden, bool long_time)
    : cancelling_(false),
      hidden_(hidden),
      kill_tree_(false),
      url_(url),
      params_(params),
      read_pipe_parent_(nullptr),
      write_pipe_parent_(nullptr),
      read_pipe_child_(nullptr),
      write_pipe_child_(nullptr),
      thread_(nullptr),
      return_code_(0),
      start_time_(DateTime::Null),
      end_time_(DateTime::Null) {
  if (long_time == true) {
    sleep_time_ = 0.0010f;  // 10 milliseconds sleep
  } else {
    sleep_time_ = 0.f;
  }
}

InteractiveProcess::~InteractiveProcess() {
  if (IsRunning()) {
    Cancel(false);
    thread_->WaitForCompletion();
    delete thread_;
  }
}

Timespan InteractiveProcess::GetDuration() const {
  if (IsRunning() == true) {
    return (DateTime::UtcNow() - start_time_);
  }

  return (end_time_ - start_time_);
}

bool InteractiveProcess::Launch() {
  if (IsRunning() == true) {
    // fun_log(LogInteractiveProcess, Warning, TEXT("The process is already
    // running"));
    return false;
  }

  // For reading from child process
  if (CPlatformProcess::CreatePipe(read_pipe_parent_, write_pipe_child_) ==
      false) {
    // fun_log(LogInteractiveProcess, Error, TEXT("Failed to create pipes for
    // parent process"));
    return false;
  }

  // For writing to child process
  if (CreatePipeWrite(read_pipe_child_, write_pipe_parent_) == false) {
    // fun_log(LogInteractiveProcess, Error, TEXT("Failed to create pipes for
    // parent process"));
    return false;
  }

  ProcessHandle = CPlatformProcess::CreateNewProcess(
      *url_, *params_, false, hidden_, hidden_, nullptr, 0, nullptr,
      write_pipe_child_, read_pipe_child_);
  if (ProcessHandle.IsValid() == false) {
    // fun_log(LogInteractiveProcess, Error, TEXT("Failed to create process"));
    return false;
  }

  // Creating name for the process
  static uint32 TempInteractiveProcessIndex = 0;
  thread_name_ = String::Format(TEXT("InteractiveProcess %d"),
                                TempInteractiveProcessIndex);
  TempInteractiveProcessIndex++;

  thread_ = RunnableThread::Create(this, *thread_name_);
  if (thread_ == nullptr) {
    // fun_log(LogInteractiveProcess, Error, TEXT("Failed to create process
    // thread!"));
    return false;
  }

  // fun_log(LogInteractiveProcess, Info, TEXT("Process creation succesfull
  // %s"), *thread_name_);
  return true;
}

void InteractiveProcess::ProcessOutput(const String& output) {
  Array<String> log_lines;
  output.SplitLines(log_lines);

  for (int32 i = 0; i < log_lines.Count(); ++i) {
    // Don't accept if it is just an empty string
    if (log_lines[i].IsEmpty() == false) {
      output_delegate_.ExecuteIfBound(log_lines[i]);
      // fun_log(LogInteractiveProcess, Info, TEXT("Child Process -> %s"),
      // *log_lines[i]);
    }
  }
}

void InteractiveProcess::SendMessageToProcessif() {
  // If there is not a message
  if (messages_to_process_.IsEmpty() == true) {
    return;
  }

  if (write_pipe_parent_ == nullptr) {
    // fun_log(LogInteractiveProcess, Warning, TEXT("write_pipe is not valid"));
    return;
  }

  if (ProcessHandle.IsValid() == false) {
    // fun_log(LogInteractiveProcess, Warning, TEXT("Process handle is not
    // valid"));
    return;
  }

  // A string for original message and one for written message
  String written_message, message;
  messages_to_process_.Dequeue(message);

  CPlatformProcess::WritePipe(write_pipe_parent_, message, &written_message);

  // fun_log(LogInteractiveProcess, Info, TEXT("Parent Process -> Original
  // message: %s, Written message: %s"), *message, *written_message);

  if (written_message.Len() == 0) {
    // fun_log(LogInteractiveProcess, Error, TEXT("Writing message through pipe
    // failed"));
    return;
  } else if (message.Len() > written_message.Len()) {
    // fun_log(LogInteractiveProcess, Error, TEXT("Writing some part of the
    // message through pipe failed"));
    return;
  }
}

void InteractiveProcess::SendWhenReady(const String& message) {
  messages_to_process_.Enqueue(message);
}

void InteractiveProcess::Run() {
  // control and interact with the process
  start_time_ = DateTime::UtcNow();
  {
    do {
      Thread::Sleep(sleep_time_);

      // Read pipe and redirect it to ProcessOutput function
      ProcessOutput(CPlatformProcess::ReadPipe(read_pipe_parent_));

      // Write to process if there is a message
      SendMessageToProcessif();

      // If wanted to stop program
      if (cancelling_ == true) {
        CPlatformProcess::TerminateProcess(ProcessHandle, kill_tree_);
        CanceledDelegate.ExecuteIfBound();

        // TODO
        // fun_log(LogInteractiveProcess, Info, TEXT("The process is being
        // canceled"));
        return;
      }
    } while (CPlatformProcess::IsProcessRunning(ProcessHandle) == true);
  }

  // close pipes
  CPlatformProcess::ClosePipe(read_pipe_parent_, write_pipe_child_);
  read_pipe_parent_ = write_pipe_child_ = nullptr;
  CPlatformProcess::ClosePipe(read_pipe_child_, write_pipe_parent_);
  read_pipe_child_ = write_pipe_parent_ = nullptr;

  // get completion status
  if (CPlatformProcess::GetProcessReturnCode(ProcessHandle, &return_code_) ==
      false) {
    return_code_ = -1;
  }

  end_time_ = DateTime::UtcNow();

  CompletedDelegate.ExecuteIfBound(return_code_, cancelling_);
}

}  // namespace fun

#endif  // FUN_WITH_INTERACTIVE_PROCESS
