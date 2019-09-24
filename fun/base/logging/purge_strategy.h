#pragma once

#include "fun/base/base.h"
#include "fun/base/container/array.h"
#include "fun/base/file.h"
#include "fun/base/timespan.h"

namespace fun {

/**
 * The PurgeStrategy is used by FileSink to purge archived log files.
 */
class FUN_BASE_API PurgeStrategy {
 public:
  PurgeStrategy();
  virtual ~PurgeStrategy();

  /**
   * Purges archived log files. The path to the
   * current "hot" log file is given.
   * To find archived log files, look for files
   * with a name consisting of the given path
   * plus any suffix (e.g., .1, .20050929081500, .1.gz).
   * A list of archived files can be obtained by calling
   * the List() method.
   */
  virtual void Purge(const String& path) = 0;

  FUN_DISALLOW_COPY_AND_ASSIGNMENT(PurgeStrategy);

 protected:
  /**
   * Fills the given vector with a list of archived log
   * files. The path of the current "Hot" log file is
   * given in path.
   *
   * All files with the same name as the one given in path,
   * plus some suffix (e.g., .1, .20050929081500, .1.gz) are
   * considered archived files.
   */
  void List(const String& path, Array<File>& files);
};

/**
 * This purge strategy purges all files that have
 * exceeded a given age (given in seconds).
 */
class FUN_BASE_API PurgeByAgeStrategy : public PurgeStrategy {
 public:
  PurgeByAgeStrategy(const Timespan& age);
  ~PurgeByAgeStrategy();

  void Purge(const String& path) override;

 private:
  Timespan age_;
};

/**
 * This purge strategy ensures that a maximum number
 * of archived files is not exceeded. Files are deleted
 * based on their age, with oldest files deleted first.
 */
class FUN_BASE_API PurgeByCountStrategy : public PurgeStrategy {
 public:
  PurgeByCountStrategy(int32 count);
  ~PurgeByCountStrategy();

  void Purge(const String& path) override;

 private:
  int32 count_;
};

}  // namespace fun
