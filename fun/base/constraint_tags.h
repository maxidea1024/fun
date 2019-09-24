#pragma once

namespace fun {

// 유효하지 않은 인덱스.
enum { INVALID_INDEX = -1 };

// 강제로 초기.
enum ForceInit_TAG { ForceInit };

// 강제로 0으로 초기화 하기.
enum ForceInitToZero_TAG { ForceInitToZero };

// 초기화 하지 않음.
enum NoInit_TAG { NoInit };

// 0으로 초기화 하기.
enum ZeroedInit_TAG { ZeroedInit };

// 0xFF으로 초기화 하기.
enum OnedInit_TAG { OnedInit };

// 메모리 확보하여 초기화하기.
enum ReservationInit_TAG { ReservationInit };

// 엔진 전용 기능임을 나타냄.
enum EngineOnlyFeature_TAG { EngineOnlyFeature };

// 롤백이 일어날 수 있음을 나타냄.
enum Rollbackable_TAG { Rollbackable };

} // namespace fun
