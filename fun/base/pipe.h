#pragma once

#include "fun/base/base.h"
#include "fun/base/pipe_impl.h"

namespace fun {

/**
 * This class implements an anonymous pipe.
 *
 * Pipes are a common method of inter-process communication -
 * on Unix, pipes are the oldest form of IPC.
 *
 * A pipe is a half-duplex communication channel, which means
 * that data only flows in one direction.
 * Pipes have a read-end and a write-end. One process writes to
 * the pipe and another process reads the data written by
 * its peer.
 * Read and write operations are always synchronous. A read will
 * block until data is available and a write will block until
 * the reader reads the data.
 *
 * The WriteBytes() and ReadBytes() methods of Pipe are usually
 * used through a PipeOutputStream or PipeInputStream and are
 * not called directly.
 *
 * Pipe objects have value semantics; the actual work is delegated
 * to a reference-counted PipeImpl object.
 */
class FUN_BASE_API Pipe {
 public:
  /**
   * The read/write handle or file descriptor.
   */
  typedef PipeImpl::Handle Handle;

  /**
   * used by close()
   */
  enum CloseMode {
    /**
     * Close reading end of pipe.
     */
    CLOSE_READ = 0x01,
    /**
     * Close writing end of pipe.
     */
    CLOSE_WRITE = 0x02,
    /**
     * Close both ends of pipe.
     */
    CLOSE_BOTH = 0x03
  };
  //TODO flags를 적용하는게 좋을까나...??

  /**
   * Creates the Pipe.
   *
   * Throws a CreateFileException if the pipe cannot be
   * created.
   */
  Pipe();

  /**
   * Closes and destroys the Pipe.
   */
  ~Pipe();

  /**
   * Creates the Pipe using the PipeImpl from another one.
   */
  Pipe(const Pipe& rhs);

  /**
   * Releases the Pipe's PipeImpl and assigns another one.
   */
  Pipe& operator = (const Pipe& rhs);

  /**
   * Sends the contents of the given buffer through
   * the pipe. Blocks until the receiver is ready
   * to read the data.
   *
   * Returns the number of bytes sent.
   *
   * Throws a WriteFileException if the data cannot be written.
   */
  int WriteBytes(const void* buffer, int length);

  /**
   * Receives data from the pipe and stores it
   * in buffer. Up to length bytes are received.
   * Blocks until data becomes available.
   *
   * Returns the number of bytes received, or 0
   * if the pipe has been closed.
   *
   * Throws a ReadFileException if nothing can be read.
   */
  int ReadBytes(void* buffer, int length);

  /**
   * Returns the read handle or file descriptor
   * for the Pipe. For internal use only.
   */
  Handle ReadHandle() const;

  /**
   * Returns the write handle or file descriptor
   * for the Pipe. For internal use only.
   */
  Handle WriteHandle() const;

  /**
   * Depending on the argument, closes either the
   * reading end, the writing end, or both ends
   * of the Pipe.
   */
  void Close(CloseMode mode = CLOSE_BOTH);

 private:
  PipeImpl* impl_;
};


//
// inlines
//

FUN_ALWAYS_INLINE int Pipe::WriteBytes(const void* buffer, int length) {
  return impl_->WriteBytes(buffer, length);
}

FUN_ALWAYS_INLINE int Pipe::ReadBytes(void* buffer, int length) {
  return impl_->ReadBytes(buffer, length);
}

FUN_ALWAYS_INLINE Pipe::Handle Pipe::ReadHandle() const {
  return impl_->ReadHandle();
}

FUN_ALWAYS_INLINE Pipe::Handle Pipe::WriteHandle() const {
  return impl_->WriteHandle();
}

} // namespace fun
