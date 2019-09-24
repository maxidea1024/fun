#include "fun/base/file.h"
#include "fun/base/path.h"
#include "fun/base/directory_iterator.h"
#include "fun/base/exception.h"
#include "fun/base/string/string.h"
#include "fun/base/str.h"

#if FUN_PLATFORM_WINDOWS_FAMILY
#if defined(_WIN32_WCE)
#include "fun/base/file_wince.cc"
#else
#include "fun/base/file_win32.cc"
#endif
#elif FUN_PLATFORM != FUN_PLATFORM_VXWORKS
#include "fun/base/file_vx.cc"
#elif FUN_PLATFORM_UNIX_FAMILY
#include "fun/base/file_unix.cc"
#endif

#include "fun/base/thread.h" // Thread::Sleep()

namespace fun {

File::File() {}

File::File(const String& path)
  : FileImpl(path) {
  fun_check(CStringTraitsA::Strlen(path.ConstData()) == path.Len());
}

File::File(const char* path)
  : FileImpl(String(path)) {
}

File::File(const Path& path)
  : FileImpl(path.ToString()) {
}

File::File(const File& file)
  : FileImpl(file.GetPathImpl()) {
}

File::~File() {
}

File& File::operator = (const File& file) {
  SetPathImpl(file.GetPathImpl());
  return *this;
}

File& File::operator = (const String& path) {
  fun_check(CStringTraitsA::Strlen(path.ConstData()) == path.Len());
  SetPathImpl(path);
  return *this;
}

File& File::operator = (const char* path) {
  fun_check_ptr(path);
  SetPathImpl(path);
  return *this;
}

File& File::operator = (const Path& path) {
  SetPathImpl(path.ToString());
  return *this;
}

void File::Swap(File& file) {
  SwapImpl(file);
}

bool File::Exists() const {
  return ExistsImpl();
}

bool File::CanRead() const {
  return CanReadImpl();
}

bool File::CanWrite() const {
  return CanWriteImpl();
}

bool File::CanExecute() const {
  return CanExecuteImpl();
}

bool File::IsFile() const {
  return IsFileImpl();
}

bool File::IsDirectory() const {
  return IsDirectoryImpl();
}

bool File::IsLink() const {
  return IsLinkImpl();
}

bool File::IsDevice() const {
  return IsDeviceImpl();
}

bool File::IsHidden() const {
  return IsHiddenImpl();
}

Timestamp File::GetCreated() const {
  return GetCreatedImpl();
}

Timestamp File::GetLastModified() const {
  return GetLastModifiedImpl();
}

File& File::SetLastModified(const Timestamp& ts) {
  SetLastModifiedImpl(ts);
  return *this;
}

File::FileSize File::GetSize() const {
  return GetSizeImpl();
}

File& File::SetSize(FileSizeImpl size) {
  SetSizeImpl(size);
  return *this;
}

File& File::SetWritable(bool flag) {
  SetWriteableImpl(flag);
  return *this;
}

File& File::SetReadOnly(bool flag) {
  SetWriteableImpl(!flag);
  return *this;
}

File& File::SetExecutable(bool flag) {
  SetExecutableImpl(flag);
  return *this;
}

void File::CopyTo(const String& path) const {
  fun_check(CStringTraitsA::Strlen(path.ConstData()) == path.Len());

  Path src(GetPathImpl());
  Path dst(path);
  File dest_file(path);
  if ((dest_file.Exists() && dest_file.IsDirectory()) || dst.IsDirectory()) {
    dst.MakeDirectory();
    dst.SetFileName(src.GetFileName());
  }

  if (IsDirectory()) {
    CopyDirectory(dst.ToString());
  } else {
    CopyToImpl(dst.ToString());
  }
}

void File::CopyDirectory(const String& path) const {
  fun_check(CStringTraitsA::Strlen(path.ConstData()) == path.Len());

  File target(path);
  target.CreateDirectories();

  Path src(GetPathImpl());
  src.MakeFile();
  DirectoryIterator it(src);
  DirectoryIterator end;
  for (; it != end; ++it) {
    it->CopyTo(path);
  }
}

void File::MoveTo(const String& path) {
  fun_check(CStringTraitsA::Strlen(path.ConstData()) == path.Len());

  CopyTo(path);
  Remove(true);
  SetPathImpl(path);
}

void File::RenameTo(const String& path) {
  fun_check(CStringTraitsA::Strlen(path.ConstData()) == path.Len());

  RenameToImpl(path);
  SetPathImpl(path);
}

void File::LinkTo(const String& path, LinkType type) const {
  LinkToImpl(path, type);
}

void File::Remove(bool recursive) {
  if (recursive && !IsLink() && IsDirectory()) {
    Array<File> files;
    List(files);
    for (auto& file : files) {
      file.Remove(true);
    }

    // Note: On Windows, removing a directory may not succeed at first
    // try because deleting files is not a synchronous operation. Files
    // are merely marked as deleted, and actually removed at a later time.
    //
    // An alternate strategy would be moving files to a different directory
    // first (on the same drive, but outside the deleted tree), and marking
    // them as hidden, before deleting them, but this could lead to other issues.
    // So we simply retry after some time until we succeed, or give up.

    int retry = 8;
    long sleep = 10;
    while (retry > 0) {
      try {
        RemoveImpl();
        retry = 0;
      } catch (DirectoryNotEmptyException&) {
        if (--retry == 0) {
          throw;
        }
        Thread::Sleep(sleep);
        sleep *= 2;
      }
    }
  } else {
    RemoveImpl();
  }
}

bool File::CreateFile() {
  return CreateFileImpl();
}

bool File::CreateDirectory() {
  return CreateDirectoryImpl();
}

void File::CreateDirectories() {
  if (!Exists()) {
    Path p(GetPathImpl());
    p.MakeDirectory();
    if (p.GetDepth() > 1) {
      p.MakeParent();
      File f(p);
      f.CreateDirectories();
    }
    try {
      CreateDirectoryImpl();
    } catch (FileExistsException&) {
    }
  }
}

void File::List(Array<String>& files) const {
  files.Clear();
  DirectoryIterator it(*this);
  DirectoryIterator end;
  while (it != end) {
    files.Add(it.GetName());
    ++it;
  }
}

File::FileSize File::TotalSpace() const {
  return TotalSpaceImpl();
}

File::FileSize File::UsableSpace() const {
  return UsableSpaceImpl();
}

File::FileSize File::FreeSpace() const {
  return FreeSpaceImpl();
}

void File::List(Array<File>& files) const {
  files.Clear();
  DirectoryIterator it(*this);
  DirectoryIterator end;
  while (it != end) {
    files.Add(*it);
    ++it;
  }
}

void File::HandleLastError(const String& path) {
  HandleLastErrorImpl(path);
}

} // namespace fun
