#pragma once

#include "TransformCalculus.h"

namespace fun {

//////////////////////////////////////////////////////////////////////////
// transform calculus for 3D types. Since FUN already has existing 3D transform
// types, this is mostly a set of adapter overloads for the primitive operations
// requires by the transform calculus framework.
//
// The following types are adapted.
// * float           -> represents a uniform scale.
// * CScale          -> represents a 3D non-uniform scale.
// * Vector         -> represents a 3D translation.
// * Rotator        -> represents a pure rotation.
// * Quat           -> represents a pure rotation.
// * Matrix         -> represents a general 3D homogeneous transform.
//
//////////////////////////////////////////////////////////////////////////

/**
 * Represents a 3D non-uniform scale (to disambiguate from an Vector, which is used for translation).
 *
 * Serves as a good base example of how to write a class that supports the basic transform calculus
 * operations.
 */
class CScale
{
 public:
  /** Ctor. initialize to an identity scale, 1.0. */
  CScale() : Scale(1.0f) {}

  /** Ctor. initialize from a uniform scale. */
  explicit CScale(float InScale) : Scale(InScale) {}

  /** Ctor. initialize from an Vector defining the 3D scale. */
  explicit CScale(const Vector& InScale) : Scale(InScale) {}

  /** Access to the underlying Vector that stores the scale. */
  const Vector& GetVector() const { return Scale; }

  /** Concatenate two scales. */
  const CScale Concatenate(const CScale& rhs) const
  {
    return CScale(Scale * rhs.GetVector());
  }

  /** Invert the scale. */
  const CScale Inverse() const
  {
    return CScale(Vector(1.0f / Scale.x, 1.0f / Scale.y, 1.0f / Scale.z));
  }

 private:
  /** Underlying storage of the 3D scale. */
  Vector Scale;
};

/** Specialization for converting a Matrix to an Rotator. It uses a non-standard explicit conversion function. */
template <> template <> inline Rotator TransformConverter<Rotator>::Convert<Matrix>(const Matrix& transform)
{
  return transform.rotator();
}

//////////////////////////////////////////////////////////////////////////
// Matrix Support
//////////////////////////////////////////////////////////////////////////

/**
 * Converts a generic transform to a matrix using a ToMatrix() member function.
 * Uses decltype to allow some classes to return const-ref types for efficiency.
 *
 * \param transform
 *
 * \return the Matrix stored by the transform.
 */
template <typename TransformType>
inline auto ToMatrix(const TransformType& transform) -> decltype(transform.ToMatrix())
{
  return transform.ToMatrix();
}

/**
 * Specialization for the NULL matrix conversion.
 *
 * \param Scale - Uniform Scale
 *
 * \return matrix that represents the uniform Scale space.
 */
inline const Matrix& ToMatrix(const Matrix& transform)
{
  return transform;
}

/**
 * Specialization for floats as a uniform scale.
 *
 * \param Scale - Uniform Scale
 *
 * \return matrix that represents the uniform Scale space.
 */
inline Matrix ToMatrix(float Scale)
{
  return ScaleMatrix(Scale);
}

/**
 * Specialization for non-uniform Scale.
 *
 * \param Scale - Non-uniform Scale
 *
 * \return matrix that represents the non-uniform Scale space.
 */
inline Matrix ToMatrix(const CScale& Scale)
{
  return ScaleMatrix(Scale.GetVector());
}

/**
 * Specialization for translation.
 *
 * \param translation - translation
 *
 * \return matrix that represents the translated space.
 */
inline Matrix ToMatrix(const Vector& translation)
{
  return CTranslationMatrix(translation);
}

/**
 * Specialization for rotation.
 *
 * \param Rotation - Rotation
 *
 * \return matrix that represents the rotated space.
 */
inline Matrix ToMatrix(const Rotator& Rotation)
{
  return RotationMatrix(Rotation);
}

/**
 * Specialization for rotation.
 *
 * \param Rotation - Rotation
 *
 * \return matrix that represents the rotated space.
 */
inline Matrix ToMatrix(const Quat& Rotation)
{
  return RotationMatrix::Make(Rotation);
}


/**
 * Specialization of TransformConverter for Matrix. Calls ToMatrix() by default.
 * Allows custom types to easily provide support via a ToMatrix() overload or a ToMatrix() member function.
 * Uses decltype to support efficient passthrough of classes that can convert to a Matrix without creating
 * a new instance.
 */
template <>
struct TransformConverter<Matrix>
{
  template <typename OtherTransformType>
  static auto Convert(const OtherTransformType& transform) -> decltype(ToMatrix(transform))
  {
    return ToMatrix(transform);
  }
};

/** concatenation rules for basic FUN types. */
template <> struct ConcatenateRules<float        , CScale       > { typedef CScale ResultType; };
template <> struct ConcatenateRules<CScale       , float        > { typedef CScale ResultType; };
template <> struct ConcatenateRules<float        , Vector      > { typedef Matrix ResultType; };
template <> struct ConcatenateRules<Vector      , float        > { typedef Matrix ResultType; };
template <> struct ConcatenateRules<float        , Rotator     > { typedef Matrix ResultType; };
template <> struct ConcatenateRules<Rotator     , float        > { typedef Matrix ResultType; };
template <> struct ConcatenateRules<float        , Quat        > { typedef Matrix ResultType; };
template <> struct ConcatenateRules<Quat        , float        > { typedef Matrix ResultType; };
template <> struct ConcatenateRules<float        , Matrix      > { typedef Matrix ResultType; };
template <> struct ConcatenateRules<Matrix      , float        > { typedef Matrix ResultType; };
template <> struct ConcatenateRules<CScale       , Vector      > { typedef Matrix ResultType; };
template <> struct ConcatenateRules<Vector      , CScale       > { typedef Matrix ResultType; };
template <> struct ConcatenateRules<CScale       , Rotator     > { typedef Matrix ResultType; };
template <> struct ConcatenateRules<Rotator     , CScale       > { typedef Matrix ResultType; };
template <> struct ConcatenateRules<CScale       , Quat        > { typedef Matrix ResultType; };
template <> struct ConcatenateRules<Quat        , CScale       > { typedef Matrix ResultType; };
template <> struct ConcatenateRules<CScale       , Matrix      > { typedef Matrix ResultType; };
template <> struct ConcatenateRules<Matrix      , CScale       > { typedef Matrix ResultType; };
template <> struct ConcatenateRules<Vector      , Rotator     > { typedef Matrix ResultType; };
template <> struct ConcatenateRules<Rotator     , Vector      > { typedef Matrix ResultType; };
template <> struct ConcatenateRules<Vector      , Quat        > { typedef Matrix ResultType; };
template <> struct ConcatenateRules<Quat        , Vector      > { typedef Matrix ResultType; };
template <> struct ConcatenateRules<Vector      , Matrix      > { typedef Matrix ResultType; };
template <> struct ConcatenateRules<Matrix      , Vector      > { typedef Matrix ResultType; };
template <> struct ConcatenateRules<Rotator     , Quat        > { typedef Quat ResultType; };
template <> struct ConcatenateRules<Quat        , Rotator     > { typedef Quat ResultType; };
template <> struct ConcatenateRules<Rotator     , Matrix      > { typedef Matrix ResultType; };
template <> struct ConcatenateRules<Matrix      , Rotator     > { typedef Matrix ResultType; };
template <> struct ConcatenateRules<Quat        , Matrix      > { typedef Matrix ResultType; };
template <> struct ConcatenateRules<Matrix      , Quat        > { typedef Matrix ResultType; };

//////////////////////////////////////////////////////////////////////////
// Concatenate overloads.
//
// Since these are existing FUN types, we cannot rely on the default
// template that calls member functions. Instead, we provide direct overloads.
//////////////////////////////////////////////////////////////////////////

/**
 * Specialization for concatenating two Matrices.
 *
 * \param lhs - rotation that goes from space A to space B
 * \param rhs - rotation that goes from space B to space C.
 *
 * \return a new rotation representing the transformation from the input space of lhs to the output space of rhs.
 */
inline Matrix Concatenate(const Matrix& lhs, const Matrix& rhs)
{
  return lhs * rhs;
}

/**
 * Specialization for concatenating two rotations.
 *
 * NOTE: Quat concatenates right to left, opposite of how Matrix implements it.
 *       Confusing, no? That's why we have these high level functions!
 *
 * \param lhs - rotation that goes from space A to space B
 * \param rhs - rotation that goes from space B to space C.
 *
 * \return a new rotation representing the transformation from the input space of lhs to the output space of rhs.
 */
inline Quat Concatenate(const Quat& lhs, const Quat& rhs)
{
  return rhs * lhs;
}

/**
 * Specialization for concatenating two rotations.
 *
 * \param lhs - rotation that goes from space A to space B
 * \param rhs - rotation that goes from space B to space C.
 *
 * \return a new rotation representing the transformation from the input space of lhs to the output space of rhs.
 */
inline Rotator Concatenate(const Rotator& lhs, const Rotator& rhs)
{
  //@todo implement a more efficient way to do this.
  return TransformCast<Rotator>(Concatenate(TransformCast<Matrix>(lhs), TransformCast<Matrix>(rhs)));
}

/**
 * Specialization for concatenating two translations.
 *
 * \param lhs - translation that goes from space A to space B
 * \param rhs - translation that goes from space B to space C.
 *
 * \return a new translation representing the transformation from the input space of lhs to the output space of rhs.
 */
inline Vector Concatenate(const Vector& lhs, const Vector& rhs)
{
  return lhs + rhs;
}


//////////////////////////////////////////////////////////////////////////
// Inverse overloads.
//
// Since these are existing FUN types, we cannot rely on the default
// template that calls member functions. Instead, we provide direct overloads.
//////////////////////////////////////////////////////////////////////////

/**
 * Inverts a transform from space A to space B so it transforms from space B to space A.
 * Specialization for Matrix.
 *
 * \param transform - Input transform from space A to space B.
 *
 * \return Inverted transform from space B to space A.
 */
inline Matrix Inverse(const Matrix& transform)
{
  return transform.Inverse();
}

/**
 * Inverts a transform from space A to space B so it transforms from space B to space A.
 * Specialization for Rotator.
 *
 * \param transform - Input transform from space A to space B.
 *
 * \return Inverted transform from space B to space A.
 */
inline Rotator Inverse(const Rotator& transform)
{
  Vector EulerAngles = transform.euler();
  return Rotator::MakeFromEuler(Vector(-EulerAngles.z, -EulerAngles.y, -EulerAngles.x));
}

/**
 * Inverts a transform from space A to space B so it transforms from space B to space A.
 * Specialization for Quat.
 *
 * \param transform - Input transform from space A to space B.
 *
 * \return Inverted transform from space B to space A.
 */
inline Quat Inverse(const Quat& transform)
{
  return transform.Inverse();
}

/**
 * Inverts a transform from space A to space B so it transforms from space B to space A.
 * Specialization for translation.
 *
 * \param transform - Input transform from space A to space B.
 *
 * \return Inverted transform from space B to space A.
 */
inline Vector Inverse(const Vector& transform)
{
  return -transform;
}


//////////////////////////////////////////////////////////////////////////
// TransformPoint overloads.
//
// Since these are existing FUN types, we cannot rely on the default
// template that calls member functions. Instead, we provide direct overloads.
//////////////////////////////////////////////////////////////////////////

/**
 * Specialization for Matrix as it's member function is called something slightly different.
 */
inline Vector TransformPoint(const Matrix& transform, const Vector& point)
{
  return transform.TransformPosition(point);
}

/**
 * Specialization for Quat as it's member function is called something slightly different.
 */
inline Vector TransformPoint(const Quat& transform, const Vector& point)
{
  return transform.RotateVector(point);
}

/**
 * Specialization for Quat as it's member function is called something slightly different.
 */
inline Vector TransformVector(const Quat& transform, const Vector& vector)
{
  return transform.RotateVector(vector);
}

/**
 * Specialization for Rotator as it's member function is called something slightly different.
 */
inline Vector TransformPoint(const Rotator& transform, const Vector& point)
{
  return transform.RotateVector(point);
}

/**
 * Specialization for Rotator as it's member function is called something slightly different.
 */
inline Vector TransformVector(const Rotator& transform, const Vector& vector)
{
  return transform.RotateVector(vector);
}

/**
 * Specialization for Vector translation.
 */
inline Vector TransformPoint(const Vector& transform, const Vector& point)
{
  return transform + point;
}

/**
 * Specialization for Vector translation (does nothing).
 */
inline const Vector& TransformVector(const Vector& transform, const Vector& vector)
{
  return vector;
}

/**
 * Specialization for Scale.
 */
inline Vector TransformPoint(const CScale& transform, const Vector& point)
{
  return transform.GetVector() * point;
}

/**
 * Specialization for Scale.
 */
inline Vector TransformVector(const CScale& transform, const Vector& vector)
{
  return transform.GetVector() * vector;
}

} // namespace fun
