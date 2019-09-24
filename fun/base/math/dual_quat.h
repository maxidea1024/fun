#pragma once

#include "fun/base/base.h"
#include "fun/base/math/vector.h"

namespace fun {

/** Dual quaternion class */
class DualQuat {
 public:
  /** rotation or real part */
  Quat r;

  /** half trans or dual part */
  Quat d;

  DualQuat(const Quat& r, const Quat& d) : r(r), d(d) {}

  DualQuat(const Transform& t) {
    Vector v = t.GetTranslation()*0.5f;
    *this = DualQuat(Quat(0, 0, 0, 1), Quat(v.x, v.y, v.z, 0.f)) * DualQuat(t.GetRotation(), Quat(0, 0, 0, 0));
  }

  /** Dual quat addition */
  DualQuat operator + (const DualQuat& b) const {
    return { r + b.r, d + b.d };
  }

  /** Dual quat product */
  DualQuat operator * (const DualQuat& b) const {
    return { r*b.r, d*b.r + b.d*r };
  }

  /** Scale dual quat */
  DualQuat operator * (const float s) const {
    return { r*s, d*s };
  }

  /** Return normalized dual quat */
  DualQuat Normalized() const {
    float min_v = 1.f / Math::Sqrt(r | r);
    return { r*min_v, d*min_v };
  }

  /** Convert dual quat to transform */
  Transform AsTransform(const Vector& scale = Vector(1.f, 1.f, 1.f)) {
    Quat tq = d*Quat(-r.x, -r.y, -r.z, r.w);
    return Transform(r, Vector(tq.x, tq.y, tq.z)*2.f, scale);
  }
};

} // namespace fun
