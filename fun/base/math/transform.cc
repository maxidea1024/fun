#include "CorePrivatePCH.h"

#if !ENABLE_VECTORIZED_TRANSFORM

#include "DefaultValueHelper.h"

namespace fun {

DEFINE_LOG_CATEGORY_STATIC(LogTransform, Info, All);

// Transform identity
const Transform Transform::Identity(Quat(0.f,0.f,0.f,1.f), Vector::ZeroVector, Vector(1.f));

// Replacement of Inverse of Matrix

void Transform::DebugPrint() const {
  fun_log(LogTransform, Info, "%s", *ToHumanReadableString());
}

String Transform::ToHumanReadableString() const {
  Quat r(GetRotation());
  Vector t(GetTranslation());
  Vector s(GetScale3D());

  String ret = String::Format("Rotation: %f %f %f %f\r\n", r.x, r.y, r.z, r.w);
  ret += String::Format("translation: %f %f %f\r\n", t.x, t.y, t.z);
  ret += String::Format("Scale3D: %f %f %f\r\n", s.x, s.y, s.z);
  return ret;
}

String Transform::ToString() const {
  const Rotator r(rotator());
  const Vector t(GetTranslation());
  const Vector s(GetScale3D());

  return String::Format("%f,%f,%f|%f,%f,%f|%f,%f,%f", t.x, t.y, t.z, r.pitch, r.yaw, r.roll, s.x, s.y, s.z);
}

bool Transform::InitFromString(const String& string) {
  Array<String> component_strings;
  string.ParseIntoArray(component_strings, "|", true);
  const int32 component_count = component_strings.Count();
  if (3 != component_count) {
    return false;
  }

  // translation
  Vector parsed_translation = Vector::ZeroVector;
  if (!CDefaultValueHelper::ParseVector(component_strings[0], parsed_translation)) {
    return false;
  }

  // Rotation
  Rotator parsed_rotation = Rotator::ZeroRotator;
  if (!CDefaultValueHelper::ParseRotator(component_strings[1], parsed_rotation)) {
    return false;
  }

  // Scale
  Vector parsed_scale = Vector(1.f);
  if (!CDefaultValueHelper::ParseVector(component_strings[2], parsed_scale)) {
    return false;
  }

  SetComponents(Quat(parsed_rotation), parsed_translation, parsed_scale);

  return true;
}

#define DEBUG_INVERSE_TRANSFORM  0

Transform Transform::GetRelativeTransformReverse(const Transform& other) const {
  // A (-1) * B = VQS(B)(VQS (A)(-1))
  //
  // scale = S(B)/S(A)
  // rotation = q(B) * q(A)(-1)
  // translation = T(B)-S(B)/S(A) *[q(B)*q(A)(-1)*T(A)*q(A)*q(B)(-1)]
  // where A = this, and B = other
  Transform result;

  Vector safe_recip_scale = GetSafeScaleReciprocal(scale);
  result.scale = other.scale*safe_recip_scale;

  result.rotation = other.rotation*rotation.Inverse();

  result.translation = other.translation - result.scale * (result.rotation * translation);

#if DEBUG_INVERSE_TRANSFORM
  Matrix am = ToMatrixWithScale();
  Matrix bm = other.ToMatrixWithScale();

  result.DebugEqualMatrix(am.InverseFast() *  bm);
#endif

  return result;
}

void Transform::SetToRelativeTransform(const Transform& parent_transform) {
  // A * B(-1) = VQS(B)(-1) (VQS (A))
  //
  // scale = S(A)/S(B)
  // rotation = q(B)(-1) * q(A)
  // translation = 1/S(B) *[q(B)(-1)*(T(A)-T(B))*q(B)]
  // where A = this, B = other
#if DEBUG_INVERSE_TRANSFORM
  Matrix am = ToMatrixWithScale();
  Matrix bm = parent_transform.ToMatrixWithScale();
#endif

  const Vector safe_recip_scale = GetSafeScaleReciprocal(parent_transform.Scale3D);
  const Quat inverse_rot = parent_transform.rotation.Inverse();

  Scale3D *= safe_recip_scale;
  translation = (inverse_rot * (translation - parent_transform.translation)) * safe_recip_scale;
  rotation = inverse_rot * rotation;

#if DEBUG_INVERSE_TRANSFORM
  DebugEqualMatrix(am *  bm.InverseFast());
#endif
}

Transform Transform::GetRelativeTransform(const Transform& other) const {
  // A * B(-1) = VQS(B)(-1) (VQS (A))
  //
  // scale = S(A)/S(B)
  // rotation = q(B)(-1) * q(A)
  // translation = 1/S(B) *[q(B)(-1)*(T(A)-T(B))*q(B)]
  // where A = this, B = other
  Transform result;

  Vector safe_recip_scale = GetSafeScaleReciprocal(other.scale);
  result.scale = scale*safe_recip_scale;

  if (other.rotation.IsNormalized() == false) {
    return Transform::Identity;
  }

  Quat inverse = other.rotation.Inverse();
  result.rotation = inverse*rotation;

  result.translation = (inverse*(translation - other.translation))*(safe_recip_scale);

#if DEBUG_INVERSE_TRANSFORM
  Matrix am = ToMatrixWithScale();
  Matrix bm = other.ToMatrixWithScale();

  result.DebugEqualMatrix(am * bm.InverseFast());
#endif

  return result;
}

bool Transform::DebugEqualMatrix(const Matrix& matrix) const {
  Transform test_result(matrix);
  if (!Equals(test_result)) {
    // see now which one isn't equal
    if (!scale.Equals(test_result.scale, 0.01f)) {
      fun_log(LogTransform, Info, "matrix(S)\t%s", *test_result.scale.ToString());
      fun_log(LogTransform, Info, "VQS(S)\t%s", *scale.ToString());
    }

    // see now which one isn't equal
    if (!rotation.Equals(test_result.rotation)) {
      fun_log(LogTransform, Info, "matrix(r)\t%s", *test_result.rotation.ToString());
      fun_log(LogTransform, Info, "VQS(r)\t%s", *rotation.ToString());
    }

    // see now which one isn't equal
    if (!translation.Equals(test_result.translation, 0.01f)) {
      fun_log(LogTransform, Info, "matrix(T)\t%s", *test_result.translation.ToString());
      fun_log(LogTransform, Info, "VQS(T)\t%s", *translation.ToString());
    }
    return false;
  }

  return true;
}

} // namespace fun

#endif //#if !ENABLE_VECTORIZED_TRANSFORM
