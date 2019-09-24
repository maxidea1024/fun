#include "fun/base/pipe_impl_win32.h"
#include "fun/base/exception.h"

namespace fun {

PipeImpl::PipeImpl() {
  SECURITY_ATTRIBUTES attr;
  attr.nLength = sizeof(attr);
  attr.lpSecurityDescriptor = NULL;
  attr.bInheritHandle = FALSE;

  if (!CreatePipe(&read_handle_, &write_handle_, &attr, 0)) {
    throw CreateFileException("anonymous pipe");
  }
}

PipeImpl::~PipeImpl() {
  CloseRead();
  CloseWrite();
}

int PipeImpl::WriteBytes(const void* data, int len) {
  fun_check(write_handle_ != INVALID_HANDLE_VALUE);

  DWORD written = 0;
  if (!WriteFile(write_handle_, data, len, &written, NULL)) {
    throw WriteFileException("anonymous pipe");
  }
  return written;
}

int PipeImpl::ReadBytes(void* buf, int len) {
  fun_check(read_handle_ != INVALID_HANDLE_VALUE);

  DWORD readed = 0;
  BOOL ok = ReadFile(read_handle_, buf, len, &readed, NULL);
  if (ok || GetLastError() == ERROR_BROKEN_PIPE) {
    return readed;
  } else {
    throw ReadFileException("anonymous pipe");
  }
}

PipeImpl::Handle PipeImpl::ReadHandle() const { return read_handle_; }

PipeImpl::Handle PipeImpl::WriteHandle() const { return write_handle_; }

void PipeImpl::CloseRead() {
  if (read_handle_ != INVALID_HANDLE_VALUE) {
    CloseHandle(read_handle_);
    read_handle_ = INVALID_HANDLE_VALUE;
  }
}

void PipeImpl::CloseWrite() {
  if (write_handle_ != INVALID_HANDLE_VALUE) {
    CloseHandle(write_handle_);
    write_handle_ = INVALID_HANDLE_VALUE;
  }
}

}  // namespace fun
