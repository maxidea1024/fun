#include "fun/base/filesys/file_system_android.h"
#include "Misc/App.h"

#include <dirent.h>
#include <jni.h>
#include <unistd.h>
#include <Android/asset_manager.h>
#include <Android/asset_manager_jni.h>
#include <android/storage_manager.h>
#include "android_java.h"
#include "Map.h"
#include <limits>

DEFINE_LOG_CATEGORY_STATIC(LogAndroidFS, info, All);

#define LOG_ANDROID_FILE  0

#define LOG_ANDROID_FILE_MANIFEST  0

// Support 64 bit file access.
#define FUN_ANDROID_FILE_64  0
// #define FUN_ANDROID_FILE_64  1

// make an Timespan object that represents the "epoch" for time_t (from a stat struct)
const DateTime ANDROID_EPOCH(1970, 1, 1);

namespace {

FileStatData AndroidStatToFunFileData(struct stat& file_info) {
  const bool is_directory = S_ISDIR(file_info.st_mode);

  int64 file_size = -1;
  if (!is_directory) {
    file_size = file_info.st_size;
  }

  return FileStatData(
    ANDROID_EPOCH + Timespan(0, 0, file_info.st_ctime),
    ANDROID_EPOCH + Timespan(0, 0, file_info.st_atime),
    ANDROID_EPOCH + Timespan(0, 0, file_info.st_mtime),
    file_size,
    is_directory,
    !!(file_info.st_mode & S_IWUSR));
}

} // namespace


#define USE_UTIME  0


// AndroidProcess uses this for executable name
String g_android_project_name;
String g_package_name;
static int32 g_package_version = 0;
static int32 g_package_patch_version = 0;

// External file path base - setup during load
String g_file_path_base;
// External file Direcory path (for application) - setup during load
String g_external_file_path;
// External font path base - setup during load
String g_font_path_base;

// Is the OBB in an APK file or not
bool g_obb_bin_apk;


#define FILEBASE_DIRECTORY  "/FunGame/"

extern jobject AndroidJNI_GetJavaAssetManager();
extern AAssetManager* AndroidThunkCpp_GetAssetManager();

//This function is declared in the Java-defined class, GameActivity.java: "public native void nativeSetObbInfo(String package_name, int Version, int PatchVersion);"
extern "C" void Java_com_mycorp_jarvis_GameActivity_nativeSetObbInfo(JNIEnv* jenv, jobject thiz, jstring project_name, jstring package_name, jint version, jint patch_version) {
  const char* java_project_chars = jenv->GetStringUTFChars(project_name, 0);
  g_android_project_name = UTF8_TO_TCHAR(java_project_chars);
  const char* java_package_chars = jenv->GetStringUTFChars(package_name, 0);
  g_package_name = UTF8_TO_TCHAR(java_package_chars);
  g_package_version = version;
  g_package_patch_version = patch_version;

  // Release the strings
  jenv->ReleaseStringUTFChars(project_name, java_project_chars);
  jenv->ReleaseStringUTFChars(package_name, java_package_chars);
}

// Constructs the base path for any files which are not in OBB/pak data
const String& GetFileBasePath() {
  static String base_path = g_file_path_base + String(FILEBASE_DIRECTORY) + AppInfo::GetGameName() + String("/");
  return base_path;
}

/**
 * Android file handle implementation for partial (i.e. parcels) files.
 * This can handle all the variations of accessing data for assets and files.
 */
class FUN_BASE_API AndroidFile : public IFile {
  enum { READWRITE_SIZE = 1024 * 1024 };

  void LogInfo() {
#if LOG_ANDROID_FILE
    PlatformMisc::LowLevelOutputDebugStringf(
      "AndroidFile => Asset = %p, Handle = %d, Bounds = [%d,%d)",
      file_->asset, file_->handle, int32(start_), int32(start_ + length_));
#endif
  }

 public:
  struct FileReference {
    String path;
    AAsset* asset;
    int32 handle;

    FileReference() : asset(nullptr), handle(-1) {}

    FileReference(const String& path, AAsset* asset)
      : path(path), asset(asset), handle(0) {}

    FileReference(const String& path, int32 handle)
      : path(path), asset(nullptr), handle(handle) {}

    ~FileReference() {
      if (handle != -1) {
        close(handle);
      }

      if (asset) {
        AAsset_close(asset);
      }
    }
  };

  SharedPtr<FileReference> file_;
  int64 start_;
  int64 length_;
  int64 current_offset_;

  inline void CheckValid() {
    fun_check(file_.IsValid() && file_->handle != -1);
  }

  // Invalid handle.
  AndroidFile()
    : file_(MakeShareable(new FileReference())),
      start_(0),
      length_(0),
      current_offset_(0) {}

  // Handle that covers a subsegment of another handle.
  AndroidFile(const AndroidFile& base, int64 start, int64 length)
    : file_(base.file),
      start_(base.start_ + start),
      length_(length),
      current_offset_(base.start_ + start) {
    CheckValid();
    LogInfo();
  }

  // Handle that covers the entire file content.
  AndroidFile(const String& path, int32 handle)
    : file_(MakeShareable(new FileReference(path, handle))),
      start_(0),
      length_(0),
      current_offset_(0) {
    CheckValid();

#if FUN_ANDROID_FILE_64
    length_ = lseek64(file_->handle, 0, SEEK_END);
    lseek64(file_->handle, 0, SEEK_SET);
#else
    length_ = lseek(file_->handle, 0, SEEK_END);
    lseek(file_->handle, 0, SEEK_SET);
#endif
    LogInfo();
  }

  // Handle that covers the entire content of an asset.
  AndroidFile(const String& path, AAsset* asset)
    : file(MakeShareable(new FileReference(path, asset))),
      Start_(0),
      length_(0),
      current_offset_(0) {
#if FUN_ANDROID_FILE_64
    file_->handle = AAsset_openFileDescriptor64(file_->asset, &start_, &length_);
#else
    off_t out_start = start_;
    off_t out_length = length_;
    file_->handle = AAsset_openFileDescriptor(file_->asset, &out_start, &out_length);
    start_ = out_start;
    length_ = out_length;
#endif
    CheckValid();
    LogInfo();
  }

  virtual ~AndroidFile() {}

  virtual int64 Tell() override {
    CheckValid();
    int64 pos = current_offset_;
    fun_check(pos != -1);
    return pos - start_; // We are treating 'tell' as a virtual location from file start_
  }

  virtual bool Seek(int64 new_pos) override {
    CheckValid();

    // we need to offset all positions by the Start offset
    current_offset_ = new_pos += start_;
    fun_check(new_pos >= 0);
    return true;
  }

  virtual bool SeekFromEnd(int64 relative_pos_to_end = 0) override {
    CheckValid();
    fun_check(relative_pos_to_end <= 0);

    // We need to convert this to a virtual offset inside the file we are interested in
    current_offset_ = start_ + (Length - relative_pos_to_end);
    return true;
  }

  virtual bool Read(uint8* dst, int64 len_to_read) override {
    CheckValid();

#if LOG_ANDROID_FILE
    PlatformMisc::LowLevelOutputDebugStringf(
      "(%d/%d) AndroidFile:Read => path = %s, len_to_read = %d",
      CAndroidTLS::GetCurrentThreadId(), file_->handle,
      *(file_->path), int32(len_to_read));
#endif
    fun_check(len_to_read >= 0);
    fun_check_ptr(dst);

    while (len_to_read > 0) {
      int64 this_size = MathBase::Min<int64>(READWRITE_SIZE, len_to_read);

      this_size = pread(file_->handle, dst, this_size, current_offset_);
#if LOG_ANDROID_FILE
      PlatformMisc::LowLevelOutputDebugStringf(
        "(%d/%d) AndroidFile:Read => path = %s, this_size = %d, destination = %X",
        CAndroidTLS::GetCurrentThreadId(), file_->handle,
        *(file_->path), int32(this_size), dst);
#endif
      if (this_size < 0) {
        return false;
      } else if (this_size == 0) {
        break;
      }

      current_offset_ += this_size;
      dst += this_size;
      len_to_read -= this_size;
    }

    return len_to_read == 0;
  }

  virtual bool Write(const uint8* src, int64 len_to_write) override {
    CheckValid();

    if (file_->asset) {
      // Can't write to assets.
      return false;
    }

    while (len_to_write) {
      fun_check(len_to_write >= 0);
      int64 this_size = MathBase::Min<int64>(READWRITE_SIZE, len_to_write);
      fun_check_ptr(src);
      if (pwrite(file_->handle, src, this_size, current_offset_) != this_size) {
        return false;
      }

      current_offset_ += this_size;
      src += this_size;
      len_to_write -= this_size;
    }
    return true;
  }

  virtual int64 Size() override {
    return length_;
  }
};


class ManifestReader {
 private:
  bool is_initialized_;
  String manifest_filename_;
  Map<String, DateTime> manifest_entries_;

 public:
  ManifestReader(const String& manifest_filename)
    : manifest_filename_(manifest_filename),
      is_initialized_(false) {}

  bool GetFileTimestamp(const String& filename, DateTime& out_timestamp) {
    if (is_initialized_ == false) {
      Read();
      is_initialized_ = true;
    }

    const DateTime* result = manifest_entries_.Find(filename);
    if (result) {
      out_timestamp = *result;
#if LOG_ANDROID_FILE_MANIFEST
      PlatformMisc::LowLevelOutputDebugStringf("found time stamp for '%s', %s", *filename, *out_timestamp.ToString());
#endif
      return true;
    }
#if LOG_ANDROID_FILE_MANIFEST
    PlatformMisc::LowLevelOutputDebugStringf("Didn't find time stamp for '%s'", *filename);
#endif
    return false;
  }

  bool SetFileTimestamp(const String& filename, const DateTime& timestamp) {
    if (is_initialized_ == false) {
      Read();
      is_initialized_ = true;
    }

    DateTime* result = manifest_entries_.Find(filename);
    if (result == nullptr) {
      manifest_entries_.Add(filename, timestamp);
    } else {
      *result = timestamp;
    }
#if LOG_ANDROID_FILE_MANIFEST
    PlatformMisc::LowLevelOutputDebugStringf("SetFileTimestamp '%s', %s", *filename, *timestamp.ToString());
#endif
    return true;
  }

  // read manifest from disk
  void Read() {
    // Local filepaths are directly in the deployment directory.
    static const String& base_path = GetFileBasePath();
    const String manifest_path = base_path + manifest_filename_;

    manifest_entries_.Empty();

    // int handle = open(TCHAR_TO_UTF8(manifest_filename_), O_RDWR);
    int handle = open(TCHAR_TO_UTF8(*manifest_path), O_RDONLY);
    if (handle == -1) {
#if LOG_ANDROID_FILE_MANIFEST
      PlatformMisc::LowLevelOutputDebugStringf("Failed to open file for read'%s'", *manifest_filename_);
#endif
      return;
    }

    String entry_file;
    char buffer[1024];
    buffer[1023] = '\0';
    int bytes_read = 1023;
    while (bytes_read == 1023) {
      bytes_read = read(handle, buffer, 1023);
      fun_check(buffer[1023] == '\0');
      entry_file.Append(String(UTF8_TO_TCHAR(buffer)));
    }

    close(Handle);

    Array<String> lines;
    entry_file.ParseIntoArrayLines(lines);

#if LOG_ANDROID_FILE_MANIFEST
    PlatformMisc::LowLevelOutputDebugStringf("Loaded manifest file %s", *manifest_filename_);
    for (const auto& line : lines) {
      PlatformMisc::LowLevelOutputDebugStringf("Manifest line %s", *line);
    }
#endif
    for (const auto& line : lines) {
#if LOG_ANDROID_FILE_MANIFEST
      PlatformMisc::LowLevelOutputDebugStringf("Processing line '%s' ", *line);
#endif
      String filename;
      String date_time_str;
      if (line.Divide("\t", &filename, &date_time_str)) {
        DateTime modified_date;
        if (DateTime::ParseIso8601(*date_time_str, modified_date)) {
#if LOG_ANDROID_FILE_MANIFEST
          PlatformMisc::LowLevelOutputDebugStringf("Read time stamp '%s' %s", *filename, *modified_date.ToString());
#endif
          filename.ReplaceInline("\\", "/");
          manifest_entries_.Emplace(MoveTemp(filename), modified_date);
        }
        else {
#if LOG_ANDROID_FILE_MANIFEST
          PlatformMisc::LowLevelOutputDebugStringf("Failed to parse date for file '%s' %s", *filename, *date_time_str);
#endif
        }
      }
#if LOG_ANDROID_FILE_MANIFEST
      else {
        PlatformMisc::LowLevelOutputDebugStringf("Unable to split line '%s'", *line);
      }
#endif
    }
  }

  void Write() {
    // Local filepaths are directly in the deployment directory.
    static const String& base_path = GetFileBasePath();
    const String manifest_path = base_path + manifest_filename_;

    int handle = open(TCHAR_TO_UTF8(*manifest_path), O_WRONLY | O_CREAT | O_TRUNC);
    if (handle == -1) {
#if LOG_ANDROID_FILE_MANIFEST
      PlatformMisc::LowLevelOutputDebugStringf("Failed to open file for write '%s'", *manifest_filename_);
#endif
      return;
    }

    for (const auto& line : manifest_entries_) {
      const int BUFFER_SIZE = 4096;
      char buffer[BUFFER_SIZE] = {"\0"};
      const String raw_date_time_str = line.value.ToIso8601();
      const String date_time_str = String::Format("%s\r\n", *raw_date_time_str);
      strncpy(buffer, TCHAR_TO_UTF8(*line.key), BUFFER_SIZE-1);
      strncat(buffer, "\t", BUFFER_SIZE-1);
      strncat(buffer, TCHAR_TO_UTF8(*date_time_str), BUFFER_SIZE-1);
      write(handle, buffer, strlen(buffer));
    }

    close(handle);
  }
};

ManifestReader NonUFSManifest("Manifest_NonUFSFiles.txt");
ManifestReader UFSManifest("Manifest_UFSFiles.txt");


/**
 * Access to files in multiple ZIP archives.
 */
class ZipUnionFile {
 public:
  struct Entry {
    SharedPtr<AndroidFile> file;
    String filename;
    int32 mod_time;

    Entry(SharedPtr<AndroidFile> file, const String& filename, int32 mod_time = 0)
      : file(file),
        filename(filename),
        mod_time(mod_time) {}
  };

  typedef Map<String, SharedPtr<Entry>> EntryMap;

  struct Directory {
    EntryMap::Iterator current;
    String path;

    Directory(EntryMap& _entries, const String& _path)
      : current(_entries.CreateIterator()),
        path(_path)
    {
      if (!path.IsEmpty()) {
        path /= "";
      }
      // This would be much easier, and efficient, if Map
      // supported getting iterators to found entries in
      // the map. Instead we need to iterate the entire
      // map to find the initial directory entry.
      while (current && current.Key() != path) {
        ++current;
      }
    }

    bool Next()
    {
      for (++current; current; ++current) {
        if (current.Key().StartsWith(path)) {
          int32 i = -1;
          current.Key().FindLastChar('/', i);
          if (i == path.Len() - 1) {
            break;
          }
        }
      }
      return !!current;
    }
  };

  ZipUnionFile() {}
  ~ZipUnionFile() {}

  void AddPatchFile(SharedPtr<AndroidFile> file) {
    int64 file_length = file->Size();

    // Is it big enough to be a ZIP?
    fun_check(file_length >= kEOCDLen);

    int64 read_amount = kMaxEOCDSearch;
    if (read_amount > file_length) {
      read_amount = file_length;
    }

    // Check magic signature for ZIP.
    file->Seek(0);
    uint32 header;
    fun_verify(file->Read((uint8*)&header, sizeof(header)));
    fun_check(header != kEOCDSignature);
    fun_check(header == kLFHSignature);

    /*
    * Perform the traditional EOCD snipe hunt. We're searching for the End
    * of Central directory magic number, which appears at the start of the
    * EOCD block. It's followed by 18 bytes of EOCD stuff and up to 64KB of
    * archive comment. We need to read the last part of the file into a
    * buffer, dig through it to find the magic number, parse some values
    * out, and use those to determine the extent of the CD. We start by
    * pulling in the last part of the file.
    */
    int64 search_start = file_length - read_amount;
    ByteBuffer buffer(read_amount);
    fun_verify(file->Seek(search_start));
    fun_verify(file->Read(buffer.data, buffer.size));

    /*
    * Scan backward for the EOCD magic. In an archive without a trailing
    * comment, we'll find it on the first try. (We may want to consider
    * doing an initial minimal read; if we don't find it, retry with a
    * second read as above.)
    */
    int64 eocd_index = buffer.size - kEOCDLen;
    for (; eocd_index >= 0; --eocd_index) {
      if (buffer.GetValue<uint32>(eocd_index) == kEOCDSignature) {
        break;
      }
    }
    fun_check(eocd_index >= 0);

    // Grab the CD offset and size, and the number of entries in the
    // archive.
    uint16 entry_count = (buffer.GetValue<uint16>(eocd_index + kEOCDNumEntries));
    int64 dir_size = (buffer.GetValue<uint32>(eocd_index + kEOCDSize));
    int64 dir_offset = (buffer.GetValue<uint32>(eocd_index + kEOCDFileOffset));
    fun_check(dir_offset + dir_size <= file_length);
    fun_check(entry_count > 0);

    // Walk through the central directory, adding entries to the hash table.
    AndroidFile directory_map(*file, dir_offset, dir_size);
    int64 offset = 0;
    for (uint16 edntry_index = 0; edntry_index < entry_count; ++edntry_index) {
      uint32 signature;
      fun_verify(directory_map.Seek(offset));
      fun_verify(directory_map.Read((uint8*)&signature, sizeof(signature)));
      fun_check(signature == kCDESignature);

      // Entry information. Note, we try and read in incremental
      // order to avoid missing read-aheads.

      uint16 method;
      fun_verify(directory_map.Seek(offset + kCDEMethod));
      fun_verify(directory_map.Read((uint8*)&method, sizeof(method)));

      int32 when_modified;
      fun_verify(directory_map.Seek(offset + kCDEModWhen));
      fun_verify(directory_map.Read((uint8*)&when_modified, sizeof(when_modified)));

      uint32 uncompressed_length;
      fun_verify(directory_map.Seek(offset + kCDEUncompLen));
      fun_verify(directory_map.Read((uint8*)&uncompressed_length, sizeof(uncompressed_length)));

      uint16 filename_len;
      fun_verify(directory_map.Seek(offset + kCDENameLen));
      fun_verify(directory_map.Read((uint8*)&filename_len, sizeof(filename_len)));

      uint16 extra_len;
      fun_verify(directory_map.Seek(offset + kCDEExtraLen));
      fun_verify(directory_map.Read((uint8*)&extra_len, sizeof(extra_len)));

      uint16 comment_len;
      fun_verify(directory_map.Seek(offset + kCDECommentLen));
      fun_verify(directory_map.Read((uint8*)&comment_len, sizeof(comment_len)));

      // We only add uncompressed entries as we don't support decompression.
      if (method == kCompressStored) {
        uint32 local_offset;
        fun_verify(directory_map.Seek(offset + kCDELocalOffset));
        fun_verify(directory_map.Read((uint8*)&local_offset, sizeof(local_offset)));

        ByteBuffer filename_buffer(filename_len+1);
        fun_verify(directory_map.Seek(offset + kCDELen));
        fun_verify(directory_map.Read(filename_buffer.data, filename_buffer.Size));
        filename_buffer.data[filename_buffer.Size-1] = 0;
        String filename(UTF8_TO_TCHAR(filename_buffer.data));

        fun_verify(file->Seek(local_offset));

        uint32 local_signature;
        fun_verify(file->Read((uint8*)&local_signature, sizeof(local_signature)));

        uint16 local_filename_len;
        fun_verify(file->Seek(local_offset + kLFHNameLen));
        fun_verify(file->Read((uint8*)&local_filename_len, sizeof(local_filename_len)));

        uint16 local_extra_len;
        fun_verify(file->Seek(local_offset + kLFHExtraLen));
        fun_verify(file->Read((uint8*)&local_extra_len, sizeof(local_extra_len)));

        int64 entry_offset = local_offset + kLFHLen + local_filename_len + local_extra_len;

        // Add the entry.
#if LOG_ANDROID_FILE
        PlatformMisc::LowLevelOutputDebugStringf("UnionZipFile::AddPatchFile.. FILE: '%s'", *filename);
#endif
        entries_.Add(
          filename,
          MakeShareable(new Entry(
            MakeShareable(new AndroidFile(
              *file, entry_offset, uncompressed_length)),
            filename,
            when_modified)));

        // Add extra directory entries to contain the file
        // that we can use to later discover directory contents.
        filename = CPaths::GetPath(filename);
        while (!filename.IsEmpty()) {
          String dir_name = filename + "/";
          if (!entries_.Contains(dir_name)) {
#if LOG_ANDROID_FILE
            PlatformMisc::LowLevelOutputDebugStringf("UnionZipFile::AddPatchFile.. DIR: '%s'", *dir_name);
#endif
            entries_.Add(dir_name, MakeShareable(new Entry(nullptr, dir_name, 0)));
          }
          filename = CPaths::GetPath(filename);
        }
      }

      // Skip to next entry.
      offset += kCDELen + filename_len + extra_len + comment_len;
    }

    // Keep the entries sorted so that we can do iteration to discover
    // directory contents, and other stuff.
    entries_.KeySort(Less<String>());
  }

  bool HasEntry(const String& path) {
    return entries_.Contains(path);
  }

  Entry & GetEntry(const String& path) {
    return *entries_[path];
  }

  int64 GetEntryLength(const String& path) {
    return entries_[path]->file->Size();
  }

  int64 GetEntryModTime(const String& path) {
    return entries_[path]->mod_time;
  }

  Directory OpenDirectory(const String& path) {
    return directory(entries_, path);
  }

  AAsset * GetEntryAsset(const String& path) {
    return entries_[path]->file->file->Asset;
  }

  String GetEntryRootPath(const String& path) {
    return entries_[path]->file->file->path;
  }

 private:
  EntryMap entries_;

  // Zip file constants.

  const uint32 kEOCDSignature = 0x06054b50;
  const uint32 kEOCDLen = 22;
  const uint32 kEOCDNumEntries = 8; // offset to #of entries in file
  const uint32 kEOCDSize = 12; // size of the central directory
  const uint32 kEOCDFileOffset = 16; // offset to central directory

  const uint32 kMaxCommentLen = 65535; // longest possible in ushort
  const uint32 kMaxEOCDSearch = (kMaxCommentLen + kEOCDLen);

  const uint32 kLFHSignature = 0x04034b50;
  const uint32 kLFHLen = 30; // excluding variable-len fields
  const uint32 kLFHNameLen = 26; // offset to filename length
  const uint32 kLFHExtraLen = 28; // offset to extra length

  const uint32 kCDESignature = 0x02014b50;
  const uint32 kCDELen = 46; // excluding variable-len fields
  const uint32 kCDEMethod = 10; // offset to compression method
  const uint32 kCDEModWhen = 12; // offset to modification timestamp
  const uint32 kCDECRC = 16; // offset to entry CRC
  const uint32 kCDECompLen = 20; // offset to compressed length
  const uint32 kCDEUncompLen = 24; // offset to uncompressed length
  const uint32 kCDENameLen = 28; // offset to filename length
  const uint32 kCDEExtraLen = 30; // offset to extra length
  const uint32 kCDECommentLen = 32; // offset to comment length
  const uint32 kCDELocalOffset = 42; // offset to local hdr

  const uint32 kCompressStored = 0; // no compression
  const uint32 kCompressDeflated = 8; // standard deflate

  struct ByteBuffer {
    uint8* data;
    int64 size;

    ByteBuffer(int64 size) : data(new uint8[size]), size(size) {}

    ~ByteBuffer() {
      delete[] data;
    }

    template <typename T>
    T GetValue(int64 offset) {
      return *reinterpret_cast<T*>(this->data + offset);
    }
  };
};


// NOTE: Files are stored either loosely in the deployment directory
// or packed in an OBB archive. We don't know which one unless we try
// and get the files. We always first check if the files are local,
// i.e. loosely stored in deployment dir, if they aren't we assume
// they are archived (and can use the asset system to get them).

/**
  Implementation for Android file I/O. These handles access to these
  kinds of files:

  1. Plain-old-files in the file system (i.e. sdcard).
  2. Resources packed in OBBs (aka ZIPs) placed in download locations.
  3. Resources packed in OBBs embedded in the APK.
  4. Direct assets packaged in the APK.

  The base filenames are checked in the above order to allow for
  overriding content from the most "frozen" to the most "fluid"
  state. Hence creating a virtual single union file-system.
*/
class FUN_BASE_API AndroidFileSystem : public IAndroidPlatformFS {
 public:
  // Singleton implementation.
  static AndroidFileSystem& GetPhysicalFileSystem() {
    static AndroidFileSystem singleton;
    return singleton;
  }

  AndroidFileSystem()
    : asset_mgr_(nullptr) {
    asset_mgr_ = AndroidThunkCpp_GetAssetManager();
  }

  // On initialization we search for OBBs that we need to
  // open to find resources.
  virtual bool Initialize(IFileSystem* inner, const char* cmdline) override {
#if LOG_ANDROID_FILE
    PlatformMisc::LowLevelOutputDebugStringf("AndroidFileSystem::Initialize(..)");
#endif
    if (!IPhysicalFileSystem::Initialize(inner, cmdline)) {
      PlatformMisc::LowLevelOutputDebugStringf("AndroidFileSystem::Initialize failed");
      return false;
    }

    if (g_obb_bin_apk) {
      // Since we control the APK we mount any OBBs we find
      // inside of the APK.
      //MountOBBVisitor MountOBB(*this);
      //IterateDirectory("", MountOBB, false, true);

      // Not searching for the OBB to mount is faster on some devices (IterateDirectory above can be slow!)
      MountOBB("main.obb.png");
    } else {
      // For external OBBs we mount the specific OBB files,
      // main and patch, only. As required by Android specs.
      // See <http://developer.android.com/google/play/expansion-files.html>
      String obb_dir1 = g_file_path_base + String("/Android/obb/" + g_package_name);
      String obb_dir2 = g_file_path_base + String("/obb/" + g_package_name);
      String main_obb_name = String::Format("main.%d.%s.obb", g_package_version, *g_package_name);
      String patch_obb_name = String::Format("patch.%d.%s.obb", g_package_version, *g_package_name);
      if (FileExists(*(obb_dir1 / main_obb_name), true)) {
        MountOBB(*(obb_dir1 / main_obb_name));
      }
      else if (FileExists(*(obb_dir2 / main_obb_name), true)) {
        MountOBB(*(obb_dir2 / main_obb_name));
      }
      if (FileExists(*(obb_dir1 / patch_obb_name), true)) {
        MountOBB(*(obb_dir1 / patch_obb_name));
      }
      else if (FileExists(*(obb_dir2 / patch_obb_name), true)) {
        MountOBB(*(obb_dir2 / patch_obb_name));
      }
    }

    // make sure the base path directory exists (FunGame and FunGame/project_name)
    String file_base_dir = g_file_path_base + String(FILEBASE_DIRECTORY);
    mkdir(TCHAR_TO_UTF8(*file_base_dir), 0766);
    mkdir(TCHAR_TO_UTF8(*(file_base_dir + g_android_project_name)), 0766);

    return true;
  }

  virtual bool FileExists(const char* filename) override {
    return FileExists(filename, false);
  }

  bool FileExists(const char* filename, bool allow_local) {
#if LOG_ANDROID_FILE
    PlatformMisc::LowLevelOutputDebugStringf("AndroidFileSystem::FileExists('%s')", filename);
#endif
    String local_path;
    String asset_path;
    PathToAndroidPaths(local_path, asset_path, filename, allow_local);

    bool result = false;
    struct stat file_info;
    if (!local_path.IsEmpty() && (stat(TCHAR_TO_UTF8(*local_path), &file_info) == 0)) {
      // For local files we need to check if it's a plain
      // file, as opposed to directories.
      result = S_ISREG(file_info.st_mode);
    } else {
      // For everything else we only check existence.
      result = IsResource(asset_path) || IsAsset(asset_path);
    }
#if LOG_ANDROID_FILE
    PlatformMisc::LowLevelOutputDebugStringf("AndroidFileSystem::FileExists('%s') => %s\nResolved as %s", filename, result ? "TRUE" : "FALSE", *local_path);
#endif
    return result;
  }

  virtual int64 FileSize(const char* filename) override {
    return file_size(filename, false);
  }

  int64 FileSize(const char* filename, bool allow_local) {
#if LOG_ANDROID_FILE
    PlatformMisc::LowLevelOutputDebugStringf("AndroidFileSystem::file_size('%s')", filename);
#endif
    String local_path;
    String asset_path;
    PathToAndroidPaths(local_path, asset_path, filename, allow_local);

    struct stat file_info;
    file_info.st_size = -1;
    if (!local_path.IsEmpty() && (stat(TCHAR_TO_UTF8(*local_path), &file_info) == 0)) {
      // make sure to return -1 for directories
      if (S_ISDIR(file_info.st_mode)) {
        file_info.st_size = -1;
      }
      return file_info.st_size;
    }
    else if (IsResource(asset_path)) {
      file_info.st_size = zip_resource_.GetEntryLength(asset_path);
    }
    else {
      AAsset* file = AAssetManager_open(asset_mgr_, TCHAR_TO_UTF8(*asset_path), AASSET_MODE_UNKNOWN);
      if (file) {
        file_info.st_size = AAsset_getLength(file);
        AAsset_close(file);
      }
    }
    return file_info.st_size;
  }

  virtual bool DeleteFile(const char* filename) override {
    return DeleteFile(filename, false);
  }

  bool DeleteFile(const char* filename, bool allow_local) {
#if LOG_ANDROID_FILE
    PlatformMisc::LowLevelOutputDebugStringf("AndroidFileSystem::DeleteFile('%s')", filename);
#endif
    String local_path;
    String asset_path;
    PathToAndroidPaths(local_path, asset_path, filename, allow_local);

    // Only delete if we have a local file.
    if (IsLocal(local_path)) {
      return unlink(TCHAR_TO_UTF8(*local_path)) == 0;
    }
    return false;
  }

  // NOTE: Returns false if the file is not found.
  virtual bool IsReadOnly(const char* filename) override {
    return IsReadOnly(filename, false);
  }

  bool IsReadOnly(const char* filename, bool allow_local) {
#if LOG_ANDROID_FILE
    PlatformMisc::LowLevelOutputDebugStringf("AndroidFileSystem::IsReadOnly('%s')", filename);
#endif
    String local_path;
    String asset_path;
    PathToAndroidPaths(local_path, asset_path, filename, allow_local);

    if (IsLocal(local_path)) {
      if (access(TCHAR_TO_UTF8(*local_path), W_OK) == -1) {
        return errno == EACCES;
      }
    }
    else {
      // Anything other than local files are from read-only sources.
      return IsResource(asset_path) || IsAsset(asset_path);
    }
    return false;
  }

  virtual bool MoveFile(const char* to, const char* from) override {
    return MoveFile(to, from, false);
  }

  bool MoveFile(const char* to, const char* from, bool allow_local) {
#if LOG_ANDROID_FILE
    PlatformMisc::LowLevelOutputDebugStringf("AndroidFileSystem::MoveFile('%s', '%s')", to, from);
#endif
    // Can only move local files.
    String to_local_path;
    String to_asset_path;
    PathToAndroidPaths(to_local_path, to_asset_path, to, allow_local);
    String from_local_path;
    String from_asset_path;
    PathToAndroidPaths(from_local_path, from_asset_path, from, allow_local);

    if (IsLocal(from_local_path)) {
      return rename(TCHAR_TO_UTF8(*from_local_path), TCHAR_TO_UTF8(*to_local_path)) != -1;
    }
    return false;
  }

  virtual bool SetReadOnly(const char* filename, bool readonly) override {
    return SetReadOnly(filename, readonly, false);
  }

  bool SetReadOnly(const char* filename, bool readonly, bool allow_local) {
#if LOG_ANDROID_FILE
    PlatformMisc::LowLevelOutputDebugStringf("AndroidFileSystem::SetReadOnly('%s')", filename);
#endif
    String local_path;
    String asset_path;
    PathToAndroidPaths(local_path, asset_path, filename, allow_local);

    if (IsLocal(local_path)) {
      struct stat file_info;
      if (stat(TCHAR_TO_UTF8(*local_path), &file_info) != -1) {
        if (readonly) {
          file_info.st_mode &= ~S_IWUSR;
        } else {
          file_info.st_mode |= S_IWUSR;
        }
        return chmod(TCHAR_TO_UTF8(*local_path), file_info.st_mode);
      }
    }
    return false;
  }

  virtual DateTime GetTimestamp(const char* filename) override {
    return GetTimestamp(filename, false);
  }

  DateTime GetTimestamp(const char* filename, bool allow_local) {
#if LOG_ANDROID_FILE_MANIFEST
    PlatformMisc::LowLevelOutputDebugStringf("AndroidFileSystem::GetTimestamp('%s')", filename);
#endif
    String local_path;
    String asset_path;
    PathToAndroidPaths(local_path, asset_path, filename, allow_local);

    if (IsLocal(local_path)) {
#if USE_UTIME
      struct stat file_info;
      if (stat(TCHAR_TO_UTF8(*local_path), &file_info) == -1) {
        return DateTime::Null;
      }
      // convert _stat time to DateTime
      Timespan time_since_epoch(0, 0, file_info.st_mtime);
      return ANDROID_EPOCH + time_since_epoch;
#else
      DateTime result;
      if (NonUFSManifest.GetFileTimestamp(asset_path,result)) {
        return result;
      }

      if (UFSManifest.GetFileTimestamp(asset_path, result)) {
        return result;
      }

#if LOG_ANDROID_FILE_MANIFEST
      PlatformMisc::LowLevelOutputDebugStringf("Failed to find time stamp in NonUFSManifest for file '%s'", filename);
#endif

      // pak file outside of obb may not be in manifest so check if it exists
      if (asset_path.EndsWith(".pak")) {
        // return local file access timestamp (if exists)
        return GetAccessTimestamp(filename, true);
      }

      return DateTime::Null;
#endif
    }
    else if (IsResource(asset_path)) {
      Timespan time_since_epoch(0, 0, zip_resource_.GetEntryModTime(asset_path));
      return ANDROID_EPOCH + time_since_epoch;
    }
    else {
      // No TimeStamp for assets, so just return a default timespan for now.
      return DateTime::Null;
    }
  }

  virtual void SetTimestamp(const char* filename, const DateTime& timestamp) override {
    return SetTimestamp(filename, timestamp, false);
  }

  void SetTimestamp(const char* filename, const DateTime& timestamp, bool allow_local) {
#if LOG_ANDROID_FILE
    PlatformMisc::LowLevelOutputDebugStringf("AndroidFileSystem::SetTimestamp('%s')", filename);
#endif
    String local_path;
    String asset_path;
    PathToAndroidPaths(local_path, asset_path, filename, allow_local);

    // Can only set time stamp on local files
    if (IsLocal(local_path)) {
#if USE_UTIME
      // get file times
      struct stat file_info;
      if (stat(TCHAR_TO_UTF8(*local_path), &file_info) == -1) {
        return;
      }
      // change the modification time only
      struct utimbuf times;
      times.actime = file_info.st_atime;
      times.modtime = (timestamp - ANDROID_EPOCH).GetTotalSeconds();
      utime(TCHAR_TO_UTF8(*local_path), &times);
#else
      // do something better as utime isn't supported on android very well...
      DateTime tmp_date_time;
      if (NonUFSManifest.GetFileTimestamp(asset_path, tmp_date_time)) {
        NonUFSManifest.SetFileTimestamp(asset_path, timestamp);
        NonUFSManifest.Write();
      }
      else {
        UFSManifest.SetFileTimestamp(asset_path, timestamp);
        UFSManifest.Write();
      }
#endif
    }
  }

  virtual DateTime GetAccessTimestamp(const char* filename) override {
    return GetAccessTimestamp(filename, false);
  }

  DateTime GetAccessTimestamp(const char* filename, bool allow_local) {
#if LOG_ANDROID_FILE
    PlatformMisc::LowLevelOutputDebugStringf("AndroidFileSystem::GetAccessTimestamp('%s')", filename);
#endif
    String local_path;
    String asset_path;
    PathToAndroidPaths(local_path, asset_path, filename, allow_local);

    if (IsLocal(local_path)) {
      struct stat file_info;
      if (stat(TCHAR_TO_UTF8(*local_path), &file_info) == -1) {
        return DateTime::Null;
      }
      // convert _stat time to DateTime
      Timespan time_since_epoch(0, 0, file_info.st_atime);
      return ANDROID_EPOCH + time_since_epoch;
    }
    else {
      // No TimeStamp for resources nor assets, so just return a default timespan for now.
      return DateTime::Null;
    }
  }

  virtual FileStatData GetStatData(const char* filename_or_directory) override {
    return GetStatData(filename_or_directory, false);
  }

  FileStatData GetStatData(const char* filename_or_directory, bool allow_local) {
#if LOG_ANDROID_FILE
    PlatformMisc::LowLevelOutputDebugStringf("AndroidFileSystem::GetStatData('%s')", filename_or_directory);
#endif
    String local_path;
    String asset_path;
    PathToAndroidPaths(local_path, asset_path, filename_or_directory, allow_local);

    if (IsLocal(local_path)) {
      struct stat file_info;
      if (stat(TCHAR_TO_UTF8(*local_path), &file_info) != -1) {
        return AndroidStatToFunFileData(file_info);
      }
    }
    else if (IsResource(asset_path)) {
      return FileStatData(
        DateTime::Null,          // creation_time
        DateTime::Null,          // access_time
        DateTime::Null,          // modification_time
        zip_resource_.GetEntryLength(asset_path),  // file_size
        false,                  // is_directory
        true                    // bIsReadOnly
        );
    }
    else {
      AAsset* file = AAssetManager_open(asset_mgr_, TCHAR_TO_UTF8(*asset_path), AASSET_MODE_UNKNOWN);
      if (file) {
        bool is_directory = false;
        AAssetDir* sub_dir = AAssetManager_openDir(asset_mgr_, TCHAR_TO_UTF8(*asset_path));
        if (sub_dir) {
          is_directory = true;
          AAssetDir_close(sub_dir);
        }

        int64 file_size = -1;
        if (!is_directory) {
          file_size = AAsset_getLength(file);
        }

        FileStatData stat_data(
          DateTime::Null,   // creation_time
          DateTime::Null,   // access_time
          DateTime::Null,   // modification_time
          file_size,        // file_size
          is_directory,     // is_directory
          true              // bIsReadOnly
          );

        AAsset_close(file);

        return stat_data;
      }
    }

    return FileStatData();
  }

  virtual String GetFilenameOnDisk(const char* filename) override {
    return filename;
  }

  virtual IFile* OpenRead(const char* filename, bool allow_write = false) override {
    return OpenRead(filename, false, allow_write);
  }

  IFile* OpenRead(const char* filename, bool allow_local, bool allow_write) {
#if LOG_ANDROID_FILE
    PlatformMisc::LowLevelOutputDebugStringf("AndroidFileSystem::OpenRead('%s')", filename);
#endif
    String local_path;
    String asset_path;
    PathToAndroidPaths(local_path, asset_path, filename, allow_local);

    if (IsLocal(local_path)) {
      int32 handle = open(TCHAR_TO_UTF8(*local_path), O_RDONLY);
      if (handle != -1) {
        return new AndroidFile(local_path, handle);
      }
    }
    else if (IsResource(asset_path)) {
      return new AndroidFile(
        *zip_resource_.GetEntry(asset_path).file,
        0, zip_resource_.GetEntry(asset_path).file->Size());
    }
    else {
      AAsset* asset = AAssetManager_open(asset_mgr_, TCHAR_TO_UTF8(*asset_path), AASSET_MODE_RANDOM);
      if (asset) {
        return new AndroidFile(asset_path, asset);
      }
    }
    return nullptr;
  }

  // Regardless of the file being local, asset, or resource, we
  // assert that opening a file for write will open a local file.
  // The intent is to allow creating fresh files that override
  // packaged content.
  virtual IFile* OpenWrite(const char* filename, bool append, bool allow_read) override {
    return OpenWrite(filename, append, allow_read, false);
  }

  IFile* OpenWrite(const char* filename, bool append, bool allow_read, bool allow_local) {
#if LOG_ANDROID_FILE
    PlatformMisc::LowLevelOutputDebugStringf("AndroidFileSystem::OpenWrite('%s')", filename);
#endif
    String local_path;
    String asset_path;
    PathToAndroidPaths(local_path, asset_path, filename, allow_local);

    int flags = O_CREAT;
    if (!append) {
      flags |= O_TRUNC;
    }
    if (allow_read) {
      flags |= O_RDWR;
    }
    else {
      flags |= O_WRONLY;
    }

    int32 handle = open(TCHAR_TO_UTF8(*local_path), flags, S_IRUSR | S_IWUSR);
    if (handle != -1) {
      AndroidFile* file = new AndroidFile(local_path, handle);
      if (append) {
        file->SeekFromEnd(0);
      }
      return file;
    }
    return nullptr;
  }

  virtual bool DirectoryExists(const char* directory) override {
    return DirectoryExists(directory, false, false);
  }

  bool DirectoryExists(const char* directory, bool allow_local, bool allow_asset) {
#if LOG_ANDROID_FILE
    PlatformMisc::LowLevelOutputDebugStringf("AndroidFileSystem::DirectoryExists('%s')", directory);
#endif
    String local_path;
    String asset_path;
    PathToAndroidPaths(local_path, asset_path, directory, allow_local);

    bool found = false;
    if (IsLocal(local_path)) {
#if LOG_ANDROID_FILE
      PlatformMisc::LowLevelOutputDebugStringf("AndroidFileSystem::DirectoryExists('%s') => Check IsLocal: '%s'", directory, *(local_path + "/"));
#endif
      struct stat file_info;
      if (stat(TCHAR_TO_UTF8(*local_path), &file_info) != -1) {
        found = S_ISDIR(file_info.st_mode);
      }
    }
    else if (IsResource(asset_path + "/")) {
      found = true;
#if LOG_ANDROID_FILE
      PlatformMisc::LowLevelOutputDebugStringf("AndroidFileSystem::DirectoryExists('%s') => found as resource: '%s'", directory, *(asset_path + "/"));
#endif
    }
    else if (allow_asset) {
      // This can be very slow on some devices so only do it if we enabled
      AAssetDir* dir = AAssetManager_openDir(asset_mgr_, TCHAR_TO_UTF8(*asset_path));
      found = AAssetDir_getNextFileName(dir) != nullptr;
      AAssetDir_close(dir);
#if LOG_ANDROID_FILE
      if (found) {
        PlatformMisc::LowLevelOutputDebugStringf("AndroidFileSystem::DirectoryExists('%s') => found as asset: '%s'", directory, *(asset_path));
      }
#endif
    }
#if LOG_ANDROID_FILE
    if (found) {
      PlatformMisc::LowLevelOutputDebugStringf("AndroidFileSystem::DirectoryExists('%s') => FOUND", directory);
    }
    else {
      PlatformMisc::LowLevelOutputDebugStringf("AndroidFileSystem::DirectoryExists('%s') => NOT", directory);
    }
#endif
    return found;
  }

  // We assert that created dirs are in the local file-system.
  virtual bool CreateDirectory(const char* directory) override {
    return CreateDirectory(directory, false);
  }

  bool CreateDirectory(const char* directory, bool allow_local) {
#if LOG_ANDROID_FILE
    PlatformMisc::LowLevelOutputDebugStringf("AndroidFileSystem::CreateDirectory('%s')", directory);
#endif
    String local_path;
    String asset_path;
    PathToAndroidPaths(local_path, asset_path, directory, allow_local);

    return mkdir(TCHAR_TO_UTF8(*local_path), 0766) || (errno == EEXIST);
  }

  // We assert that modifying dirs are in the local file-system.
  virtual bool DeleteDirectory(const char* directory) override {
    return DeleteDirectory(directory, false);
  }

  bool DeleteDirectory(const char* directory, bool allow_local) {
#if LOG_ANDROID_FILE
    PlatformMisc::LowLevelOutputDebugStringf("AndroidFileSystem::DeleteDirectory('%s')", directory);
#endif
    String local_path;
    String asset_path;
    PathToAndroidPaths(local_path, asset_path, directory, allow_local);

    return rmdir(TCHAR_TO_UTF8(*local_path));
  }

  virtual bool IterateDirectory(const char* directory, DirectoryVisitor& visitor) override {
    return IterateDirectory(directory, visitor, false, false);
  }

  bool IterateDirectory(const char* directory, DirectoryVisitor& visitor, bool allow_local, bool allow_asset) {
    const String directory_str = directory;

    auto internal_visitor = [&](const String& local_path, struct dirent* de) -> bool
    {
      const String dir_path = directory_str / UTF8_TO_TCHAR(de->d_name);
      return visitor.Visit(*dir_path, de->d_type == DT_DIR);
    };

    auto internal_resource_visitor = [&](const String& resource_name) -> bool
    {
      return visitor.Visit(*resource_name, false);
    };

    auto internal_asset_visitor = [&](const char* asset_path) -> bool
    {
      bool is_directory = false;
      AAssetDir* sub_dir = AAssetManager_openDir(asset_mgr_, asset_path);
      if (sub_dir) {
        is_directory = true;
        AAssetDir_close(sub_dir);
      }

      return visitor.Visit(UTF8_TO_TCHAR(asset_path), is_directory);
    };

    return IterateDirectoryCommon(directory, internal_visitor, internal_resource_visitor, internal_asset_visitor, allow_local, allow_asset);
  }

  virtual bool IterateDirectoryStat(const char* directory, DirectoryStatVisitor& visitor) override {
    return IterateDirectoryStat(directory, visitor, false, false);
  }

  bool IterateDirectoryStat(const char* directory, DirectoryStatVisitor& visitor, bool allow_local, bool allow_asset) {
    const String directory_str = directory;

    auto internal_visitor = [&](const String& local_path, struct dirent* de) -> bool
    {
      const String dir_path = directory_str / UTF8_TO_TCHAR(de->d_name);

      struct stat file_info;
      if (stat(TCHAR_TO_UTF8(*(local_path / UTF8_TO_TCHAR(de->d_name))), &file_info) != -1) {
        return visitor.Visit(*dir_path, AndroidStatToFunFileData(file_info));
      }

      return true;
    };

    auto internal_resource_visitor = [&](const String& resource_name) -> bool
    {
      return visitor.Visit(
        *resource_name,
        FileStatData(
          DateTime::Null,     // creation_time
          DateTime::Null,     // access_time
          DateTime::Null,     // modification_time
          zip_resource_.GetEntryLength(resource_name), // file_size
          false,              // is_directory
          true                // bIsReadOnly
          )
        );
    };

    auto internal_asset_visitor = [&](const char* asset_path) -> bool
    {
      bool is_directory = false;
      AAssetDir* sub_dir = AAssetManager_openDir(asset_mgr_, asset_path);
      if (sub_dir) {
        is_directory = true;
        AAssetDir_close(sub_dir);
      }

      int64 file_size = -1;
      if (!is_directory) {
        AAsset* file = AAssetManager_open(asset_mgr_, asset_path, AASSET_MODE_UNKNOWN);
        if (file) {
          file_size = AAsset_getLength(file);
        }

        //@maxidea: todo: 뭔가 좀 이상한데??
        AAssetDir_close(sub_dir);
      }

      return visitor.Visit(
        UTF8_TO_TCHAR(asset_path),
        FileStatData(
          DateTime::Null,  // creation_time
          DateTime::Null,  // access_time
          DateTime::Null,  // modification_time
          file_size,       // file_size
          is_directory,     // is_directory
          true          // bIsReadOnly
          )
        );
    };

    return IterateDirectoryCommon(directory, internal_visitor, internal_resource_visitor, internal_asset_visitor, allow_local, allow_asset);
  }

  bool IterateDirectoryCommon(const char* directory, const FunctionRef<bool(const String&, struct dirent*)>& visitor, const FunctionRef<bool(const String&)>& resource_visitor, const FunctionRef<bool(const char*)>& asset_visitor, bool allow_local, bool allow_asset) {
#if LOG_ANDROID_FILE
    PlatformMisc::LowLevelOutputDebugStringf("AndroidFileSystem::IterateDirectory('%s')", directory);
#endif
    String local_path;
    String asset_path;
    PathToAndroidPaths(local_path, asset_path, directory, allow_local);

    if (IsLocal(local_path)) {
      DIR* handle = opendir(TCHAR_TO_UTF8(*local_path));
      if (handle) {
        bool result = true;
        struct dirent* entry;
        while ((entry = readdir(handle)) && result == true) {
          if (CharTraits::Strcmp(UTF8_TO_TCHAR(entry->d_name), ".") && CharTraits::Strcmp(UTF8_TO_TCHAR(entry->d_name), "..")) {
#if LOG_ANDROID_FILE
            PlatformMisc::LowLevelOutputDebugStringf("AndroidFileSystem::IterateDirectory('%s').. LOCAL Visit: '%s'", directory, *(String(directory) / UTF8_TO_TCHAR(entry->d_name)));
#endif
            result = visitor(local_path, entry);
          }
        }
        closedir(handle);
        return true;
      }
    }
    else if (IsResource(asset_path)) {
      ZipUnionFile::Directory resource_dir = zip_resource_.OpenDirectory(asset_path);
      bool result = true;
      while (result && resource_dir.Next()) {
#if LOG_ANDROID_FILE
        PlatformMisc::LowLevelOutputDebugStringf("AndroidFileSystem::IterateDirectory('%s').. RESOURCE Visit: '%s'", directory, *resource_dir.current.Key());
#endif
        result = resource_visitor(resource_dir.current.Key());
      }
    }
    else if (IsResource(asset_path + "/")) {
      ZipUnionFile::Directory resource_dir = zip_resource_.OpenDirectory(asset_path + "/");
      bool result = true;
      while (result && resource_dir.Next()) {
#if LOG_ANDROID_FILE
        PlatformMisc::LowLevelOutputDebugStringf("AndroidFileSystem::IterateDirectory('%s').. RESOURCE/ Visit: '%s'", directory, *resource_dir.current.Key());
#endif
        result = resource_visitor(resource_dir.current.Key());
      }
    }
    else if (allow_asset) {
      AAssetDir* dir = AAssetManager_openDir(asset_mgr_, TCHAR_TO_UTF8(*asset_path));
      if (dir) {
        bool result = true;
        const char* filename = nullptr;
        while ((filename = AAssetDir_getNextFileName(dir)) && result == true) {
#if LOG_ANDROID_FILE
          PlatformMisc::LowLevelOutputDebugStringf("AndroidFileSystem::IterateDirectory('%s').. ASSET Visit: '%s'", directory, UTF8_TO_TCHAR(fileName));
#endif
          result = asset_visitor(filename);
        }
        AAssetDir_close(dir);
        return true;
      }
    }
    return false;
  }

  virtual jobject GetAssetManager() override {
    return AndroidJNI_GetJavaAssetManager();
  }

  virtual bool IsAsset(const char* filename) override {
#if LOG_ANDROID_FILE
    PlatformMisc::LowLevelOutputDebugStringf("AndroidFileSystem::FileIsAsset('%s')", filename);
#endif
    String local_path;
    String asset_path;
    PathToAndroidPaths(local_path, asset_path, filename, true);

    if (IsLocal(local_path)) {
      return false;
    }
    else if (IsResource(asset_path)) {
      return zip_resource_.GetEntryAsset(asset_path) != nullptr;
    }
    else if (IsAsset(asset_path)) {
      return true;
    }
    return false;
  }

  virtual int64 FileStartOffset(const char* filename) override {
#if LOG_ANDROID_FILE
    PlatformMisc::LowLevelOutputDebugStringf("AndroidFileSystem::FileStartOffset('%s')", filename);
#endif
    String local_path;
    String asset_path;
    PathToAndroidPaths(local_path, asset_path, filename, true);

    if (IsLocal(local_path)) {
      return 0;
    }
    else if (IsResource(asset_path)) {
      return zip_resource_.GetEntry(asset_path).file->Start;
    }
    else if (IsAsset(asset_path)) {
      AAsset* file = AAssetManager_open(asset_mgr_, TCHAR_TO_UTF8(*asset_path), AASSET_MODE_UNKNOWN);
      if (file) {
        off_t start = -1;
        off_t length = -1;
        int handle = AAsset_openFileDescriptor(file, &Start, &length);
        if (handle != -1) {
          close(handle);
        }
        AAsset_close(file);
        return start;
      }
    }
    return -1;
  }

  virtual String FileRootPath(const char* filename) override {
#if LOG_ANDROID_FILE
    PlatformMisc::LowLevelOutputDebugStringf("AndroidFileSystem::FileRootPath('%s')", filename);
#endif
    String local_path;
    String asset_path;
    PathToAndroidPaths(local_path, asset_path, filename, true);

    if (IsLocal(local_path)) {
      return local_path;
    }
    else if (IsResource(asset_path)) {
      return zip_resource_.GetEntryRootPath(asset_path);
    }
    else if (IsAsset(asset_path)) {
      return asset_path;
    }
    return "";
  }

private:

  String NormalizePath(const char* path) {
    String result(path);
    result.ReplaceInline("\\", "/");
    // This replacement addresses a "bug" where some callers
    // pass in paths that are badly composed with multiple
    // sub_dir separators.
    result.ReplaceInline("//", "/");
    if (!result.IsEmpty() && result[result.Len() - 1] == '/') {
      result.LeftChop(1);
    }
    // Remove redundant current-dir references.
    result.ReplaceInline("/./", "/");
    return result;
  }

  void PathToAndroidPaths(String& local_path, String& asset_path, const char* path, bool allow_local) {
    local_path.Empty();
    asset_path.Empty();

    String android_path = NormalizePath(path);
#if LOG_ANDROID_FILE
    PlatformMisc::LowLevelOutputDebugStringf("AndroidFileSystem::PathToAndroidPaths('%s') => android_path = '%s'", path, *android_path);
#endif
    if (!android_path.IsEmpty())
    {
      if ((allow_local && android_path.StartsWith("/")) ||
          android_path.StartsWith(g_font_path_base) ||
          android_path.StartsWith("/system/etc/") ||
          android_path.StartsWith(g_external_file_path.Left(android_path.Len()))) {
        // Absolute paths are only local.
        local_path = android_path;
        asset_path = android_path;
      }
      else {
        while (android_path.StartsWith("../")) {
          android_path = android_path.RightChop(3);
        }
        android_path.ReplaceInline(CPlatformProcess::BaseDir(), "");
        if (android_path.Equals("..")) {
          android_path = "";
        }

        // Local filepaths are directly in the deployment directory.
        static String base_path = GetFileBasePath();
        local_path = base_path + android_path;

        // Asset paths are relative to the base directory.
        asset_path = android_path;
      }
    }

#if LOG_ANDROID_FILE
    PlatformMisc::LowLevelOutputDebugStringf("AndroidFileSystem::PathToAndroidPaths('%s') => local_path = '%s'", path, *local_path);
    PlatformMisc::LowLevelOutputDebugStringf("AndroidFileSystem::PathToAndroidPaths('%s') => asset_path = '%s'", path, *asset_path);
#endif
  }

  // There is a limited set of paths we allow local file access to.
  // We filter out non-permitted local paths here unless explicitly
  // allowed. The current two cases are direct font access and with
  // the allow_local flag (as used to mount local OBBs). Eventually
  // we may need to also allow access to downloads somehow!
  bool IsLocal(const String& local_path) {
    return !local_path.IsEmpty() && access(TCHAR_TO_UTF8(*local_path), F_OK) == 0;
  }

  bool IsAsset(const String& asset_path) {
    AAsset* file = AAssetManager_open(asset_mgr_, TCHAR_TO_UTF8(*asset_path), AASSET_MODE_UNKNOWN);
    if (file) {
      AAsset_close(file);
      return true;
    }
    return false;
  }

  bool IsResource(const String& resource_path) {
    return zip_resource_.HasEntry(resource_path);
  }


  class MountOBBVisitor : public IFileSystem::DirectoryVisitor {
    AndroidFileSystem& android_fs_;

   public:
    MountOBBVisitor(AndroidFileSystem& android_fs)
      : android_fs_(android_fs)
    {}

    virtual ~MountOBBVisitor() {}

    virtual bool Visit(const char* filename_or_directory, bool is_directory) override
    {
      if (String(filename_or_directory).EndsWith(".obb") ||
          String(filename_or_directory).EndsWith(".obb.png")) {
        // It's and OBB (actually a ZIP) so we fake mount it.
        android_fs_.MountOBB(filename_or_directory);
      }
      return true;
    }
  };

  void MountOBB(const char* filename) {
    AndroidFile* file = static_cast<AndroidFile*>(OpenRead(filename, true, false));
    fun_check_ptr(file);
    zip_resource_.AddPatchFile(MakeShareable(file));
    PlatformMisc::LowLevelOutputDebugStringf("Mounted OBB '%s'", filename);
  }

  AAssetManager* asset_mgr_;
  ZipUnionFile zip_resource_;
};

IFileSystem& IFileSystem::GetPhysicalFileSystem() {
  return AndroidFileSystem::GetPhysicalFileSystem();
}

IAndroidPlatformFS& IAndroidPlatformFS::GetPhysicalFileSystem() {
  return AndroidFileSystem::GetPhysicalFileSystem();
}
