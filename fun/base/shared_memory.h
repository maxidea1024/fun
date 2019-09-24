#pragma once

#include "fun/base/base.h"
#include "fun/base/ftl/template.h" // Swap

#include <algorithm>
#include <cstddef>

namespace fun {

class SharedMemoryImpl;
class File;

/**
 * Create and manage a shared memory object.
 *
 * A SharedMemory object has value semantics, but
 * is implemented using a handle/implementation idiom.
 * Therefore, multiple SharedMemory objects can share
 * a single, reference counted SharedMemoryImpl object.
 */
class FUN_BASE_API SharedMemory {
 public:
  enum AccessMode {
    AM_READ = 0,
    AM_WRITE
  };

  /**
   * Default constructor creates an unmapped SharedMemory object.
   * No clients can connect to an unmapped SharedMemory object.
   */
  SharedMemory();

  /**
   * Creates or connects to a shared memory object with the given name.
   *
   * For maximum portability, name should be a valid Unix filename and not
   * contain any slashes or backslashes.
   *
   * An address hint can be passed to the system, specifying the desired
   * start address of the shared memory area. Whether the hint
   * is actually honored is, however, up to the system. Windows platform
   * will generally ignore the hint.
   *
   * If server is set to true, the shared memory region will be unlinked
   * by calling shm_unlink() (on POSIX platforms) when the SharedMemory object is destroyed.
   * The server parameter is ignored on Windows platforms.
   */
  SharedMemory( const String& name,
                size_t size,
                AccessMode mode,
                const void* addr_hint = nullptr,
                bool server = true);

  /**
   * Maps the entire contents of file into a shared memory segment.
   *
   * An address hint can be passed to the system, specifying the desired
   * start address of the shared memory area. Whether the hint
   * is actually honored is, however, up to the system. Windows platform
   * will generally ignore the hint.
   */
  SharedMemory( const File& file,
                AccessMode mode,
                const void* addr_hint = nullptr);

  /**
   * Creates a SharedMemory object by copying another one.
   */
  SharedMemory(const SharedMemory& other);

  /**
   * Destroys the SharedMemory.
   */
  ~SharedMemory();

  /**
   * Assigns another SharedMemory object.
   */
  SharedMemory& operator = (const SharedMemory& other);

  /**
   * Swaps the SharedMemory object with another one.
   */
  void Swap(SharedMemory& other);

  /**
   * Returns the start address of the shared memory segment.
   * Will be NULL for illegal segments.
   */
  char* begin() const;

  /**
   * Returns the one-past-end end address of the shared memory segment.
   * Will be NULL for illegal segments.
   */
  char* end() const;

 private:
  SharedMemoryImpl* impl_;
};


//
// inlines
//

inline void SharedMemory::Swap(SharedMemory& other) {
  fun::Swap(impl_, other.impl_);
}

} // namespace fun
