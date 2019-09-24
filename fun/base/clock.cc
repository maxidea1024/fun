#include "fun/base/clock.h"

#include <chrono>

namespace fun {

const Clock::ValueType Clock::MIN_CLOCK_VALUE = std::numeric_limits<Clock::ValueType>::min();
const Clock::ValueType Clock::MAX_CLOCK_VALUE = std::numeric_limits<Clock::ValueType>::max();

void Clock::Update() {
  value_ = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

Clock::DiffType Clock::Accuracy() {
  ValueType acc = static_cast<ValueType>(std::chrono::duration_cast<std::chrono::duration<double, std::micro>>(std::chrono::steady_clock::duration(1)).count());
  return acc > 0 ? acc : 1;
}

bool Clock::IsMonotonic() {
  return std::chrono::steady_clock::is_steady;
}

} // namespace fun
