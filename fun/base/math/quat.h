#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * Floating point quaternion that can represent a rotation about an axis in 3-D space.
 * The x, y, z, w components also double as the axis/angle format.
 *
 * Order matters when composing quaternions: C = a * b will yield a quaternion C that logically
 * first applies b then a to any subsequent transformation (right first, then left).
 * Note that this is the opposite order of Transform multiplication.
 *
 * Example: local_to_world = (local_to_world * delta_rotation) will change rotation in local space by delta_rotation.
 * Example: local_to_world = (delta_rotation * local_to_world) will change rotation in world space by delta_rotation.
 */
class alignas(16) Quat {
 public:
  float x;
  float y;
  float z;
  float w;

  static FUN_BASE_API const Quat Identity;

  Quat();
  explicit Quat(ForceInit_TAG);
  Quat(float x, float y, float z, float w);
  Quat(const Quat& q);
  Quat(const Matrix& m);
  Quat(const Rotator& r);
  Quat(const Vector& axis, float angle_radians);
  Quat& operator = (const Quat& other);

  Quat operator + (const Quat& q) const;
  Quat operator += (const Quat& q);
  Quat operator -(const Quat& q) const;

  bool Equals(const Quat& q, float tolerance = KINDA_SMALL_NUMBER) const;
  bool IsIdentity() const;

  Quat operator -= (const Quat& q);
  Quat operator * (const Quat& q) const;
  Quat operator *= (const Quat& q);

  Vector operator * (const Vector& v) const;
  Matrix operator * (const Matrix& m) const;
  Quat operator *= (const float scale);
  Quat operator * (const float scale) const;
  Quat operator /= (const float scale);
  Quat operator / (const float scale) const;
  bool operator == (const Quat& q) const;
  bool operator != (const Quat& q) const;
  float operator | (const Quat& q) const;

  static FUN_BASE_API Quat MakeFromEuler(const Vector& euler_degrees);
  FUN_BASE_API Vector ToEuler() const;

  void Normalize(float tolerance = SMALL_NUMBER);
  Quat GetNormalized(float tolerance = SMALL_NUMBER) const;
  bool IsNormalized() const;
  float Size() const;
  float SizeSquared() const;

  void ToAxisAndAngle(Vector& out_axis, float& out_angle) const;

  Vector RotateVector(const Vector& v) const;
  Vector UnrotateVector(const Vector& v) const;

  FUN_BASE_API Quat Log() const;
  FUN_BASE_API Quat Exp() const;

  Quat Inverse() const;

  void EnforceShortestArcWith(const Quat& other_quat);

  Vector GetAxisX() const;
  Vector GetAxisY() const;
  Vector GetAxisZ() const;
  Vector GetForwardVector() const;
  Vector GetRightVector() const;
  Vector GetUpVector() const;
  Vector ToVector() const;
  FUN_BASE_API Rotator ToRotator() const;
  Vector GetRotationAxis() const;

  FUN_BASE_API bool NetSerialize(Archive& ar, class RPackageMap* map, bool& out_success);

  bool ContainsNaN() const;

  String ToString() const;

#if FUN_ENABLE_NAN_DIAGNOSTIC
  FUN_ALWAYS_INLINE void DiagnosticCheckNaN() const {
    if (ContainsNaN()) {
      LOG_OR_ENSURE_NAN_ERROR("Quat contains NaN: %s", *ToString());
      *const_cast<Quat*>(this) = Quat::Identity;
    }
  }
#else
  void DiagnosticCheckNaN() const {}
#endif

 public:
  static Quat FindBetween(const Vector& vector1, const Vector& vector2) {
    return FindBetweenVectors(vector1, vector2);
  }

  static FUN_BASE_API Quat FindBetweenNormals(const Vector& normal1, const Vector& normal2);
  static FUN_BASE_API Quat FindBetweenVectors(const Vector& vector1, const Vector& vector2);

  static float Error(const Quat& q1, const Quat& q2);
  static float ErrorAutoNormalize(const Quat& a, const Quat& b);

  static Quat FastLerp(const Quat& a, const Quat& b, const float alpha);
  static Quat FastBilerp(const Quat& p00, const Quat& p10, const Quat& p01, const Quat& p11, float frac_x, float frac_y);
  static FUN_BASE_API Quat Slerp_NotNormalized(const Quat& quat1, const Quat& quat2, float slerp);

  static Quat slerp(const Quat& quat1, const Quat& quat2, float slerp) {
    return Slerp_NotNormalized(quat1, quat2, slerp).GetNormalized();
  }

  static FUN_BASE_API Quat SlerpFullPath_NotNormalized(const Quat& quat1, const Quat& quat2, float alpha);

  static Quat SlerpFullPath(const Quat& quat1, const Quat& quat2, float alpha) {
    return SlerpFullPath_NotNormalized(quat1, quat2, alpha).GetNormalized();
  }

  static FUN_BASE_API Quat Squad(const Quat& quat1, const Quat& tan1, const Quat& quat2, const Quat& tan2, float alpha);
  static FUN_BASE_API void CalcTangents(const Quat& prev_p, const Quat& p, const Quat& next_p, float tension, Quat& out_tan);

  friend Archive& operator & (Archive& ar, Quat& v) {
    return ar & v.x & v.y & v.z & v.w;
  }

  bool Serialize(Archive& ar) {
    ar & *this;
    return true;
  }
};


//
// inlines
//

FUN_ALWAYS_INLINE Quat::Quat() {}

FUN_ALWAYS_INLINE Quat::Quat(const Matrix& m) {
  // If matrix is NULL, return Identity quaternion. If any of them is 0, you won't be able to construct rotation
  // if you have two plane at least, we can reconstruct the frame using cross product, but that's a bit expensive op to do here
  // for now, if you convert to matrix from 0 scale and convert back, you'll lose rotation. Don't do that.
  if (m.GetScaledAxis(Axis::X).IsNearlyZero() || m.GetScaledAxis(Axis::Y).IsNearlyZero() || m.GetScaledAxis(Axis::Z).IsNearlyZero()) {
    *this = Quat::Identity;
    return;
  }

#if !(FUN_BUILD_SHIPPING || FUN_BUILD_TEST)
  // Make sure the Rotation part of the matrix is unit length.
  // Changed to this (same as RemoveScaling) from RotDeterminant as using two different ways of checking unit length matrix caused inconsistency.
  if (!ENSURE((Math::Abs(1.f - m.GetScaledAxis(Axis::X).SizeSquared()) <= KINDA_SMALL_NUMBER) && (Math::Abs(1.f - m.GetScaledAxis(Axis::Y).SizeSquared()) <= KINDA_SMALL_NUMBER) && (Math::Abs(1.f - m.GetScaledAxis(Axis::Z).SizeSquared()) <= KINDA_SMALL_NUMBER))) {
    *this = Quat::Identity;
    return;
  }
#endif

  //const MeReal *const t = (MeReal *) tm;
  float s;

  // Check diagonal (trace)
  const float tr = m.m[0][0] + m.m[1][1] + m.m[2][2];

  if (tr > 0.f) {
    float inv_s = Math::InvSqrt(tr + 1.f);
    this->w = 0.5f * (1.f / inv_s);
    s = 0.5f * inv_s;

    this->x = (m.m[1][2] - m.m[2][1]) * s;
    this->y = (m.m[2][0] - m.m[0][2]) * s;
    this->z = (m.m[0][1] - m.m[1][0]) * s;
  } else {
    // diagonal is negative
    int32 i = 0;

    if (m.m[1][1] > m.m[0][0]) {
      i = 1;
    }

    if (m.m[2][2] > m.m[i][i]) {
      i = 2;
    }

    static const int32 nxt[3] = { 1, 2, 0 };
    const int32 j = nxt[i];
    const int32 k = nxt[j];

    s = m.m[i][i] - m.m[j][j] - m.m[k][k] + 1.f;

    float inv_s = Math::InvSqrt(s);

    float qt[4];
    qt[i] = 0.5f * (1.f / inv_s);

    s = 0.5f * inv_s;

    qt[3] = (m.m[j][k] - m.m[k][j]) * s;
    qt[j] = (m.m[i][j] + m.m[j][i]) * s;
    qt[k] = (m.m[i][k] + m.m[k][i]) * s;

    this->x = qt[0];
    this->y = qt[1];
    this->z = qt[2];
    this->w = qt[3];

    DiagnosticCheckNaN();
  }
}

FUN_ALWAYS_INLINE Quat::Quat(const Rotator& r) {
  *this = r.Quaternion();
  DiagnosticCheckNaN();
}

FUN_ALWAYS_INLINE Vector Quat::operator * (const Vector& v) const {
  return RotateVector(v);
}

FUN_ALWAYS_INLINE Matrix Quat::operator * (const Matrix& m) const {
  Matrix result;
  Quat vt, vr;
  Quat inv = Inverse();
  for (int32 i = 0; i < 4; ++i) {
    Quat vq(m.m[i][0], m.m[i][1], m.m[i][2], m.m[i][3]);
    VectorQuaternionMultiply(&vt, this, &vq);
    VectorQuaternionMultiply(&vr, &vt, &inv);
    result.m[i][0] = vr.x;
    result.m[i][1] = vr.y;
    result.m[i][2] = vr.z;
    result.m[i][3] = vr.w;
  }

  return result;
}

FUN_ALWAYS_INLINE Quat::Quat(ForceInit_TAG zero_or_not)
  : x(0), y(0), z(0),
    w(zero_or_not == ForceInitToZero ? 0.f : 1.f) {}

FUN_ALWAYS_INLINE Quat::Quat(float x, float y, float z, float w)
  : x(x), y(y), z(z), w(w) {
  DiagnosticCheckNaN();
}

FUN_ALWAYS_INLINE Quat::Quat(const Quat& q)
  : x(q.x), y(q.y), z(q.z), w(q.w) {}

FUN_ALWAYS_INLINE String Quat::ToString() const {
  return String::Format("x=%.6f y=%.6f z=%.6f w=%.6f", x, y, z, w);
}

FUN_ALWAYS_INLINE Quat& Quat::operator = (const Quat& q) {
  x = q.x;
  y = q.y;
  z = q.z;
  w = q.w;
  return *this;
}

FUN_ALWAYS_INLINE Quat::Quat(const Vector& axis, float angle_radians) {
  const float half_angle = 0.5f * angle_radians;
  float s, c;
  Math::SinCos(&s, &c, half_angle);

  x = s * axis.x;
  y = s * axis.y;
  z = s * axis.z;
  w = c;

  DiagnosticCheckNaN();
}

FUN_ALWAYS_INLINE Quat Quat::operator+(const Quat& q) const {
  return Quat(x + q.x, y + q.y, z + q.z, w + q.w);
}

FUN_ALWAYS_INLINE Quat Quat::operator += (const Quat& q) {
  x += q.x;
  y += q.y;
  z += q.z;
  w += q.w;
  DiagnosticCheckNaN();
  return *this;
}

FUN_ALWAYS_INLINE Quat Quat::operator - (const Quat& q) const {
  return Quat(x - q.x, y - q.y, z - q.z, w - q.w);
}

FUN_ALWAYS_INLINE bool Quat::Equals(const Quat& q, float tolerance) const {
#if FUN_PLATFORM_ENABLE_VECTORINTRINSICS
  const VectorRegister tolerance_v = VectorLoadFloat1(&tolerance);
  const VectorRegister a = VectorLoadAligned(this);
  const VectorRegister b = VectorLoadAligned(&q);

  const VectorRegister rotation_sub = VectorAbs(VectorSubtract(a, b));
  const VectorRegister rotation_add = VectorAbs(VectorAdd(a, b));
  return !VectorAnyGreaterThan(rotation_sub, tolerance_v) || !VectorAnyGreaterThan(rotation_add, tolerance_v);
#else
  return (Math::Abs(x - q.x) <= tolerance && Math::Abs(y - q.y) <= tolerance && Math::Abs(z - q.z) <= tolerance && Math::Abs(w - q.w) <= tolerance)
      || (Math::Abs(x + q.x) <= tolerance && Math::Abs(y + q.y) <= tolerance && Math::Abs(z + q.z) <= tolerance && Math::Abs(w + q.w) <= tolerance);
#endif //FUN_PLATFORM_ENABLE_VECTORINTRINSICS
}

FUN_ALWAYS_INLINE bool Quat::IsIdentity() const {
  return (w * w) > ((1.f - DELTA) * (1.f - DELTA));
}

FUN_ALWAYS_INLINE Quat Quat::operator -= (const Quat& q) {
  x -= q.x;
  y -= q.y;
  z -= q.z;
  w -= q.w;
  DiagnosticCheckNaN();
  return *this;
}

FUN_ALWAYS_INLINE Quat Quat::operator * (const Quat& q) const {
  Quat result;
  VectorQuaternionMultiply(&result, this, &q);
  result.DiagnosticCheckNaN();
  return result;
}

FUN_ALWAYS_INLINE Quat Quat::operator *= (const Quat& q) {
  // Now this uses VectorQuaternionMultiply that is optimized per platform.
  VectorRegister a = VectorLoadAligned(this);
  VectorRegister b = VectorLoadAligned(&q);
  VectorRegister result;
  VectorQuaternionMultiply(&result, &a, &b);
  VectorStoreAligned(result, this);
  DiagnosticCheckNaN();
  return *this;
}

FUN_ALWAYS_INLINE Quat Quat::operator *= (const float scale) {
  x *= scale;
  y *= scale;
  z *= scale;
  w *= scale;
  DiagnosticCheckNaN();
  return *this;
}

FUN_ALWAYS_INLINE Quat Quat::operator * (const float scale) const {
  return Quat(scale * x, scale * y, scale * z, scale * w);
}

FUN_ALWAYS_INLINE Quat Quat::operator /= (const float scale) {
  const float one_over_scale = 1.f / scale;
  x *= one_over_scale;
  y *= one_over_scale;
  z *= one_over_scale;
  w *= one_over_scale;
  DiagnosticCheckNaN();
  return *this;
}

FUN_ALWAYS_INLINE Quat Quat::operator / (const float scale) const {
  const float one_over_scale = 1.f / scale;
  return Quat(x * one_over_scale, y * one_over_scale, z * one_over_scale, w * one_over_scale);
}

FUN_ALWAYS_INLINE bool Quat::operator == (const Quat& q) const {
#if FUN_PLATFORM_ENABLE_VECTORINTRINSICS
  const VectorRegister a = VectorLoadAligned(this);
  const VectorRegister b = VectorLoadAligned(&q);
  return VectorMaskBits(VectorCompareEQ(a, b)) == 0x0F;
#else
  return x == q.x && y == q.y && z == q.z && w == q.w;
#endif
}

FUN_ALWAYS_INLINE bool Quat::operator != (const Quat& q) const {
#if FUN_PLATFORM_ENABLE_VECTORINTRINSICS
  const VectorRegister a = VectorLoadAligned(this);
  const VectorRegister b = VectorLoadAligned(&q);
  return VectorMaskBits(VectorCompareNE(a, b)) != 0x00;
#else
  return x != q.x || y != q.y || z != q.z || w != q.w;
#endif
}

FUN_ALWAYS_INLINE float Quat::operator | (const Quat& q) const {
  return x*q.x + y*q.y + z*q.z + w*q.w;
}

FUN_ALWAYS_INLINE void Quat::Normalize(float tolerance) {
#if FUN_PLATFORM_ENABLE_VECTORINTRINSICS
  const VectorRegister vector = VectorLoadAligned(this);

  const VectorRegister length_sqr = VectorDot4(vector, vector);
  const VectorRegister non_zero_mask = VectorCompareGE(length_sqr, VectorLoadFloat1(&tolerance));
  const VectorRegister inv_length = VectorReciprocalSqrtAccurate(length_sqr);
  const VectorRegister normalized_vector = VectorMultiply(inv_length, vector);
  VectorRegister result = VectorSelect(non_zero_mask, normalized_vector, VectorizeConstants::Float0001);

  VectorStoreAligned(result, this);
#else
  const float length_sqr = x*x + y*y + z*z + w*w;

  if (length_sqr >= tolerance) {
    const float scale = Math::InvSqrt(length_sqr);

    x *= scale;
    y *= scale;
    z *= scale;
    w *= scale;
  } else {
    *this = Quat::Identity;
  }
#endif
}

FUN_ALWAYS_INLINE Quat Quat::GetNormalized(float tolerance) const {
  Quat result(*this);
  result.Normalize(tolerance);
  return result;
}

FUN_ALWAYS_INLINE bool Quat::IsNormalized() const {
  return Math::Abs(1.f - SizeSquared()) < THRESH_QUAT_NORMALIZED;
}

FUN_ALWAYS_INLINE float Quat::Size() const {
  return Math::Sqrt(x*x + y*y + z*z + w*w);
}

FUN_ALWAYS_INLINE float Quat::SizeSquared() const {
  return x*x + y*y + z*z + w*w;
}

FUN_ALWAYS_INLINE void Quat::ToAxisAndAngle(Vector& out_axis, float& out_angle) const {
  out_angle = 2.f * Math::Acos(w);
  out_axis = GetRotationAxis();
}

FUN_ALWAYS_INLINE Vector Quat::GetRotationAxis() const {
  // Ensure we never try to sqrt a neg number
  const float s = Math::Sqrt(Math::Max(1.f - (w * w), 0.f));
  if (s >= 0.0001f) {
    float one_over_scale = 1.f / s;
    return Vector(x * one_over_scale, y * one_over_scale, z * one_over_scale);
  }

  //TODO 이 값이 맞을지는 확인을...
  return Vector(1.f, 0.f, 0.f);
}

FUN_ALWAYS_INLINE Vector Quat::RotateVector(const Vector& v) const {
#if FUN_WITH_DIRECTXMATH
  Vector result;
  VectorQuaternionVector3Rotate(&result, &v, this);
  return result;

#else

  // http://people.csail.mit.edu/bkph/articles/Quaternions.pdf
  // v' = v + 2w(q x v) + (2Q x (q x v))
  // refactor:
  // v' = v + w(2(q x v)) + (q x (2(q x v)))
  // t = 2(q x v);
  // v' = v + w*(t) + (q x t)

  const Vector q(x, y, z);
  const Vector t = 2.f * Vector::CrossProduct(q, v);
  const Vector result = v + (w * t) + Vector::CrossProduct(q, t);
  return result;
#endif
}

FUN_ALWAYS_INLINE Vector Quat::UnrotateVector(const Vector& v) const {
#if FUN_WITH_DIRECTXMATH
  Vector result;
  VectorQuaternionVector3InverseRotate(&result, &v, this);
  return result;
#else
  //return Inverse().RotateVector(v);

  const Vector q(-x, -y, -z); // Inverse
  const Vector t = 2.f * Vector::CrossProduct(q, v);
  const Vector result = v + (w * t) + Vector::CrossProduct(q, t);
  return result;
#endif
}

FUN_ALWAYS_INLINE Quat Quat::Inverse() const {
  fun_check_dbg(IsNormalized());

  return Quat(-x, -y, -z, w);
}

FUN_ALWAYS_INLINE void Quat::EnforceShortestArcWith(const Quat& other_quat) {
  const float dot = (other_quat | *this);
  const float bias = Math::FloatSelect(dot, 1.f, -1.f);

  x *= bias;
  y *= bias;
  z *= bias;
  w *= bias;
}

FUN_ALWAYS_INLINE Vector Quat::GetAxisX() const {
  return RotateVector(Vector(1.f, 0.f, 0.f));
}

FUN_ALWAYS_INLINE Vector Quat::GetAxisY() const {
  return RotateVector(Vector(0.f, 1.f, 0.f));
}

FUN_ALWAYS_INLINE Vector Quat::GetAxisZ() const {
  return RotateVector(Vector(0.f, 0.f, 1.f));
}

FUN_ALWAYS_INLINE Vector Quat::GetForwardVector() const {
  return GetAxisX();
}

FUN_ALWAYS_INLINE Vector Quat::GetRightVector() const {
  return GetAxisY();
}

FUN_ALWAYS_INLINE Vector Quat::GetUpVector() const {
  return GetAxisZ();
}

FUN_ALWAYS_INLINE Vector Quat::ToVector() const {
  return GetAxisX();
}

FUN_ALWAYS_INLINE float Quat::Error(const Quat& q1, const Quat& q2) {
  const float cosom = Math::Abs(q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w);
  return (Math::Abs(cosom) < 0.9999999f) ? Math::Acos(cosom)*(1.f / PI) : 0.f;
}

FUN_ALWAYS_INLINE float Quat::ErrorAutoNormalize(const Quat& a, const Quat& b) {
  Quat q1 = a;
  q1.Normalize();

  Quat q2 = b;
  q2.Normalize();

  return Quat::Error(q1, q2);
}

FUN_ALWAYS_INLINE Quat Quat::FastLerp(const Quat& a, const Quat& b, const float alpha) {
  // to ensure the 'shortest route', we make sure the dot product
  // between the both rotations is positive.
  const float dot = (a | b);
  const float bias = Math::FloatSelect(dot, 1.f, -1.f);
  return (b * alpha) + (a * (bias * (1.f - alpha)));
}

FUN_ALWAYS_INLINE Quat Quat::FastBilerp( const Quat& p00,
                              const Quat& p10,
                              const Quat& p01,
                              const Quat& p11,
                              float frac_x,
                              float frac_y) {
  return Quat::FastLerp(
              Quat::FastLerp(p00, p10, frac_x),
              Quat::FastLerp(p01, p11, frac_x),
              frac_y);
}

FUN_ALWAYS_INLINE bool Quat::ContainsNaN() const {
  return (Math::IsNaN(x) || !Math::IsFinite(x) ||
          Math::IsNaN(y) || !Math::IsFinite(y) ||
          Math::IsNaN(z) || !Math::IsFinite(z) ||
          Math::IsNaN(w) || !Math::IsFinite(w)
  );
}

template <> struct IsPOD<Quat> { enum { Value = true }; };

} // namespace fun
