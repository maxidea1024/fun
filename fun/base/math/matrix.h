#pragma once

namespace fun {

/**
 * 4x4 matrix of floating point values.
 * matrix-matrix multiplication happens with a pre-multiple of the transpose --
 * in other words, Res = Mat1.operator*(Mat2) means Res = Mat2^T * Mat1, as
 * opposed to Res = Mat1 * Mat2.
 * matrix elements are accessed with m[row_index][column_index].
 */
class Matrix {
 public:
  union {
    alignas(16) float m[4][4];
  };

  alignas(16) static FUN_BASE_API const Matrix Identity;

  Matrix();

  explicit Matrix(ForceInit_TAG) {
    UnsafeMemory::Memzero(this, sizeof(*this));
  }

  Matrix(const Vector4& x, const Vector4& y, const Vector4& z, const Vector4& w);
  Matrix(const Vector& x, const Vector& y, const Vector& z, const Vector& w);

  void SetIdentity();

  Matrix operator * (const Matrix& other) const;
  void operator *= (const Matrix& other);
  Matrix operator + (const Matrix& other) const;
  void operator += (const Matrix& other);
  Matrix operator * (float other) const;
  void operator *= (float other);
  bool operator == (const Matrix& other) const;
  bool Equals(const Matrix& other, float tolerance = KINDA_SMALL_NUMBER) const;
  bool operator != (const Matrix& other) const;

  Vector4 TransformVector4(const Vector4& v) const;
  Vector4 TransformPosition(const Vector& v) const;
  Vector InverseTransformPosition(const Vector& v) const;
  Vector4 TransformVector(const Vector& v) const;
  Vector InverseTransformVector(const Vector& v) const;

  Matrix GetTransposed() const;

  float Determinant() const;
  float RotDeterminant() const;

  Matrix InverseFast() const;
  Matrix Inverse() const;

  Matrix TransposeAdjoint() const;

  void RemoveScaling(float tolerance = SMALL_NUMBER);
  Matrix GetMatrixWithoutScale(float tolerance = SMALL_NUMBER) const;
  Vector ExtractScaling(float tolerance = SMALL_NUMBER);
  Vector GetScaleVector(float tolerance = SMALL_NUMBER) const;

  Matrix RemoveTranslation() const;
  Matrix ConcatTranslation(const Vector& translation) const;

  bool ContainsNaN() const;

  void ScaleTranslation(const Vector& scale_3d);

  float GetMaximumAxisScale() const;

  Matrix ApplyScale(float scale);

  Vector GetOrigin() const;

  Vector GetScaledAxis(Axis axis) const;

  void GetScaledAxes(Vector& x, Vector& y, Vector& z) const;

  Vector GetUnitAxis(Axis axis) const;

  void GetUnitAxes(Vector& x, Vector& y, Vector& z) const;

  void SetAxis(int32 i, const Vector& axis);

  void SetOrigin(const Vector& new_origin);

  void SetAxes( const Vector* axis0 = nullptr,
                const Vector* axis1 = nullptr,
                const Vector* axis2 = nullptr,
                const Vector* origin = nullptr);

  Vector GetColumn(int32 i) const;

  FUN_BASE_API Rotator ToRotator() const;

  FUN_BASE_API Quat ToQuat() const;

  // Frustum plane extraction.

  bool GetFrustumNearPlane(Plane& out_plane) const;
  bool GetFrustumFarPlane(Plane& out_plane) const;
  bool GetFrustumLeftPlane(Plane& out_plane) const;
  bool GetFrustumRightPlane(Plane& out_plane) const;
  bool GetFrustumTopPlane(Plane& out_plane) const;
  bool GetFrustumBottomPlane(Plane& out_plane) const;

  void Mirror(Axis mirror_axis, Axis flip_axis);

  FUN_BASE_API String ToString() const;

  void DebugPrint() const;

  FUN_BASE_API uint32 ComputeHash() const;

  friend FUN_BASE_API Archive& operator & (Archive& ar, Matrix& m);

  bool Serialize(Archive& ar) {
    ar & *this;
    return true;
  }

  void To3x4MatrixTranspose(float* out) const {
    const float* __restrict src = &(m[0][0]);
    float* __restrict dst = out;

    dst[0] = src[0]; // [0][0]
    dst[1] = src[4]; // [1][0]
    dst[2] = src[8]; // [2][0]
    dst[3] = src[12];  // [3][0]

    dst[4] = src[1]; // [0][1]
    dst[5] = src[5]; // [1][1]
    dst[6] = src[9]; // [2][1]
    dst[7] = src[13];  // [3][1]

    dst[8] = src[2]; // [0][2]
    dst[9] = src[6]; // [1][2]
    dst[10] = src[10]; // [2][2]
    dst[11] = src[14]; // [3][2]
  }
};


/**
 * A storage class for compile-time fixed size matrices.
 */
template <uint32 NumRows, uint32 NumColumns>
class TMatrix {
 public:
  // Variables.
  MS_ALIGN(16) float m[NumRows][NumColumns] GCC_ALIGN(16);

  // Constructor
  TMatrix();

  /**
   * Constructor
   *
   * \param InMatrix - Matrix reference
   */
  TMatrix(const Matrix& InMatrix);
};

template <uint32 NumRows, uint32 NumColumns>
FUN_ALWAYS_INLINE TMatrix<NumRows, NumColumns>::TMatrix() {}

template <uint32 NumRows, uint32 NumColumns>
FUN_ALWAYS_INLINE TMatrix<NumRows, NumColumns>::TMatrix(const Matrix& InMatrix) {
  for (uint32 row_index = 0; row_index < NumRows && row_index < 4; row_index++) {
    for (uint32 column_index = 0; column_index < NumColumns && column_index < 4; column_index++) {
      m[row_index][column_index] = InMatrix.m[row_index][column_index];
    }
  }
}


//
// BasisVectorMatrix
//

struct BasisVectorMatrix : public Matrix {
  // Create Basis matrix from 3 axis vectors and the origin
  BasisVectorMatrix(const Vector& x_axis,
                    const Vector& y_axis,
                    const Vector& z_axis,
                    const Vector& origin);
};

FUN_ALWAYS_INLINE BasisVectorMatrix::BasisVectorMatrix(const Vector& x_axis,
                                            const Vector& y_axis,
                                            const Vector& z_axis,
                                            const Vector& origin) {
  for (uint32 row_index = 0; row_index < 3; ++row_index) {
    m[row_index][0] = (&x_axis.x)[row_index];
    m[row_index][1] = (&y_axis.x)[row_index];
    m[row_index][2] = (&z_axis.x)[row_index];
    m[row_index][3] = 0.f;
  }
  m[3][0] = origin | x_axis;
  m[3][1] = origin | y_axis;
  m[3][2] = origin | z_axis;
  m[3][3] = 1.f;
}


//
// LookAtMatrix
//

struct LookAtMatrix : public Matrix {
  /**
   * Creates a view matrix given an eye position, a position to look at, and an up vector.
   * This does the same thing as D3DXMatrixLookAtLH.
   */
  LookAtMatrix(const Vector& eye_position, const Vector& look_at_position, const Vector& up_vector);
};


//
// inlines
//

FUN_ALWAYS_INLINE LookAtMatrix::LookAtMatrix(const Vector& eye_position, const Vector& look_at_position, const Vector& up_vector) {
  const Vector z_axis = (look_at_position - eye_position).GetSafeNormal();
  const Vector x_axis = (up_vector ^ z_axis).GetSafeNormal();
  const Vector y_axis = z_axis ^ x_axis;

  for (uint32 row_index = 0; row_index < 3; ++row_index) {
    m[row_index][0] = (&x_axis.x)[row_index];
    m[row_index][1] = (&y_axis.x)[row_index];
    m[row_index][2] = (&z_axis.x)[row_index];
    m[row_index][3] = 0.f;
  }
  m[3][0] = -eye_position | x_axis;
  m[3][1] = -eye_position | y_axis;
  m[3][2] = -eye_position | z_axis;
  m[3][3] = 1.f;
}

template <> struct IsPOD<Matrix> { enum { Value = true }; };


// very high quality 4x4 matrix inverse
static FUN_ALWAYS_INLINE void Inverse4x4(double* dst, const float* src) {
  const double s0  = (double)(src[ 0]); const double s1  = (double)(src[ 1]); const double s2  = (double)(src[ 2]); const double s3  = (double)(src[ 3]);
  const double s4  = (double)(src[ 4]); const double s5  = (double)(src[ 5]); const double s6  = (double)(src[ 6]); const double s7  = (double)(src[ 7]);
  const double s8  = (double)(src[ 8]); const double s9  = (double)(src[ 9]); const double s10 = (double)(src[10]); const double s11 = (double)(src[11]);
  const double s12 = (double)(src[12]); const double s13 = (double)(src[13]); const double s14 = (double)(src[14]); const double s15 = (double)(src[15]);

  double inv[16];
  inv[0]  =  s5 * s10 * s15 - s5 * s11 * s14 - s9 * s6 * s15 + s9 * s7 * s14 + s13 * s6 * s11 - s13 * s7 * s10;
  inv[1]  = -s1 * s10 * s15 + s1 * s11 * s14 + s9 * s2 * s15 - s9 * s3 * s14 - s13 * s2 * s11 + s13 * s3 * s10;
  inv[2]  =  s1 * s6  * s15 - s1 * s7  * s14 - s5 * s2 * s15 + s5 * s3 * s14 + s13 * s2 * s7  - s13 * s3 * s6;
  inv[3]  = -s1 * s6  * s11 + s1 * s7  * s10 + s5 * s2 * s11 - s5 * s3 * s10 - s9  * s2 * s7  + s9  * s3 * s6;
  inv[4]  = -s4 * s10 * s15 + s4 * s11 * s14 + s8 * s6 * s15 - s8 * s7 * s14 - s12 * s6 * s11 + s12 * s7 * s10;
  inv[5]  =  s0 * s10 * s15 - s0 * s11 * s14 - s8 * s2 * s15 + s8 * s3 * s14 + s12 * s2 * s11 - s12 * s3 * s10;
  inv[6]  = -s0 * s6  * s15 + s0 * s7  * s14 + s4 * s2 * s15 - s4 * s3 * s14 - s12 * s2 * s7  + s12 * s3 * s6;
  inv[7]  =  s0 * s6  * s11 - s0 * s7  * s10 - s4 * s2 * s11 + s4 * s3 * s10 + s8  * s2 * s7  - s8  * s3 * s6;
  inv[8]  =  s4 * s9  * s15 - s4 * s11 * s13 - s8 * s5 * s15 + s8 * s7 * s13 + s12 * s5 * s11 - s12 * s7 * s9;
  inv[9]  = -s0 * s9  * s15 + s0 * s11 * s13 + s8 * s1 * s15 - s8 * s3 * s13 - s12 * s1 * s11 + s12 * s3 * s9;
  inv[10] =  s0 * s5  * s15 - s0 * s7  * s13 - s4 * s1 * s15 + s4 * s3 * s13 + s12 * s1 * s7  - s12 * s3 * s5;
  inv[11] = -s0 * s5  * s11 + s0 * s7  * s9  + s4 * s1 * s11 - s4 * s3 * s9  - s8  * s1 * s7  + s8  * s3 * s5;
  inv[12] = -s4 * s9  * s14 + s4 * s10 * s13 + s8 * s5 * s14 - s8 * s6 * s13 - s12 * s5 * s10 + s12 * s6 * s9;
  inv[13] =  s0 * s9  * s14 - s0 * s10 * s13 - s8 * s1 * s14 + s8 * s2 * s13 + s12 * s1 * s10 - s12 * s2 * s9;
  inv[14] = -s0 * s5  * s14 + s0 * s6  * s13 + s4 * s1 * s14 - s4 * s2 * s13 - s12 * s1 * s6  + s12 * s2 * s5;
  inv[15] =  s0 * s5  * s10 - s0 * s6  * s9  - s4 * s1 * s10 + s4 * s2 * s9  + s8  * s1 * s6  - s8  * s2 * s5;

  double det = s0 * inv[0] + s1 * inv[4] + s2 * inv[8] + s3 * inv[12];
  if (det != 0.0) {
    det = 1.0 / det;
  }
  for (int i = 0; i < 16; i++) {
    dst[i] = inv[i] * det;
  }
}

} // namespace fun
