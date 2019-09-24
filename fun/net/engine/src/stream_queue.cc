#include "fun/net/net.h"
#include "send_data.h"
#include "stream_queue.h"

namespace fun {

StreamQueue::StreamQueue(int32 grow_by) {
  fun_check(grow_by > 0);
  grow_by_ = grow_by;
  head_ = 0;
  contents_len_ = 0;
}

void StreamQueue::EnqueueCopy(const uint8* data, int32 len) {
  // 블럭 크기가 초과되지 않는 경우 그냥 추가한다.
  if ((head_ + contents_len_ + len) < block_.Count()) {
    UnsafeMemory::Memcpy(&block_[head_ + contents_len_], data, len);
    contents_len_ += len;
  } else {
    // 추가하려 해도 블럭 크기를 초과하면 일단 앞으로 땡긴다.
    if (head_ > 0 && !block_.IsEmpty()) {
      Shrink();
    }

    // 블럭 크기가 초과되고 앞으로 땡기더라도 공간이 모자란 경우 블럭 크기를 재할당한다.
    if ((contents_len_ + len) > block_.Count()) {
      block_.ResizeUninitialized(contents_len_ + len + grow_by_);
    }

    // 복사하기
    UnsafeMemory::Memcpy(&block_[contents_len_], data, len);
    contents_len_ += len;
  }
}

void StreamQueue::EnqueueCopy(const SendFragRefs& data_to_send) {
  for (int32 i = 0; i < data_to_send.Count(); ++i) {
    if (data_to_send[i].data) {
      EnqueueCopy(data_to_send[i].data, data_to_send[i].len);
    }
  }
}

int32 StreamQueue::DequeueNoCopy(int32 len) {
  if (len > 0) {
    fun_check(len <= contents_len_); //fun_check

    len = MathBase::Min(len, contents_len_);
    head_ += len;
    contents_len_ -= len;

    if (contents_len_ <= grow_by_ / 64) {
      Shrink();
    }
  }

  return len;
}

void StreamQueue::Shrink() {
  if (contents_len_ > 0) {
    UnsafeMemory::Memmove(&block_[0], &block_[head_], contents_len_);
  }

  head_ = 0;
}

} // namespace fun
