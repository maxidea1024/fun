#include "fun/base/logging/log_file_win32.h"
#include "fun/base/exception.h"
#include "fun/base/file.h"

namespace fun {

LogFileImpl::LogFileImpl(const String& path)
    : path_(path), file_handle_(INVALID_HANDLE_VALUE) {
  File file(path);
  if (file.Exists()) {
    if (GetSizeImpl() == 0) {
      creation_date_ = file.GetLastModified();
    } else {
      creation_date_ = file.GetCreated();
    }
  }
}

LogFileImpl::~LogFileImpl() {
  if (file_handle_ != INVALID_HANDLE_VALUE) {
    CloseHandle(file_handle_);
    file_handle_ = INVALID_HANDLE_VALUE;
  }
}

void LogFileImpl::WriteImpl(const String& text, bool flush) {
  if (INVALID_HANDLE_VALUE == file_handle_) {
    CreateFile();
  }

  DWORD written;
  BOOL res = WriteFile(file_handle_, text.ConstData(), (DWORD)text.Len(),
                       &written, NULL);
  if (!res) {
    throw WriteFileException(path_);
  }

  res = WriteFile(file_handle_, "\r\n", 2, &written, NULL);
  if (!res) {
    throw WriteFileException(path_);
  }

  if (flush) {
    res = FlushFileBuffers(file_handle_);
    if (!res) {
      throw WriteFileException(path_);
    }
  }
}

uint64 LogFileImpl::GetSizeImpl() const {
  if (INVALID_HANDLE_VALUE == file_handle_) {
    File file(path_);
    if (file.Exists()) {
      return file.GetSize();
    } else {
      return 0;
    }
  }

  LARGE_INTEGER li;
  li.HighPart = 0;
  li.LowPart = SetFilePointer(file_handle_, 0, &li.HighPart, FILE_CURRENT);
  return li.QuadPart;
}

Timestamp LogFileImpl::GetCreationDateImpl() const { return creation_date_; }

const String& LogFileImpl::GetPathImpl() const { return path_; }

void LogFileImpl::CreateFile() {
  UString upath;
  FileImpl::ConvertPath(path_, upath);

  file_handle_ = CreateFileW(upath.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
                             NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (file_handle_ == INVALID_HANDLE_VALUE) {
    throw OpenFileException(path_);
  }

  SetFilePointer(file_handle_, 0, 0, FILE_END);

  // There seems to be a strange "optimization" in the Windows NTFS
  // filesystem that causes it to reuse directory entries of deleted
  // files. Example:
  // 1. create a file named "test.dat"
  //    note the file's creation date
  // 2. delete the file "test.dat"
  // 3. wait a few seconds
  // 4. create a file named "test.dat"
  //    the new file will have the same creation
  //    date as the old one.
  // We work around this bug by taking the file's
  // modification date as a reference when the
  // file is empty.
  if (GetSizeImpl() == 0) {
    creation_date_ = File(path_).GetLastModified();
  } else {
    creation_date_ = File(path_).GetCreated();
  }
}

}  // namespace fun
