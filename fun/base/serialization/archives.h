#pragma once

#include "fun/base/base.h"
#include "fun/base/container/array.h"
#include "fun/base/serialization/archive.h"
#include "fun/base/string/string.h"

namespace fun {

/**
 * Base class for serializing arbitrary data in memory.
 */
class MemoryArchive : public Archive {
 public:
  String GetArchiveName() const override { return "MemoryArchive"; }

  void Seek(int64 new_pos) override { offset_ = new_pos; }

  int64 Tell() override { return offset_; }

  // TODO
  // Archive& operator & (Name& name) override {
  //  // Serialize the Name as a String
  //  if (IsLoading()) {
  //    String string_name;
  //    *this & string_name;
  //    name = Name(*string_name);
  //  } else {
  //    String string_name = name.ToString();
  //    *this & string_name;
  //  }
  //  return *this;
  //}

  // TODO
  // Archive& operator & (class FObject*& object) override {
  //  // Not supported through this archive
  //  //fun_check(0);
  //  return *this;
  //}

 protected:
  /**
   * Marked as protected to avoid instantiating this class directly.
   */
  MemoryArchive() : Archive(), offset_(0) {}

  /** Byte position for reading/writing. */
  int64 offset_;
};

/**
 * Archive for storing arbitrary data to the specified memory location
 */
class MemoryWriter : public MemoryArchive {
 public:
  MemoryWriter(Array<uint8>& bytes, bool is_persistent = false,
               bool appendable = false, const String archive_name = "")
      : MemoryArchive(), bytes_(bytes), archive_name_(archive_name) {
    SetSaving(true);
    SetPersistent(is_persistent);

    if (appendable) {
      offset_ = bytes.Count();
    }
  }

  void Serialize(void* v, int64 len) override {
    const int64 len_to_add = offset_ + len - bytes_.Count();
    if (len_to_add > 0) {
      const int64 new_array_len = bytes_.Count() + len_to_add;
      if (new_array_len >= int32_MAX) {
        // TODO
        // fun_log2(Serialization, Fatal, "MemoryWriter does not support data
        // larger than 2GB. Archive name: {}.", archive_name_.ToString());
      }

      bytes_.AddUninitialized((int32)len_to_add);
    }

    // fun_check((offset_ + len) <= (int64)bytes_.Count());

    if (len > 0) {
      UnsafeMemory::Memcpy(&bytes_[(int32)offset_], v, len);
      offset_ += len;
    }
  }

  int64 TotalSize() override { return bytes_.Count(); }

  String GetArchiveName() const override {
    return archive_name_.Len() > 0 ? archive_name_ : "MemoryWriter";
  }

 protected:
  Array<uint8>& bytes_;

  /** Archive name, used to debugging, by default set to "". */
  const String archive_name_;
};

/**
 * Buffer archiver.
 */
class BufferArchive : public MemoryWriter, public Array<uint8> {
 public:
  BufferArchive(bool is_persistent = false, const String archive_name = "")
      : MemoryWriter((Array<uint8>&)*this, is_persistent, false, archive_name) {
  }

  String GetArchiveName() const override {
    if (archive_name_.Len() > 0) {
      return String::Format("BufferArchive {0}", archive_name_);
    } else {
      return "BufferArchive";
    }
  }
};

/**
 * Archive for reading arbitrary data from the specified memory location
 */
class MemoryReader : public MemoryArchive {
 public:
  /**
   * Returns the name of the Archive.  Useful for getting the name of the
   * package a struct or object is in when a loading error occurs.
   *
   * This is overridden for the specific Archive Types
   */
  String GetArchiveName() const override { return "MemoryReader"; }

  int64 TotalSize() override {
    return MathBase::Min((int64)bytes_.Count(), limit_size_);
  }

  void Seek(int64 new_pos) override {
    // fun_check(new_pos <= TotalSize());
    MemoryArchive::Seek(new_pos);
  }

  void Serialize(void* v, int64 len) override {
    if (len > 0 && !GetError()) {
      // Only serialize if we have the requested amount of data
      if (offset_ + len <= TotalSize()) {
        UnsafeMemory::Memcpy(v, &bytes_[(int32)offset_], len);
        offset_ += len;
      } else {
        SetError();
      }
    }
  }

  MemoryReader(const Array<uint8>& bytes, bool is_persistent = false)
      : MemoryArchive(), bytes_(bytes), limit_size_(int64_MAX) {
    SetLoading(true);
    SetPersistent(is_persistent);
  }

  /**
   * With this method it's possible to attach data behind some serialized data.
   */
  void SetLimitSize(int64 limit_size) { limit_size_ = limit_size; }

 protected:
  const Array<uint8>& bytes_;
  int64 limit_size_;
};

/**
 * Archive objects that are also a Array. Since BufferArchive is already the
 * writer version, we just typedef to the old, outdated name
 */
typedef BufferArchive ArrayWriter;

class ArrayReader : public MemoryReader, public Array<uint8> {
 public:
  ArrayReader(bool is_persistent = false)
      : MemoryReader((Array<uint8>&)*this, is_persistent) {}

  String GetArchiveName() const override { return "ArrayReader"; }
};

/**
 * Similar to MemoryReader, but able to internally
 * manage the memory for the buffer.
 */
class BufferReader : public Archive {
 public:
  /**
   * Constructor
   *
   * @param data - Buffer to use as the source data to read from
   * @param size - Size of Data
   * @param free_on_close - If true, Data will be UnsafeMemory::Free'd when this
   * archive is closed
   * @param is_persistent - Uses this value for ArIsPersistent
   */
  BufferReader(void* data, int64 size, bool free_on_close,
               bool is_persistent = false)
      : reader_data_(data),
        reader_pos_(0),
        reader_size_(size),
        free_on_close_(free_on_close) {
    SetLoading(true);
    SetPersistent(is_persistent);
  }

  ~BufferReader() { Close(); }

  bool Close() override {
    if (free_on_close_) {
      UnsafeMemory::Free(reader_data_);
      reader_data_ = nullptr;
    }
    return !GetError();
  }

  void Serialize(void* v, int64 len) override {
    // fun_check(reader_pos_ >= 0);
    // fun_check(reader_pos_ + len <= reader_size_);
    UnsafeMemory::Memcpy(v, (uint8*)reader_data_ + reader_pos_, len);
    reader_pos_ += len;
  }

  int64 Tell() override { return reader_pos_; }

  int64 TotalSize() override { return reader_size_; }

  void Seek(int64 new_pos) override {
    // fun_check(new_pos >= 0);
    // fun_check(new_pos <= reader_size_);
    reader_pos_ = new_pos;
  }

  bool AtEnd() override { return reader_pos_ >= reader_size_; }

  String GetArchiveName() const override { return "BufferReader"; }

 protected:
  void* reader_data_;
  int64 reader_pos_;
  int64 reader_size_;
  bool free_on_close_;
};

/**
 * Similar to MemoryWriter, but able to internally
 * manage the memory for the buffer.
 */
class BufferWriter : public Archive {
 public:
  /**
   * Constructor
   *
   * @param data - Buffer to use as the source data to read from
   * @param size - Size of Data
   * @param free_on_close - If true, Data will be UnsafeMemory::Free'd when this
   * archive is closed
   * @param is_persistent - Uses this value for ArIsPersistent
   */
  BufferWriter(void* data, int64 size, bool free_on_close,
               bool is_persistent = false)
      : writer_data_(data),
        writer_pos_(0),
        writer_size_(size),
        free_on_close_(free_on_close) {
    SetSaving(true);
    SetPersistent(is_persistent);
  }

  ~BufferWriter() { Close(); }

  bool Close() override {
    if (free_on_close_) {
      UnsafeMemory::Free(writer_data_);
      writer_data_ = nullptr;
    }
    return !GetError();
  }

  void Serialize(void* v, int64 len) override {
    const int64 len_to_add = writer_pos_ + len - writer_size_;
    if (len_to_add > 0) {
      const int64 new_array_len = writer_size_ + len_to_add;
      if (new_array_len >= int32_MAX) {
        // TODO
        // fun_log(LogSerialization, Fatal, "BufferWriter does not support data
        // larger than 2GB. Archive name: {}.", *GetArchiveName());
      }

      UnsafeMemory::Realloc(writer_data_, new_array_len);
      writer_size_ = new_array_len;
    }

    // fun_check(writer_pos_ >= 0);
    // fun_check((writer_pos_ + len) <= writer_size_);
    UnsafeMemory::Memcpy((uint8*)writer_data_ + writer_pos_, v, len);
    writer_pos_ += len;
  }

  int64 Tell() override { return writer_pos_; }

  int64 TotalSize() override { return writer_size_; }

  void Seek(int64 new_pos) override {
    // fun_check(new_pos >= 0);
    // fun_check(new_pos <= writer_size_);
    writer_pos_ = new_pos;
  }

  bool AtEnd() override { return writer_pos_ >= writer_size_; }

  String GetArchiveName() const override { return "BufferWriter"; }

  void* GetWriterData() { return writer_data_; }

 protected:
  void* writer_data_;
  int64 writer_pos_;
  int64 writer_size_;
  bool free_on_close_;
};

/**
 * Archive Proxy to transparently write out compressed data to an array.
 */
class FUN_BASE_API ArchiveSaveCompressedProxy : public Archive {
 public:
  /**
   * Constructor, initializing all member variables and allocating temp memory.
   *
   * @param compressed_data - [ref]  Array of bytes that is going to hold
   * compressed data
   * @param compression_flags - Compression flags to use for compressing data
   */
  ArchiveSaveCompressedProxy(Array<uint8>& compressed_data,
                             CompressionFlags compression_flags);

  /** Destructor, flushing array if needed. Also frees temporary memory. */
  ~ArchiveSaveCompressedProxy();

  // Archive interface
  void Flush() override;
  void Serialize(void* v, int64 len) override;
  void Seek(int64 new_pos) override;
  int64 Tell() override;

 private:
  /** Array to write compressed data to. */
  Array<uint8>& compressed_data_;

  /** Current index in array. */
  int32 current_index_;

  /** Pointer to start of temporary buffer. */
  uint8* tmp_data_start_;

  /** Pointer to end of temporary buffer. */
  uint8* tmp_data_end_;

  /** Pointer to current position in temporary buffer. */
  uint8* tmp_data_;

  /** Whether to serialize to temporary buffer of array. */
  bool should_serialize_to_array_;

  /** Number of raw (uncompressed) bytes serialized. */
  int64 raw_bytes_serialized_;

  /** Flags to use for compression. */
  CompressionFlags compression_flags_;
};

/**
 * Archive Proxy to transparently load compressed data from an array.
 */
class FUN_BASE_API ArchiveLoadCompressedProxy : public Archive {
 public:
  /**
   * Constructor, initializing all member variables and allocating temp memory.
   *
   * @param compressed_data - Array of bytes that is holding compressed data
   * @param compression_flags - Compression flags that were used to compress
   * data
   */
  ArchiveLoadCompressedProxy(const Array<uint8>& compressed_data,
                             CompressionFlags compression_flags);

  /** Destructor, freeing temporary memory. */
  ~ArchiveLoadCompressedProxy();

  // Archive interface
  void Serialize(void* v, int64 len) override;
  void Seek(int64 new_pos) override;
  int64 Tell() override;

 private:
  /** Flushes tmp data to array. */
  void DecompressMoreData();

  /** Array to write compressed data to. */
  const Array<uint8>& compressed_data_;

  /** Current index into compressed data array. */
  int32 current_index_;

  /** Pointer to start of temporary buffer. */
  uint8* tmp_data_start_;

  /** Pointer to end of temporary buffer. */
  uint8* tmp_data_end_;

  /** Pointer to current position in temporary buffer. */
  uint8* tmp_data_;

  /** Whether to serialize from temporary buffer of array. */
  bool should_serialize_from_array_;

  /** Number of raw (uncompressed) bytes serialized. */
  int64 raw_bytes_serialized_;

  /** Flags used for compression. */
  CompressionFlags compression_flags_;
};

}  // namespace fun
