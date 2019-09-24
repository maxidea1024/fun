#include "fun/base/serialization/archives.h"
#include "fun/base/serialization/custom_version.h"
#include "fun/base/serialization/compression.h"
#include "fun/base/byte_order.h"
#include "fun/base/uuid.h"
#include "fun/base/string/string.h"
#include "fun/base/container/array.h"

#ifdef _MSC_VER
#pragma warning(disable : 4244)
#endif

namespace fun {

//
// ArchiveProxy implementation.
//

ArchiveProxy::ArchiveProxy(Archive& inner_archive)
  : Archive(inner_archive)
  , inner_archive_(inner_archive) {}


//
// Archive implementation.
//

Archive::Archive() {
  custom_version_container_ = new CustomVersionContainer;

  Reset();
}

Archive::Archive(const Archive& rhs) {
  CopyTrivialArchiveStatusMembers(rhs);

  // Don't know why this is set to false, but this is what the original copying code did
  //ArIsFilterEditorOnly  = false;

  custom_version_container_ = new CustomVersionContainer(*rhs.custom_version_container_);
}

Archive& Archive::operator = (const Archive& rhs) {
  CopyTrivialArchiveStatusMembers(rhs);

  // Don't know why this is set to false, but this is what the original copying code did
  //ArIsFilterEditorOnly  = false;

  *custom_version_container_ = *rhs.custom_version_container_;

  return *this;
}

Archive::~Archive() {
  delete custom_version_container_;
}

void Archive::Reset() {
  //TODO 다른건 적용안해도 되나, 기본 버젼 정보는 지원해야함.

  //TODO
  //ArNetVer = GEngineNegotiationVersion;
  //ArVer = GPackageFileVersion;
  //ArLicenseeVer = GPackageFileLicenseeVersion;
  //ArEngineVer = CEngineVersion::Current();

  archive_flags_.ClearAll();

  custom_versions_are_reset_ = false;

  /*
  ArIsLoading = false;
  ArIsSaving = false;
  //ArIsTransacting = false;
  //ArWantBinaryPropertySerialization = false;
  ArForceUnicode = false;
  ArIsPersistent = false;
  ArIsError = false;
  ArIsCriticalError = false;
  //ArContainsCode = false;
  //ArContainsMap = false;
  //ArRequiresLocalizationGather = false;
  ArForceByteSwapping = false;
  //ArSerializingDefaults = false;
  //ArIgnoreArchetypeRef = false;
  //ArNoDelta = false;
  //ArIgnoreOuterRef = false;
  //ArIgnoreClassGeneratedByRef = false;
  //ArIgnoreClassRef = false;
  //ArAllowLazyLoading = false;
  //ArIsObjectReferenceCollector = false;
  //ArIsModifyingWeakAndStrongReferences = false;
  ArIsCountingMemory = false;
  //ArPortFlags = 0;
  //ArShouldSkipBulkData = false;
  ArMaxSerializeSize = 0;
  //ArIsFilterEditorOnly = false;
  //ArIsSaveGame = false;
  //CookingTargetPlatform = nullptr;
  //SerializedProperty = nullptr;
//#if WITH_EDITORONLY_DATA
//  EditorOnlyPropertyStack = 0;
//#endif
//#if FUN_WITH_EDITOR
//  ArDebugSerializationFlags = 0;
//#endif
*/

  // Reset all custom versions to the current registered versions.
  ResetCustomVersions();
}

void Archive::CopyTrivialArchiveStatusMembers(const Archive& rhs) {
  //TODO
  //ArNetVer = rhs.ArNetVer;
  //ArVer = rhs.ArVer;
  //ArLicenseeVer = rhs.ArLicenseeVer;
  //ArEngineVer = rhs.ArEngineVer;

  archive_flags_ = rhs.archive_flags_;

  /*
  ArIsLoading = rhs.ArIsLoading;
  ArIsSaving = rhs.ArIsSaving;
  //ArIsTransacting = rhs.ArIsTransacting;
  //ArWantBinaryPropertySerialization = rhs.ArWantBinaryPropertySerialization;
  ArForceUnicode = rhs.ArForceUnicode;
  ArIsPersistent = rhs.ArIsPersistent;
  ArIsError = rhs.ArIsError;
  ArIsCriticalError = rhs.ArIsCriticalError;
  //ArContainsCode = rhs.ArContainsCode;
  //ArContainsMap = rhs.ArContainsMap;
  //ArRequiresLocalizationGather = rhs.ArRequiresLocalizationGather;
  ArForceByteSwapping = rhs.ArForceByteSwapping;
  //ArSerializingDefaults = rhs.ArSerializingDefaults;
  //ArIgnoreArchetypeRef = rhs.ArIgnoreArchetypeRef;
  //ArNoDelta = rhs.ArNoDelta;
  //ArIgnoreOuterRef = rhs.ArIgnoreOuterRef;
  //ArIgnoreClassGeneratedByRef = rhs.ArIgnoreClassGeneratedByRef;
  //ArIgnoreClassRef = rhs.ArIgnoreClassRef;
  //ArAllowLazyLoading = rhs.ArAllowLazyLoading;
  //ArIsObjectReferenceCollector = rhs.ArIsObjectReferenceCollector;
  //ArIsModifyingWeakAndStrongReferences = rhs.ArIsModifyingWeakAndStrongReferences;
  ArIsCountingMemory = rhs.ArIsCountingMemory;
  //ArPortFlags = rhs.ArPortFlags;
  //ArShouldSkipBulkData = rhs.ArShouldSkipBulkData;
  ArMaxSerializeSize = rhs.ArMaxSerializeSize;
  //ArIsFilterEditorOnly = rhs.ArIsFilterEditorOnly;
  //ArIsSaveGame = rhs.ArIsSaveGame;
  //CookingTargetPlatform = rhs.CookingTargetPlatform;
  //SerializedProperty = rhs.SerializedProperty;

//#if WITH_EDITORONLY_DATA
//  EditorOnlyPropertyStack = rhs.EditorOnlyPropertyStack;
//#endif
  */
}

String Archive::GetArchiveName() const {
  return "Archive";
}

//#if FUN_WITH_EDITOR
//Archive::ScopeAddDebugData::ScopeAddDebugData(Archive& InAr, const Name& debug_data)
//  : ar(InAr)
//{
//  ar.PushDebugDataString(debug_data);
//}
//
//void Archive::PushDebugDataString(const Name& debug_data)
//{
//}
//#endif

//Archive& Archive::operator & (class LazyObjectPtr& lazy_object_ptr)
//{
//  // The base Archive does not implement this method. Use ArchiveFObject instead.
//  fun_log(LogSerialization, Fatal, "Archive does not support LazyObjectPtr serialization. Use CArchiveUObject instead.");
//  return *this;
//}
//
//Archive& Archive::operator & (class AssetPtr& asset_ptr)
//{
//  // The base Archive does not implement this method. Use ArchiveFObject instead.
//  fun_log(LogSerialization, Fatal, "Archive does not support AssetPtr serialization. Use CArchiveUObject instead.");
//  return *this;
//}
//
//Archive& Archive::operator & (struct StringAssetReference& value)
//{
//  // The base Archive does not implement this method. Use ArchiveFObject instead.
//  fun_log(LogSerialization, Fatal, "Archive does not support AssetPtr serialization. Use CArchiveUObject instead.");
//  return *this;
//}
//
const CustomVersionContainer& Archive::GetCustomVersions() const {
  if (custom_versions_are_reset_) {
    custom_versions_are_reset_ = false;

    // If the archive is for reading then we want to use currently registered custom versions, otherwise we expect
    // serialization code to use UsingCustomVersion to populate the container.
    if (IsLoading()) {
      *custom_version_container_ = CustomVersionContainer::GetRegistered();
    } else {
      custom_version_container_->Clear();
    }
  }

  return *custom_version_container_;
}

void Archive::SetCustomVersions(const CustomVersionContainer& new_versions) {
  *custom_version_container_ = new_versions;
  custom_versions_are_reset_ = false;
}

void Archive::ResetCustomVersions() {
  custom_versions_are_reset_ = true;
}

void Archive::UsingCustomVersion(const Uuid& key) {
  // If we're loading, we want to use the version that the archive
  // was serialized with, not register a new one.
  if (IsLoading()) {
    return;
  }

  auto* registered_version = CustomVersionContainer::GetRegistered().GetVersion(key);

  // Ensure that the version has already been registered.
  // If this fails, you probably don't have an CustomVersionRegistration variable defined for this GUID.
  fun_check_ptr(registered_version);

  const_cast<CustomVersionContainer&>(GetCustomVersions()).SetVersion(key, registered_version->version, registered_version->GetFriendlyName());
}

int32 Archive::CustomVer(const Uuid& key) const {
  auto* custom_version = GetCustomVersions().GetVersion(key);

  // If this fails, you have forgotten to make an ar.UsingCustomVersion call
  // before serializing your custom version-dependent object.
  fun_check(IsLoading() || custom_version != nullptr);

  return custom_version != nullptr ? custom_version->version : -1;
}

void Archive::SetCustomVersion(const Uuid& key, int32 version, const String& friendly_name) {
  const_cast<CustomVersionContainer&>(GetCustomVersions()).SetVersion(key, version, friendly_name);
}

String ArchiveProxy::GetArchiveName() const {
  return inner_archive_.GetArchiveName();
}

//TODO
//Archive& NameAsStringProxyArchive::operator & (Name& name) {
//  if (IsLoading()) {
//    String loaded_string;
//    inner_archive_ & loaded_string;
//    name = Name(*loaded_string);
//  } else {
//    String saved_string(name.ToString());
//    inner_archive_ & saved_string;
//  }
//  return *this;
//}

//Archive& operator & (Archive& ar, CompressedChunkInfo& chunk) {
//  // The order of serialization needs to be identical to
//  // the memory layout as the async IO code is reading it in bulk.
//  // The size of the structure also needs to match what is being serialized.
//  return ar & chunk.compressed_size & chunk.uncompressed_size;
//}

/** Accumulative time spent in IsSaving portion of SerializeCompressed. */
FUN_BASE_API double GArchiveSerializedCompressedSavingTime = 0;

// MT compression disabled on console due to memory impact
// and lack of beneficial usage case.
#define WITH_MULTI_THREADED_COMPRESSION  (WITH_EDITORONLY_DATA)
#if WITH_MULTI_THREADED_COMPRESSION
// Helper structure to keep information about async chunks that are in-flight.
class AsyncCompressionChunk : public NonAbandonableTask {
 public:
  /** Pointer to source (uncompressed) memory. */
  void* uncompressed_buffer;

  /** Pointer to destination (compressed) memory. */
  void* compressed_buffer;

  /** Compressed size in bytes as passed to/ returned from compressor. */
  int32 compressed_size;

  /** Uncompressed size in bytes as passed to compressor. */
  int32 uncompressed_size;

  /** flags to control compression */
  CompressionFlags flags;

  /** Constructor, zeros everything */
  AsyncCompressionChunk()
    : uncompressed_buffer(0)
    , compressed_buffer(0)
    , compressed_size(0)
    , uncompressed_size(0)
    , flags(compression_flags_(0)) {}

  /** Performs the async compression */
  void DoWork() {
    // Compress from memory to memory.
    fun_verify(Compression::CompressMemory(flags, compressed_buffer, compressed_size, uncompressed_buffer, uncompressed_size));
  }

  ALWAYS_INLINE TStatId GetStatId() const {
    RETURN_QUICK_DECLARE_CYCLE_STAT(AsyncCompressionChunk, STATGROUP_ThreadPoolAsyncTasks);
  }
};
#endif //WITH_MULTI_THREADED_COMPRESSION

void Archive::SerializeCompressed(void* v, int64 length, CompressionFlags flags, bool treat_buffer_as_file_reader) {
  //TODO
  /*
  if (IsLoading()) {
    // Serialize package file tag used to determine endianess.
    CompressedChunkInfo package_file_tag;
    package_file_tag.compressed_size = 0;
    package_file_tag.uncompressed_size = 0;
    *this & package_file_tag;
    bool was_byte_swapped = package_file_tag.compressed_size != PACKAGE_FILE_TAG;

    // Read in base summary.
    CompressedChunkInfo summary;
    *this & summary;

    if (was_byte_swapped) {
      fun_check(package_file_tag.compressed_size   == PACKAGE_FILE_TAG_SWAPPED);
      summary.compressed_size = ByteOrder::Flip(summary.compressed_size);
      summary.uncompressed_size = ByteOrder::Flip(summary.uncompressed_size);
      package_file_tag.uncompressed_size = ByteOrder::Flip(package_file_tag.uncompressed_size);
    } else {
      fun_check(package_file_tag.compressed_size   == PACKAGE_FILE_TAG);
    }

    // Handle change in compression chunk size in backward compatible way.
    int64 loading_compression_chunk_size = package_file_tag.uncompressed_size;
    if (loading_compression_chunk_size == PACKAGE_FILE_TAG) {
      loading_compression_chunk_size = CompressionConstants::LOADING_COMPRESSION_CHUNK_SIZE;
    }

    // Figure out how many chunks there are going to be based on uncompressed size and compression chunk size.
    int64 total_chunk_count = (summary.uncompressed_size + loading_compression_chunk_size - 1) / loading_compression_chunk_size;

    // Allocate compression chunk infos and serialize them, keeping track of max size of compression chunks used.
    CompressedChunkInfo* compression_chunks = new CompressedChunkInfo[total_chunk_count];
    int64 max_compressed_size = 0;
    for (int32 chunk_index = 0; chunk_index < total_chunk_count; chunk_index++) {
      *this & compression_chunks[chunk_index];
      if (was_byte_swapped) {
        compression_chunks[chunk_index].compressed_size = ByteOrder::Flip(compression_chunks[chunk_index].compressed_size);
        compression_chunks[chunk_index].uncompressed_size = ByteOrder::Flip(compression_chunks[chunk_index].uncompressed_size);
      }
      max_compressed_size = MathBase::Max(compression_chunks[chunk_index].compressed_size, max_compressed_size);
    }

    int64 padding = 0;

    // Set up destination pointer and allocate memory for compressed chunk[s] (one at a time).
    uint8* dst = (uint8*)v;
    void* compressed_buffer = UnsafeMemory::Malloc(max_compressed_size + padding);

    // Iterate over all chunks, serialize them into memory and decompress them directly into the destination pointer
    for (int64 chunk_index = 0; chunk_index < total_chunk_count; chunk_index++) {
      const CompressedChunkInfo& chunk = compression_chunks[chunk_index];
      // Read compressed data.
      Serialize(compressed_buffer, chunk.compressed_size);
      // Decompress into dst pointer directly.
      fun_verify(Compression::UncompressMemory(flags, dst, chunk.uncompressed_size, compressed_buffer, chunk.compressed_size, (padding > 0) ? true : false));
      // And advance it by read amount.
      dst += chunk.uncompressed_size;
    }

    // Free up allocated memory.
    UnsafeMemory::Free(compressed_buffer);
    delete[] compression_chunks;
  } else if (IsSaving()) {
    SCOPED_SECONDS_COUNTER(GArchiveSerializedCompressedSavingTime);
    fun_check(length > 0);

    // Serialize package file tag used to determine endianess in LoadCompressedData.
    CompressedChunkInfo package_file_tag;
    package_file_tag.compressed_size = PACKAGE_FILE_TAG;
    package_file_tag.uncompressed_size = GSavingCompressionChunkSize;
    *this & package_file_tag;

    // Figure out how many chunks there are going to be based on uncompressed size and compression chunk size.
    int64 total_chunk_count = (length + GSavingCompressionChunkSize - 1) / GSavingCompressionChunkSize + 1;

    // Keep track of current position so we can later seek back and overwrite stub compression chunk infos.
    int64 start_pos = Tell();

    // Allocate compression chunk infos and serialize them so we can later overwrite the data.
    CompressedChunkInfo* compression_chunks = new CompressedChunkInfo[total_chunk_count];
    for (int64 chunk_index = 0; chunk_index < total_chunk_count; chunk_index++) {
      *this & compression_chunks[chunk_index];
    }

    // The uncompressd size is equal to the passed in length.
    compression_chunks[0].uncompressed_size = length;
    // Zero initialize compressed size so we can update it during chunk compression.
    compression_chunks[0].compressed_size = 0;

#if WITH_MULTI_THREADED_COMPRESSION

  #define MAX_COMPRESSION_JOBS  (16)
    // Don't scale more than 16x to avoid going overboard wrt temporary memory.
    CAsyncTask<AsyncCompressionChunk> async_chunks[MAX_COMPRESSION_JOBS];

    // used to keep track of which job is the next one we need to retire
    int32 async_chunk_index[MAX_COMPRESSION_JOBS] = {0};

    static uint32 GNumUnusedThreads_SerializeCompressed = -1;
    if (GNumUnusedThreads_SerializeCompressed == (uint32)-1) {
      // one-time initialization
      GNumUnusedThreads_SerializeCompressed = 1;
      // if we should use all available cores then we want to compress with all
      if (Parse::Param(CommandLine::Get(), TEXT("USEALLAVAILABLECORES")) == true) {
        GNumUnusedThreads_SerializeCompressed = 0;
      }
    }

    // Maximum number of concurrent async tasks we're going to kick off. This is based on the number of processors
    // available in the system.
    int32 MaxConcurrentAsyncChunks = MathBase::Clamp<int32>(CPlatformMisc::NumberOfCores() - GNumUnusedThreads_SerializeCompressed, 1, MAX_COMPRESSION_JOBS);
    if (Parse::Param(CommandLine::Get(), TEXT("MTCHILD"))) {
      // throttle this back when doing MT cooks
      MaxConcurrentAsyncChunks = MathBase::Min<int32>(MaxConcurrentAsyncChunks,4);
    }

    // Number of chunks left to finalize.
    int64 num_chunks_left_to_finalize = (length + GSavingCompressionChunkSize - 1) / GSavingCompressionChunkSize;
    // Number of chunks left to kick off
    int64 num_chunks_left_to_kick_off = num_chunks_left_to_finalize;
    // Start at index 1 as first chunk info is summary.
    int64 current_chunk_index = 1;
    // Start at index 1 as first chunk info is summary.
    int64 retire_chunk_index = 1;

    // Number of bytes remaining to kick off compression for.
    int64 byte_remaining_to_kick_off = length;
    // Pointer to src data if buffer is memory pointer, NULL if it's a Archive.
    uint8* SrcBuffer = treat_buffer_as_file_reader ? nullptr : (uint8*)v;

    fun_check(!treat_buffer_as_file_reader || ((Archive*)v)->IsLoading());
    fun_check(num_chunks_left_to_finalize > 0);

    // Loop while there is work left to do based on whether we have finalized all chunks yet.
    while (num_chunks_left_to_finalize) {
      // If true we are waiting for async tasks to complete and should wait to complete some
      // if there are no async tasks finishing this iteration.
      bool need_to_wait_for_async_task = false;

      // Try to kick off async tasks if there are chunks left to kick off.
      if (num_chunks_left_to_kick_off > 0) {
        // Find free index based on looking at uncompressed size. We can't use the thread counter
        // for this as that might be a chunk ready for finalization.
        int32 free_index = INVALID_INDEX;
        for (int32 i = 0; i < MaxConcurrentAsyncChunks; ++i) {
          if (!async_chunk_index[i]) {
            free_index = i;
            fun_check(async_chunks[free_index].IsIdle()); // this is not supposed to be in use
            break;
          }
        }

        // Kick off async compression task if we found a chunk for it.
        if (free_index != INVALID_INDEX) {
          AsyncCompressionChunk& new_chunk = async_chunks[free_index].GetTask();
          // 2 times the uncompressed size should be more than enough; the compressed data shouldn't be that much larger
          new_chunk.compressed_size = 2 * GSavingCompressionChunkSize;
          // Allocate compressed buffer placeholder on first use.
          if (new_chunk.compressed_buffer == nullptr) {
            new_chunk.compressed_buffer = UnsafeMemory::Malloc(new_chunk.compressed_size);
          }

          // By default everything is chunked up into GSavingCompressionChunkSize chunks.
          new_chunk.uncompressed_size   = MathBase::Min(byte_remaining_to_kick_off, (int64)GSavingCompressionChunkSize);
          fun_check(new_chunk.uncompressed_size > 0);

          // Need to serialize source data if passed in pointer is an Archive.
          if (treat_buffer_as_file_reader) {
            // Allocate memory on first use. We allocate the maximum amount to allow reuse.
            if (new_chunk.uncompressed_buffer == nullptr) {
              new_chunk.uncompressed_buffer = UnsafeMemory::Malloc(GSavingCompressionChunkSize);
            }
            ((Archive*)v)->Serialize(new_chunk.uncompressed_buffer, new_chunk.uncompressed_size);
          }
          // Advance src pointer by amount to be compressed.
          else {
            new_chunk.uncompressed_buffer = SrcBuffer;
            SrcBuffer += new_chunk.uncompressed_size;
          }

          // Update status variables for tracking how much work is left, what to do next.
          byte_remaining_to_kick_off -= new_chunk.uncompressed_size;
          async_chunk_index[free_index] = current_chunk_index++;
          new_chunk.flags = flags;
          num_chunks_left_to_kick_off--;

          async_chunks[free_index].StartBackgroundTask();
        }
        // No chunks were available to use, complete some
        else {
          need_to_wait_for_async_task = true;
        }
      }

      // Index of oldest chunk, needed as we need to serialize in order.
      int32 oldest_async_chunk_index = INVALID_INDEX;
      for (int32 i = 0; i < MaxConcurrentAsyncChunks; ++i) {
        fun_check(async_chunk_index[i] == 0 || async_chunk_index[i] >= retire_chunk_index);
        fun_check(async_chunk_index[i] < retire_chunk_index + MaxConcurrentAsyncChunks);
        if (async_chunk_index[i] == retire_chunk_index) {
          oldest_async_chunk_index = i;
        }
      }
      fun_check(oldest_async_chunk_index != INVALID_INDEX);  // the retire chunk better be outstanding

      bool is_chunk_ready;
      if (need_to_wait_for_async_task) {
        // This guarantees that the async work has finished, doing it on this thread if it hasn't been started
        async_chunks[oldest_async_chunk_index].EnsureCompletion();
        is_chunk_ready = true;
      } else {
        is_chunk_ready = async_chunks[oldest_async_chunk_index].IsDone();
      }
      if (is_chunk_ready) {
        AsyncCompressionChunk& done_chunk = async_chunks[oldest_async_chunk_index].GetTask();
        // Serialize the data via archive.
        Serialize(done_chunk.compressed_buffer, done_chunk.compressed_size);

        // Update associated chunk.
        int64 compression_chunk_index = retire_chunk_index++;
        fun_check(compression_chunk_index < total_chunk_count);
        compression_chunks[compression_chunk_index].compressed_size = done_chunk.compressed_size;
        compression_chunks[compression_chunk_index].uncompressed_size = done_chunk.uncompressed_size;

        // Keep track of total compressed size, stored in first chunk.
        compression_chunks[0].compressed_size += done_chunk.compressed_size;

        // Clean up chunk. src and dst buffer are not touched as the contain allocations we keep till the end.
        async_chunk_index[oldest_async_chunk_index] = 0;
        done_chunk.compressed_size = 0;
        done_chunk.uncompressed_size = 0;

        // Finalized one :)
        num_chunks_left_to_finalize--;
        need_to_wait_for_async_task = false;
      }
    }

    // Free intermediate buffer storage.
    for (int32 i = 0; i < MaxConcurrentAsyncChunks; ++i) {
      // Free temporary compressed buffer storage.
      UnsafeMemory::Free(async_chunks[i].GetTask().compressed_buffer);
      async_chunks[i].GetTask().compressed_buffer = nullptr;
      // Free temporary uncompressed buffer storage if data was serialized in.
      if (treat_buffer_as_file_reader) {
        UnsafeMemory::Free(async_chunks[i].GetTask().uncompressed_buffer);
        async_chunks[i].GetTask().uncompressed_buffer = nullptr;
      }
    }

#else

    // Set up source pointer amount of data to copy (in bytes)
    uint8*  src;
    // allocate memory to read into
    if (treat_buffer_as_file_reader) {
      src = (uint8*)UnsafeMemory::Malloc(GSavingCompressionChunkSize);
      fun_check(((Archive*)v)->IsLoading());
    } else {
      src = (uint8*)v;
    }
    int64 bytes_remaining = length;
    // Start at index 1 as first chunk info is summary.
    int32 current_chunk_index = 1;
    // 2 times the uncompressed size should be more than enough; the compressed data shouldn't be that much larger
    int64 compressed_buffer_size = 2 * GSavingCompressionChunkSize;
    void* compressed_buffer = UnsafeMemory::Malloc(compressed_buffer_size);

    while (bytes_remaining > 0) {
      int64 bytes_to_compress = MathBase::Min(bytes_remaining, (int64)GSavingCompressionChunkSize);
      int64 compressed_size = compressed_buffer_size;

      // read in the next chunk from the reader
      if (treat_buffer_as_file_reader) {
        ((Archive*)v)->Serialize(src, bytes_to_compress);
      }

      fun_check(compressed_size < int32_MAX);
      int32 compressed_size_int = (int32)compressed_size;
      fun_verify(Compression::CompressMemory(flags, compressed_buffer, compressed_size_int, src, bytes_to_compress));
      compressed_size = compressed_size_int;
      // move to next chunk if not reading from file
      if (!treat_buffer_as_file_reader) {
        src += bytes_to_compress;
      }
      Serialize(compressed_buffer, compressed_size);
      // Keep track of total compressed size, stored in first chunk.
      compression_chunks[0].compressed_size += compressed_size;

      // Update current chunk.
      fun_check(current_chunk_index < total_chunk_count);
      compression_chunks[current_chunk_index].compressed_size = compressed_size;
      compression_chunks[current_chunk_index].uncompressed_size = bytes_to_compress;
      current_chunk_index++;

      bytes_remaining -= GSavingCompressionChunkSize;
    }

    // free the buffer we read into
    if (treat_buffer_as_file_reader) {
      UnsafeMemory::Free(src);
    }

    // Free allocated memory.
    UnsafeMemory::Free(compressed_buffer);
#endif

    // Overrwrite chunk infos by seeking to the beginning, serializing the data and then
    // seeking back to the end.
    auto end_pos = Tell();
    // Seek to the beginning.
    Seek(start_pos);
    // Serialize chunk infos.
    for (int32 chunk_index = 0; chunk_index < total_chunk_count; chunk_index++) {
      *this & compression_chunks[chunk_index];
    }
    // Seek back to end.
    Seek(end_pos);

    // Free intermediate data.
    delete[] compression_chunks;
  }
  */
}

void Archive::ByteSwap(void* v, int64 len) {
  uint8* ptr = (uint8*)v;
  int32 top = len - 1;
  int32 bottom = 0;
  while (bottom < top) {
    Swap(ptr[top--], ptr[bottom++]);
  }
}

void Archive::SerializeIntPacked(uint32& value) {
  if (IsLoading()) {
    value = 0;
    uint8 count = 0;
    uint8 more = 1;
    while (more) {
      uint8 next_byte;
      Serialize(&next_byte, 1);             // Read next byte

      more = next_byte & 1;                 // Check 1 bit to see if theres more after this
      next_byte = next_byte >> 1;           // Shift to get actual 7 bit value
      value += next_byte << (7 * count++);  // Add to total value
    }
  } else {
    Array<uint8> packed_bytes;
    uint32 remaining = value;
    while (true) {
      uint8 next_byte = remaining & 0x7f;   // Get next 7 bits to write
      remaining = remaining >> 7;           // Update remaining
      next_byte = next_byte << 1;           // Make room for 'more' bit
      if (remaining > 0) {
        next_byte |= 1;                     // Set more bit
        packed_bytes.Add(next_byte);
      } else {
        packed_bytes.Add(next_byte);
        break;
      }
    }

    Serialize(packed_bytes.MutableData(), packed_bytes.Count()); // Actually serialize the bytes we made
  }
}

/*
//TODO("would like UTF8 format rather than ANSI when saving.");
VARARG_BODY(void, Archive::Printf, const TCHAR*, VARARG_NONE) {
  // We need to use malloc here directly as GMalloc might not be safe, e.g. if called from within GMalloc!
  int32 BufferSize  = 1024;
  TCHAR* Buffer = nullptr;
  int32 Result = -1;

  while (Result == -1) {
    UnsafeMemory::SystemFree(Buffer);
    Buffer = (TCHAR*)UnsafeMemory::SystemMalloc(BufferSize * sizeof(TCHAR));
    GET_VARARGS_RESULT(Buffer, BufferSize, BufferSize-1, Fmt, Fmt, Result);
    BufferSize *= 2;
  };
  Buffer[Result] = 0;

  // Convert to ANSI and serialize as ANSI char.
  for (int32 i = 0; i < Result; ++i) {
    ANSICHAR Char = CharCast<ANSICHAR>(Buffer[i]);
    Serialize(&Char, 1);
  }

  // Write out line terminator.
  for (int32 i = 0; FUN_LINE_TERMINATOR[i]; ++i) {
    ANSICHAR Char = FUN_LINE_TERMINATOR[i];
    Serialize(&Char, 1);
  }

  // Free temporary buffers.
  UnsafeMemory::SystemFree(Buffer);
}
*/


//
// Transparent compression/ decompression archives.
//

ArchiveSaveCompressedProxy::ArchiveSaveCompressedProxy(
      Array<uint8>& compressed_data,
      CompressionFlags compression_flags)
  : compressed_data_(compressed_data)
  , compression_flags_(compression_flags) {
  SetPersistent();
  SetSaving();

  //TODO
  //ArWantBinaryPropertySerialization = true;

  should_serialize_to_array_ = false;
  raw_bytes_serialized_ = 0;
  current_index_ = 0;

  // Allocate temporary memory.
  tmp_data_start_ = (uint8*)UnsafeMemory::Malloc(CompressionConstants::LOADING_COMPRESSION_CHUNK_SIZE);
  tmp_data_end_ = tmp_data_start_ + CompressionConstants::LOADING_COMPRESSION_CHUNK_SIZE;
  tmp_data_ = tmp_data_start_;
}

ArchiveSaveCompressedProxy::~ArchiveSaveCompressedProxy() {
  // Flush is required to write out remaining tmp data to array.
  Flush();

  // Free temporary memory allocated.
  UnsafeMemory::Free(tmp_data_start_);
  tmp_data_start_ = nullptr;
  tmp_data_end_ = nullptr;
  tmp_data_ = nullptr;
}

void ArchiveSaveCompressedProxy::Flush() {
  if ((tmp_data_ - tmp_data_start_) > 0) {
    // This will call Serialize so we need to indicate that we want to serialize to array.
    should_serialize_to_array_ = true;
    SerializeCompressed(tmp_data_start_, tmp_data_ - tmp_data_start_, compression_flags_);
    should_serialize_to_array_ = false;
    // Buffer is drained, reset.
    tmp_data_ = tmp_data_start_;
  }
}

void ArchiveSaveCompressedProxy::Serialize(void* v, int64 len) {
  uint8* src_data = (uint8*)v;

  // If counter > 1 it means we're calling recursively
  // and therefore need to write to compressed data.
  if (should_serialize_to_array_) {
    // Add space in array if needed and copy data there.
    const int32 len_to_add = current_index_ + len - compressed_data_.Count();
    if (len_to_add > 0) {
      compressed_data_.AddUninitialized(len_to_add);
    }
    // Copy memory to array.
    UnsafeMemory::Memcpy(&compressed_data_[current_index_], src_data, len);
    current_index_ += len;
  }
  // Regular call to serialize, queue for compression.
  else {
    while (len) {
      const int32 len_to_copy = MathBase::Min<int32>(len, (int32)(tmp_data_end_ - tmp_data_));
      // Enough room in buffer to copy some data.
      if (len_to_copy) {
        UnsafeMemory::Memcpy(tmp_data_, src_data, len_to_copy);
        len -= len_to_copy;
        tmp_data_ += len_to_copy;
        src_data += len_to_copy;
        raw_bytes_serialized_ += len_to_copy;
      }
      // Tmp buffer fully exhausted, compress it.
      else {
        // Flush existing data to array after compressing it. This will call Serialize again
        // so we need to handle recursion.
        Flush();
      }
    }
  }
}

void ArchiveSaveCompressedProxy::Seek(int64 pos) {
  // Support setting position in array.
  if (should_serialize_to_array_) {
    current_index_ = pos;
  } else {
    //TODO
    //fun_log(Serialization, Fatal, "Seeking not supported with ArchiveSaveCompressedProxy");
  }
}

int64 ArchiveSaveCompressedProxy::Tell() {
  // If we're serializing to array, return position in array.
  if (should_serialize_to_array_) {
    return current_index_;
  }
  // Return global position in raw uncompressed stream.
  else {
    return raw_bytes_serialized_;
  }
}


//
// ArchiveLoadCompressedProxy
//

ArchiveLoadCompressedProxy::ArchiveLoadCompressedProxy(
      const Array<uint8>& compressed_data,
      CompressionFlags compression_flags)
  : compressed_data_(compressed_data)
  , compression_flags_(compression_flags) {
  SetPersistent();
  SetLoading();

  //TODO?
  //ArWantBinaryPropertySerialization = true;

  should_serialize_from_array_ = false;
  raw_bytes_serialized_ = 0;
  current_index_ = 0;

  // Allocate temporary memory.
  tmp_data_start_ = (uint8*)UnsafeMemory::Malloc(CompressionConstants::LOADING_COMPRESSION_CHUNK_SIZE);
  tmp_data_end_ = tmp_data_start_ + CompressionConstants::LOADING_COMPRESSION_CHUNK_SIZE;
  tmp_data_ = tmp_data_end_;
}

ArchiveLoadCompressedProxy::~ArchiveLoadCompressedProxy() {
  // Free temporary memory allocated.
  UnsafeMemory::Free(tmp_data_start_);
  tmp_data_start_ = nullptr;
  tmp_data_end_ = nullptr;
  tmp_data_ = nullptr;
}

void ArchiveLoadCompressedProxy::DecompressMoreData() {
  // This will call Serialize so we need to indicate that we want to serialize from array.
  should_serialize_from_array_ = true;
  SerializeCompressed(tmp_data_start_, CompressionConstants::LOADING_COMPRESSION_CHUNK_SIZE /** it's ignored, but that's how much we serialize */, compression_flags_);
  should_serialize_from_array_ = false;
  // Buffer is filled again, reset.
  tmp_data_ = tmp_data_start_;
}

void ArchiveLoadCompressedProxy::Serialize(void* v, int64 len) {
  uint8* dst_data = (uint8*)v;

  // If counter > 1 it means we're calling recursively and
  // therefore need to write to compressed data.
  if (should_serialize_from_array_) {
    // Add space in array and copy data there.
    fun_check(current_index_ + len <= compressed_data_.Count());
    UnsafeMemory::Memcpy(dst_data, &compressed_data_[current_index_], len);
    current_index_ += len;
  }
  // Regular call to serialize, read from temp buffer
  else {
    while (len) {
      const int32 len_to_copy = MathBase::Min<int32>(len, (int32)(tmp_data_end_ - tmp_data_));
      // Enough room in buffer to copy some data.
      if (len_to_copy) {
        // We pass in a NULL pointer when forward seeking. In that case we don't want
        // to copy the data but only care about pointing to the proper spot.
        if (dst_data) {
          UnsafeMemory::Memcpy(dst_data, tmp_data_, len_to_copy);
          dst_data += len_to_copy;
        }
        len -= len_to_copy;
        tmp_data_ += len_to_copy;
        raw_bytes_serialized_ += len_to_copy;
      }
      // Tmp buffer fully exhausted, decompress new one.
      else {
        // Decompress more data. This will call Serialize again so we need to handle recursion.
        DecompressMoreData();
      }
    }
  }
}

void ArchiveLoadCompressedProxy::Seek(int64 pos) {
  const int64 cur_pos = Tell();
  const int64 diff = pos - cur_pos;
  // We only support forward seeking.
  fun_check(diff >= 0);
  // Seek by serializing data, albeit with NULL destination so it's just decompressing data.
  Serialize(nullptr, diff);
}

int64 ArchiveLoadCompressedProxy::Tell() {
  return raw_bytes_serialized_;
}

} // namespace fun
