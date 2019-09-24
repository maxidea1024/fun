#pragma once

#include "fun/base/base.h"

namespace fun {

//  Constants.
extern FUN_BASE_API float NormalizationConstants[9];
extern FUN_BASE_API int32 BasisL[9];
extern FUN_BASE_API int32 BasisM[9];

extern FUN_BASE_API float LegendrePolynomial(int32 L, int32 m, float x);

/** Returns the basis index of the SH basis L, m. */
inline int32 SHGetBasisIndex(int32 L, int32 m)
{
  return L * (L + 1) + m;
}


/** a vector of spherical harmonic coefficients. */
template <int32 Order>
class alignas(16) TSHVector
{
 public:
  enum { MaxSHOrder = Order };
  enum { MaxSHBasis = MaxSHOrder * MaxSHOrder };
  enum { NumComponentsPerSIMDVector = 4 };
  enum { NumSIMDVectors = (MaxSHBasis + NumComponentsPerSIMDVector - 1) / NumComponentsPerSIMDVector };

  float v[NumSIMDVectors * NumComponentsPerSIMDVector];

  /** The integral of the constant SH basis. */
  FUN_BASE_API static const float ConstantBasisIntegral;

  /** Default constructor. */
  TSHVector()
  {
    UnsafeMemory::Memzero(v, sizeof(v));
  }

  TSHVector(float V0, float V1, float V2, float V3)
  {
    UnsafeMemory::Memzero(v, sizeof(v));

    v[0] = V0;
    v[1] = V1;
    v[2] = V2;
    v[3] = V3;
  }

  explicit TSHVector(const Vector4& vector)
  {
    UnsafeMemory::Memzero(v, sizeof(v));

    v[0] = vector.x;
    v[1] = vector.y;
    v[2] = vector.z;
    v[3] = vector.w;
  }

  /** scalar multiplication operator. */
  /** Changed to float& from float to avoid lhs **/
  inline friend TSHVector operator*(const TSHVector& a, const float& b)
  {
    const VectorRegister ReplicatedScalar = VectorLoadFloat1(&b);

    TSHVector result;
    for (int32 BasisIndex = 0; BasisIndex < NumSIMDVectors; BasisIndex++) {
      VectorRegister MulResult = VectorMultiply(
        VectorLoadAligned(&a.v[BasisIndex * NumComponentsPerSIMDVector]),
        ReplicatedScalar
        );
      VectorStoreAligned(MulResult, &result.v[BasisIndex * NumComponentsPerSIMDVector]);
    }
    return result;
  }

  /** scalar division operator. */
  inline friend TSHVector operator/(const TSHVector& a, const float& scalar)
  {
    const float b = (1.0f / scalar);
    const VectorRegister ReplicatedScalar = VectorLoadFloat1(&b);

    TSHVector result;
    for (int32 BasisIndex = 0; BasisIndex < NumSIMDVectors; BasisIndex++) {
      VectorRegister MulResult = VectorMultiply(
        VectorLoadAligned(&a.v[BasisIndex * NumComponentsPerSIMDVector]),
        ReplicatedScalar
        );
      VectorStoreAligned(MulResult, &result.v[BasisIndex * NumComponentsPerSIMDVector]);
    }
    return result;
  }

  /** Addition operator. */
  inline friend TSHVector operator+(const TSHVector& a, const TSHVector& b)
  {
    TSHVector result;
    for (int32 BasisIndex = 0; BasisIndex < NumSIMDVectors; BasisIndex++) {
      VectorRegister AddResult = VectorAdd(
        VectorLoadAligned(&a.v[BasisIndex * NumComponentsPerSIMDVector]),
        VectorLoadAligned(&b.v[BasisIndex * NumComponentsPerSIMDVector])
        );

      VectorStoreAligned(AddResult, &result.v[BasisIndex * NumComponentsPerSIMDVector]);
    }
    return result;
  }

  /** Subtraction operator. */
  inline friend TSHVector operator-(const TSHVector& a, const TSHVector& b)
  {
    TSHVector result;
    for (int32 BasisIndex = 0; BasisIndex < NumSIMDVectors; BasisIndex++) {
      VectorRegister SubResult = VectorSubtract(
        VectorLoadAligned(&a.v[BasisIndex * NumComponentsPerSIMDVector]),
        VectorLoadAligned(&b.v[BasisIndex * NumComponentsPerSIMDVector])
        );

      VectorStoreAligned(SubResult, &result.v[BasisIndex * NumComponentsPerSIMDVector]);
    }
    return result;
  }

  /** Dot product operator. */
  inline friend float Dot(const TSHVector& a, const TSHVector& b)
  {
    VectorRegister ReplicatedResult = VectorZero();
    for (int32 BasisIndex = 0; BasisIndex < NumSIMDVectors; BasisIndex++) {
      ReplicatedResult = VectorAdd(
        ReplicatedResult,
        VectorDot4(
          VectorLoadAligned(&a.v[BasisIndex * NumComponentsPerSIMDVector]),
          VectorLoadAligned(&b.v[BasisIndex * NumComponentsPerSIMDVector])
          )
        );
    }
    float result;
    VectorStoreFloat1(ReplicatedResult, &result);
    return result;
  }

  /** In-place addition operator. */
  /** Changed from (*this = *this + b;} to calculate here to avoid lhs **/
  /** Now this avoids TSHVector + operator thus lhs on *this as well as result and more **/
  inline TSHVector& operator+=(const TSHVector& b)
  {
    for (int32 BasisIndex = 0; BasisIndex < NumSIMDVectors; BasisIndex++) {
      VectorRegister AddResult = VectorAdd(
        VectorLoadAligned(&v[BasisIndex * NumComponentsPerSIMDVector]),
        VectorLoadAligned(&b.v[BasisIndex * NumComponentsPerSIMDVector])
        );

      VectorStoreAligned(AddResult, &v[BasisIndex * NumComponentsPerSIMDVector]);
    }
    return *this;
  }

  /** In-place subtraction operator. */
  /** Changed from (*this = *this - b;} to calculate here to avoid lhs **/
  /** Now this avoids TSHVector - operator thus lhs on *this as well as result and **/
  inline TSHVector& operator-=(const TSHVector& b)
  {
    for (int32 BasisIndex = 0; BasisIndex < NumSIMDVectors; BasisIndex++) {
      VectorRegister SubResult = VectorSubtract(
        VectorLoadAligned(&v[BasisIndex * NumComponentsPerSIMDVector]),
        VectorLoadAligned(&b.v[BasisIndex * NumComponentsPerSIMDVector])
        );

      VectorStoreAligned(SubResult, &v[BasisIndex * NumComponentsPerSIMDVector]);
    }
    return *this;
  }

  /** In-place scalar division operator. */
  /** Changed to float& from float to avoid lhs **/
  /** Changed from (*this = *this * (1.0f/b);) to calculate here to avoid lhs **/
  /** Now this avoids TSHVector * operator thus lhs on *this as well as result and lhs **/
  inline TSHVector& operator/=(const float& scalar)
  {
    const float b = (1.0f/scalar);
    const VectorRegister ReplicatedScalar = VectorLoadFloat1(&b);

    for (int32 BasisIndex = 0; BasisIndex < NumSIMDVectors; BasisIndex++) {
      VectorRegister MulResult = VectorMultiply(
        VectorLoadAligned(&v[BasisIndex * NumComponentsPerSIMDVector]),
        ReplicatedScalar
        );
      VectorStoreAligned(MulResult, &v[BasisIndex * NumComponentsPerSIMDVector]);
    }
    return *this;
  }

  /** In-place scalar multiplication operator. */
  /** Changed to float& from float to avoid lhs **/
  /** Changed from (*this = *this * b;) to calculate here to avoid lhs **/
  /** Now this avoids TSHVector * operator thus lhs on *this as well as result and lhs **/
  inline TSHVector& operator*=(const float& b)
  {
    const VectorRegister ReplicatedScalar = VectorLoadFloat1(&b);

    for (int32 BasisIndex = 0;BasisIndex < NumSIMDVectors;BasisIndex++) {
      VectorRegister MulResult = VectorMultiply(
        VectorLoadAligned(&v[BasisIndex * NumComponentsPerSIMDVector]),
        ReplicatedScalar
        );
      VectorStoreAligned(MulResult, &v[BasisIndex * NumComponentsPerSIMDVector]);
    }
    return *this;
  }

  friend Archive& operator & (Archive& ar, TSHVector& SH)
  {
    for (int32 BasisIndex = 0; BasisIndex < MaxSHBasis; BasisIndex++) {
      ar & SH.v[BasisIndex];
    }

    return ar;
  }

  /** Calculates the integral of the function over the surface of the sphere. */
  float CalcIntegral() const
  {
    return v[0] * ConstantBasisIntegral;
  }

  /** Scales the function uniformly so its integral equals one. */
  void Normalize()
  {
    const float Integral = CalcIntegral();
    if (Integral > DELTA) {
      *this /= Integral;
    }
  }

  bool AreFloatsValid() const
  {
    bool bValid = true;

    for (int32 BasisIndex = 0; BasisIndex < MaxSHBasis; BasisIndex++) {
      bValid = bValid && Math::IsFinite(v[BasisIndex]) && !Math::IsNaN(v[BasisIndex]);
    }

    return bValid;
  }

  /** Compute the direction which the spherical harmonic is highest at. */
  Vector GetMaximumDirection() const
  {
    // This is an approximation which only takes into account first and second order spherical harmonics.
    return Vector(-v[3], -v[1], v[2]).GetSafeNormal();
  }

  static TSHVector CalcDiffuseTransfer(const Vector& Normal)
  {
    TSHVector result = SHBasisFunction(Normal);

    // These formula are scaling factors for each SH band that convolve a SH with the circularly symmetric function
    // max(0, cos(theta))
    float L0 = PI;
    float L1 = 2 * PI / 3;
    float L2 = PI / 4;

    // Multiply the coefficients in each band with the appropriate band scaling factor.
    for (int32 BasisIndex = 0;BasisIndex < MaxSHBasis;BasisIndex++) {
      float Scale = L2;

      if (BasisIndex < 1) {
        Scale = L0;
      }
      else if (BasisIndex < 4) {
        Scale = L1;
      }

      result.v[BasisIndex] *= Scale;
    }

    return result;
  }

  /** Returns the value of the SH basis L, m at the point on the sphere defined by the unit vector vector. */
  static TSHVector SHBasisFunction(const Vector& vector)
  {
    TSHVector result;

    // Initialize the result to the normalization constant.
    for (int32 BasisIndex = 0; BasisIndex < TSHVector::MaxSHBasis; BasisIndex++) {
      result.v[BasisIndex] = NormalizationConstants[BasisIndex];
    }

    // Multiply the result by the phi-dependent part of the SH bases.
    // Skip this for x=0 and y=0, because atan will be undefined and
    // we know the vector will be (0, 0, +1) or (0, 0, -1).
    if (Math::Abs(vector.x) > KINDA_SMALL_NUMBER || Math::Abs(vector.y) > KINDA_SMALL_NUMBER) {
      const float Phi = Math::Atan2(vector.y, vector.x);

      for (int32 BandIndex = 1; BandIndex < TSHVector::MaxSHOrder; BandIndex++) {
        const float SinPhiM = Math::Sin(BandIndex * Phi);
        const float CosPhiM = Math::Cos(BandIndex * Phi);

        for (int32 RecurrentBandIndex = BandIndex; RecurrentBandIndex < TSHVector::MaxSHOrder; RecurrentBandIndex++) {
          result.v[SHGetBasisIndex(RecurrentBandIndex, -BandIndex)] *= SinPhiM;
          result.v[SHGetBasisIndex(RecurrentBandIndex, +BandIndex)] *= CosPhiM;
        }
      }
    }

    // Multiply the result by the theta-dependent part of the SH bases.
    for (int32 BasisIndex = 1; BasisIndex < TSHVector::MaxSHBasis; BasisIndex++) {
      result.v[BasisIndex] *= LegendrePolynomial(BasisL[BasisIndex], Math::Abs(BasisM[BasisIndex]), vector.z);
    }

    return result;
  }

  /** The ambient incident lighting function. */
  static TSHVector AmbientFunction()
  {
    TSHVector AmbientFunctionSH;
    AmbientFunctionSH.v[0] = 1.0f / (2.0f * Math::Sqrt(PI));
    return AmbientFunctionSH;
  }
};


/** Specialization for 2nd order to avoid expensive trig functions. */
template <>
inline TSHVector<2> TSHVector<2>::SHBasisFunction(const Vector& vector)
{
  TSHVector<2> result;
  result.v[0] = 0.282095f;
  result.v[1] = -0.488603f * vector.y;
  result.v[2] = 0.488603f * vector.z;
  result.v[3] = -0.488603f * vector.x;
  return result;
}

/** Specialization for 3rd order to avoid expensive trig functions. */
template <>
inline TSHVector<3> TSHVector<3>::SHBasisFunction(const Vector& vector)
{
  TSHVector<3> result;
  result.v[0] = 0.282095f;
  result.v[1] = -0.488603f * vector.y;
  result.v[2] = 0.488603f * vector.z;
  result.v[3] = -0.488603f * vector.x;

  Vector VectorSquared = vector * vector;
  result.v[4] = 1.092548f * vector.x * vector.y;
  result.v[5] = -1.092548f * vector.y * vector.z;
  result.v[6] = 0.315392f * (3.0f * VectorSquared.z - 1.0f);
  result.v[7] = -1.092548f * vector.x * vector.z;
  result.v[8] = 0.546274f * (VectorSquared.x - VectorSquared.y);
  return result;
}


/** a vector of colored spherical harmonic coefficients. */
template <int32 MaxSHOrder>
class TSHVectorRGB
{
 public:
  TSHVector<MaxSHOrder> r;
  TSHVector<MaxSHOrder> G;
  TSHVector<MaxSHOrder> b;

  TSHVectorRGB() {}

  /** Calculates greyscale spherical harmonic coefficients. */
  TSHVector<MaxSHOrder> GetLuminance() const
  {
    return r * 0.3f + G * 0.59f + b * 0.11f;
  }

  void Desaturate(float DesaturateFraction)
  {
    TSHVector<MaxSHOrder> Desaturated = GetLuminance() * DesaturateFraction;

    r = r * (1 - DesaturateFraction) + Desaturated;
    G = G * (1 - DesaturateFraction) + Desaturated;
    b = b * (1 - DesaturateFraction) + Desaturated;
  }

  /** Calculates the integral of the function over the surface of the sphere. */
  LinearColor CalcIntegral() const
  {
    LinearColor result;
    result.r = r.CalcIntegral();
    result.G = G.CalcIntegral();
    result.b = b.CalcIntegral();
    result.a = 1.0f;
    return result;
  }

  bool AreFloatsValid() const
  {
    return r.AreFloatsValid() && G.AreFloatsValid() && b.AreFloatsValid();
  }

  /** scalar multiplication operator. */
  /** Changed to float& from float to avoid lhs **/
  inline friend TSHVectorRGB operator*(const TSHVectorRGB& a, const float& scalar)
  {
    TSHVectorRGB result;
    result.r = a.r * scalar;
    result.G = a.G * scalar;
    result.b = a.b * scalar;
    return result;
  }

  /** scalar multiplication operator. */
  /** Changed to float& from float to avoid lhs **/
  inline friend TSHVectorRGB operator*(const float& scalar, const TSHVectorRGB& a)
  {
    TSHVectorRGB result;
    result.r = a.r * scalar;
    result.G = a.G * scalar;
    result.b = a.b * scalar;
    return result;
  }

  /** color multiplication operator. */
  inline friend TSHVectorRGB operator*(const TSHVectorRGB& a, const LinearColor& color)
  {
    TSHVectorRGB result;
    result.r = a.r * color.r;
    result.G = a.G * color.G;
    result.b = a.b * color.b;
    return result;
  }

  /** color multiplication operator. */
  inline friend TSHVectorRGB operator*(const LinearColor& color, const TSHVectorRGB& a)
  {
    TSHVectorRGB result;
    result.r = a.r * color.r;
    result.G = a.G * color.G;
    result.b = a.b * color.b;
    return result;
  }

  /** Division operator. */
  inline friend TSHVectorRGB operator/(const TSHVectorRGB& a, const float& InB)
  {
    TSHVectorRGB result;
    result.r = a.r / InB;
    result.G = a.G / InB;
    result.b = a.b / InB;
    return result;
  }

  /** Addition operator. */
  inline friend TSHVectorRGB operator+(const TSHVectorRGB& a, const TSHVectorRGB& InB)
  {
    TSHVectorRGB result;
    result.r = a.r + InB.r;
    result.G = a.G + InB.G;
    result.b = a.b + InB.b;
    return result;
  }

  /** Subtraction operator. */
  inline friend TSHVectorRGB operator-(const TSHVectorRGB& a, const TSHVectorRGB& InB)
  {
    TSHVectorRGB result;
    result.r = a.r - InB.r;
    result.G = a.G - InB.G;
    result.b = a.b - InB.b;
    return result;
  }

  /** Dot product operator. */
  inline friend LinearColor Dot(const TSHVectorRGB& a, const TSHVector<MaxSHOrder>& InB)
  {
    LinearColor result;
    result.r = Dot(a.r, InB);
    result.G = Dot(a.G, InB);
    result.b = Dot(a.b, InB);
    result.a = 1.0f;
    return result;
  }

  /** In-place addition operator. */
  /** Changed from (*this = *this + InB;) to separate all calc to avoid lhs **/

  /** Now it calls directly += operator in TSHVector (avoid TSHVectorRGB + operator) **/
  inline TSHVectorRGB& operator+=(const TSHVectorRGB& InB)
  {
    r += InB.r;
    G += InB.G;
    b += InB.b;

    return *this;
  }

  /** In-place subtraction operator. */
  /** Changed from (*this = *this - InB;) to separate all calc to avoid lhs **/
  /** Now it calls directly -= operator in TSHVector (avoid TSHVectorRGB - operator) **/
  inline TSHVectorRGB& operator-=(const TSHVectorRGB& InB)
  {
    r -= InB.r;
    G -= InB.G;
    b -= InB.b;

    return *this;
  }

  /** In-place scalar multiplication operator. */
  /** Changed from (*this = *this * InB;) to separate all calc to avoid lhs **/
  /** Now it calls directly *= operator in TSHVector (avoid TSHVectorRGB * operator) **/
  inline TSHVectorRGB& operator*=(const float& scalar)
  {
    r *= scalar;
    G *= scalar;
    b *= scalar;

    return *this;
  }

  friend Archive& operator & (Archive& ar, TSHVectorRGB& SH)
  {
    return ar & SH.r & SH.G & SH.b;
  }

  /** adds an impulse to the SH environment. */
  inline void AddIncomingRadiance(const LinearColor& IncomingRadiance, float Weight, const Vector4& WorldSpaceDirection)
  {
    *this += TSHVector<MaxSHOrder>::SHBasisFunction(WorldSpaceDirection) * (IncomingRadiance * Weight);
  }

  /** adds ambient lighting. */
  inline void AddAmbient(const LinearColor& Intensity)
  {
    *this += TSHVector<MaxSHOrder>::AmbientFunction() * Intensity;
  }
};

/** color multiplication operator. */
template <int32 Order>
inline TSHVectorRGB<Order> operator*(const TSHVector<Order>& a, const LinearColor& b)
{
  TSHVectorRGB<Order> result;
  result.r = a * b.r;
  result.G = a * b.G;
  result.b = a * b.b;

  return result;
}

typedef TSHVector<3> CSHVector3;
typedef TSHVector<2> CSHVector2;
typedef TSHVectorRGB<3> CSHVectorRGB3;
typedef TSHVectorRGB<2> CSHVectorRGB2;

} // namespace fun
