#include "fun/base/math/sh_math.h"

namespace fun {

//
// Spherical harmonic globals.
//

float NormalizationConstants[9];
int32 BasisL[9];
int32 BasisM[9];
template <> FUN_BASE_API const float TSHVector<2>::ConstantBasisIntegral = 2.0f * Math::Sqrt(PI);
template <> FUN_BASE_API const float TSHVector<3>::ConstantBasisIntegral = 2.0f * Math::Sqrt(PI);

/** Computes a factorial. */
static int32 Factorial(int32 A)
{
  if (A == 0) {
    return 1;
  }
  else {
    return A * Factorial(A - 1);
  }
}

/** Initializes the tables used to calculate SH values. */
static int32 InitSHTables()
{
  int32 L = 0;
  int32 m = 0;

  for (int32 BasisIndex = 0; BasisIndex < 9; BasisIndex++) {
    BasisL[BasisIndex] = L;
    BasisM[BasisIndex] = m;

    NormalizationConstants[BasisIndex] = Math::Sqrt(
      (float(2 * L + 1) / float(4 * PI)) *
      (float(Factorial(L - Math::Abs(m))) / float(Factorial(L + Math::Abs(m))))
      );

    if (m != 0) {
      NormalizationConstants[BasisIndex] *= Math::Sqrt(2.f);
    }

    m++;
    if (m > L) {
      L++;
      m = -L;
    }
  }

  return 0;
}

static int32 InitDummy = InitSHTables();

/** So that e.g. LP(1,1,1) which evaluates to -sqrt(1-1^2) is 0.*/
inline float SafeSqrt(float F)
{
  return Math::Abs(F) > KINDA_SMALL_NUMBER ? Math::Sqrt(F) : 0.f;
}

/** Evaluates the LegendrePolynomial for L,m at x */
float LegendrePolynomial(int32 L,int32 m,float x)
{
  switch (L) {
  case 0:
    return 1;
  case 1:
    if (m == 0)
      return x;
    else if (m == 1)
      return -SafeSqrt(1 - x * x);
    break;
  case 2:
    if (m == 0)
      return -0.5f + (3 * x * x) / 2;
    else if (m == 1)
      return -3 * x * SafeSqrt(1 - x * x);
    else if (m == 2)
      return -3 * (-1 + x * x);
    break;
  case 3:
    if (m == 0)
      return -(3 * x) / 2 + (5 * x * x * x) / 2;
    else if (m == 1)
      return -3 * SafeSqrt(1 - x * x) / 2 * (-1 + 5 * x * x);
    else if (m == 2)
      return -15 * (-x + x * x * x);
    else if (m == 3)
      return -15 * Math::Pow(1 - x * x,1.5f);
    break;
  case 4:
    if (m == 0)
      return 0.125f * (3.0f - 30.0f * x * x + 35.0f * x * x * x * x);
    else if (m == 1)
      return -2.5f * x * SafeSqrt(1.0f - x * x) * (7.0f * x * x - 3.0f);
    else if (m == 2)
      return -7.5f * (1.0f - 8.0f * x * x + 7.0f * x * x * x * x);
    else if (m == 3)
      return -105.0f * x * Math::Pow(1 - x * x,1.5f);
    else if (m == 4)
      return 105.0f * Math::Square(x * x - 1.0f);
    break;
  case 5:
    if (m == 0)
      return 0.125f * x * (15.0f - 70.0f * x * x + 63.0f * x * x * x * x);
    else if (m == 1)
      return -1.875f * SafeSqrt(1.0f - x * x) * (1.0f - 14.0f * x * x + 21.0f * x * x * x * x);
    else if (m == 2)
      return -52.5f * (x - 4.0f * x * x * x + 3.0f * x * x * x * x * x);
    else if (m == 3)
      return -52.5f * Math::Pow(1.0f - x * x,1.5f) * (9.0f * x * x - 1.0f);
    else if (m == 4)
      return 945.0f * x * Math::Square(x * x - 1);
    else if (m == 5)
      return -945.0f * Math::Pow(1.0f - x * x,2.5f);
    break;
  };

  return 0.0f;
}

} // namespace fun
