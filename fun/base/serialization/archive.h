#pragma once

#include "fun/base/base.h"
#include "fun/base/serialization/compression.h"
//TODO base_forward_decls.h에 이미 있으므로 제거해도 무방함.
#include "fun/base/container/container_forward_decls.h"

namespace fun {

class CustomVersionContainer;
class Uuid;

class FUN_BASE_API Archive {
 public:
  Archive();
  virtual ~Archive();

  Archive(const Archive&);
  Archive& operator = (const Archive&);

  /**
   * Resets all of the base archive member variables.
   */
  virtual void Reset();

  FUN_ALWAYS_INLINE friend Archive& operator & (Archive& ar, char& v) {
    ar.Serialize(&v, 1);
    return ar;
  }

  FUN_ALWAYS_INLINE friend Archive& operator & (Archive& ar, wchar_t& v) {
    ar.ByteOrderAwaredSerialize(&v, sizeof(v));
    return ar;
  }

  FUN_ALWAYS_INLINE friend Archive& operator & (Archive& ar, int8& v) {
    ar.Serialize(&v,1);
    return ar;
  }
  FUN_ALWAYS_INLINE friend Archive& operator & (Archive& ar, uint8& v) {
    ar.Serialize(&v,1);
    return ar;
  }

  FUN_ALWAYS_INLINE friend Archive& operator & (Archive& ar, int16& v) {
    ar.ByteOrderAwaredSerialize(&v,sizeof(v));
    return ar;
  }
  FUN_ALWAYS_INLINE friend Archive& operator & (Archive& ar, uint16& v) {
    ar.ByteOrderAwaredSerialize(&v,sizeof(v));
    return ar;
  }

  FUN_ALWAYS_INLINE friend Archive& operator & (Archive& ar, int32& v) {
    ar.ByteOrderAwaredSerialize(&v,sizeof(v));
    return ar;
  }
  FUN_ALWAYS_INLINE friend Archive& operator & (Archive& ar, uint32& v) {
    ar.ByteOrderAwaredSerialize(&v,sizeof(v));
    return ar;
  }

  FUN_ALWAYS_INLINE friend Archive& operator & (Archive& ar, int64& v) {
    ar.ByteOrderAwaredSerialize(&v,sizeof(v));
    return ar;
  }
  FUN_ALWAYS_INLINE friend Archive& operator & (Archive& ar, uint64& v) {
    ar.ByteOrderAwaredSerialize(&v,sizeof(v));
    return ar;
  }

  // long을 별도의 타입으로 구분할까???
  //FUN_ALWAYS_INLINE friend Archive& operator & (Archive& ar, long& v) {
  //  ar.ByteOrderAwaredSerialize(&v,sizeof(v));
  //  return ar;
  //}

  FUN_ALWAYS_INLINE friend Archive& operator & (Archive& ar, float& v) {
    ar.ByteOrderAwaredSerialize(&v,sizeof(v));
    return ar;
  }
  FUN_ALWAYS_INLINE friend Archive& operator & (Archive& ar, double& v) {
    ar.ByteOrderAwaredSerialize(&v,sizeof(v));
    return ar;
  }

  FUN_BASE_API friend Archive& operator & (Archive& ar, String& v);

  virtual void Serialize(void* v, int64 len) {}

  virtual void SerializeBits(void* v, int64 len_in_bits) {
    Serialize(v, (len_in_bits + 7) / 8);
    if (IsLoading() && (len_in_bits & 7) != 0) {
      ((uint8*)v)[len_in_bits / 8] &= ((1 << (len_in_bits & 7)) - 1);
    }
  }

  virtual void SerializeInt(uint32& v, uint32 max) {
    ByteOrderAwaredSerialize(&v, sizeof(v));
  }

  virtual void SerializeIntPacked(uint32& v);

  //TODO??
  //virtual void Prelad(FObject* object) {}

  virtual void CountBytes(int64 used_count, int64 allocated_count) {}

  virtual String GetArchiveName() const;

  //TODO
  //virtual class FLinker* GetLinker() { return nullptr; }

  virtual int64 Tell() { return -1; }
  virtual int64 TotalSize() { return -1; }
  virtual bool AtEnd() {
    const int64 pos = Tell();
    return pos != -1 && pos >= TotalSize();
  }

  virtual void Seek(int64 new_pos) {}

  //TODO?
  //virtual void AttachBulkData(FObject* owner, UntypedBulkData* bulk_data) {}
  //virtual void DetachBulkData(UntypedBulkData* bulk_data, bool ensure_bulk_data_is_loaded) {}

  virtual bool Precache(int64 offset, int64 size) { return true; }
  virtual void FlushCache() {}

  virtual bool SetCompressionMap(Array<struct CompressedChunk>* chunks, CompressionFlags flags) { return false; }

  virtual void Flush() {}

  virtual bool Close() {
    return !(GetError() || IsCriticalError());
  }

  void SerializeCompressed(void* v, int64 len, CompressionFlags flags, bool treat_buffer_as_file_reader = false);

  FUN_ALWAYS_INLINE bool IsByteSwapping() const {
#if FUN_ARCH_LITTLE_ENDIAN
    const bool swap_bytes = archive_flags_.HasAny(ARCH_FORCE_BYTE_SWAPPING);
#else
    const bool swap_bytes = archive_flags_.HasAny(ARCH_PERSISTENT);
#endif
    return swap_bytes;
  }

  void ByteSwap(void* v, int64 len);

  FUN_ALWAYS_INLINE Archive& ByteOrderAwaredSerialize(void* v, int64 len) {
    Serialize(v, len);

    if (IsByteSwapping()) {
      ByteSwap(v, len);
    }

    return *this;
  }


  //TODO More
  enum ArchiveFlag {
    ARCH_LOADING = 0x01,
    ARCH_SAVING = 0x02,
    ARCH_TRANSACTING = 0x04,
    ARCH_FORCE_UNICODE = 0x08,
    ARCH_PERSISTENT = 0x10,
    ARCH_ERROR = 0x20,
    ARCH_CRITICAL_ERROR= 0x40,
    ARCH_FORCE_BYTE_SWAPPING = 0x80,
    ARCH_ALLOW_LAZY_LOADING = 0x100,
    ARCH_COUNTING_MEMORY = 0x200,
  };
  FUN_DECLARE_FLAGS_IN_CLASS(ArchiveFlags, ArchiveFlag);

  virtual bool IsCloseComplete(bool& out_has_error) {
    out_has_error = false;
    return true;
  }


  //
  // Flags operations.
  //

  bool IsLoading() const { return archive_flags_.HasAny(ARCH_LOADING); }
  void SetLoading(bool flag = true) { archive_flags_.SetBool(ARCH_LOADING, flag); }

  bool IsSaving() const { return archive_flags_.HasAny(ARCH_SAVING); }
  void SetSaving(bool flag = true) { archive_flags_.SetBool(ARCH_SAVING, flag); }

  bool IsTransacting() const { return archive_flags_.HasAny(ARCH_TRANSACTING); }
  void SetTransacting(bool flag = true) { archive_flags_.SetBool(ARCH_TRANSACTING, flag); }

  bool IsForceUnicode() const { return archive_flags_.HasAny(ARCH_FORCE_UNICODE); }
  void SetForceUnicode(bool flag = true) { archive_flags_.SetBool(ARCH_FORCE_UNICODE, flag); }

  bool IsPersistent() const { return archive_flags_.HasAny(ARCH_PERSISTENT); }
  void SetPersistent(bool flag = true) { archive_flags_.SetBool(ARCH_PERSISTENT, flag); }

  virtual bool GetError() const { return archive_flags_.HasAny(ARCH_ERROR); }
  void SetError(bool flag = true) { archive_flags_.SetBool(ARCH_ERROR, flag); }

  bool IsCriticalError() const { return archive_flags_.HasAny(ARCH_CRITICAL_ERROR); }
  void SetCriticalError(bool flag = true) { archive_flags_.SetBool(ARCH_CRITICAL_ERROR, flag); }

  bool IsForceByteSwapping() const { return archive_flags_.HasAny(ARCH_FORCE_BYTE_SWAPPING); }
  void SetForceByteSwapping(bool flag = true) { archive_flags_.SetBool(ARCH_FORCE_BYTE_SWAPPING, flag); }

  bool IsAllowLazyLoading() const { return archive_flags_.HasAny(ARCH_ALLOW_LAZY_LOADING); }
  void SetAllowLazyLoading(bool flag = true) { archive_flags_.SetBool(ARCH_ALLOW_LAZY_LOADING, flag); }

  bool IsCountingMemory() const { return archive_flags_.HasAny(ARCH_COUNTING_MEMORY); }
  void SetCountingMemory(bool flag = true) { archive_flags_.SetBool(ARCH_COUNTING_MEMORY, flag); }


  //
  // Custom version
  //

  virtual void SetCustomVersions(const CustomVersionContainer& custom_version_container);
  virtual void ResetCustomVersions();
  void SetCustomVersion(const Uuid& key, int32 version, const String& friendly_name);
  const CustomVersionContainer& GetCustomVersions() const;
  void UsingCustomVersion(const Uuid& key);
  int32 CustomVer(const Uuid& key) const;

 protected:
  ArchiveFlags archive_flags_;
  mutable CustomVersionContainer* custom_version_container_;
  mutable bool custom_versions_are_reset_;

  void CopyTrivialArchiveStatusMembers(const Archive& other);
};

class ArchiveProxy : public Archive {
 public:
  FUN_BASE_API ArchiveProxy(Archive& inner_archive);

  void Serialize(void* v, int64 len) override {
    inner_archive_.Serialize(v, len);
  }

  void SerializeBits(void* bits, int64 bit_count) override {
    inner_archive_.SerializeBits(bits, bit_count);
  }

  void SerializeInt(uint32& value, uint32 max) override {
    inner_archive_.SerializeInt(value, max);
  }

  //TODO
  //void Preload(FObject* object) override {
  //  inner_archive_.Preload(object);
  //}

  void CountBytes(int64 used_count, int64 allocated_count) override {
    inner_archive_.CountBytes(used_count, allocated_count);
  }

  FUN_BASE_API String GetArchiveName() const override;

  int64 Tell() override {
    return inner_archive_.Tell();
  }

  int64 TotalSize() override {
    return inner_archive_.TotalSize();
  }

  bool AtEnd() override {
    return inner_archive_.AtEnd();
  }

  void Seek(int64 new_pos) override {
    inner_archive_.Seek(new_pos);
  }

  //TODO
  //void AttachBulkData(FObject* owner, UntypedBulkData* bulk_data) override {
  //  inner_archive_.AttachBulkData(owner, bulk_data);
  //}
  //
  //void DetachBulkData(UntypedBulkData* bulk_data, bool ensure_bulk_data_is_loaded) override {
  //  inner_archive_.DetachBulkData(bulk_data, ensure_bulk_data_is_loaded);
  //}

  bool Precache(int64 offset, int64 size) override {
    return inner_archive_.Precache(offset, size);
  }

  bool SetCompressionMap(Array<struct CompressedChunk>* chunks,
                        CompressionFlags flags) override {
    return inner_archive_.SetCompressionMap(chunks, flags);
  }

  void Flush() override {
    inner_archive_.Flush();
  }

  bool Close() override {
    return inner_archive_.Close();
  }

  bool GetError() const override {
    return inner_archive_.GetError();
  }

  //TODO
  //void MarkScriptSerializationStart(const FObject* obj) override {
  //  inner_archive_.MarkScriptSerializationStart(obj);
  //}
  //
  //void MarkScriptSerializationEnd(const FObject* obj) override {
  //  inner_archive_.MarkScriptSerializationEnd(obj);
  //}

  bool IsCloseComplete(bool& out_has_error) override {
    return inner_archive_.IsCloseComplete(out_has_error);
  }

//TODO
//#if FUN_WITH_EDITOR
//  void PushDebugDataString(const String& debug_data) override {
//    inner_archive_.PushDebugDataString(debug_data);
//  }
//
//  void PopDebugDataString() override {
//    inner_archive_.PopDebugDataString();
//  }
//#endif //FUN_WITH_EDITOR

 protected:
  /** Holds the archive that this archive is a proxy to. */
  Archive& inner_archive_;
};

/**
 * Implements a helper structure for compression support
 *
 * This structure contains information on the compressed and
 * uncompressed size of a chunk of data.
 */
struct CompressedChunkInfo {
  /** Holds the data's compressed size. */
  int64 compressed_size;
  /** Holds the data's uncompresses size. */
  int64 uncompressed_size;

  FUN_ALWAYS_INLINE friend
  Archive& operator & (Archive& ar, CompressedChunkInfo& chunk) {
    return ar & chunk.compressed_size & chunk.uncompressed_size;
  }
};

} // namespace fun
