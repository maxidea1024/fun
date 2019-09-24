#include "CorePrivatePCH.h"

#if ENABLE_VECTORIZED_TRANSFORM

#include "Misc/DefaultValueHelper.h"

namespace fun {

DEFINE_LOG_CATEGORY_STATIC(LogTransform, Info, All);

// Transform identity
const Transform Transform::Identity(Quat(0.f, 0.f, 0.f, 1.f), Vector::ZeroVector, Vector(1.f));


// Replacement of Inverse of Matrix

void Transform::DebugPrint() const {
  fun_log(LogTransform, Info, "%s", *ToHumanReadableString());
}

String Transform::ToHumanReadableString() const {
  Quat r(GetRotation());
  Vector T(GetTranslation());
  Vector S(GetScale3D());

  String Output = String::Format("Rotation: %f %f %f %f\r\n", r.x, r.y, r.z, r.w);
  Output += String::Format("translation: %f %f %f\r\n", T.x, T.y, T.z);
  Output += String::Format("Scale3D: %f %f %f\r\n", S.x, S.y, S.z);

  return Output;
}

String Transform::ToString() const {
  const Rotator r(rotator());
  const Vector T(GetTranslation());
  const Vector S(GetScale3D());

  return String::Format("%f, %f, %f|%f, %f, %f|%f, %f, %f), T.x, T.y, T.z, r.pitch, r.yaw, r.roll, S.x, S.y, S.z);
}

bool Transform::InitFromString(const String& Source) {
  Array<String> component_strings;
  Source.Split(component_strings, "|", 0, StringSplitOption::CullEmpty);
  const int32 component_count = component_strings.Count();
  if (3 != component_count) {
    return false;
  }

  // translation
  Vector ParsedTranslation = Vector::ZeroVector;
  if (!CDefaultValueHelper::ParseVector(component_strings[0], ParsedTranslation)) {
    return false;
  }

  // Rotation
  Rotator ParsedRotation = Rotator::ZeroRotator;
  if (!CDefaultValueHelper::ParseRotator(component_strings[1], ParsedRotation)) {
    return false;
  }

  // Scale
  Vector Scale = Vector(1.f);
  if (!CDefaultValueHelper::ParseVector(component_strings[2], Scale)) {
    return false;
  }

  SetComponents(Quat(ParsedRotation), ParsedTranslation, Scale);

  return true;
}


#define DEBUG_INVERSE_TRANSFORM  0

Transform Transform::GetRelativeTransformReverse(const Transform& other) const {
  // A (-1) * B = VQS(B)(VQS (A)(-1))
  //
  // Scale = S(B)/S(A)
  // Rotation = q(B) * q(A)(-1)
  // translation = T(B)-S(B)/S(A) *[q(B)*q(A)(-1)*T(A)*q(A)*q(B)(-1)]
  // where A = this, and B = other
  Transform result;

  // Scale = S(B)/S(A)
  VectorRegister VSafeScale3D = VectorSet_W0(GetSafeScaleReciprocal(Scale3D));
  VectorRegister VScale3D = VectorMultiply(other.Scale3D, VSafeScale3D);

  // Rotation = q(B) * q(A)(-1)
  VectorRegister VInverseRot = VectorQuaternionInverse(Rotation);
  VectorRegister VRotation = VectorQuaternionMultiply2(other.Rotation, VInverseRot);

  // RotatedTranslation
  VectorRegister VR = VectorQuaternionRotateVector(VRotation, translation);

  // translation = T(B)-S(B)/S(A) *[q(B)*q(A)(-1)*T(A)*q(A)*q(B)(-1)]
  VectorRegister VTranslation = VectorSet_W0(VectorSubtract(other.translation, VectorMultiply(VScale3D, VR)));

  result.Scale3D = VScale3D;
  result.translation = VTranslation;
  result.Rotation = VRotation;

  result.DiagnosticCheckNaN_All();

#if DEBUG_INVERSE_TRANSFORM
  Matrix AM = ToMatrixWithScale();
  Matrix BM = other.ToMatrixWithScale();

  result.DebugEqualMatrix(AM.InverseFast() *  BM);
#endif

  return result;
}

void Transform::SetToRelativeTransform(const Transform& parent_transform) {
  // A * B(-1) = VQS(B)(-1) (VQS (A))
  //
  // Scale = S(A)/S(B)
  // Rotation = q(B)(-1) * q(A)
  // translation = 1/S(B) *[q(B)(-1)*(T(A)-T(B))*q(B)]
  // where A = this, B = other
#if DEBUG_INVERSE_TRANSFORM
  Matrix AM = ToMatrixWithScale();
  Matrix BM = parent_transform.ToMatrixWithScale();
#endif

  fun_check_dbg(parent_transform.IsRotationNormalized());

  // Scale = S(A)/S(B)
  VectorRegister VSafeScale3D = VectorSet_W0(GetSafeScaleReciprocal(parent_transform.Scale3D));
  Scale3D = VectorMultiply(Scale3D, VSafeScale3D);

  //VQTranslation = ((T(A).x - T(B).x), (T(A).y - T(B).y), (T(A).z - T(B).z), 0.f);
  VectorRegister VQTranslation = VectorSet_W0(VectorSubtract(translation, parent_transform.translation));

  // Inverse RotatedTranslation
  VectorRegister VInverseParentRot = VectorQuaternionInverse(parent_transform.Rotation);
  VectorRegister VR = VectorQuaternionRotateVector(VInverseParentRot, VQTranslation);

  // translation = 1/S(B)
  translation = VectorMultiply(VR, VSafeScale3D);

  // Rotation = q(B)(-1) * q(A)
  Rotation = VectorQuaternionMultiply2(VInverseParentRot, Rotation);

  DiagnosticCheckNaN_All();

#if DEBUG_INVERSE_TRANSFORM
  DebugEqualMatrix(AM *  BM.InverseFast());
#endif
}

Transform Transform::GetRelativeTransform(const Transform& other) const {
  // A * B(-1) = VQS(B)(-1) (VQS (A))
  //
  // Scale = S(A)/S(B)
  // Rotation = q(B)(-1) * q(A)
  // translation = 1/S(B) *[q(B)(-1)*(T(A)-T(B))*q(B)]
  // where A = this, B = other
  Transform result;

  if (other.IsRotationNormalized() == false) {
    return Transform::Identity;
  }

  // Scale = S(A)/S(B)
  static ScalarRegister STolerance(SMALL_NUMBER);
  VectorRegister VSafeScale3D = VectorSet_W0(GetSafeScaleReciprocal(other.Scale3D, STolerance));

  VectorRegister VScale3D = VectorMultiply(Scale3D, VSafeScale3D);

  //VQTranslation = ((T(A).x - T(B).x), (T(A).y - T(B).y), (T(A).z - T(B).z), 0.f);
  VectorRegister VQTranslation =  VectorSet_W0(VectorSubtract(translation, other.translation));

  // Inverse RotatedTranslation
  VectorRegister VInverseRot = VectorQuaternionInverse(other.Rotation);
  VectorRegister VR = VectorQuaternionRotateVector(VInverseRot, VQTranslation);

  //translation = 1/S(B)
  VectorRegister VTranslation = VectorMultiply(VR, VSafeScale3D);

  // Rotation = q(B)(-1) * q(A)
  VectorRegister VRotation = VectorQuaternionMultiply2(VInverseRot, Rotation);

  result.Scale3D = VScale3D;
  result.translation = VTranslation;
  result.Rotation = VRotation;

  result.DiagnosticCheckNaN_All();
#if DEBUG_INVERSE_TRANSFORM
  Matrix AM = ToMatrixWithScale();
  Matrix BM = other.ToMatrixWithScale();

  result.DebugEqualMatrix(AM *  BM.InverseFast());

#endif
  return result;
}

bool Transform::DebugEqualMatrix(const Matrix& matrix) const {
  Transform TestResult(matrix);
  if (!Equals(TestResult)) {
    // see now which one isn't equal
    if (!Scale3DEquals(TestResult, 0.01f)) {
      fun_log(LogTransform, Info, "matrix(S)\t%s", *TestResult.GetScale3D().ToString());
      fun_log(LogTransform, Info, "VQS(S)\t%s", *GetScale3D().ToString());
    }

    // see now which one isn't equal
    if (!RotationEquals(TestResult)) {
      fun_log(LogTransform, Info, "matrix(r)\t%s", *TestResult.GetRotation().ToString());
      fun_log(LogTransform, Info, "VQS(r)\t%s", *GetRotation().ToString());
    }

    // see now which one isn't equal
    if (!TranslationEquals(TestResult, 0.01f)) {
      fun_log(LogTransform, Info, "matrix(T)\t%s", *TestResult.GetTranslation().ToString());
      fun_log(LogTransform, Info, "VQS(T)\t%s", *GetTranslation().ToString());
    }
    return false;
  }

  return true;
}

} // namespace fun

#endif //ENABLE_VECTORIZED_TRANSFORM
