#include "fun/base/uuid64_generator.h"
#include "fun/base/date_time.h"
#include "fun/base/system_time.h"

// TODO 논리적으로 shard를 0 ~ 2047까지 배정할 수 있으면 좋을듯 한데...

namespace fun {

Uuid64Generator::Uuid64Generator(int32 data_center_id, int32 worker_id,
                                 int32 sequence) {
  if (data_center_id < 0 || data_center_id > MAX_DATACENTER_ID) {
    // throw exception
  }

  if (worker_id < 0 || worker_id > MAX_WORKER_ID) {
    // throw exception
  }

  if (sequence < 0 || sequence > SEQUENCE_MASK) {
    // throw exception
  }

  data_center_id_ = data_center_id;
  worker_id_ = worker_id;
  sequence_ = sequence;
  last_timestamp_ = -1;
}

int64 Uuid64Generator::NextId(int32 object_type) {
  if (object_type < 0 || object_type > MAX_OBJECT_TYPE) {
    // throw exception
  }

  ScopedLock<FastMutex> guard(mutex_);

  int64 timestamp = GetCurrentTimestamp();
  if (timestamp < last_timestamp_) {
    // throw clock is moving backwards.
  }

  if (timestamp == last_timestamp_) {
    sequence_ = (sequence_ + 1) & SEQUENCE_MASK;
    if (sequence_ == 0) {  // Overflow
      timestamp = TilNextMillisec(last_timestamp_);
    }
  } else {
    sequence_ = 0;
  }

  last_timestamp_ = timestamp;

  const int64 time_offset = timestamp - EPOCH;
  const int64 id = (time_offset << TIMESTAMP_SHIFT) |
                   ((int64)object_type << OBJECT_TYPE_SHIFT) |
                   (data_center_id_ << DATACENTER_ID_SHIFT) |
                   (worker_id_ << WORKER_ID_SHIFT) | sequence_;
  return id;
}

int64 Uuid64Generator::TilNextMillisec(int64 last_timestamp) const {
  int64 timestamp = GetCurrentTimestamp();
  while (timestamp <= last_timestamp) {
    timestamp = GetCurrentTimestamp();
  }
  return timestamp;
}

int64 Uuid64Generator::GetCurrentTimestamp() const {
  return SystemTime::Milliseconds();
}

}  // namespace fun
