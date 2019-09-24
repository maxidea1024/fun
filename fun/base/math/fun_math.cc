#include "CorePrivatePCH.h"
//#include "PropertyPortFlags.h"

namespace fun {

DEFINE_LOG_CATEGORY(LogFunMath);

//
// Math stats
//

//TODO?
//DECLARE_CYCLE_STAT("Convert rotator to quat", STAT_MathConvertRotatorToQuat, STATGROUP_MathVerbose);
//DECLARE_CYCLE_STAT("Convert quat to rotator", STAT_MathConvertQuatToRotator, STATGROUP_MathVerbose);


//
// Globals
//

const Vector Vector::ZeroVector(0.f, 0.f, 0.f);
const Vector Vector::up_vector(0.f, 0.f, 1.f);
const Vector Vector::ForwardVector(1.f, 0.f, 0.f);
const Vector Vector::RightVector(0.f, 1.f, 0.f);
const Vector2 Vector2::ZeroVector(0.f, 0.f);
const Vector2 Vector2::UnitVector(1.f, 1.f);
const Rotator Rotator::ZeroRotator(0.f, 0.f, 0.f);

const VectorRegister VECTOR_INV_255 = DECLARE_VECTOR_REGISTER(1.f/255.f, 1.f/255.f, 1.f/255.f, 1.f/255.f);

const uint32 Math::BitFlag[32] = {
  (1U << 0),  (1U << 1),  (1U << 2),  (1U << 3),
  (1U << 4),  (1U << 5),  (1U << 6),  (1U << 7),
  (1U << 8),  (1U << 9),  (1U << 10), (1U << 11),
  (1U << 12), (1U << 13), (1U << 14), (1U << 15),
  (1U << 16), (1U << 17), (1U << 18), (1U << 19),
  (1U << 20), (1U << 21), (1U << 22), (1U << 23),
  (1U << 24), (1U << 25), (1U << 26), (1U << 27),
  (1U << 28), (1U << 29), (1U << 30), (1U << 31),
};


const IntPoint IntPoint::ZeroValue(0, 0);
const IntPoint IntPoint::NoneValue(INVALID_INDEX, INVALID_INDEX);
const IntVector IntVector::ZeroValue(0, 0, 0);
const IntVector IntVector::NoneValue(INVALID_INDEX, INVALID_INDEX, INVALID_INDEX);


/** Vectors NetSerialize without quantization. Use the CVectors_NetQuantize etc (NetSerialization.h) instead. */
bool Vector::NetSerialize(Archive& ar, class RPackageMap* map, bool& out_success) {
  ar & x & y & z;
  return true;
}

bool Vector2::NetSerialize(Archive& ar, class RPackageMap* map, bool& out_success) {
  ar & x & y;
  return true;
}

bool Rotator::NetSerialize(Archive& ar, class RPackageMap* map, bool& out_success) {
  SerializeCompressedShort(ar);
  out_success = true;
  return true;
}

void Rotator::SerializeCompressed(Archive& ar) {
  uint8 byte_pitch = Rotator::CompressAxisToByte(pitch);
  uint8 byte_yaw = Rotator::CompressAxisToByte(yaw);
  uint8 byte_roll = Rotator::CompressAxisToByte(roll);

  uint8 b = (byte_pitch != 0);
  ar.SerializeBits(&b, 1);
  if (b) {
    ar & byte_pitch;
  } else {
    byte_pitch = 0;
  }

  b = (byte_yaw != 0);
  ar.SerializeBits(&b, 1);
  if (b) {
    ar & byte_yaw;
  } else {
    byte_yaw = 0;
  }

  b = (byte_roll != 0);
  ar.SerializeBits(&b, 1);
  if (b) {
    ar & byte_roll;
  } else {
    byte_roll = 0;
  }

  if (ar.IsLoading()) {
    pitch = Rotator::DecompressAxisFromByte(byte_pitch);
    yaw = Rotator::DecompressAxisFromByte(byte_yaw);
    roll = Rotator::DecompressAxisFromByte(byte_roll);
  }
}

void Rotator::SerializeCompressedShort(Archive& ar) {
  uint16 short_pitch = Rotator::CompressAxisToShort(pitch);
  uint16 short_yaw = Rotator::CompressAxisToShort(yaw);
  uint16 short_roll = Rotator::CompressAxisToShort(roll);

  uint8 b = (short_pitch != 0);
  ar.SerializeBits(&b, 1);
  if (b) {
    ar & short_pitch;
  } else {
    short_pitch = 0;
  }

  b = (short_yaw != 0);
  ar.SerializeBits(&b, 1);
  if (b) {
    ar & short_yaw;
  } else {
    short_yaw = 0;
  }

  b = (short_roll != 0);
  ar.SerializeBits(&b, 1);
  if (b) {
    ar & short_roll;
  } else {
    short_roll = 0;
  }

  if (ar.IsLoading()) {
    pitch = Rotator::DecompressAxisFromShort(short_pitch);
    yaw = Rotator::DecompressAxisFromShort(short_yaw);
    roll = Rotator::DecompressAxisFromShort(short_roll);
  }
}

Rotator Vector::ToOrientationRotator() const {
  Rotator r;

  // Find yaw.
  r.yaw = Math::Atan2(y, x) * RADIANS_TO_DEGREES;

  // Find pitch.
  r.pitch = Math::Atan2(z, Math::Sqrt(x*x + y*y)) * RADIANS_TO_DEGREES;

  // Find roll.
  r.roll = 0;

#if FUN_ENABLE_NAN_DIAGNOSTIC
  if (r.ContainsNaN()) {
    LOG_OR_ENSURE_NAN_ERROR("Vector::Rotation(): rotator result %s contains NaN! Input Vector = %s", *r.ToString(), *this->ToString());
    r = Rotator::ZeroRotator;
  }
#endif

  return r;
}

Rotator Vector4::ToOrientationRotator() const {
  Rotator r;

  // Find yaw.
  r.yaw = Math::Atan2(y, x) * RADIANS_TO_DEGREES;

  // Find pitch.
  r.pitch = Math::Atan2(z, Math::Sqrt(x*x + y*y)) * RADIANS_TO_DEGREES;

  // Find roll.
  r.roll = 0;

#if FUN_ENABLE_NAN_DIAGNOSTIC
  if (r.ContainsNaN()) {
    LOG_OR_ENSURE_NAN_ERROR("Vector4::Rotation(): rotator result %s contains NaN! Input Vector4 = %s", *r.ToString(), *this->ToString());
    r = Rotator::ZeroRotator;
  }
#endif

  return r;
}

Quat Vector::ToOrientationQuat() const {
  // Essentially an optimized vector->rotator->quat made possible by knowing roll == 0, and avoiding radians->degrees->radians.
  // This is done to avoid adding any roll (which our API states as a constraint).
  const float yaw_rad = Math::Atan2(y, x);
  const float pitch_rad = Math::Atan2(z, Math::Sqrt(x*x + y*y));

  float sp, sy;
  float cp, cy;
  Math::SinCos(&sp, &cp, pitch_rad * 0.5f);
  Math::SinCos(&sy, &cy, yaw_rad * 0.5f);

  Quat q;
  q.x =  sp*sy;
  q.y = -sp*cy;
  q.z =  cp*sy;
  q.w =  cp*cy;
  return q;
}

Quat Vector4::ToOrientationQuat() const {
  // Essentially an optimized vector->rotator->quat made possible by knowing roll == 0, and avoiding radians->degrees->radians.
  // This is done to avoid adding any roll (which our API states as a constraint).
  const float yaw_rad = Math::Atan2(y, x);
  const float pitch_rad = Math::Atan2(z, Math::Sqrt(x*x + y*y));

  float sp, sy;
  float cp, cy;
  Math::SinCos(&sp, &cp, pitch_rad * 0.5f);
  Math::SinCos(&sy, &cy, yaw_rad * 0.5f);

  Quat q;
  q.x =  sp*sy;
  q.y = -sp*cy;
  q.z =  cp*sy;
  q.w =  cp*cy;
  return q;
}

void Vector::FindBestAxisVectors(Vector& axis1, Vector& axis2) const {
  const float nx = Math::Abs(x);
  const float ny = Math::Abs(y);
  const float nz = Math::Abs(z);

  // Find best basis vectors.
  if (nz > nx && nz > ny) {
    axis1 = Vector(1, 0, 0);
  } else {
    axis1 = Vector(0, 0, 1);
  }

  axis1 = (axis1 - *this * (axis1 | *this)).GetSafeNormal();
  axis2 = axis1 ^ *this;
}

Vector Math::ClosestPointOnLine(const Vector& line_start, const Vector& line_end, const Vector& point) {
  // Solve to find alpha along line that is closest point
  // Weisstein, Eric w. "point-Line Distance--3-Dimensional."
  // From MathWorld--a Wolfram Web Resource.
  // http://mathworld.wolfram.com/point-LineDistance3-Dimensional.html

  const float a = (line_start - point) | (line_end - line_start);
  const float b = (line_end - line_start).SizeSquared();
  const float t = Math::Clamp01(-(a / b));

  // Generate closest point
  Vector closest_point = line_start + (t * (line_end - line_start));

  return closest_point;
}

void Vector::CreateOrthonormalBasis(Vector& x_axis, Vector& y_axis, Vector& z_axis) {
  // Project the x and y axes onto the plane perpendicular to the z axis.
  x_axis -= (x_axis | z_axis) / (z_axis | z_axis) * z_axis;
  y_axis -= (y_axis | z_axis) / (z_axis | z_axis) * z_axis;

  // If the x axis was parallel to the z axis, choose a vector which is orthogonal to the y and z axes.
  if (x_axis.SizeSquared() < DELTA*DELTA) {
    x_axis = y_axis ^ z_axis;
  }

  // If the y axis was parallel to the z axis, choose a vector which is orthogonal to the x and z axes.
  if (y_axis.SizeSquared() < DELTA*DELTA) {
    y_axis = x_axis ^ z_axis;
  }

  // Normalize the basis vectors.
  x_axis.Normalize();
  y_axis.Normalize();
  z_axis.Normalize();
}

void Vector::UnwindEuler() {
  x = Math::UnwindDegrees(x);
  y = Math::UnwindDegrees(y);
  z = Math::UnwindDegrees(z);
}

Rotator::Rotator(const Quat& quat) {
  *this = quat.rotator();
  DiagnosticCheckNaN();
}

Vector Rotator::ToVector() const {
  float cp, sp, cy, sy;
  Math::SinCos(&sp, &cp, Math::DegreesToRadians(pitch));
  Math::SinCos(&sy, &cy, Math::DegreesToRadians(yaw));
  return Vector(cp*cy, cp*sy, sp);
}

Rotator Rotator::GetInverse() const {
  return Quaternion().Inverse().Rotator();
}

Quat Rotator::ToQuaternion() const {
  //TODO?
  //SCOPED_CYCLE_COUNTER(STAT_MathConvertRotatorToQuat);

  DiagnosticCheckNaN();

#if FUN_PLATFORM_ENABLE_VECTORINTRINSICS
  const VectorRegister angles = MakeVectorRegister(pitch, yaw, roll, 0.f);
  const VectorRegister half_angles = VectorMultiply(angles, VectorizeConstants::DEG_TO_RAD_HALF);

  VectorRegister sin_angles, cos_angles;
  VectorSinCos(&sin_angles, &cos_angles, &half_angles);

  // Vectorized conversion, measured 20% faster than using scalar version after VectorSinCos.
  // Indices within VectorRegister (for shuffles): P=0, y=1, r=2
  const VectorRegister sr = VectorReplicate(sin_angles, 2);
  const VectorRegister cr = VectorReplicate(cos_angles, 2);

  const VectorRegister sy_sy_cy_cy_temp = VectorShuffle(sin_angles, cos_angles, 1, 1, 1, 1);

  const VectorRegister sp_sp_cp_cp = VectorShuffle(sin_angles, cos_angles, 0, 0, 0, 0);
  const VectorRegister sy_cy_sy_cy = VectorShuffle(sy_sy_cy_cy_temp, sy_sy_cy_cy_temp, 0, 2, 0, 2);

  const VectorRegister cp_cp_sp_sp = VectorShuffle(cos_angles, sin_angles, 0, 0, 0, 0);
  const VectorRegister cy_sy_cy_sy = VectorShuffle(sy_sy_cy_cy_temp, sy_sy_cy_cy_temp, 2, 0, 2, 0);

  const uint32 neg = uint32(1 << 31);
  const uint32 pos = uint32(0);
  const VectorRegister sign_bits_left = MakeVectorRegister(pos, neg, pos, pos);
  const VectorRegister sign_bits_right = MakeVectorRegister(neg, neg, neg, pos);
  const VectorRegister left_term = VectorBitwiseXor(sign_bits_left , VectorMultiply(cr, VectorMultiply(sp_sp_cp_cp, sy_cy_sy_cy)));
  const VectorRegister right_term = VectorBitwiseXor(sign_bits_right, VectorMultiply(sr, VectorMultiply(cp_cp_sp_sp, cy_sy_cy_sy)));

  Quat rotation_quat;
  const VectorRegister result = VectorAdd(left_term, right_term);
  VectorStoreAligned(result, &rotation_quat);
#else
  const float DEG_TO_RAD = PI / (180.f);
  const float DIVIDE_BY_2 = DEG_TO_RAD / 2.f;
  float sp, sy, sr;
  float cp, cy, cr;

  Math::SinCos(&sp, &cp, pitch * DIVIDE_BY_2);
  Math::SinCos(&sy, &cy, yaw   * DIVIDE_BY_2);
  Math::SinCos(&sr, &cr, roll  * DIVIDE_BY_2);

  Quat rotation_quat;
  rotation_quat.x =  cr*sp*sy - sr*cp*cy;
  rotation_quat.y = -cr*sp*cy - sr*cp*sy;
  rotation_quat.z =  cr*cp*sy - sr*sp*cy;
  rotation_quat.w =  cr*cp*cy + sr*sp*sy;
#endif //FUN_PLATFORM_ENABLE_VECTORINTRINSICS

  rotation_quat.DiagnosticCheckNaN();

  return rotation_quat;
}

Vector Rotator::ToEuler() const
{
  return Vector(roll, pitch, yaw);
}

Rotator Rotator::MakeFromEuler(const Vector& euler)
{
  return Rotator(euler.y, euler.z, euler.x);
}

Vector Rotator::UnrotateVector(const Vector& v) const
{
  return RotationMatrix(*this).GetTransposed().TransformVector(v);
}

Vector Rotator::RotateVector(const Vector& v) const
{
  return RotationMatrix(*this).TransformVector(v);
}

void Rotator::GetWindingAndRemainder(Rotator& out_winding, Rotator& out_remainder) const
{
  // Yaw
  out_remainder.yaw = NormalizeAxis(yaw);
  out_winding.yaw = yaw - out_remainder.yaw;

  // Pitch
  out_remainder.pitch = NormalizeAxis(pitch);
  out_winding.pitch = pitch - out_remainder.pitch;

  // Roll
  out_remainder.roll = NormalizeAxis(roll);
  out_winding.roll = roll - out_remainder.roll;
}

Rotator Matrix::ToRotator() const
{
  const Vector x_axis = GetScaledAxis(Axis::X);
  const Vector y_axis = GetScaledAxis(Axis::Y);
  const Vector z_axis = GetScaledAxis(Axis::Z);

  Rotator rotator = Rotator(
        Math::Atan2(x_axis.z, Math::Sqrt(Math::Square(x_axis.x) + Math::Square(x_axis.y))) * 180.f / PI,
        Math::Atan2(x_axis.y, x_axis.x) * 180.f / PI,
        0);

  const Vector sy_axis = RotationMatrix(rotator).GetScaledAxis(Axis::Y);
  rotator.roll = Math::Atan2(z_axis | sy_axis, y_axis | sy_axis) * 180.f / PI;

  rotator.DiagnosticCheckNaN();
  return rotator;
}

Quat Matrix::ToQuat() const
{
  Quat result(*this);
  return result;
}

const Matrix Matrix::Identity(Vector4(1,0,0,0), Vector4(0,1,0,0), Vector4(0,0,1,0), Vector4(0,0,0,1));

const Quat Quat::Identity(0, 0, 0, 1);

String Matrix::ToString() const
{
  String ret;
  ret += String::Format("[%f %f %f %f] ", m[0][0], m[0][1], m[0][2], m[0][3]);
  ret += String::Format("[%f %f %f %f] ", m[1][0], m[1][1], m[1][2], m[1][3]);
  ret += String::Format("[%f %f %f %f] ", m[2][0], m[2][1], m[2][2], m[2][3]);
  ret += String::Format("[%f %f %f %f] ", m[3][0], m[3][1], m[3][2], m[3][3]);
  return ret;
}

void Matrix::DebugPrint() const
{
  fun_log(LogFunMath, Info, "%s", *ToString());
}

uint32 Matrix::ComputeHash() const
{
  uint32 hash = 0;
  const uint32* data = (uint32*)this;
  for (uint32 i = 0; i < 16; ++i) {
    hash ^= data[i] + i;
  }
  return hash;
}


//
// Quat
//

Rotator Quat::ToRotator() const
{
  //TODO?
  //SCOPED_CYCLE_COUNTER(STAT_MathConvertQuatToRotator);

  DiagnosticCheckNaN();
  const float singularity_test = z*x - w*y;
  const float yaw_y = 2.f*(w*z + x*y);
  const float yaw_x = (1.f - 2.f*(Math::Square(y) + Math::Square(z)));

  // reference
  // http://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
  // http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToEuler/

  // this value was found from experience, the above websites recommend different values
  // but that isn't the case for us, so I went through different testing, and finally found the case
  // where both of world lives happily.
  const float SINGULARITY_THRESHOLD = 0.4999995f;
  const float RAD_TO_DEG = 180.f / PI;
  Rotator rotator_from_quat;

  if (singularity_test < -SINGULARITY_THRESHOLD) {
    rotator_from_quat.pitch = -90.f;
    rotator_from_quat.yaw = Math::Atan2(yaw_y, yaw_x) * RAD_TO_DEG;
    rotator_from_quat.roll = Rotator::NormalizeAxis(-rotator_from_quat.yaw - (2.f * Math::Atan2(x, w) * RAD_TO_DEG));
  }
  else if (singularity_test > SINGULARITY_THRESHOLD) {
    rotator_from_quat.pitch = 90.f;
    rotator_from_quat.yaw = Math::Atan2(yaw_y, yaw_x) * RAD_TO_DEG;
    rotator_from_quat.roll = Rotator::NormalizeAxis(rotator_from_quat.yaw - (2.f * Math::Atan2(x, w) * RAD_TO_DEG));
  }
  else {
    rotator_from_quat.pitch = Math::FastAsin(2.f*(singularity_test)) * RAD_TO_DEG;
    rotator_from_quat.yaw = Math::Atan2(yaw_y, yaw_x) * RAD_TO_DEG;
    rotator_from_quat.roll = Math::Atan2(-2.f * (w*x + y*z), (1.f - 2.f*(Math::Square(x) + Math::Square(y)))) * RAD_TO_DEG;
  }

#if FUN_ENABLE_NAN_DIAGNOSTIC
  if (rotator_from_quat.ContainsNaN()) {
    LOG_OR_ENSURE_NAN_ERROR("Quat::rotator(): rotator result %s contains NaN! quat = %s, yaw_y = %.9f, yaw_x = %.9f", *rotator_from_quat.ToString(), *this->ToString(), yaw_y, yaw_x);
    rotator_from_quat = Rotator::ZeroRotator;
  }
#endif

  return rotator_from_quat;
}

Quat Quat::MakeFromEuler(const Vector& euler)
{
  return Rotator::MakeFromEuler(euler).Quaternion();
}

Matrix RotationAboutPointMatrix::Make(const Quat& rot, const Vector& origin)
{
  return RotationAboutPointMatrix(rot.rotator(), origin);
}

Matrix RotationMatrix::Make(Quat const& rot)
{
  return QuatRotationTranslationMatrix(rot, Vector::ZeroVector);
}

Matrix RotationMatrix::MakeFromX(Vector const& x_axis)
{
  Vector const new_x = x_axis.GetSafeNormal();

  // try to use up if possible
  Vector const up_vector = (Math::Abs(new_x.z) < (1.f - KINDA_SMALL_NUMBER)) ? Vector(0, 0, 1.f) : Vector(1.f, 0, 0);

  const Vector new_y = (up_vector ^ new_x).GetSafeNormal();
  const Vector new_z = new_x ^ new_y;

  return Matrix(new_x, new_y, new_z, Vector::ZeroVector);
}

Matrix RotationMatrix::MakeFromY(Vector const& y_axis)
{
  Vector const new_y = y_axis.GetSafeNormal();

  // try to use up if possible
  Vector const up_vector = (Math::Abs(new_y.z) < (1.f - KINDA_SMALL_NUMBER)) ? Vector(0, 0, 1.f) : Vector(1.f, 0, 0);

  const Vector new_z = (up_vector ^ new_y).GetSafeNormal();
  const Vector new_x = new_y ^ new_z;

  return Matrix(new_x, new_y, new_z, Vector::ZeroVector);
}

Matrix RotationMatrix::MakeFromZ(Vector const& z_axis)
{
  Vector const new_z = z_axis.GetSafeNormal();

  // try to use up if possible
  Vector const up_vector = (Math::Abs(new_z.z) < (1.f - KINDA_SMALL_NUMBER)) ? Vector(0, 0, 1.f) : Vector(1.f, 0, 0);

  const Vector new_x = (up_vector ^ new_z).GetSafeNormal();
  const Vector new_y = new_z ^ new_x;

  return Matrix(new_x, new_y, new_z, Vector::ZeroVector);
}

Matrix RotationMatrix::MakeFromXY(Vector const& x_axis, Vector const& y_axis)
{
  Vector new_x = x_axis.GetSafeNormal();
  Vector norm = y_axis.GetSafeNormal();

  // if they're almost same, we need to find arbitrary vector
  if (Math::IsNearlyEqual(Math::Abs(new_x | norm), 1.f)) {
    // make sure we don't ever pick the same as new_x
    norm = (Math::Abs(new_x.z) < (1.f - KINDA_SMALL_NUMBER)) ? Vector(0, 0, 1.f) : Vector(1.f, 0, 0);
  }

  const Vector new_z = (new_x ^ norm).GetSafeNormal();
  const Vector new_y = new_z ^ new_x;

  return Matrix(new_x, new_y, new_z, Vector::ZeroVector);
}

Matrix RotationMatrix::MakeFromXZ(Vector const& x_axis, Vector const& z_axis)
{
  Vector const new_x = x_axis.GetSafeNormal();
  Vector norm = z_axis.GetSafeNormal();

  // if they're almost same, we need to find arbitrary vector
  if (Math::IsNearlyEqual(Math::Abs(new_x | norm), 1.f)) {
    // make sure we don't ever pick the same as new_x
    norm = (Math::Abs(new_x.z) < (1.f - KINDA_SMALL_NUMBER)) ? Vector(0, 0, 1.f) : Vector(1.f, 0, 0);
  }

  const Vector new_y = (norm ^ new_x).GetSafeNormal();
  const Vector new_z = new_x ^ new_y;

  return Matrix(new_x, new_y, new_z, Vector::ZeroVector);
}

Matrix RotationMatrix::MakeFromYX(Vector const& y_axis, Vector const& x_axis)
{
  Vector const new_y = y_axis.GetSafeNormal();
  Vector norm = x_axis.GetSafeNormal();

  // if they're almost same, we need to find arbitrary vector
  if (Math::IsNearlyEqual(Math::Abs(new_y | norm), 1.f)) {
    // make sure we don't ever pick the same as new_x
    norm = (Math::Abs(new_y.z) < (1.f - KINDA_SMALL_NUMBER)) ? Vector(0, 0, 1.f) : Vector(1.f, 0, 0);
  }

  const Vector new_z = (norm ^ new_y).GetSafeNormal();
  const Vector new_x = new_y ^ new_z;

  return Matrix(new_x, new_y, new_z, Vector::ZeroVector);
}

Matrix RotationMatrix::MakeFromYZ(Vector const& y_axis, Vector const& z_axis)
{
  Vector const new_y = y_axis.GetSafeNormal();
  Vector norm = z_axis.GetSafeNormal();

  // if they're almost same, we need to find arbitrary vector
  if (Math::IsNearlyEqual(Math::Abs(new_y | norm), 1.f)) {
    // make sure we don't ever pick the same as new_x
    norm = (Math::Abs(new_y.z) < (1.f - KINDA_SMALL_NUMBER)) ? Vector(0, 0, 1.f) : Vector(1.f, 0, 0);
  }

  const Vector new_x = (new_y ^ norm).GetSafeNormal();
  const Vector new_z = new_x ^ new_y;

  return Matrix(new_x, new_y, new_z, Vector::ZeroVector);
}

Matrix RotationMatrix::MakeFromZX(Vector const& z_axis, Vector const& x_axis)
{
  Vector const new_z = z_axis.GetSafeNormal();
  Vector norm = x_axis.GetSafeNormal();

  // if they're almost same, we need to find arbitrary vector
  if (Math::IsNearlyEqual(Math::Abs(new_z | norm), 1.f)) {
    // make sure we don't ever pick the same as new_x
    norm = (Math::Abs(new_z.z) < (1.f - KINDA_SMALL_NUMBER)) ? Vector(0, 0, 1.f) : Vector(1.f, 0, 0);
  }

  const Vector new_y = (new_z ^ norm).GetSafeNormal();
  const Vector new_x = new_y ^ new_z;

  return Matrix(new_x, new_y, new_z, Vector::ZeroVector);
}

Matrix RotationMatrix::MakeFromZY(Vector const& z_axis, Vector const& y_axis)
{
  Vector const new_z = z_axis.GetSafeNormal();
  Vector norm = y_axis.GetSafeNormal();

  // if they're almost same, we need to find arbitrary vector
  if (Math::IsNearlyEqual(Math::Abs(new_z | norm), 1.f)) {
    // make sure we don't ever pick the same as new_x
    norm = (Math::Abs(new_z.z) < (1.f - KINDA_SMALL_NUMBER)) ? Vector(0, 0, 1.f) : Vector(1.f, 0, 0);
  }

  const Vector new_x = (norm ^ new_z).GetSafeNormal();
  const Vector new_y = new_z ^ new_x;

  return Matrix(new_x, new_y, new_z, Vector::ZeroVector);
}

Vector Quat::ToEuler() const
{
  return ToRotator().ToEuler();
}

bool Quat::NetSerialize(Archive& ar, class RPackageMap*, bool& out_success)
{
  Quat& q = *this;

  if (ar.IsSaving()) {
    // Make sure we have a non null length_sqr. It shouldn't happen with a quaternion, but better be safe.
    if (q.SizeSquared() <= SMALL_NUMBER) {
      q = Quat::Identity;
    }
    else {
      // All transmitted quaternions *MUST BE* unit quaternions, in which case we can deduce the value of w.
      q.Normalize();
      // force w component to be non-negative
      if (q.w < 0.f) {
        q.x *= -1.f;
        q.y *= -1.f;
        q.z *= -1.f;
        q.w *= -1.f;
      }
    }
  }

  ar & q.x & q.y & q.z;
  if (ar.IsLoading()) {
    const float xyz_sqr_length = (q.x*q.x + q.y*q.y + q.z*q.z);
    const float w_sqr_length = 1.f - xyz_sqr_length;
    // If mag of (x, y, z) <= 1.0, then we calculate w to make magnitude of q 1.0
    if (w_sqr_length >= 0.f) {
      q.w = Math::Sqrt(w_sqr_length);
    }
    // If mag of (x, y, z) > 1.0, we set w to zero, and then renormalize
    else {
      q.w = 0.f;

      const float one_over_xyz_sqr_length = Math::InvSqrt(xyz_sqr_length);
      q.x *= one_over_xyz_sqr_length;
      q.y *= one_over_xyz_sqr_length;
      q.z *= one_over_xyz_sqr_length;
    }
  }

  out_success = true;
  return true;
}


// Based on:
// http://lolengine.net/blog/2014/02/24/quaternion-from-two-vectors-final
// http://www.euclideanspace.com/maths/algebra/vectors/angleBetween/index.htm
FUN_ALWAYS_INLINE_DEBUGGABLE Quat FindBetween_Helper(const Vector& a, const Vector& b, float norm_ab)
{
  float w = norm_ab + Vector::DotProduct(a, b);
  Quat result;

  if (w >= 1e-6f * norm_ab) {
    //axis = Vector::CrossProduct(a, b);
    result = Quat(  a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x,
            w);
  }
  else {
    // a and b point in opposite directions
    w = 0.f;
    result = Math::Abs(a.x) > Math::Abs(a.y)
        ? Quat(-a.z, 0.f, a.x, w)
        : Quat(0.f, -a.z, a.y, w);
  }

  result.Normalize();
  return result;
}

Quat Quat::FindBetweenNormals(const Vector& a, const Vector& b)
{
  const float norm_ab = 1.f;
  return FindBetween_Helper(a, b, norm_ab);
}

Quat Quat::FindBetweenVectors(const Vector& a, const Vector& b)
{
  const float norm_ab = Math::Sqrt(a.SizeSquared() * b.SizeSquared());
  return FindBetween_Helper(a, b, norm_ab);
}

Quat Quat::Log() const
{
  Quat result;
  result.w = 0.f;

  if (Math::Abs(w) < 1.f) {
    const float angle = Math::Acos(w);
    const float sin_angle = Math::Sin(angle);

    if (Math::Abs(sin_angle) >= SMALL_NUMBER) {
      const float Scale = angle / sin_angle;
      result.x = Scale * x;
      result.y = Scale * y;
      result.z = Scale * z;

      return result;
    }
  }

  result.x = x;
  result.y = y;
  result.z = z;

  return result;
}

Quat Quat::Exp() const
{
  const float angle = Math::Sqrt(x*x + y*y + z*z);
  const float sin_angle = Math::Sin(angle);

  Quat result;
  result.w = Math::Cos(angle);

  if (Math::Abs(sin_angle) >= SMALL_NUMBER) {
    const float Scale = sin_angle / angle;
    result.x = Scale * x;
    result.y = Scale * y;
    result.z = Scale * z;
  }
  else {
    result.x = x;
    result.y = y;
    result.z = z;
  }

  return result;
}


//
// Swept-Box vs Box test.
//

/* Line-extent/Box Test Util */
bool Math::LineExtentBoxIntersection( const Box& box,
                                      const Vector& start,
                                      const Vector& end,
                                      const Vector& extent,
                                      Vector& hit_location,
                                      Vector& hit_normal,
                                      float& hit_time)
{
  Box test_box = box;
  test_box.max.x += extent.x;
  test_box.max.y += extent.y;
  test_box.max.z += extent.z;

  test_box.min.x -= extent.x;
  test_box.min.y -= extent.y;
  test_box.min.z -= extent.z;

  const Vector dir = (end - start);

  Vector time;
  bool is_inside = true;
  float face_dir[3] = { 1, 1, 1 };

  // x
  if (start.x < test_box.min.x) {
    if (dir.x <= 0.f) {
      return false;
    }
    else {
      is_inside = false;
      face_dir[0] = -1;
      time.x = (test_box.min.x - start.x) / dir.x;
    }
  }
  else if (start.x > test_box.max.x) {
    if (dir.x >= 0.f) {
      return false;
    }
    else {
      is_inside = false;
      time.x = (test_box.max.x - start.x) / dir.x;
    }
  }
  else {
    time.x = 0.f;
  }

  // y
  if (start.y < test_box.min.y) {
    if (dir.y <= 0.f) {
      return false;
    }
    else {
      is_inside = false;
      face_dir[1] = -1;
      time.y = (test_box.min.y - start.y) / dir.y;
    }
  }
  else if (start.y > test_box.max.y) {
    if (dir.y >= 0.f) {
      return false;
    }
    else {
      is_inside = false;
      time.y = (test_box.max.y - start.y) / dir.y;
    }
  }
  else {
    time.y = 0.f;
  }

  // z
  if (start.z < test_box.min.z) {
    if (dir.z <= 0.f) {
      return false;
    }
    else {
      is_inside = false;
      face_dir[2] = -1;
      time.z = (test_box.min.z - start.z) / dir.z;
    }
  }
  else if (start.z > test_box.max.z) {
    if (dir.z >= 0.f) {
      return false;
    }
    else {
      is_inside = false;
      time.z = (test_box.max.z - start.z) / dir.z;
    }
  }
  else {
    time.z = 0.f;
  }

  // If the line started inside the box
  // (ie. player started in contact with the fluid)
  if (is_inside) {
    hit_location = start;
    hit_normal = Vector(0, 0, 1);
    hit_time = 0;
    return true;
  }
  // Otherwise, calculate when hit occured
  else {
    if (time.y > time.z) {
      hit_time = time.y;
      hit_normal = Vector(0, face_dir[1], 0);
    }
    else {
      hit_time = time.z;
      hit_normal = Vector(0, 0, face_dir[2]);
    }

    if (time.x > hit_time) {
      hit_time = time.x;
      hit_normal = Vector(face_dir[0], 0, 0);
    }

    if (hit_time >= 0.f && hit_time <= 1.f) {
      hit_location = start + dir * hit_time;
      const float BOX_SIDE_THRESHOLD = 0.1f;
      if (hit_location.x > test_box.min.x - BOX_SIDE_THRESHOLD && hit_location.x < test_box.max.x + BOX_SIDE_THRESHOLD &&
          hit_location.y > test_box.min.y - BOX_SIDE_THRESHOLD && hit_location.y < test_box.max.y + BOX_SIDE_THRESHOLD &&
          hit_location.z > test_box.min.z - BOX_SIDE_THRESHOLD && hit_location.z < test_box.max.z + BOX_SIDE_THRESHOLD) {
        return true;
      }
    }

    return false;
  }
}

float Vector::EvaluateBezier(const Vector* control_point_list, int32 control_point_count, Array<Vector>& out_points)
{
  fun_check_ptr(control_point_list);
  fun_check(control_point_count >= 2);

  // var q is the change in t between successive evaluations.
  const float q = 1.f / (control_point_count - 1); // q is dependent on the number of GAPS = POINTS-1

  // recreate the names used in the derivation
  const Vector& p0 = control_point_list[0];
  const Vector& p1 = control_point_list[1];
  const Vector& p2 = control_point_list[2];
  const Vector& p3 = control_point_list[3];

  // coefficients of the cubic polynomial that we're FDing -
  const Vector a = p0;
  const Vector b = 3*(p1-p0);
  const Vector c = 3*(p2-2*p1+p0);
  const Vector d = p3-3*p2+3*p1-p0;

  // initial values of the poly and the 3 diffs -
  Vector s = a;           // the poly value
  Vector u = b*q + c*q*q + d*q*q*q; // 1st order diff (quadratic)
  Vector v = 2*c*q*q + 6*d*q*q*q; // 2nd order diff (linear)
  Vector w = 6*d*q*q*q;       // 3rd order diff (constant)

  // Path length.
  float length = 0.f;

  Vector old_pos = p0;
  out_points.Add(p0); // first point on the curve is always p0.

  for (int32 i = 1; i < control_point_count; ++i) {
    // calculate the next value and update the deltas
    s += u;   // update poly value
    u += v;   // update 1st order diff value
    v += w;   // update 2st order diff value
    // 3rd order diff is constant => no update needed.

    // Update Length.
    length += Vector::Dist(s, old_pos);
    old_pos  = s;

    out_points.Add(s);
  }

  // Return path length as experienced in sequence (linear interpolation between points).
  return length;
}

float LinearColor::EvaluateBezier(const LinearColor* control_point_list, int32 control_point_count, Array<LinearColor>& out_points)
{
  fun_check_ptr(control_point_list);
  fun_check(control_point_count >= 2);

  // var q is the change in t between successive evaluations.
  const float q = 1.f / (control_point_count - 1); // q is dependent on the number of GAPS = POINTS-1

  // recreate the names used in the derivation
  const LinearColor& p0 = control_point_list[0];
  const LinearColor& p1 = control_point_list[1];
  const LinearColor& p2 = control_point_list[2];
  const LinearColor& p3 = control_point_list[3];

  // coefficients of the cubic polynomial that we're FDing -
  const LinearColor a = p0;
  const LinearColor b = 3*(p1-p0);
  const LinearColor c = 3*(p2-2*p1+p0);
  const LinearColor d = p3-3*p2+3*p1-p0;

  // initial values of the poly and the 3 diffs -
  LinearColor s = a;           // the poly value
  LinearColor u = b*q + c*q*q + d*q*q*q; // 1st order diff (quadratic)
  LinearColor v = 2*c*q*q + 6*d*q*q*q; // 2nd order diff (linear)
  LinearColor w = 6*d*q*q*q;       // 3rd order diff (constant)

  // Path length.
  float length = 0.f;

  LinearColor old_pos = p0;
  out_points.Add(p0);  // first point on the curve is always p0.

  for (int32 i = 1; i < control_point_count; ++i) {
    // calculate the next value and update the deltas
    s += u;   // update poly value
    u += v;   // update 1st order diff value
    v += w;   // update 2st order diff value
    // 3rd order diff is constant => no update needed.

    // Update Length.
    length += LinearColor::Dist(s, old_pos);
    old_pos  = s;

    out_points.Add(s);
  }

  // Return path length as experienced in sequence (linear interpolation between points).
  return length;
}

Quat Quat::Slerp_NotNormalized(const Quat& quat1, const Quat& quat2, float slerp)
{
  // Get cosine of angle between quats.
  const float raw_cosom =
      quat1.x * quat2.x +
      quat1.y * quat2.y +
      quat1.z * quat2.z +
      quat1.w * quat2.w;
  // Unaligned quats - compensate, results in taking shorter route.
  const float cosom = Math::FloatSelect(raw_cosom, raw_cosom, -raw_cosom);

  float scale0, scale1;

  if (cosom < 0.9999f) {
    const float omega = Math::Acos(cosom);
    const float inv_sin = 1.f / Math::Sin(omega);
    scale0 = Math::Sin((1.f - slerp) * omega) * inv_sin;
    scale1 = Math::Sin(slerp * omega) * inv_sin;
  }
  else {
    // Use linear interpolation.
    scale0 = 1.f - slerp;
    scale1 = slerp;
  }

  // In keeping with our flipped cosom:
  scale1 = Math::FloatSelect(raw_cosom, scale1, -scale1);

  Quat result;
  result.x = scale0 * quat1.x + scale1 * quat2.x;
  result.y = scale0 * quat1.y + scale1 * quat2.y;
  result.z = scale0 * quat1.z + scale1 * quat2.z;
  result.w = scale0 * quat1.w + scale1 * quat2.w;
  return result;
}

Quat Quat::SlerpFullPath_NotNormalized(const Quat& quat1, const Quat& quat2, float alpha)
{
  const float cos_angle = Math::Clamp(quat1 | quat2, -1.f, 1.f);
  const float angle = Math::Acos(cos_angle);

  //fun_log(LogFunMath, Info, TEXT("cos_angle: %f angle: %f"), cos_angle, angle);

  if (Math::Abs(angle) < KINDA_SMALL_NUMBER) {
    return quat1;
  }

  const float sin_angle = Math::Sin(angle);
  const float inv_sin_angle = 1.f / sin_angle;

  const float scale0 = Math::Sin((1.f - alpha) * angle) * inv_sin_angle;
  const float scale1 = Math::Sin(alpha * angle) * inv_sin_angle;

  return quat1*scale0 + quat2*scale1;
}

Quat Quat::Squad(const Quat& quat1, const Quat& tan1, const Quat& quat2, const Quat& tan2, float alpha)
{
  const Quat q1 = Quat::SlerpFullPath_NotNormalized(quat1, quat2, alpha);
  const Quat q2 = Quat::SlerpFullPath_NotNormalized(tan1, tan2, alpha);
  const Quat result = Quat::SlerpFullPath(q1, q2, 2.f * alpha * (1.f - alpha));
  return result;
}

void Quat::CalcTangents(const Quat& prev_p, const Quat& p, const Quat& next_p, float tension, Quat& out_tan)
{
  const Quat inv_p = p.Inverse();
  const Quat part1 = (inv_p * prev_p).Log();
  const Quat part2 = (inv_p * next_p).Log();
  const Quat prev_exp = (part1 + part2) * -0.5f;

  out_tan = p * prev_exp.Exp();
}

namespace {

static void FindBounds(float& out_min, float& out_max, float start, float start_leave_tan, float start_t, float end, float end_arrive_tan, float end_t, bool is_curve)
{
  out_min = Math::Min(start, end);
  out_max = Math::Max(start, end);

  // Do we need to consider extermeties of a curve?
  if (is_curve) {
    // Scale tangents based on time interval, so this code matches the behaviour in InterpCurve::Eval
    float diff = end_t - start_t;
    start_leave_tan *= diff;
    end_arrive_tan *= diff;

    const float a = 6.f*start + 3.f*start_leave_tan + 3.f*end_arrive_tan - 6.f*end;
    const float b = -6.f*start - 4.f*start_leave_tan - 2.f*end_arrive_tan + 6.f*end;
    const float c = start_leave_tan;

    const float discriminant = (b*b) - (4.f*a*c);
    if (discriminant > 0.f && !Math::IsNearlyZero(a)) { // Solving doesn't work if a is zero, which usually indicates co-incident start and end, and zero tangents anyway
      const float sqrt_dist = Math::Sqrt(discriminant);

      const float x0 = (-b + sqrt_dist) / (2.f*a); // x0 is the 'Alpha' ie between 0 and 1
      const float t0 = start_t + x0 * (end_t - start_t); // Then t0 is the actual 'time' on the curve
      if (t0 > start_t && t0 < end_t) {
        const float val = Math::CubicInterp(start, start_leave_tan, end, end_arrive_tan, x0);
        out_min = Math::Min(out_min, val);
        out_max = Math::Max(out_max, val);
      }

      const float x1 = (-b - sqrt_dist) / (2.f*a);
      const float t1 = start_t + x1 * (end_t - start_t);
      if (t1 > start_t && t1 < end_t) {
        const float val = Math::CubicInterp(start, start_leave_tan, end, end_arrive_tan, x1);
        out_min = Math::Min(out_min, val);
        out_max = Math::Max(out_max, val);
      }
    }
  }
}

} // namespace

void CurveFloatFindIntervalBounds(const InterpCurvePoint<float>& start, const InterpCurvePoint<float>& end, float& current_min, float& current_max)
{
  const bool is_curve = start.IsCurveKey();

  float out_min, out_max;
  FindBounds(out_min, out_max, start.out_val, start.leave_tangent, start.in_val, end.out_val, end.arrive_tangent, end.in_val, is_curve);

  current_min = Math::Min(current_min, out_min);
  current_max = Math::Max(current_max, out_max);
}

void CurveVector2DFindIntervalBounds(const InterpCurvePoint<Vector2>& start, const InterpCurvePoint<Vector2>& end, Vector2& current_min, Vector2& current_max)
{
  const bool is_curve = start.IsCurveKey();

  float out_min, out_max;

  FindBounds(out_min, out_max, start.out_val.x, start.leave_tangent.x, start.in_val, end.out_val.x, end.arrive_tangent.x, end.in_val, is_curve);
  current_min.x = Math::Min(current_min.x, out_min);
  current_max.x = Math::Max(current_max.x, out_max);

  FindBounds(out_min, out_max, start.out_val.y, start.leave_tangent.y, start.in_val, end.out_val.y, end.arrive_tangent.y, end.in_val, is_curve);
  current_min.y = Math::Min(current_min.y, out_min);
  current_max.y = Math::Max(current_max.y, out_max);
}

void CurveVectorFindIntervalBounds(const InterpCurvePoint<Vector>& start, const InterpCurvePoint<Vector>& end, Vector& current_min, Vector& current_max)
{
  const bool is_curve = start.IsCurveKey();

  float out_min, out_max;

  FindBounds(out_min, out_max, start.out_val.x, start.leave_tangent.x, start.in_val, end.out_val.x, end.arrive_tangent.x, end.in_val, is_curve);
  current_min.x = Math::Min(current_min.x, out_min);
  current_max.x = Math::Max(current_max.x, out_max);

  FindBounds(out_min, out_max, start.out_val.y, start.leave_tangent.y, start.in_val, end.out_val.y, end.arrive_tangent.y, end.in_val, is_curve);
  current_min.y = Math::Min(current_min.y, out_min);
  current_max.y = Math::Max(current_max.y, out_max);

  FindBounds(out_min, out_max, start.out_val.z, start.leave_tangent.z, start.in_val, end.out_val.z, end.arrive_tangent.z, end.in_val, is_curve);
  current_min.z = Math::Min(current_min.z, out_min);
  current_max.z = Math::Max(current_max.z, out_max);
}

void CurveTwoVectorsFindIntervalBounds(const InterpCurvePoint<TwoVectors>& start, const InterpCurvePoint<TwoVectors>& end, TwoVectors& current_min, TwoVectors& current_max)
{
  const bool is_curve = start.IsCurveKey();

  float out_min, out_max;

  // Do the first curve
  FindBounds(out_min, out_max, start.out_val.v1.x, start.leave_tangent.v1.x, start.in_val, end.out_val.v1.x, end.arrive_tangent.v1.x, end.in_val, is_curve);
  current_min.v1.x = Math::Min(current_min.v1.x, out_min);
  current_max.v1.x = Math::Max(current_max.v1.x, out_max);

  FindBounds(out_min, out_max, start.out_val.v1.y, start.leave_tangent.v1.y, start.in_val, end.out_val.v1.y, end.arrive_tangent.v1.y, end.in_val, is_curve);
  current_min.v1.y = Math::Min(current_min.v1.y, out_min);
  current_max.v1.y = Math::Max(current_max.v1.y, out_max);

  FindBounds(out_min, out_max, start.out_val.v1.z, start.leave_tangent.v1.z, start.in_val, end.out_val.v1.z, end.arrive_tangent.v1.z, end.in_val, is_curve);
  current_min.v1.z = Math::Min(current_min.v1.z, out_min);
  current_max.v1.z = Math::Max(current_max.v1.z, out_max);

  // Do the second curve
  FindBounds(out_min, out_max, start.out_val.v2.x, start.leave_tangent.v2.x, start.in_val, end.out_val.v2.x, end.arrive_tangent.v2.x, end.in_val, is_curve);
  current_min.v2.x = Math::Min(current_min.v2.x, out_min);
  current_max.v2.x = Math::Max(current_max.v2.x, out_max);

  FindBounds(out_min, out_max, start.out_val.v2.y, start.leave_tangent.v2.y, start.in_val, end.out_val.v2.y, end.arrive_tangent.v2.y, end.in_val, is_curve);
  current_min.v2.y = Math::Min(current_min.v2.y, out_min);
  current_max.v2.y = Math::Max(current_max.v2.y, out_max);

  FindBounds(out_min, out_max, start.out_val.v2.z, start.leave_tangent.v2.z, start.in_val, end.out_val.v2.z, end.arrive_tangent.v2.z, end.in_val, is_curve);
  current_min.v2.z = Math::Min(current_min.v2.z, out_min);
  current_max.v2.z = Math::Max(current_max.v2.z, out_max);
}

void CurveLinearColorFindIntervalBounds(const InterpCurvePoint<LinearColor>& start, const InterpCurvePoint<LinearColor>& end, LinearColor& current_min, LinearColor& current_max)
{
  const bool is_curve = start.IsCurveKey();

  float out_min, out_max;

  FindBounds(out_min, out_max, start.out_val.r, start.leave_tangent.r, start.in_val, end.out_val.r, end.arrive_tangent.r, end.in_val, is_curve);
  current_min.r = Math::Min(current_min.r, out_min);
  current_max.r = Math::Max(current_max.r, out_max);

  FindBounds(out_min, out_max, start.out_val.G, start.leave_tangent.G, start.in_val, end.out_val.G, end.arrive_tangent.G, end.in_val, is_curve);
  current_min.G = Math::Min(current_min.G, out_min);
  current_max.G = Math::Max(current_max.G, out_max);

  FindBounds(out_min, out_max, start.out_val.b, start.leave_tangent.b, start.in_val, end.out_val.b, end.arrive_tangent.b, end.in_val, is_curve);
  current_min.b = Math::Min(current_min.b, out_min);
  current_max.b = Math::Max(current_max.b, out_max);

  FindBounds(out_min, out_max, start.out_val.a, start.leave_tangent.a, start.in_val, end.out_val.a, end.arrive_tangent.a, end.in_val, is_curve);
  current_min.a = Math::Min(current_min.a, out_min);
  current_max.a = Math::Max(current_max.a, out_max);
}

float Math::PointDistToLine(const Vector& point, const Vector& direction, const Vector& origin, Vector& out_closest_point)
{
  const Vector safe_dir = direction.GetSafeNormal();
  out_closest_point = origin + (safe_dir * ((point - origin) | SafeDir));
  return (out_closest_point - point).Size();
}

float Math::PointDistToLine(const Vector& point, const Vector& direction, const Vector& origin)
{
  const Vector safe_dir = direction.GetSafeNormal();
  const Vector out_closest_point = origin + (safe_dir * ((point - origin) | safe_dir));
  return (out_closest_point - point).Size();
}

Vector Math::ClosestPointOnSegment(const Vector& point, const Vector& start_point, const Vector& end_point)
{
  const Vector segment = end_point - start_point;
  const Vector vec_to_point = point - start_point;

  // See if closest point is before start_point
  const float dot1 = vec_to_point | segment;
  if (dot1 <= 0) {
    return start_point;
  }

  // See if closest point is beyond end_point
  const float dot2 = segment | segment;
  if (dot2 <= dot1) {
    return end_point;
  }

  // Closest point is within segment
  return start_point + segment * (dot1 / dot2);
}

Vector2 Math::ClosestPointOnSegment2D(const Vector2& point, const Vector2& start_point, const Vector2& end_point)
{
  const Vector2 segment = end_point - start_point;
  const Vector2 vec_to_point = point - start_point;

  // See if closest point is before start_point
  const float dot1 = vec_to_point | segment;
  if (dot1 <= 0) {
    return start_point;
  }

  // See if closest point is beyond end_point
  const float dot2 = segment | segment;
  if (dot2 <= dot1) {
    return end_point;
  }

  // Closest point is within segment
  return start_point + segment * (dot1 / dot2);
}

float Math::PointDistToSegment(const Vector& point, const Vector& start_point, const Vector& end_point)
{
  const Vector closest_point = ClosestPointOnSegment(point, start_point, end_point);
  return (point - closest_point).Size();
}

float Math::PointDistToSegmentSquared(const Vector& point, const Vector& start_point, const Vector& end_point)
{
  const Vector closest_point = ClosestPointOnSegment(point, start_point, end_point);
  return (point - closest_point).SizeSquared();
}

void Math::SegmentDistToSegmentSafe(const Vector& a1, const Vector& b1, const Vector& a2, const Vector& b2, Vector& out_p1, Vector& out_p2)
{
  // Segments
  const Vector s1 = b1 - a1;
  const Vector s2 = b2 - a2;
  const Vector s3 = a1 - a2;

  const Vector s1_norm = s1.GetSafeNormal();
  const Vector s2_norm = s2.GetSafeNormal();

  const float dot11 = s1 | s1; // always >= 0
  const float dot22 = s2 | s2; // always >= 0
  const float dot12 = s1 | s2;
  const float dot13 = s1 | s3;
  const float dot23 = s2 | s3;

  const float dot11_norm = s1_norm | s1_norm; // always >= 0
  const float dot22_norm = s2_norm | s2_norm; // always >= 0
  const float dot12_norm = s1_norm | s2_norm;

  // Numerator
  float n1, n2;

  // Denominator
  const float d = dot11*dot22 - dot12*dot12; // always >= 0
  const float d_norm = dot11_norm*dot22_norm - dot12_norm*dot12_norm; // always >= 0

  float d1 = d; // t1 = n1 / d1, default d1 = d >= 0
  float d2 = d; // t2 = n2 / d2, default d2 = d >= 0

  // compute the line parameters of the two closest points
  if (d < KINDA_SMALL_NUMBER || d_norm < KINDA_SMALL_NUMBER) {
    // the lines are almost parallel
    n1 = 0.f; // force using point a on segment s1
    d1 = 1.f; // to prevent possible division by 0 later
    n2 = dot23;
    d2 = dot22;
  }
  else {
    // get the closest points on the infinite lines
    n1 = (dot12*dot23 - dot22*dot13);
    n2 = (dot11*dot23 - dot12*dot13);

    if (n1 < 0.f) {
      // t1 < 0.f => the s==0 edge is visible
      n1 = 0.f;
      n2 = dot23;
      d2 = dot22;
    }
    else if (n1 > d1) {
      // t1 > 1 => the t1==1 edge is visible
      n1 = d1;
      n2 = dot23 + dot12;
      d2 = dot22;
    }
  }

  if (n2 < 0.f) {
    // t2 < 0 => the t2==0 edge is visible
    n2 = 0.f;

    // recompute t1 for this edge
    if (-dot13 < 0.f) {
      n1 = 0.f;
    }
    else if (-dot13 > dot11) {
      n1 = d1;
    }
    else {
      n1 = -dot13;
      d1 = dot11;
    }
  }
  else if (n2 > d2) {
    // t2 > 1 => the t2=1 edge is visible
    n2 = d2;

    // recompute t1 for this edge
    if ((-dot13 + dot12) < 0.f) {
      n1 = 0.f;
    }
    else if ((-dot13 + dot12) > dot11) {
      n1 = d1;
    }
    else {
      n1 = (-dot13 + dot12);
      d1 = dot11;
    }
  }

  // finally do the division to get the points' location
  const float t1 = (Math::Abs(n1) < KINDA_SMALL_NUMBER ? 0.f : n1 / d1);
  const float t2 = (Math::Abs(n2) < KINDA_SMALL_NUMBER ? 0.f : n2 / d2);

  // return the closest points
  out_p1 = a1 + t1 * s1;
  out_p2 = a2 + t2 * s2;
}

void Math::SegmentDistToSegment(const Vector& a1, const Vector& b1, const Vector& a2, const Vector& b2, Vector& out_p1, Vector& out_p2)
{
  // Segments
  const Vector s1 = b1 - a1;
  const Vector s2 = b2 - a2;
  const Vector s3 = a1 - a2;

  const float dot11 = s1 | s1; // always >= 0
  const float dot22 = s2 | s2; // always >= 0
  const float dot12 = s1 | s2;
  const float dot13 = s1 | s3;
  const float dot23 = s2 | s3;

  // Numerator
  float n1, n2;

  // Denominator
  const float d = dot11*dot22 - dot12*dot12; // always >= 0
  float d1 = d; // t1 = n1 / d1, default d1 = d >= 0
  float d2 = d; // t2 = n2 / d2, default d2 = d >= 0

  // compute the line parameters of the two closest points
  if (d < KINDA_SMALL_NUMBER) {
    // the lines are almost parallel
    n1 = 0.f; // force using point a on segment s1
    d1 = 1.f; // to prevent possible division by 0 later
    n2 = dot23;
    d2 = dot22;
  }
  else {
    // get the closest points on the infinite lines
    n1 = (dot12*dot23 - dot22*dot13);
    n2 = (dot11*dot23 - dot12*dot13);

    if (n1 < 0.f) {
      // t1 < 0.f => the s==0 edge is visible
      n1 = 0.f;
      n2 = dot23;
      d2 = dot22;
    }
    else if (n1 > d1) {
      // t1 > 1 => the t1==1 edge is visible
      n1 = d1;
      n2 = dot23 + dot12;
      d2 = dot22;
    }
  }

  if (n2 < 0.f) {
    // t2 < 0 => the t2==0 edge is visible
    n2 = 0.f;

    // recompute t1 for this edge
    if (-dot13 < 0.f) {
      n1 = 0.f;
    }
    else if (-dot13 > dot11) {
      n1 = d1;
    }
    else {
      n1 = -dot13;
      d1 = dot11;
    }
  }
  else if (n2 > d2) {
    // t2 > 1 => the t2=1 edge is visible
    n2 = d2;

    // recompute t1 for this edge
    if ((-dot13 + dot12) < 0.f) {
      n1 = 0.f;
    }
    else if ((-dot13 + dot12) > dot11) {
      n1 = d1;
    }
    else {
      n1 = (-dot13 + dot12);
      d1 = dot11;
    }
  }

  // finally do the division to get the points' location
  const float t1 = (Math::Abs(n1) < KINDA_SMALL_NUMBER ? 0.f : n1 / d1);
  const float t2 = (Math::Abs(n2) < KINDA_SMALL_NUMBER ? 0.f : n2 / d2);

  // return the closest points
  out_p1 = a1 + t1 * s1;
  out_p2 = a2 + t2 * s2;
}

float Math::GetTForSegmentPlaneIntersect(const Vector& start_point, const Vector& end_point, const Plane& plane)
{
  return (plane.w - (start_point | plane)) / ((end_point - start_point) | plane);
}

bool Math::SegmentPlaneIntersection(const Vector& start_point, const Vector& end_point, const Plane& plane, Vector& out_intersection_point)
{
  const float t = Math::GetTForSegmentPlaneIntersect(start_point, end_point, plane);
  // If the parameter value is not between 0 and 1, there is no intersection
  if (t > -KINDA_SMALL_NUMBER && t < 1.f + KINDA_SMALL_NUMBER) {
    out_intersection_point = start_point + t * (end_point - start_point);
    return true;
  }

  return false;
}

bool Math::SegmentIntersection2D(const Vector& segment_start_a, const Vector& segment_end_a, const Vector& segment_start_b, const Vector& segment_end_b, Vector& out_intersection_point)
{
  const Vector vector_a = segment_end_a - segment_start_a;
  const Vector vector_b = segment_end_b - segment_start_b;

  const float s = (-vector_a.y * (segment_start_a.x - segment_start_b.x) + vector_a.x * (segment_start_a.y - segment_start_b.y)) / (-vector_b.x * vector_a.y + vector_a.x * vector_b.y);
  const float t = ( vector_b.x * (segment_start_a.y - segment_start_b.y) - vector_b.y * (segment_start_a.x - segment_start_b.x)) / (-vector_b.x * vector_a.y + vector_a.x * vector_b.y);

  const bool intersects = (s >= 0.f && s <= 1.f && t >= 0.f && t <= 1.f);

  if (intersects) {
    out_intersection_point.x = segment_start_a.x + (t * vector_a.x);
    out_intersection_point.y = segment_start_a.y + (t * vector_a.y);
    out_intersection_point.z = segment_start_a.z + (t * vector_a.z);
  }

  return intersects;
}

namespace {

/**
 * Compute the screen bounds of a point light along one axis.
 * Based on http://www.gamasutra.com/features/20021011/lengyel_06.htm
 * and http://sourceforge.net/mailarchive/message.php?msg_id=10501105
 */
bool ComputeProjectedSphereShaft(
  float light_x,
  float light_z,
  float radius,
  const Matrix& proj_matrix,
  const Vector& axis,
  float axis_sign,
  int32& ref_min_x,
  int32& ref_max_x)
{
  float view_x = ref_min_x;
  float view_size_x = ref_max_x - ref_min_x;

  // Vertical planes: T = <Nx, 0, Nz, 0>
  float discriminant = (Math::Square(light_x) - Math::Square(radius) + Math::Square(light_z)) * Math::Square(light_z);
  if (discriminant >= 0) {
    float sqrt_discriminant = Math::Sqrt(discriminant);
    float inv_light_square = 1.f / (Math::Square(light_x) + Math::Square(light_z));

    float n_xa = (radius * light_x - sqrt_discriminant) * inv_light_square;
    float n_xb = (radius * light_x + sqrt_discriminant) * inv_light_square;
    float n_za = (radius - n_xa * light_x) / light_z;
    float n_zb = (radius - n_xb * light_x) / light_z;
    float p_za = light_z - radius * n_za;
    float p_zb = light_z - radius * n_zb;

    // Tangent a
    if (p_za > 0) {
      float Pxa = -p_za * n_za / n_xa;
      Vector4 p = proj_matrix.TransformVector4(Vector4(axis.x * Pxa, axis.y * Pxa, p_za, 1));
      float x = (Dot3(p, axis) / p.w + 1.f * axis_sign) / 2.f * axis_sign;
      if (Math::IsNegativeFloat(n_xa) ^ Math::IsNegativeFloat(axis_sign)) {
        ref_max_x = Math::Min<int64>(Math::CeilToInt(view_size_x * x + view_x), ref_max_x);
      }
      else {
        ref_min_x = Math::Max<int64>(Math::FloorToInt(view_size_x * x + view_x), ref_min_x);
      }
    }

    // Tangent b
    if (p_zb > 0) {
      float Pxb = -p_zb * n_zb / n_xb;
      Vector4 p = proj_matrix.TransformVector4(Vector4(axis.x * Pxb, axis.y * Pxb, p_zb, 1));
      float x = (Dot3(p, axis) / p.w + 1.f * axis_sign) / 2.f * axis_sign;
      if (Math::IsNegativeFloat(n_xb) ^ Math::IsNegativeFloat(axis_sign)) {
        ref_max_x = Math::Min<int64>(Math::CeilToInt(view_size_x * x + view_x), ref_max_x);
      }
      else {
        ref_min_x = Math::Max<int64>(Math::FloorToInt(view_size_x * x + view_x), ref_min_x);
      }
    }
  }

  return ref_min_x <= ref_max_x;
}

} // namespace

uint32 Math::ComputeProjectedSphereScissorRect( IntRect& ref_scissor_rect,
                                                const Vector& sphre_origin,
                                                float radius,
                                                const Vector& view_origin,
                                                const Matrix& view_matrix,
                                                const Matrix& proj_matrix)
{
  // Calculate a scissor rectangle for the light's radius.
  if ((sphre_origin - view_origin).SizeSquared() > Math::Square(radius)) {
    Vector light_vector = view_matrix.TransformPosition(sphre_origin);

    if (!ComputeProjectedSphereShaft(
            light_vector.x,
            light_vector.z,
            radius,
            proj_matrix,
            Vector(+1, 0, 0),
            +1,
            ref_scissor_rect.min.x,
            ref_scissor_rect.max.x)) {
      return 0;
    }

    if (!ComputeProjectedSphereShaft(
            light_vector.y,
            light_vector.z,
            radius,
            proj_matrix,
            Vector(0, +1, 0),
            -1,
            ref_scissor_rect.min.y,
            ref_scissor_rect.max.y)) {
      return 0;
    }

    return 1;
  }
  else {
    return 2;
  }
}

bool Math::PlaneAABBIntersection(const Plane& p, const Box& aabb)
{
  // find diagonal most closely aligned with normal of plane
  Vector v_min, v_max;

  // Bypass the slow Vector[] operator. Not __restrict because it won't update v_min, v_max
  float* v_min_ptr = (float*)&v_min;
  float* v_max_ptr = (float*)&v_max;

  // Use restrict to get better instruction scheduling and to bypass the slow Vector[] operator
  const float* __restrict aabb_min_ptr = (const float*)&aabb.min;
  const float* __restrict aabb_max_ptr = (const float*)&aabb.max;
  const float* __restrict plane_ptr = (const float*)&p;

  for (int32 i = 0; i < 3; ++i) {
    if (plane_ptr[i] >= 0.f) {
      v_min_ptr[i] = aabb_min_ptr[i];
      v_max_ptr[i] = aabb_max_ptr[i];
    }
    else {
      v_min_ptr[i] = aabb_max_ptr[i];
      v_max_ptr[i] = aabb_min_ptr[i];
    }
  }

  // if either diagonal is right on the plane, or one is on either side we have an interesection
  float dot_max = P.PlaneDot(v_max);
  float dot_min = P.PlaneDot(v_min);

  // if Max is below plane, or Min is above we know there is no intersection.. otherwise there must be one
  return (dot_max >= 0.f && dot_min <= 0.f);
}

bool Math::SphereConeIntersection(const Vector& sphere_center,
                                  float sphre_radius,
                                  const Vector& cone_axis,
                                  float cone_angle_sin,
                                  float cone_angle_cos)
{
  // from http://www.geometrictools.com/Documentation/IntersectionSphereCone.pdf
  // (Copyright c 1998-2008. All Rights Reserved.) http://www.geometrictools.com (boost license)

  // the following code assumes the cone tip is at 0, 0, 0
  // (means the sphere_center is relative to the cone tip)

  Vector u = cone_axis * (-sphre_radius / cone_angle_sin);
  Vector d = sphere_center - u;
  float sqr_d = d | d;
  float e = cone_axis | d;

  if (e > 0 && e*e >= sqr_d * Math::Square(cone_angle_cos)) {
    sqr_d = sphere_center |sphere_center;
    e = -cone_axis | sphere_center;
    if (e > 0 && e*e >= sqr_d * Math::Square(cone_angle_sin)) {
      return sqr_d <= Math::Square(sphre_radius);
    }
    else {
      return true;
    }
  }
  return false;
}

Vector Math::ClosestPointOnTriangleToPoint( const Vector& point,
                                            const Vector& a,
                                            const Vector& b,
                                            const Vector& c)
{
  //Figure out what region the point is in and compare against that "point" or "edge"
  const Vector ba = a - b;
  const Vector ac = c - a;
  const Vector cb = b - c;
  const Vector tri_norm = ba ^ cb;

  // Get the planes that define this triangle
  // edges ba, ac, BC with normals perpendicular to the edges facing outward
  const Plane planes[3] = { Plane(b, tri_norm ^ ba), Plane(a, tri_norm ^ ac), Plane(c, tri_norm ^ cb) };
  int32 plane_halfspace_bitmask = 0;

  //Determine which side of each plane the test point exists
  for (int32 i = 0; i < 3; i++) {
    if (planes[i].PlaneDot(point) > 0.f) {
      plane_halfspace_bitmask |= (1 << i);
    }
  }

  Vector result(point.x, point.y, point.z);
  switch (plane_halfspace_bitmask) {
  case 0: //000 Inside
    return Vector::PointPlaneProject(point, a, b, c);
  case 1: //001 segment ba
    result = Math::ClosestPointOnSegment(point, b, a);
    break;
  case 2: //010 segment ac
    result = Math::ClosestPointOnSegment(point, a, c);
    break;
  case 3: //011 point a
    return a;
  case 4: //100 segment BC
    result = Math::ClosestPointOnSegment(point, b, c);
    break;
  case 5: //101 point b
    return b;
  case 6: //110 point c
    return c;
  default:
    fun_log(LogFunMath, Info, "Impossible result in Math::ClosestPointOnTriangleToPoint");
    break;
  }

  return result;
}

Vector Math::GetBaryCentric2D(const Vector& point, const Vector& a, const Vector& b, const Vector& c)
{
  float a = ((b.y-c.y)*(point.x-c.x) + (c.x-b.x)*(point.y-c.y)) / ((b.y-c.y)*(a.x-c.x) + (c.x-b.x)*(a.y-c.y));
  float b = ((c.y-a.y)*(point.x-c.x) + (a.x-c.x)*(point.y-c.y)) / ((b.y-c.y)*(a.x-c.x) + (c.x-b.x)*(a.y-c.y));

  return Vector(a, b, 1.f - a - b);
}

Vector Math::ComputeBaryCentric2D(const Vector& point, const Vector& a, const Vector& b, const Vector& c)
{
  // Compute the normal of the triangle
  const Vector tri_norm = (b - a) ^ (c - a);

  //check collinearity of a, b, c
  fun_check_msg(tri_norm.SizeSquared() > SMALL_NUMBER, "Collinear points in Math::ComputeBaryCentric2D()");

  const Vector n = tri_norm.GetSafeNormal();

  // Compute twice area of triangle ABC
  const float area_abc_inv = 1.f / (n | tri_norm);

  // Compute a contribution
  const float area_pbc = N | ((b - point) ^ (c - point));
  const float a = area_pbc * area_abc_inv;

  // Compute b contribution
  const float area_pca = n | ((c - point) ^ (a - point));
  const float b = area_pca * area_abc_inv;

  // Compute c contribution
  return Vector(a, b, 1.f - a - b);
}

Vector4 Math::ComputeBaryCentric3D(const Vector& point, const Vector& a, const Vector& b, const Vector& c, const Vector& d)
{
  //http://www.devmaster.net/wiki/Barycentric_coordinates
  //Pick a as our origin and
  //Setup three basis vectors AB, ac, AD
  const Vector b1 = (b - a);
  const Vector b2 = (c - a);
  const Vector b3 = (d - a);

  //check co-planarity of a, b, c, d
  fun_check(fabsf(b1 | (b2 ^ b3)) > SMALL_NUMBER && "Coplanar points in Math::ComputeBaryCentric3D()");

  //Transform point into this new space
  const Vector v = (point - a);

  //Create a matrix of linearly independent vectors
  const Matrix solve_mat(b1, b2, b3, Vector::ZeroVector);

  //The point v can be expressed as Ax=v where x is the vector containing the weights {w1...wn}
  //Solve for x by multiplying both sides by AInv   (AInv * a)x = AInv * v ==> x = AInv * v
  const Matrix inv_solve_math = solve_mat.Inverse();
  const Plane bary_coords = inv_solve_math.TransformVector(v);

  //Reorder the weights to be a, b, c, d
  return Vector4(1.f - bary_coords.x - bary_coords.y - bary_coords.z, bary_coords.x, bary_coords.y, bary_coords.z);
}

Vector Math::ClosestPointOnTetrahedronToPoint(const Vector& point, const Vector& a, const Vector& b, const Vector& c, const Vector& d)
{
  //Check for coplanarity of all four points
  fun_check(fabsf((c-a) | ((b-a)^(d-c))) > 0.0001f && "Coplanar points in Math::ComputeBaryCentric3D()");

  //http://osdir.com/ml/games.devel.algorithms/2003-02/msg00394.html
  //     d
  //    /|\         c-----------b
  //   / | \         \         /
  //  /  |  \    or   \  \a/  /
  // c   |   b         \  |  /
  //  \  |  /           \ | /
  //   \ | /             \|/
  //     a                d

  // Figure out the ordering (is d in the direction of the CCW triangle ABC)
  Vector pt1(a), pt2(b), pt3(c), pt4(d);
  const Plane abc(a, b, c);
  if (abc.PlaneDot(d) < 0.f) {
    //Swap two points to maintain CCW orders
    pt3 = d;
    pt4 = c;
  }

  //Tetrahedron made up of 4 CCW faces - DCA, DBC, DAB, ACB
  const Plane planes[4] = { Plane(pt4, pt3, pt1), Plane(pt4, pt2, pt3), Plane(pt4, pt1, pt2), Plane(pt1, pt3, pt2) };

  //Determine which side of each plane the test point exists
  int32 plane_halfspace_bitmask = 0;
  for (int32 i = 0; i < 4; i++) {
    if (planes[i].PlaneDot(point) > 0.f) {
      plane_halfspace_bitmask |= (1 << i);
    }
  }

  //Verts + Faces - Edges = 2 (euler)
  Vector result(point.x, point.y, point.z);
  switch (plane_halfspace_bitmask) {
  case 0: //inside (0000)
    //@TODO - could project point onto any face
    break;
  case 1: //0001 Face    DCA
    return Math::ClosestPointOnTriangleToPoint(point, pt4, pt3, pt1);
  case 2: //0010 Face    DBC
    return Math::ClosestPointOnTriangleToPoint(point, pt4, pt2, pt3);
  case 3: //0011 Edge    DC
    result = Math::ClosestPointOnSegment(point, pt4, pt3);
    break;
  case 4: //0100 Face    DAB
    return Math::ClosestPointOnTriangleToPoint(point, pt4, pt1, pt2);
  case 5: //0101 Edge    DA
    result = Math::ClosestPointOnSegment(point, pt4, pt1);
    break;
  case 6: //0110 Edge    DB
    result = Math::ClosestPointOnSegment(point, pt4, pt2);
    break;
  case 7: //0111 point   d
    return pt4;
  case 8: //1000 Face    ACB
    return Math::ClosestPointOnTriangleToPoint(point, pt1, pt3, pt2);
  case 9: //1001 Edge    ac
    result = Math::ClosestPointOnSegment(point, pt1, pt3);
    break;
  case 10: //1010 Edge    BC
    result = Math::ClosestPointOnSegment(point, pt2, pt3);
    break;
  case 11: //1011 point   c
    return pt3;
  case 12: //1100 Edge    ba
    result = Math::ClosestPointOnSegment(point, pt2, pt1);
    break;
  case 13: //1101 point   a
    return pt1;
  case 14: //1110 point   b
    return pt2;
  default: //impossible (1111)
    fun_log(LogFunMath, Info, "Math::ClosestPointOnTetrahedronToPoint() : impossible result");
    break;
  }

  return result;
}

void Math::SphereDistToLine(const Vector& sphre_origin,
                            float sphre_radius,
                            const Vector& line_origin,
                            const Vector& line_dir,
                            Vector& out_closest_point)
{
  const float a = line_dir | line_dir;
  const float b = 2.f * (line_dir | (line_origin - sphre_origin));
  const float c = (sphre_origin|sphre_origin) + (line_origin|line_origin) - 2.f *(sphre_origin|line_origin) - Math::Square(sphre_radius);
  const float d = Math::Square(b) - 4.f * a * c;

  if (d <= KINDA_SMALL_NUMBER) {
    // line is not intersecting sphere (or is tangent at one point if d == 0)
    const Vector point_on_line = line_origin + (-b / 2.f * a) * line_dir;
    out_closest_point = sphre_origin + (point_on_line - sphre_origin).GetSafeNormal() * sphre_radius;
  }
  else {
    // Line intersecting sphere in 2 points. Pick closest to line origin.
    const float e = Math::Sqrt(d);
    const float t1 = (-b + e) / (2.f * a);
    const float t2 = (-b - e) / (2.f * a);
    const float t = Math::Abs(t1) < Math::Abs(t2) ? t1 : t2;

    out_closest_point = line_origin + T * line_dir;
  }
}

bool Math::GetDistanceWithinConeSegment(const Vector& point,
                                        const Vector& cone_start_point,
                                        const Vector& cone_line,
                                        float radius_at_start,
                                        float radius_at_end,
                                        float& out_percentage)
{
  fun_check(radius_at_start >= 0.f && radius_at_end >= 0.f && cone_line.SizeSquared() > 0);
  // -- First we'll draw out a line from the cone_start_point down the cone_line. We'll find the closest point on that line to point.
  //    If we're outside the max distance, or behind the start_point, we bail out as that means we've no chance to be in the cone.

  Vector point_on_cone; // Stores the point on the cone's center line closest to our target point.

  const float distance = Math::PointDistToLine(point, cone_line, cone_start_point, point_on_cone); // distance is how far from the viewline we are

  out_percentage = 0.f; // start assuming we're outside cone until proven otherwise.

  const Vector vec_to_start = cone_start_point - point_on_cone;
  const Vector vec_to_end = (cone_start_point + cone_line) - point_on_cone;

  const float cone_sqr_length = cone_line.SizeSquared();
  const float sqr_dist_to_start = vec_to_start.SizeSquared();
  const float sqr_dist_to_end = vec_to_end.SizeSquared();

  if (sqr_dist_to_start > cone_sqr_length || sqr_dist_to_end > cone_sqr_length) {
    //Outside cone
    return false;
  }

  const float percent_along_cone = Math::Sqrt(sqr_dist_to_start) / Math::Sqrt(cone_sqr_length); // don't have to catch outside 0->1 due to above code (saves 2 sqrts if outside)
  const float radius_at_point = radius_at_start + ((radius_at_end - radius_at_start) * percent_along_cone);

  if (distance > radius_at_point) { // target is farther from the line than the radius at that distance)
    return false;
  }

  out_percentage = radius_at_point > 0.f ? (radius_at_point - distance) / radius_at_point : 1.f;

  return true;
}

bool Math::PointsAreCoplanar(const Array<Vector>& points, const float tolerance)
{
  //less than 4 points = coplanar
  if (points.Count() < 4) {
    return true;
  }

  //Get the Normal for plane determined by first 3 points
  const Vector normal = Vector::CrossProduct(points[2] - points[0], points[1] - points[0]).GetSafeNormal();

  const int32 count = points.Count();
  for (int32 i = 3; i < count; i++) {
    //Abs of PointPlaneDist, dist should be 0
    if (Math::Abs(Vector::PointPlaneDist(points[i], points[0], normal)) > tolerance) {
      return false;
    }
  }

  return true;
}

bool Math::GetDotDistance(Vector2& out_dot_dist,
                          const Vector& direction,
                          const Vector& axis_x,
                          const Vector& axis_y,
                          const Vector& axis_z)
{
  const Vector normal_dir = direction.GetSafeNormal();

  // Find projected point (on axis_x and axis_y, remove axis_z component)
  const Vector no_z_proj_dir = (normal_dir - (normal_dir | axis_z) * axis_z).GetSafeNormal();

  // Figure out if projection is on right or left.
  const float azimuth_sign = ((no_z_proj_dir | axis_y) < 0.f) ? -1.f : 1.f;

  out_dot_dist.y = normal_dir | axis_z;
  const float dir_dot_x = no_z_proj_dir | axis_x;
  out_dot_dist.x = azimuth_sign * Math::Abs(dir_dot_x);

  return (dir_dot_x >= 0.f);
}

Vector2 Math::GetAzimuthAndElevation( const Vector& direction,
                                      const Vector& axis_x,
                                      const Vector& axis_y,
                                      const Vector& axis_z)
{
  const Vector normal_dir = direction.GetSafeNormal();
  // Find projected point (on axis_x and axis_y, remove axis_z component)
  const Vector no_z_proj_dir = (normal_dir - (normal_dir | axis_z) * axis_z).GetSafeNormal();
  // Figure out if projection is on right or left.
  const float azimuth_sign = ((no_z_proj_dir | axis_y) < 0.f) ? -1.f : 1.f;
  const float elevation_sin = normal_dir | axis_z;
  const float azimuth_cos = no_z_proj_dir | axis_x;

  // Convert to angles in Radian.
  return Vector2(Math::Acos(azimuth_cos) * azimuth_sign, Math::Asin(elevation_sin));
}

Vector Math::VInterpNormalRotationTo(const Vector& current,
                                                  const Vector& target,
                                                  float delta_time,
                                                  float rotation_speed_degrees)
{
  // Find delta rotation between both normals.
  Quat delta_quat = Quat::FindBetween(current, target);

  // Decompose into an axis and angle for rotation
  Vector delta_axis(0.f);
  float delta_angle = 0.f;
  delta_quat.ToAxisAndAngle(delta_axis, delta_angle);

  // Find rotation step for this frame
  const float rotation_step_radians = rotation_speed_degrees * (PI / 180) * delta_time;

  if (Math::Abs(delta_angle) > rotation_step_radians) {
    delta_angle = Math::Clamp(delta_angle, -rotation_step_radians, rotation_step_radians);
    delta_quat = Quat(delta_axis, delta_angle);
    return delta_quat.RotateVector(current);
  }
  return target;
}

Vector Math::VInterpConstantTo(const Vector& current,
                                            const Vector& target,
                                            float delta_time,
                                            float interp_speed)
{
  const Vector delta = target - current;
  const float delta_m = delta.Size();
  const float max_step = interp_speed * delta_time;

  if (delta_m > max_step) {
    if (max_step > 0.f) {
      const Vector delta_n = delta / delta_m;
      return current + delta_n * max_step;
    }
    else {
      return current;
    }
  }

  return target;
}

Vector Math::VInterpTo(const Vector& current,
                                    const Vector& target,
                                    float delta_time,
                                    float interp_speed)
{
  // If no interp speed, jump to target value
  if (interp_speed <= 0.f) {
    return target;
  }

  // Distance to reach
  const Vector dist = target - current;

  // If distance is too small, just set the desired location
  if (dist.SizeSquared() < KINDA_SMALL_NUMBER) {
    return target;
  }

  // Delta Move, Clamp so we do not over shoot.
  const Vector delta_move = dist * Math::Clamp01<float>(delta_time * interp_speed);

  return current + delta_move;
}

Vector2 Math::Vector2DInterpConstantTo( const Vector2& current,
                                        const Vector2& target,
                                        float delta_time,
                                        float interp_speed)
{
  const Vector2 delta = target - current;
  const float delta_m = delta.Size();
  const float max_step = interp_speed * delta_time;

  if (delta_m > max_step) {
    if (max_step > 0.f) {
      const Vector2 delta_n = delta / delta_m;
      return current + delta_n * max_step;
    }
    else {
      return current;
    }
  }

  return target;
}

Vector2 Math::Vector2DInterpTo( const Vector2& current,
                                const Vector2& target,
                                float delta_time,
                                float interp_speed)
{
  if (interp_speed <= 0.f) {
    return target;
  }

  const Vector2 dist = target - current;
  if (dist.SizeSquared() < KINDA_SMALL_NUMBER) {
    return target;
  }

  const Vector2 delta_move = dist * Math::Clamp01<float>(delta_time * interp_speed);
  return current + delta_move;
}

Rotator Math::RInterpConstantTo(const Rotator& current,
                                const Rotator& target,
                                float delta_time,
                                float interp_speed)
{
  // if delta_time is 0, do not perform any interpolation (Location was already calculated for that frame)
  if (delta_time == 0.f || current == target) {
    return current;
  }

  // If no interp speed, jump to target value
  if (interp_speed <= 0.f) {
    return target;
  }

  const float delta_interp_speed = interp_speed * delta_time;

  const Rotator delta_move = (target - current).GetNormalized();
  Rotator result = current;
  result.pitch += Math::Clamp(delta_move.pitch, -delta_interp_speed, delta_interp_speed);
  result.yaw += Math::Clamp(delta_move.yaw, -delta_interp_speed, delta_interp_speed);
  result.roll += Math::Clamp(delta_move.roll, -delta_interp_speed, delta_interp_speed);
  return result.GetNormalized();
}

Rotator Math::RInterpTo(const Rotator& current,
                        const Rotator& target,
                        float delta_time,
                        float interp_speed)
{
  // if delta_time is 0, do not perform any interpolation
  // (Location was already calculated for that frame)
  if (delta_time == 0.f || current == target) {
    return current;
  }

  // If no interp speed, jump to target value
  if (interp_speed <= 0.f) {
    return target;
  }

  const float delta_interp_speed = interp_speed * delta_time;

  const Rotator delta = (target - current).GetNormalized();

  // If steps are too small, just return target and assume
  // we have reached our destination.
  if (delta.IsNearlyZero()) {
    return target;
  }

  // Delta move, clamp so we do not over shoot.
  const Rotator delta_move = delta * Math::Clamp01<float>(delta_interp_speed);
  return (current + delta_move).GetNormalized();
}

float Math::FInterpTo(float current,
                      float target,
                      float delta_time,
                      float interp_speed)
{
  // If no interp speed, jump to target value
  if (interp_speed == 0.f) {
    return target;
  }

  // Distance to reach
  const float dist = target - current;

  // If distance is too small, just set the desired location
  if (Math::Square(dist) < SMALL_NUMBER) {
    return target;
  }

  // Delta move, clamp so we do not over shoot.
  const float delta_move = dist * Math::Clamp01<float>(delta_time * interp_speed);

  return current + delta_move;
}

float Math::FInterpConstantTo(float current, float target, float delta_time, float interp_speed)
{
  const float dist = target - current;

  // If distance is too small, just set the desired location
  if (Math::Square(dist) < SMALL_NUMBER) {
    return target;
  }

  const float Step = interp_speed * delta_time;
  return current + Math::Clamp<float>(dist, -Step, Step);
}

LinearColor Math::CInterpTo(const LinearColor& current, const LinearColor& target, float delta_time, float interp_speed)
{
  // If no interp speed, jump to target value
  if (interp_speed <= 0.f) {
    return target;
  }

  // Difference between colors
  const float dist = LinearColor::Dist(target, current);

  // If distance is too small, just set the desired color
  if (dist < KINDA_SMALL_NUMBER) {
    return target;
  }

  // Delta change, clamp so we do not over shoot.
  const LinearColor delta_move = (target - current) * Math::Clamp01<float>(delta_time * interp_speed);

  return current + delta_move;
}

float ClampFloatTangent(float prev_point_val,
                        float prev_time,
                        float cur_point_val,
                        float cur_time,
                        float next_point_val,
                        float next_time)
{
  const float prev_to_next_time_diff = Math::Max<double>(KINDA_SMALL_NUMBER, next_time - prev_time);
  const float prev_to_cur_time_diff = Math::Max<double>(KINDA_SMALL_NUMBER, cur_time - prev_time);
  const float cur_to_next_time_diff = Math::Max<double>(KINDA_SMALL_NUMBER, next_time - cur_time);

  float out_tangent_val = 0.f;

  const float prev_to_next_height_diff = next_point_val - prev_point_val;
  const float prev_to_cur_height_diff = cur_point_val - prev_point_val;
  const float cur_to_next_height_diff = next_point_val - cur_point_val;

  // Check to see if the current point is crest
  if ((prev_to_cur_height_diff >= 0.f && cur_to_next_height_diff <= 0.f) ||
      (prev_to_cur_height_diff <= 0.f && cur_to_next_height_diff >= 0.f)) {
    // Neighbor points are both both on the same side, so zero out the tangent
    out_tangent_val = 0.f;
  }
  else {
    // The three points form a slope

    // Constants
    const float CLAMP_THRESHOLD = 0.333f;

    // Compute height deltas
    const float cur_to_next_tangent = cur_to_next_height_diff / cur_to_next_time_diff;
    const float prev_to_cur_tangent = prev_to_cur_height_diff / prev_to_cur_time_diff;
    const float prev_to_next_tangent = prev_to_next_height_diff / prev_to_next_time_diff;

    // Default to not clamping
    const float unclamped_tangent = prev_to_next_tangent;
    float clamped_tangent = unclamped_tangent;

    const float LOWE_CLAMP_THRESHOLD = CLAMP_THRESHOLD;
    const float UPPER_CLAMP_THRESHOLD = 1.f - CLAMP_THRESHOLD;

    // @todo: Would we get better results using percentange of TIME instead of HEIGHT?
    const float cur_height_alpha = prev_to_cur_height_diff / prev_to_next_height_diff;

    if (prev_to_next_height_diff > 0.f) {
      if (cur_height_alpha < LOWE_CLAMP_THRESHOLD) {
        // 1.0 = maximum clamping (flat), 0.0 = minimal clamping (don't touch)
        const float clamp_alpha = 1.f - cur_height_alpha / CLAMP_THRESHOLD;
        const float lower_clamp = Math::Lerp(prev_to_next_tangent, prev_to_cur_tangent, clamp_alpha);
        clamped_tangent = Math::Min(clamped_tangent, lower_clamp);
      }

      if (cur_height_alpha > UPPER_CLAMP_THRESHOLD) {
        // 1.0 = maximum clamping (flat), 0.0 = minimal clamping (don't touch)
        const float clamp_alpha = (cur_height_alpha - UPPER_CLAMP_THRESHOLD) / CLAMP_THRESHOLD;
        const float upper_clamp = Math::Lerp(prev_to_next_tangent, cur_to_next_tangent, clamp_alpha);
        clamped_tangent = Math::Min(clamped_tangent, upper_clamp);
      }
    }
    else {

      if (cur_height_alpha < LOWE_CLAMP_THRESHOLD) {
        // 1.0 = maximum clamping (flat), 0.0 = minimal clamping (don't touch)
        const float clamp_alpha = 1.f - cur_height_alpha / CLAMP_THRESHOLD;
        const float lower_clamp = Math::Lerp(prev_to_next_tangent, prev_to_cur_tangent, clamp_alpha);
        clamped_tangent = Math::Max(clamped_tangent, lower_clamp);
      }

      if (cur_height_alpha > UPPER_CLAMP_THRESHOLD) {
        // 1.0 = maximum clamping (flat), 0.0 = minimal clamping (don't touch)
        const float clamp_alpha = (cur_height_alpha - UPPER_CLAMP_THRESHOLD) / CLAMP_THRESHOLD;
        const float upper_clamp = Math::Lerp(prev_to_next_tangent, cur_to_next_tangent, clamp_alpha);
        clamped_tangent = Math::Max(clamped_tangent, upper_clamp);
      }
    }

    out_tangent_val = clamped_tangent;
  }

  return out_tangent_val;
}

Vector Math::VRandCone(Vector const& dir, float cone_half_angle_rad)
{
  if (cone_half_angle_rad > 0.f) {
    float const rand_u = Math::FRand();
    float const rand_v = Math::FRand();

    // Get spherical coords that have an even distribution over the unit sphere
    // Method described at http://mathworld.wolfram.com/SpherePointPicking.html
    float theta = 2.f * PI * rand_u;
    float phi = Math::Acos((2.f * rand_v) - 1.f);

    // restrict phi to [0, cone_half_angle_rad]
    // this gives an even distribution of points on the surface of the cone
    // centered at the origin, pointing upward (z), with the desired angle
    phi = Math::Fmod(phi, cone_half_angle_rad);

    // get axes we need to rotate around
    Matrix const dir_mat = RotationMatrix(dir.Rotation());
    // note the axis translation, since we want the variation to be around x
    Vector const dir_z = dir_mat.GetScaledAxis(Axis::X);
    Vector const dir_y = dir_mat.GetScaledAxis(Axis::Y);

    Vector result = dir.RotateAngleAxis(phi * 180.f / PI, dir_y);
    result = result.RotateAngleAxis(theta * 180.f / PI, dir_z);

    // ensure it's a unit vector (might not have been passed in that way)
    result = result.GetSafeNormal();

    return result;
  }
  else {
    return dir.GetSafeNormal();
  }
}

Vector Math::VRandCone( Vector const& dir,
                        float horizontal_cone_half_angle_rad,
                        float vertical_cone_half_angle_rad)
{
  if ((vertical_cone_half_angle_rad > 0.f) && (horizontal_cone_half_angle_rad > 0.f)) {
    float const rand_u = Math::FRand();
    float const rand_v = Math::FRand();

    // Get spherical coords that have an even distribution over the unit sphere
    // Method described at http://mathworld.wolfram.com/SpherePointPicking.html
    float theta = 2.f * PI * rand_u;
    float phi = Math::Acos((2.f * rand_v) - 1.f);

    // restrict phi to [0, cone_half_angle_rad]
    // where cone_half_angle_rad is now a function of theta
    // (specifically, radius of an ellipse as a function of angle)
    // function is ellipse function (x/a)^2 + (y/b)^2 = 1, converted to polar coords
    float cone_half_angle_rad = Math::Square(Math::Cos(theta) / vertical_cone_half_angle_rad) + Math::Square(Math::Sin(theta) / horizontal_cone_half_angle_rad);
    cone_half_angle_rad = Math::Sqrt(1.f / cone_half_angle_rad);

    // clamp to make a cone instead of a sphere
    phi = Math::Fmod(phi, cone_half_angle_rad);

    // get axes we need to rotate around
    Matrix const dir_mat = RotationMatrix(dir.Rotation());
    // note the axis translation, since we want the variation to be around x
    Vector const dir_z = dir_mat.GetScaledAxis(Axis::X);
    Vector const dir_y = dir_mat.GetScaledAxis(Axis::Y);

    Vector result = dir.RotateAngleAxis(phi * 180.f / PI, dir_y);
    result = result.RotateAngleAxis(theta * 180.f / PI, dir_z);

    // ensure it's a unit vector (might not have been passed in that way)
    result = result.GetSafeNormal();

    return result;
  }
  else {
    return dir.GetSafeNormal();
  }
}

Vector Math::RandPointInBox(const Box& box)
{
  //TODO Random의 함수로 교체해야함.
  return Vector(FRandRange(box.min.x, box.max.x),
                FRandRange(box.min.y, box.max.y),
                FRandRange(box.min.z, box.max.z));
}

Vector Math::GetReflectionVector(const Vector& direction, const Vector& surface_normal)
{
  return direction - 2 * (direction | surface_normal.GetSafeNormal()) * surface_normal.GetSafeNormal();
}


struct ClusterMovedHereToMakeCompile
{
  Vector cluster_pos_accum;
  int32 cluster_size;
};

void Vector::GenerateClusterCenters(Array<Vector>& clusters,
                                    const Array<Vector>& points,
                                    int32 iteration_count,
                                    int32 connection_to_bea_valid_count)
{
  // Check we have >0 points and clusters
  if (points.Count() == 0 || clusters.Count() == 0) {
    return;
  }

  // Temp storage for each cluster that mirrors the order of the passed in clusters array
  Array<ClusterMovedHereToMakeCompile> cluster_data;
  cluster_data.AddZeroed(clusters.Count());

  // Then iterate
  for (int32 iteration = 0; i < iteration; i++) {
    // Classify each point - find closest cluster center
    for (int32 i = 0; i < points.Count(); i++) {
      const Vector& pos = points[i];

      // Iterate over all clusters to find closes one
      int32 nearest_cluster_index = INVALID_INDEX;
      float nearest_cluster_sqr_dist = BIG_NUMBER;
      for (int32 j = 0; j < clusters.Count() ; j++) {
        const float sqr_dist = (pos - clusters[j]).SizeSquared();
        if (sqr_dist < nearest_cluster_sqr_dist) {
          nearest_cluster_sqr_dist = sqr_dist;
          nearest_cluster_index = j;
        }
      }
      // Update its info with this point
      if (nearest_cluster_index != INVALID_INDEX) {
        cluster_data[nearest_cluster_index].cluster_pos_accum += pos;
        cluster_data[nearest_cluster_index].cluster_size++;
      }
    }

    // All points classified - update cluster center as average of membership
    for (int32 i = 0; i < clusters.Count(); i++) {
      if (cluster_data[i].cluster_size > 0) {
        clusters[i] = cluster_data[i].cluster_pos_accum / (float)cluster_data[i].cluster_size;
      }
    }
  }

  // so now after we have possible cluster centers we want to remove the ones that are outliers and not part of the main cluster
  for (int32 i = 0; i < cluster_data.Count(); i++) {
    if (cluster_data[i].cluster_size < connection_to_bea_valid_count) {
      clusters.RemoveAt(i);
    }
  }
}

namespace MathRounding_internal {

float TruncateToHalfIfClose(float f)
{
  float value_to_fudge_integral_part = 0.f;
  float value_to_fudge_fractional_part = Math::Modf(f, &value_to_fudge_integral_part);
  if (f < 0.f) {
    return value_to_fudge_integral_part + ((Math::IsNearlyEqual(value_to_fudge_fractional_part, -0.5f)) ? -0.5f : value_to_fudge_fractional_part);
  }
  else {
    return value_to_fudge_integral_part + ((Math::IsNearlyEqual(value_to_fudge_fractional_part, 0.5f)) ? 0.5f : value_to_fudge_fractional_part);
  }
}

double TruncateToHalfIfClose(double f)
{
  double value_to_fudge_integral_part = 0.0;
  double value_to_fudge_fractional_part = Math::Modf(f, &value_to_fudge_integral_part);
  if (f < 0.0) {
    return value_to_fudge_integral_part + ((Math::IsNearlyEqual(value_to_fudge_fractional_part, -0.5)) ? -0.5 : value_to_fudge_fractional_part);
  }
  else {
    return value_to_fudge_integral_part + ((Math::IsNearlyEqual(value_to_fudge_fractional_part, 0.5)) ? 0.5 : value_to_fudge_fractional_part);
  }
}

} // namespace MathRounding_internal

float Math::RoundHalfToEven(float f)
{
  f = MathRounding_internal::TruncateToHalfIfClose(f);

  const bool is_negative = f < 0.f;
  const bool value_is_even = static_cast<uint32>(FloorToFloat(((is_negative) ? -f : f))) % 2 == 0;
  if (value_is_even) {
    // Round towards value (eg, value is -2.5 or 2.5, and should become -2 or 2)
    return (is_negative) ? FloorToFloat(f + 0.5f) : CeilToFloat(f - 0.5f);
  }
  else {
    // Round away from value (eg, value is -3.5 or 3.5, and should become -4 or 4)
    return (is_negative) ? CeilToFloat(f - 0.5f) : FloorToFloat(f + 0.5f);
  }
}

double Math::RoundHalfToEven(double f)
{
  f = MathRounding_internal::TruncateToHalfIfClose(f);

  const bool is_negative = f < 0.0;
  const bool value_is_even = static_cast<uint64>(Math::FloorToDouble(((is_negative) ? -f : f))) % 2 == 0;
  if (value_is_even) {
    // Round towards value (eg, value is -2.5 or 2.5, and should become -2 or 2)
    return (is_negative) ? FloorToDouble(f + 0.5) : CeilToDouble(f - 0.5);
  }
  else {
    // Round away from value (eg, value is -3.5 or 3.5, and should become -4 or 4)
    return (is_negative) ? CeilToDouble(f - 0.5) : FloorToDouble(f + 0.5);
  }
}

float Math::RoundHalfFromZero(float f)
{
  f = MathRounding_internal::TruncateToHalfIfClose(f);
  return (f < 0.f) ? CeilToFloat(f - 0.5f) : FloorToFloat(f + 0.5f);
}

double Math::RoundHalfFromZero(double f)
{
  f = MathRounding_internal::TruncateToHalfIfClose(f);
  return (f < 0.0) ? CeilToDouble(f - 0.5) : FloorToDouble(f + 0.5);
}

float Math::RoundHalfToZero(float f)
{
  f = MathRounding_internal::TruncateToHalfIfClose(f);
  return (f < 0.f) ? FloorToFloat(f + 0.5f) : CeilToFloat(f - 0.5f);
}

double Math::RoundHalfToZero(double f)
{
  f = MathRounding_internal::TruncateToHalfIfClose(f);
  return (f < 0.0) ? FloorToDouble(f + 0.5) : CeilToDouble(f - 0.5);
}

//bool Math::MemoryTest(void* BaseAddress, uint32 NumBytes)
//{
//  volatile uint32* Ptr;
//  uint32 NumDwords = NumBytes / 4;
//  uint32 TestWords[2] = { 0xdeadbeef, 0x1337c0de };
//  bool bSucceeded = true;
//
//  for (int32 TestIndex = 0; TestIndex < 2; ++TestIndex) {
//    // Fill the memory with a pattern.
//    Ptr = (uint32*)BaseAddress;
//    for (uint32 Index = 0; Index < NumDwords; ++Index) {
//      *Ptr = TestWords[TestIndex];
//      Ptr++;
//    }
//
//    // Check that each uint32 is still ok and overwrite it with the complement.
//    Ptr = (uint32*)BaseAddress;
//    for (uint32 Index = 0; Index < NumDwords; ++Index) {
//      if (*Ptr != TestWords[TestIndex]) {
//        CPlatformMisc::LowLevelOutputDebugStringf(TEXT("Failed memory test at 0x%08x, wrote: 0x%08x, read: 0x%08x\n"), Ptr, TestWords[TestIndex], *Ptr);
//        bSucceeded = false;
//      }
//      *Ptr = ~TestWords[TestIndex];
//      Ptr++;
//    }
//
//    // Check again, now going backwards in memory.
//    Ptr = ((uint32*)BaseAddress) + NumDwords;
//    for (uint32 Index = 0; Index < NumDwords; ++Index) {
//      Ptr--;
//      if (*Ptr != ~TestWords[TestIndex]) {
//        CPlatformMisc::LowLevelOutputDebugStringf(TEXT("Failed memory test at 0x%08x, wrote: 0x%08x, read: 0x%08x\n"), Ptr, ~TestWords[TestIndex], *Ptr);
//        bSucceeded = false;
//      }
//      *Ptr = TestWords[TestIndex];
//    }
//  }
//
//  return bSucceeded;
//}
//
///**
// * Converts a string to it's numeric equivalent, ignoring whitespace.
// * "123  45" - becomes 12, 345
// *
// * \param Value - The string to convert.
// *
// * \return The converted value.
// */
//float val(const String& Value)
//{
//  float RetValue = 0;
//
//  for (int32 i = 0; i < Value.Len(); ++i) {
//    String Char = Value.Mid(i, 1);
//
//    if (Char >= TEXT("0") && Char <= TEXT("9")) {
//      RetValue *= 10;
//      RetValue += CCharTraits::Atoi(*Char);
//    }
//    else {
//      if (Char != TEXT(" ")) {
//        break;
//      }
//    }
//  }
//
//  return RetValue;
//}
//
//String GrabChar(String* Str)
//{
//  String GrabChar;
//  if (Str->Len()) {
//    do {
//      GrabChar = Str->Left(1);
//      *Str = Str->Mid(1);
//    } while (GrabChar == TEXT(" "));
//  }
//  else {
//    GrabChar = TEXT("");
//  }
//
//  return GrabChar;
//}
//
//bool SubEval(String* pStr, float* pResult, int32 Prec)
//{
//  String c;
//  float v, w, N;
//
//  v = w = N = 0.f;
//
//  c = GrabChar(pStr);
//
//  if ((c >= TEXT("0") && c <= TEXT("9")) || c == TEXT(".")) { // Number
//    v = 0;
//    while (c >= TEXT("0") && c <= TEXT("9")) {
//      v = v * 10 + val(c);
//      c = GrabChar(pStr);
//    }
//
//    if (c == TEXT(".")) {
//      N = 0.1f;
//      c = GrabChar(pStr);
//
//      while (c >= TEXT("0") && c <= TEXT("9")) {
//        v = v + N * val(c);
//        N = N / 10.f;
//        c = GrabChar(pStr);
//      }
//    }
//  }
//  else if (c == TEXT("(")) { // Opening parenthesis
//    if (!SubEval(pStr, &v, 0)) {
//      return 0;
//    }
//    c = GrabChar(pStr);
//  }
//  else if (c == TEXT("-")) { // Negation
//    if (!SubEval(pStr, &v, 1000)) {
//      return 0;
//    }
//    v = -v;
//    c = GrabChar(pStr);
//  }
//  else if (c == TEXT("+")) { // Positive
//    if (!SubEval(pStr, &v, 1000)) {
//      return 0;
//    }
//    c = GrabChar(pStr);
//  }
//  else if (c == TEXT("@")) { // Square root
//    if (!SubEval(pStr, &v, 1000)) {
//      return 0;
//    }
//
//    if (v < 0) {
//      fun_log(LogFunMath, Info, TEXT("Expression Error : Can't take square root of negative number"));
//      return 0;
//    }
//    else {
//      v = Math::Sqrt(v);
//    }
//
//    c = GrabChar(pStr);
//  }
//  else // Error {
//    fun_log(LogFunMath, Info, TEXT("Expression Error : No value recognized"));
//    return 0;
//  }
//
//PrecLoop:
//  if (c == TEXT("")) {
//    *pResult = v;
//    return 1;
//  }
//  else if (c == TEXT(")")) {
//    *pStr = String(TEXT(")")) + *pStr;
//    *pResult = v;
//    return 1;
//  }
//  else if (c == TEXT("+")) {
//    if (Prec > 1) {
//      *pResult = v;
//      *pStr = c + *pStr;
//      return 1;
//    }
//    else {
//      if (SubEval(pStr, &w, 2)) {
//        v = v + w;
//        c = GrabChar(pStr);
//        goto PrecLoop;
//      }
//      else {
//        return 0;
//      }
//    }
//  }
//  else if (c == TEXT("-")) {
//    if (Prec > 1) {
//      *pResult = v;
//      *pStr = c + *pStr;
//      return 1;
//    }
//    else {
//      if (SubEval(pStr, &w, 2)) {
//        v = v - w;
//        c = GrabChar(pStr);
//        goto PrecLoop;
//      }
//      else {
//        return 0;
//      }
//    }
//  }
//  else if (c == TEXT("/")) {
//    if (Prec > 2) {
//      *pResult = v;
//      *pStr = c + *pStr;
//      return 1;
//    }
//    else {
//      if (SubEval(pStr, &w, 3)) {
//        if (w == 0) {
//          fun_log(LogFunMath, Info, TEXT("Expression Error : Division by zero isn't allowed"));
//          return 0;
//        }
//        else {
//          v = v / w;
//          c = GrabChar(pStr);
//          goto PrecLoop;
//        }
//      }
//      else {
//        return 0;
//      }
//    }
//  }
//  else if (c == TEXT("%")) {
//    if (Prec > 2) {
//      *pResult = v;
//      *pStr = c + *pStr;
//      return 1;
//    }
//    else {
//      if (SubEval(pStr, &w, 3)) {
//        if (w == 0) {
//          fun_log(LogFunMath, Info, TEXT("Expression Error : Modulo zero isn't allowed"));
//          return 0;
//        }
//        else {
//          v = (int32)v % (int32)w;
//          c = GrabChar(pStr);
//          goto PrecLoop;
//        }
//      }
//      else {
//        return 0;
//      }
//    }
//  }
//  else if (c == TEXT("*")) {
//    if (Prec > 3) {
//      *pResult = v;
//      *pStr = c + *pStr;
//      return 1;
//    }
//    else {
//      if (SubEval(pStr, &w, 4)) {
//        v = v * w;
//        c = GrabChar(pStr);
//        goto PrecLoop;
//      }
//      else {
//        return 0;
//      }
//    }
//  }
//  else {
//    fun_log(LogFunMath, Info, TEXT("Expression Error : Unrecognized Operator"));
//  }
//
//  *pResult = v;
//  return 1;
//}
//
//bool Math::Eval(String str, float& out_value)
//{
//  bool result = true;
//
//  // Check for a matching number of brackets right up front.
//  int32 bracket_count = 0;
//  for (int32 i = 0; i < str.Len(); ++i) {
//    if (str.Mid(i, 1) == "(") {
//      bracket_count++;
//    }
//    if (str.Mid(i, 1) == ")") {
//      bracket_count--;
//    }
//  }
//
//  if (bracket_count != 0) {
//    fun_log(LogFunMath, Info, "Expression Error : Mismatched brackets");
//    result = false;
//  }
//
//  else {
//    if (!SubEval(&str, &out_value, 0)) {
//      fun_log(LogFunMath, Info, "Expression Error : Error in expression");
//      result = false;
//    }
//  }
//
//  return result;
//}

float Math::FixedTurn(float current, float desired, float delta_rate)
{
  if (delta_rate == 0.f) {
    return Rotator::ClampAxis(current);
  }

  if (delta_rate >= 360.f) {
    return Rotator::ClampAxis(desired);
  }

  float result = Rotator::ClampAxis(current);
  current = result;
  desired = Rotator::ClampAxis(desired);

  if (current > desired) {
    if (current - desired < 180.f) {
      result -= Math::Min((current - desired), Math::Abs(delta_rate));
    }
    else {
      result += Math::Min((desired + 360.f - current), Math::Abs(delta_rate));
    }
  }
  else {
    if (desired - current < 180.f) {
      result += Math::Min((desired - current), Math::Abs(delta_rate));
    }
    else {
      result -= Math::Min((current + 360.f - desired), Math::Abs(delta_rate));
    }
  }
  return Rotator::ClampAxis(result);
}

float Math::ClampAngle(float angle_degrees, float min_angle_degrees, float max_angle_degrees)
{
  float const max_delta = Rotator::ClampAxis(max_angle_degrees - min_angle_degrees) * 0.5f; // 0..180
  float const range_center = Rotator::ClampAxis(min_angle_degrees + max_delta);             // 0..360
  float const delta_from_center = Rotator::NormalizeAxis(angle_degrees - range_center);     // -180..180

  // maybe clamp to nearest edge
  if (delta_from_center > max_delta) {
    return Rotator::NormalizeAxis(range_center + max_delta);
  }
  else if (delta_from_center < -max_delta) {
    return Rotator::NormalizeAxis(range_center - max_delta);
  }

  // already in range, just return it
  return Rotator::NormalizeAxis(angle_degrees);
}

void Math::ApplyScaleToFloat(float& dst, const Vector& delta_scale, float magnitude)
{
  const float multiplier = (delta_scale.x > 0.f || delta_scale.y > 0.f || delta_scale.z > 0.f) ? magnitude : -magnitude;
  dst += multiplier * delta_scale.Size();
  dst = Math::Max(0.f, dst);
}

void Math::CartesianToPolar(const Vector2& cartesian, Vector2& out_polar)
{
  out_polar.x = Sqrt(Square(cartesian.x) + Square(cartesian.y));
  out_polar.y = Atan2(cartesian.y, cartesian.x);
}

void Math::PolarToCartesian(const Vector2& polar, Vector2& out_cartesian)
{
  out_cartesian.x = polar.x * Cos(polar.y);
  out_cartesian.y = polar.x * Sin(polar.y);
}

//TODO?
//bool CRandomStream::ExportTextItem(String& ValueStr, CRandomStream const& DefaultValue, class UObject* Parent, int32 PortFlags, class UObject* ExportRootScope) const
//{
//  if (0 != (PortFlags & EPropertyPortFlags::PPF_ExportCpp)) {
//    ValueStr += String::Format(TEXT("CRandomStream(%i)"), DefaultValue.GetInitialSeed());
//    return true;
//  }
//  return false;
//}

} // namespace fun
