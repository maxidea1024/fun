#include "fun/base/directory_iterator_unix.h"

#if FUN_PLATFORM == FUN_PLATFORM_VXWORKS
#include "fun/base/file_vx.h"
#else
#include "fun/base/file_unix.h"
#endif

#include "fun/base/path.h"

namespace fun {

DirectoryIteratorImpl::DirectoryIteratorImpl(const String& path)
  : dir_(nullptr), rc_(1) {
  Path p(path);
  p.MakeFile();

#if FUN_PLATFORM == FUN_PLATFORM_VXWORKS
  dir_ = opendir(const_cast<char*>(p.ToString().c_str()));
#else
  dir_ = opendir(p.ToString().c_str());
#endif
  if (!dir_) {
    File::HandleLastError(path);
  }

  Next();
}

DirectoryIteratorImpl::~DirectoryIteratorImpl() {
  if (dir_) {
    closedir(dir_);
  }
}

const String& DirectoryIteratorImpl::Next() {
  do {
    struct dirent* entry = readdir(dir_);
    if (entry) {
      current_ = entry->d_name;
    } else {
      current_.Clear();
    }
  } while (current_ == "." || current_ == "..");

  return current_;
}

} // namespace fun
