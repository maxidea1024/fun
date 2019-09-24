#include "fun/base/shared_memory_posix.h"
#include "fun/base/exception.h"
#include "fun/base/file.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

namespace fun {

SharedMemoryImpl::SharedMemoryImpl( const String& name,
                                    size_t size,
                                    SharedMemory::AccessMode mode,
                                    const void* addr_hint,
                                    bool server)
  : size_(size),
    fd_(-1),
    address_(0),
    access_mode_(mode),
    name_("/"),
    file_mapped_(false),
    server_(server) {
#if FUN_PLATFORM == FUN_PLATFORM_HPUX
  name_.Append("tmp/");
#endif

  name_.Append(name);

  int flags = server_ ? O_CREAT : 0;
  if (access_mode_ == SharedMemory::AM_WRITE) {
    flags |= O_RDWR;
  } else {
    flags |= O_RDONLY;
  }

  // open the shared memory segment
  fd_ = ::shm_open(name_.c_str(), flags, S_IRUSR | S_IWUSR);
  if (fd_ == -1) {
    throw SystemException("Cannot create shared memory object", name_);
  }

  // now set the correct size for the segment
  if (server_ && -1 == ::ftruncate(fd_, size)) {
    ::close(fd_);
    fd_ = -1;
    ::shm_unlink(name_.c_str());
    throw SystemException("Cannot resize shared memory object", name_);
  }
  map(addr_hint);
}

SharedMemoryImpl::SharedMemoryImpl( const File& file,
                                    SharedMemory::AccessMode mode,
                                    const void* addr_hint)
  : size_(0),
    fd_(-1),
    address_(0),
    access_mode_(mode),
    name_(file.GetPath()),
    file_mapped_(true),
    server_(false) {
  if (!file.Exists() || !file.IsFile()) {
    throw FileNotFoundException(file.GetPath());
  }

  size_ = file.GetSize();
  int flag = O_RDONLY;
  if (mode == SharedMemory::AM_WRITE) {
    flag = O_RDWR;
  }

  fd_ = ::open(name_.c_str(), flag);
  if (-1 == fd_) {
    throw OpenFileException("Cannot open memory mapped file", name_);
  }

  Map(addr_hint);
}

SharedMemoryImpl::~SharedMemoryImpl() {
  Unmap();
  Close();
}

void SharedMemoryImpl::Map(const void* addr_hint) {
  int access = PROT_READ;
  if (access_mode_ == SharedMemory::AM_WRITE) {
    access |= PROT_WRITE;
  }

  void* addr = ::mmap(const_cast<void*>(addr_hint), size_, access, MAP_SHARED, fd_, 0);
  if (addr == MAP_FAILED) {
    throw SystemException("Cannot map file into shared memory", name_);
  }

  address_ = static_cast<char*>(addr);
}

void SharedMemoryImpl::Unmap() {
  if (address_) {
    ::munmap(address_, size_);
  }
}

void SharedMemoryImpl::Close() {
  if (fd_ != -1) {
    ::close(fd_);
    fd_ = -1;
  }

  if (!file_mapped_ && server_) {
    ::shm_unlink(name_.c_str());
  }
}

} // namespace fun
