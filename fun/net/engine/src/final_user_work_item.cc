#include "FinalUserWorkItem.h"
#include "fun/net/net.h"

namespace fun {
namespace net {

//
// FinalUserWorkItem
//

void FinalUserWorkItem::From(FinalUserWorkItem_S& src, HostId host_id) {
  type = src.type;
  unsafe_message.remote_id = host_id;

  if (src.type == FinalUserWorkItemType::LocalEvent) {
    local_event = src.local_event;
  } else if (src.type == FinalUserWorkItemType::UserTask) {
    user_func = src.user_func;
  } else {
    unsafe_message.unsafe_message = src.unsafe_message;
  }
}

//
// FinalUserWorkItem_HasLockedDtor
//

FinalUserWorkItem_HasLockedDtor::FinalUserWorkItem_HasLockedDtor(
    CCriticalSection2* mutex) {
  mutex_ = mutex;
  UnsafeMemory::Memset(storage_, 0x00, sizeof(storage_));
  uwi_ = new (storage_) FinalUserWorkItem();
}

FinalUserWorkItem_HasLockedDtor::~FinalUserWorkItem_HasLockedDtor() {
  CScopedLock2 guard(*mutex_);
  uwi_->~FinalUserWorkItem();  //잠금 상태에서 객체 파괴
}

}  // namespace net
}  // namespace fun
