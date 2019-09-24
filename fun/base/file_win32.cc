#include "fun/base/file_win32.h"
#include "fun/base/exception.h"
#include "fun/base/path.h"
#include "fun/base/str.h"
#include "fun/base/windows_less.h"

namespace fun {

//namespace {

class FileHandle {
 public:
  FileHandle(const String& path, const UString& upath, DWORD access, DWORD share, DWORD disp) {
    handle_ = CreateFileW(upath.c_str(), access, share, 0, disp, 0, 0);
    if (handle_ == INVALID_HANDLE_VALUE) {
      FileImpl::HandleLastErrorImpl(path);
    }
  }

  ~FileHandle() {
    if (handle_ != INVALID_HANDLE_VALUE) {
      CloseHandle(handle_);
    }
  }

  HANDLE Get() const {
    return handle_;
  }

 private:
  HANDLE handle_;
};

//} // namespace


FileImpl::FileImpl() {}

FileImpl::FileImpl(const String& path) : path_(path) {
  int32 n = path_.Len();
  if (n > 1 && (path_[n - 1] == '\\' || path_[n - 1] == '/') && !((n == 3 && path_[1]==':'))) {
    path_.Truncate(n - 1);
  }
  ConvertPath(path_, upath_);
}

FileImpl::~FileImpl() {}

void FileImpl::SwapImpl(FileImpl& file) {
  fun::Swap(path_, file.path_);
  fun::Swap(upath_, file.upath_);
}

void FileImpl::SetPathImpl(const String& path) {
  path_ = path;
  int32 n = path_.Len();
  if (n > 1 && (path_[n - 1] == '\\' || path_[n - 1] == '/') && !((n == 3 && path_[1]==':'))) {
    path_.Truncate(n - 1);
  }
  ConvertPath(path_, upath_);
}

bool FileImpl::ExistsImpl() const {
  fun_check(!path_.IsEmpty());

  DWORD attr = GetFileAttributesW(upath_.c_str());
  if (attr == INVALID_FILE_ATTRIBUTES) {
    switch (GetLastError()) {
      case ERROR_FILE_NOT_FOUND:
      case ERROR_PATH_NOT_FOUND:
      case ERROR_NOT_READY:
      case ERROR_INVALID_DRIVE:
        return false;
      default:
        HandleLastErrorImpl(path_);
    }
  }
  return true;
}

bool FileImpl::CanReadImpl() const {
  fun_check(!path_.IsEmpty());

  DWORD attr = GetFileAttributesW(upath_.c_str());
  if (attr == INVALID_FILE_ATTRIBUTES) {
    switch (GetLastError()) {
      case ERROR_ACCESS_DENIED:
        return false;
      default:
        HandleLastErrorImpl(path_);
    }
  }
  return true;
}

bool FileImpl::CanWriteImpl() const {
  fun_check(!path_.IsEmpty());

  DWORD attr = GetFileAttributesW(upath_.c_str());
  if (attr == INVALID_FILE_ATTRIBUTES) {
    HandleLastErrorImpl(path_);
  }
  return (attr & FILE_ATTRIBUTE_READONLY) == 0;
}

bool FileImpl::CanExecuteImpl() const {
  Path p(path_);
  return icompare(p.GetExtension(), "exe") == 0;
}

bool FileImpl::IsFileImpl() const {
  return !IsDirectoryImpl() && !IsDeviceImpl();
}

bool FileImpl::IsDirectoryImpl() const {
  fun_check(!path_.IsEmpty());

  const DWORD attr = GetFileAttributesW(upath_.c_str());
  if (attr == INVALID_FILE_ATTRIBUTES) {
    HandleLastErrorImpl(path_);
  }
  return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

bool FileImpl::IsLinkImpl() const {
  fun_check(!path_.IsEmpty());

  const DWORD attr = GetFileAttributesW(upath_.c_str());
  if (attr == INVALID_FILE_ATTRIBUTES) {
    HandleLastErrorImpl(path_);
  }
  return (attr & FILE_ATTRIBUTE_DIRECTORY) == 0 && (attr & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
}

bool FileImpl::IsDeviceImpl() const {
  return
    //TODO
    //path_.Compare(0, 4, "\\\\.\\") == 0 ||
    path_.MidRef(0,4).Compare("\\\\.\\") == 0 ||
    icompare(path_, "CON") == 0 ||
    icompare(path_, "PRN") == 0 ||
    icompare(path_, "AUX") == 0 ||
    icompare(path_, "NUL") == 0 ||
    ( (icompare(path_, 0, 3, "LPT") == 0 || icompare(path_, 0, 3, "COM") == 0) &&
       path_.Len() == 4 &&
       path_[3] > 0x30   &&
       isdigit(path_[3])
    );
}

bool FileImpl::IsHiddenImpl() const {
  fun_check(!path_.IsEmpty());

  const DWORD attr = GetFileAttributesW(upath_.c_str());
  if (attr == INVALID_FILE_ATTRIBUTES) {
    HandleLastErrorImpl(path_);
  }
  return (attr & FILE_ATTRIBUTE_HIDDEN) != 0;
}

Timestamp FileImpl::GetCreatedImpl() const {
  fun_check(!path_.IsEmpty());

  WIN32_FILE_ATTRIBUTE_DATA fad;
  if (GetFileAttributesExW(upath_.c_str(), GetFileExInfoStandard, &fad) == 0) {
    HandleLastErrorImpl(path_);
  }
  return Timestamp::FromFileTimeNP(fad.ftCreationTime.dwLowDateTime, fad.ftCreationTime.dwHighDateTime);
}

Timestamp FileImpl::GetLastModifiedImpl() const {
  fun_check(!path_.IsEmpty());

  WIN32_FILE_ATTRIBUTE_DATA fad;
  if (GetFileAttributesExW(upath_.c_str(), GetFileExInfoStandard, &fad) == 0) {
    HandleLastErrorImpl(path_);
  }
  return Timestamp::FromFileTimeNP(fad.ftLastWriteTime.dwLowDateTime, fad.ftLastWriteTime.dwHighDateTime);
}

void FileImpl::SetLastModifiedImpl(const Timestamp& ts) {
  fun_check(!path_.IsEmpty());

  uint32 low;
  uint32 high;
  ts.ToFileTimeNP(low, high);
  FILETIME ft;
  ft.dwLowDateTime = low;
  ft.dwHighDateTime = high;
  FileHandle fh(path_, upath_, FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE, OPEN_EXISTING);
  if (SetFileTime(fh.Get(), 0, &ft, &ft) == 0) {
    HandleLastErrorImpl(path_);
  }
}

FileImpl::FileSizeImpl FileImpl::GetSizeImpl() const {
  fun_check(!path_.IsEmpty());

  WIN32_FILE_ATTRIBUTE_DATA fad;
  if (GetFileAttributesExW(upath_.c_str(), GetFileExInfoStandard, &fad) == 0) {
    HandleLastErrorImpl(path_);
  }
  LARGE_INTEGER li;
  li.LowPart  = fad.nFileSizeLow;
  li.HighPart = fad.nFileSizeHigh;
  return li.QuadPart;
}

void FileImpl::SetSizeImpl(FileSizeImpl size) {
  fun_check(!path_.IsEmpty());

  FileHandle fh(path_, upath_, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, OPEN_EXISTING);
  LARGE_INTEGER li;
  li.QuadPart = size;
  if (SetFilePointer(fh.Get(), li.LowPart, &li.HighPart, FILE_BEGIN) == INVALID_SET_FILE_POINTER) {
    HandleLastErrorImpl(path_);
  }
  if (SetEndOfFile(fh.Get()) == 0) {
    HandleLastErrorImpl(path_);
  }
}

void FileImpl::SetWriteableImpl(bool flag) {
  fun_check(!path_.IsEmpty());

  DWORD attr = GetFileAttributesW(upath_.c_str());
  if (attr == -1) {
    HandleLastErrorImpl(path_);
  }
  if (flag) {
    attr &= ~FILE_ATTRIBUTE_READONLY;
  } else {
    attr |= FILE_ATTRIBUTE_READONLY;
  }
  if (SetFileAttributesW(upath_.c_str(), attr) == 0) {
    HandleLastErrorImpl(path_);
  }
}

void FileImpl::SetExecutableImpl(bool /*flag*/) {
  // not supported
}

void FileImpl::CopyToImpl(const String& path) const {
  fun_check(!path_.IsEmpty());

  UString upath;
  ConvertPath(path, upath);
  if (CopyFileW(upath_.c_str(), upath.c_str(), FALSE) == 0) {
    HandleLastErrorImpl(path_);
  }
}

void FileImpl::RenameToImpl(const String& path) {
  fun_check(!path_.IsEmpty());

  UString upath;
  ConvertPath(path, upath);
  if (MoveFileExW(upath_.c_str(), upath.c_str(), MOVEFILE_REPLACE_EXISTING) == 0) {
    HandleLastErrorImpl(path_);
  }
}

void FileImpl::LinkToImpl(const String& path, int type) const {
  fun_check(!path_.IsEmpty());

  UString upath;
  ConvertPath(path, upath);

  if (type == 0) {
    if (CreateHardLinkW(upath.c_str(), upath_.c_str(), NULL) == 0) {
      HandleLastErrorImpl(path_);
    }
  } else {
#if _WIN32_WINNT >= 0x0600 && defined(SYMBOLIC_LINK_FLAG_DIRECTORY)
    DWORD flags = 0;
    if (IsDirectoryImpl()) {
      flags |= SYMBOLIC_LINK_FLAG_DIRECTORY;
    }
#ifdef SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE
    flags |= SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE;
#endif
    if (CreateSymbolicLinkW(upath.c_str(), upath_.c_str(), flags) == 0) {
      HandleLastErrorImpl(path_);
    }
#else
    throw NotImplementedException("Symbolic link support not available in used version of the Windows SDK");
#endif
  }
}

void FileImpl::RemoveImpl() {
  fun_check(!path_.IsEmpty());

  if (IsDirectoryImpl()) {
    if (RemoveDirectoryW(upath_.c_str()) == 0) {
      HandleLastErrorImpl(path_);
    }
  } else {
    if (DeleteFileW(upath_.c_str()) == 0) {
      HandleLastErrorImpl(path_);
    }
  }
}

bool FileImpl::CreateFileImpl() {
  fun_check(!path_.IsEmpty());

  HANDLE file_handle = CreateFileW(upath_.c_str(), GENERIC_WRITE, 0, 0, CREATE_NEW, 0, 0);
  if (file_handle != INVALID_HANDLE_VALUE) {
    CloseHandle(file_handle);
    return true;
  } else if (GetLastError() == ERROR_FILE_EXISTS) {
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
  if (CreateDirectoryW(upath_.c_str(), 0) == 0) {
    HandleLastErrorImpl(path_);
  }
  return true;
}

FileImpl::FileSizeImpl FileImpl::TotalSpaceImpl() const {
  fun_check(!path_.IsEmpty());

  ULARGE_INTEGER space;
  if (!GetDiskFreeSpaceExW(upath_.c_str(), NULL, &space, NULL)) {
    HandleLastErrorImpl(path_);
  }
  return space.QuadPart;
}

FileImpl::FileSizeImpl FileImpl::UsableSpaceImpl() const {
  fun_check(!path_.IsEmpty());

  ULARGE_INTEGER space;
  if (!GetDiskFreeSpaceExW(upath_.c_str(), &space, NULL, NULL)) {
    HandleLastErrorImpl(path_);
  }
  return space.QuadPart;
}

FileImpl::FileSizeImpl FileImpl::FreeSpaceImpl() const {
  fun_check(!path_.IsEmpty());

  ULARGE_INTEGER space;
  if (!GetDiskFreeSpaceExW(upath_.c_str(), NULL, NULL, &space)) {
    HandleLastErrorImpl(path_);
  }
  return space.QuadPart;
}

void FileImpl::HandleLastErrorImpl(const String& path) {
  const DWORD err = GetLastError();
  switch (err) {
    case ERROR_FILE_NOT_FOUND:
      throw FileNotFoundException(path, err);
    case ERROR_PATH_NOT_FOUND:
    case ERROR_BAD_NETPATH:
    case ERROR_CANT_RESOLVE_FILENAME:
    case ERROR_INVALID_DRIVE:
      throw PathNotFoundException(path, err);
    case ERROR_ACCESS_DENIED:
      throw FileAccessDeniedException(path, err);
    case ERROR_ALREADY_EXISTS:
    case ERROR_FILE_EXISTS:
      throw FileExistsException(path, err);
    case ERROR_INVALID_NAME:
    case ERROR_DIRECTORY:
    case ERROR_FILENAME_EXCED_RANGE:
    case ERROR_BAD_PATHNAME:
      throw PathSyntaxException(path, err);
    case ERROR_FILE_READ_ONLY:
      throw FileReadOnlyException(path, err);
    case ERROR_CANNOT_MAKE:
      throw CreateFileException(path, err);
    case ERROR_DIR_NOT_EMPTY:
      throw DirectoryNotEmptyException(path, err);
    case ERROR_WRITE_FAULT:
      throw WriteFileException(path, err);
    case ERROR_READ_FAULT:
      throw ReadFileException(path, err);
    case ERROR_SHARING_VIOLATION:
      throw FileException("sharing violation", path, err);
    case ERROR_LOCK_VIOLATION:
      throw FileException("lock violation", path, err);
    case ERROR_HANDLE_EOF:
      throw ReadFileException("EOF reached", path, err);
    case ERROR_HANDLE_DISK_FULL:
    case ERROR_DISK_FULL:
      throw WriteFileException("disk is full", path, err);
    case ERROR_NEGATIVE_SEEK:
      throw FileException("negative seek", path, err);
    default:
      throw FileException(path, err);
  }
}

void FileImpl::ConvertPath(const String& utf8_path, UString& utf16_path) {
  utf16_path.FromUtf8(utf8_path);

  if (utf16_path.Len() >= MAX_PATH - 12) { // Note: CreateDirectory has a limit of MAX_PATH - 12 (room for 8.3 file name)
    if (utf16_path[0] == UTEXT('\\') || utf16_path[1] == UTEXT(':')) {
      //TODO String / UString에 길이를 부여하여 비교하는 함수가 없음.
      //if (utf16_path.Compare(UTEXT("\\\\?\\"), 4) != 0) {
      if (utf16_path.MidRef(0,4).Compare(UTEXT("\\\\?\\")) != 0) {
        if (utf16_path[1] == UTEXT('\\')) {
          utf16_path.Insert(0, UTEXT("\\\\?\\UNC\\"), 8);
        } else {
          utf16_path.Insert(0, UTEXT("\\\\?\\"), 4);
        }
      }
    }
  }
}

} // namespace fun
