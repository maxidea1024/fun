#pragma once

#include "fun/net/net.h"

namespace fun {
namespace net {

/**
Log Writer.
*/
class LogWriter {
 public:
  enum class ELogType { NewFile = 0, Default, UserDefine };

 public:
  /** Create a log writer. */
  static LogWriter* New(const char* log_filename,
                        int32 MewFileForLineLimit = int32_MAX);
  /** Destructor. */
  virtual ~LogWriter();

 public:
  /** Sets the log filename. */
  void SetFilename(const char* log_filename);

  /** Write a text line. */
  void WriteLine(LogCategory Category, const char* text);
  void WriteLine(const char* text);

  // private:
  //  /** Log file object. */
  //  SharedPtr<IFile> File;
  //  /** Shemaphore object. */
  //  CSemaphore WriteSemaphore;
  //  /** Mutex object. */
  //  TLockable<> CS;
  //  bool bChangeFileFailed;
  //  int32 NewFileForLineLimit;
  //  int32 CurrentLineCount;
  //  String Filename;
  //  int32 CurrentFileCount;
  //  CThread worker_thread_;
  //  FUN_ALIGNED_VOLATILE bool should_stop_thread_;
  //
  //  static void StaticThreadProc(void* context);
  //
  //  void ThreadProc();
  //
  //  struct CLogData
  //  {
  //    ELogType Type;
  //    String text;
  //    LogCategory Category;
  //    DateTime added_time;
  //  };
  //  typedef SharedPtr<CLogData> CLogDataPtr;
  //  typedef List<CLogDataPtr> CLogList;
  //  CLogList LogList;
  //
  LogWriter(const char* log_filename, int32 NewFileForLineLimit);
};

}  // namespace net
}  // namespace fun
