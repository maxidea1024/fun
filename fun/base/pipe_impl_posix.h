#pragma once

#include "fun/base/base.h"
#include "fun/base/ref_counted_object.h"

namespace fun {

class FUN_BASE_API PipeImpl : public RefCountedObject {
 public:
  typedef int Handle;

  PipeImpl();
  ~PipeImpl();

  int WriteBytes(const void* data, int len);
  int ReadBytes(void* buf, int len);

  Handle ReadHandle() const;
  Handle WriteHandle() const;

  void CloseRead();
  void CloseWrite();

 private:
  int read_fd_;
  int write_fd_;
};

}  // namespace fun
