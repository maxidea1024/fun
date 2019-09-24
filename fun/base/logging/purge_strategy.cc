#include "fun/base/logging/purge_strategy.h"
#include "fun/base/path.h"
#include "fun/base/directory_iterator.h"
#include "fun/base/timestamp.h"
#include "fun/base/string/string.h"

namespace fun {

//
// PurgeStrategy
//

PurgeStrategy::PurgeStrategy() {}

PurgeStrategy::~PurgeStrategy() {}

void PurgeStrategy::List(const String& path, Array<File>& files) {
  Path p(path);
  p.MakeAbsolute();
  Path parent = p.Parent();
  String base_name = p.GetFileName();
  base_name.Append(".");

  DirectoryIterator it(parent);
  DirectoryIterator end;
  while (it != end) {
    if (it.GetName().StartsWith(base_name)) {
      files.Add(*it);
    }
    ++it;
  }
}


//
// PurgeByAgeStrategy
//

PurgeByAgeStrategy::PurgeByAgeStrategy(const Timespan& age) : age_(age) {}

PurgeByAgeStrategy::~PurgeByAgeStrategy() {}

void PurgeByAgeStrategy::Purge(const String& path) {
  Array<File> files;
  List(path, files);

  for (auto& file : files) {
    if (file.GetLastModified().IsElapsed(age_.TotalMicroseconds())) {
      file.Remove();
    }
  }
}


//
// PurgeByCountStrategy
//

PurgeByCountStrategy::PurgeByCountStrategy(int32 count) : count_(count) {
  fun_check(count > 0);
}

PurgeByCountStrategy::~PurgeByCountStrategy() {}

void PurgeByCountStrategy::Purge(const String& path) {
  // collect log file list.
  Array<File> files;
  List(path, files);

  // delete the oldest file if there are more files than the set number.
  while (files.Count() > count_) {
    int32 oldest_index = 0;
    Timestamp oldest_ts = files[0].GetLastModified();

    for (int32 i = 1; i < files.Count(); ++i) {
      Timestamp md = files[i].GetLastModified();
      if (md <= oldest_ts) {
        oldest_ts = md;
        oldest_index = i;
      }
    }

    files[oldest_index].Remove();
    files.RemoveAt(oldest_index);
  }
}

} // namespace fun
