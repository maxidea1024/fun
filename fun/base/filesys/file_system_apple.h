#pragma once

#include "fun/base/base.h"
#include "fun/base/filesys/file_system_base.h"

namespace fun {

/**
 * Mac File I/O implementation
 */
class FUN_BASE_API AppleFileSystem : public IPhysicalFileSystem {
 public:
  bool FileExists(const char* filename) override;
  int64 FileSize(const char* filename) override;
  bool DeleteFile(const char* filename) override;
  bool IsReadOnly(const char* filename) override;
  bool MoveFile(const char* to, const char* from) override;
  bool SetReadOnly(const char* filename, bool readonly) override;

  DateTime GetTimestamp(const char* filename) override;

  void SetTimestamp(const char* filename, const DateTime& timestamp) override;

  DateTime GetAccessTimestamp(const char* filename) override;
  String GetFilenameOnDisk(const char* filename) override;

  IFile* OpenRead(const char* filename, bool allow_write = false) override;
  IFile* OpenWrite(const char* filename, bool append = false,
                   bool allow_read = false) override;
  bool DirectoryExists(const char* directory) override;
  bool CreateDirectory(const char* directory) override;
  bool DeleteDirectory(const char* directory) override;

  FileStatData GetStatData(const char* filename_or_directory) override;

  bool IterateDirectory(const char* directory,
                        DirectoryVisitor& visitor) override;
  bool IterateDirectoryStat(const char* directory,
                            DirectoryStatVisitor& visitor) override;

  bool CopyFile(const char* to, const char* from) override;

 private:
  bool IterateDirectoryCommon(const char* directory,
                              const FunctionRef<bool(struct dirent*)>& visitor);

  int32 stat(const char* filename, struct stat* out_file_info);

 protected:
  virtual String NormalizeFilename(const char* filename);
  virtual String NormalizeDirectory(const char* directory);
};

}  // namespace fun
