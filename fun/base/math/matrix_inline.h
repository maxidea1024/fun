#pragma once

namespace fun {

//
// Matrix FUN_ALWAYS_INLINE functions.
//

FUN_ALWAYS_INLINE Matrix::Matrix() {}

FUN_ALWAYS_INLINE Matrix::Matrix(const Vector4& x, const Vector4& y, const Vector4& z, const Vector4& w) {
  m[0][0] = x.x; m[0][1] = x.y; m[0][2] = x.z; m[0][3] = x.w;
  m[1][0] = y.x; m[1][1] = y.y; m[1][2] = y.z; m[1][3] = y.w;
  m[2][0] = z.x; m[2][1] = z.y; m[2][2] = z.z; m[2][3] = z.w;
  m[3][0] = w.x; m[3][1] = w.y; m[3][2] = w.z; m[3][3] = w.w;
}

FUN_ALWAYS_INLINE Matrix::Matrix(const Vector& x, const Vector& y, const Vector& z, const Vector& w) {
  m[0][0] = x.x; m[0][1] = x.y; m[0][2] = x.z; m[0][3] = 0.f;
  m[1][0] = y.x; m[1][1] = y.y; m[1][2] = y.z; m[1][3] = 0.f;
  m[2][0] = z.x; m[2][1] = z.y; m[2][2] = z.z; m[2][3] = 0.f;
  m[3][0] = w.x; m[3][1] = w.y; m[3][2] = w.z; m[3][3] = 1.f;
}

FUN_ALWAYS_INLINE void Matrix::SetIdentity() {
  m[0][0] = 1; m[0][1] = 0; m[0][2] = 0; m[0][3] = 0;
  m[1][0] = 0; m[1][1] = 1; m[1][2] = 0; m[1][3] = 0;
  m[2][0] = 0; m[2][1] = 0; m[2][2] = 1; m[2][3] = 0;
  m[3][0] = 0; m[3][1] = 0; m[3][2] = 0; m[3][3] = 1;
}

FUN_ALWAYS_INLINE void Matrix::operator *= (const Matrix& other) {
  VectorMatrixMultiply(this, this, &other);
}

FUN_ALWAYS_INLINE Matrix Matrix::operator * (const Matrix& other) const {
  Matrix result;
  VectorMatrixMultiply(&result, this, &other);
  return result;
}

FUN_ALWAYS_INLINE Matrix Matrix::operator+(const Matrix& other) const {
  Matrix result;
  for (int32 x = 0; x < 4; ++x) {
    for (int32 y = 0; y < 4; ++y) {
      result.m[x][y] = m[x][y] + other.m[x][y];
    }
  }
  return result;
}

FUN_ALWAYS_INLINE void Matrix::operator += (const Matrix& other) {
  *this = *this + other;
}

FUN_ALWAYS_INLINE Matrix Matrix::operator * (float other) const {
  Matrix result;
  for (int32 x = 0; x < 4; ++x) {
    for (int32 y = 0; y < 4; ++y) {
      result.m[x][y] = m[x][y] * other;
    }
  }
  return result;
}

FUN_ALWAYS_INLINE void Matrix::operator *= (float other) {
  *this = *this * other;
}

// Comparison operators.

FUN_ALWAYS_INLINE bool Matrix::operator == (const Matrix& other) const {
  for (int32 x = 0; x < 4; ++x) {
    for (int32 y = 0; y < 4; ++y) {
      if (m[x][y] != other.m[x][y]) {
        return false;
      }
    }
  }
  return true;
}

// Error-tolerant comparison.
FUN_ALWAYS_INLINE bool Matrix::Equals(const Matrix& other, float tolerance/*=KINDA_SMALL_NUMBER*/) const {
  for (int32 x = 0; x < 4; ++x) {
    for (int32 y = 0; y < 4; ++y) {
      if (Math::Abs(m[x][y] - other.m[x][y]) > tolerance) {
        return false;
      }
    }
  }

  return true;
}

FUN_ALWAYS_INLINE bool Matrix::operator != (const Matrix& other) const {
  return !(*this == other);
}

//
// Homogeneous transform.
//

FUN_ALWAYS_INLINE Vector4 Matrix::TransformVector4(const Vector4& p) const {
  Vector4 result;
  VectorRegister vec_p = VectorLoadAligned(&p);
  VectorRegister vec_r = VectorTransformVector(vec_p, this);
  VectorStoreAligned(vec_r, &result);
  return result;
}

//
// transform position
//

FUN_ALWAYS_INLINE Vector4 Matrix::TransformPosition(const Vector& v) const {
  return TransformVector4(Vector4(v.x, v.y, v.z, 1.f));
}

FUN_ALWAYS_INLINE Vector Matrix::InverseTransformPosition(const Vector& v) const {
  //TODO 역행렬을 구하지 않고 하면 오류가 발생하려나??
  Matrix inv = this->InverseFast();
  return inv.TransformPosition(v);
}

//
// transform vector
//

FUN_ALWAYS_INLINE Vector4 Matrix::TransformVector(const Vector& v) const {
  return TransformVector4(Vector4(v.x, v.y, v.z, 0.f));
}

FUN_ALWAYS_INLINE Vector Matrix::InverseTransformVector(const Vector& v) const {
  Matrix inv_self = this->InverseFast();
  return inv_self.TransformVector(v);
}

//
// Transpose.
//

FUN_ALWAYS_INLINE Matrix Matrix::GetTransposed() const {
  Matrix result;

  result.m[0][0] = m[0][0];
  result.m[0][1] = m[1][0];
  result.m[0][2] = m[2][0];
  result.m[0][3] = m[3][0];

  result.m[1][0] = m[0][1];
  result.m[1][1] = m[1][1];
  result.m[1][2] = m[2][1];
  result.m[1][3] = m[3][1];

  result.m[2][0] = m[0][2];
  result.m[2][1] = m[1][2];
  result.m[2][2] = m[2][2];
  result.m[2][3] = m[3][2];

  result.m[3][0] = m[0][3];
  result.m[3][1] = m[1][3];
  result.m[3][2] = m[2][3];
  result.m[3][3] = m[3][3];

  return result;
}

//
// Determinant.
//

FUN_ALWAYS_INLINE float Matrix::Determinant() const {
  return  m[0][0] * (
        m[1][1] * (m[2][2] * m[3][3] - m[2][3] * m[3][2]) -
        m[2][1] * (m[1][2] * m[3][3] - m[1][3] * m[3][2]) +
        m[3][1] * (m[1][2] * m[2][3] - m[1][3] * m[2][2])
        ) -
      m[1][0] * (
        m[0][1] * (m[2][2] * m[3][3] - m[2][3] * m[3][2]) -
        m[2][1] * (m[0][2] * m[3][3] - m[0][3] * m[3][2]) +
        m[3][1] * (m[0][2] * m[2][3] - m[0][3] * m[2][2])
        ) +
      m[2][0] * (
        m[0][1] * (m[1][2] * m[3][3] - m[1][3] * m[3][2]) -
        m[1][1] * (m[0][2] * m[3][3] - m[0][3] * m[3][2]) +
        m[3][1] * (m[0][2] * m[1][3] - m[0][3] * m[1][2])
        ) -
      m[3][0] * (
        m[0][1] * (m[1][2] * m[2][3] - m[1][3] * m[2][2]) -
        m[1][1] * (m[0][2] * m[2][3] - m[0][3] * m[2][2]) +
        m[2][1] * (m[0][2] * m[1][3] - m[0][3] * m[1][2])
        );
}

FUN_ALWAYS_INLINE float Matrix::RotDeterminant() const {
  return
    m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
    m[1][0] * (m[0][1] * m[2][2] - m[0][2] * m[2][1]) +
    m[2][0] * (m[0][1] * m[1][2] - m[0][2] * m[1][1]);
}

//
// Inverse.
//

FUN_ALWAYS_INLINE Matrix Matrix::InverseFast() const {
  // If we're in non final release, then make sure we're not creating NaNs
#if !(FUN_BUILD_SHIPPING || FUN_BUILD_TEST)
  // Check for zero scale matrix to invert
  if (GetScaledAxis(Axis::X).IsNearlyZero(SMALL_NUMBER) &&
      GetScaledAxis(Axis::Y).IsNearlyZero(SMALL_NUMBER) &&
      GetScaledAxis(Axis::Z).IsNearlyZero(SMALL_NUMBER)) {
    //TODO?
    //fun_log(LogFunMath, Error, "Matrix::InverseFast(), trying to invert a NIL matrix, this results in NaNs! Use Inverse() instead.");
    ENSURE_MSGF(false, "Matrix::InverseFast(), trying to invert a NIL matrix, this results in NaNs! Use Inverse() instead.");
  }
#endif
  Matrix result;
  VectorMatrixInverse(&result, this);
  return result;
}

FUN_ALWAYS_INLINE Matrix Matrix::Inverse() const {
  Matrix result;

  // Check for zero scale matrix to invert
  if (GetScaledAxis(Axis::X).IsNearlyZero(SMALL_NUMBER) &&
      GetScaledAxis(Axis::Y).IsNearlyZero(SMALL_NUMBER) &&
      GetScaledAxis(Axis::Z).IsNearlyZero(SMALL_NUMBER)) {
    // just set to zero - avoids unsafe inverse of zero and duplicates what QNANs were resulting in before (scaling away all children)
    result = Matrix::Identity;
  } else {
    const float det = Determinant();

    if (det == 0.f) {
      result = Matrix::Identity;
    } else {
      VectorMatrixInverse(&result, this);
    }
  }

  return result;
}

FUN_ALWAYS_INLINE Matrix Matrix::TransposeAdjoint() const {
  Matrix ta;

  ta.m[0][0] = this->m[1][1] * this->m[2][2] - this->m[1][2] * this->m[2][1];
  ta.m[0][1] = this->m[1][2] * this->m[2][0] - this->m[1][0] * this->m[2][2];
  ta.m[0][2] = this->m[1][0] * this->m[2][1] - this->m[1][1] * this->m[2][0];
  ta.m[0][3] = 0.f;

  ta.m[1][0] = this->m[2][1] * this->m[0][2] - this->m[2][2] * this->m[0][1];
  ta.m[1][1] = this->m[2][2] * this->m[0][0] - this->m[2][0] * this->m[0][2];
  ta.m[1][2] = this->m[2][0] * this->m[0][1] - this->m[2][1] * this->m[0][0];
  ta.m[1][3] = 0.f;

  ta.m[2][0] = this->m[0][1] * this->m[1][2] - this->m[0][2] * this->m[1][1];
  ta.m[2][1] = this->m[0][2] * this->m[1][0] - this->m[0][0] * this->m[1][2];
  ta.m[2][2] = this->m[0][0] * this->m[1][1] - this->m[0][1] * this->m[1][0];
  ta.m[2][3] = 0.f;

  ta.m[3][0] = 0.f;
  ta.m[3][1] = 0.f;
  ta.m[3][2] = 0.f;
  ta.m[3][3] = 1.f;

  return ta;
}

FUN_ALWAYS_INLINE void Matrix::RemoveScaling(float tolerance/* = SMALL_NUMBER*/) {
  // For each row, find magnitude, and if its non-zero re-scale so its unit length.
  const float length_sqr0 = (m[0][0] * m[0][0]) + (m[0][1] * m[0][1]) + (m[0][2] * m[0][2]);
  const float length_sqr1 = (m[1][0] * m[1][0]) + (m[1][1] * m[1][1]) + (m[1][2] * m[1][2]);
  const float length_sqr2 = (m[2][0] * m[2][0]) + (m[2][1] * m[2][1]) + (m[2][2] * m[2][2]);
  const float scale0 = Math::FloatSelect(length_sqr0 - tolerance, Math::InvSqrt(length_sqr0), 1.f);
  const float scale1 = Math::FloatSelect(length_sqr1 - tolerance, Math::InvSqrt(length_sqr1), 1.f);
  const float scale2 = Math::FloatSelect(length_sqr2 - tolerance, Math::InvSqrt(length_sqr2), 1.f);
  m[0][0] *= scale0;
  m[0][1] *= scale0;
  m[0][2] *= scale0;
  m[1][0] *= scale1;
  m[1][1] *= scale1;
  m[1][2] *= scale1;
  m[2][0] *= scale2;
  m[2][1] *= scale2;
  m[2][2] *= scale2;
}

FUN_ALWAYS_INLINE Matrix Matrix::GetMatrixWithoutScale(float tolerance/* = SMALL_NUMBER*/) const {
  Matrix result = *this;
  result.RemoveScaling(tolerance);
  return result;
}

// 스케일 팩터를 구함과 동시에 이 행렬에서 스케일 요소를 제거함.
FUN_ALWAYS_INLINE Vector Matrix::ExtractScaling(float tolerance/* = SMALL_NUMBER*/) {
  Vector scale(0, 0, 0);

  // For each row, find magnitude, and if its non-zero re-scale so its unit length.
  const float length_sqr0 = (m[0][0] * m[0][0]) + (m[0][1] * m[0][1]) + (m[0][2] * m[0][2]);
  const float length_sqr1 = (m[1][0] * m[1][0]) + (m[1][1] * m[1][1]) + (m[1][2] * m[1][2]);
  const float length_sqr2 = (m[2][0] * m[2][0]) + (m[2][1] * m[2][1]) + (m[2][2] * m[2][2]);

  if (length_sqr0 > tolerance) {
    float scale0 = Math::Sqrt(length_sqr0);
    scale[0] =  scale0;
    float one_over_scale0 = 1.f / scale0;
    m[0][0] *= one_over_scale0;
    m[0][1] *= one_over_scale0;
    m[0][2] *= one_over_scale0;
  } else {
    scale[0] =  0;
  }

  if (length_sqr1 > tolerance) {
    float scale1 = Math::Sqrt(length_sqr1);
    scale[1] =  scale1;
    float one_over_scale1 = 1.f / scale1;
    m[1][0] *= one_over_scale1;
    m[1][1] *= one_over_scale1;
    m[1][2] *= one_over_scale1;
  } else {
    scale[1] =  0;
  }

  if (length_sqr2 > tolerance) {
    float scale2 = Math::Sqrt(length_sqr2);
    scale[2] =  scale2;
    float one_over_scale2 = 1.f / scale2;
    m[2][0] *= one_over_scale2;
    m[2][1] *= one_over_scale2;
    m[2][2] *= one_over_scale2;
  } else {
    scale[2] = 0;
  }

  return scale;
}

FUN_ALWAYS_INLINE Vector Matrix::GetScaleVector(float tolerance/* = SMALL_NUMBER*/) const {
  Vector scale(1, 1, 1);

  // For each row, find magnitude, and if its non-zero re-scale so its unit length.
  for (int32 i = 0; i < 3; i++) {
    const float length_sqr = (m[i][0] * m[i][0]) + (m[i][1] * m[i][1]) + (m[i][2] * m[i][2]);
    if (length_sqr > tolerance) {
      scale[i] = Math::Sqrt(length_sqr);
    } else {
      scale[i] = 0.f;
    }
  }

  return scale;
}

FUN_ALWAYS_INLINE Matrix Matrix::RemoveTranslation() const {
  Matrix result = *this;
  result.m[3][0] = 0.f;
  result.m[3][1] = 0.f;
  result.m[3][2] = 0.f;
  return result;
}

FUN_ALWAYS_INLINE Matrix Matrix::ConcatTranslation(const Vector& translation) const {
  Matrix result;

  float* __restrict dst = &result.m[0][0];
  const float* __restrict src = &m[0][0];
  const float* __restrict trans = &translation.x;

  dst[0] = src[0];
  dst[1] = src[1];
  dst[2] = src[2];
  dst[3] = src[3];
  dst[4] = src[4];
  dst[5] = src[5];
  dst[6] = src[6];
  dst[7] = src[7];
  dst[8] = src[8];
  dst[9] = src[9];
  dst[10] = src[10];
  dst[11] = src[11];
  dst[12] = src[12] + trans[0];
  dst[13] = src[13] + trans[1];
  dst[14] = src[14] + trans[2];
  dst[15] = src[15];

  return result;
}

FUN_ALWAYS_INLINE bool Matrix::ContainsNaN() const {
  for (int32 x = 0; x < 4; ++x) {
    for (int32 y = 0; y < 4; ++y) {
      if (Math::IsNaN(m[x][y]) || !Math::IsFinite(m[x][y])) {
        return true;
      }
    }
  }

  return false;
}

FUN_ALWAYS_INLINE float Matrix::GetMaximumAxisScale() const {
  const float max_row_scale_squared = Math::Max(
      GetScaledAxis(Axis::X).SizeSquared(),
      GetScaledAxis(Axis::Y).SizeSquared(),
      GetScaledAxis(Axis::Z).SizeSquared());
  return Math::Sqrt(max_row_scale_squared);
}

FUN_ALWAYS_INLINE void Matrix::ScaleTranslation(const Vector& scale) {
  m[3][0] *= scale.x;
  m[3][1] *= scale.y;
  m[3][2] *= scale.z;
}

FUN_ALWAYS_INLINE Vector Matrix::GetOrigin() const {
  return Vector(m[3][0], m[3][1], m[3][2]);
}

FUN_ALWAYS_INLINE Vector Matrix::GetScaledAxis(Axis axis) const {
  switch (axis) {
    case Axis::X:
      return Vector(m[0][0], m[0][1], m[0][2]);

    case Axis::Y:
      return Vector(m[1][0], m[1][1], m[1][2]);

    case Axis::Z:
      return Vector(m[2][0], m[2][1], m[2][2]);

    default:
      ENSURE(0);
      return Vector::ZeroVector;
  }
}

FUN_ALWAYS_INLINE void Matrix::GetScaledAxes(Vector& x, Vector& y, Vector& z) const {
  x.x = m[0][0]; x.y = m[0][1]; x.z = m[0][2];
  y.x = m[1][0]; y.y = m[1][1]; y.z = m[1][2];
  z.x = m[2][0]; z.y = m[2][1]; z.z = m[2][2];
}

FUN_ALWAYS_INLINE Vector Matrix::GetUnitAxis(Axis axis) const {
  return GetScaledAxis(axis).GetSafeNormal();
}

FUN_ALWAYS_INLINE void Matrix::GetUnitAxes(Vector& x, Vector& y, Vector& z) const {
  GetScaledAxes(x, y, z);
  x.Normalize();
  y.Normalize();
  z.Normalize();
}

FUN_ALWAYS_INLINE void Matrix::SetAxis(int32 i, const Vector& axis) {
  fun_check_dbg(i >= 0 && i <= 2);
  m[i][0] = axis.x;
  m[i][1] = axis.y;
  m[i][2] = axis.z;
}

FUN_ALWAYS_INLINE void Matrix::SetOrigin(const Vector& new_origin) {
  m[3][0] = new_origin.x;
  m[3][1] = new_origin.y;
  m[3][2] = new_origin.z;
}

FUN_ALWAYS_INLINE void Matrix::SetAxes(const Vector* axis0 /*= nullptr*/, const Vector* axis1 /*= nullptr*/, const Vector* axis2 /*= nullptr*/, const Vector* origin /*= nullptr*/) {
  if (axis0) {
    m[0][0] = axis0->x;
    m[0][1] = axis0->y;
    m[0][2] = axis0->z;
  }

  if (axis1) {
    m[1][0] = axis1->x;
    m[1][1] = axis1->y;
    m[1][2] = axis1->z;
  }

  if (axis2) {
    m[2][0] = axis2->x;
    m[2][1] = axis2->y;
    m[2][2] = axis2->z;
  }

  if (origin) {
    m[3][0] = origin->x;
    m[3][1] = origin->y;
    m[3][2] = origin->z;
  }
}

FUN_ALWAYS_INLINE Vector Matrix::GetColumn(int32 i) const {
  fun_check_dbg(i >= 0 && i <= 3);
  return Vector(m[0][i], m[1][i], m[2][i]);
}

FUN_ALWAYS_INLINE bool MakeFrustumPlane(float a, float b, float c, float d, Plane& out_plane) {
  const float length_squared = a*a + b*b + c*c;
  if (length_squared > DELTA*DELTA) {
    const float inv_length = Math::InvSqrt(length_squared);
    out_plane = Plane(-a * inv_length, -b * inv_length, -c * inv_length, d * inv_length);
    return true;
  } else {
    return false;
  }
}


//
// Frustum plane extraction.
//

FUN_ALWAYS_INLINE bool Matrix::GetFrustumNearPlane(Plane& out_plane) const {
  return MakeFrustumPlane(
    m[0][2],
    m[1][2],
    m[2][2],
    m[3][2],
    out_plane);
}

FUN_ALWAYS_INLINE bool Matrix::GetFrustumFarPlane(Plane& out_plane) const {
  return MakeFrustumPlane(
    m[0][3] - m[0][2],
    m[1][3] - m[1][2],
    m[2][3] - m[2][2],
    m[3][3] - m[3][2],
    out_plane);
}

FUN_ALWAYS_INLINE bool Matrix::GetFrustumLeftPlane(Plane& out_plane) const {
  return MakeFrustumPlane(
    m[0][3] + m[0][0],
    m[1][3] + m[1][0],
    m[2][3] + m[2][0],
    m[3][3] + m[3][0],
    out_plane);
}

FUN_ALWAYS_INLINE bool Matrix::GetFrustumRightPlane(Plane& out_plane) const {
  return MakeFrustumPlane(
    m[0][3] - m[0][0],
    m[1][3] - m[1][0],
    m[2][3] - m[2][0],
    m[3][3] - m[3][0],
    out_plane);
}

FUN_ALWAYS_INLINE bool Matrix::GetFrustumTopPlane(Plane& out_plane) const {
  return MakeFrustumPlane(
    m[0][3] - m[0][1],
    m[1][3] - m[1][1],
    m[2][3] - m[2][1],
    m[3][3] - m[3][1],
    out_plane);
}

FUN_ALWAYS_INLINE bool Matrix::GetFrustumBottomPlane(Plane& out_plane) const {
  return MakeFrustumPlane(
    m[0][3] + m[0][1],
    m[1][3] + m[1][1],
    m[2][3] + m[2][1],
    m[3][3] + m[3][1],
    out_plane);
}


FUN_ALWAYS_INLINE void Matrix::Mirror(Axis mirror_axis, Axis flip_axis) {
  if (mirror_axis == Axis::X) {
    m[0][0] *= -1.f;
    m[1][0] *= -1.f;
    m[2][0] *= -1.f;

    m[3][0] *= -1.f;
  } else if (mirror_axis == Axis::Y) {
    m[0][1] *= -1.f;
    m[1][1] *= -1.f;
    m[2][1] *= -1.f;

    m[3][1] *= -1.f;
  } else if (mirror_axis == Axis::Z) {
    m[0][2] *= -1.f;
    m[1][2] *= -1.f;
    m[2][2] *= -1.f;

    m[3][2] *= -1.f;
  }

  if (flip_axis == Axis::X) {
    m[0][0] *= -1.f;
    m[0][1] *= -1.f;
    m[0][2] *= -1.f;
  } else if (flip_axis == Axis::Y) {
    m[1][0] *= -1.f;
    m[1][1] *= -1.f;
    m[1][2] *= -1.f;
  } else if (flip_axis == Axis::Z) {
    m[2][0] *= -1.f;
    m[2][1] *= -1.f;
    m[2][2] *= -1.f;
  }
}

FUN_ALWAYS_INLINE Matrix Matrix::ApplyScale(float scale) {
  return ScaleMatrix(scale) * (*this);
}


//
// Serializer.
//

FUN_ALWAYS_INLINE Archive& operator & (Archive& ar, Matrix& m) {
  ar & m.m[0][0] & m.m[0][1] & m.m[0][2] & m.m[0][3];
  ar & m.m[1][0] & m.m[1][1] & m.m[1][2] & m.m[1][3];
  ar & m.m[2][0] & m.m[2][1] & m.m[2][2] & m.m[2][3];
  ar & m.m[3][0] & m.m[3][1] & m.m[3][2] & m.m[3][3];
  return ar;
}

} // namespace fun
