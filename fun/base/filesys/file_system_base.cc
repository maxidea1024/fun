#include "fun/base/filesys/file_system_base.h"

namespace fun {

int64 IFile::Size() {
  const int64 original_pos = Tell();
  SeekFromEnd();
  const int64 result = Tell();
  Seek(original_pos);
  return result;
}

const char* IFileSystem::GetPhysicalTypeName() {
  return "PhysicalFileSystem";
}

void IFileSystem::GetTimestampPair(const char* path_a, const char* path_b, DateTime& out_timestamp_a, DateTime& out_timestamp_b) {
  if (GetLowerLevel()) {
    GetLowerLevel()->GetTimestampPair(path_a, path_b, out_timestamp_a, out_timestamp_b);
  } else {
    out_timestamp_a = GetTimestamp(path_a);
    out_timestamp_b = GetTimestamp(path_b);
  }
}

bool IFileSystem::IterateDirectoryRecursively(const char* directory, DirectoryVisitor& visitor) {
  struct Recurse : public DirectoryVisitor {
    IFileSystem& platform_fs;
    DirectoryVisitor& visitor;

    Recurse(IFileSystem& platform_fs, DirectoryVisitor& visitor)
      : platform_fs(platform_fs), visitor(visitor) {}

    bool Visit(const char* filename_or_directory, bool is_directory) override {
      bool result = visitor.Visit(filename_or_directory, is_directory);
      if (result && is_directory) {
        result = platform_fs.IterateDirectory(filename_or_directory, *this);
      }
      return result;
    }
  };
  Recurse recurse(*this, visitor);
  return IterateDirectory(directory, recurse);
}


bool IFileSystem::IterateDirectoryStatRecursively(const char* directory, DirectoryStatVisitor& visitor) {
  struct StatRecurse : public DirectoryStatVisitor {
    IFileSystem& platform_fs;
    DirectoryStatVisitor& visitor;

    StatRecurse(IFileSystem& platform_fs, DirectoryStatVisitor& visitor)
      : platform_fs(platform_fs), visitor(visitor) {}

    bool Visit(const char* filename_or_directory, const FileStatData& stat_data) override {
      bool result = visitor.Visit(filename_or_directory, stat_data);
      if (result && stat_data.is_directory) {
        result = platform_fs.IterateDirectoryStat(filename_or_directory, *this);
      }
      return result;
    }
  };
  StatRecurse recurse(*this, visitor);
  return IterateDirectoryStat(directory, recurse);
}


bool IFileSystem::DeleteDirectoryRecursively(const char* directory) {
  struct Recurse : public DirectoryVisitor {
    IFileSystem& platform_fs;

    Recurse(IFileSystem& platform_fs)
      : platform_fs(platform_fs) {}

    bool Visit(const char* filename_or_directory, bool is_directory) override {
      if (is_directory) {
        platform_fs.IterateDirectory(filename_or_directory, *this);
        platform_fs.DeleteDirectory(filename_or_directory);
      } else {
        platform_fs.SetReadOnly(filename_or_directory, false);
        platform_fs.DeleteFile(filename_or_directory);
      }
      return true; // continue searching
    }
  };
  Recurse recurse(*this);
  recurse.Visit(directory, true);
  return !DirectoryExists(directory);
}

bool IFileSystem::CopyFile(const char* to, const char* from) {
  const int64 MAX_BUFFER_SIZE = 1024*1024;

  AutoPtr<IFile> from_file(OpenRead(from));
  if (!from_file.IsValid()) {
    return false;
  }

  AutoPtr<IFile> to_file(OpenWrite(to));
  if (!to_file.IsValid()) {
    return false;
  }

  int64 size = from_file->Size();
  if (size < 1) {
    fun_check(size == 0);
    return true;
  }

  const int64 alloc_size = MathBase::Min<int64>(MAX_BUFFER_SIZE, Size);
  fun_check(alloc_size);
  uint8* buffer = (uint8*)UnsafeMemory::Malloc(int32(alloc_size));
  fun_check_ptr(buffer);
  while (size) {
    const int64 this_size = MathBase::Min<int64>(alloc_size, Size);
    from_file->Read(buffer, this_size);
    to_file->Write(buffer, this_size);
    size -= this_size;
    fun_check(size >= 0);
  }
  UnsafeMemory::Free(buffer);
  return true;
}

bool IFileSystem::CopyDirectoryTree(const char* destination_directory, const char* src, bool overwwrite_all_existing) {
  fun_check_ptr(destination_directory);
  fun_check_ptr(src);

  string dest_dir(destination_directory);
  CPaths::NormalizeDirectoryName(dest_dir);

  string source_dir(src);
  CPaths::NormalizeDirectoryName(source_dir);

  // Does src dir exist?
  if (!DirectoryExists(*source_dir)) {
    return false;
  }

  // dst directory exists already or can be created ?
  if (!DirectoryExists(*dest_dir) &&
      !CreateDirectory(*dest_dir)) {
    return false;
  }

  // Copy all files and directories
  struct CopyFilesAndDirs : public DirectoryVisitor {
    IFileSystem & platform_fs;
    const char* source_root;
    const char* dest_root;
    bool overwrite;

    CopyFilesAndDirs(IFileSystem& platform_fs, const char* source_root, const char* dest_root, bool overwrite)
      : platform_fs(platform_fs),
        source_root(source_root),
        dest_root(dest_root),
        overwrite(overwrite) {}

    bool Visit(const char* filename_or_directory, bool is_directory) override {
      string new_name(filename_or_directory);
      // change the root
      new_name.Replace(source_root, dest_root);

      if (is_directory) {
        // create new directory structure
        if (!platform_fs.CreateDirectoryTree(*new_name) && !platform_fs.DirectoryExists(*new_name)) {
          return false;
        }
      } else {
        // Delete destination file if it exists and we are overwriting
        if (platform_fs.FileExists(*new_name) && overwrite) {
          platform_fs.DeleteFile(*new_name);
        }

        // Copy file from src
        if (!platform_fs.CopyFile(*new_name, filename_or_directory)) {
          // Not all files could be copied
          return false;
        }
      }
      return true; // continue searching
    }
  };

  // copy files and directories visitor
  CopyFilesAndDirs copy_files_and_dirs(*this, *source_dir, *dest_dir, overwwrite_all_existing);

  // create all files subdirectories and files in subdirectories!
  return IterateDirectoryRecursively(*source_dir, copy_files_and_dirs);
}

String IFileSystem::ConvertToAbsolutePathForExternalAppForRead(const char* filename) {
  return CPaths::ConvertRelativePathToFull(filename);
}

String IFileSystem::ConvertToAbsolutePathForExternalAppForWrite(const char* filename) {
  return CPaths::ConvertRelativePathToFull(filename);
}

bool IFileSystem::CreateDirectoryTree(const char* directory) {
  string local_filename(directory);
  CPaths::NormalizeDirectoryName(local_filename);
  const char* local_path = *local_filename;
  int32 create_count = 0;
  const int32 max_characters = MAX_FUN_FILENAME_LENGTH - 1;
  int32 index = 0;
  for (char full[max_characters + 1] = "", *ptr = full; index < max_characters; *ptr++ = *local_path++, index++) {
    if (*local_path == '/' || *local_path == 0) {
      *ptr = 0;
      if (ptr != full && !CPaths::IsDrive(full)) {
        if (!CreateDirectory(full) && !DirectoryExists(full)) {
          break;
        }
        create_count++;
      }
    }
    if (*local_path == 0) {
      break;
    }
  }
  return DirectoryExists(*local_filename);
}

bool IPhysicalFileSystem::Initialize(IFileSystem* inner, const char* cmdline) {
  // Physical platform file should never wrap anything.
  fun_check(inner == nullptr);
  return true;
}

} // namespace fun
