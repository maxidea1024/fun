#include "fun/base/sorted_directory_iterator.h"
#include <algorithm>

namespace fun {

SortedDirectoryIterator::SortedDirectoryIterator()
  : DirectoryIterator(),
    is_finished_(true) {
}

SortedDirectoryIterator::SortedDirectoryIterator(const String& path)
  : DirectoryIterator(path),
    is_finished_(false) {
  Scan();
  Next();
}

SortedDirectoryIterator::SortedDirectoryIterator(const SortedDirectoryIterator& iterator)
  : DirectoryIterator(iterator),
    is_finished_(false) {
  Scan();
  Next();
}

SortedDirectoryIterator::SortedDirectoryIterator(const File& file)
  : DirectoryIterator(file),
    is_finished_(false) {
  Scan();
  Next();
}

SortedDirectoryIterator::SortedDirectoryIterator(const Path& path)
  : DirectoryIterator(path),
    is_finished_(false) {
  Scan();
  Next();
}

SortedDirectoryIterator::~SortedDirectoryIterator() {
  // NOOP
}

SortedDirectoryIterator& SortedDirectoryIterator::operator ++ () {
  if (!is_finished_) {
    Next();
  }
  return *this;
}

void SortedDirectoryIterator::Scan() {
  DirectoryIterator end_it;
  while (*this != end_it) {
    bool is_dir = false;
    try {
      is_dir = (*this)->IsDirectory();
    } catch (...) {}

    if (is_dir) {
      directories_.PushBack(path_.ToString());
    } else {
      files_.PushBack(path_.ToString());
    }

    DirectoryIterator::operator++();
  }

  //TODO List라 sorting이 안될텐데...
  //그냥 배열로 처리해야할까??
  //std::sort(directories_.begin(), directories_.end());
  //std::sort(files_.begin(), files_.end());
  directories_.Sort();
  files_.Sort();
}

void SortedDirectoryIterator::Next() {
  DirectoryIterator end_it;
  if (!directories_.IsEmpty()) {
    path_.Assign(directories_.Front());
    directories_.PopFront();
    file_ = path_;
  } else if (!files_.IsEmpty()) {
    path_.Assign(files_.Front());
    files_.PopFront();
    file_ = path_;
  } else {
    is_finished_ = true;
    path_ = end_it.GetPath();
    file_ = path_;
  }
}

} // namespace fun
