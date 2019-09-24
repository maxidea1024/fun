#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * Enum for async IO priorities.
 */
enum class AsyncIoPriority { Min = 0, Low, BelowNormal, Normal, High, Max };

/*

  char* buffer = new uint8[1024];
  AtomicCounter32 counter = 1;
  IoSystem::Get().LoadData("sample.dat", 0, 1024, buffer, &counter,
  AsyncIoPriority::Normal);

  if (count == 0) {
    StoreData(buffer);
    delete[] buffer;
  }


  TODO lambda callback을 지원하는것은 어떨런지??

*/

/**
 * Virtual base class of IO systems.
 */
struct FUN_BASE_API IoSystem {
  /**
   * System singleton
   */
  static IoSystem& Get();

  /**
   * Shutdown the async IO system
   */
  static void Shutdown();

  /**
   * Checks if the async IO system has already been shut down.
   */
  static bool HasShutdown();

  /**
   * virtual dummy destructor.
   */
  virtual ~IoSystem() {}

  /**
   * Requests data to be loaded async. Returns immediately.
   *
   * @param filename - filename to load
   * @param offset - offset into file
   * @param size - size of load request
   * @param dst - Pointer to load data into
   * @param counter - Thread safe counter to decrement when loading has
   * finished, can be nullptr
   * @param priority - priority of request
   *
   * @return Returns an index to the request that can be used for canceling or 0
   * if the request failed.
   */
  virtual uint64 LoadData(const String& filename, int64 offset, int64 size,
                          void* dst, AtomicCounter32* counter,
                          AsyncIoPriority priority) = 0;

  /**
   * Requests compressed data to be loaded async. Returns immediately.
   *
   * @param filename - filename to load
   * @param offset - offset into file
   * @param compressed_size - size of load request
   * @param uncompressed_size - size of uncompressed data
   * @param dst - Pointer to load data into
   * @param compression_flags - Flags controlling data decompression
   * @param counter - Thread safe counter to decrement when loading has
   * finished, can be nullptr
   * @param priority - priority of request
   *
   * @return Returns an index to the request that can be used for canceling or 0
   * if the request failed.
   */
  virtual uint64 LoadCompressedData(const String& filename, int64 offset,
                                    int64 compressed_size,
                                    int64 uncompressed_size, void* dst,
                                    CompressionFlags compression_flags,
                                    AtomicCounter32* counter,
                                    AsyncIoPriority priority) = 0;

  /**
   * Removes N outstanding requests from the queue and returns
   * how many were canceled. We can't cancel requests currently
   * fulfilled and ones that have already been fulfilled.
   *
   * @param request_indices - Indices of requests to cancel.
   *
   * @return The number of requests that were canceled
   */
  virtual int32 CancelRequests(uint64* request_indices, int32 index_count) = 0;

  /**
   * Removes all outstanding requests from the queue
   */
  virtual void CancelAllOutstandingRequests() = 0;

  /**
   * Blocks till all requests are finished and also
   * flushes potentially open handles.
   */
  virtual void BlockTillAllRequestsFinishedAndFlushHandles() = 0;

  /**
   * Suspend any IO operations (can be called from another thread)
   */
  virtual void Suspend() = 0;

  /**
   * Resume IO operations (can be called from another thread)
   */
  virtual void Resume() = 0;

  /**
   * Force update async loading when multithreading is not supported.
   */
  virtual void TickSingleThreaded() = 0;

  /**
   * Sets the min priority of requests to fulfill. Lower priority requests
   * will still be queued and start to be fulfilled once the min priority
   * is lowered sufficiently. This is only a hint and implementations
   * are free to ignore it.
   *
   * @param min_priority - Min priority of requests to fulfill
   */
  virtual void SetMinPriority(AsyncIoPriority min_priority) = 0;

  /**
   * Give the IO system a hint that it is done with the file for now
   *
   * @param filename - File that was being async loaded from, but no longer is
   */
  virtual void HintDoneWithFile(const String& filename) = 0;

  /**
   * The minimum read size...used to be DVD_ECC_BLOCK_SIZE
   *
   * @return Minimum read size
   */
  virtual int64 MinimumReadSize() = 0;
};

}  // namespace fun
