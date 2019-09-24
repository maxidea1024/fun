#pragma once

#include "TransformCalculus.h"

namespace fun {

//////////////////////////////////////////////////////////////////////////
// transform calculus for 2D types. FUN already has a 2D vector class that we
// will adapt to be interpreted as a translate transform. The rest we create
// new classes for.
//
// The following types are supported
// * float           -> represents a uniform scale.
// * CScale2D        -> represents a 2D non-uniform scale.
// * Vector2       -> represents a 2D translation.
// * CShear2D        -> represents a "2D shear", interpreted as a shear parallel to the x axis followed by a shear parallel to the y axis.
// * CQuat2D         -> represents a pure 2D rotation.
// * CMatrix2x2      -> represents a general 2D transform.
//
//////////////////////////////////////////////////////////////////////////

class CMatrix2x2;

//////////////////////////////////////////////////////////////////////////
// Adapters for Vector2.
//
// Since it is an existing FUN types, we cannot rely on the default
// template that calls member functions. Instead, we provide direct overloads.
//////////////////////////////////////////////////////////////////////////

/** Specialization for concatenating two 2D Translations. */
inline Vector2 Concatenate(const Vector2& lhs, const Vector2& rhs)
{
  return lhs + rhs;
}

/** Specialization for inverting a 2D translation. */
inline Vector2 Inverse(const Vector2& transform)
{
  return -transform;
}

/** Specialization for Vector2 translation. */
inline Vector2 TransformPoint(const Vector2& transform, const Vector2& point)
{
  return transform + point;
}

/** Specialization for Vector2 translation (does nothing). */
inline const Vector2& TransformVector(const Vector2& transform, const Vector2& vector)
{
  return vector;
}

//////////////////////////////////////////////////////////////////////////
// Adapters for 2D uniform scale.
//
// Since it is a fundamental type, we cannot rely on the default
// template that calls member functions. Instead, we provide direct overloads.
//////////////////////////////////////////////////////////////////////////

/**
 * Specialization for uniform Scale.
 */
inline Vector2 TransformPoint(float transform, const Vector2& point)
{
  return transform * point;
}

/**
 * Specialization for uniform Scale.
 */
inline Vector2 TransformVector(float transform, const Vector2& vector)
{
  return transform * vector;
}


/** Represents a 2D non-uniform scale (to disambiguate from an Vector2, which is used for translation) */
class CScale2D
{
public:
  /** Ctor. initialize to an identity scale, 1.0. */
  CScale2D() :Scale(1.0f, 1.0f) {}

  /** Ctor. initialize from a uniform scale. */
  explicit CScale2D(float InScale) : Scale(InScale, InScale) {}

  /** Ctor. initialize from a non-uniform scale. */
  explicit CScale2D(float InScaleX, float InScaleY) : Scale(InScaleX, InScaleY) {}

  /** Ctor. initialize from an Vector defining the 3D scale. */
  explicit CScale2D(const Vector2& InScale) : Scale(InScale) {}

  /** transform 2D point */
  Vector2 TransformPoint(const Vector2& point) const
  {
    return Scale * point;
  }

  /** transform 2D vector*/
  Vector2 TransformVector(const Vector2& vector) const
  {
    return TransformPoint(vector);
  }

  /** Concatenate two scales. */
  CScale2D Concatenate(const CScale2D& rhs) const
  {
    return CScale2D(Scale * rhs.Scale);
  }

  /** Invert the scale. */
  CScale2D Inverse() const
  {
    return CScale2D(Vector2(1.0f / Scale.x, 1.0f / Scale.y));
  }

  /** Equality. */
  bool operator==(const CScale2D& other) const
  {
    return Scale == other.Scale;
  }

  /** Inequality. */
  bool operator!=(const CScale2D& other) const
  {
    return !operator==(other);
  }

  /** Access to the underlying Vector2 that stores the scale. */
  const Vector2& GetVector() const { return Scale; }

private:
  /** Underlying storage of the 2D scale. */
  Vector2 Scale;
};

/** concatenation rules for 2D scales. */
template <> struct ConcatenateRules<float, CScale2D> { typedef CScale2D ResultType; };
/** concatenation rules for 2D scales. */
template <> struct ConcatenateRules<CScale2D, float> { typedef CScale2D ResultType; };


/**
 * Represents a 2D shear:
 *   [1 YY]
 *   [XX 1]
 * XX represents a shear parallel to the x axis. YY represents a shear parallel to the y axis.
 */
class CShear2D
{
public:
  /** Ctor. initialize to an identity. */
  CShear2D() : Shear(0, 0) {}

  /** Ctor. initialize from a set of shears parallel to the x and y axis, respectively. */
  explicit CShear2D(float ShearX, float ShearY) : Shear(ShearX, ShearY) {}

  /** Ctor. initialize from a 2D vector representing a set of shears parallel to the x and y axis, respectively. */
  explicit CShear2D(const Vector2& InShear) : Shear(InShear) {}

  /**
  Generates a shear structure based on angles instead of slope.

  \param InShearAngles - The angles of shear.

  \return the sheare structure.
  */
  static CShear2D FromShearAngles(const Vector2& InShearAngles)
  {
    // Compute the m (Shear Slot) = CoTan(90 - SlopeAngle)

    // 0 is a special case because Tan(90) == infinity
    float ShearX = InShearAngles.x == 0 ? 0 : (1.0f / Math::Tan(Math::DegreesToRadians(90 - Math::Clamp(InShearAngles.x, -89.0f, 89.0f))));
    float ShearY = InShearAngles.y == 0 ? 0 : (1.0f / Math::Tan(Math::DegreesToRadians(90 - Math::Clamp(InShearAngles.y, -89.0f, 89.0f))));

    return CShear2D(ShearX, ShearY);
  }

  /**
  transform 2D point
  [x y] * [1 YY] == [x+y*XX y+x*YY]
          [XX 1]
  */
  Vector2 TransformPoint(const Vector2& point) const
  {
    return point + Vector2(point.y, point.x) * Shear;
  }

  /** transform 2D vector*/
  Vector2 TransformVector(const Vector2& vector) const
  {
    return TransformPoint(vector);
  }

  /**
  Concatenate two shears. The result is NOT a shear, but must be represented by a generalized 2x2 transform.
  Defer the implementation until we can declare a 2x2 matrix.
  [1 YYA] * [1 YYB] == [1+YYA*XXB YYB*YYA]
  [XXA 1]   [XXB 1]    [XXA+XXB XXA*XXB+1]
  */
  inline CMatrix2x2 Concatenate(const CShear2D& rhs) const;

  /**
  Invert the shear. The result is NOT a shear, but must be represented by a generalized 2x2 transform.
  Defer the implementation until we can declare a 2x2 matrix.
  [1 YY]^-1  == 1/(1-YY*XX) * [1 -YY]
  [XX 1]                      [-XX 1]
  */
  CMatrix2x2 Inverse() const;


  /** Equality. */
  bool operator==(const CShear2D& other) const
  {
    return Shear == other.Shear;
  }

  /** Inequality. */
  bool operator!=(const CShear2D& other) const
  {
    return !operator==(other);
  }

  /** Access to the underlying Vector2 that stores the scale. */
  const Vector2& GetVector() const { return Shear; }

private:
  /** Underlying storage of the 2D shear. */
  Vector2 Shear;
};


/**
 * Represents a 2D rotation as a complex number (analagous to quaternions).
 *   rot(theta) == cos(theta) + i * sin(theta)
 *   General transformation follows complex number algebra from there.
 * Does not use "spinor" notation using theta/2 as we don't need that decomposition for our purposes.
 * This makes the implementation for straightforward and efficient for 2D.
 */
class CQuat2D
{
public:
  /** Ctor. initialize to an identity rotation. */
  CQuat2D() : rot(1.0f, 0.0f) {}

  /** Ctor. initialize from a rotation in radians. */
  explicit CQuat2D(float RotRadians) : rot(Math::Cos(RotRadians), Math::Sin(RotRadians)) {}

  /** Ctor. initialize from an Vector2, representing a complex number. */
  explicit CQuat2D(const Vector2& InRot) : rot(InRot) {}

  /**
  transform a 2D point by the 2D complex number representing the rotation:
  In imaginary land: (x + yi) * (u + vi) == (xu - yv) + (xv + yu)i

  Looking at this as a matrix, x == cos(A), y == sin(A)

  [x y] * [ cosA  sinA] == [x y] * [ u v] == [xu-yv xv+yu]
          [-sinA  cosA]            [-v u]

  Looking at the above results, we see the equivalence with matrix multiplication.
  */
  Vector2 TransformPoint(const Vector2& point) const
  {
    return Vector2(
      point.x * rot.x - point.y * rot.y,
      point.x * rot.y + point.y * rot.x);
  }

  /**
  vector rotation is equivalent to rotating a point.
  */
  Vector2 TransformVector(const Vector2& vector) const
  {
    return TransformPoint(vector);
  }

  /**
  transform 2 rotations defined by complex numbers:
  In imaginary land: (A + Bi) * (C + Di) == (AC - BD) + (AD + BC)i

  Looking at this as a matrix, A == cos(theta), B == sin(theta), C == cos(sigma), D == sin(sigma):

  [ A B] * [ C D] == [  AC-BD  AD+BC]
  [-B A]   [-D C]    [-(AD+BC) AC-BD]

  If you look at how the vector multiply works out: [x(AC-BD)+y(-BC-AD)  x(AD+BC)+y(-BD+AC)]
  you can see it follows the same form of the imaginary form. Indeed, check out how the matrix nicely works
  out to [ A B] for a visual proof of the results.
         [-B A]
  */
  CQuat2D Concatenate(const CQuat2D& rhs) const
  {
    return CQuat2D(TransformPoint(rhs.rot));
  }

  /**
  Invert the rotation  defined by complex numbers:
  In imaginary land, an inverse is a complex conjugate, which is equivalent to reflecting about the x axis:
  Conj(A + Bi) == A - Bi
  */
  CQuat2D Inverse() const
  {
    return CQuat2D(Vector2(rot.x, -rot.y));
  }

  /** Equality. */
  bool operator==(const CQuat2D& other) const
  {
    return rot == other.rot;
  }

  /** Inequality. */
  bool operator!=(const CQuat2D& other) const
  {
    return !operator==(other);
  }

  /** Access to the underlying Vector2 that stores the complex number. */
  const Vector2& GetVector() const { return rot; }

private:
  /** Underlying storage of the rotation (x = cos(theta), y = sin(theta). */
  Vector2 rot;
};


/**
 * 2x2 generalized matrix. As Matrix, we assume row vectors, row major storage:
 *    [x y] * [m00 m01]
 *            [m10 m11]
 */
class CMatrix2x2
{
public:
  /** Ctor. initialize to an identity. */
  CMatrix2x2()
  {
    m[0][0] = 1; m[0][1] = 0;
    m[1][0] = 0; m[1][1] = 1;
  }

  CMatrix2x2(float m00, float m01, float m10, float m11)
  {
    m[0][0] = m00; m[0][1] = m01;
    m[1][0] = m10; m[1][1] = m11;
  }

  /** Ctor. initialize from a scale. */
  explicit CMatrix2x2(float UniformScale)
  {
    m[0][0] = UniformScale; m[0][1] = 0;
    m[1][0] = 0; m[1][1] = UniformScale;
  }

  /** Ctor. initialize from a scale. */
  explicit CMatrix2x2(const CScale2D& Scale)
  {
    float ScaleX = Scale.GetVector().x;
    float ScaleY = Scale.GetVector().y;
    m[0][0] = ScaleX; m[0][1] = 0;
    m[1][0] = 0; m[1][1] = ScaleY;
  }

  /** Factory function. initialize from a 2D shear. */
  explicit CMatrix2x2(const CShear2D& Shear)
  {
    float XX = Shear.GetVector().x;
    float YY = Shear.GetVector().y;
    m[0][0] = 1; m[0][1] =YY;
    m[1][0] = XX; m[1][1] = 1;
  }

  /** Ctor. initialize from a rotation. */
  explicit CMatrix2x2(const CQuat2D& Rotation)
  {
    float CosAngle = Rotation.GetVector().x;
    float SinAngle = Rotation.GetVector().y;
    m[0][0] = CosAngle; m[0][1] = SinAngle;
    m[1][0] = -SinAngle; m[1][1] = CosAngle;
  }

  /**
  transform a 2D point
     [x y] * [m00 m01]
             [m10 m11]
  */
  Vector2 TransformPoint(const Vector2& point) const
  {
    return Vector2(
      point.x * m[0][0] + point.y * m[1][0],
      point.x * m[0][1] + point.y * m[1][1]);
  }

  /**
  vector transformation is equivalent to point transformation as our matrix is not homogeneous.
  */
  Vector2 TransformVector(const Vector2& vector) const
  {
    return TransformPoint(vector);
  }

  /**
  Concatenate 2 matrices:
  [A B] * [E F] == [AE+BG AF+BH]
  [C D]   [G H]    [CE+DG CF+DH]
  */
  CMatrix2x2 Concatenate(const CMatrix2x2& rhs) const
  {
    float A, B, C, D;
    GetMatrix(A, B, C, D);
    float E, F, G, H;
    rhs.GetMatrix(E, F, G, H);
    return CMatrix2x2(
      A*E + B*G, A*F + B*H,
      C*E + D*G, C*F + D*H);
  }

  /**
  Invert the transform.
  */
  CMatrix2x2 Inverse() const
  {
    float A, B, C, D;
    GetMatrix(A, B, C, D);
    float InvDet = InverseDeterminant();
    return CMatrix2x2(
       D*InvDet, -B*InvDet,
      -C*InvDet, A*InvDet);
  }

  /** Equality. */
  bool operator==(const CMatrix2x2& rhs) const
  {
    float A, B, C, D;
    GetMatrix(A, B, C, D);
    float E, F, G, H;
    rhs.GetMatrix(E, F, G, H);
    return
      Math::IsNearlyEqual(A, E, KINDA_SMALL_NUMBER) &&
      Math::IsNearlyEqual(B, F, KINDA_SMALL_NUMBER) &&
      Math::IsNearlyEqual(C, G, KINDA_SMALL_NUMBER) &&
      Math::IsNearlyEqual(D, H, KINDA_SMALL_NUMBER);
  }

  /** Inequality. */
  bool operator!=(const CMatrix2x2& other) const
  {
    return !operator==(other);
  }

  void GetMatrix(float &A, float &B, float &C, float &D) const
  {
    A = m[0][0]; B = m[0][1];
    C = m[1][0]; D = m[1][1];
  }

  float Determinant() const
  {
    float A, B, C, D;
    GetMatrix(A, B, C, D);
    return (A*D - B*C);
  }

  float InverseDeterminant() const
  {
    float det = Determinant();
    fun_check_dbg(det != 0.0f);
    return 1.0f / det;
  }

  /** Extracts the squared scale from the matrix (avoids sqrt). */
  CScale2D GetScaleSquared() const
  {
    float A, B, C, D;
    GetMatrix(A, B, C, D);
    return CScale2D(A*A + B*B, C*C + D*D);
  }

  /** Gets the squared scale from the matrix (avoids sqrt). */
  CScale2D GetScale() const
  {
    CScale2D ScaleSquared = GetScaleSquared();
    return CScale2D(Math::Sqrt(ScaleSquared.GetVector().x), Math::Sqrt(ScaleSquared.GetVector().y));
  }

  /** Determines if the matrix is identity or not. Uses exact float comparison, so rounding error is not considered. */
  bool IsIdentity() const
  {
    return m[0][0] == 1.0f && m[0][1] == 0.0f
      && m[1][0] == 0.0f && m[1][1] == 1.0f;
  }

private:
  float m[2][2];
};

inline CMatrix2x2 CShear2D::Concatenate(const CShear2D& rhs) const
{
  float XXA = Shear.x;
  float YYA = Shear.y;
  float XXB = rhs.Shear.x;
  float YYB = rhs.Shear.y;
  return CMatrix2x2(
    1+YYA*XXB, YYB*YYA,
    XXA+XXB, XXA*XXB+1);
}

inline CMatrix2x2 CShear2D::Inverse() const
{
  float InvDet = 1.0f / (1.0f - Shear.x*Shear.y);
  return CMatrix2x2(
    InvDet, -Shear.y * InvDet,
    -Shear.x * InvDet, InvDet);
}

/** Partial specialization of ConcatenateRules for 2x2 and any other type via Upcast to 2x2 first. Requires a conversion ctor on CMatrix2x2. Funky template logic so we don't hide the default rule for NULL conversions. */
template <typename TransformType> struct ConcatenateRules<typename TEnableIf<!TAreTypesEqual<CMatrix2x2, TransformType>::Value, CMatrix2x2>::Type, TransformType> { typedef CMatrix2x2 ResultType; };
/** Partial specialization of ConcatenateRules for 2x2 and any other type via Upcast to 2x2 first. Requires a conversion ctor on CMatrix2x2. Funky template logic so we don't hide the default rule for NULL conversions. */
template <typename TransformType> struct ConcatenateRules<TransformType, typename TEnableIf<!TAreTypesEqual<CMatrix2x2, TransformType>::Value, CMatrix2x2>::Type> { typedef CMatrix2x2 ResultType; };

/** concatenation rules for 2x2 transform types. Convert to 2x2 matrix as the fully decomposed math is not that perf critical right now. */
template <> struct ConcatenateRules<CScale2D, CShear2D> { typedef CMatrix2x2 ResultType; };
template <> struct ConcatenateRules<CScale2D, CQuat2D> { typedef CMatrix2x2 ResultType; };
template <> struct ConcatenateRules<CShear2D, CScale2D> { typedef CMatrix2x2 ResultType; };
template <> struct ConcatenateRules<CQuat2D, CScale2D> { typedef CMatrix2x2 ResultType; };
template <> struct ConcatenateRules<CShear2D, CQuat2D> { typedef CMatrix2x2 ResultType; };
template <> struct ConcatenateRules<CQuat2D, CShear2D> { typedef CMatrix2x2 ResultType; };


/**
 * Support for generalized 2D affine transforms.
 * Implemented as a 2x2 transform followed by translation. In matrix form:
 *   [A B 0]
 *   [C D 0]
 *   [x y 1]
 */
class CTransform2D
{
public:
  /** Initialize the transform using an identity matrix and a translation. */
  CTransform2D(const Vector2& translation = Vector2(0.f, 0.f))
    : Trans(translation)
  {}

  /** Initialize the transform using a uniform scale and a translation. */
  explicit CTransform2D(float UniformScale, const Vector2& translation = Vector2(0.f, 0.f))
    : m(CScale2D(UniformScale)), Trans(translation)
  {}

  /** Initialize the transform using a 2D scale and a translation. */
  explicit CTransform2D(const CScale2D& Scale, const Vector2& translation = Vector2(0.f, 0.f))
    : m(Scale), Trans(translation)
  {}

  /** Initialize the transform using a 2D shear and a translation. */
  explicit CTransform2D(const CShear2D& Shear, const Vector2& translation = Vector2(0.f, 0.f))
    : m(Shear), Trans(translation)
  {}

  /** Initialize the transform using a 2D rotation and a translation. */
  explicit CTransform2D(const CQuat2D& rot, const Vector2& translation = Vector2(0.f, 0.f))
    : m(rot), Trans(translation)
  {}

  /** Initialize the transform using a general 2x2 transform and a translation. */
  explicit CTransform2D(const CMatrix2x2& transform, const Vector2& translation = Vector2(0.f, 0.f))
    : m(transform), Trans(translation)
  {}

  /** 2D transformation of a point. */
  Vector2 TransformPoint(const Vector2& point) const
  {
    return ::TransformPoint(Trans, ::TransformPoint(m, point));
  }

  /** 2D transformation of a vector. */
  Vector2 TransformVector(const Vector2& vector) const
  {
    return ::TransformVector(m, vector);
  }

  /**
  Concatenates two transforms. result is equivalent to transforming first by this, followed by rhs.
   Concat(A, B) == (P * MA + TA) * MB + TB
               == (P * MA * MB) + TA*MB + TB
   NewM == MA * MB
   NewT == TA * MB + TB
  */
  CTransform2D Concatenate(const CTransform2D& rhs) const
  {
    return CTransform2D(
      ::Concatenate(m, rhs.m),
      ::Concatenate(::TransformPoint(rhs.m, Trans), rhs.Trans));
  }

  /**
  Inverts a transform. So a transform from space A to space B results in a transform from space B to space A.
  Since this class applies the 2x2 transform followed by translation, our inversion logic needs to be able to recast
  the result as a m * T. It does it using the following identity:
    (m * T)^-1 == T^-1 * m^-1

  In homogeneous form, we represent our affine transform like so:
       m    *    T
    [A B 0]   [1 0 0]   [A B 0]
    [C D 0] * [0 1 0] = [C D 0]. This class simply decomposes the 2x2 transform and translation.
    [0 0 1]   [x y 1]   [x y 1]

  But if we were applying the transforms in reverse order (as we need to for the inverse identity above):
     T^-1   *  m^-1
    [1 0 0]   [A B 0]   [A  B  0]  where [x' y'] = [x y] * [A B]
    [0 1 0] * [C D 0] = [C  D  0]                          [C D]
    [x y 1]   [0 0 1]   [x' y' 1]

  This can be conceptualized by seeing that a translation effectively defines a new local origin for that
  frame of reference. Since there is a 2x2 transform AFTER that, the concatenated frame of reference has an origin
  that is the old origin transformed by the 2x2 transform.

  In the last equation:
  We know that [x y] is the translation induced by inverting T, or -Translate.
  We know that [[A B][C D]] == Inverse(m), so we can represent T^-1 * m^-1 as m'* T' where:
    m' == Inverse(m)
    T' == Inverse(Translate) * Inverse(m)
  */
  CTransform2D Inverse() const
  {
    CMatrix2x2 InvM = ::Inverse(m);
    Vector2 InvTrans = ::TransformPoint(InvM, ::Inverse(Trans));
    return CTransform2D(InvM, InvTrans);
  }

  /** Equality. */
  bool operator==(const CTransform2D& other) const
  {
    return m == other.m && Trans == other.Trans;
  }

  /** Inequality. */
  bool operator!=(const CTransform2D& other) const
  {
    return !operator==(other);
  }

  /** Access to the 2x2 transform */
  const CMatrix2x2& GetMatrix() const { return m; }
  /** Access to the translation */
  const Vector2& GetTranslation() const { return Trans; }

  /**
  Specialized function to determine if a transform is precisely the identity transform. Uses exact float comparison, so rounding error is not considered.
  */
  bool IsIdentity() const
  {
    return m.IsIdentity() && Trans == Vector2::ZeroVector;
  }

private:

  CMatrix2x2 m;
  Vector2 Trans;
};

template <> struct IsPOD<CTransform2D> { enum { Value = true }; };


//////////////////////////////////////////////////////////////////////////
// Concatenate overloads.
//
// Efficient overloads for concatenating 2D affine transforms.
// Better than simply upcasting both to CTransform2D first.
//////////////////////////////////////////////////////////////////////////

/** Specialization for concatenating a 2D scale and 2D translation. */
inline CTransform2D Concatenate(const CScale2D& Scale, const Vector2& translation)
{
  return CTransform2D(Scale, translation);
}

/** Specialization for concatenating a 2D shear and 2D translation. */
inline CTransform2D Concatenate(const CShear2D& Shear, const Vector2& translation)
{
  return CTransform2D(Shear, translation);
}

/** Specialization for concatenating 2D Rotation and 2D translation. */
inline CTransform2D Concatenate(const CQuat2D& rot, const Vector2& translation)
{
  return CTransform2D(rot, translation);
}

/** Specialization for concatenating 2D generalized transform and 2D translation. */
inline CTransform2D Concatenate(const CMatrix2x2& transform, const Vector2& translation)
{
  return CTransform2D(transform, translation);
}

/** Specialization for concatenating transform and 2D translation. */
inline CTransform2D Concatenate(const CTransform2D& transform, const Vector2& translation)
{
  return CTransform2D(transform.GetMatrix(), Concatenate(transform.GetTranslation(), translation));
}

/** Specialization for concatenating a 2D translation and 2D scale. */
inline CTransform2D Concatenate(const Vector2& translation, const CScale2D& Scale)
{
  return CTransform2D(Scale, ::TransformPoint(Scale, translation));
}

/** Specialization for concatenating a 2D translation and 2D shear. */
inline CTransform2D Concatenate(const Vector2& translation, const CShear2D& Shear)
{
  return CTransform2D(Shear, ::TransformPoint(Shear, translation));
}

/** Specialization for concatenating 2D translation and 2D Rotation. */
inline CTransform2D Concatenate(const Vector2& translation, const CQuat2D& rot)
{
  return CTransform2D(rot, ::TransformPoint(rot, translation));
}

/** Specialization for concatenating 2D translation and 2D generalized transform. See docs for CTransform2D::Inverse for details on how this math is derived. */
inline CTransform2D Concatenate(const Vector2& translation, const CMatrix2x2& transform)
{
  return CTransform2D(transform, ::TransformPoint(transform, translation));
}

/** Specialization for concatenating 2D translation and transform. See docs for CTransform2D::Inverse for details on how this math is derived. */
inline CTransform2D Concatenate(const Vector2& translation, const CTransform2D& transform)
{
  return CTransform2D(transform.GetMatrix(), Concatenate(::TransformPoint(transform.GetMatrix(), translation), transform.GetTranslation()));
}

/** Partial specialization of ConcatenateRules for CTransform2D and any other type via Upcast to CTransform2D first. Requires a conversion ctor on CTransform2D. Funky template logic so we don't hide the default rule for NULL conversions. */
template <typename TransformType> struct ConcatenateRules<typename TEnableIf<!TAreTypesEqual<CTransform2D, TransformType>::Value, CTransform2D>::Type, TransformType> { typedef CTransform2D ResultType; };
/** Partial specialization of ConcatenateRules for CTransform2D and any other type via Upcast to CTransform2D first. Requires a conversion ctor on CTransform2D. Funky template logic so we don't hide the default rule for NULL conversions. */
template <typename TransformType> struct ConcatenateRules<TransformType  , typename TEnableIf<!TAreTypesEqual<CTransform2D, TransformType>::Value, CTransform2D>::Type> { typedef CTransform2D ResultType; };

/** Provide a disambiguating overload for 2x2 and CTransform2D, since both try to provide a generic set of ConcatenateRules for all types. */
template <> struct ConcatenateRules<CMatrix2x2, CTransform2D> { typedef CTransform2D ResultType; };
template <> struct ConcatenateRules<CTransform2D, CMatrix2x2> { typedef CTransform2D ResultType; };

} // namespace fun
