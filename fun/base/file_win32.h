#pragma once

#include "fun/base/base.h"
#include "fun/base/timestamp.h"
#include "fun/base/string/string.h"

namespace fun {

class FUN_BASE_API FileImpl {
 protected:
  typedef uint64 FileSizeImpl;

  FileImpl();
  FileImpl(const String& path);
  virtual ~FileImpl();

  void SwapImpl(FileImpl& file);
  void SetPathImpl(const String& path);
  const String& GetPathImpl() const;
  bool ExistsImpl() const;
  bool CanReadImpl() const;
  bool CanWriteImpl() const;
  bool CanExecuteImpl() const;
  bool IsFileImpl() const;
  bool IsDirectoryImpl() const;
  bool IsLinkImpl() const;
  bool IsDeviceImpl() const;
  bool IsHiddenImpl() const;
  Timestamp GetCreatedImpl() const;
  Timestamp GetLastModifiedImpl() const;
  void SetLastModifiedImpl(const Timestamp& ts);
  FileSizeImpl GetSizeImpl() const;
  void SetSizeImpl(FileSizeImpl size);
  void SetWriteableImpl(bool flag = true);
  void SetExecutableImpl(bool flag = true);
  void CopyToImpl(const String& path) const;
  void RenameToImpl(const String& path);
  void LinkToImpl(const String& path, int type) const;
  void RemoveImpl();
  bool CreateFileImpl();
  bool CreateDirectoryImpl();
  FileSizeImpl TotalSpaceImpl() const;
  FileSizeImpl UsableSpaceImpl() const;
  FileSizeImpl FreeSpaceImpl() const;
  static void HandleLastErrorImpl(const String& path);
  static void ConvertPath(const String& utf8_path, UString& utf16_path);

 private:
  String path_;
  UString upath_;

  friend class FileHandle;
  friend class DirectoryIteratorImpl;
  friend class WindowsDirectoryWatcherStrategy;
  friend class FileStreamBuf;
  friend class LogFileImpl;
};


//
// inlines
//

FUN_ALWAYS_INLINE const String& FileImpl::GetPathImpl() const {
  return path_;
}

} // namespace fun
