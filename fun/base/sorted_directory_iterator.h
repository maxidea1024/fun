#pragma once

#include "fun/base/base.h"
#include "fun/base/directory_iterator.h"
#include "fun/base/file.h"
#include "fun/base/path.h"
// TODO List<T>가 sorting이 안되므로 일단은 Array<T>로 대체함. 차후에 이부분에
// 대해서는 고민이 필요해보임. List<T>가 sorting이 되면 좋을텐데...
#include "fun/base/container/array.h"
#include "fun/base/container/list.h"

namespace fun {

/**
 * The SortedDirectoryIterator class is similar to
 * SortedDirectoryIterator class, but places directories before files
 * and sorts content alphabetically.
 */
class FUN_BASE_API SortedDirectoryIterator : public DirectoryIterator {
 public:
  /**
   * Creates the end iterator.
   */
  SortedDirectoryIterator();

  /**
   * Creates a directory iterator for the given path.
   */
  SortedDirectoryIterator(const String& path);

  /**
   * Creates a directory iterator for the given path.
   */
  SortedDirectoryIterator(const SortedDirectoryIterator& iterator);

  /**
   * Creates a directory iterator for the given file.
   */
  SortedDirectoryIterator(const File& file);

  /**
   * Creates a directory iterator for the given path.
   */
  SortedDirectoryIterator(const Path& path);

  /**
   * Destroys the SortedDirectoryIterator.
   */
  virtual ~SortedDirectoryIterator();

  virtual SortedDirectoryIterator& operator++();  // prefix

 protected:
  bool is_finished_;
  // List<String> directories_;
  // List<String> files_;
  Array<String> directories_;
  Array<String> files_;

  void Next();
  void Scan();
};

}  // namespace fun
