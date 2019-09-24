#pragma once

#include "fun/base/base.h"
#include "fun/base/mutex.h"
#include "fun/base/ndc.h"

#include <cstddef>
#include <cstring>
#include <iostream>
#include <vector>

namespace fun {

/**
 * A simple pool for fixed-size memory blocks.
 *
 * The main purpose of this class is to speed-up
 * memory allocations, as well as to reduce memory
 * fragmentation in situations where the same blocks
 * are allocated all over again, such as in server
 * applications.
 *
 * All allocated blocks are retained for future use.
 * A limit on the number of blocks can be specified.
 * Blocks can be preallocated.
 */
class FUN_BASE_API MemoryPool {
 public:
  /**
   * Creates a MemoryPool for blocks with the given blockSize.
   * The number of blocks given in preAlloc are preallocated.
   */
  MemoryPool(size_t block_size, int32 pre_alloc = 0, int32 max_alloc = 0);
  ~MemoryPool();

  /**
   * Returns a memory block. If there are no more blocks
   * in the pool, a new block will be allocated.
   *
   * If maxAlloc blocks are already allocated, an
   * OutOfMemoryException is thrown.
   */
  void* Get();

  /**
   * Releases a memory block and returns it to the pool.
   */
  void Release(void* ptr);

  /**
   * Returns the block size.
   */
  size_t GetBlockSize() const { return block_size_; }

  /**
   * Returns the number of allocated blocks.
   */
  int32 AllocatedCount() const { return allocated_count_; }

  /**
   * Returns the number of available blocks in the pool.
   */
  int32 AvailableCount() const { return (int32)blocks_.Count(); }

 private:
  MemoryPool() = delete;
  MemoryPool(const MemoryPool&) = delete;
  MemoryPool& operator=(const MemoryPool&) = delete;

  void Clear();

  enum { BLOCK_RESERVE = 128 };

  size_t block_size_;
  int32 max_alloc_;
  int32 allocated_count_;
  Array<char*> blocks_;
  FastMutex mutex_;
};

// Macro defining the default initial size of any
// FastMemoryPool; can be overriden by specifying
// FastMemoryPool pre-alloc at runtime.
#define FUN_FAST_MEMORY_POOL_PREALLOC 1000

/**
 * FastMemoryPool is a class for pooling fixed-size blocks of memory.
 *
 * The main purpose of this class is to speed-up memory allocations,
 * as well as to reduce memory fragmentation in situations where the
 * same blocks are GetAllocatedCount all over again, such as in server
 * applications. It differs from the MemoryPool in the way the block
 * size is determined - it is inferred form the held type size and
 * applied statically. It is also, as its name implies, faster than
 * fun::MemoryPool. It is likely to be significantly faster than
 * the runtime platform generic memory allocation functionality
 * as well, but it has certain limitations (aside from only giving
 * blocks of fixed size) - see more below.
 *
 * An object using memory from the pool should be created using
 * in-place new operator; once released back to the pool, its
 * destructor will be called by the pool. The returned pointer
 * must be a valid pointer to the type for which it was obtained.
 *
 * Example use:
 *
 *   using Array;
 *   using std:string;
 *   using std::to_string;
 *   using fun::FastMemoryPool;
 *
 *   int blocks = 10;
 *   FastMemoryPool<int> fast_int_pool(blocks);
 *   FastMemoryPool<String> fast_string_pool(blocks);
 *
 *   vector<int*> int_vec(blocks, 0);
 *   vector<String*> str_vec(blocks);
 *
 *   for (int i = 0; i < blocks; ++i) {
 *     int_vec[i] = new(fast_int_pool.Get()) int(i);
 *     str_vec[i] = new(fast_string_pool.Get()) String(to_string(i));
 *   }
 *
 *   for (int i = 0; i < blocks; ++i) {
 *     fast_int_pool.Release(int_vec[i]);
 *     fast_string_pool.Release(str_vec[i]);
 *   }
 *
 * Pool keeps memory blocks in "buckets". A bucket is an array of
 * blocks; it is always GetAllocatedCount with a single `new[]`, and its blocks
 * are initialized at creation time. Whenever the current capacity
 * of the pool is reached, a new bucket is GetAllocatedCount and its blocks
 * initialized for internal use. If the new bucket allocation would
 * exceed allowed maximum size, std::bad_alloc() exception is thrown,
 * with object itself left intact.
 *
 * Pool internally keeps track of GetAvailableCount blocks through a linked-list
 * and utilizes unused memory blocks for that purpose. This means that,
 * for types smaller than pointer the size of a block will be greater
 * than the size of the type. The implications are following:
 *
 *   - FastMemoryPool can not be used for arrays of types smaller
 *     than pointer
 *
 *   - if FastMemoryPool is used to store variable-size arrays, it
 *     must not have multiple buckets; the way to achieve this is by
 *     specifying proper argument values at construction.
 *
 * Neither of the above are primarily intended or recommended modes
 * of use. It is recommended to use a FastMemoryPool for creation of
 * many objects of the same type. Furthermore, it is perfectly fine
 * to have arrays or STL containers of pointers to objects created
 * in blocks of memory obtained from the FastMemoryPool.
 *
 * Before a block is given to the user, it is removed from the list;
 * when a block is returned to the pool, it is re-inserted in the
 * list. Pool will return held memory to the system at destruction,
 * and will not leak memory after destruction; this means that after
 * pool destruction, any memory that was taken from, but not returned
 * to the pool becomes invalid.
 *
 * FastMemoryPool is thread safe; it uses fun::SpinlockMutex by
 * default, but other mutexes can be specified through te template
 * parameter, if needed. fun::NullMutex can be specified as template
 * parameter to avoid locking and improve speed in single-threaded
 * scenarios.
 */
template <typename T, typename MutexT = SpinlockMutex>
class FUN_BASE_API FastMemoryPool {
 private:
  /**
   * A block of memory. This class represents a memory
   * block. It has dual use, the primary one being
   * obvious - memory provided to the user of the pool.
   * The secondary use is for internal "housekeeping"
   * purposes.
   *
   * It works like this:
   *
   *    - when initially created, a Block is properly
   *      constructed and positioned into the internal
   *      linked list of blocks
   *
   *    - when given to the user, the Block is removed
   *      from the internal linked list of blocks
   *
   *    - when returned back to the pool, the Block
   *      is again in-place constructed and inserted
   *      as next available block in the linked list
   *      of blocks
   */
  class Block {
   public:
    /**
     * Creates a Block and sets its next pointer.
     * This constructor should ony be used to initialize
     * a block sequence (an array of blocks) in a newly
     * allocated bucket.
     *
     * After the construction, the last block's `next`
     * pointer points outside the allocated memory and
     * must be set to zero. This design improves performance,
     * because otherwise the block array would require an
     * initialization loop after the allocation.
     */
    Block() { memory_.next = this + 1; }

    /**
     * Creates a Block and sets its next pointer.
     */
    explicit Block(Block* next) { memory_.next = next; }

    /**
     * Memory block storage.
     *
     * Note that this storage is properly aligned
     * for the datatypes it holds. It will not work
     * for arrays of types smaller than pointer size.
     * Furthermore, the pool itself will not work for
     * a variable-size array of any type after it is
     * resized.
     */
    union {
      char buffer[sizeof(T)];
      Block* next;
    } memory_;

   private:
    Block(const Block&);
    Block& operator=(const Block&);
    Block(Block&&);
    Block& operator=(Block&&);
  };

 public:
  typedef MutexT MutexType;
  typedef typename MutexT::ScopedLock ScopedLock;

  typedef Block* Bucket;
  typedef Array<Bucket> BucketList;

  /**
   * Creates the FastMemoryPool.
   *
   * The size of a block is inferred from the type size. Number of blocks
   * per bucket, pre-allocated bucket pointer storage and maximum allowed
   * total size of the pool can be customized by overriding default
   * parameter value:
   *
   *   - blocks_per_bucket specifies how many blocks each bucket contains
   *                     defaults to FUN_FAST_MEMORY_POOL_PREALLOC
   *
   *   - bucket_pre_alloc specifies how much space for bucket pointers
   *                    (buckets themselves are not prealocated) will be
   *                    pre-alocated.
   *
   *   - max_alloc specifies maximum allowed total pool size in bytes.
   */
  FastMemoryPool(size_t blocks_per_bucket = FUN_FAST_MEMORY_POOL_PREALLOC,
                 size_t bucket_pre_alloc = 10, size_t max_alloc = 0)
      : blocks_per_bucket_(blocks_per_bucket),
        max_alloc_(max_alloc),
        available_(0) {
    if (blocks_per_bucket_ < 2) {
      throw std::invalid_argument(
          "FastMemoryPool: blocks_per_bucket must be >=2");
    }
    buckets_.Reserve(bucket_pre_alloc);
    Resize();
  }

  /**
   * Destroys the FastMemoryPool and releases all memory.
   * Any emory taken from, but not returned to, the pool
   * becomes invalid.
   */
  ~FastMemoryPool() { Clear(); }

  /**
   * Returns pointer to the next available memory block. If the pool is
   * exhausted, it will be resized by allocating a new bucket.
   */
  void* Get() {
    Block* ret;
    {
      ScopedLock guard(mutex_);
      if (first_block_ == 0) {
        Resize();
      }
      ret = first_block_;
      first_block_ = first_block_->memory_.next;
    }
    --available_;
    return ret;
  }

  /**
   * Recycles the released memory by initializing it for
   * internal use and setting it as next available block;
   * previously next block becomes this block's next.
   * Releasing of null pointers is silently ignored.
   * Destructor is called for the returned pointer.
   */
  template <typename P>
  void Release(P* ptr) {
    if (!ptr) {
      return;
    }

    reinterpret_cast<P*>(ptr)->~P();
    ++available_;

    ScopedLock guard(mutex_);
    first_block_ = new (ptr) Block(first_block_);
  }

  /**
   * Returns the block size in bytes.
   */
  size_t GetBlockSize() const { return sizeof(Block); }

  /**
   * Returns the total amount of memory allocated, in bytes.
   */
  size_t AllocatedCount() const {
    return buckets_.Count() * blocks_per_bucket_;
  }

  /**
   * Returns currently available amount of memory in bytes.
   */
  size_t AvailableCount() const { return available_; }

 private:
  FastMemoryPool(const FastMemoryPool&) = delete;
  FastMemoryPool& operator=(const FastMemoryPool&) = delete;
  FastMemoryPool(FastMemoryPool&&) = delete;
  FastMemoryPool& operator=(FastMemoryPool&&) = delete;

  /**
   * Creates new bucket and initializes it for internal use.
   * Sets the previously next block to point to the new bucket's
   * first block and the new bucket's last block becomes the
   * last block.
   */
  void Resize() {
    if (buckets_.Count() == buckets_.Capacity()) {
      size_t new_size = buckets_.Capacity() * 2;
      if (max_alloc_ != 0 && new_size > max_alloc_) {
        throw std::bad_alloc();
      }
      buckets_.Reserve(new_size);
    }
    buckets_.emplace_back(new Block[blocks_per_bucket_]);
    first_block_ = buckets_.back();
    // terminate last block
    first_block_[blocks_per_bucket_ - 1].memory_.next = 0;
    available_ += blocks_per_bucket_;
  }

  void Clear() {
    for (auto& block : buckets_) {
      delete[] block;
    }
  }

  typedef std::atomic<size_t> Counter;

  size_t blocks_per_bucket_;
  BucketList buckets_;
  Block* first_block_;
  size_t max_alloc_;
  Counter available_;
  mutable MutexT mutex_;
};

}  // namespace fun
