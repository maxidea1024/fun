#include "fun/base/shared_memory_win32.h"
#include "fun/base/error.h"
#include "fun/base/exception.h"
#include "fun/base/file.h"
#include "fun/base/windows_less.h"

namespace fun {

SharedMemoryImpl::SharedMemoryImpl( const String& name,
                                    size_t size,
                                    SharedMemory::AccessMode mode,
                                    const void*, bool)
  : name_(name),
    mem_handle_(INVALID_HANDLE_VALUE),
    file_handle_(INVALID_HANDLE_VALUE),
    size_(static_cast<DWORD>(size)),
    mode_(PAGE_READONLY),
    address_(0) {
  if (mode == SharedMemory::AM_WRITE) {
    mode_ = PAGE_READWRITE;
  }

  UString uname = UString::FromUtf8(name_);
  mem_handle_ = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, mode_, 0, size_, uname.c_str());

  if (!mem_handle_) {
    DWORD rc = GetLastError();
#if defined (_WIN32_WCE)
    throw SystemException(String::Format("Cannot create shared memory object {0} [Error {1}: {2}]", name_, static_cast<int>(rc), Error::Message(rc)));
#else
    if (mode_ != PAGE_READONLY || rc != 5) {
      throw SystemException(String::Format("Cannot create shared memory object {0} [Error {1}: {2}]", name_, static_cast<int>(rc), Error::Message(rc)));
    }

    mem_handle_ = OpenFileMappingW(PAGE_READONLY, FALSE, uname.c_str());

    if (!mem_handle_) {
      rc = GetLastError();
      throw SystemException(String::Format("Cannot open shared memory object {0} [Error {1}: {2}]", name_, static_cast<int>(rc), Error::Message(rc)));
    }
#endif
  }

  Map();
}

SharedMemoryImpl::SharedMemoryImpl( const File& file,
                                    SharedMemory::AccessMode mode,
                                    const void*)
  : name_(file.GetPath()),
    mem_handle_(INVALID_HANDLE_VALUE),
    file_handle_(INVALID_HANDLE_VALUE),
    size_(0),
    mode_(PAGE_READONLY),
    address_(0) {
  if (!file.Exists() || !file.IsFile()) {
    throw FileNotFoundException(name_);
  }

  size_ = static_cast<DWORD>(file.GetSize());

  DWORD share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;
  DWORD file_mode  = GENERIC_READ;

  if (mode == SharedMemory::AM_WRITE) {
    mode_ = PAGE_READWRITE;
    file_mode |= GENERIC_WRITE;
  }

  UString uname = UString::FromUtf8(name_);
  file_handle_ = CreateFileW( uname.c_str(),
                              file_mode,
                              share_mode,
                              NULL,
                              OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL,
                              NULL);

  if (file_handle_ == INVALID_HANDLE_VALUE) {
    throw OpenFileException("Cannot open memory mapped file", name_);
  }

  mem_handle_ = CreateFileMapping(file_handle_, NULL, mode_, 0, 0, NULL);
  if (!mem_handle_) {
    DWORD rc = GetLastError();
    CloseHandle(file_handle_);
    file_handle_ = INVALID_HANDLE_VALUE;
    throw SystemException(String::Format("Cannot map file into shared memory {0} [Error {1}: {2}]", name_, (int)rc, Error::Message(rc)));
  }
  Map();
}

SharedMemoryImpl::~SharedMemoryImpl() {
  Unmap();
  Close();
}

void SharedMemoryImpl::Map() {
  DWORD access = FILE_MAP_READ;
  if (mode_ == PAGE_READWRITE) {
    access = FILE_MAP_WRITE;
  }

  LPVOID addr = MapViewOfFile(mem_handle_, access, 0, 0, size_);
  if (!addr) {
    DWORD rc = GetLastError();
    throw SystemException(String::Format("Cannot map shared memory object {0} [Error {1}: {2}]", name_, (int)rc, Error::Message(rc)));
  }

  address_ = static_cast<char*>(addr);
}

void SharedMemoryImpl::Unmap() {
  if (address_) {
    UnmapViewOfFile(address_);
    address_ = 0;
    return;
  }
}

void SharedMemoryImpl::Close() {
  if (mem_handle_ != INVALID_HANDLE_VALUE) {
    CloseHandle(mem_handle_);
    mem_handle_ = INVALID_HANDLE_VALUE;
  }

  if (file_handle_ != INVALID_HANDLE_VALUE) {
    CloseHandle(file_handle_);
    file_handle_ = INVALID_HANDLE_VALUE;
  }
}

} // namespace fun
