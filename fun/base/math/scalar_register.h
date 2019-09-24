#pragma once

/**
 * This define controls whether a scalar implementation or vector implementation is used for Transform.
 * The vector implementation works even when using UnMathFPU, but it will be much slower than the equivalent
 * scalar implementation, so the scalar code is maintained and enabled when vector intrinsics are off.
 */
//Currently disabled because FBoneAtom became Transform and we want to iterate quickly on it.
#define ENABLE_VECTORIZED_FBONEATOM  0 //FUN_PLATFORM_ENABLE_VECTORINTRINSICS && !__ARM_NEON__ && 1 // currently no support for VectorPermute needed for the boneatoms
#define ENABLE_VECTORIZED_TRANSFORM  FUN_PLATFORM_ENABLE_VECTORINTRINSICS

#if ENABLE_VECTORIZED_FBONEATOM || ENABLE_VECTORIZED_TRANSFORM

namespace fun {

/**
 * The ScalarRegister class wraps the concept of a 'float-in-vector', allowing common scalar operations like bone
 * weight calculations to be done in vector registers.  This will avoid some lhs hazards that arise when mixing float
 * and vector math on some platforms.  However, doing the math for four elements is slower if the vector operations are
 * being emulated on a scalar FPU, so ScalarRegister is defined to float when ENABLE_VECTORIZED_FBONEATOM == 0.
 */
class ScalarRegister {
 public:
  VectorRegister value;

  /** default constructor */
  FUN_ALWAYS_INLINE ScalarRegister();

  /** Copy Constructor */
  FUN_ALWAYS_INLINE ScalarRegister(const ScalarRegister& vector_value);

  /** Constructor using float value */
  explicit FUN_ALWAYS_INLINE ScalarRegister(const float& scalar_value);

  /**
   * Constructor
   *
   * \param vector_value - float4 vector register type
   */
  explicit FUN_ALWAYS_INLINE ScalarRegister(VectorRegister vector_value);

  /**
   * Gets the result of multiplying a scalar register to this.
   *
   * \param other - The scalar register to multiply this by.
   *
   * \return The result of multiplication.
   */
  FUN_ALWAYS_INLINE ScalarRegister operator * (const ScalarRegister& other) const;

  /**
   * Gets the result of adding a scalar register to this.
   *
   * \param other - The scalar register to add.
   *
   * \return The result of addition.
   */
  FUN_ALWAYS_INLINE ScalarRegister operator + (const ScalarRegister& other) const;

  /**
   * adds to this scalar register.
   *
   * \param other - The scalar register to add to this.
   *
   * \return Reference to this after addition.
   */
  FUN_ALWAYS_INLINE ScalarRegister& operator += (const ScalarRegister& other);

  /**
   * subtracts another scalar register from this.
   *
   * \param other The other scalar register.
   *
   * \return reference to this after subtraction.
   */
  FUN_ALWAYS_INLINE ScalarRegister& operator -= (const ScalarRegister& other);

  /**
   * Gets the result of subtracting a scalar register to this.
   *
   * \param other - The scalar register to subtract.
   *
   * \return The result of subtraction.
   */
  FUN_ALWAYS_INLINE ScalarRegister operator - (const ScalarRegister& other) const;

  /**
   * assignment operator
   *
   * \param other - a ScalarRegister
   */
  FUN_ALWAYS_INLINE ScalarRegister& operator = (const ScalarRegister& other);

  /**
   * assignment operator
   *
   * \param other - a VectorRegister
   */
  FUN_ALWAYS_INLINE ScalarRegister& operator = (const VectorRegister& other);

  /**
   * ScalarRegister to VectorRegister conversion operator.
   */
  FUN_ALWAYS_INLINE operator VectorRegister() const;
};


//
// inlines
//

FUN_ALWAYS_INLINE ScalarRegister::ScalarRegister() {
  // NOOP
}

FUN_ALWAYS_INLINE ScalarRegister::ScalarRegister(const ScalarRegister& vector_value) {
  value = vector_value.value;
}

FUN_ALWAYS_INLINE ScalarRegister::ScalarRegister(const float& scalar_value) {
  value = VectorLoadFloat1(&scalar_value);
}

FUN_ALWAYS_INLINE ScalarRegister::ScalarRegister(VectorRegister vector_value) {
  value = vector_value;
}

FUN_ALWAYS_INLINE ScalarRegister ScalarRegister::operator * (const ScalarRegister& other) const {
  return ScalarRegister(VectorMultiply(value, other.value));
}

FUN_ALWAYS_INLINE ScalarRegister ScalarRegister::operator + (const ScalarRegister& other) const {
  return ScalarRegister(VectorAdd(value, other.value));
}

FUN_ALWAYS_INLINE ScalarRegister& ScalarRegister::operator += (const ScalarRegister& other) {
  value = VectorAdd(value, other.value);
  return *this;
}

FUN_ALWAYS_INLINE ScalarRegister& ScalarRegister::operator -= (const ScalarRegister& other) {
  value = VectorSubtract(value, other.value);
  return *this;
}

FUN_ALWAYS_INLINE ScalarRegister ScalarRegister::operator - (const ScalarRegister& other) const {
  return ScalarRegister(VectorSubtract(value, other.value));
}

FUN_ALWAYS_INLINE ScalarRegister& ScalarRegister::operator = (const ScalarRegister& other) {
  value = other.value;
  return *this;
}

FUN_ALWAYS_INLINE ScalarRegister& ScalarRegister::operator = (const VectorRegister& other) {
  value = other;
  return *this;
}

FUN_ALWAYS_INLINE ScalarRegister::operator VectorRegister() const {
  return value;
}

#define ScalarOne   (ScalarRegister)ScalarRegister(VectorOne())
#define ScalarZero  (ScalarRegister)ScalarRegister(VectorZero())


//
// ScalarRegister specialization of templates.
//

/** Returns the smaller of the two values */
FUN_ALWAYS_INLINE ScalarRegister ScalarMin(const ScalarRegister& a, const ScalarRegister& b) {
  return ScalarRegister(VectorMin(a.value, b.value));
}

/** Returns the larger of the two values */
FUN_ALWAYS_INLINE ScalarRegister ScalarMax(const ScalarRegister& a, const ScalarRegister& b) {
  return ScalarRegister(VectorMax(a.value, b.value));
}

// Specialization of Lerp template that works with scalar (float in vector) registers
template <> FUN_ALWAYS_INLINE ScalarRegister Math::Lerp(const ScalarRegister& a, const ScalarRegister& b, const ScalarRegister& alpha) {
  const VectorRegister delta = VectorSubtract(b.value, a.value);
  return ScalarRegister(VectorMultiplyAdd(alpha.value, delta, a.value));
}

/**
 * Computes the reciprocal of the scalar register (component-wise) and returns the result.
 *
 * \param a - 1st scalar
 *
 * \return ScalarRegister(1.f / a.x, 1.f / a.y, 1.f / a.z, 1.f / a.w)
 */
FUN_ALWAYS_INLINE ScalarRegister ScalarReciprocal(const ScalarRegister& a) {
  return ScalarRegister(VectorReciprocalAccurate(a.value));
}

/**
 * Returns zero if any element in a is greater than the corresponding element in the global AnimWeightThreshold.
 *
 * \param a - 1st source vector
 *
 * \return zero integer if (a.x > AnimWeightThreshold.x) || (a.y > AnimWeightThreshold.y) || (a.z > AnimWeightThreshold.z) || (a.w > AnimWeightThreshold.w), non-zero Otherwise
 */
#define NonZeroAnimWeight(a)  VectorAnyGreaterThan(a.value, VectorizeConstants::AnimWeightThreshold)

/**
 * Returns non-zero if any element in a is greater than the corresponding element in the global AnimWeightThreshold.
 *
 * \param a - 1st source vector
 *
 * \return Non-zero integer if (a.x > AnimWeightThreshold.x) || (a.y > AnimWeightThreshold.y) || (a.z > AnimWeightThreshold.z) || (a.w > AnimWeightThreshold.w)
 */
#define NonOneAnimWeight(a)  !VectorAnyGreaterThan(a.value, VectorSubtract(VectorOne(), VectorizeConstants::AnimWeightThreshold))

} // namespace fun

#else

#define ScalarRegister  float

#define ScalarOne   1.f
#define ScalarZero  0.f

#define ScalarMin  Min
#define ScalarMax  Max

#define ScalarReciprocal(a)  (1.f / (a))

#define NonZeroAnimWeight(a)  ((a) > ZERO_ANIMWEIGHT_THRESH)
#define NonOneAnimWeight(a)  ((a) < 1.f - ZERO_ANIMWEIGHT_THRESH)

#endif


/**
 * Include the current implementation of a Transform, depending on the vector processing mode
 */
#if ENABLE_VECTORIZED_FBONEATOM
#include "BoneAtomVectorized.h"
#elif ENABLE_VECTORIZED_TRANSFORM
#include "fun/base/math/transform_vertorized.h"
#else
#include "fun/base/math/transform.h"
#endif

#include "fun/base/math/matrix_inline.h"
