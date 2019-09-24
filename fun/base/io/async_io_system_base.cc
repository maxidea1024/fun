#include "Async/AsyncWork.h"
#include "CorePrivatePCH.h"
#include "HAL/PlatformFSManager.h"
#include "Serialization/AsyncIoSystemBase.h"
#include "Version/ObjectVersion.h"

FUN_BEGIN_NAMESPACE

DECLARE_STATS_GROUP_VERBOSE("AsyncIoSystem", STATGROUP_AsyncIO_Verbose,
                            STATCAT_Advanced);

DECLARE_CYCLE_STAT("Platform read time", STAT_AsyncIO_PlatformReadTime,
                   STATGROUP_AsyncIO);

DECLARE_DWORD_ACCUMULATOR_STAT("Fulfilled read count",
                               STAT_AsyncIO_FulfilledReadCount,
                               STATGROUP_AsyncIO);
DECLARE_DWORD_ACCUMULATOR_STAT("Canceled read count",
                               STAT_AsyncIO_CanceledReadCount,
                               STATGROUP_AsyncIO);

DECLARE_MEMORY_STAT("Fulfilled read size", STAT_AsyncIO_FulfilledReadSize,
                    STATGROUP_AsyncIO);
DECLARE_MEMORY_STAT("Canceled read size", STAT_AsyncIO_CanceledReadSize,
                    STATGROUP_AsyncIO);
DECLARE_MEMORY_STAT("Outstanding read size", STAT_AsyncIO_OutstandingReadSize,
                    STATGROUP_AsyncIO);

DECLARE_DWORD_ACCUMULATOR_STAT("Outstanding read count",
                               STAT_AsyncIO_OutstandingReadCount,
                               STATGROUP_AsyncIO);
DECLARE_FLOAT_ACCUMULATOR_STAT("uncompressor wait time",
                               STAT_AsyncIO_UncompressorWaitTime,
                               STATGROUP_AsyncIO);

DECLARE_FLOAT_COUNTER_STAT("Bandwidth (MByte/ sec)", STAT_AsyncIO_Bandwidth,
                           STATGROUP_AsyncIO);

//
// AsyncIoSystemBase implementation.
//

#define FUN_BLOCK_ON_ASYNC_IO 0

// Constrain bandwidth if wanted. Value is in MByte/ sec.
float g_async_io_bandwidth_limit = 0.0f;
static CAutoConsoleVariableRef CVarAsyncIOBandwidthLimit(
    "s.AsyncIOBandwidthLimit", g_async_io_bandwidth_limit,
    "Constrain bandwidth if wanted. Value is in MByte/ sec.", ECVF_Default);

FUN_BASE_API bool g_log_async_loading = false;

uint64 AsyncIoSystemBase::QueueIoRequest(const String& filename, int64 offset,
                                         int64 compressed_size,
                                         int64 uncompressed_size, void* dst,
                                         CompressionFlags compression_flags,
                                         AtomicCounter32* counter,
                                         AsyncIoPriority priority) {
  fun_check(offset != INVALID_INDEX);
  fun_check(dst != nullptr || compressed_size == 0);

  CScopedLock guard(*mutex_);

  // Create an IO request containing passed in information.
  AsyncIoRequest request;
  request.request_index = next_request_index_++;
  request.find_sort_key = INVALID_INDEX;
  request.filename = filename;
  request.filename_hash = Crc::StringCrc32<char>(*filename.ToLower());
  request.offset = offset;
  request.size = compressed_size;
  request.uncompressed_size = uncompressed_size;
  request.dst = dst;
  request.compression_flags = compression_flags;
  request.counter = counter;
  request.priority = priority;

  static bool HasCheckedCommandline = false;
  if (!HasCheckedCommandline) {
    HasCheckedCommandline = true;
    if (Parse::Param(CommandLine::Get(), "logasync")) {
      g_log_async_loading = true;
      fun_log2(Streaming, Warning, "*** ASYNC LOGGING IS ENABLED");
    }
  }

  if (g_log_async_loading == true) {
    LogIoRequest("QueueIoRequest", request);
  }

  INC_DWORD_STAT(STAT_AsyncIO_OutstandingReadCount);
  INC_DWORD_STAT_BY(STAT_AsyncIO_OutstandingReadSize, request.size);

  // Add to end of queue.
  outstanding_requests_.Add(request);

  // Trigger event telling IO thread to wake up to perform work.
  outstanding_requests_event_->Trigger();

  // Return unique ID associated with request which can be used to cancel it.
  return request.request_index;
}

uint64 AsyncIoSystemBase::QueueDestroyHandleRequest(const String& filename) {
  CScopedLock guard(*mutex_);

  AsyncIoRequest request;
  request.request_index = next_request_index_++;
  request.filename = filename;
  request.filename_hash = Crc::StringCrc32<char>(*filename.ToLower());
  request.priority = AsyncIoPriority::Min;
  request.destroy_handle_request = true;

  if (g_log_async_loading) {
    LogIoRequest("QueueDestroyHandleRequest", request);
  }

  // Add to end of queue.
  outstanding_requests_.Add(request);

  // Trigger event telling IO thread to wake up to perform work.
  outstanding_requests_event_->Trigger();

  // Return unique ID associated with request which can be used to cancel it.
  return request.request_index;
}

void AsyncIoSystemBase::LogIoRequest(const String& message,
                                     const AsyncIoRequest& request) {
  String str = String::Format("ASYNC: %32s: %s\n", message, request.ToString());
  CPlatformMisc::LowLevelOutputDebugString(*str);
}

void AsyncIoSystemBase::InternalRead(IFile* file, int64 offset, int64 size,
                                     void* dst) {
  DECLARE_SCOPED_CYCLE_COUNTER("AsyncIoSystemBase::InternalRead",
                               STAT_AsyncIoSystemBase_InternalRead,
                               STATGROUP_AsyncIO_Verbose);

  CScopedLock guard(*exclusive_read_mutex_);

  STAT(double read_time = 0);
  {
    SCOPED_SECONDS_COUNTER(read_time);
    PlatformReadDoNotCallDirectly(file, offset, size, dst);
  }
  INC_FLOAT_STAT_BY(STAT_AsyncIO_PlatformReadTime, (float)read_time);

  // The platform might actually read more than size
  // due to aligning and internal min read sizes
  // though we only really care about throttling
  // requested bandwidth as it's not very accurate
  // to begin with.
  STAT(ConstrainBandwidth(size, read_time));
}

bool AsyncIoSystemBase::PlatformReadDoNotCallDirectly(IFile* file, int64 offset,
                                                      int64 size, void* dst) {
  fun_check(file);

  if (!file->Seek(offset)) {
    fun_log2(Streaming, Error, "Seek failure.");
    return false;
  }

  if (!file->Read((uint8*)dst, size)) {
    fun_log2(Streaming, Error, "Read failure.");
    return false;
  }

  return true;
}

IFile* AsyncIoSystemBase::PlatformCreateHandle(const char* filename) {
  return low_level_.OpenRead(filename);
}

int32 AsyncIoSystemBase::PlatformGetNextRequestIndex() {
  // Find first index of highest priority request level.
  // Basically FIFO per priority.
  int32 highest_priority_index = INVALID_INDEX;
  AsyncIoPriority highest_priority =
      static_cast<AsyncIoPriority>((int32)AsyncIoPriority::Min - 1);
  for (int32 current_request_index = 0;
       current_request_index < outstanding_requests_.Count();
       ++current_request_index) {
    // Calling code already entered critical section so we can access
    // outstanding_requests_.
    const AsyncIoRequest& request =
        outstanding_requests_[current_request_index];
    if (request.priority > highest_priority) {
      highest_priority = request.priority;
      highest_priority_index = current_request_index;
    }
  }
  return highest_priority_index;
}

void AsyncIoSystemBase::PlatformHandleHintDoneWithFile(const String& filename) {
  QueueDestroyHandleRequest(filename);
}

int64 AsyncIoSystemBase::PlatformMinimumReadSize() { return 32 * 1024; }

// If enabled allows tracking down crashes in decompression as it avoids using
// the async work queue.
#define BLOCK_ON_DECOMPRESSION 0

void AsyncIoSystemBase::FulfillCompressedRead(const AsyncIoRequest& request,
                                              IFile* file) {
  DECLARE_SCOPED_CYCLE_COUNTER("AsyncIoSystemBase::FulfillCompressedRead",
                               STAT_AsyncIoSystemBase_FulfillCompressedRead,
                               STATGROUP_AsyncIO_Verbose);

  if (g_log_async_loading) {
    LogIoRequest("FulfillCompressedRead", request);
  }

  // Initialize variables.
  AsyncUncompress* uncompressor = nullptr;
  uint8* uncompressed_buffer = (uint8*)request.dst;
  // First compression chunk contains information about
  // total size so we skip that one.
  int32 current_chunk_index = 1;
  int32 current_buffer_index = 0;
  bool has_processed_all_data = false;

  // read the first two ints, which will contain the magic bytes
  // (to detect byteswapping)
  // and the original size the chunks were compressed from
  int64 header_data[2];
  int32 header_size = sizeof(header_data);

  InternalRead(file, request.offset, header_size, header_data);
  RETURN_IF_EXIT_REQUESTED;

  // if the magic bytes don't match, then we are byteswapped (or corrupted)
  bool is_byte_swapped = header_data[0] != PACKAGE_FILE_TAG;
  // if its potentially byteswapped, make sure it's not just corrupted
  if (is_byte_swapped) {
    // if it doesn't equal the swapped version, then data is corrupted
    if (header_data[0] != PACKAGE_FILE_TAG_SWAPPED) {
      fun_log2(Streaming, Warning,
               "Detected data corruption [header] trying to read %lld bytes at "
               "offset %lld from '%s'. Please delete file and recook.",
               request.uncompressed_size, request.offset, request.filename);
      fun_check(0);
      CPlatformMisc::HandleIoFailure(request.filename);
    }
    // otherwise, we have a valid byteswapped file, so swap the chunk size
    else {
      header_data[1] = ByteOrder::Flip(header_data[1]);
    }
  }

  int32 compression_chunk_size = header_data[1];

  // handle old packages that don't have the chunk size in the header, in which
  // case we can use the old hardcoded size
  if (compression_chunk_size == PACKAGE_FILE_TAG) {
    compression_chunk_size = LOADING_COMPRESSION_CHUNK_SIZE;
  }

  // calculate the number of chunks based on the size they were compressed from
  int32 total_chunk_count =
      (request.uncompressed_size + compression_chunk_size - 1) /
          compression_chunk_size +
      1;

  // allocate chunk info data based on number of chunks
  CompressedChunkInfo* compression_chunks =
      (CompressedChunkInfo*)UnsafeMemory::Malloc(sizeof(CompressedChunkInfo) *
                                                 total_chunk_count);
  int32 chunk_info_size = (total_chunk_count) * sizeof(CompressedChunkInfo);
  void* compressed_buffer[2] = {0, 0};

  // Read table of compression chunks after seeking to offset (after the initial
  // header data)
  InternalRead(file, request.offset + header_size, chunk_info_size,
               compression_chunks);
  RETURN_IF_EXIT_REQUESTED;

  // Handle byte swapping. This is required for opening a cooked file on the PC.
  int64 calculated_uncompressed_size = 0;
  if (is_byte_swapped) {
    for (int32 chunk_index = 0; chunk_index < total_chunk_count;
         ++chunk_index) {
      compression_chunks[chunk_index].compressed_size =
          ByteOrder::Flip(compression_chunks[chunk_index].compressed_size);
      compression_chunks[chunk_index].uncompressed_size =
          ByteOrder::Flip(compression_chunks[chunk_index].uncompressed_size);
      if (chunk_index > 0) {
        calculated_uncompressed_size +=
            compression_chunks[chunk_index].uncompressed_size;
      }
    }
  } else {
    for (int32 chunk_index = 1; chunk_index < total_chunk_count;
         ++chunk_index) {
      calculated_uncompressed_size +=
          compression_chunks[chunk_index].uncompressed_size;
    }
  }

  if (compression_chunks[0].uncompressed_size != calculated_uncompressed_size) {
    fun_log2(Streaming, Warning,
             "Detected data corruption [incorrect uncompressed size] "
             "calculated %i bytes, requested %i bytes at offset %i from '%s'. "
             "Please delete file and recook.",
             calculated_uncompressed_size, request.uncompressed_size,
             request.offset, request.filename);
    fun_check(0);
    CPlatformMisc::HandleIoFailure(*request.filename);
  }

  if (chunk_info_size + header_size + compression_chunks[0].compressed_size >
      request.size) {
    fun_log2(Streaming, Warning,
             "Detected data corruption [undershoot] trying to read %lld bytes "
             "at offset %lld from '%s'. Please delete file and recook.",
             request.uncompressed_size, request.offset, request.filename);
    fun_check(0);
    CPlatformMisc::HandleIoFailure(*request.filename);
  }

  if (request.uncompressed_size != calculated_uncompressed_size) {
    fun_log2(Streaming, Warning,
             "Detected data corruption [incorrect uncompressed size] "
             "calculated %lld bytes, requested %lld bytes at offset %lld from "
             "'%s'. Please delete file and recook.",
             calculated_uncompressed_size, request.uncompressed_size,
             request.offset, *request.filename);
    fun_check(0);
    CPlatformMisc::HandleIoFailure(*request.filename);
  }

  // Figure out maximum size of compressed data chunk.
  int64 max_compressed_size = 0;
  for (int32 chunk_index = 1; chunk_index < total_chunk_count; ++chunk_index) {
    max_compressed_size = MathBase::Max(
        max_compressed_size, compression_chunks[chunk_index].compressed_size);
    // Verify the all chunks are 'full size' until the last one...
    if (compression_chunks[chunk_index].uncompressed_size <
        compression_chunk_size) {
      if (chunk_index != (total_chunk_count - 1)) {
        fun_check_msg(0,
                      "Calculated too many chunks: %d should be last, there "
                      "are %d from '%s'",
                      chunk_index, total_chunk_count, request.filename);
      }
    }
    fun_check(compression_chunks[chunk_index].uncompressed_size <=
              compression_chunk_size);
  }

  int32 padding = 0;

  // Allocate memory for compressed data.
  compressed_buffer[0] = UnsafeMemory::Malloc(max_compressed_size + padding);
  compressed_buffer[1] = UnsafeMemory::Malloc(max_compressed_size + padding);

  // Initial read request.
  InternalRead(file, file->Tell(),
               compression_chunks[current_chunk_index].compressed_size,
               compressed_buffer[current_buffer_index]);
  RETURN_IF_EXIT_REQUESTED;

  // Loop till we're done decompressing all data.
  while (!has_processed_all_data) {
    AsyncTask<AsyncUncompress> uncompress_task(
        request.compression_flags, uncompressed_buffer,
        compression_chunks[current_chunk_index].uncompressed_size,
        compressed_buffer[current_buffer_index],
        compression_chunks[current_chunk_index].compressed_size, (padding > 0));

#if BLOCK_ON_DECOMPRESSION
    uncompress_task.StartSynchronousTask();
#else
    uncompress_task.StartBackgroundTask();
#endif

    // Advance destination pointer.
    uncompressed_buffer +=
        compression_chunks[current_chunk_index].uncompressed_size;

    // Check whether we are already done reading.
    if (current_chunk_index < total_chunk_count - 1) {
      // Can't postincrement in if statement as we need it to remain at valid
      // value for one more loop iteration to finish the decompression.
      current_chunk_index++;
      // Swap compression buffers to read into.
      current_buffer_index = 1 - current_buffer_index;
      // Read more data.
      InternalRead(file, file->Tell(),
                   compression_chunks[current_chunk_index].compressed_size,
                   compressed_buffer[current_buffer_index]);
      RETURN_IF_EXIT_REQUESTED;
    }
    // We were already done reading the last time around so we are done
    // processing now.
    else {
      has_processed_all_data = true;
    }

    //@todo async loading: should use event for this
    STAT(double uncompressor_wait_time = 0);
    {
      SCOPED_SECONDS_COUNTER(uncompressor_wait_time);
      uncompress_task.EnsureCompletion();  // just decompress on this thread if
                                           // it isn't started yet
    }
    INC_FLOAT_STAT_BY(STAT_AsyncIO_UncompressorWaitTime,
                      (float)uncompressor_wait_time);
  }

  UnsafeMemory::Free(compression_chunks);
  UnsafeMemory::Free(compressed_buffer[0]);
  UnsafeMemory::Free(compressed_buffer[1]);
}

IFile* AsyncIoSystemBase::GetCachedFileHandle(const String& filename) {
  // We can't make any assumptions about NULL being
  // an invalid handle value so we need to use the indirection.
  IFile* file = FindCachedFileHandle(filename);

  // We have an already cached handle, let's use it.
  if (!file) {
    // So let the platform specific code create one.
    file = PlatformCreateHandle(*filename);
    // Make sure it's valid before caching and using it.
    if (file) {
      name_hash_to_handle_map_.Add(Crc::StringCrc32<char>(*filename.ToLower()),
                                   file);
    }
  }

  return file;
}

IFile* AsyncIoSystemBase::FindCachedFileHandle(const String& filename) {
  return FindCachedFileHandle(Crc::StringCrc32<char>(*filename.ToLower()));
}

IFile* AsyncIoSystemBase::FindCachedFileHandle(const uint32 filename_hash) {
  return name_hash_to_handle_map_.FindRef(filename_hash);
}

uint64 AsyncIoSystemBase::LoadData(const String& filename, int64 offset,
                                   int64 size, void* dst,
                                   AtomicCounter32* counter,
                                   AsyncIoPriority priority) {
  uint64 the_request_index;
  {
    the_request_index =
        QueueIoRequest(filename, offset, size, 0, dst, CompressionFlags::None,
                       counter, priority);
  }
#if FUN_BLOCK_ON_ASYNC_IO
  BlockTillAllRequestsFinished();
#endif
  return the_request_index;
}

uint64 AsyncIoSystemBase::LoadCompressedData(const String& filename,
                                             int64 offset, int64 size,
                                             int64 uncompressed_size, void* dst,
                                             CompressionFlags compression_flags,
                                             AtomicCounter32* counter,
                                             AsyncIoPriority priority) {
  uint64 the_request_index;
  {
    the_request_index =
        QueueIoRequest(filename, offset, size, uncompressed_size, dst,
                       compression_flags, counter, priority);
  }
#if FUN_BLOCK_ON_ASYNC_IO
  BlockTillAllRequestsFinished();
#endif
  return the_request_index;
}

int32 AsyncIoSystemBase::CancelRequests(uint64* request_indices,
                                        int32 index_count) {
  CScopedLock guard(*mutex_);

  // Iterate over all outstanding requests and cancel matching ones.
  int32 requests_canceled = 0;
  for (int32 outstanding_index = outstanding_requests_.Count() - 1;
       outstanding_index >= 0 && requests_canceled < index_count;
       --outstanding_index) {
    // Iterate over all indices of requests to cancel
    for (int32 the_request_index = 0; the_request_index < index_count;
         ++the_request_index) {
      // Look for matching request index in queue.
      const AsyncIoRequest request = outstanding_requests_[outstanding_index];
      if (request.request_index == request_indices[the_request_index]) {
        INC_DWORD_STAT(STAT_AsyncIO_CanceledReadCount);
        INC_DWORD_STAT_BY(STAT_AsyncIO_CanceledReadSize, request.size);
        DEC_DWORD_STAT(STAT_AsyncIO_OutstandingReadCount);
        DEC_DWORD_STAT_BY(STAT_AsyncIO_OutstandingReadSize, request.size);
        // Decrement thread-safe counter to indicate that request has been
        // "completed".
        request.counter->Decrement();
        // request variable no longer valid after removal.
        outstanding_requests_.RemoveAt(outstanding_index);
        requests_canceled++;
        // Break out of loop as we've modified outstanding-requests
        // AND it no longer is valid.
        break;
      }
    }
  }
  return requests_canceled;
}

void AsyncIoSystemBase::CancelAllOutstandingRequests() {
  CScopedLock guard(*mutex_);

  // simply toss all outstanding requests - the critical section
  // will guarantee we aren't removing while using elsewhere
  outstanding_requests_.Clear();
}

void AsyncIoSystemBase::ConstrainBandwidth(int64 bytes_read, float read_time) {
  // Constrain bandwidth if wanted. Value is in MByte/ sec.
  if (g_async_io_bandwidth_limit > 0.0f) {
    // Figure out how long to wait to throttle bandwidth.
    float wait_time =
        bytes_read / (g_async_io_bandwidth_limit * 1024.f * 1024.f) - read_time;
    // Only wait if there is something worth waiting for.
    if (wait_time > 0) {
      // Time in seconds to wait.
      CPlatformProcess::Sleep(wait_time);
    }
  }
}

bool AsyncIoSystemBase::InitRunnable() {
  mutex_ = new CCriticalSection();
  exclusive_read_mutex_ = new CCriticalSection();
  outstanding_requests_event_ = CPlatformProcess::GetSynchEventFromPool();
  next_request_index_ = 1;
  min_priority_ = AsyncIoPriority::Min;
  is_running_.Increment();
  return true;
}

void AsyncIoSystemBase::Suspend() {
  suspend_count_.Increment();
  exclusive_read_mutex_->Lock();
}

void AsyncIoSystemBase::Resume() {
  exclusive_read_mutex_->Unlock();
  suspend_count_.Decrement();
}

void AsyncIoSystemBase::SetMinPriority(AsyncIoPriority min_priority) {
  CScopedLock guard(*mutex_);

  // Trigger event telling IO thread to wake up to perform work
  // if we are lowering the min priority.
  if (min_priority < min_priority_) {
    outstanding_requests_event_->Trigger();
  }
  // Update priority.
  min_priority_ = min_priority;
}

void AsyncIoSystemBase::HintDoneWithFile(const String& filename) {
  // let the platform handle it
  PlatformHandleHintDoneWithFile(filename);
}

int64 AsyncIoSystemBase::MinimumReadSize() {
  // let the platform handle it
  return PlatformMinimumReadSize();
}

void AsyncIoSystemBase::ExitRunnable() {
  FlushHandles();
  delete mutex_;
  CPlatformProcess::ReturnSynchEventToPool(outstanding_requests_event_);
  outstanding_requests_event_ = nullptr;
}

void AsyncIoSystemBase::StopRunnable() {
  // Tell the thread to quit.
  is_running_.Decrement();

  // Make sure that the thread awakens even if there is
  // no work currently outstanding.
  outstanding_requests_event_->Trigger();
}

uint32 AsyncIoSystemBase::Run() {
  // is_running_ gets decremented by Stop.
  while (is_running_.GetValue() > 0) {
    // Sit and spin if requested, unless we are shutting down,
    // in which case make sure we don't deadlock.
    while (!GIsRequestingExit && suspend_count_.GetValue() > 0) {
      CPlatformProcess::Sleep(0.005f);
    }

    Tick();
  }

  return 0;
}

void AsyncIoSystemBase::Tick() {
  // Create file handles.
  {
    Array<String> filenames_to_cache_handles;

    // Only enter critical section for copying existing array over.
    // We don't operate on the real array as creating file handles might
    // take a while and we don't want to have other
    // threads stalling on submission of requests.
    {
      CScopedLock guard(*mutex_);

      for (int32 request_idx = 0; request_idx < outstanding_requests_.Count();
           ++request_idx) {
        // Early outs avoid unnecessary work and
        // string copies with implicit allocator churn.
        AsyncIoRequest& outstanding_request =
            outstanding_requests_[request_idx];
        if (outstanding_request.has_already_requested_handle_to_be_cached ==
                false &&
            outstanding_request.destroy_handle_request == false &&
            FindCachedFileHandle(outstanding_request.filename_hash) ==
                nullptr) {
          new (filenames_to_cache_handles) String(outstanding_request.filename);
          outstanding_request.has_already_requested_handle_to_be_cached = true;
        }
      }
    }

    // Create file handles for requests down the pipe.
    // This is done here so we can later on
    // use the handles to figure out the sort keys.
    for (int32 filename_index = 0;
         filename_index < filenames_to_cache_handles.Count();
         ++filename_index) {
      GetCachedFileHandle(filenames_to_cache_handles[filename_index]);
    }
  }

  // Copy of request.
  AsyncIoRequest request;
  bool is_request_pending = false;
  {
    CScopedLock guard(*mutex_);

    if (outstanding_requests_.Count() > 0) {
      // Gets next request index based on platform specific
      // criteria like layout on disc.
      int32 the_request_index = PlatformGetNextRequestIndex();
      if (the_request_index != INVALID_INDEX) {
        // We need to copy as we're going to remove it...
        request = outstanding_requests_[the_request_index];

        // ...right here.
        // NOTE: this needs to be a remove, not a RemoveSwap because
        // the base implementation of PlatformGetNextRequestIndex is
        // a FIFO taking priority into account
        outstanding_requests_.RemoveAt(the_request_index);

        // We're busy. Updated inside scoped lock to ensure
        // BlockTillAllRequestsFinished works correctly.
        busy_with_request_.Increment();
        is_request_pending = true;
      }
    }
  }

  // We only have work to do if there's a request pending.
  if (is_request_pending) {
    // handle a destroy handle request from the queue
    if (request.destroy_handle_request) {
      IFile* file = FindCachedFileHandle(request.filename_hash);
      if (file) {
        // destroy and remove the handle
        delete file;
        name_hash_to_handle_map_.Remove(request.filename_hash);
      }
    } else {
      // Retrieve cached handle or create it if it wasn't cached.
      // We purposefully don't look at currently
      // set value as it might be stale by now.
      IFile* file = GetCachedFileHandle(request.filename);
      if (file) {
        if (request.uncompressed_size > 0) {
          // Data is compressed on disc so we need to also decompress.
          FulfillCompressedRead(request, file);
        } else {
          // Read data after seeking.
          InternalRead(file, request.offset, request.size, request.dst);
        }
        INC_DWORD_STAT(STAT_AsyncIO_FulfilledReadCount);
        INC_DWORD_STAT_BY(STAT_AsyncIO_FulfilledReadSize, request.size);
      } else {
        //@todo streaming: add warning once we have thread safe logging.
      }

      DEC_DWORD_STAT(STAT_AsyncIO_OutstandingReadCount);
      DEC_DWORD_STAT_BY(STAT_AsyncIO_OutstandingReadSize, request.size);
    }

    // Request fulfilled.
    if (request.counter) {
      request.counter->Decrement();
    }

    // We're done reading for now.
    busy_with_request_.Decrement();
  } else {
    if (!outstanding_requests_.Count() &&
        CPlatformProcess::SupportsMultithreading()) {
      // We're really out of requests now, wait till the calling thread signals
      // further work
      outstanding_requests_event_->Wait();
    }
  }
}

void AsyncIoSystemBase::TickSingleThreaded() {
  // This should only be used when multithreading is disabled.
  fun_check(CPlatformProcess::SupportsMultithreading() == false);

  Tick();
}

void AsyncIoSystemBase::BlockTillAllRequestsFinished() {
  DECLARE_SCOPED_CYCLE_COUNTER(
      "AsyncIoSystemBase::BlockTillAllRequestsFinished",
      STAT_AsyncIoSystemBase_BlockTillAllRequestsFinished,
      STATGROUP_AsyncIO_Verbose);

  // Block till all requests are fulfilled.
  while (true) {
    bool has_finished_requests = false;
    {
      CScopedLock guard(*mutex_);
      has_finished_requests = (outstanding_requests_.Count() == 0) &&
                              (busy_with_request_.GetValue() == 0);
    }

    if (has_finished_requests) {
      break;
    } else {
      SHUTDOWN_IF_EXIT_REQUESTED;

      //@todo streaming: this should be replaced by waiting for an event.
      CPlatformProcess::SleepNoStats(0.001f);
    }
  }
}

void AsyncIoSystemBase::BlockTillAllRequestsFinishedAndFlushHandles() {
  // Block till all requests are fulfilled.
  BlockTillAllRequestsFinished();

  // Flush all file handles.
  FlushHandles();
}

void AsyncIoSystemBase::FlushHandles() {
  CScopedLock guard(*mutex_);
  // Iterate over all file handles, destroy them and empty name to handle map.
  for (Map<uint32, IFile*>::Iterator it(name_hash_to_handle_map_); it; ++it) {
    delete it.Value();
  }
  name_hash_to_handle_map_.Clear();
}

/** Thread used for async IO manager */
static RunnableThread* g_async_io_thread = nullptr;
static AsyncIoSystemBase* g_async_io_system = nullptr;

IoSystem& IoSystem::Get() {
  if (!g_async_io_thread) {
    fun_check(!g_async_io_system);
    g_config->GetFloat("Core.System", "AsyncIOBandwidthLimit",
                       g_async_io_bandwidth_limit, GEngineIni);
    g_async_io_system = CPlatformMisc::GetPlatformSpecificAsyncIoSystem();
    if (!g_async_io_system) {
      // the platform didn't have a specific need, so we just use
      // the base class with the normal file system.
      g_async_io_system =
          new AsyncIoSystemBase(CPlatformFSManager::Get().GetPlatformFS());
    }
    g_async_io_thread = RunnableThread::Create(
        g_async_io_system, "AsyncIoSystem", 16384, ThreadPriority::Highest,
        CPlatformAffinity::GetPoolThreadMask());
    fun_check(g_async_io_thread);
  }
  fun_check(g_async_io_system);
  return *g_async_io_system;
}

void IoSystem::Shutdown() {
  if (g_async_io_thread) {
    g_async_io_thread->Kill(true);
    delete g_async_io_thread;
  }

  if (g_async_io_system) {
    delete g_async_io_system;
    g_async_io_system = nullptr;
  }

  // non null, we don't allow a restart after a shutdown
  // as this is usually an error of some sort
  g_async_io_thread = (RunnableThread*)-1;
}

bool IoSystem::HasShutdown() {
  return g_async_io_thread == nullptr ||
         g_async_io_thread == (RunnableThread*)-1;
}

}  // namespace fun
