#pragma once

#include "fun/base/base.h"
#include "fun/base/date_time.h"
#include "fun/base/logging/log_sink.h"
#include "fun/base/mutex.h"
#include "fun/base/timespan.h"

#define FUN_WITH_FILE_SINK 1

#if FUN_WITH_FILE_SINK

namespace fun {

class LogFile;
class RotateStrategy;
class ArchiveStrategy;
class PurgeStrategy;

class FUN_BASE_API FileSink : public LogSink {
  // FUN_DECLARE_RTCLASS(FileSink, LogSink)

 public:
  FileSink();
  FileSink(const String& path);

  void Open();
  void Close();

  void Log(const LogMessage& msg) override;
  void SetProperty(const String& name, const String& value) override;
  String GetProperty(const String& name) const override;

  DateTime GetCreationData() const;
  uint64 GetSize() const;

  const String& GetPath() const;

  static const String PROP_PATH;
  static const String PROP_ROTATION;
  static const String PROP_ARCHIVE;
  static const String PROP_TIMES;
  static const String PROP_COMPRESS;
  static const String PROP_PURGE_AGE;
  static const String PROP_PURGE_COUNT;
  static const String PROP_FLUSH;
  static const String PROP_ROTATE_ON_OPEN;

 protected:
  virtual ~FileSink();

  void SetRotation(const String& rotation);
  void SetArchive(const String& archive);
  void SetCompress(const String& compress);
  void SetPurgeAge(const String& age);
  void SetPurgeCount(const String& count);
  void SetFlush(const String& flush);
  void SetRotateOnOpen(const String& rotate_on_open);
  void Purge();

 private:
  bool SetNoPurge(const String& value);
  int32 ExtractDigit(const String& value,
                     const char* next_to_digit = nullptr) const;
  void SetPurgeStrategy(PurgeStrategy* strategy);
  Timespan ExtractFactor(const String& value, const char* start) const;

  String path_;
  String times_;
  String rotation_;
  String archive_;
  bool compress_;
  String purge_age_;
  String purge_count_;
  bool flush_;
  bool rotate_on_open_;
  LogFile* log_file_;
  RotateStrategy* rotate_strategy_;
  ArchiveStrategy* archive_strategy_;
  PurgeStrategy* purge_strategy_;
  FastMutex mutex_;
};

}  // namespace fun

#endif  // FUN_WITH_FILE_SINK
