#include "fun/base/file_unix.h"
#include "fun/base/container/array.h"
#include "fun/base/error.h"
#include "fun/base/exception.h"

#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <algorithm>

#if FUN_PLATFORM_BSD_FAMILY
#include <sys/mount.h>
#include <sys/param.h>
#elif (FUN_PLATFORM == FUN_PLATFORM_SOLARIS) || \
    (FUN_PLATFORM == FUN_PLATFORM_QNX)
#include <sys/statvfs.h>
#else
#include <sys/statfs.h>
#endif

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <cstring>

#if FUN_PLATFORM == FUN_PLATFORM_SOLARIS
#define STATFSFN statvfs
#define STATFSSTRUCT statvfs
#else
#define STATFSFN statfs
#define STATFSSTRUCT statfs
#endif

namespace {

// Convert timespec structures (seconds and remaining nano secs) to TimeVal
// (microseconds)
fun::Timestamp::TimeVal Timespec2Microsecs(const struct timespec& ts) {
  return ts.tv_sec * 1000000L + ts.tv_nsec / 1000L;
}

}  // namespace

namespace fun {

FileImpl::FileImpl() {}

FileImpl::FileImpl(const String& path) : path_(path) {
  int32 n = path_.Len();
  if (n > 1 && path_[n - 1] == '/') {
    path_.Truncate(n - 1);
  }
}

FileImpl::~FileImpl() {}

void FileImpl::SwapImpl(FileImpl& file) { fun::Swap(path_, file.path_); }

void FileImpl::SetPathImpl(const String& path) {
  path_ = path;
  int32 n = path_.Len();
  if (n > 1 && path_[n - 1] == '/') {
    path_.Truncate(n - 1);
  }
}

bool FileImpl::ExistsImpl() const {
  fun_check(!path_.IsEmpty());

  struct stat st;
  return stat(path_.c_str(), &st) == 0;
}

bool FileImpl::CanReadImpl() const {
  fun_check(!path_.IsEmpty());

  struct stat st;
  if (stat(path_.c_str(), &st) == 0) {
    if (st.st_uid == geteuid()) {
      return (st.st_mode & S_IRUSR) != 0;
    } else if (st.st_gid == getegid()) {
      return (st.st_mode & S_IRGRP) != 0;
    } else {
      return (st.st_mode & S_IROTH) != 0 || geteuid() == 0;
    }
  } else {
    HandleLastErrorImpl(path_);
  }
  return false;
}

bool FileImpl::CanWriteImpl() const {
  fun_check(!path_.IsEmpty());

  struct stat st;
  if (stat(path_.c_str(), &st) == 0) {
    if (st.st_uid == geteuid()) {
      return (st.st_mode & S_IWUSR) != 0;
    } else if (st.st_gid == getegid()) {
      return (st.st_mode & S_IWGRP) != 0;
    } else {
      return (st.st_mode & S_IWOTH) != 0 || geteuid() == 0;
    }
  } else {
    HandleLastErrorImpl(path_);
  }
  return false;
}

bool FileImpl::CanExecuteImpl() const {
  fun_check(!path_.IsEmpty());

  struct stat st;
  if (stat(path_.c_str(), &st) == 0) {
    if (st.st_uid == geteuid() || geteuid() == 0) {
      return (st.st_mode & S_IXUSR) != 0;
    } else if (st.st_gid == getegid()) {
      return (st.st_mode & S_IXGRP) != 0;
    } else {
      return (st.st_mode & S_IXOTH) != 0;
    }
  } else {
    HandleLastErrorImpl(path_);
  }
  return false;
}

bool FileImpl::IsFileImpl() const {
  fun_check(!path_.IsEmpty());

  struct stat st;
  if (stat(path_.c_str(), &st) == 0) {
    return S_ISREG(st.st_mode);
  } else {
    HandleLastErrorImpl(path_);
  }
  return false;
}

bool FileImpl::IsDirectoryImpl() const {
  fun_check(!path_.IsEmpty());

  struct stat st;
  if (stat(path_.c_str(), &st) == 0) {
    return S_ISDIR(st.st_mode);
  } else {
    HandleLastErrorImpl(path_);
  }
  return false;
}

bool FileImpl::IsLinkImpl() const {
  fun_check(!path_.IsEmpty());

  struct stat st;
  if (lstat(path_.c_str(), &st) == 0) {
    return S_ISLNK(st.st_mode);
  } else {
    HandleLastErrorImpl(path_);
  }
  return false;
}

bool FileImpl::IsDeviceImpl() const {
  fun_check(!path_.IsEmpty());

  struct stat st;
  if (stat(path_.c_str(), &st) == 0) {
    return S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode);
  } else {
    HandleLastErrorImpl(path_);
  }
  return false;
}

bool FileImpl::IsHiddenImpl() const {
  fun_check(!path_.IsEmpty());
  Path p(path_);
  p.MakeFile();

  return p.GetFileName()[0] == '.';
}

Timestamp FileImpl::GetCreatedImpl() const {
  fun_check(!path_.IsEmpty());
// first, platforms with birthtime attributes
#if defined(__APPLE__) && defined(st_birthtime) && \
    !defined(FUN_NO_STAT64)  // st_birthtime is available only on 10.5
  // a macro st_birthtime makes sure there is a st_birthtimespec (nano sec
  // precision)
  struct stat64 st;
  if (stat64(path_.c_str(), &st) == 0) {
    return Timestamp(Timespec2Microsecs(st.st_birthtimespec));
  }
#elif defined(__FreeBSD__) && defined(st_birthtime)
  // a macro st_birthtime makes sure there is a st_birthtimespec (nano sec
  // precision)
  struct stat st;
  if (stat(path_.c_str(), &st) == 0) {
    return Timestamp(Timespec2Microsecs(st.st_birthtimespec));
  }
#elif defined(__FreeBSD__)
  struct stat st;
  if (stat(path_.c_str(), &st) == 0) {
    return Timestamp::FromEpochTime(st.st_birthtime);
  }
// then platforms with POSIX 2008-09 compatibility (nanosec precision)
// (linux 2.6 and later)
#elif (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200809L) || \
    (defined(_XOPEN_SOURCE) && _XOPEN_SOURCE >= 700)
  struct stat st;
  if (stat(path_.c_str(), &st) == 0) {
    return Timestamp(Timespec2Microsecs(st.st_ctim));
  }
// finally try just stat with status change with precision to the second.
#else
  struct stat st;
  if (stat(path_.c_str(), &st) == 0) {
    return Timestamp::FromEpochTime(st.st_ctime);
  }
#endif
  else {
    HandleLastErrorImpl(path_);
  }
  return 0;
}

Timestamp FileImpl::GetLastModifiedImpl() const {
  fun_check(!path_.IsEmpty());

  struct stat st;
  if (stat(path_.c_str(), &st) == 0) {
#if (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200809L) || \
    (defined(_XOPEN_SOURCE) && _XOPEN_SOURCE >= 700) ||         \
    defined(_BSD_SOURCE) || defined(_SVID_SOURCE)
    return Timestamp(Timespec2Microsecs(st.st_mtim));
#elif defined(__APPLE__)
    return Timestamp(Timespec2Microsecs(st.st_mtimespec));
#else
    return Timestamp::FromEpochTime(st.st_mtime);
#endif
  } else {
    HandleLastErrorImpl(path_);
  }
  return 0;
}

void FileImpl::SetLastModifiedImpl(const Timestamp& ts) {
  fun_check(!path_.IsEmpty());

  struct timeval tb[2];
  tb[0].tv_sec = ts.EpochMicroseconds() / 1000000;
  tb[0].tv_usec = ts.EpochMicroseconds() % 1000000;
  tb[1] = tb[0];
  if (utimes(path_.c_str(), tb) != 0) {
    HandleLastErrorImpl(path_);
  }
}

FileImpl::FileSizeImpl FileImpl::GetSizeImpl() const {
  fun_check(!path_.IsEmpty());

  struct stat st;
  if (stat(path_.c_str(), &st) == 0) {
    return st.st_size;
  } else {
    HandleLastErrorImpl(path_);
  }
  return 0;
}

void FileImpl::SetSizeImpl(FileSizeImpl size) {
  fun_check(!path_.IsEmpty());

  if (truncate(path_.c_str(), size) != 0) {
    HandleLastErrorImpl(path_);
  }
}

void FileImpl::SetWriteableImpl(bool flag) {
  fun_check(!path_.IsEmpty());

  struct stat st;
  if (stat(path_.c_str(), &st) != 0) {
    HandleLastErrorImpl(path_);
  }
  mode_t mode;
  if (flag) {
    mode = st.st_mode | S_IWUSR;
  } else {
    mode_t wmask = S_IWUSR | S_IWGRP | S_IWOTH;
    mode = st.st_mode & ~wmask;
  }
  if (chmod(path_.c_str(), mode) != 0) {
    HandleLastErrorImpl(path_);
  }
}

void FileImpl::SetExecutableImpl(bool flag) {
  fun_check(!path_.IsEmpty());

  struct stat st;
  if (stat(path_.c_str(), &st) != 0) {
    HandleLastErrorImpl(path_);
  }
  mode_t mode;
  if (flag) {
    mode = st.st_mode | S_IXUSR;
    if (st.st_mode & S_IRGRP) {
      mode |= S_IXGRP;
    }
    if (st.st_mode & S_IROTH) {
      mode |= S_IXOTH;
    }
  } else {
    mode_t wmask = S_IXUSR | S_IXGRP | S_IXOTH;
    mode = st.st_mode & ~wmask;
  }
  if (chmod(path_.c_str(), mode) != 0) {
    HandleLastErrorImpl(path_);
  }
}

void FileImpl::CopyToImpl(const String& path) const {
  fun_check(!path_.IsEmpty());

  int sd = open(path_.c_str(), O_RDONLY);
  if (sd == -1) {
    HandleLastErrorImpl(path_);
  }

  struct stat st;
  if (fstat(sd, &st) != 0) {
    close(sd);
    HandleLastErrorImpl(path_);
  }
  const long block_size = st.st_blksize;

  int dd = open(path.c_str(), O_CREAT | O_TRUNC | O_WRONLY, st.st_mode);
  if (dd == -1) {
    close(sd);
    HandleLastErrorImpl(path);
  }
  Array<char> buffer(block_size);
  try {
    int n;
    while ((n = read(sd, buffer.MutableData(), block_size)) > 0) {
      if (write(dd, buffer.ConstData(), n) != n) {
        HandleLastErrorImpl(path);
      }
    }
    if (n < 0) {
      HandleLastErrorImpl(path_);
    }
  } catch (...) {
    close(sd);
    close(dd);
    throw;
  }
  close(sd);
  if (fsync(dd) != 0) {
    close(dd);
    HandleLastErrorImpl(path);
  }
  if (close(dd) != 0) {
    HandleLastErrorImpl(path);
  }
}

void FileImpl::RenameToImpl(const String& path) {
  fun_check(!path_.IsEmpty());

  if (rename(path_.c_str(), path.c_str()) != 0) {
    HandleLastErrorImpl(path_);
  }
}

void FileImpl::LinkToImpl(const String& path, int type) const {
  fun_check(!path_.IsEmpty());

  if (type == 0) {
    if (link(path_.c_str(), path.c_str()) != 0) {
      HandleLastErrorImpl(path_);
    }
  } else {
    if (symlink(path_.c_str(), path.c_str()) != 0) {
      HandleLastErrorImpl(path_);
    }
  }
}

void FileImpl::RemoveImpl() {
  fun_check(!path_.IsEmpty());

  int rc;
  if (!IsLinkImpl() && IsDirectoryImpl()) {
    rc = rmdir(path_.c_str());
  } else {
    rc = unlink(path_.c_str());
  }
  if (rc) {
    HandleLastErrorImpl(path_);
  }
}

bool FileImpl::CreateFileImpl() {
  fun_check(!path_.IsEmpty());

  int n = open(path_.c_str(), O_WRONLY | O_CREAT | O_EXCL,
               S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
  if (n != -1) {
    close(n);
    return true;
  }
  if (n == -1 && errno == EEXIST) {
    return false;
  } else {
    HandleLastErrorImpl(path_);
  }
  return false;
}

bool FileImpl::CreateDirectoryImpl() {
  fun_check(!path_.IsEmpty());

  if (ExistsImpl() && IsDirectoryImpl()) {
    return false;
  }
  if (mkdir(path_.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) != 0) {
    HandleLastErrorImpl(path_);
  }
  return true;
}

FileImpl::FileSizeImpl FileImpl::TotalSpaceImpl() const {
  fun_check(!path_.IsEmpty());

  struct STATFSSTRUCT stats;
  if (STATFSFN(const_cast<char*>(path_.c_str()), &stats) != 0) {
    HandleLastErrorImpl(path_);
  }

  return (FileSizeImpl)stats.f_blocks * (FileSizeImpl)stats.f_bsize;
}

FileImpl::FileSizeImpl FileImpl::UsableSpaceImpl() const {
  fun_check(!path_.IsEmpty());

  struct STATFSSTRUCT stats;
  if (STATFSFN(const_cast<char*>(path_.c_str()), &stats) != 0) {
    HandleLastErrorImpl(path_);
  }

  return (FileSizeImpl)stats.f_bavail * (FileSizeImpl)stats.f_bsize;
}

FileImpl::FileSizeImpl FileImpl::FreeSpaceImpl() const {
  fun_check(!path_.IsEmpty());

  struct STATFSSTRUCT stats;
  if (STATFSFN(const_cast<char*>(path_.c_str()), &stats) != 0) {
    HandleLastErrorImpl(path_);
  }

  return (FileSizeImpl)stats.f_bfree * (FileSizeImpl)stats.f_bsize;
}

void FileImpl::HandleLastErrorImpl(const String& path) {
  switch (errno) {
    case EIO:
      throw IOException(path, errno);
    case EPERM:
      throw FileAccessDeniedException("insufficient permissions", path, errno);
    case EACCES:
      throw FileAccessDeniedException(path, errno);
    case ENOENT:
      throw FileNotFoundException(path, errno);
    case ENOTDIR:
      throw OpenFileException("not a directory", path, errno);
    case EISDIR:
      throw OpenFileException("not a file", path, errno);
    case EROFS:
      throw FileReadOnlyException(path, errno);
    case EEXIST:
      throw FileExistsException(path, errno);
    case ENOSPC:
      throw FileException("no space left on device", path, errno);
    case EDQUOT:
      throw FileException("disk quota exceeded", path, errno);
#if !defined(_AIX)
    case ENOTEMPTY:
      throw DirectoryNotEmptyException(path, errno);
#endif
    case ENAMETOOLONG:
      throw PathSyntaxException(path, errno);
    case ENFILE:
    case EMFILE:
      throw FileException("too many open files", path, errno);
    default:
      throw FileException(Error::GetMessage(errno), path, errno);
  }
}

}  // namespace fun
