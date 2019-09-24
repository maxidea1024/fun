#include "fun/base/directory_iterator_win32.h"
#include "fun/base/string/string.h"
#include "fun/base/path.h"

#if defined(_WIN32_WCE)
#include "fun/base/file_wince.h"
#elif FUN_PLATFORM_WINDOWS_FAMILY
#include "fun/base/file_win32.h"
#else
#include "fun/base/file_unix.h"
#endif

namespace fun {

DirectoryIteratorImpl::DirectoryIteratorImpl(const String& path)
  : fh_(INVALID_HANDLE_VALUE), rc_(1) {
  Path p(path);
  p.MakeDirectory();
  String find_path = p.ToString();
  find_path.Append("*");
  UString upath;
  FileImpl::ConvertPath(find_path, upath);

  fh_ = FindFirstFileW(upath.c_str(), &fd_);
  if (fh_ == INVALID_HANDLE_VALUE) {
    if (GetLastError() != ERROR_NO_MORE_FILES) {
      FileImpl::HandleLastErrorImpl(path);
    }
  } else {
    current_ = WCHAR_TO_UTF8(fd_.cFileName);
    if (current_ == "." || current_ == "..") {
      Next();
    }
  }
}

DirectoryIteratorImpl::~DirectoryIteratorImpl() {
  if (fh_ != INVALID_HANDLE_VALUE) {
    FindClose(fh_);
  }
}

const String& DirectoryIteratorImpl::Next() {
  do {
    current_.Clear();
    if (FindNextFileW(fh_, &fd_) != 0) {
      current_ = WCHAR_TO_UTF8(fd_.cFileName);
    }
  } while (current_ == "." || current_ == "..");
  return current_;
}

} // namespace fun
