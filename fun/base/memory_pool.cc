#include "fun/base/memory_pool.h"
#include "fun/base/exception.h"

namespace fun {

MemoryPool::MemoryPool(std::size_t block_size, int32 pre_alloc, int32 max_alloc)
  : block_size_(block_size),
    max_alloc_(max_alloc),
    allocated_count_(pre_alloc) {
  fun_check(max_alloc == 0 || max_alloc >= pre_alloc);
  fun_check(pre_alloc >= 0 && max_alloc >= 0);

  int32 r = BLOCK_RESERVE;
  if (pre_alloc > r) {
    r = pre_alloc;
  }
  if (max_alloc > 0 && max_alloc < r) {
    r = max_alloc;
  }
  blocks_.Reserve(r);

  try {
    for (int i = 0; i < pre_alloc; ++i) {
      blocks_.PushBack(new char[block_size_]);
    }
  } catch (...) {
    Clear();
    throw;
  }
}

MemoryPool::~MemoryPool() {
  Clear();
}

void MemoryPool::Clear() {
  for (auto& block : blocks_) {
    delete[] block;
  }
  blocks_.Clear();
}

void* MemoryPool::Get() {
  FastMutex::ScopedLock guard(mutex_);

  if (blocks_.IsEmpty()) {
    if (max_alloc_ == 0 || allocated_count_ < max_alloc_) {
      ++allocated_count_;
      return new char[block_size_];
    } else {
      throw OutOfMemoryException("MemoryPool exhausted");
    }
  } else {
    char* ptr = blocks_.Back();
    blocks_.PopBack();
    return ptr;
  }
}

void MemoryPool::Release(void* ptr) {
  FastMutex::ScopedLock guard(mutex_);

  try {
    blocks_.PushBack(reinterpret_cast<char*>(ptr));
  } catch (...) {
    delete[] reinterpret_cast<char*>(ptr);
  }
}

} // namespace fun
