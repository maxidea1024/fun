#include <sys/utime.h>
#include "CorePrivatePCH.h"

namespace fun {

#include "Platform/Windows/AllowWindowsTypes.h"
namespace FileConstants {
uint32 WIN_INVALID_SET_FILE_POINTER = INVALID_SET_FILE_POINTER;
}
#include "Platform/Windows/HideWindowsTypes.h"

namespace {

inline int32 ToWindowsDayOfWeek(const DayOfWeekType dow) {
  switch (dow) {
    case DayOfWeekType::Monday:
      return 1;
    case DayOfWeekType::Tuesday:
      return 2;
    case DayOfWeekType::Wednesday:
      return 3;
    case DayOfWeekType::Thursday:
      return 4;
    case DayOfWeekType::Friday:
      return 5;
    case DayOfWeekType::Saturday:
      return 6;
    case DayOfWeekType::Sunday:
      return 0;
    default:
      break;
  }

  return 0;
}

inline DateTime FromWindowsFileTime(const FILETIME& ft) {
  // TODO 초단위 미만까지 조회가 가능한데... 해당 요소를 살려주어야할듯!

  // This roundabout conversion clamps the precision of
  // the returned time value to match that of time_t (1 second precision)
  // This avoids issues when sending files
  // over the network via cook-on-the-fly
  SYSTEMTIME st;
  if (FileTimeToSystemTime(&ft, &st)) {
    return DateTime(Date(st.wYear, st.wMonth, st.wDay),
                    Time(st.wHour, st.wMinute, st.wSecond));
  }

  // Failed to convert
  return DateTime::Null;
}

inline FILETIME ToWindowsFileTime(const DateTime& dt) {
  // This roundabout conversion clamps the precision of the returned
  // time value to match that of time_t (1 second precision)
  // This avoids issues when sending files
  // over the network via cook-on-the-fly
  SYSTEMTIME st;
  st.wYear = dt.Year();
  st.wMonth = dt.Month();
  st.wDay = dt.Day();
  st.wDayOfWeek = ToWindowsDayOfWeek(dt.DayOfWeek());
  st.wHour = dt.Hour();
  st.wMinute = dt.Minute();
  st.wSecond = dt.Second();
  st.wMilliseconds = 0;

  FILETIME ft;
  SystemTimeToFileTime(&st, &ft);

  return ft;
}

}  // namespace

/**
 * This file reader uses overlapped i/o and double buffering to
 * asynchronously read from files.
 */
class FUN_BASE_API AsyncBufferedFileReaderWindows : public IFile {
 protected:
  enum { DEFAULT_BUFFER_SIZE = 64 * 1024 };

  /** The file handle to operate on */
  HANDLE Handle;
  /** The size of the file that is being read */
  int64 file_size;
  /** Overall position in the file and buffers combined */
  int64 FilePos;
  /** Overall position in the file as the OverlappedIO struct understands it */
  uint64 OverlappedFilePos;
  /** These are the two buffers used for reading the file asynchronously */
  int8* Buffers[2];
  /** The size of the buffers in bytes */
  const int32 BufferSize;
  /** The current index of the buffer that we are serializing from */
  int32 SerializeBuffer;
  /** The current index of the streaming buffer for async reading into */
  int32 StreamBuffer;
  /** Where we are in the serialize buffer */
  int32 SerializePos;
  /** Tracks which buffer has the async read outstanding (0 = first read after
   * create/seek, 1 = streaming buffer) */
  int32 CurrentAsyncReadBuffer;
  /** The overlapped IO struct to use for determining async state */
  OVERLAPPED OverlappedIO;
  /** Used to track whether the last read reached the end of the file or not.
   * Reset when a Seek happens */
  bool bIsAtEOF;
  /** Whether there's a read outstanding or not */
  bool bHasReadOutstanding;

  /** Closes the file handle */
  bool Close() {
    if (Handle != nullptr) {
      // Close the file handle
      CloseHandle(Handle);
      Handle = nullptr;
    }
    return true;
  }

  /** This toggles the buffers we read into & serialize out of between buffer
   * indices 0 & 1 */
  inline void SwapBuffers() {
    StreamBuffer ^= 1;
    SerializeBuffer ^= 1;
    // We are now at the beginning of the serialize buffer
    SerializePos = 0;
  }

  inline void CopyOverlappedPosition() {
    ULARGE_INTEGER li;
    li.QuadPart = OverlappedFilePos;
    OverlappedIO.Offset = li.LowPart;
    OverlappedIO.OffsetHigh = li.HighPart;
  }

  inline void UpdateFileOffsetAfterRead(uint32 AmountRead) {
    bHasReadOutstanding = false;
    OverlappedFilePos += AmountRead;
    // Update the overlapped structure since it uses this for where to read from
    CopyOverlappedPosition();
    if (OverlappedFilePos >= uint64(file_size)) {
      bIsAtEOF = true;
    }
  }

  bool WaitForAsyncRead() {
    // Check for already being at EOF because we won't issue a read
    if (bIsAtEOF || !bHasReadOutstanding) {
      return true;
    }
    uint32 NumRead = 0;
    if (GetOverlappedResult(Handle, &OverlappedIO, (::DWORD*)&NumRead, true) !=
        false) {
      UpdateFileOffsetAfterRead(NumRead);
      return true;
    } else if (GetLastError() == ERROR_HANDLE_EOF) {
      bIsAtEOF = true;
      return true;
    }
    return false;
  }

  void StartAsyncRead(int32 BufferToReadInto) {
    if (!bIsAtEOF) {
      bHasReadOutstanding = true;
      CurrentAsyncReadBuffer = BufferToReadInto;
      uint32 NumRead = 0;
      // Now kick off an async read
      if (!ReadFile(Handle, Buffers[BufferToReadInto], BufferSize,
                    (::DWORD*)&NumRead, &OverlappedIO)) {
        uint32 ErrorCode = GetLastError();
        if (ErrorCode != ERROR_IO_PENDING) {
          bIsAtEOF = true;
          bHasReadOutstanding = false;
        }
      } else {
        // Read completed immediately
        UpdateFileOffsetAfterRead(NumRead);
      }
    }
  }

  inline void StartStreamBufferRead() { StartAsyncRead(StreamBuffer); }

  inline void StartSerializeBufferRead() { StartAsyncRead(SerializeBuffer); }

  inline bool IsValid() {
    return Handle != nullptr && Handle != INVALID_HANDLE_VALUE;
  }

 public:
  AsyncBufferedFileReaderWindows(HANDLE InHandle,
                                 int32 InBufferSize = DEFAULT_BUFFER_SIZE)
      : Handle(InHandle),
        FilePos(0),
        OverlappedFilePos(0),
        BufferSize(InBufferSize),
        SerializeBuffer(0),
        StreamBuffer(1),
        SerializePos(0),
        CurrentAsyncReadBuffer(0),
        bIsAtEOF(false),
        bHasReadOutstanding(false) {
    LARGE_INTEGER li;
    GetFileSizeEx(Handle, &li);
    file_size = li.QuadPart;
    // Allocate our two buffers
    Buffers[0] = (int8*)UnsafeMemory::Malloc(BufferSize);
    Buffers[1] = (int8*)UnsafeMemory::Malloc(BufferSize);

    // Zero the overlapped structure
    UnsafeMemory::Memzero(&OverlappedIO, sizeof(OVERLAPPED));

    // Kick off the first async read
    StartSerializeBufferRead();
  }

  virtual ~AsyncBufferedFileReaderWindows() {
    // Can't free the buffers or close the file if a read is outstanding
    WaitForAsyncRead();
    Close();
    UnsafeMemory::Free(Buffers[0]);
    UnsafeMemory::Free(Buffers[1]);
  }

  bool Seek(int64 new_pos) override {
    fun_check(IsValid());
    fun_check(new_pos >= 0);
    fun_check(new_pos <= file_size);

    // Determine the change in locations
    int64 PosDelta = new_pos - FilePos;
    if (PosDelta == 0) {
      // Same place so no work to do
      return true;
    }

    // No matter what, we need to wait for the current async read to finish
    // since we most likely need to issue a new one
    if (!WaitForAsyncRead()) {
      return false;
    }

    FilePos = new_pos;

    // If the requested location is not within our current serialize buffer, we
    // need to start the whole read process over
    const bool bWithinSerializeBuffer =
        (PosDelta < 0 && (SerializePos - MathBase::Abs(PosDelta) >= 0)) ||
        (PosDelta > 0 && ((PosDelta + SerializePos) < BufferSize));
    if (bWithinSerializeBuffer) {
      // Still within the serialize buffer so just update the position
      SerializePos += PosDelta;
    } else {
      // Reset our EOF tracking and let the read handle setting it if need be
      bIsAtEOF = false;
      // Not within our buffer so start a new async read on the serialize buffer
      OverlappedFilePos = new_pos;
      CopyOverlappedPosition();
      CurrentAsyncReadBuffer = SerializeBuffer;
      SerializePos = 0;
      StartSerializeBufferRead();
    }
    return true;
  }

  bool SeekFromEnd(int64 relative_pos_to_end = 0) override {
    fun_check(IsValid());
    fun_check(relative_pos_to_end <= 0);

    // Position is negative so this is actually subtracting
    return Seek(file_size + relative_pos_to_end);
  }

  int64 Tell() override {
    fun_check(IsValid());
    return FilePos;
  }

  int64 Size() override {
    fun_check(IsValid());
    return file_size;
  }

  bool Read(uint8* dst, int64 bytes_to_read) override {
    fun_check(IsValid());
    // If zero were requested, quit (some calls like to do zero sized reads)
    if (bytes_to_read <= 0) {
      return false;
    }

    if (CurrentAsyncReadBuffer == SerializeBuffer) {
      // First async read after either construction or a seek
      if (!WaitForAsyncRead()) {
        return false;
      }
      StartStreamBufferRead();
    }

    fun_check(dst != nullptr)
        // While there is data to copy
        while (bytes_to_read > 0) {
      // Figure out how many bytes we can read from the serialize buffer
      int64 len_to_copy =
          MathBase::Min<int64>(bytes_to_read, BufferSize - SerializePos);
      if (FilePos + len_to_copy > file_size) {
        // Tried to read past the end of the file, so fail
        return false;
      }
      // See if we are at the end of the serialize buffer or not
      if (len_to_copy > 0) {
        UnsafeMemory::Memcpy(dst, &Buffers[SerializeBuffer][SerializePos],
                             len_to_copy);

        // Update the internal positions
        SerializePos += len_to_copy;
        fun_check(SerializePos <= BufferSize);
        FilePos += len_to_copy;
        fun_check(FilePos <= file_size);

        // Decrement the number of bytes we copied
        bytes_to_read -= len_to_copy;

        // Now offset the dst pointer with the chunk we copied
        dst = (uint8*)dst + len_to_copy;
      } else {
        // We've crossed the buffer boundary and now need to make sure the
        // stream buffer read is done
        if (!WaitForAsyncRead()) {
          return false;
        }
        SwapBuffers();
        StartStreamBufferRead();
      }
    }
    return true;
  }

  bool Write(const uint8* src, int64 bytes_to_write) override {
    fun_check(0 && "This is an async reader only and doesn't support writing");
    return false;
  }
};

/**
 * Windows file handle implementation
 */
class FUN_BASE_API WindowsFile : public IFile {
  enum { READWRITE_SIZE = 1024 * 1024 };
  HANDLE file_handle_;

  inline int64 FileSeek(int64 distance, uint32 move_method) {
    LARGE_INTEGER li;
    li.QuadPart = distance;
    li.LowPart =
        SetFilePointer(file_handle_, li.LowPart, &li.HighPart, move_method);
    if (li.LowPart == FileConstants::WIN_INVALID_SET_FILE_POINTER &&
        GetLastError() != NO_ERROR) {
      li.QuadPart = -1;
    }
    return li.QuadPart;
  }

  inline bool IsValid() {
    return file_handle_ != nullptr && file_handle_ != INVALID_HANDLE_VALUE;
  }

 public:
  WindowsFile(HANDLE file_handle = nullptr) : file_handle_(file_handle) {}

  virtual ~WindowsFile() {
    CloseHandle(file_handle_);
    file_handle_ = nullptr;
  }

  int64 Tell() override {
    fun_check(IsValid());
    return FileSeek(0, FILE_CURRENT);
  }

  bool Seek(int64 new_pos) override {
    fun_check(IsValid());
    fun_check(new_pos >= 0);
    return FileSeek(new_pos, FILE_BEGIN) != -1;
  }

  bool SeekFromEnd(int64 relative_pos_to_end = 0) override {
    fun_check(IsValid());
    fun_check(relative_pos_to_end <= 0);
    return FileSeek(relative_pos_to_end, FILE_END) != -1;
  }

  bool Read(uint8* dst, int64 bytes_to_read) override {
    fun_check(IsValid());
    while (bytes_to_read) {
      fun_check(bytes_to_read >= 0);
      int64 adjusted_readable_len =
          MathBase::Min<int64>(READWRITE_SIZE, bytes_to_read);
      fun_check_ptr(dst);
      uint32 result = 0;
      if (!ReadFile(file_handle_, dst, uint32(adjusted_readable_len),
                    (::DWORD*)&result, nullptr) ||
          result != uint32(adjusted_readable_len)) {
        return false;
      }
      dst += adjusted_readable_len;
      bytes_to_read -= adjusted_readable_len;
    }
    return true;
  }

  bool Write(const uint8* src, int64 bytes_to_write) override {
    fun_check(IsValid());
    while (bytes_to_write) {
      fun_check(bytes_to_write >= 0);
      int64 adjusted_writable_len =
          MathBase::Min<int64>(READWRITE_SIZE, bytes_to_write);
      fun_check(src);
      uint32 result = 0;
      if (!WriteFile(file_handle_, src, uint32(adjusted_writable_len),
                     (::DWORD*)&result, nullptr) ||
          result != uint32(adjusted_writable_len)) {
        return false;
      }
      src += adjusted_writable_len;
      bytes_to_write -= adjusted_writable_len;
    }
    return true;
  }
};

/**
 * Windows File I/O implementation
 */
class FUN_BASE_API WindowsFileSystem : public IPhysicalFileSystem {
 protected:
  virtual String NormalizeFilename(const char* filename) {
    String result(filename);
    CPaths::NormalizeFilename(result);
    if (result.StartsWith("//")) {
      result = String("\\\\") + result.RightChopped(2);
    }
    return CPaths::ConvertRelativePathToFull(result);
  }

  virtual String NormalizeDirectory(const char* directory) {
    String result(directory);
    CPaths::NormalizeDirectoryName(result);
    if (result.StartsWith("//")) {
      result = String("\\\\") + result.RightChopped(2);
    }
    return CPaths::ConvertRelativePathToFull(result);
  }

 public:
  bool FileExists(const char* filename) override {
    uint32 result = GetFileAttributesW(*NormalizeFilename(filename));
    if (result != 0xFFFFFFFF && !(result & FILE_ATTRIBUTE_DIRECTORY)) {
      return true;
    }
    return false;
  }

  int64 FileSize(const char* filename) override {
    WIN32_FILE_ATTRIBUTE_DATA info;
    if (!!GetFileAttributesExW(*NormalizeFilename(filename),
                               GetFileExInfoStandard, &info)) {
      if ((info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
        LARGE_INTEGER li;
        li.HighPart = info.nFileSizeHigh;
        li.LowPart = info.nFileSizeLow;
        return li.QuadPart;
      }
    }
    return -1;
  }

  bool DeleteFile(const char* filename) override {
    const String normalized_filename = NormalizeFilename(filename);
    return !!DeleteFileW(*normalized_filename);
  }

  bool IsReadOnly(const char* filename) override {
    uint32 result = GetFileAttributes(*NormalizeFilename(filename));
    if (result != 0xFFFFFFFF) {
      return !!(result & FILE_ATTRIBUTE_READONLY);
    }
    return false;
  }

  bool MoveFile(const char* to, const char* from) override {
    return !!MoveFileW(*NormalizeFilename(from), *NormalizeFilename(to));
  }

  bool SetReadOnly(const char* filename, bool readonly) override {
    return !!SetFileAttributesW(
        *NormalizeFilename(filename),
        readonly ? FILE_ATTRIBUTE_READONLY : FILE_ATTRIBUTE_NORMAL);
  }

  DateTime GetTimestamp(const char* filename) override {
    WIN32_FILE_ATTRIBUTE_DATA info;
    if (GetFileAttributesExW(*NormalizeFilename(filename),
                             GetFileExInfoStandard, &info)) {
      return FromWindowsFileTime(info.ftLastWriteTime);
    }

    return DateTime::Null;
  }

  void SetTimestamp(const char* filename, const DateTime& timestamp) override {
    HANDLE Handle =
        CreateFileW(*NormalizeFilename(filename), FILE_WRITE_ATTRIBUTES,
                    FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, nullptr);
    if (Handle != INVALID_HANDLE_VALUE) {
      const FILETIME modification_ft = ToWindowsFileTime(timestamp);
      if (!SetFileTime(Handle, nullptr, nullptr, &modification_ft)) {
        fun_log(LogTemp, Warning, "SetTimestamp: Failed to SetFileTime on {}",
                filename);
      }
      CloseHandle(Handle);
    } else {
      fun_log(LogTemp, Warning, "SetTimestamp: Failed to open file {}",
              filename);
    }
  }

  DateTime GetAccessTimestamp(const char* filename) override {
    WIN32_FILE_ATTRIBUTE_DATA info;
    if (GetFileAttributesExW(*NormalizeFilename(filename),
                             GetFileExInfoStandard, &info)) {
      return FromWindowsFileTime(info.ftLastAccessTime);
    }

    return DateTime::Null;
  }

  String GetFilenameOnDisk(const char* filename) override {
    String result;
    WIN32_FIND_DATAW data;
    String normalized_filename = NormalizeFilename(filename);
    while (normalized_filename.Len()) {
      HANDLE handle = FindFirstFileW(*normalized_filename, &data);
      if (handle != INVALID_HANDLE_VALUE) {
        if (result.Len()) {
          result = String(data.cFileName) / result;
        } else {
          result = data.cFileName;
        }
        FindClose(handle);
      }

      int32 separator_index = INVALID_INDEX;
      if ((separator_index = normalized_filename.LastIndexOf('/')) !=
          INVALID_INDEX) {
        normalized_filename = normalized_filename.Mid(0, separator_index);
      }
      if (normalized_filename.Len() && (separator_index == INVALID_INDEX ||
                                        normalized_filename.EndsWith(":"))) {
        result = normalized_filename / result;
        normalized_filename.Empty();
      }
    }
    return result;
  }

  IFile* OpenRead(const char* filename, bool allow_write = false) override {
    const uint32 access = GENERIC_READ;
    const uint32 flags = FILE_SHARE_READ | (allow_write ? FILE_SHARE_WRITE : 0);
    const uint32 create = OPEN_EXISTING;
    const String normalized_filename = NormalizeFilename(filename);
    const HANDLE handle =
        CreateFileW(*normalized_filename, access, flags, nullptr, create,
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, nullptr);
    if (handle != INVALID_HANDLE_VALUE) {
      return new AsyncBufferedFileReaderWindows(handle);
    }
    return nullptr;
  }

  IFile* OpenWrite(const char* filename, bool append = false,
                   bool allow_read = false) override {
    // FIXME: 파일을 액세스하는 중에 로깅을 시도하면 클래시가 되는데, 왜
    // 그런건지?? 로깅을 위해 로그파일을 쓰기 모드로 열게 되는데, 이때 재귀가
    //발생함. OpenWrite에서는 로깅을 할 수 없다는 얘기가 되는건가? 개선을
    //해봐야할것으로 보임. fun_log(LogCore, info, "OpenWrite: %s", filename);

    // OutputDebugString("WRITE: \"");
    // OutputDebugString(filename);
    // OutputDebugString("\"\r\n");

    const uint32 access = GENERIC_WRITE;
    const uint32 flags = allow_read ? FILE_SHARE_READ : 0;
    const uint32 create = append ? OPEN_ALWAYS : CREATE_ALWAYS;
    const HANDLE handle =
        CreateFileW(*NormalizeFilename(filename), access, flags, nullptr,
                    create, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle != INVALID_HANDLE_VALUE) {
      WindowsFile* file = new WindowsFile(handle);
      if (append) {
        file->SeekFromEnd(0);
      }
      return file;
    }
    return nullptr;
  }

  bool DirectoryExists(const char* directory) override {
    // Empty directory is the current directory so assume it always exists.
    bool exists = !CharTraits::Strlen(directory);
    if (!exists) {
      const uint32 result = GetFileAttributesW(*NormalizeDirectory(directory));
      exists = (result != 0xFFFFFFFF && (result & FILE_ATTRIBUTE_DIRECTORY));
    }
    return exists;
  }

  bool CreateDirectory(const char* directory) override {
    return CreateDirectoryW(*NormalizeDirectory(directory), nullptr) ||
           GetLastError() == ERROR_ALREADY_EXISTS;
  }

  bool DeleteDirectory(const char* directory) override {
    RemoveDirectoryW(*NormalizeDirectory(directory));
    return !DirectoryExists(directory);
  }

  FileStatData GetStatData(const char* filename_or_directory) override {
    WIN32_FILE_ATTRIBUTE_DATA info;
    if (GetFileAttributesExW(*NormalizeFilename(filename_or_directory),
                             GetFileExInfoStandard, &info)) {
      const bool is_directory =
          !!(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);

      int64 file_size = -1;
      if (!is_directory) {
        LARGE_INTEGER li;
        li.HighPart = info.nFileSizeHigh;
        li.LowPart = info.nFileSizeLow;
        file_size = static_cast<int64>(li.QuadPart);
      }

      return FileStatData(FromWindowsFileTime(info.ftCreationTime),
                          FromWindowsFileTime(info.ftLastAccessTime),
                          FromWindowsFileTime(info.ftLastWriteTime), file_size,
                          is_directory,
                          !!(info.dwFileAttributes & FILE_ATTRIBUTE_READONLY));
    }

    return FileStatData();
  }

  bool IterateDirectory(const char* directory,
                        DirectoryVisitor& visitor) override {
    const String directory_str = directory;
    return IterateDirectoryCommon(
        directory, [&](const WIN32_FIND_DATAW& data) -> bool {
          const bool is_directory =
              !!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
          return visitor.Visit(*(directory_str / data.cFileName), is_directory);
        });
  }

  bool IterateDirectoryStat(const char* directory,
                            DirectoryStatVisitor& visitor) override {
    const String directory_str = directory;
    return IterateDirectoryCommon(
        directory, [&](const WIN32_FIND_DATAW& data) -> bool {
          const bool is_directory =
              !!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);

          int64 file_size = -1;
          if (!is_directory) {
            LARGE_INTEGER li;
            li.HighPart = data.nFileSizeHigh;
            li.LowPart = data.nFileSizeLow;
            file_size = static_cast<int64>(li.QuadPart);
          }

          return visitor.Visit(
              *(directory_str / data.cFileName),
              FileStatData(
                  FromWindowsFileTime(data.ftCreationTime),
                  FromWindowsFileTime(data.ftLastAccessTime),
                  FromWindowsFileTime(data.ftLastWriteTime), file_size,
                  is_directory,
                  !!(data.dwFileAttributes & FILE_ATTRIBUTE_READONLY)));
        });
  }

  bool IterateDirectoryCommon(
      const char* directory,
      const FunctionRef<bool(const WIN32_FIND_DATAW&)>& visitor) {
    bool result = false;
    WIN32_FIND_DATAW data;
    HANDLE handle =
        FindFirstFileW(*(NormalizeDirectory(directory) / "*.*"), &data);
    if (handle != INVALID_HANDLE_VALUE) {
      result = true;
      do {
        if (CharTraits::Strcmp(data.cFileName, ".") &&
            CharTraits::Strcmp(data.cFileName, "..")) {
          result = visitor(data);
        }
      } while (result && FindNextFileW(handle, &data));
      FindClose(handle);
    }
    return result;
  }
};

IFileSystem& IFileSystem::GetPhysicalFileSystem() {
  static WindowsFileSystem singleton;
  return singleton;
}

}  // namespace fun
