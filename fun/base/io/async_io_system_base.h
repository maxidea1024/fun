#pragma once

#include "fun/base/base.h"
#include "fun/base/io/io_system.h"
#include "fun/base/string/string.h"
#include "fun/base/container/array.h"
#include "fun/base/runnable.h"
#include "fun/base/mutex.h"
//#include "fun/base/thread.h"
#include "fun/base/atomic_counter.h"
#include "fun/base/filesys/file_system.h"

namespace fun {

/**
 * Base implementation of an async IO system
 * allowing most of the code to be shared across platforms.
 */
class FUN_BASE_API AsyncIoSystemBase
  : public IoSystem
  , public Runnable
{
 public:
  AsyncIoSystemBase(IPlatfformFileSystem& low_level)
    : low_level_(low_level)
  {
  }


  //
  // IoSystem interface
  //

  uint64 LoadData(
        const String& filename,
        int64 offset,
        int64 size,
        AtomicCounter32* counter,
        AsyncIoPriority priority) override;

  uint64 LoadCompressedData(
        const String& filename,
        int64 offset,
        int64 compressed_size,
        int64 uncompressed_size,
        void* dst,
        CompressionFlags compression_flags,
        AtomicCounter32* counter,
        AsyncIoPriority priority) override;

  int32 CancelRequests(const uint64* request_indices, int32 count) override;

  void CancelAllOutstandingRequests() override;

  void BlockTillAllRequestsFinishedAndFlushHandles() override;

  void Run() override;

  void Suspend() override;

  void Resume() override;

  void SetMinPriority(AsyncIoPriority min_priority) override;

  void HintDoneWithFile(const char* filename) override;

  int64 MinimumReadSize() override;

 protected:
  struct IoRequest {
    uint64 request_index;
    int32 file_sort_key;
    String filename;
    uint32 filename_hash;
    int64 offset;
    int64 size;
    int64 uncompressed_size;
    void* dst;
    CompressionFlags compression_flags;
    AtomicCounter32* counter;
    AsyncIoPriority priority;
    bool destroy_handle_request;
    bool has_already_requested_handle_to_be_cached;

    IoRequest()
      : request_index(0)
      , file_sort_key(-1)
      , offset(-1)
      , uncompressed_size(-1)
      , dst(nullptr)
      , compression_flags(CompressionFlags::None)
      , counter(nullptr)
      , priority(AsyncIoPriority::Min)
      , destroy_handle_request(false)
      , has_already_requested_handle_to_be_cached(false)
    {
    }

    String ToString() const
    {
      //TODO
      return String();
    }
  };

  void Internalread(IFile* file, int64 offset, int64 size, void* dst);

  virtual bool PlatformReadDoNotCallDirectly(IFile* file, int64 offset, int64 size, void* dst);

  virtual IFile* PlatformCreateHandle(const char* filename);

  virtual int32 PlatformGetNextRequestIndex();

  virtual void PlatformHandleHintDoneWithFile(const char* filename);

  virtual int64 PlatformMinimumReadSize();

  void FulfillCompressedRead(const IoRequest& request, IFile* file);

  IFile* GetCachedFileHandle(const char* filename);
  IFile* FindCachedFileHandle(const char* filename);
  IFile* FindCachedFileHandle(const uint32 filename_hash);
  void FlushHandles();

  void BlockTillAllRequestsFinished();

  void ConstrainBandWidth(int64 bytes_read, float read_time);

  uint64 QueueIoRequest(
      const char* filename,
      int64 offset,
      int64 size,
      int64 uncompressed_size,
      void* dst,
      CompressionFlags compression_flags,
      AtomicCounter32* counter,
      AsyncIoPriority priority);

  uint64 QueueDestroyHandleRequest(const char* filename);

  void LogIoRequest(const String& message, const IoRequest& request);

 public:
  Mutex mutex_;
  Map<uint32, IFile*> name_hash_to_handle_map_;
  Array<IoRequest> outstanding_requests_;
  Event outstanding_request_event_;
  AtomicCounter32 busy_with_request_;
  AtomicCounter32 is_running_;
  uint64 next_request_index_;
  AtomicCounter32 suspend_count_;
  Mutex exclusive_read_mutex_;
  AsyncIoPriority min_priority_;
  IPlatformFS& low_level_;
};

} // namespace fun
