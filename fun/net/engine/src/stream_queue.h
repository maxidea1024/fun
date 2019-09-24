#pragma once

#include "fun/containers/array.h"
#include "fun/net/net.h"
#include "send_data.h"

namespace fun {

class StreamQueue {
 public:
  StreamQueue(int32 grow_by);

  const uint8* ConstData() const { return block_.ConstData() + head_; }

  int32 Len() const { return contents_len_; }

  bool IsEmpty() const { return contents_len_ == 0; }

  template <typename Allocator>
  void EnqueueCopy(const Array<uint8, Allocator>& data) {
    EnqueueCopy(data.ConstData(), data.Count());
  }

  void EnqueueCopy(const ByteArray& data) {
    EnqueueCopy((const uint8*)data.ConstData(), data.Len());
  }

  void EnqueueCopy(const uint8* data, int32 len);

  void EnqueueCopy(const SendFragRefs& data_to_send);

  int32 DequeueNoCopy(int32 len);

  int32 DequeueAllNoCopy() { return DequeueNoCopy(Len()); }

 private:
  int32 grow_by_;
  Array<uint8> block_;
  int32 head_;
  int32 contents_len_;

  void Shrink();
};

}  // namespace fun
