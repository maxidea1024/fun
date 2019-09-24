//TODO 할당정책을 구지 달리할 필요가 있으려나?
#include "fun/net/net.h"
#include "host_id_factory.h"

namespace fun {
namespace net {

//
// HostIdFactory
//

HostIdFactory::HostIdFactory() : num_(HostId_Last) {}

HostId HostIdFactory::Create(double absolute_time, HostId assigned_host_id) {
  num_ = (HostId)(((uint32)num_) + 1);

  if (num_ == HostId_None) {
    num_ = (HostId)((uint32)HostId_Last + 1);
  }

  return num_;
}

void HostIdFactory::Drop(double absolute_time, HostId id_to_drop) {
  (void)absolute_time;
  (void)id_to_drop;
}

int32 HostIdFactory::GetRecycleCount(HostId host_id) {
  (void)host_id;
  return 0;
}


//
// RecycleHostIdFactory
//

RecycleHostIdFactory::RecycleHostIdFactory(double issue_valid_time)
  : issue_valid_time_(issue_valid_time)
  , last_issued_value_(HostId_Last) {}

HostId RecycleHostIdFactory::Create(double absolute_time, HostId assigned_host_id) {
  //// Drop가 된것이 없다면 새로 발급
  //if (drop_list_.Count() == 0) {
  //  return NewHostId();
  //}
  //
  //auto it = drop_map_.find(drop_list_.Front());
  //
  ////droplist에 있는데 map에 없다니...
  //if (it == drop_map_.end()) {
  //  //error! - 이런 상황이 오면 안된다.
  //  drop_list_.RemoveFront();
  //  return NewHostId();
  //}
  //
  //// 일정시간이 지난것만 재사용.
  //if ((absolute_time - it->second.dropped_time) > issue_valid_time_) {
  //  // 재사용에서 나가는 것은 droptime를 0으로 만들어 준다.
  //  // 사용자 실수로 drop를 2 3번 호출하는것을 미연에 방지 하기위해 플래그를 세운다.
  //  it->second.dropped_time = 0;
  //  ++(it->second.recycle_count);
  //  return drop_list_.RemoveFront();
  //}
  //
  ////시간이 안되었다면 새로 발급.
  //return NewHostId();

  if (drop_list_.IsEmpty()) {
    return NewHostId();
  }

  const HostId front = drop_list_.Front();
  auto* found = drop_map_.Find(front);
  if (found == nullptr) {
    fun_check(0); // unreable
    drop_list_.RemoveFront();
    return NewHostId();
  }

  if ((absolute_time - found->dropped_time) > issue_valid_time_) {
    found->dropped_time = 0;
    found->recycle_count++;
    drop_list_.RemoveFront();
    return front;
  }

  // 시간이 안되었을 경우에는 새로 발급?
  return NewHostId();
}

void RecycleHostIdFactory::Drop(double absolute_time, HostId id_to_drop) {
  if (auto* found = drop_map_.Find(id_to_drop)) {
    if (found->dropped_time != 0) {
      return;
    }

    found->dropped_time = absolute_time;
    drop_list_.Append(id_to_drop);
    return;
  }

  Node new_node;
  new_node.dropped_time = absolute_time;
  new_node.host_id = id_to_drop;
  new_node.recycle_count = 0;

  drop_map_.Emplace(id_to_drop, new_node);
  drop_list_.Append(id_to_drop);
}

HostId RecycleHostIdFactory::NewHostId() {
  last_issued_value_ = (HostId)(((uint32)last_issued_value_) + 1);

  if (last_issued_value_ == HostId_None) {
    last_issued_value_ = (HostId)((uint32)HostId_Last + 1);
  }

  return last_issued_value_;
}

int32 RecycleHostIdFactory::GetRecycleCount(HostId host_id) {
  const auto* found = drop_map_.Find(host_id);
  return found ? found->recycle_count : 0;
}


//
// AssignHostIdFactory
//

AssignHostIdFactory::AssignHostIdFactory() {
  num_ = HostId_Last;
}

HostId AssignHostIdFactory::Create(double absolute_time, HostId assigned_host_id) {
  if ((uint32)assigned_host_id <= (uint32)HostId_Last) {
    return NewHostId();
  }

  bool is_allocated = false;
  if (assigned_map_.TryGetValue(assigned_host_id, is_allocated)) {
    if (is_allocated) {
      return HostId_None;
    } else {
      assigned_map_[assigned_host_id] = true;
      return assigned_host_id;
    }
  }

  assigned_map_.Add(assigned_host_id, true);

  return assigned_host_id;
}

void AssignHostIdFactory::Drop(double absolute_time, HostId id_to_drop) {
  fun_check(assigned_map_.Contains(id_to_drop));
  assigned_map_[id_to_drop] = false;
}

int32 AssignHostIdFactory::GetRecycleCount(HostId host_id) {
  (void)host_id;
  return 0;
}

//TODO static hash로 처리하는게 좋을듯 싶은데...
HostId AssignHostIdFactory::NewHostId() {
  do {
    num_ = (HostId)(((uint32)num_) + 1);

    if (num_ == HostId_None) {
      num_ = (HostId)((uint32)HostId_Last + 1);
    }

    bool is_allocated = false;
    if (assigned_map_.TryGetValue(num_, is_allocated)) {
      if (!is_allocated) {
        assigned_map_[num_] = true;
        break;
      }
    } else {
      assigned_map_.Add(num_, true);
      break;
    }

  } while (true);

  return num_;
}

} // namespace net
} // namespace fun
