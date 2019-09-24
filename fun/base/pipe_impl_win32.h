#pragma once

#include "fun/base/base.h"
#include "fun/base/ref_counted.h"
#include "fun/base/windows_less.h"

namespace fun {

class FUN_BASE_API PipeImpl : public RefCountedObject {
 public:
  typedef HANDLE Handle;

  PipeImpl();
  ~PipeImpl();

  int WriteBytes(const void* data, int len);
  int ReadBytes(void* buf, int len);

  Handle ReadHandle() const;
  Handle WriteHandle() const;

  void CloseRead();
  void CloseWrite();

 private:
  HANDLE read_handle_;
  HANDLE write_handle_;
};

} // namespace fun
