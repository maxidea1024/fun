#include "fun/base/math/math.h"

namespace fun {

float GenericMathBase::Atan2(float y, float x) {
  const float abs_x = Math::Abs(x);
  const float abs_y = Math::Abs(y);
  const bool y_abs_bigger = (abs_y > abs_x);
  float t0 = y_abs_bigger ? abs_y : abs_x; // Max(abs_y, abs_x)
  float t1 = y_abs_bigger ? abs_x : abs_y; // Min(abs_x, abs_y)

  if (t0 == 0.f) {
    return 0.f;
  }

  float t3 = t1 / t0;
  float t4 = t3 * t3;

  static const float c[7] = {
    +7.2128853633444123e-03f,
    -3.5059680836411644e-02f,
    +8.1675882859940430e-02f,
    -1.3374657325451267e-01f,
    +1.9856563505717162e-01f,
    -3.3324998579202170e-01f,
    +1.f
  };

  t0 = c[0];
  t0 = t0 * t4 + c[1];
  t0 = t0 * t4 + c[2];
  t0 = t0 * t4 + c[3];
  t0 = t0 * t4 + c[4];
  t0 = t0 * t4 + c[5];
  t0 = t0 * t4 + c[6];
  t3 = t0 * t3;

  t3 = y_abs_bigger ? (0.5f * PI) - t3 : t3;
  t3 = (x < 0.f) ? PI - t3 : t3;
  t3 = (y < 0.f) ? -t3 : t3;

  return t3;
}

} // namespace fun
