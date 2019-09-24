#pragma once

#include "fun/base/base.h"

namespace fun {

/**
 * Implements a container for rotation information.
 *
 * All rotation values are stored in degrees.
 */
class Rotator {
 public:
  /**
   * Rotation around the right axis (around y axis), Looking up and down (0=Straight Ahead, +Up, -Down)
   */
  float pitch;

  /**
   * Rotation around the up axis (around z axis), Running in circles 0=East, +North, -South.
   */
  float yaw;

  /**
   * Rotation around the forward axis (around x axis), Tilting your head, 0=Straight, +Clockwise, -CCW.
   */
  float roll;

  /**
   * A rotator of zero degrees on each axis.
   */
  static FUN_BASE_API const Rotator ZeroRotator;

#if FUN_ENABLE_NAN_DIAGNOSTIC

  FUN_ALWAYS_INLINE void DiagnosticCheckNaN() const {
    if (ContainsNaN()) {
      LOG_OR_ENSURE_NAN_ERROR("Rotator contains NaN: %s", *ToString());
      *const_cast<Rotator*>(this) = ZeroRotator;
    }
  }

#else

  FUN_ALWAYS_INLINE void DiagnosticCheckNaN() const {}

#endif

  FUN_ALWAYS_INLINE Rotator() {
    // NOOP
  }

  explicit Rotator(float f);

  FUN_ALWAYS_INLINE Rotator(float pitch, float yaw, float roll);

  explicit Rotator(ForceInit_TAG);

  explicit FUN_BASE_API Rotator(const Quat& quat);

  Rotator operator + (const Rotator& r) const;
  Rotator operator - (const Rotator& r) const;
  Rotator operator * (float scale) const;
  Rotator operator *= (float scale);

  bool operator == (const Rotator& r) const;
  bool operator != (const Rotator& v) const;

  Rotator operator += (const Rotator& r);
  Rotator operator -= (const Rotator& r);

  bool IsNearlyZero(float tolerance = KINDA_SMALL_NUMBER) const;
  bool IsZero() const;
  bool Equals(const Rotator& r, float tolerance = KINDA_SMALL_NUMBER) const;

  Rotator Add(float delta_pitch, float delta_yaw, float delta_roll);

  FUN_BASE_API Rotator GetInverse() const;

  Rotator GridSnap(const Rotator& grid) const;

  FUN_BASE_API Vector ToVector() const;
  FUN_BASE_API Quat ToQuaternion() const;
  FUN_BASE_API Vector ToEuler() const;

  FUN_BASE_API Vector RotateVector(const Vector& v) const;

  FUN_BASE_API Vector UnrotateVector(const Vector& v) const;

  Rotator Clamped() const;

  Rotator GetNormalized() const;

  Rotator GetDenormalized() const;

  void Normalize();

  FUN_BASE_API void GetWindingAndRemainder(Rotator& out_winding, Rotator& out_remainder) const;

  String ToString() const;
  String ToCompactString() const;
  bool InitFromString(const String& string);

  bool ContainsNaN() const;

  FUN_BASE_API void SerializeCompressed(Archive& ar);
  FUN_BASE_API void SerializeCompressedShort(Archive& ar);
  FUN_BASE_API bool NetSerialize(Archive& ar, class RPackageMap* map, bool& out_success);

  static float ClampAxis(float degrees);
  static float NormalizeAxis(float degrees);
  static uint8 CompressAxisToByte(float degrees);
  static float DecompressAxisFromByte(uint16 degrees);
  static uint16 CompressAxisToShort(float degrees);
  static float DecompressAxisFromShort(uint16 degrees);
  static FUN_BASE_API Rotator MakeFromEuler(const Vector& euler);

  friend Archive& operator & (Archive& ar, Rotator& r) {
    ar & r.pitch & r.yaw & r.roll;
    return ar;
  }

  bool Serialize(Archive& ar) {
    ar & *this;
    return true;
  }
};


//
// inlines
//

FUN_ALWAYS_INLINE Rotator operator * (float scale, const Rotator& r) {
  return r.operator*(scale);
}

FUN_ALWAYS_INLINE Rotator::Rotator(float f)
  : pitch(f), yaw(f), roll(f) {
  DiagnosticCheckNaN();
}

FUN_ALWAYS_INLINE Rotator::Rotator(float pitch, float yaw, float roll)
  : pitch(pitch), yaw(yaw), roll(roll) {
  DiagnosticCheckNaN();
}

FUN_ALWAYS_INLINE Rotator::Rotator(ForceInit_TAG)
  : pitch(0), yaw(0), roll(0) {}

FUN_ALWAYS_INLINE Rotator Rotator::operator + (const Rotator& r) const {
  return Rotator(pitch + r.pitch, yaw + r.yaw, roll + r.roll);
}

FUN_ALWAYS_INLINE Rotator Rotator::operator - (const Rotator& r) const {
  return Rotator(pitch - r.pitch, yaw - r.yaw, roll - r.roll);
}

FUN_ALWAYS_INLINE Rotator Rotator::operator * (float scale) const {
  return Rotator(pitch * scale, yaw * scale, roll * scale);
}

FUN_ALWAYS_INLINE Rotator Rotator::operator *= (float scale) {
  pitch *= scale;
  yaw *= scale;
  roll *= scale;
  DiagnosticCheckNaN();
  return *this;
}

FUN_ALWAYS_INLINE bool Rotator::operator == (const Rotator& r) const {
  return pitch == r.pitch && yaw == r.yaw && roll == r.roll;
}

FUN_ALWAYS_INLINE bool Rotator::operator != (const Rotator& v) const {
  return pitch != v.pitch || yaw != v.yaw || roll != v.roll;
}

FUN_ALWAYS_INLINE Rotator Rotator::operator += (const Rotator& r) {
  pitch += r.pitch;
  yaw += r.yaw;
  roll += r.roll;
  DiagnosticCheckNaN();
  return *this;
}

FUN_ALWAYS_INLINE Rotator Rotator::operator -= (const Rotator& r) {
  pitch -= r.pitch; yaw -= r.yaw; roll -= r.roll;
  DiagnosticCheckNaN();
  return *this;
}

FUN_ALWAYS_INLINE bool Rotator::IsNearlyZero(float tolerance) const {
#if FUN_PLATFORM_ENABLE_VECTORINTRINSICS
  const VectorRegister reg_a = VectorLoadFloat3_W0(this);
  const VectorRegister norm = VectorNormalizeRotator(reg_a);
  const VectorRegister abs_norm = VectorAbs(norm);
  return !VectorAnyGreaterThan(abs_norm, VectorLoadFloat1(&tolerance));
#else
  return
    Math::Abs(NormalizeAxis(pitch)) <= tolerance
    && Math::Abs(NormalizeAxis(yaw)) <= tolerance
    && Math::Abs(NormalizeAxis(roll)) <= tolerance;
#endif
}

FUN_ALWAYS_INLINE bool Rotator::IsZero() const {
  return (ClampAxis(pitch) == 0.f) && (ClampAxis(yaw) == 0.f) && (ClampAxis(roll) == 0.f);
}

FUN_ALWAYS_INLINE bool Rotator::Equals(const Rotator& r, float tolerance) const {
#if FUN_PLATFORM_ENABLE_VECTORINTRINSICS
  const VectorRegister reg_a = VectorLoadFloat3_W0(this);
  const VectorRegister reg_b = VectorLoadFloat3_W0(&r);
  const VectorRegister norm_delta = VectorNormalizeRotator(VectorSubtract(reg_a, reg_b));
  const VectorRegister abs_norm_delta = VectorAbs(norm_delta);
  return !VectorAnyGreaterThan(abs_norm_delta, VectorLoadFloat1(&tolerance));
#else
  return (Math::Abs(NormalizeAxis(pitch - r.pitch)) <= tolerance)
      && (Math::Abs(NormalizeAxis(yaw - r.yaw)) <= tolerance)
      && (Math::Abs(NormalizeAxis(roll - r.roll)) <= tolerance);
#endif
}

FUN_ALWAYS_INLINE Rotator Rotator::Add(float delta_pitch, float delta_yaw, float delta_roll) {
  yaw += delta_yaw;
  pitch += delta_pitch;
  roll += delta_roll;
  DiagnosticCheckNaN();
  return *this;
}

FUN_ALWAYS_INLINE Rotator Rotator::GridSnap(const Rotator& grid) const {
  return Rotator( Math::GridSnap(pitch, grid.pitch),
                  Math::GridSnap(yaw, grid.yaw),
                  Math::GridSnap(roll, grid.roll));
}

FUN_ALWAYS_INLINE Rotator Rotator::Clamped() const {
  return Rotator(ClampAxis(pitch), ClampAxis(yaw), ClampAxis(roll));
}

FUN_ALWAYS_INLINE float Rotator::ClampAxis(float degrees) {
  // returns degrees in the range (-360, 360)
  degrees = Math::Fmod(degrees, 360.f);

  if (degrees < 0.f) {
    // shift to [0, 360) range
    degrees += 360.f;
  }

  return degrees;
}

FUN_ALWAYS_INLINE float Rotator::NormalizeAxis(float degrees) {
  // returns degrees in the range [0, 360)
  degrees = ClampAxis(degrees);

  if (degrees > 180.f) {
    // shift to (-180, 180]
    degrees -= 360.f;
  }

  return degrees;
}

FUN_ALWAYS_INLINE uint8 Rotator::CompressAxisToByte(float degrees) {
  // map [0->360) to [0->256) and mask off any winding
  return Math::RoundToInt(degrees * 256.f / 360.f) & 0xFF;
}

FUN_ALWAYS_INLINE float Rotator::DecompressAxisFromByte(uint16 degrees) {
  // map [0->256) to [0->360)
  return (degrees * 360.f / 256.f);
}

FUN_ALWAYS_INLINE uint16 Rotator::CompressAxisToShort(float degrees) {
  // map [0->360) to [0->65536) and mask off any winding
  return Math::RoundToInt(degrees * 65536.f / 360.f) & 0xFFFF;
}

FUN_ALWAYS_INLINE float Rotator::DecompressAxisFromShort(uint16 degrees) {
  // map [0->65536) to [0->360)
  return (degrees * 360.f / 65536.f);
}

FUN_ALWAYS_INLINE Rotator Rotator::GetNormalized() const {
  Rotator rot = *this;
  rot.Normalize();
  return rot;
}

FUN_ALWAYS_INLINE Rotator Rotator::GetDenormalized() const {
  Rotator rot = *this;
  rot.pitch = ClampAxis(rot.pitch);
  rot.yaw = ClampAxis(rot.yaw);
  rot.roll = ClampAxis(rot.roll);
  return rot;
}

FUN_ALWAYS_INLINE void Rotator::Normalize() {
#if FUN_PLATFORM_ENABLE_VECTORINTRINSICS
  VectorRegister v_rotator = VectorLoadFloat3_W0(this);
  v_rotator = VectorNormalizeRotator(v_rotator);
  VectorStoreFloat3(v_rotator, this);
#else
  pitch = NormalizeAxis(pitch);
  yaw = NormalizeAxis(yaw);
  roll = NormalizeAxis(roll);
#endif
  DiagnosticCheckNaN();
}

FUN_ALWAYS_INLINE String Rotator::ToString() const {
  return String::Format("p=%f y=%f r=%f", pitch, yaw, roll);
}

FUN_ALWAYS_INLINE String Rotator::ToCompactString() const {
  if (IsNearlyZero()) {
    return String::Format("r(0)");
  }

  String ret("r(");
  bool is_empty_string = true;
  if (!Math::IsNearlyZero(pitch)) {
    ret += String::Format("p=%.2f", pitch);
    is_empty_string = false;
  }
  if (!Math::IsNearlyZero(yaw)) {
    if (!is_empty_string) {
      ret += String(",");
    }
    ret += String::Format("y=%.2f", yaw);
    is_empty_string = false;
  }
  if (!Math::IsNearlyZero(roll)) {
    if (!is_empty_string) {
      ret += String(",");
    }
    ret += String::Format("r=%.2f", roll);
    is_empty_string = false;
  }
  ret += String(")");
  return ret;
}

FUN_ALWAYS_INLINE bool Rotator::InitFromString(const String& string) {
  pitch = yaw = roll = 0.f;

  // The initialization is only successful if the x, y, and z values can all be parsed from the string
  const bool ok =
        Parse::Value(*string, "p=", pitch) &&
        Parse::Value(*string, "y=", yaw) &&
        Parse::Value(*string, "r=", roll);
  DiagnosticCheckNaN();
  return ok;
}

FUN_ALWAYS_INLINE bool Rotator::ContainsNaN() const {
  return (Math::IsNaN(pitch) || !Math::IsFinite(pitch)
        || Math::IsNaN(yaw) || !Math::IsFinite(yaw)
        || Math::IsNaN(roll) || !Math::IsFinite(roll));
}

template <> struct IsPOD<Rotator> { enum { Value = true }; };

} // namespace fun
