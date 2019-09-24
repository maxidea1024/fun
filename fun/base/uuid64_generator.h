#pragma once

#include "fun/base/base.h"
#include "fun/base/mutex.h"

namespace fun {

class FUN_BASE_API Uuid64Generator {
 public:
  Uuid64Generator(int32 data_center_id, int32 worker_id, int32 sequence = 0);

  int64 NextId(int32 object_type);

 private:
  // 2017-1-1 0:00:00 (UTC) (in millisec)
  static const int64 EPOCH = 63618825600000LL;

  static const int64 SEQUENCE_BITS = 13;
  static const int64 WORKER_ID_BITS = 2;
  static const int64 DATACENTER_ID_BITS = 2;
  static const int64 OBJECT_TYPE_BITS = 5;
  static const int64 TIMESTAMP_BITS = 42;

  static const int64 WORKER_ID_SHIFT = SEQUENCE_BITS;
  static const int64 DATACENTER_ID_SHIFT = (SEQUENCE_BITS + WORKER_ID_BITS);
  static const int64 OBJECT_TYPE_SHIFT =
      (SEQUENCE_BITS + WORKER_ID_BITS + DATACENTER_ID_BITS);
  static const int64 TIMESTAMP_SHIFT =
      (SEQUENCE_BITS + WORKER_ID_BITS + DATACENTER_ID_BITS + OBJECT_TYPE_BITS);

  static const int64 MAX_WORKER_ID = (1LL << WORKER_ID_BITS) - 1;
  static const int64 MAX_DATACENTER_ID = (1LL << DATACENTER_ID_BITS) - 1;
  static const int64 MAX_OBJECT_TYPE = (1LL << OBJECT_TYPE_BITS) - 1;
  static const int64 MAX_TIMESTAMP = (1LL << TIMESTAMP_BITS) - 1;

  static const int64 SEQUENCE_MASK = (1LL << SEQUENCE_BITS) - 1;

  FastMutex mutex_;

  int64 sequence_;
  int64 worker_id_;
  int64 data_center_id_;

  int64 last_timestamp_;

  int64 TilNextMillisec(int64 timestamp) const;
  int64 GetCurrentTimestamp() const;
};

}  // namespace fun
