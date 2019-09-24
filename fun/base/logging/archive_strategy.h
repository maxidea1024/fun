#pragma once

#include "fun/base/base.h"
#include "fun/base/date_time.h"
#include "fun/base/string/string.h"

#define FUN_WITH_ARCHIVE_STRATEGY 0

#if FUN_WITH_ARCHIVE_STRATEGY

namespace fun {

class ArchiveCompressor;
class LogFile;

/**
 * The ArchiveStrategy is used by FileSink to rename a rotated log file for
 * archiving.
 *
 * Archived files can be automatically compressed,
 * using the gzip file format.
 */
class FUN_BASE_API ArchiveStrategy {
 public:
  ArchiveStrategy();
  virtual ~ArchiveStrategy();

  // Disable copy and assignment.
  ArchiveStrategy(const ArchiveStrategy&) = delete;
  ArchiveStrategy& operator=(const ArchiveStrategy&) = delete;

  // TODO 이름을 어떤것으로 변경하는게 좋으려나...
  /**
   * Renames the given log file for archiving
   * and creates and returns a new log file.
   * The given LogFile object is deleted.
   */
  virtual LogFile* Archive(LogFile* file) = 0;

  /**
   * Enables or disables compression of archived files.
   */
  void Compress(bool flag = true);

 protected:
  void MoveFile(const String& old_name, const String& new_name);
  bool Exists(const String& name);

 private:
  bool compress_;
  ArchiveCompressor* compressor_;
};

/**
 * A monotonic increasing number is appended to the
 * log file name. The most recent archived file
 * always has the number zero.
 */
class FUN_BASE_API ArchiveByNumberStrategy : public ArchiveStrategy {
 public:
  ArchiveByNumberStrategy();
  ~ArchiveByNumberStrategy();

  LogFile* Archive(LogFile* file) override;
};

// TODO DT를 별도로 지정할 필요가 없음. 그냥 설정으로 하면됨!!!
/**
 * A timestamp (YYYYMMDDhhmmssiii) is appended to archived
 * log files.
 */
template <typename DT>
class ArchiveByTimestampStrategy : public ArchiveStrategy {
 public:
  ArchiveByTimestampStrategy() {}
  ~ArchiveByTimestampStrategy() {}

  /**
   * Archives the file by appending the current timestamp to the
   * file name. If the new file name exists, additionally a monotonic
   * increasing number is appended to the log file name.
   */
  LogFile* Archive(LogFile* file) override {
    String path = file->GetPath();
    delete file;
    String arch_path = path;
    arch_path.Append(".");
    DateTimeFormatter::Append(arch_path, DT().timestamp(), "%Y%m%d%H%M%S%i");

    if (Exists(arch_path)) {
      ArchiveByNumber(arch_path);
    } else {
      MoveFile(path, arch_path);
    }

    return new LogFile(path);
  }

 private:
  /**
   * A monotonic increasing number is appended to the
   * log file name. The most recent archived file
   * always has the number zero.
   */
  void ArchiveByNumber(const String& base_path) {
    int n = -1;
    String path;
    do {
      path = base_path;
      path.Append(".");
      NumberFormatter::Append(path, ++n);
    } while (Exists(path));

    while (n >= 0) {
      String old_path = base_path;
      if (n > 0) {
        old_path.Append(".");
        NumberFormatter::Append(old_path, n - 1);
      }
      String new_path = base_path;
      new_path.Append(".");
      NumberFormatter::Append(new_path, n);
      MoveFile(old_path, new_path);
      --n;
    }
  }
};

}  // namespace fun

#endif  // FUN_WITH_ARCHIVE_STRATEGY
